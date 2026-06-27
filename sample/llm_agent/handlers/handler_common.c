#include "handler_common.h"
#include "../agent_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AGENT_PROMPT_RECENT_ITEM_LIMIT 8
#define AGENT_PROMPT_ITEM_CHAR_LIMIT   240
#define AGENT_PROMPT_FOCUSED_RESULT_CHAR_LIMIT 4096

typedef struct {
    char*  buf;
    size_t len;
    size_t cap;
} PROMPT_BUFFER;

static int prompt_buffer_init(PROMPT_BUFFER* pb, size_t initial_cap)
{
    if (pb == NULL || initial_cap == 0) return -1;
    pb->buf = (char*)calloc(1, initial_cap);
    if (pb->buf == NULL) return -1;
    pb->len = 0;
    pb->cap = initial_cap;
    return 0;
}

static void prompt_buffer_free(PROMPT_BUFFER* pb)
{
    if (pb == NULL) return;
    free(pb->buf);
    pb->buf = NULL;
    pb->len = 0;
    pb->cap = 0;
}

static int prompt_buffer_reserve(PROMPT_BUFFER* pb, size_t extra)
{
    size_t needed;
    size_t new_cap;
    char* resized;

    if (pb == NULL) return -1;
    needed = pb->len + extra + 1;
    if (needed <= pb->cap) return 0;

    new_cap = pb->cap ? pb->cap : 256;
    while (new_cap < needed) new_cap *= 2;

    resized = (char*)realloc(pb->buf, new_cap);
    if (resized == NULL) return -1;
    pb->buf = resized;
    pb->cap = new_cap;
    return 0;
}

static int prompt_buffer_append_n(PROMPT_BUFFER* pb, const char* text, size_t text_len)
{
    if (pb == NULL || text == NULL) return -1;
    if (prompt_buffer_reserve(pb, text_len) != 0) return -1;

    memcpy(pb->buf + pb->len, text, text_len);
    pb->len += text_len;
    pb->buf[pb->len] = '\0';
    return 0;
}

static int prompt_buffer_append(PROMPT_BUFFER* pb, const char* text)
{
    return prompt_buffer_append_n(pb, text, text ? strlen(text) : 0);
}

static int prompt_buffer_appendf(PROMPT_BUFFER* pb, const char* format, ...)
{
    int needed;
    va_list args;
    va_list args_copy;

    if (pb == NULL || format == NULL) return -1;

    va_start(args, format);
    va_copy(args_copy, args);
    needed = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);
    if (needed < 0) {
        va_end(args);
        return -1;
    }

    if (prompt_buffer_reserve(pb, (size_t)needed) != 0) {
        va_end(args);
        return -1;
    }

    vsnprintf(pb->buf + pb->len, pb->cap - pb->len, format, args);
    va_end(args);
    pb->len += (size_t)needed;
    return 0;
}

static const char* agent_item_type_name(AGENT_ITEM_TYPE type)
{
    switch (type) {
        case AGENT_ITEM_USER_QUERY:    return "user_query";
        case AGENT_ITEM_MODEL_ACTION:  return "model_action";
        case AGENT_ITEM_TOOL_CALL:     return "tool_call";
        case AGENT_ITEM_TOOL_RESULT:   return "tool_result";
        case AGENT_ITEM_WARNING:       return "warning";
        case AGENT_ITEM_FINAL_ANSWER:  return "final_answer";
        default:                       return "info";
    }
}

static int agent_parse_tool_name_from_label(const char* label,
                                            char* out,
                                            size_t out_size)
{
    const char* start;
    const char* end;
    size_t len;

    if (out == NULL || out_size == 0) return -1;
    out[0] = '\0';

    if (label == NULL || strncmp(label, "tool_call(", 10) != 0) return -1;

    start = label + 10;
    end = strchr(start, ')');
    if (end == NULL || end <= start) return -1;

    len = (size_t)(end - start);
    if (len >= out_size) len = out_size - 1;
    memcpy(out, start, len);
    out[len] = '\0';
    return 0;
}

static int agent_tool_call_matches_pending_path(const AGENT_CONVERSATION* conv,
                                                const AGENT_CONVERSATION_ITEM* item)
{
    char user_path[AGENT_PATH_MAX] = "";
    char resolved_path[AGENT_PATH_MAX] = "";

    if (conv == NULL || item == NULL || item->content == NULL) return 0;
    if (!conv->pending_verification || conv->pending_verification_path[0] == '\0') return 0;

    if (!hc_extract_str(item->content, "path", user_path, sizeof(user_path))) return 0;
    if (agent_resolve_workspace_path(user_path, resolved_path, sizeof(resolved_path)) != 0) return 0;

    return strcmp(resolved_path, conv->pending_verification_path) == 0;
}

static const AGENT_CONVERSATION_ITEM* agent_find_tool_call_for_result(const AGENT_CONVERSATION* conv,
                                                                      size_t result_index)
{
    size_t i;

    if (conv == NULL || conv->items == NULL || result_index >= conv->item_count) return NULL;

    for (i = result_index; i > 0; i--) {
        AGENT_CONVERSATION_ITEM* item = &conv->items[i - 1];
        if (item->type == AGENT_ITEM_TOOL_CALL) {
            return item;
        }
        if (item->type == AGENT_ITEM_TOOL_RESULT) {
            break;
        }
    }

    return NULL;
}

static int agent_tool_result_priority(const AGENT_CONVERSATION* conv,
                                      const char* tool_name,
                                      const AGENT_CONVERSATION_ITEM* call_item,
                                      const AGENT_CONVERSATION_ITEM* result_item)
{
    int count;

    if (tool_name == NULL || call_item == NULL || result_item == NULL) return -1;

    if (conv != NULL &&
        conv->pending_verification &&
        strcmp(tool_name, "file_read") == 0 &&
        agent_tool_call_matches_pending_path(conv, call_item)) {
        return 100;
    }

    if (strcmp(tool_name, "grep_search") == 0) {
        count = hc_extract_int(result_item->content, "count", 0);
        return (count > 0) ? 80 : 20;
    }

    if (strcmp(tool_name, "file_search") == 0) {
        count = hc_extract_int(result_item->content, "count", 0);
        return (count > 0) ? 70 : 15;
    }

    if (strcmp(tool_name, "file_read") == 0) {
        return 60;
    }

    return 10;
}

static int agent_prompt_append_focused_tool_result(PROMPT_BUFFER* pb,
                                                   const AGENT_CONVERSATION* conv)
{
    const AGENT_CONVERSATION_ITEM* selected_call = NULL;
    const AGENT_CONVERSATION_ITEM* selected_result = NULL;
    char selected_tool_name[64] = "";
    int selected_priority = -1;

    if (pb == NULL || conv == NULL || conv->items == NULL || conv->item_count == 0) {
        return 0;
    }

    for (size_t i = conv->item_count; i > 0; i--) {
        AGENT_CONVERSATION_ITEM* result_item = &conv->items[i - 1];
        const AGENT_CONVERSATION_ITEM* call_item;
        char tool_name[64] = "";

        if (result_item->type != AGENT_ITEM_TOOL_RESULT) continue;

        call_item = agent_find_tool_call_for_result(conv, i - 1);
        if (call_item == NULL) continue;
        if (agent_parse_tool_name_from_label(call_item->label, tool_name, sizeof(tool_name)) != 0) {
            continue;
        }

        {
            int priority = agent_tool_result_priority(conv,
                                                      tool_name,
                                                      call_item,
                                                      result_item);
            if (priority > selected_priority) {
                selected_priority = priority;
                selected_call = call_item;
                selected_result = result_item;
                strncpy(selected_tool_name, tool_name, sizeof(selected_tool_name) - 1);
                selected_tool_name[sizeof(selected_tool_name) - 1] = '\0';
            }
        }

        if (selected_priority >= 100) {
            break;
        }

        if (selected_result == NULL &&
            (strcmp(tool_name, "file_read") == 0 ||
             strcmp(tool_name, "grep_search") == 0 ||
             strcmp(tool_name, "file_search") == 0)) {
            selected_call = call_item;
            selected_result = result_item;
            strncpy(selected_tool_name, tool_name, sizeof(selected_tool_name) - 1);
            selected_tool_name[sizeof(selected_tool_name) - 1] = '\0';
        }
    }

    if (selected_result == NULL || selected_call == NULL) return 0;

    if (prompt_buffer_append(pb, "[focused_tool_result]\n") != 0 ||
        prompt_buffer_appendf(pb, "tool=%s\n", selected_tool_name[0] ? selected_tool_name : "unknown") != 0 ||
        prompt_buffer_appendf(pb, "tool_args=%s\n", selected_call->content ? selected_call->content : "{}") != 0 ||
        prompt_buffer_append(pb, "raw_result=\n") != 0) {
        return -1;
    }

    if (selected_result->content != NULL) {
        size_t raw_len = strlen(selected_result->content);
        size_t write_len = raw_len;
        if (write_len > AGENT_PROMPT_FOCUSED_RESULT_CHAR_LIMIT) {
            write_len = AGENT_PROMPT_FOCUSED_RESULT_CHAR_LIMIT;
        }
        if (prompt_buffer_append_n(pb, selected_result->content, write_len) != 0) {
            return -1;
        }
        if (write_len < raw_len &&
            prompt_buffer_append(pb, "\n...(truncated)\n\n") != 0) {
            return -1;
        }
        if (write_len == raw_len && prompt_buffer_append(pb, "\n\n") != 0) {
            return -1;
        }
    } else {
        if (prompt_buffer_append(pb, "(none)\n\n") != 0) return -1;
    }

    return 0;
}

static size_t agent_append_compact_text(char* out,
                                        size_t out_size,
                                        const char* text,
                                        size_t limit)
{
    size_t src_len;
    size_t write_len;
    size_t w = 0;

    if (out == NULL || out_size == 0) return 0;
    if (text == NULL) text = "";

    src_len = strlen(text);
    write_len = src_len;
    if (write_len > limit) write_len = limit;

    for (size_t i = 0; i < write_len && w + 1 < out_size; i++) {
        char c = text[i];
        if (c == '\n' || c == '\r' || c == '\t') c = ' ';
        out[w++] = c;
    }

    while (w > 0 && out[w - 1] == ' ') w--;
    if (src_len > write_len && w + 3 < out_size) {
        out[w++] = '.';
        out[w++] = '.';
        out[w++] = '.';
    }
    out[w] = '\0';
    return w;
}

static int agent_prompt_append_recent_items(PROMPT_BUFFER* pb,
                                            const AGENT_CONVERSATION* conv,
                                            size_t max_items,
                                            size_t max_chars_per_item)
{
    size_t start;
    size_t rendered = 0;
    ssize_t warning_index = -1;

    if (pb == NULL || conv == NULL) return -1;
    if (prompt_buffer_append(pb, "[recent_items]\n") != 0) return -1;

    if (conv->item_count == 0 || conv->items == NULL) {
        return prompt_buffer_append(pb, "- (none)\n\n");
    }

    start = (conv->item_count > max_items) ? (conv->item_count - max_items) : 0;

    for (size_t i = 0; i < conv->item_count; i++) {
        if (conv->items[i].type == AGENT_ITEM_WARNING) warning_index = (ssize_t)i;
    }

    if (warning_index >= 0 && (size_t)warning_index < start) {
        char compact[AGENT_PROMPT_ITEM_CHAR_LIMIT + 8];
        AGENT_CONVERSATION_ITEM* item = &conv->items[warning_index];
        agent_append_compact_text(compact, sizeof(compact), item->content, max_chars_per_item);
        if (prompt_buffer_appendf(pb, "- warning(%s): %s\n",
                                  item->label ? item->label : "warning",
                                  compact) != 0) {
            return -1;
        }
        rendered++;
    }

    for (size_t i = start; i < conv->item_count; i++) {
        AGENT_CONVERSATION_ITEM* item = &conv->items[i];
        char compact[AGENT_PROMPT_ITEM_CHAR_LIMIT + 8];
        const char* type_name;

        if (item->type == AGENT_ITEM_USER_QUERY || item->type == AGENT_ITEM_FINAL_ANSWER) {
            continue;
        }

        agent_append_compact_text(compact, sizeof(compact), item->content, max_chars_per_item);
        type_name = agent_item_type_name(item->type);

        if (item->label && item->label[0] && strcmp(item->label, type_name) != 0) {
            if (prompt_buffer_appendf(pb, "- %s(%s): %s\n", type_name, item->label, compact) != 0) {
                return -1;
            }
        } else {
            if (prompt_buffer_appendf(pb, "- %s: %s\n", type_name, compact) != 0) {
                return -1;
            }
        }
        rendered++;
    }

    if (rendered == 0) {
        if (prompt_buffer_append(pb, "- (none)\n") != 0) return -1;
    }
    return prompt_buffer_append(pb, "\n");
}

static char* agent_render_compact_context(const AGENT_CONVERSATION* conv)
{
    PROMPT_BUFFER pb;
    const char* pending_path;
    const char* last_mutation_tool;

    if (conv == NULL) return NULL;
    if (prompt_buffer_init(&pb, 1024) != 0) return NULL;

    pending_path = conv->pending_verification_path[0] ? conv->pending_verification_path : "(none)";
    last_mutation_tool = conv->last_mutation_tool[0] ? conv->last_mutation_tool : "(none)";

    if (prompt_buffer_append(&pb, "[runtime_state]\n") != 0 ||
        prompt_buffer_appendf(&pb, "iteration=%d/%d\n", conv->iteration, conv->max_iterations) != 0 ||
        prompt_buffer_appendf(&pb, "pending_verification=%s\n", conv->pending_verification ? "true" : "false") != 0 ||
        prompt_buffer_appendf(&pb, "pending_verification_path=%s\n", pending_path) != 0 ||
        prompt_buffer_appendf(&pb, "last_mutation_tool=%s\n\n", last_mutation_tool) != 0) {
        prompt_buffer_free(&pb);
        return NULL;
    }

    if (conv->item_count > 0) {
        if (agent_prompt_append_recent_items(&pb,
                                             conv,
                                             AGENT_PROMPT_RECENT_ITEM_LIMIT,
                                             AGENT_PROMPT_ITEM_CHAR_LIMIT) != 0) {
            prompt_buffer_free(&pb);
            return NULL;
        }
        if (agent_prompt_append_focused_tool_result(&pb, conv) != 0) {
            prompt_buffer_free(&pb);
            return NULL;
        }
    } else if (conv->accumulated_context && conv->accumulated_context[0]) {
        if (prompt_buffer_append(&pb, "[compatibility_context]\n") != 0 ||
            prompt_buffer_append(&pb, conv->accumulated_context) != 0 ||
            prompt_buffer_append(&pb, "\n") != 0) {
            prompt_buffer_free(&pb);
            return NULL;
        }
    } else {
        if (prompt_buffer_append(&pb, "[recent_items]\n- (none)\n\n") != 0) {
            prompt_buffer_free(&pb);
            return NULL;
        }
    }

    return pb.buf;
}

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
    "Use grep_search for exact identifiers or strings, and use file_read after the target path is narrowed.\n"
    "For grep_search, file_pattern must be a basename glob like *.c, never a path. If you want one file, put that file path in path.\n"
    "If runtime_state says pending_verification=true, inspect pending_verification_path with file_read before broader exploration or final_answer.\n"
    "\n"
    "Always respond with ONLY a single valid JSON object. No markdown, no code fences.\n"
    "The first character must be '{' and the last must be '}'.\n"
    "\n"
    "Schema for tool use:\n"
    "{\"action\":\"use_tool\",\"summary\":\"<optional short summary>\",\"tool_name\":\"<name>\",\"tool_args\":{<args>},\"answer\":\"\"}\n"
    "\n"
    "Schema for final answer:\n"
    "{\"action\":\"final_answer\",\"summary\":\"<optional short summary>\",\"tool_name\":\"\",\"tool_args\":{},\"answer\":\"<complete answer>\"}\n"
    "\n"
    "Available tools:\n"
    "- file_list: List files in a directory.\n"
    "  Args: {\"path\":\"./dir\",\"recursive\":0,\"pattern\":\"*.c\"}\n"
    "- grep_search: Search file contents for an exact identifier or text match.\n"
    "  Args: {\"pattern\":\"focused_tool_result\",\"path\":\".\",\"file_pattern\":\"*.c\",\"recursive\":1}\n"
    "  file_pattern filters basenames only. For a single file, set path to that file and keep file_pattern broad.\n"
    "- file_read: Read lines from a file.\n"
    "  Args: {\"path\":\"./file.c\",\"start_line\":1,\"end_line\":50}\n"
    "If a scoped grep_search returns no matches, retry once with a broader path or simpler file_pattern before concluding the target does not exist.\n"
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

char* agent_build_think_prompt_from_conversation(const AGENT_CONVERSATION* conv)
{
    char* compact_context;
    char* prompt;

    if (conv == NULL || conv->user_query[0] == '\0') return NULL;

    compact_context = agent_render_compact_context(conv);
    if (compact_context == NULL) return NULL;

    prompt = agent_build_think_prompt(conv->user_query, compact_context);
    free(compact_context);
    return prompt;
}
