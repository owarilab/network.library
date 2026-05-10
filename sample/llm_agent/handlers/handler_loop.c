#include "agent_handlers.h"
#include "handler_common.h"
#include "../agent_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * POST /api/agent/loop
 * Input:  {"query":"...", "context":"...",
 *           "iteration":N, "max_iterations":M}
 * Output (tool use):   {"status":"continue", "action":"use_tool",
 *                        "thought":"...", "tool_name":"...",
 *                        "tool_result":{...}, "context":"...",
 *                        "iteration":N+1}
 * Output (done):       {"status":"done", "action":"final_answer",
 *                        "thought":"...", "answer":"...",
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

    hc_extract_str(body, "query", query, sizeof(query));
    if (!query[0]) {
        return send_json_response(params, 400, "{\"error\":\"query is required\"}");
    }
    if (max_iter <= 0 || max_iter > AGENT_MAX_ITERATIONS)
        max_iter = AGENT_MAX_ITERATIONS;

    /* Check iteration limit */
    if (iteration >= max_iter) {
        char resp[128];
        snprintf(resp, sizeof(resp),
            "{\"status\":\"max_iterations_reached\",\"iteration\":%d}",
            iteration);
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

    iteration++;

    /* Final answer path */
    if (think_result.action == AGENT_ACTION_FINAL_ANSWER) {
        free(context);
        char* esc_thought = hc_json_escape(think_result.thought);
        char* esc_answer  = hc_json_escape(think_result.answer);

        size_t sz = 256 +
            strlen(esc_thought ? esc_thought : "") +
            strlen(esc_answer  ? esc_answer  : "");
        char* resp = (char*)malloc(sz);
        if (!resp) {
            if (esc_thought) free(esc_thought);
            if (esc_answer)  free(esc_answer);
            return send_json_response(params, 500, "{\"error\":\"out of memory\"}");
        }
        snprintf(resp, sz,
            "{\"status\":\"done\",\"action\":\"final_answer\","
            "\"thought\":\"%s\",\"answer\":\"%s\",\"iteration\":%d}",
            esc_thought ? esc_thought : "",
            esc_answer  ? esc_answer  : "",
            iteration);
        if (esc_thought) free(esc_thought);
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

    agent_tool_execute(think_result.tool_call.tool_name, tool_args,
                       tool_result_buf, tool_result_size);

    /* Append this iteration to the context string */
    size_t new_ctx_size = AGENT_CONTEXT_INIT_SIZE;
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
    char* esc_thought  = hc_json_escape(think_result.thought);
    char* esc_new_ctx  = hc_json_escape(new_context);

    size_t resp_size = 512 +
        strlen(esc_thought  ? esc_thought  : "") +
        strlen(tool_result_buf) +
        strlen(esc_new_ctx  ? esc_new_ctx  : "");
    char* resp = (char*)malloc(resp_size);

    if (!resp) {
        if (esc_thought) free(esc_thought);
        if (esc_new_ctx) free(esc_new_ctx);
        free(context); free(tool_result_buf); free(new_context);
        return send_json_response(params, 500, "{\"error\":\"out of memory\"}");
    }

    snprintf(resp, resp_size,
        "{\"status\":\"continue\",\"action\":\"use_tool\","
        "\"thought\":\"%s\","
        "\"tool_name\":\"%s\","
        "\"tool_result\":%s,"
        "\"context\":\"%s\","
        "\"iteration\":%d}",
        esc_thought ? esc_thought : "",
        think_result.tool_call.tool_name,
        tool_result_buf,
        esc_new_ctx ? esc_new_ctx : "",
        iteration);

    if (esc_thought) free(esc_thought);
    if (esc_new_ctx) free(esc_new_ctx);
    free(context);
    free(tool_result_buf);
    free(new_context);

    send_json_response(params, 200, resp);
    free(resp);
    return 0;
}
