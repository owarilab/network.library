#include "handler_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------
 * LLM buffer
 * --------------------------------------------------------------- */
int llm_buffer_init(LLM_BUFFER* b, size_t initial_cap)
{
    if (!b || initial_cap == 0) return -1;
    b->buf = (char*)calloc(1, initial_cap);
    if (!b->buf) return -1;
    b->len = 0;
    b->cap = initial_cap;
    return 0;
}

void llm_buffer_free(LLM_BUFFER* b)
{
    if (!b) return;
    if (b->buf) { free(b->buf); b->buf = NULL; }
    b->len = 0;
    b->cap = 0;
}

int llm_token_callback(void* user_data, const char* token, int is_last)
{
    LLM_BUFFER* b = (LLM_BUFFER*)user_data;
    if (!b) return -1;
    if (is_last || !token || !token[0]) return 0;

    size_t tlen = strlen(token);
    size_t req  = b->len + tlen + 1;
    if (req > b->cap) {
        size_t new_cap = b->cap ? b->cap : 4096;
        while (new_cap < req) new_cap *= 2;
        char* resized = (char*)realloc(b->buf, new_cap);
        if (!resized) return -1;
        b->buf = resized;
        b->cap = new_cap;
    }
    memcpy(b->buf + b->len, token, tlen);
    b->len += tlen;
    b->buf[b->len] = '\0';
    return 0;
}

/* ---------------------------------------------------------------
 * HTTP response
 * --------------------------------------------------------------- */
int send_json_response(QS_EVENT_PARAMETER params, int status, const char* body)
{
    if (!body) return -1;

    const char* status_text = "OK";
    if      (status == 400) status_text = "Bad Request";
    else if (status == 404) status_text = "Not Found";
    else if (status == 500) status_text = "Internal Server Error";

    char header[512];
    snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: application/json; charset=utf-8\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",
        status, status_text, strlen(body));

    size_t resp_size = strlen(header) + strlen(body) + 1;
    char*  resp      = (char*)malloc(resp_size);
    if (!resp) return -1;

    resp[0] = '\0';
    strcat(resp, header);
    strcat(resp, body);
    api_qs_send_response(params, resp);
    free(resp);
    return 0;
}

/* ---------------------------------------------------------------
 * JSON field extractors
 * --------------------------------------------------------------- */
int hc_extract_str(const char* json, const char* key, char* out, size_t out_size)
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
    if (*p != '"') return 0;
    p++;

    size_t i = 0;
    while (*p && i < out_size - 1) {
        if (*p == '\\' && *(p + 1)) {
            p++;
            switch (*p) {
                case 'n':  out[i++] = '\n'; break;
                case 't':  out[i++] = '\t'; break;
                case 'r':  out[i++] = '\r'; break;
                case '"':  out[i++] = '"';  break;
                case '\\': out[i++] = '\\'; break;
                default:   out[i++] = *p;   break;
            }
            p++;
        } else if (*p == '"') {
            break;
        } else {
            out[i++] = *p++;
        }
    }
    out[i] = '\0';
    return 1;
}

int hc_extract_int(const char* json, const char* key, int default_val)
{
    if (!json || !key) return default_val;

    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char* p = strstr(json, pattern);
    if (!p) return default_val;

    p += strlen(pattern);
    while (*p == ' ' || *p == '\t') p++;
    if (*p != ':') return default_val;
    p++;
    while (*p == ' ' || *p == '\t') p++;
    if (*p == '"') return default_val; /* string field, not int */
    return atoi(p);
}

int hc_extract_obj_raw(const char* json, const char* key, char* out, size_t out_size)
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

    int depth = 0, in_string = 0, escaped = 0;
    size_t i = 0;
    while (*p && i < out_size - 1) {
        char c = *p;
        if (escaped)   { escaped = 0; out[i++] = *p++; continue; }
        if (c == '\\') { escaped = 1; out[i++] = *p++; continue; }
        if (c == '"')  { in_string = !in_string; out[i++] = *p++; continue; }
        if (!in_string) {
            if (c == '{') depth++;
            else if (c == '}') {
                depth--;
                if (depth == 0) { out[i++] = *p++; break; }
            }
        }
        out[i++] = *p++;
    }
    out[i] = '\0';
    return (depth == 0 && i > 0) ? 1 : 0;
}

/* ---------------------------------------------------------------
 * JSON string escape
 * --------------------------------------------------------------- */
char* hc_json_escape(const char* src)
{
    if (!src) return NULL;
    size_t src_len = strlen(src);
    char*  out     = (char*)malloc(src_len * 2 + 1);
    if (!out) return NULL;

    size_t w = 0;
    for (size_t i = 0; i < src_len; i++) {
        unsigned char c = (unsigned char)src[i];
        if      (c == '"')  { out[w++] = '\\'; out[w++] = '"';  }
        else if (c == '\\') { out[w++] = '\\'; out[w++] = '\\'; }
        else if (c == '\n') { out[w++] = '\\'; out[w++] = 'n';  }
        else if (c == '\r') { out[w++] = '\\'; out[w++] = 'r';  }
        else if (c == '\t') { out[w++] = '\\'; out[w++] = 't';  }
        else if (c < 0x20)  { /* skip control chars */ }
        else                { out[w++] = (char)c; }
    }
    out[w] = '\0';
    return out;
}

/* ---------------------------------------------------------------
 * Agent think-prompt builder
 *
 * Uses g_agent_prompt_template if set (loaded from prompt.conf),
 * replacing {query} and {accumulated_context} placeholders.
 * Falls back to a built-in template when not set.
 * --------------------------------------------------------------- */
static const char* BUILTIN_THINK_TEMPLATE =
    "[SYSTEM]\n"
    "You are a helpful coding assistant with access to file system tools.\n"
    "When you need information, use a tool. When you have enough information, provide the final answer.\n"
    "\n"
    "Always respond with ONLY a single valid JSON object. No markdown, no code fences.\n"
    "The first character must be '{' and the last must be '}'.\n"
    "\n"
    "Schema for tool use:\n"
    "{\"action\":\"use_tool\",\"thought\":\"<reasoning>\",\"tool_name\":\"<name>\",\"tool_args\":{<args>},\"answer\":\"\"}\n"
    "\n"
    "Schema for final answer:\n"
    "{\"action\":\"final_answer\",\"thought\":\"<reasoning>\",\"tool_name\":\"\",\"tool_args\":{},\"answer\":\"<complete answer>\"}\n"
    "\n"
    "Available tools:\n"
    "- file_list: List files in a directory.\n"
    "  Args: {\"path\":\"./dir\",\"recursive\":0,\"pattern\":\"*.c\"}\n"
    "- file_read: Read lines from a file.\n"
    "  Args: {\"path\":\"./file.c\",\"start_line\":1,\"end_line\":50}\n"
    "\n"
    "[QUERY]\n"
    "%s\n"
    "\n"
    "[CONTEXT]\n"
    "%s\n"
    "\n"
    "[ASSISTANT]\n";

/* Replace the first occurrence of `placeholder` with `value` inside `src`.
 * Returns a newly malloc'd string, or NULL on failure.
 * If placeholder is not found, returns a copy of src. */
static char* str_replace_first(const char* src,
                                const char* placeholder,
                                const char* value)
{
    const char* pos = strstr(src, placeholder);
    if (!pos) {
        /* no placeholder found — return copy */
        char* copy = (char*)malloc(strlen(src) + 1);
        if (copy) strcpy(copy, src);
        return copy;
    }

    size_t prefix_len  = (size_t)(pos - src);
    size_t ph_len      = strlen(placeholder);
    size_t value_len   = strlen(value);
    size_t suffix_len  = strlen(pos + ph_len);
    size_t total       = prefix_len + value_len + suffix_len + 1;

    char* out = (char*)malloc(total);
    if (!out) return NULL;

    memcpy(out, src, prefix_len);
    memcpy(out + prefix_len, value, value_len);
    memcpy(out + prefix_len + value_len, pos + ph_len, suffix_len);
    out[total - 1] = '\0';
    return out;
}

char* agent_build_think_prompt(const char* query, const char* context)
{
    if (!query) return NULL;
    const char* ctx = (context && context[0]) ? context : "(none)";

    /* Use external template if loaded */
    if (g_agent_prompt_template && g_agent_prompt_template[0]) {
        char* step1 = str_replace_first(g_agent_prompt_template, "{query}", query);
        if (!step1) return NULL;
        char* step2 = str_replace_first(step1, "{accumulated_context}", ctx);
        free(step1);
        return step2;
    }

    /* Built-in fallback */
    size_t total = strlen(BUILTIN_THINK_TEMPLATE) + strlen(query) + strlen(ctx) + 1;
    char*  prompt = (char*)malloc(total);
    if (!prompt) return NULL;
    snprintf(prompt, total, BUILTIN_THINK_TEMPLATE, query, ctx);
    return prompt;
}
