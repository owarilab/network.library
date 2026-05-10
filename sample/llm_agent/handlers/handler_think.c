#include "agent_handlers.h"
#include "handler_common.h"
#include "../agent_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * POST /api/agent/think
 * Input:  {"query":"...", "context":"..."}
 * Output: {"action":"use_tool"|"final_answer",
 *           "thought":"...", "tool_name":"...",
 *           "tool_args":{...}, "answer":"..."}
 *
 * Stateless: calls LLM and parses the result.
 * The client is responsible for passing accumulated context.
 */
int handler_think(QS_EVENT_PARAMETER params)
{
    const char* body = api_qs_get_http_post_body(params);
    if (!body || !body[0]) {
        return send_json_response(params, 400,
            "{\"error\":\"request body required\"}");
    }

    char query[2048] = "";
    hc_extract_str(body, "query", query, sizeof(query));
    if (!query[0]) {
        return send_json_response(params, 400,
            "{\"error\":\"query is required\"}");
    }

    /* Context may be large — allocate on heap */
    char* context = (char*)calloc(1, AGENT_CONTEXT_INIT_SIZE);
    if (!context) {
        return send_json_response(params, 500, "{\"error\":\"out of memory\"}");
    }
    hc_extract_str(body, "context", context, AGENT_CONTEXT_INIT_SIZE);

    /* Build the think prompt */
    char* prompt = agent_build_think_prompt(query, context);
    free(context);
    if (!prompt) {
        return send_json_response(params, 500, "{\"error\":\"out of memory\"}");
    }

    /* Call LLM and accumulate tokens */
    LLM_BUFFER llm_buf;
    if (llm_buffer_init(&llm_buf, 1024 * 64) != 0) {
        free(prompt);
        return send_json_response(params, 500, "{\"error\":\"out of memory\"}");
    }

    printf("[Think] calling LLM (query=%.80s...)\n", query);
    fflush(stdout);
    int llm_ret = qs_llama_module_stream_text(prompt, llm_token_callback, &llm_buf);
    free(prompt);
    printf("[Think] LLM returned %d, output_len=%zu\n", llm_ret, llm_buf.buf ? strlen(llm_buf.buf) : 0);
    fflush(stdout);

    if (llm_ret != 0 || !llm_buf.buf || !llm_buf.buf[0]) {
        llm_buffer_free(&llm_buf);
        return send_json_response(params, 500,
            "{\"error\":\"LLM inference failed\"}");
    }

    /* Parse the LLM output */
    AGENT_THINK_RESULT think_result;
    int parse_ret = agent_parse_think_result(llm_buf.buf, &think_result);
    printf("[Think] parse_ret=%d action=%d tool=%s\n", parse_ret, think_result.action, think_result.tool_call.tool_name);
    fflush(stdout);
    llm_buffer_free(&llm_buf);

    if (parse_ret != 0) {
        return send_json_response(params, 500,
            "{\"error\":\"failed to parse LLM response\"}");
    }

    /* Map action enum to string */
    const char* action_str = "unknown";
    if (think_result.action == AGENT_ACTION_USE_TOOL)     action_str = "use_tool";
    if (think_result.action == AGENT_ACTION_FINAL_ANSWER) action_str = "final_answer";

    /* JSON-escape free-text fields */
    char* esc_thought = hc_json_escape(think_result.thought);
    char* esc_answer  = hc_json_escape(think_result.answer);

    const char* tool_args = think_result.tool_call.json_args;
    if (!tool_args || !tool_args[0]) tool_args = "{}";

    size_t resp_size = 256 + strlen(esc_thought ? esc_thought : "") +
                       strlen(esc_answer  ? esc_answer  : "") +
                       strlen(tool_args);
    char*  resp_body = (char*)malloc(resp_size);
    if (!resp_body) {
        if (esc_thought) free(esc_thought);
        if (esc_answer)  free(esc_answer);
        return send_json_response(params, 500, "{\"error\":\"out of memory\"}");
    }

    snprintf(resp_body, resp_size,
        "{\"action\":\"%s\","
        "\"thought\":\"%s\","
        "\"tool_name\":\"%s\","
        "\"tool_args\":%s,"
        "\"answer\":\"%s\"}",
        action_str,
        esc_thought ? esc_thought : "",
        think_result.tool_call.tool_name,
        tool_args,
        esc_answer  ? esc_answer  : "");

    if (esc_thought) free(esc_thought);
    if (esc_answer)  free(esc_answer);

    send_json_response(params, 200, resp_body);
    free(resp_body);
    return 0;
}
