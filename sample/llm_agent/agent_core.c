#include "agent_core.h"
#include "tools/tool_file_list.h"
#include "tools/tool_file_read.h"
#include "tools/tool_file_write.h"
#include "tools/tool_file_edit.h"
#include "tools/tool_file_search.h"
#include "tools/tool_grep_search.h"
#include "tools/tool_http_request.h"
#include "qs_api.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>

/* Forward declarations */
static const char* json_find_toplevel_key(const char* json, const char* key);
static const char* json_find_value_start(const char* json, const char* key);
static int read_text_file(const char* path, char** out_buf, size_t* out_size);
static int alloc_json_memory(QS_MEMORY_CONTEXT* ctx, size_t json_size);
int tool_http_request_execute(const char* json_args, char* output, size_t output_size);
int tool_url_whitelist_get_execute(const char* json_args, char* output, size_t output_size);
int tool_url_whitelist_info_get_execute(const char* json_args, char* output, size_t output_size);

char g_agent_workspace_root[AGENT_PATH_MAX] = ".";

/* ---------------------------------------------------------------
 * URL whitelist (loaded from url_whitelist.json)
 * --------------------------------------------------------------- */
#define WHITELIST_MAX_HOSTS 64

typedef struct {
    char host[1024];
    char description[512];
} WhitelistEntry;

static WhitelistEntry g_whitelist[WHITELIST_MAX_HOSTS];
static int g_whitelist_count = 0;

/* ---------------------------------------------------------------
 * API documentation (loaded from api_docs.json)
 * Raw JSON string owned by this module.
 * --------------------------------------------------------------- */
static char* g_api_docs_raw = NULL;

static int read_text_file(const char* path, char** out_buf, size_t* out_size)
{
    if (!path || !out_buf) return -1;

    FILE* fp = fopen(path, "rb");
    if (!fp) return -1;

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return -1;
    }

    long file_size = ftell(fp);
    if (file_size < 0) {
        fclose(fp);
        return -1;
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return -1;
    }

    char* buf = (char*)malloc((size_t)file_size + 1);
    if (!buf) {
        fclose(fp);
        return -1;
    }

    size_t read_size = fread(buf, 1, (size_t)file_size, fp);
    fclose(fp);
    buf[read_size] = '\0';

    *out_buf = buf;
    if (out_size) *out_size = read_size;
    return 0;
}

static int alloc_json_memory(QS_MEMORY_CONTEXT* ctx, size_t json_size)
{
    size_t alloc_size = json_size * 8 + (64 * 1024);
    if (alloc_size < 256 * 1024) alloc_size = 256 * 1024;
    return api_qs_memory_alloc(ctx, alloc_size);
}

/* ---------------------------------------------------------------
 * agent_load_url_whitelist
 * Reads url_whitelist.json, parses "hosts" array using inline JSON.
 * --------------------------------------------------------------- */
int agent_load_url_whitelist(const char* json_path)
{
    char* json_str = NULL;
    size_t json_size = 0;
    QS_MEMORY_CONTEXT memory = {0};
    QS_JSON_ELEMENT_OBJECT root;
    QS_JSON_ELEMENT_ARRAY hosts;

    g_whitelist_count = 0;

    if (!json_path || !json_path[0]) return -1;
    if (read_text_file(json_path, &json_str, &json_size) != 0) {
        printf("[Whitelist] Cannot open %s: %s\n", json_path, strerror(errno));
        return -1;
    }
    if (alloc_json_memory(&memory, json_size) != 0) {
        free(json_str);
        return -1;
    }
    if (api_qs_json_decode_object(&memory, &root, json_str) != 0 ||
        api_qs_object_get_array(&root, "hosts", &hosts) != 0) {
        api_qs_memory_free(&memory);
        free(json_str);
        return -1;
    }

    int host_count = api_qs_array_get_length(&hosts);
    for (int i = 0; i < host_count && g_whitelist_count < WHITELIST_MAX_HOSTS; i++) {
        QS_JSON_ELEMENT_OBJECT host_obj;
        if (api_qs_array_get_object(&hosts, i, &host_obj) != 0) continue;

        char* host = api_qs_object_get_string(&host_obj, "host");
        char* desc = api_qs_object_get_string(&host_obj, "description");
        if (!host || !host[0]) continue;

        strncpy(g_whitelist[g_whitelist_count].host, host,
                sizeof(g_whitelist[g_whitelist_count].host) - 1);
        g_whitelist[g_whitelist_count].host[sizeof(g_whitelist[g_whitelist_count].host) - 1] = '\0';

        if (desc) {
            strncpy(g_whitelist[g_whitelist_count].description, desc,
                    sizeof(g_whitelist[g_whitelist_count].description) - 1);
            g_whitelist[g_whitelist_count].description[sizeof(g_whitelist[g_whitelist_count].description) - 1] = '\0';
        } else {
            g_whitelist[g_whitelist_count].description[0] = '\0';
        }

        g_whitelist_count++;
    }

    api_qs_memory_free(&memory);
    free(json_str);
    printf("[Whitelist] Loaded %d hosts from %s\n", g_whitelist_count, json_path);
    return 0;
}

int agent_is_host_allowed(const char* host)
{
    if (!host || !host[0]) return 0;
    for (int i = 0; i < g_whitelist_count; i++) {
        if (strcmp(host, g_whitelist[i].host) == 0) return 1;
    }
    return 0;
}

int agent_get_whitelist_count(void)
{
    return g_whitelist_count;
}

const char* agent_get_whitelist_host(int index)
{
    if (index < 0 || index >= g_whitelist_count) return NULL;
    return g_whitelist[index].host;
}

const char* agent_get_whitelist_desc(int index)
{
    if (index < 0 || index >= g_whitelist_count) return "";
    return g_whitelist[index].description;
}

void agent_free_url_whitelist(void)
{
    g_whitelist_count = 0;
}

/* ---------------------------------------------------------------
 * API docs loading
 * --------------------------------------------------------------- */
int agent_load_api_docs(const char* json_path)
{
    if (!json_path || !json_path[0]) return -1;

    agent_free_api_docs();

    size_t json_size = 0;
    if (read_text_file(json_path, &g_api_docs_raw, &json_size) != 0) {
        printf("[ApiDocs] Cannot open %s: %s\n", json_path, strerror(errno));
        return -1;
    }

    QS_MEMORY_CONTEXT memory = {0};
    QS_JSON_ELEMENT_OBJECT root;
    QS_JSON_ELEMENT_ARRAY keys;
    int host_entry_count = 0;

    if (alloc_json_memory(&memory, json_size) != 0) {
        free(g_api_docs_raw);
        g_api_docs_raw = NULL;
        return -1;
    }

    if (api_qs_json_decode_object(&memory, &root, g_api_docs_raw) != 0) {
        api_qs_memory_free(&memory);
        agent_free_api_docs();
        return -1;
    }

    if (api_qs_object_get_keys(&root, &keys) == 0) {
        host_entry_count = api_qs_array_get_length(&keys);
    }

    api_qs_memory_free(&memory);
    printf("[ApiDocs] Loaded %d host entries from %s\n", host_entry_count, json_path);
    return 0;
}

char* agent_get_api_doc_for_host(const char* host, size_t* out_size)
{
    if (out_size) *out_size = 0;
    if (!host || !host[0] || !g_api_docs_raw) return NULL;

    QS_MEMORY_CONTEXT memory = {0};
    QS_JSON_ELEMENT_OBJECT root;
    QS_JSON_ELEMENT_ARRAY endpoints;
    char* result = NULL;

    size_t json_size = strlen(g_api_docs_raw);
    if (alloc_json_memory(&memory, json_size) != 0) return NULL;
    if (api_qs_json_decode_object(&memory, &root, g_api_docs_raw) != 0 ||
        api_qs_object_get_array(&root, host, &endpoints) != 0) {
        api_qs_memory_free(&memory);
        return NULL;
    }

    size_t encode_size = json_size + 4096;
    char* encoded = api_qs_json_encode_array(&endpoints, encode_size);
    if (encoded) {
        size_t len = strlen(encoded);
        result = (char*)malloc(len + 1);
        if (result) {
            memcpy(result, encoded, len + 1);
            if (out_size) *out_size = len;
        }
    }

    api_qs_memory_free(&memory);
    return result;
}

char* agent_filter_api_docs(const char* host_json_str, const char* filter_pattern, size_t* out_size)
{
    if (!host_json_str || !out_size) return NULL;
    *out_size = 0;

    QS_MEMORY_CONTEXT req_mem = {0};
    QS_JSON_ELEMENT_OBJECT req_obj;
    char* host = NULL;
    char* doc = NULL;
    char* wrapped = NULL;
    char* result = NULL;

    if (alloc_json_memory(&req_mem, strlen(host_json_str)) != 0) return NULL;
    if (api_qs_json_decode_object(&req_mem, &req_obj, host_json_str) != 0) {
        api_qs_memory_free(&req_mem);
        return NULL;
    }

    host = api_qs_object_get_string(&req_obj, "host");
    if (!host || !host[0]) {
        api_qs_memory_free(&req_mem);
        return NULL;
    }

    doc = agent_get_api_doc_for_host(host, out_size);
    if (!doc) {
        api_qs_memory_free(&req_mem);
        return NULL;
    }
    if (!filter_pattern || !filter_pattern[0]) {
        api_qs_memory_free(&req_mem);
        return doc;
    }

    size_t wrapped_size = strlen(doc) + 32;
    wrapped = (char*)malloc(wrapped_size);
    if (!wrapped) {
        free(doc);
        api_qs_memory_free(&req_mem);
        return NULL;
    }
    snprintf(wrapped, wrapped_size, "{\"endpoints\":%s}", doc);

    QS_MEMORY_CONTEXT doc_mem = {0};
    QS_JSON_ELEMENT_OBJECT doc_obj;
    QS_JSON_ELEMENT_ARRAY endpoints;
    QS_JSON_ELEMENT_ARRAY filtered;
    if (alloc_json_memory(&doc_mem, strlen(wrapped)) != 0 ||
        api_qs_json_decode_object(&doc_mem, &doc_obj, wrapped) != 0 ||
        api_qs_object_get_array(&doc_obj, "endpoints", &endpoints) != 0 ||
        api_qs_array_create(&doc_mem, &filtered) != 0) {
        free(doc);
        free(wrapped);
        api_qs_memory_free(&req_mem);
        if (doc_mem.memory) api_qs_memory_free(&doc_mem);
        return NULL;
    }

    int endpoint_count = api_qs_array_get_length(&endpoints);
    for (int i = 0; i < endpoint_count; i++) {
        QS_JSON_ELEMENT_OBJECT endpoint;
        if (api_qs_array_get_object(&endpoints, i, &endpoint) != 0) continue;

        char* path_pattern = api_qs_object_get_string(&endpoint, "path_pattern");
        if (!path_pattern || strstr(path_pattern, filter_pattern) == NULL) continue;
        api_qs_array_push_object(&filtered, &endpoint);
    }

    char* encoded = api_qs_json_encode_array(&filtered, strlen(doc) + 1024);
    if (encoded) {
        size_t len = strlen(encoded);
        result = (char*)malloc(len + 1);
        if (result) {
            memcpy(result, encoded, len + 1);
            *out_size = len;
        }
    }

    free(doc);
    free(wrapped);
    api_qs_memory_free(&doc_mem);
    api_qs_memory_free(&req_mem);
    return result;
}

void agent_free_api_docs(void)
{
    if (g_api_docs_raw) { free(g_api_docs_raw); g_api_docs_raw = NULL; }
}

/* ---------------------------------------------------------------
 * Tool registry (static table — add new tools here)
 * --------------------------------------------------------------- */
static TOOL_ENTRY g_tool_registry[] = {
    { "file_list",         tool_file_list_execute        },
    { "file_read",         tool_file_read_execute        },
    { "file_write",        tool_file_write_execute       },
    { "file_edit",         tool_file_edit_execute        },
    { "file_search",       tool_file_search_execute      },
    { "grep_search",       tool_grep_search_execute      },
    { "http_request",      tool_http_request_execute     },
    { "url_whitelist_get", tool_url_whitelist_get_execute },
    { "url_whitelist_info_get", tool_url_whitelist_info_get_execute },
    { NULL,                NULL                          }
};

int agent_set_workspace_root(const char* path)
{
    const char* src = (path && path[0]) ? path : ".";
    char resolved[AGENT_PATH_MAX];

    if (realpath(src, resolved) == NULL) return -1;

    strncpy(g_agent_workspace_root, resolved, sizeof(g_agent_workspace_root) - 1);
    g_agent_workspace_root[sizeof(g_agent_workspace_root) - 1] = '\0';
    return 0;
}

const char* agent_get_workspace_root(void)
{
    return (g_agent_workspace_root[0] != '\0') ? g_agent_workspace_root : ".";
}

int agent_resolve_workspace_path(const char* user_path, char* out, size_t out_size)
{
    if (out == NULL || out_size == 0) return -1;

    const char* root = agent_get_workspace_root();
    const char* rel  = (user_path && user_path[0]) ? user_path : ".";

    if (rel[0] == '/') return -1;
    if (strstr(rel, "..") != NULL) return -1;

    while (rel[0] == '.' && rel[1] == '/') rel += 2;

    char joined[AGENT_PATH_MAX * 2];
    if (rel[0] == '\0' || (rel[0] == '.' && rel[1] == '\0')) {
        snprintf(joined, sizeof(joined), "%s", root);
    } else {
        snprintf(joined, sizeof(joined), "%s/%s", root, rel);
    }

    char resolved[AGENT_PATH_MAX];
    if (realpath(joined, resolved) == NULL) return -1;

    size_t root_len = strlen(root);
    if (strncmp(resolved, root, root_len) != 0 ||
        !((resolved[root_len] == '\0') || (resolved[root_len] == '/'))) {
        return -1;
    }

    strncpy(out, resolved, out_size - 1);
    out[out_size - 1] = '\0';
    return 0;
}

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

    conv->pending_verification = 0;
    conv->pending_verification_path[0] = '\0';
    conv->last_mutation_tool[0] = '\0';

    conv->context_capacity  = AGENT_CONTEXT_INIT_SIZE;
    conv->accumulated_context = (char*)calloc(1, conv->context_capacity);
    if (conv->accumulated_context == NULL) {
        free(conv);
        return NULL;
    }
    conv->context_length = 0;

    conv->item_capacity = 16;
    conv->items = (AGENT_CONVERSATION_ITEM*)calloc(conv->item_capacity,
                                                   sizeof(AGENT_CONVERSATION_ITEM));
    if (conv->items == NULL) {
        free(conv->accumulated_context);
        free(conv);
        return NULL;
    }
    conv->item_count = 0;

    conv->tool_use_file_list = 0;
    conv->tool_use_file_read = 0;

    return conv;
}

/* ---------------------------------------------------------------
 * agent_conversation_destroy
 * --------------------------------------------------------------- */
void agent_conversation_destroy(AGENT_CONVERSATION* conv)
{
    size_t i;

    if (conv == NULL) return;
    if (conv->accumulated_context != NULL) {
        free(conv->accumulated_context);
        conv->accumulated_context = NULL;
    }
    if (conv->items != NULL) {
        for (i = 0; i < conv->item_count; i++) {
            free(conv->items[i].label);
            free(conv->items[i].content);
        }
        free(conv->items);
        conv->items = NULL;
    }
    free(conv);
}

static char* agent_strdup_local(const char* src)
{
    size_t len;
    char* out;

    if (src == NULL) src = "";
    len = strlen(src);
    out = (char*)malloc(len + 1);
    if (out == NULL) return NULL;
    memcpy(out, src, len + 1);
    return out;
}

static AGENT_ITEM_TYPE agent_item_type_from_label(const char* label)
{
    if (label == NULL) return AGENT_ITEM_WARNING;
    if (strcmp(label, "user_query") == 0) return AGENT_ITEM_USER_QUERY;
    if (strcmp(label, "summary") == 0) return AGENT_ITEM_MODEL_ACTION;
    if (strcmp(label, "tool_result") == 0) return AGENT_ITEM_TOOL_RESULT;
    if (strncmp(label, "tool_call(", 10) == 0) return AGENT_ITEM_TOOL_CALL;
    return AGENT_ITEM_WARNING;
}

int agent_conversation_append_item(AGENT_CONVERSATION* conv,
                                   AGENT_ITEM_TYPE type,
                                   const char* label,
                                   const char* content)
{
    AGENT_CONVERSATION_ITEM* item;
    char* label_copy;
    char* content_copy;

    if (conv == NULL) return -1;

    if (conv->item_count >= conv->item_capacity) {
        size_t new_capacity = conv->item_capacity * 2;
        AGENT_CONVERSATION_ITEM* new_items =
            (AGENT_CONVERSATION_ITEM*)realloc(conv->items,
                                              new_capacity * sizeof(AGENT_CONVERSATION_ITEM));
        if (new_items == NULL) return -1;
        memset(new_items + conv->item_capacity, 0,
               (new_capacity - conv->item_capacity) * sizeof(AGENT_CONVERSATION_ITEM));
        conv->items = new_items;
        conv->item_capacity = new_capacity;
    }

    label_copy = agent_strdup_local(label != NULL ? label : "info");
    if (label_copy == NULL) return -1;
    content_copy = agent_strdup_local(content != NULL ? content : "");
    if (content_copy == NULL) {
        free(label_copy);
        return -1;
    }

    item = &conv->items[conv->item_count++];
    item->type = type;
    item->label = label_copy;
    item->content = content_copy;
    return 0;
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

    if (agent_conversation_append_item(conv,
                                       agent_item_type_from_label(safe_label),
                                       safe_label,
                                       content) != 0) {
        return -1;
    }

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
    snprintf(output, output_size, "{\"error\":\"unknown tool: %s\"}", tool_name);
    return -1;
}

/* ---------------------------------------------------------------
 * agent_parse_think_result
 *
 * Minimal JSON field extractor (no external library).
 * Looks for "action", "summary", "tool_name", "tool_args", "answer"
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

static const char* json_find_value_start(const char* json, const char* key)
{
    const char* p = json_find_toplevel_key(json, key);
    if (p == NULL) return NULL;

    p += strlen(key) + 2;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    if (*p != ':') return NULL;
    p++;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    return p;
}

/* Extract the value of a JSON string field: "key":"value"
 * Writes into out (max out_size bytes incl. NUL).
 * Returns 1 if found, 0 if not. */
static int json_extract_string(const char* json, const char* key,
                               char* out, size_t out_size)
{
    if (!json || !key || !out || out_size == 0) return 0;

    const char* p = json_find_value_start(json, key);
    if (p == NULL) return 0;
    if (*p != '"') return 0;
    p++;

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

    const char* p = json_find_value_start(json, key);
    if (p == NULL) return 0;
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
    json_extract_string(json_start, "action",     action_str,          sizeof(action_str));
    if (!json_extract_string(json_start, "summary", out->summary, sizeof(out->summary))) {
        json_extract_string(json_start, "thought", out->summary, sizeof(out->summary));
    }
    json_extract_string(json_start, "tool_name",  out->tool_call.tool_name,
                          sizeof(out->tool_call.tool_name));
    json_extract_object(json_start, "tool_args",  out->tool_call.json_args,
                          sizeof(out->tool_call.json_args));
    json_extract_string(json_start, "answer",     out->answer,         sizeof(out->answer));

    out->action = agent_parse_action_string(action_str);
    return (out->action != AGENT_ACTION_UNKNOWN) ? 0 : -1;
}

int agent_validate_think_result(const AGENT_THINK_RESULT* result,
                                char* error, size_t error_size)
{
    if (error != NULL && error_size > 0) {
        error[0] = '\0';
    }

    if (result == NULL) {
        if (error != NULL && error_size > 0) {
            snprintf(error, error_size, "missing think result");
        }
        return -1;
    }

    if (result->action == AGENT_ACTION_UNKNOWN) {
        if (error != NULL && error_size > 0) {
            snprintf(error, error_size, "invalid action");
        }
        return -1;
    }

    if (result->action == AGENT_ACTION_USE_TOOL) {
        if (result->tool_call.tool_name[0] == '\0') {
            if (error != NULL && error_size > 0) {
                snprintf(error, error_size, "use_tool requires tool_name");
            }
            return -1;
        }
        if (!agent_tool_is_registered(result->tool_call.tool_name)) {
            if (error != NULL && error_size > 0) {
                snprintf(error, error_size, "unknown tool: %s", result->tool_call.tool_name);
            }
            return -1;
        }
        if (result->tool_call.json_args[0] != '\0' &&
            result->tool_call.json_args[0] != '{') {
            if (error != NULL && error_size > 0) {
                snprintf(error, error_size, "tool_args must be a JSON object");
            }
            return -1;
        }
    }

    if (result->action == AGENT_ACTION_FINAL_ANSWER && result->answer[0] == '\0') {
        if (error != NULL && error_size > 0) {
            snprintf(error, error_size, "final_answer requires non-empty answer");
        }
        return -1;
    }

    return 0;
}

int agent_tool_requires_verification(const char* tool_name)
{
    if (tool_name == NULL) return 0;
    return (strcmp(tool_name, "file_write") == 0 ||
            strcmp(tool_name, "file_edit") == 0);
}

int agent_tool_args_extract_path(const char* json_args, char* out, size_t out_size)
{
    if (out == NULL || out_size == 0) return -1;
    out[0] = '\0';

    if (json_args == NULL || json_args[0] == '\0') return -1;
    return json_extract_string(json_args, "path", out, out_size) ? 0 : -1;
}
