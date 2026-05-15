#ifndef TOOL_COMMON_H
#define TOOL_COMMON_H

#include <stddef.h>

/* ---------------------------------------------------------------
 * Common JSON parsing utilities for tools
 * --------------------------------------------------------------- */

/* Extract a string field from JSON: "key":"value"
 * Returns 1 if found, 0 if not. */
int tool_json_extract_str(const char* json, const char* key, char* out, size_t out_size);

/* Extract an object field from JSON: "key": {...}
 * Copies the raw object including braces into out.
 * Returns 1 if found, 0 if not. */
int tool_json_extract_object(const char* json, const char* key, char* out, size_t out_size);

/* Extract an integer field from JSON: "key": N
 * Returns the value or defaultval if not found. */
int tool_json_extract_int(const char* json, const char* key, int defaultval);

/* ---------------------------------------------------------------
 * JSON escaping utilities
 * --------------------------------------------------------------- */

/* JSON-escape src into dst buffer.
 * Handles: ", \, \n, \r, \t, and control characters.
 * Returns 0 on success, -1 if dst is too small. */
int tool_json_escape(const char* src, char* dst, size_t dst_size);

/* JSON-escape src into a newly malloc'd string.
 * Caller must free() the result.
 * Returns NULL if allocation fails. */
char* tool_json_escape_alloc(const char* src, size_t src_len);

/* ---------------------------------------------------------------
 * Buffer utilities
 * --------------------------------------------------------------- */

/* Append text to a buffer with position tracking.
 * Returns 0 on success, -1 if would overflow. */
int tool_buf_append(char* buf, size_t buf_size, size_t* pos, const char* text);

#endif /* TOOL_COMMON_H */
