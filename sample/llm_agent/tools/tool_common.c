#include "tool_common.h"
#include "qs_api.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

/* ---------------------------------------------------------------
 * Extract string field from JSON: "key":"value"
 * --------------------------------------------------------------- */
int tool_json_extract_str(const char* json, const char* key, char* out, size_t out_size)
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
        if (*p == '\\' && *(p+1)) {
            p++;
            /* Handle JSON escape sequences */
            if (*p == 'n') {
                out[i++] = '\n';
                p++;
            } else if (*p == 'r') {
                out[i++] = '\r';
                p++;
            } else if (*p == 't') {
                out[i++] = '\t';
                p++;
            } else if (*p == '\\') {
                out[i++] = '\\';
                p++;
            } else if (*p == '"') {
                out[i++] = '"';
                p++;
            } else if (*p == '/') {
                out[i++] = '/';
                p++;
            } else {
                /* Unknown escape, just output the char after backslash */
                out[i++] = *p++;
            }
        } else if (*p == '"') {
            break;
        } else {
            out[i++] = *p++;
        }
    }
    out[i] = '\0';
    return 1;
}

int tool_json_extract_object(const char* json, const char* key, char* out, size_t out_size)
{
    if (!json || !key || !out || out_size == 0) return 0;

    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char* p = strstr(json, pattern);
    if (!p) return 0;

    p += strlen(pattern);
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    if (*p != ':') return 0;
    p++;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    if (*p != '{') return 0;

    int depth = 0;
    size_t i = 0;
    while (*p && i < out_size - 1) {
        if (*p == '"') {
            out[i++] = *p++;
            while (*p && i < out_size - 1) {
                out[i++] = *p;
                if (*p == '\\' && *(p + 1) && i < out_size - 1) {
                    p++;
                    out[i++] = *p;
                } else if (*p == '"') {
                    p++;
                    break;
                }
                p++;
            }
            continue;
        }

        if (*p == '{') depth++;
        if (*p == '}') depth--;
        out[i++] = *p++;
        if (depth == 0) break;
    }

    out[i] = '\0';
    return (i > 0 && depth == 0) ? 1 : 0;
}

/* ---------------------------------------------------------------
 * Extract integer field from JSON: "key": N
 * --------------------------------------------------------------- */
int tool_json_extract_int(const char* json, const char* key, int defaultval)
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
    if (*p == '"') return defaultval; /* string, not int */
    
    return atoi(p);
}

/* ---------------------------------------------------------------
 * JSON-escape src into dst buffer
 * Delegates to the core implementation (api_qs_json_escape).
 * Handles: ", \, \n, \r, \t, and control characters (\u00xx).
 * --------------------------------------------------------------- */
int tool_json_escape(const char* src, char* dst, size_t dst_size)
{
    return api_qs_json_escape(src, dst, dst_size);
}

/* ---------------------------------------------------------------
 * JSON-escape src into a newly malloc'd string
 * Delegates to the core implementation (api_qs_json_escape).
 * Worst case: every byte becomes \u00xx (6 bytes), so allocate
 * src_len * 6 + 1.
 * --------------------------------------------------------------- */
char* tool_json_escape_alloc(const char* src, size_t src_len)
{
    if (!src) return NULL;

    size_t cap = src_len * 6 + 1;
    char* out = (char*)malloc(cap);
    if (!out) return NULL;

    if (0 != api_qs_json_escape(src, out, cap)) {
        free(out);
        return NULL;
    }
    return out;
}

/* ---------------------------------------------------------------
 * Append text to output buffer with position tracking
 * --------------------------------------------------------------- */
int tool_buf_append(char* buf, size_t buf_size, size_t* pos, const char* text)
{
    if (!buf || !pos || !text) return -1;
    
    size_t len = strlen(text);
    if (*pos + len >= buf_size - 1) return -1;
    
    memcpy(buf + *pos, text, len);
    *pos += len;
    buf[*pos] = '\0';
    return 0;
}
