#include "tool_url_whitelist_get.h"
#include "../agent_core.h"
#include "qs_string.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int tool_url_whitelist_get_execute(const char* json_args, char* output, size_t output_size)
{
    (void)json_args;

    if (!output || output_size == 0) return -1;

    int count = agent_get_whitelist_count();

    /* Build JSON response */
    size_t buf_cap = 4096;
    char*  buf     = (char*)malloc(buf_cap);
    if (!buf) {
        snprintf(output, output_size, "{\"error\":\"out of memory\"}");
        return -1;
    }

    int pos = snprintf(buf, buf_cap, "{\"ok\":true,\"hosts\":[");
    for (int i = 0; i < count && (size_t)pos < (int)(buf_cap - 512); i++) {
        if (i > 0) pos += snprintf(buf + pos, buf_cap - (size_t)pos, ",");

        char escaped_host[2048];
        if (QS_SYSTEM_OK != qs_json_escape(agent_get_whitelist_host(i), escaped_host, sizeof(escaped_host))) {
            escaped_host[0] = '\0';
        }

        const char* desc = agent_get_whitelist_desc(i);
        char escaped_desc[1024] = "";
        if (desc && desc[0]) {
            if (QS_SYSTEM_OK != qs_json_escape(desc, escaped_desc, sizeof(escaped_desc))) {
                escaped_desc[0] = '\0';
            }
        }

        pos += snprintf(buf + pos, buf_cap - (size_t)pos,
                        "{\"host\":\"%s\",\"description\":\"%s\"}",
                        escaped_host, escaped_desc);
    }
    pos += snprintf(buf + pos, buf_cap - (size_t)pos, "],\"count\":%d}", count);

    int written = snprintf(output, output_size, "%s", buf);
    free(buf);
    return (written > 0 && (size_t)written < output_size) ? 0 : -1;
}
