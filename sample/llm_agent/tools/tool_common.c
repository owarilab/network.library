#include "tool_common.h"
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
            out[i++] = *p++;
        } else if (*p == '"') {
            break;
        } else {
            out[i++] = *p++;
        }
    }
    out[i] = '\0';
    return 1;
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
 * Handles: ", \, \n, \r, \t, and control characters
 * --------------------------------------------------------------- */
int tool_json_escape(const char* src, char* dst, size_t dst_size)
{
    if (!src || !dst || dst_size == 0) return -1;
    
    size_t w = 0;
    for (size_t i = 0; src[i] && w + 2 < dst_size; i++) {
        unsigned char c = (unsigned char)src[i];
        
        if (c == '"') {
            dst[w++] = '\\';
            dst[w++] = '"';
        } else if (c == '\\') {
            dst[w++] = '\\';
            dst[w++] = '\\';
        } else if (c == '\n') {
            dst[w++] = '\\';
            dst[w++] = 'n';
        } else if (c == '\r') {
            dst[w++] = '\\';
            dst[w++] = 'r';
        } else if (c == '\t') {
            dst[w++] = '\\';
            dst[w++] = 't';
        } else if (c < 0x20) {
            /* skip other control chars */
        } else {
            dst[w++] = (char)c;
        }
    }
    
    if (w >= dst_size - 1) return -1;
    dst[w] = '\0';
    return 0;
}

/* ---------------------------------------------------------------
 * JSON-escape src into a newly malloc'd string
 * --------------------------------------------------------------- */
char* tool_json_escape_alloc(const char* src, size_t src_len)
{
    if (!src) return NULL;
    
    /* Worst case: every char becomes 2 chars */
    char* out = (char*)malloc(src_len * 2 + 1);
    if (!out) return NULL;
    
    size_t w = 0;
    for (size_t i = 0; i < src_len && w + 2 < src_len * 2; i++) {
        unsigned char c = (unsigned char)src[i];
        
        if (c == '"') {
            out[w++] = '\\';
            out[w++] = '"';
        } else if (c == '\\') {
            out[w++] = '\\';
            out[w++] = '\\';
        } else if (c == '\n') {
            out[w++] = '\\';
            out[w++] = 'n';
        } else if (c == '\r') {
            out[w++] = '\\';
            out[w++] = 'r';
        } else if (c == '\t') {
            out[w++] = '\\';
            out[w++] = 't';
        } else if (c < 0x20) {
            /* skip other control chars */
        } else {
            out[w++] = (char)c;
        }
    }
    out[w] = '\0';
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
