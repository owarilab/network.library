#include "agent_handlers.h"
#include "handler_common.h"
#include "../agent_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void loop_mark_pending_verification(int* pending_verification,
                                           char* pending_verification_path,
                                           size_t pending_verification_path_size,
                                           const char* tool_name,
                                           const char* tool_args)
{
    char user_path[AGENT_PATH_MAX] = "";
    char resolved_path[AGENT_PATH_MAX] = "";

    if (!pending_verification || !pending_verification_path || !tool_name || !tool_args) {
        return;
    }
    if (!agent_tool_requires_verification(tool_name)) return;
    if (agent_tool_args_extract_path(tool_args, user_path, sizeof(user_path)) != 0) return;
    if (agent_resolve_workspace_path(user_path, resolved_path, sizeof(resolved_path)) != 0) return;

    *pending_verification = 1;
    strncpy(pending_verification_path, resolved_path, pending_verification_path_size - 1);
    pending_verification_path[pending_verification_path_size - 1] = '\0';
}

static void loop_try_clear_pending_verification(int* pending_verification,
                                                char* pending_verification_path,
                                                const char* tool_name,
                                                const char* tool_args)
{
    char user_path[AGENT_PATH_MAX] = "";
    char resolved_path[AGENT_PATH_MAX] = "";

    if (!pending_verification || !pending_verification_path || !*pending_verification) return;
    if (!tool_name || strcmp(tool_name, "file_read") != 0) return;
    if (!tool_args) return;
    if (agent_tool_args_extract_path(tool_args, user_path, sizeof(user_path)) != 0) return;
    if (agent_resolve_workspace_path(user_path, resolved_path, sizeof(resolved_path)) != 0) return;
    if (strcmp(resolved_path, pending_verification_path) != 0) return;

    *pending_verification = 0;
    pending_verification_path[0] = '\0';
}

/*
 * POST /api/agent/loop
 * Input:  {"query":"...", "context":"...",
 *           "iteration":N, "max_iterations":M,
 *           "pending_verification":0|1,
 *           "pending_verification_path":"..."}
 * Output (tool use):   {"status":"continue", "action":"use_tool",
 *                        "summary":"...", "tool_name":"...",
 *                        "tool_result":{...}, "context":"...",
 *                        "iteration":N+1,
 *                        "pending_verification":0|1,
 *                        "pending_verification_path":"..."}
 * Output (done):       {"status":"done", "action":"final_answer",
 *                        "summary":"...", "answer":"...",
 *                        "iteration":N+1}
 * Output (limit):      {"status":"max_iterations_reached", "iteration":N}
 *
 * Stateless: one think+execute cycle.  Client manages context.
 */
int handler_loop(QS_EVENT_PARAMETER params)
{
    const char* body = api_qs_get_http_post_body(params);
    if (!body || !body[0]) {
        return send_json_response(params, 400,
            "{\"error\":\"request body required\"}");
    }

    char query[2048] = "";
    int  iteration     = hc_extract_int(body, "iteration",     0);
    int  max_iter      = hc_extract_int(body, "max_iterations", AGENT_MAX_ITERATIONS);
    int  pending_verification = hc_extract_int(body, "pending_verification", 0) ? 1 : 0;
    char pending_verification_path[AGENT_PATH_MAX] = "";

    hc_extract_str(body, "query", query, sizeof(query));
    hc_extract_str(body, "pending_verification_path",
                   pending_verification_path,
                   sizeof(pending_verification_path));
    if (!query[0]) {
        return send_json_response(params, 400, "{\"error\":\"query is required\"}");
    }
    if (max_iter <= 0 || max_iter > AGENT_MAX_ITERATIONS)
        max_iter = AGENT_MAX_ITERATIONS;

    /* Check iteration limit */
    if (iteration >= max_iter) {
        char resp[AGENT_PATH_MAX + 160];
        char* esc_pending_path = hc_json_escape(pending_verification_path);
        snprintf(resp, sizeof(resp),
            "{\"status\":\"max_iterations_reached\",\"iteration\":%d,"
            "\"pending_verification\":%d,\"pending_verification_path\":\"%s\"}",
            iteration,
            pending_verification,
            esc_pending_path ? esc_pending_path : "");
        if (esc_pending_path) free(esc_pending_path);
        return send_json_response(params, 200, resp);
    }

    /* Context: heap-allocated */
    char* context = (char*)calloc(1, AGENT_CONTEXT_INIT_SIZE);
    if (!context) {
        return send_json_response(params, 500, "{\"error\":\"out of memory\"}");
    }
    hc_extract_str(body, "context", context, AGENT_CONTEXT_INIT_SIZE);

    /* Think step */
    char* prompt = agent_build_think_prompt(query, context);
    if (!prompt) {
        free(context);
        return send_json_response(params, 500, "{\"error\":\"out of memory\"}");
    }

    LLM_BUFFER llm_buf;
    if (llm_buffer_init(&llm_buf, 1024 * 64) != 0) {
        free(prompt); free(context);
        return send_json_response(params, 500, "{\"error\":\"out of memory\"}");
    }

    int llm_ret = qs_llama_module_stream_text(prompt, llm_token_callback, &llm_buf);
    free(prompt);

    if (llm_ret != 0 || !llm_buf.buf || !llm_buf.buf[0]) {
        llm_buffer_free(&llm_buf);
        free(context);
        return send_json_response(params, 500,
            "{\"error\":\"LLM inference failed\"}");
    }

    AGENT_THINK_RESULT think_result;
    int parse_ret = agent_parse_think_result(llm_buf.buf, &think_result);
    llm_buffer_free(&llm_buf);

    if (parse_ret != 0) {
        free(context);
        return send_json_response(params, 500,
            "{\"error\":\"failed to parse LLM response\"}");
    }

    {
        char validation_error[256] = "";
        if (agent_validate_think_result(&think_result,
                                        validation_error,
                                        sizeof(validation_error)) != 0) {
            free(context);
            char err_body[384];
            snprintf(err_body, sizeof(err_body),
                "{\"error\":\"invalid model action: %s\"}",
                validation_error[0] ? validation_error : "validation failed");
            return send_json_response(params, 500, err_body);
        }
    }

    iteration++;

    /* Final answer path */
    if (think_result.action == AGENT_ACTION_FINAL_ANSWER) {
        if (pending_verification) {
            const char* blocked_path = pending_verification_path[0]
                ? pending_verification_path : "the last mutated file";
            size_t warning_len = strlen(blocked_path) + 64;
            char* warning = (char*)malloc(warning_len);
            char* new_context;
            char* esc_summary;
            char* esc_warning;
            char* esc_new_ctx;
            char* esc_pending_path;
            char* resp;
            size_t new_ctx_size;
            size_t resp_size;

            if (!warning) {
                free(context);
                return send_json_response(params, 500, "{\"error\":\"out of memory\"}");
            }
            snprintf(warning, warning_len,
                "final_answer rejected: verify %s with file_read first.",
                blocked_path);

            new_ctx_size = strlen(context) + strlen("[policy_violation]\n\n") + strlen(warning) + 1;
            new_context = (char*)malloc(new_ctx_size);
            if (!new_context) {
                free(warning);
                free(context);
                return send_json_response(params, 500, "{\"error\":\"out of memory\"}");
            }
            snprintf(new_context, new_ctx_size,
                "%s[policy_violation]\n%s\n\n",
                context,
                warning);

            esc_summary = hc_json_escape(think_result.summary);
            esc_warning = hc_json_escape(warning);
            esc_new_ctx = hc_json_escape(new_context);
            esc_pending_path = hc_json_escape(pending_verification_path);

            resp_size = strlen(esc_summary ? esc_summary : "")
                + strlen(esc_warning ? esc_warning : "")
                + strlen(esc_new_ctx ? esc_new_ctx : "")
                + strlen(esc_pending_path ? esc_pending_path : "")
                + 256;
            resp = (char*)malloc(resp_size);
            if (!resp) {
                if (esc_summary) free(esc_summary);
                if (esc_warning) free(esc_warning);
                if (esc_new_ctx) free(esc_new_ctx);
                if (esc_pending_path) free(esc_pending_path);
                free(new_context);
                free(warning);
                free(context);
                return send_json_response(params, 500, "{\"error\":\"out of memory\"}");
            }

            snprintf(resp, resp_size,
                "{\"status\":\"continue\",\"action\":\"final_answer\","
                "\"summary\":\"%s\","
                "\"warning\":\"%s\","
                "\"context\":\"%s\","
                "\"iteration\":%d,"
                "\"pending_verification\":1,"
                "\"pending_verification_path\":\"%s\"}",
                esc_summary ? esc_summary : "",
                esc_warning ? esc_warning : "",
                esc_new_ctx ? esc_new_ctx : "",
                iteration,
                esc_pending_path ? esc_pending_path : "");

            if (esc_summary) free(esc_summary);
            if (esc_warning) free(esc_warning);
            if (esc_new_ctx) free(esc_new_ctx);
            if (esc_pending_path) free(esc_pending_path);
            free(new_context);
            free(warning);
            free(context);
            send_json_response(params, 200, resp);
            free(resp);
            return 0;
        }

        free(context);
        char* esc_summary = hc_json_escape(think_result.summary);
        char* esc_answer  = hc_json_escape(think_result.answer);

        size_t sz = 256 +
            strlen(esc_summary ? esc_summary : "") +
            strlen(esc_answer  ? esc_answer  : "");
        char* resp = (char*)malloc(sz);
        if (!resp) {
            if (esc_summary) free(esc_summary);
            if (esc_answer)  free(esc_answer);
            return send_json_response(params, 500, "{\"error\":\"out of memory\"}");
        }
        snprintf(resp, sz,
            "{\"status\":\"done\",\"action\":\"final_answer\","
            "\"summary\":\"%s\",\"answer\":\"%s\",\"iteration\":%d}",
            esc_summary ? esc_summary : "",
            esc_answer  ? esc_answer  : "",
            iteration);
        if (esc_summary) free(esc_summary);
        if (esc_answer)  free(esc_answer);
        send_json_response(params, 200, resp);
        free(resp);
        return 0;
    }

    /* Tool use path */
    size_t tool_result_size = 1024 * 512;
    char*  tool_result_buf  = (char*)malloc(tool_result_size);
    if (!tool_result_buf) {
        free(context);
        return send_json_response(params, 500, "{\"error\":\"out of memory\"}");
    }
    tool_result_buf[0] = '\0';

    const char* tool_args = think_result.tool_call.json_args;
    if (!tool_args || !tool_args[0]) tool_args = "{}";

    {
        int tool_ret = agent_tool_execute(think_result.tool_call.tool_name, tool_args,
                                          tool_result_buf, tool_result_size);
        if (tool_ret == 0) {
            loop_mark_pending_verification(&pending_verification,
                                           pending_verification_path,
                                           sizeof(pending_verification_path),
                                           think_result.tool_call.tool_name,
                                           tool_args);
            loop_try_clear_pending_verification(&pending_verification,
                                                pending_verification_path,
                                                think_result.tool_call.tool_name,
                                                tool_args);
        }
    }

    /* Append this iteration to the context string */
    size_t new_ctx_size = strlen(context) + strlen(think_result.tool_call.tool_name)
        + strlen(tool_args) + strlen(tool_result_buf) + 64;
    char*  new_context  = (char*)calloc(1, new_ctx_size);
    if (!new_context) {
        free(context); free(tool_result_buf);
        return send_json_response(params, 500, "{\"error\":\"out of memory\"}");
    }

    snprintf(new_context, new_ctx_size,
        "%s"
        "[tool_call:%d]\n%s(%s)\n\n"
        "[tool_result:%d]\n%s\n\n",
        context,
        iteration, think_result.tool_call.tool_name, tool_args,
        iteration, tool_result_buf);

    /* Escape the new context and tool result for JSON */
    char* esc_summary  = hc_json_escape(think_result.summary);
    char* esc_new_ctx  = hc_json_escape(new_context);
    char* esc_pending_path = hc_json_escape(pending_verification_path);

    size_t resp_size = 512 +
        strlen(esc_summary  ? esc_summary  : "") +
        strlen(tool_result_buf) +
        strlen(esc_new_ctx  ? esc_new_ctx  : "") +
        strlen(esc_pending_path ? esc_pending_path : "");
    char* resp = (char*)malloc(resp_size);

    if (!resp) {
        if (esc_summary) free(esc_summary);
        if (esc_new_ctx) free(esc_new_ctx);
        if (esc_pending_path) free(esc_pending_path);
        free(context); free(tool_result_buf); free(new_context);
        return send_json_response(params, 500, "{\"error\":\"out of memory\"}");
    }

    snprintf(resp, resp_size,
        "{\"status\":\"continue\",\"action\":\"use_tool\","
        "\"summary\":\"%s\","
        "\"tool_name\":\"%s\","
        "\"tool_result\":%s,"
        "\"context\":\"%s\","
        "\"iteration\":%d,"
        "\"pending_verification\":%d,"
        "\"pending_verification_path\":\"%s\"}",
        esc_summary ? esc_summary : "",
        think_result.tool_call.tool_name,
        tool_result_buf,
        esc_new_ctx ? esc_new_ctx : "",
        iteration,
        pending_verification,
        esc_pending_path ? esc_pending_path : "");

    if (esc_summary) free(esc_summary);
    if (esc_new_ctx) free(esc_new_ctx);
    if (esc_pending_path) free(esc_pending_path);
    free(context);
    free(tool_result_buf);
    free(new_context);

    send_json_response(params, 200, resp);
    free(resp);
    return 0;
}
