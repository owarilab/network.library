#include "agent_handlers.h"
#include "handler_common.h"
#include "../agent_core.h"
#include "qs_llama_module.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void agent_mark_pending_verification(AGENT_CONVERSATION* conv,
                                            const char* tool_name,
                                            const char* tool_args)
{
    char user_path[AGENT_PATH_MAX] = "";
    char resolved_path[AGENT_PATH_MAX] = "";
    char msg[AGENT_PATH_MAX + 128];

    if (conv == NULL || tool_name == NULL || tool_args == NULL) return;
    if (agent_tool_args_extract_path(tool_args, user_path, sizeof(user_path)) != 0) return;
    if (agent_resolve_workspace_path(user_path, resolved_path, sizeof(resolved_path)) != 0) return;

    conv->pending_verification = 1;
    strncpy(conv->pending_verification_path, resolved_path,
            sizeof(conv->pending_verification_path) - 1);
    conv->pending_verification_path[sizeof(conv->pending_verification_path) - 1] = '\0';
    strncpy(conv->last_mutation_tool, tool_name, sizeof(conv->last_mutation_tool) - 1);
    conv->last_mutation_tool[sizeof(conv->last_mutation_tool) - 1] = '\0';

    snprintf(msg, sizeof(msg),
        "Verify %s with file_read before final_answer.",
        user_path);
    agent_conversation_append_context(conv, "verification_required", msg);
}

static int agent_try_clear_pending_verification(AGENT_CONVERSATION* conv,
                                                const char* tool_name,
                                                const char* tool_args)
{
    char user_path[AGENT_PATH_MAX] = "";
    char resolved_path[AGENT_PATH_MAX] = "";
    char msg[AGENT_PATH_MAX + 96];

    if (conv == NULL || !conv->pending_verification) return 0;
    if (tool_name == NULL || strcmp(tool_name, "file_read") != 0) return 0;
    if (tool_args == NULL) return 0;
    if (agent_tool_args_extract_path(tool_args, user_path, sizeof(user_path)) != 0) return 0;
    if (agent_resolve_workspace_path(user_path, resolved_path, sizeof(resolved_path)) != 0) return 0;
    if (strcmp(resolved_path, conv->pending_verification_path) != 0) return 0;

    conv->pending_verification = 0;
    conv->pending_verification_path[0] = '\0';
    conv->last_mutation_tool[0] = '\0';

    snprintf(msg, sizeof(msg), "Verification read completed for %s.", user_path);
    agent_conversation_append_context(conv, "verification_complete", msg);
    return 1;
}

/*
 * POST /api/agent/run/stream
 *
 * SSE streaming version of /api/agent/run.
 * Emits server-sent events as the ReAct loop progresses:
 *
 *   event: summary
 *   data: {"iteration":1,"summary":"..."}
 *
 *   event: tool_call
 *   data: {"iteration":1,"tool":"file_list","args":{...}}
 *
 *   event: tool_result
 *   data: {"iteration":1,"tool":"file_list","result":{...}}
 *
 *   event: answer
 *   data: {"status":"completed","answer":"...","iterations":2,
 *          "conversation_id":"...","tool_use_file_list":1,"tool_use_file_read":1}
 *
 * Errors during the loop are emitted as:
 *   event: error
 *   data: {"message":"..."}
 */
int handler_run_stream(QS_EVENT_PARAMETER params)
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

    int max_iter = hc_extract_int(body, "max_iterations", AGENT_MAX_ITERATIONS);
    if (max_iter <= 0 || max_iter > AGENT_MAX_ITERATIONS)
        max_iter = AGENT_MAX_ITERATIONS;

    /* Open SSE stream before creating heavy resources */
    QS_LLM_HTTP_STREAM_CONTEXT stream;
    if (-1 == qs_llm_http_stream_open(params, &stream)) {
        return send_json_response(params, 500,
            "{\"error\":\"failed to open SSE stream\"}");
    }

    /* Create conversation */
    AGENT_CONVERSATION* conv = agent_conversation_create(query, max_iter);
    if (!conv) {
        qs_llm_http_stream_send_event(&stream, "error",
            "{\"message\":\"failed to create conversation\"}");
        qs_llm_http_stream_send_done(&stream);
        qs_llm_http_stream_close(&stream);
        return 0;
    }

    agent_conversation_append_context(conv, "user_query", query);

    const char* status          = "error";
    char        answer[16384]   = "(no answer)";
    int         done            = 0;

    /* ReAct loop */
    while (!done && conv->iteration < conv->max_iterations) {

        printf("[RunStream] iteration %d/%d query=%.60s...\n",
               conv->iteration + 1, conv->max_iterations, conv->user_query);
        fflush(stdout);

        /* ---- Think step ---- */
        char* prompt = agent_build_think_prompt_from_conversation(conv);
        if (!prompt) {
            qs_llm_http_stream_send_event(&stream, "error",
                "{\"message\":\"prompt build failed\"}");
            break;
        }

        LLM_BUFFER llm_buf;
        if (llm_buffer_init(&llm_buf, 1024 * 64) != 0) {
            free(prompt);
            qs_llm_http_stream_send_event(&stream, "error",
                "{\"message\":\"out of memory\"}");
            break;
        }

        int llm_ret = qs_llama_module_stream_text(
            prompt, llm_token_callback, &llm_buf);
        free(prompt);

        if (llm_ret != 0 || !llm_buf.buf || !llm_buf.buf[0]) {
            llm_buffer_free(&llm_buf);
            qs_llm_http_stream_send_event(&stream, "error",
                "{\"message\":\"LLM inference failed\"}");
            break;
        }

        AGENT_THINK_RESULT think_result;
        int parse_ret = agent_parse_think_result(llm_buf.buf, &think_result);
        llm_buffer_free(&llm_buf);

        conv->iteration++;

        if (parse_ret != 0) {
            /* Parse failure: emit a summary event so the client can see it */
            char ev[128];
            snprintf(ev, sizeof(ev),
                "{\"iteration\":%d,\"summary\":\"(parse error, retrying)\"}",
                conv->iteration);
            qs_llm_http_stream_send_event(&stream, "summary", ev);
            agent_conversation_append_context(conv, "parse_error",
                "LLM response could not be parsed as valid action JSON.");
            continue;
        }

        {
            char validation_error[256] = "";
            if (agent_validate_think_result(&think_result,
                                            validation_error,
                                            sizeof(validation_error)) != 0) {
                char* esc_error = hc_json_escape(
                    validation_error[0] ? validation_error : "invalid model action");
                if (esc_error) {
                    size_t ev_size = strlen(esc_error) + 64;
                    char* ev = (char*)malloc(ev_size);
                    if (ev) {
                        snprintf(ev, ev_size,
                            "{\"iteration\":%d,\"summary\":\"%s\"}",
                            conv->iteration, esc_error);
                        qs_llm_http_stream_send_event(&stream, "summary", ev);
                        free(ev);
                    }
                    free(esc_error);
                }
                agent_conversation_append_context(conv, "validation_error",
                    validation_error[0] ? validation_error : "invalid model action");
                continue;
            }
        }

        /* ---- Emit summary event ---- */
        if (think_result.summary[0] != '\0') {
            agent_conversation_append_context(conv, "summary", think_result.summary);

            char* esc_summary = hc_json_escape(think_result.summary);
            if (esc_summary) {
                size_t ev_size = strlen(esc_summary) + 64;
                char*  ev      = (char*)malloc(ev_size);
                if (ev) {
                    snprintf(ev, ev_size,
                        "{\"iteration\":%d,\"summary\":\"%s\"}",
                        conv->iteration, esc_summary);
                    qs_llm_http_stream_send_event(&stream, "summary", ev);
                    free(ev);
                }
                free(esc_summary);
            }
        }

        /* ---- Final answer ---- */
        if (think_result.action == AGENT_ACTION_FINAL_ANSWER) {
            if (conv->pending_verification) {
                char* esc_msg;
                size_t ev_size;
                char* ev;
                char msg[AGENT_PATH_MAX + 160];

                snprintf(msg, sizeof(msg),
                    "final_answer rejected: verify %s with file_read first.",
                    conv->pending_verification_path[0] ? conv->pending_verification_path : "the last mutated file");
                agent_conversation_append_context(conv, "policy_violation", msg);

                esc_msg = hc_json_escape(msg);
                if (esc_msg) {
                    ev_size = strlen(esc_msg) + 64;
                    ev = (char*)malloc(ev_size);
                    if (ev) {
                        snprintf(ev, ev_size,
                            "{\"iteration\":%d,\"summary\":\"%s\"}",
                            conv->iteration, esc_msg);
                        qs_llm_http_stream_send_event(&stream, "summary", ev);
                        free(ev);
                    }
                    free(esc_msg);
                }
                continue;
            }
            agent_conversation_append_item(conv,
                                           AGENT_ITEM_FINAL_ANSWER,
                                           "final_answer",
                                           think_result.answer);
            strncpy(answer, think_result.answer, sizeof(answer) - 1);
            answer[sizeof(answer) - 1] = '\0';
            status = "completed";
            done   = 1;
            break;
        }

        /* ---- Tool call ---- */
        if (think_result.action == AGENT_ACTION_USE_TOOL) {
            const char* tool_name = think_result.tool_call.tool_name;
            const char* tool_args = think_result.tool_call.json_args;
            if (!tool_args || !tool_args[0]) tool_args = "{}";

            /* Emit tool_call event */
            {
                size_t ev_size = strlen(tool_args) + 128;
                char*  ev      = (char*)malloc(ev_size);
                if (ev) {
                    snprintf(ev, ev_size,
                        "{\"iteration\":%d,\"tool\":\"%s\",\"args\":%s}",
                        conv->iteration, tool_name, tool_args);
                    qs_llm_http_stream_send_event(&stream, "tool_call", ev);
                    free(ev);
                }
            }

            /* Execute tool */
            size_t out_size = 1024 * 512;
            char*  out_buf  = (char*)malloc(out_size);
            if (!out_buf) {
                qs_llm_http_stream_send_event(&stream, "error",
                    "{\"message\":\"out of memory (tool result buffer)\"}");
                break;
            }
            out_buf[0] = '\0';

            {
                int tool_ret = agent_tool_execute(tool_name, tool_args, out_buf, out_size);

                if (tool_ret == 0 && agent_tool_requires_verification(tool_name)) {
                    agent_mark_pending_verification(conv, tool_name, tool_args);
                }
                if (tool_ret == 0) {
                    agent_try_clear_pending_verification(conv, tool_name, tool_args);
                }
            }
            agent_conversation_count_tool(conv, tool_name);

            /* Emit tool_result event */
            {
                size_t ev_size = strlen(out_buf) + 128;
                char*  ev      = (char*)malloc(ev_size);
                if (ev) {
                    snprintf(ev, ev_size,
                        "{\"iteration\":%d,\"tool\":\"%s\",\"result\":%s}",
                        conv->iteration, tool_name, out_buf);
                    qs_llm_http_stream_send_event(&stream, "tool_result", ev);
                    free(ev);
                }
            }

            char call_label[128];
            snprintf(call_label, sizeof(call_label), "tool_call(%s)", tool_name);
            agent_conversation_append_context(conv, call_label, tool_args);
            agent_conversation_append_context(conv, "tool_result", out_buf);
            free(out_buf);
            continue;
        }

        /* Unknown action */
        agent_conversation_append_context(conv, "unknown_action",
            "LLM returned unknown action; retrying.");
    }

    if (!done && strcmp(status, "error") != 0)
        status = "max_iterations_reached";

    /* ---- Emit final answer event ---- */
    {
        char* esc_answer = hc_json_escape(answer);
        char* esc_convid = hc_json_escape(conv->conversation_id);

        size_t ev_size = 256
            + strlen(esc_answer ? esc_answer : "")
            + strlen(esc_convid ? esc_convid : "");
        char* ev = (char*)malloc(ev_size);
        if (ev) {
            snprintf(ev, ev_size,
                "{\"status\":\"%s\","
                "\"answer\":\"%s\","
                "\"conversation_id\":\"%s\","
                "\"iterations\":%d,"
                "\"tool_use_file_list\":%d,"
                "\"tool_use_file_read\":%d}",
                status,
                esc_answer ? esc_answer : "",
                esc_convid ? esc_convid : "",
                conv->iteration,
                conv->tool_use_file_list,
                conv->tool_use_file_read);
            qs_llm_http_stream_send_event(&stream, "answer", ev);
            free(ev);
        }

        if (esc_answer) free(esc_answer);
        if (esc_convid) free(esc_convid);
    }

    qs_llm_http_stream_send_done(&stream);
    qs_llm_http_stream_close(&stream);
    agent_conversation_destroy(conv);
    return 200;
}
