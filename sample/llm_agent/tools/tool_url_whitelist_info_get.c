#include "tool_url_whitelist_info_get.h"
#include "tool_common.h"
#include "../agent_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int tool_url_whitelist_info_get_execute(const char* json_args, char* output, size_t output_size)
{
    if (!output || output_size == 0) return -1;

    /* Parse "host" from args */
    char host[1024] = "";
    if (json_args && json_args[0]) {
        tool_json_extract_str(json_args, "host", host, sizeof(host));
    }

    if (!host[0]) {
        snprintf(output, output_size, "{\"error\":\"host is required\"}");
        return -1;
    }

    /* Parse optional endpoint_filter */
    char filter[256] = "";
    if (json_args && json_args[0]) {
        tool_json_extract_str(json_args, "endpoint_filter", filter, sizeof(filter));
    }

    /* Get the doc for this host */
    size_t doc_size = 0;
    char* doc = agent_get_api_doc_for_host(host, &doc_size);
    if (!doc) {
        snprintf(output, output_size,
                 "{\"error\":\"no documentation found for host '%s'\"}", host);
        return -1;
    }

    /* If filter specified, apply it */
    char* result = doc;
    size_t result_size = doc_size;
    if (filter[0]) {
        free(doc);
        result = agent_filter_api_docs(json_args, filter, &result_size);
        if (!result) {
            snprintf(output, output_size, "{\"error\":\"failed to filter results\"}");
            return -1;
        }
    }

    /* Build response: {"ok":true,"host":"...","endpoints":[...]} */
    size_t resp_cap = strlen(host) + result_size + 256;
    char*  resp     = (char*)malloc(resp_cap);
    if (!resp) { free(result); snprintf(output, output_size, "{\"error\":\"out of memory\"}"); return -1; }

    int written = snprintf(resp, resp_cap,
        "{\"ok\":true,\"host\":\"%s\",\"endpoints\":%s}", host, result);

    int ret = (written > 0 && (size_t)written < output_size) ? 0 : -1;
    snprintf(output, output_size, "%s", resp);

    free(resp);
    free(result);
    return ret;
}
