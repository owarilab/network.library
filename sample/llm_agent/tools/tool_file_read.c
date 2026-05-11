#include "tool_file_read.h"
#include "tool_common.h"
#include "../agent_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int tool_file_read_execute(const char* json_args, char* output, size_t output_size)
{
    if (!output || output_size == 0) return -1;

    /* Parse arguments */
    char path[1024] = "";
    int  start_line = 1;
    int  end_line   = AGENT_FILE_READ_DEFAULT_LINES;
    int  max_size   = (int)AGENT_FILE_READ_MAX_SIZE;

    if (json_args && json_args[0]) {
        tool_json_extract_str(json_args, "path", path, sizeof(path));
        start_line = tool_json_extract_int(json_args, "start_line", 1);
        end_line   = tool_json_extract_int(json_args, "end_line",   AGENT_FILE_READ_DEFAULT_LINES);
        max_size   = tool_json_extract_int(json_args, "max_size",   (int)AGENT_FILE_READ_MAX_SIZE);
    }

    /* Validate */
    if (path[0] == '\0') {
        snprintf(output, output_size, "{\"error\":\"path is required\"}");
        return -1;
    }
    if (start_line < 1) start_line = 1;
    if (end_line < start_line) end_line = start_line + AGENT_FILE_READ_DEFAULT_LINES - 1;
    if (max_size <= 0 || max_size > (int)AGENT_FILE_READ_MAX_SIZE)
        max_size = (int)AGENT_FILE_READ_MAX_SIZE;

    char resolved_path[AGENT_PATH_MAX];
    if (agent_resolve_workspace_path(path, resolved_path, sizeof(resolved_path)) != 0) {
        snprintf(output, output_size,
                 "{\"error\":\"path is outside workspace or does not exist\"}");
        return -1;
    }

    FILE* fp = fopen(resolved_path, "r");
    if (!fp) {
        snprintf(output, output_size, "{\"error\":\"cannot open file: %s\"}", path);
        return -1;
    }

    /* Read lines start_line..end_line into content buffer */
    size_t content_cap  = (size_t)max_size + 1;
    char*  content      = (char*)calloc(1, content_cap);
    if (!content) {
        fclose(fp);
        snprintf(output, output_size, "{\"error\":\"out of memory\"}");
        return -1;
    }

    char   line_buf[8192];
    int    current_line      = 0;
    int    last_collected    = 0;
    size_t content_len       = 0;
    int    truncated         = 0;
    long   file_size         = 0;

    while (fgets(line_buf, sizeof(line_buf), fp) != NULL) {
        current_line++;
        if (current_line >= start_line && current_line <= end_line) {
            size_t llen = strlen(line_buf);
            if (content_len + llen >= content_cap - 1) {
                truncated = 1;
                break;
            }
            memcpy(content + content_len, line_buf, llen);
            content_len += llen;
            last_collected = current_line;
        }
        if (current_line >= end_line) break;
    }
    content[content_len] = '\0';

    /* Get file size */
    if (fseek(fp, 0, SEEK_END) == 0) {
        file_size = ftell(fp);
    }
    fclose(fp);

    /* JSON-escape content */
    char* escaped = tool_json_escape_alloc(content, content_len);
    free(content);
    if (!escaped) {
        snprintf(output, output_size, "{\"error\":\"out of memory\"}");
        return -1;
    }

    int written = snprintf(output, output_size,
        "{\"content\":\"%s\","
        "\"lines_read\":%d,"
        "\"start_line\":%d,"
        "\"end_line\":%d,"
        "\"file_size\":%ld,"
        "\"truncated\":%s,"
        "\"path\":\"%s\"}",
        escaped,
        last_collected - start_line + 1,
        start_line,
        last_collected,
        file_size,
        truncated ? "true" : "false",
        path);

    free(escaped);
    return (written > 0 && (size_t)written < output_size) ? 0 : -1;
}
