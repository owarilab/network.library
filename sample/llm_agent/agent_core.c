#include "agent_core.h"
#include "tools/tool_file_list.h"
#include "tools/tool_file_read.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ---------------------------------------------------------------
 * Tool registry (static table — add new tools here)
 * --------------------------------------------------------------- */
static TOOL_ENTRY g_tool_registry[] = {
    { "file_list", tool_file_list_execute },
    { "file_read", tool_file_read_execute },
    { NULL,        NULL                   }
};

/* ---------------------------------------------------------------
 * agent_conversation_generate_id
 * Produces "conv-XXXXXXXXXXXXXXXX" using time + rand.
 * --------------------------------------------------------------- */
void agent_conversation_generate_id(char* out, size_t len)
{
    if (out == NULL || len < 24) return;

    static int seeded = 0;
    if (!seeded) {
        srand((unsigned int)time(NULL));
        seeded = 1;
    }

    unsigned int a = (unsigned int)time(NULL);
    unsigned int b = (unsigned int)rand();
    unsigned int c = (unsigned int)rand();
    snprintf(out, len, "conv-%08x%08x%04x", a, b, c & 0xFFFF);
}

/* ---------------------------------------------------------------
 * agent_conversation_create
 * --------------------------------------------------------------- */
AGENT_CONVERSATION* agent_conversation_create(const char* user_query, int max_iterations)
{
    AGENT_CONVERSATION* conv = (AGENT_CONVERSATION*)calloc(1, sizeof(AGENT_CONVERSATION));
    if (conv == NULL) return NULL;

    agent_conversation_generate_id(conv->conversation_id, AGENT_CONV_ID_LEN);

    conv->iteration      = 0;
    conv->max_iterations = (max_iterations > 0 && max_iterations <= AGENT_MAX_ITERATIONS)
                           ? max_iterations : AGENT_MAX_ITERATIONS;

    if (user_query != NULL) {
        strncpy(conv->user_query, user_query, sizeof(conv->user_query) - 1);
        conv->user_query[sizeof(conv->user_query) - 1] = '\0';
    }

    conv->context_capacity  = AGENT_CONTEXT_INIT_SIZE;
    conv->accumulated_context = (char*)calloc(1, conv->context_capacity);
    if (conv->accumulated_context == NULL) {
        free(conv);
        return NULL;
    }
    conv->context_length = 0;

    conv->tool_use_file_list = 0;
    conv->tool_use_file_read = 0;

    return conv;
}

/* ---------------------------------------------------------------
 * agent_conversation_destroy
 * --------------------------------------------------------------- */
void agent_conversation_destroy(AGENT_CONVERSATION* conv)
{
    if (conv == NULL) return;
    if (conv->accumulated_context != NULL) {
        free(conv->accumulated_context);
        conv->accumulated_context = NULL;
    }
    free(conv);
}

/* ---------------------------------------------------------------
 * agent_conversation_append_context
 * Appends "[label]\ncontent\n" to accumulated_context.
 * Grows the buffer with realloc if needed.
 * Returns 0 on success, -1 on allocation failure.
 * --------------------------------------------------------------- */
int agent_conversation_append_context(AGENT_CONVERSATION* conv,
                                       const char* label, const char* content)
{
    if (conv == NULL || content == NULL) return -1;

    const char* safe_label   = (label != NULL) ? label : "info";
    size_t      label_len    = strlen(safe_label);
    size_t      content_len  = strlen(content);
    /* "[label]\n" + content + "\n\n" */
    size_t      needed       = label_len + 3 + content_len + 2 + 1;

    if (conv->context_length + needed > AGENT_CONTEXT_MAX_SIZE) {
        /* Context is full — silently skip */
        return 0;
    }

    /* Grow if necessary */
    while (conv->context_length + needed > conv->context_capacity) {
        size_t new_cap = conv->context_capacity * 2;
        if (new_cap > AGENT_CONTEXT_MAX_SIZE) new_cap = AGENT_CONTEXT_MAX_SIZE;
        char* new_buf = (char*)realloc(conv->accumulated_context, new_cap);
        if (new_buf == NULL) return -1;
        conv->accumulated_context = new_buf;
        conv->context_capacity    = new_cap;
    }

    /* Append "[label]\ncontent\n\n" */
    int written = snprintf(conv->accumulated_context + conv->context_length,
                           conv->context_capacity - conv->context_length,
                           "[%s]\n%s\n\n", safe_label, content);
    if (written > 0) {
        conv->context_length += (size_t)written;
    }
    return 0;
}

/* ---------------------------------------------------------------
 * agent_conversation_count_tool
 * --------------------------------------------------------------- */
void agent_conversation_count_tool(AGENT_CONVERSATION* conv, const char* tool_name)
{
    if (conv == NULL || tool_name == NULL) return;
    if (strcmp(tool_name, "file_list") == 0) conv->tool_use_file_list++;
    else if (strcmp(tool_name, "file_read") == 0) conv->tool_use_file_read++;
}

/* ---------------------------------------------------------------
 * agent_parse_action_string
 * --------------------------------------------------------------- */
AGENT_ACTION agent_parse_action_string(const char* action_str)
{
    if (action_str == NULL) return AGENT_ACTION_UNKNOWN;
    if (strcmp(action_str, "use_tool")     == 0) return AGENT_ACTION_USE_TOOL;
    if (strcmp(action_str, "final_answer") == 0) return AGENT_ACTION_FINAL_ANSWER;
    return AGENT_ACTION_UNKNOWN;
}

/* ---------------------------------------------------------------
 * agent_tool_is_registered
 * --------------------------------------------------------------- */
int agent_tool_is_registered(const char* tool_name)
{
    if (tool_name == NULL) return 0;
    for (int i = 0; g_tool_registry[i].name != NULL; i++) {
        if (strcmp(g_tool_registry[i].name, tool_name) == 0) return 1;
    }
    return 0;
}

/* ---------------------------------------------------------------
 * agent_tool_execute
 * Returns 0 on success, -1 on failure.
 * output receives a JSON string.
 * --------------------------------------------------------------- */
int agent_tool_execute(const char* tool_name, const char* json_args,
                       char* output, size_t output_size)
{
    if (tool_name == NULL || output == NULL || output_size == 0) return -1;

    for (int i = 0; g_tool_registry[i].name != NULL; i++) {
        if (strcmp(g_tool_registry[i].name, tool_name) == 0) {
            printf("[Tool] %s args=%s\n", tool_name, json_args ? json_args : "null");
            fflush(stdout);
            int ret = g_tool_registry[i].executor(json_args, output, output_size);
            printf("[Tool] %s => %s\n", tool_name, ret == 0 ? "ok" : "error");
            fflush(stdout);
            return ret;
        }
    }

    /* Unknown tool */
    printf("[Tool] unknown tool: %s\n", tool_name);
    fflush(stdout);
    snprintf(output, output_size, "{\"error\":\"unknown tool: %s\"}", tool_name);
    return -1;
}

/* ---------------------------------------------------------------
 * agent_parse_think_result
 *
 * Minimal JSON field extractor (no external library).
 * Looks for "action", "thought", "tool_name", "tool_args", "answer"
 * as top-level string / object fields.
 *
 * Returns 0 on success (action is valid), -1 otherwise.
 * --------------------------------------------------------------- */

/* Find a top-level JSON key (not inside any string value).
 * Returns pointer to the opening '"' of the key, or NULL if not found.
 * The key is confirmed to be a key (followed by ':') not a string value. */
static const char* json_find_toplevel_key(const char* json, const char* key)
{
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    size_t plen = strlen(pattern);

    const char* p = json;
    while (*p) {
        if (*p == '"') {
            /* Check if this is our target key */
            if (strncmp(p, pattern, plen) == 0) {
                const char* after = p + plen;
                while (*after == ' ' || *after == '\t' || *after == '\r' || *after == '\n') after++;
                if (*after == ':') return p; /* confirmed key */
            }
            /* Skip this string (key or value) */
            p++;
            while (*p && *p != '"') {
                if (*p == '\\' && *(p + 1)) p++; /* skip escaped char */
                p++;
            }
            if (*p == '"') p++; /* skip closing quote */
        } else {
            p++;
        }
    }
    return NULL;
}

/* Extract the value of a JSON string field: "key":"value"
 * Writes into out (max out_size bytes incl. NUL).
 * Returns 1 if found, 0 if not. */
static int json_extract_string(const char* json, const char* key,
                               char* out, size_t out_size)
{
    if (!json || !key || !out || out_size == 0) return 0;

    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);

    const char* p = json_find_toplevel_key(json, key);
    if (p == NULL) return 0;

    p += strlen(pattern);
    /* Skip whitespace and colon */
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    if (*p != ':') return 0;
    p++;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    if (*p != '"') return 0;
    p++; /* skip opening quote */

    size_t i = 0;
    while (*p != '\0' && i < out_size - 1) {
        if (*p == '\\' && *(p + 1) != '\0') {
            /* Handle common escape sequences */
            p++;
            switch (*p) {
                case 'n': out[i++] = '\n'; break;
                case 't': out[i++] = '\t'; break;
                case 'r': out[i++] = '\r'; break;
                case '"': out[i++] = '"';  break;
                case '\\': out[i++] = '\\'; break;
                default:  out[i++] = *p;   break;
            }
            p++;
        } else if (*p == '"') {
            break; /* closing quote */
        } else {
            out[i++] = *p++;
        }
    }
    out[i] = '\0';
    return 1;
}

/* Extract the value of a JSON object field: "key":{...}
 * Copies the raw {...} (including braces) into out.
 * Returns 1 if found, 0 if not. */
static int json_extract_object(const char* json, const char* key,
                               char* out, size_t out_size)
{
    if (!json || !key || !out || out_size == 0) return 0;

    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);

    const char* p = json_find_toplevel_key(json, key);
    if (p == NULL) return 0;

    p += strlen(pattern);
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    if (*p != ':') return 0;
    p++;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    if (*p != '{') return 0;

    /* Copy balanced braces, skipping string contents */
    int depth = 0;
    size_t i = 0;
    while (*p != '\0' && i < out_size - 1) {
        if (*p == '"') {
            /* Copy string verbatim */
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
        } else {
            if (*p == '{') depth++;
            else if (*p == '}') {
                depth--;
                if (depth == 0) {
                    out[i++] = *p++;
                    break;
                }
            }
            out[i++] = *p++;
        }
    }
    out[i] = '\0';
    return (depth == 0 && i > 0) ? 1 : 0;
}

int agent_parse_think_result(const char* llm_json, AGENT_THINK_RESULT* out)
{
    if (llm_json == NULL || out == NULL) return -1;

    memset(out, 0, sizeof(AGENT_THINK_RESULT));
    out->action = AGENT_ACTION_UNKNOWN;

    /* Find the first '{' to skip any leading text */
    /* Strip <think>...</think> blocks (Qwen3 thinking mode) before parsing.
     * The model sometimes emits these before or after the JSON. */
    const char* scan = llm_json;
    /* If <think> appears before {, skip to after </think> */
    const char* think_open = strstr(scan, "<think>");
    const char* first_brace = strchr(scan, '{');
    if (think_open != NULL && (first_brace == NULL || think_open < first_brace)) {
        const char* think_close = strstr(think_open, "</think>");
        if (think_close != NULL) {
            scan = think_close + 8; /* skip past </think> */
        }
    }

    const char* json_start = strchr(scan, '{');
    if (json_start == NULL) return -1;

    char action_str[64] = {0};
    json_extract_string(json_start, "action",    action_str,         sizeof(action_str));
    json_extract_string(json_start, "thought",   out->thought,       sizeof(out->thought));
    json_extract_string(json_start, "tool_name", out->tool_call.tool_name,
                        sizeof(out->tool_call.tool_name));
    json_extract_object(json_start, "tool_args", out->tool_call.json_args,
                        sizeof(out->tool_call.json_args));
    json_extract_string(json_start, "answer",    out->answer,        sizeof(out->answer));

    out->action = agent_parse_action_string(action_str);
    return (out->action != AGENT_ACTION_UNKNOWN) ? 0 : -1;
}
