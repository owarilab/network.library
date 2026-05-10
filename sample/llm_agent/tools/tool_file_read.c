#include "tool_file_read.h"
#include "../agent_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Minimal JSON field extractors (same pattern as tool_file_list.c) */
static int extract_str(const char* json, const char* key, char* out, size_t out_size)
{
    if (!json || !key || !out || out_size == 0) return 0;
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char* p = strstr(json, pattern);
    if (!p) return 0;
    p += strlen(pattern);
    while (*p == ' ' || *p == '\t') p++;
    if (*p != ':') return 0;
    p++;
    while (*p == ' ' || *p == '\t') p++;
    if (*p != '"') return 0;
    p++;
    size_t i = 0;
    while (*p && i < out_size - 1) {
        if (*p == '\\' && *(p+1)) { p++; out[i++] = *p++; }
        else if (*p == '"') break;
        else out[i++] = *p++;
    }
    out[i] = '\0';
    return 1;
}

static int extract_int(const char* json, const char* key, int defaultval)
{
    if (!json || !key) return defaultval;
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char* p = strstr(json, pattern);
    if (!p) return defaultval;
    p += strlen(pattern);
    while (*p == ' ' || *p == '\t') p++;
    if (*p != ':') return defaultval;
    p++;
    while (*p == ' ' || *p == '\t') p++;
    if (*p == '"') return defaultval;
    return atoi(p);
}

/* JSON-escape src into a newly malloc'd string. Caller must free(). */
static char* json_escape_alloc(const char* src, size_t src_len)
{
    /* Worst case: every char becomes 2 chars (\n -> \n) */
    char* out = (char*)malloc(src_len * 2 + 1);
    if (!out) return NULL;
    size_t w = 0;
    for (size_t i = 0; i < src_len; i++) {
        unsigned char c = (unsigned char)src[i];
        if      (c == '"')  { out[w++] = '\\'; out[w++] = '"';  }
        else if (c == '\\') { out[w++] = '\\'; out[w++] = '\\'; }
        else if (c == '\n') { out[w++] = '\\'; out[w++] = 'n';  }
        else if (c == '\r') { out[w++] = '\\'; out[w++] = 'r';  }
        else if (c == '\t') { out[w++] = '\\'; out[w++] = 't';  }
        else if (c < 0x20)  { /* skip other control chars */ }
        else                { out[w++] = (char)c; }
    }
    out[w] = '\0';
    return out;
}

int tool_file_read_execute(const char* json_args, char* output, size_t output_size)
{
    if (!output || output_size == 0) return -1;

    /* Parse arguments */
    char path[1024] = "";
    int  start_line = 1;
    int  end_line   = AGENT_FILE_READ_DEFAULT_LINES;
    int  max_size   = (int)AGENT_FILE_READ_MAX_SIZE;

    if (json_args && json_args[0]) {
        extract_str(json_args, "path", path, sizeof(path));
        start_line = extract_int(json_args, "start_line", 1);
        end_line   = extract_int(json_args, "end_line",   AGENT_FILE_READ_DEFAULT_LINES);
        max_size   = extract_int(json_args, "max_size",   (int)AGENT_FILE_READ_MAX_SIZE);
    }

    /* Validate */
    if (path[0] == '\0') {
        snprintf(output, output_size, "{\"error\":\"path is required\"}");
        return -1;
    }
    if (strstr(path, "..")) {
        snprintf(output, output_size, "{\"error\":\"path traversal not allowed\"}");
        return -1;
    }
    if (start_line < 1) start_line = 1;
    if (end_line < start_line) end_line = start_line + AGENT_FILE_READ_DEFAULT_LINES - 1;
    if (max_size <= 0 || max_size > (int)AGENT_FILE_READ_MAX_SIZE)
        max_size = (int)AGENT_FILE_READ_MAX_SIZE;

    FILE* fp = fopen(path, "r");
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
    char* escaped = json_escape_alloc(content, content_len);
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
