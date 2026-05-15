#include "tool_url_whitelist_get.h"
#include "../agent_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* JSON-escape a string into dst. Returns 0 on success, -1 if too small. */
static int json_escape_str(const char* src, size_t src_len, char* dst, size_t dst_size)
{
    size_t i = 0;
    for (size_t si = 0; si < src_len && i < dst_size - 1; si++) {
        char c = src[si];
        if (c == '"')       { if (i + 2 > dst_size - 1) return -1; dst[i++] = '\\'; dst[i++] = '"'; }
        else if (c == '\\') { if (i + 2 > dst_size - 1) return -1; dst[i++] = '\\'; dst[i++] = '\\'; }
        else if (c == '\n') { if (i + 2 > dst_size - 1) return -1; dst[i++] = '\\'; dst[i++] = 'n'; }
        else if (c == '\r') { if (i + 2 > dst_size - 1) return -1; dst[i++] = '\\'; dst[i++] = 'r'; }
        else if (c == '\t') { if (i + 2 > dst_size - 1) return -1; dst[i++] = '\\'; dst[i++] = 't'; }
        else                { dst[i++] = c; }
    }
    dst[i] = '\0';
    return 0;
}

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
        json_escape_str(agent_get_whitelist_host(i), strlen(agent_get_whitelist_host(i)),
                        escaped_host, sizeof(escaped_host));

        const char* desc = agent_get_whitelist_desc(i);
        char escaped_desc[1024] = "";
        if (desc && desc[0]) {
            json_escape_str(desc, strlen(desc), escaped_desc, sizeof(escaped_desc));
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
