#include "agent_handlers.h"
#include "handler_common.h"
#include "../agent_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * POST /api/agent/execute
 * Input:  {"tool_name":"file_list", "tool_args":{"path":"./"}}
 * Output: {"status":"ok"|"error", "tool_name":"...", "result":{...}}
 *
 * Stateless tool executor.
 */
int handler_execute(QS_EVENT_PARAMETER params)
{
    const char* body = api_qs_get_http_post_body(params);
    if (!body || !body[0]) {
        return send_json_response(params, 400,
            "{\"error\":\"request body required\"}");
    }

    char tool_name[64]   = "";
    char tool_args[4096] = "{}";

    hc_extract_str(body, "tool_name", tool_name, sizeof(tool_name));
    /* tool_args may be a nested JSON object or a JSON string */
    if (!hc_extract_obj_raw(body, "tool_args", tool_args, sizeof(tool_args))) {
        hc_extract_str(body, "tool_args", tool_args, sizeof(tool_args));
        if (!tool_args[0]) {
            tool_args[0] = '{'; tool_args[1] = '}'; tool_args[2] = '\0';
        }
    }

    if (!tool_name[0]) {
        return send_json_response(params, 400,
            "{\"error\":\"tool_name is required\"}");
    }
    if (!agent_tool_is_registered(tool_name)) {
        char err_body[128];
        snprintf(err_body, sizeof(err_body),
            "{\"error\":\"unknown tool: %s\"}", tool_name);
        return send_json_response(params, 400, err_body);
    }

    /* Execute the tool */
    size_t result_size = 1024 * 1024; /* 1 MB */
    char*  result      = (char*)malloc(result_size);
    if (!result) {
        return send_json_response(params, 500, "{\"error\":\"out of memory\"}");
    }
    result[0] = '\0';

    int tool_ret = agent_tool_execute(tool_name, tool_args, result, result_size);

    /* Build response envelope */
    size_t resp_size = strlen(result) + 256;
    char*  resp_body = (char*)malloc(resp_size);
    if (!resp_body) {
        free(result);
        return send_json_response(params, 500, "{\"error\":\"out of memory\"}");
    }

    snprintf(resp_body, resp_size,
        "{\"status\":\"%s\",\"tool_name\":\"%s\",\"result\":%s}",
        tool_ret == 0 ? "ok" : "error",
        tool_name,
        result);

    send_json_response(params, 200, resp_body);
    free(result);
    free(resp_body);
    return 0;
}
