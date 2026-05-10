#include "agent_handlers.h"
#include "handler_common.h"
#include "../agent_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * POST /api/agent/init
 * Input:  {"query":"...", "max_iterations": N}
 * Output: {"conversation_id":"conv-xxx", "status":"ready", "max_iterations":N}
 *
 * Stateless: generates an ID only.  The client tracks conversation state.
 */
int handler_init(QS_EVENT_PARAMETER params)
{
    const char* body = api_qs_get_http_post_body(params);

    char query[2048] = "";
    int  max_iter    = AGENT_MAX_ITERATIONS;

    if (body && body[0]) {
        hc_extract_str(body, "query", query, sizeof(query));
        int v = hc_extract_int(body, "max_iterations", AGENT_MAX_ITERATIONS);
        if (v > 0 && v <= AGENT_MAX_ITERATIONS) max_iter = v;
    }

    char conv_id[AGENT_CONV_ID_LEN];
    agent_conversation_generate_id(conv_id, sizeof(conv_id));

    char* esc_query = hc_json_escape(query);

    char resp_body[512 + 2048];
    snprintf(resp_body, sizeof(resp_body),
        "{\"conversation_id\":\"%s\","
        "\"status\":\"ready\","
        "\"max_iterations\":%d,"
        "\"query\":\"%s\"}",
        conv_id, max_iter,
        esc_query ? esc_query : "");

    if (esc_query) free(esc_query);
    return send_json_response(params, 200, resp_body);
}
