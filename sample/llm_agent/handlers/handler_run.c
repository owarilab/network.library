#include "agent_handlers.h"
#include "handler_common.h"
#include "../agent_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * POST /api/agent/run
 * Input:  {"query":"...", "max_iterations": N}
 * Output: {"status":"completed"|"max_iterations_reached"|"error",
 *           "answer":"...", "iterations":N,
 *           "tool_use_file_list":N, "tool_use_file_read":N}
 *
 * Stateful within the request: runs the full ReAct loop and returns
 * the final answer (or error) in a single HTTP response.
 */
int handler_run(QS_EVENT_PARAMETER params)
{
    const char* body = api_qs_get_http_post_body(params);
    if (!body || !body[0]) {
        return send_json_response(params, 400,
            "{\"error\":\"request body required\"}");
    }

    char query[2048] = "";
    hc_extract_str(body, "query", query, sizeof(query));
    if (!query[0]) {
        return send_json_response(params, 400, "{\"error\":\"query is required\"}");
    }

    int max_iter = hc_extract_int(body, "max_iterations", AGENT_MAX_ITERATIONS);
    if (max_iter <= 0 || max_iter > AGENT_MAX_ITERATIONS)
        max_iter = AGENT_MAX_ITERATIONS;

    /* Create conversation */
    AGENT_CONVERSATION* conv = agent_conversation_create(query, max_iter);
    if (!conv) {
        return send_json_response(params, 500,
            "{\"error\":\"failed to create conversation\"}");
    }

    /* Record the initial query in context */
    agent_conversation_append_context(conv, "user_query", query);

    const char* status    = "error";
    char        answer[16384] = "(no answer)";
    int         done      = 0;
    int         empty_answer_warned = 0;

    /* Accumulated thoughts across all iterations (newline-separated) */
    size_t thoughts_cap = 1024 * 16;
    size_t thoughts_len = 0;
    char*  thoughts_buf = (char*)calloc(1, thoughts_cap);
    if (!thoughts_buf) {
        agent_conversation_destroy(conv);
        return send_json_response(params, 500, "{\"error\":\"out of memory\"}");
    }

    /* ReAct loop */
    while (!done && conv->iteration < conv->max_iterations) {

        printf("[Run] iteration %d/%d query=%.60s...\n",
               conv->iteration + 1, conv->max_iterations, conv->user_query);
        fflush(stdout);

        /* ---- Think step ---- */
        char* prompt = agent_build_think_prompt(
            conv->user_query,
            conv->accumulated_context ? conv->accumulated_context : "");
        if (!prompt) {
            status = "error";
            break;
        }

        LLM_BUFFER llm_buf;
        if (llm_buffer_init(&llm_buf, 1024 * 64) != 0) {
            free(prompt);
            status = "error";
            break;
        }

        int llm_ret = qs_llama_module_stream_text(
            prompt, llm_token_callback, &llm_buf);
        free(prompt);
        size_t llm_out_len = llm_buf.buf ? strlen(llm_buf.buf) : 0;
        printf("[Run] LLM returned %d, output_len=%zu\n", llm_ret, llm_out_len);
        if (llm_buf.buf) {
            printf("[Run] LLM raw (first 500): %.500s\n", llm_buf.buf);
        }
        fflush(stdout);

        if (llm_ret != 0 || !llm_buf.buf || !llm_buf.buf[0]) {
            llm_buffer_free(&llm_buf);
            status = "error";
            break;
        }

        AGENT_THINK_RESULT think_result;
        int parse_ret = agent_parse_think_result(llm_buf.buf, &think_result);
        if (parse_ret == 0) {
            printf("[Run] parsed answer (first 200): %.200s\n", think_result.answer);
            fflush(stdout);
        }
        llm_buffer_free(&llm_buf);

        conv->iteration++;
        printf("[Run] parse_ret=%d action=%d tool=%s\n",
               parse_ret, think_result.action, think_result.tool_call.tool_name);
        fflush(stdout);

        if (parse_ret != 0) {
            /* Couldn't parse — count as a wasted iteration and continue */
            agent_conversation_append_context(conv, "parse_error",
                "LLM response could not be parsed as valid action JSON.");
            continue;
        }

        /* Append thought to context and accumulate for response */
        agent_conversation_append_context(conv, "thought", think_result.thought);
        if (think_result.thought[0] != '\0') {
            size_t tlen = strlen(think_result.thought);
            if (thoughts_len + tlen + 2 < thoughts_cap) {
                if (thoughts_len > 0) thoughts_buf[thoughts_len++] = '\n';
                memcpy(thoughts_buf + thoughts_len, think_result.thought, tlen);
                thoughts_len += tlen;
                thoughts_buf[thoughts_len] = '\0';
            }
        }

        if (think_result.action == AGENT_ACTION_FINAL_ANSWER) {
            if (think_result.answer[0] != '\0') {
                strncpy(answer, think_result.answer, sizeof(answer) - 1);
                answer[sizeof(answer) - 1] = '\0';
                status = "completed";
                done   = 1;
                break;
            } else if (!empty_answer_warned && think_result.thought[0] != '\0') {
                /* First time: remind LLM to fill answer field */
                empty_answer_warned = 1;
                agent_conversation_append_context(conv, "reminder",
                    "You output final_answer but the 'answer' field was empty. "
                    "Please output the complete answer in the 'answer' field.");
                continue;
            } else {
                /* Still empty after reminder — use thought as fallback */
                strncpy(answer, think_result.thought, sizeof(answer) - 1);
                answer[sizeof(answer) - 1] = '\0';
                status = "completed";
                done   = 1;
                break;
            }
        }

        if (think_result.action == AGENT_ACTION_USE_TOOL) {
            const char* tool_name = think_result.tool_call.tool_name;
            const char* tool_args = think_result.tool_call.json_args;
            if (!tool_args || !tool_args[0]) tool_args = "{}";

            /* ---- Execute tool ---- */
            size_t out_size = 1024 * 512;
            char*  out_buf  = (char*)malloc(out_size);
            if (!out_buf) { status = "error"; break; }
            out_buf[0] = '\0';

            agent_tool_execute(tool_name, tool_args, out_buf, out_size);
            agent_conversation_count_tool(conv, tool_name);

            /* Append to context */
            char call_label[128];
            snprintf(call_label, sizeof(call_label),
                "tool_call(%s)", tool_name);
            agent_conversation_append_context(conv, call_label, tool_args);
            agent_conversation_append_context(conv, "tool_result", out_buf);

            free(out_buf);
            continue;
        }

        /* Unknown action — record and continue */
        agent_conversation_append_context(conv, "unknown_action",
            "LLM returned unknown action; retrying.");
    }

    if (!done) {
        if (strcmp(status, "error") != 0)
            status = "max_iterations_reached";
    }

    /* Build response */
    char* esc_answer  = hc_json_escape(answer);
    char* esc_thought = hc_json_escape(thoughts_buf ? thoughts_buf : "");
    char* esc_convid  = hc_json_escape(conv->conversation_id);
    free(thoughts_buf);

    size_t resp_size = 512 +
        strlen(esc_answer  ? esc_answer  : "") +
        strlen(esc_thought ? esc_thought : "") +
        AGENT_CONV_ID_LEN;
    char* resp_body = (char*)malloc(resp_size);
    if (!resp_body) {
        if (esc_answer)  free(esc_answer);
        if (esc_thought) free(esc_thought);
        if (esc_convid)  free(esc_convid);
        agent_conversation_destroy(conv);
        return send_json_response(params, 500, "{\"error\":\"out of memory\"}");
    }

    snprintf(resp_body, resp_size,
        "{\"status\":\"%s\","
        "\"answer\":\"%s\","
        "\"thought\":\"%s\","
        "\"conversation_id\":\"%s\","
        "\"iterations\":%d,"
        "\"max_iterations\":%d,"
        "\"tool_use_file_list\":%d,"
        "\"tool_use_file_read\":%d}",
        status,
        esc_answer  ? esc_answer  : "",
        esc_thought ? esc_thought : "",
        esc_convid  ? esc_convid  : "",
        conv->iteration,
        conv->max_iterations,
        conv->tool_use_file_list,
        conv->tool_use_file_read);

    if (esc_answer)  free(esc_answer);
    if (esc_thought) free(esc_thought);
    if (esc_convid)  free(esc_convid);
    agent_conversation_destroy(conv);

    send_json_response(params, 200, resp_body);
    free(resp_body);
    return 0;
}
