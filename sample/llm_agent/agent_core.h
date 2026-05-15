#ifndef AGENT_CORE_H
#define AGENT_CORE_H

#include <stddef.h>

/* ---------------------------------------------------------------
 * Limits & Defaults
 * --------------------------------------------------------------- */
#define AGENT_MAX_ITERATIONS       10
#define AGENT_TIMEOUT_SECONDS      60
#define AGENT_CONTEXT_MAX_SIZE     (1024 * 1024)    /* 1 MB */
#define AGENT_CONTEXT_INIT_SIZE    (1024 * 64)       /* 64 KB initial */
#define AGENT_FILE_READ_MAX_SIZE   (1024 * 1024)    /* 1 MB */
#define AGENT_FILE_READ_DEFAULT_LINES 300
#define AGENT_CONV_ID_LEN          64
#define AGENT_PATH_MAX             4096

/* ---------------------------------------------------------------
 * Action types returned by LLM
 * --------------------------------------------------------------- */
typedef enum {
    AGENT_ACTION_USE_TOOL     = 0,
    AGENT_ACTION_FINAL_ANSWER = 1,
    AGENT_ACTION_UNKNOWN      = -1,
} AGENT_ACTION;

/* ---------------------------------------------------------------
 * Tool call parsed from LLM output
 * --------------------------------------------------------------- */
typedef struct AGENT_TOOL_CALL {
    char tool_name[64];
    char json_args[4096];
} AGENT_TOOL_CALL;

/* ---------------------------------------------------------------
 * Result of one LLM "think" step
 * --------------------------------------------------------------- */
typedef struct AGENT_THINK_RESULT {
    AGENT_ACTION   action;
    char           thought[8192];
    AGENT_TOOL_CALL tool_call;
    char           answer[16384];
} AGENT_THINK_RESULT;

/* ---------------------------------------------------------------
 * Per-conversation state
 * --------------------------------------------------------------- */
typedef struct AGENT_CONVERSATION {
    char   conversation_id[AGENT_CONV_ID_LEN];
    int    iteration;
    int    max_iterations;
    char   user_query[2048];

    char*  accumulated_context;
    size_t context_length;
    size_t context_capacity;

    /* tool usage counters */
    int    tool_use_file_list;
    int    tool_use_file_read;
} AGENT_CONVERSATION;

/* ---------------------------------------------------------------
 * Tool executor function pointer type
 * --------------------------------------------------------------- */
typedef int (*ToolExecutor)(const char* json_args, char* output, size_t output_size);

/* ---------------------------------------------------------------
 * Tool registry entry
 * --------------------------------------------------------------- */
typedef struct TOOL_ENTRY {
    const char*  name;
    ToolExecutor executor;
} TOOL_ENTRY;

/* ---------------------------------------------------------------
 * agent_core.c  — public API
 * --------------------------------------------------------------- */

/* Conversation lifecycle */
AGENT_CONVERSATION* agent_conversation_create(const char* user_query, int max_iterations);
void                agent_conversation_destroy(AGENT_CONVERSATION* conv);

/* Append "label: content\n" to accumulated_context. Returns 0 on success, -1 on failure. */
int  agent_conversation_append_context(AGENT_CONVERSATION* conv,
                                       const char* label, const char* content);

/* Generate a unique conversation ID into out (must be >= AGENT_CONV_ID_LEN bytes) */
void agent_conversation_generate_id(char* out, size_t len);

/* Parse "use_tool" / "final_answer" string into AGENT_ACTION */
AGENT_ACTION agent_parse_action_string(const char* action_str);

/* Tool registry */
int agent_tool_execute(const char* tool_name, const char* json_args,
                       char* output, size_t output_size);
int agent_tool_is_registered(const char* tool_name);

/* Parse LLM JSON output into AGENT_THINK_RESULT. Returns 0 on success, -1 on error. */
int agent_parse_think_result(const char* llm_json, AGENT_THINK_RESULT* out);

/* Increment the tool usage counter for the given tool name */
void agent_conversation_count_tool(AGENT_CONVERSATION* conv, const char* tool_name);

/* Workspace root used by file tools. Defaults to current directory. */
extern char g_agent_workspace_root[AGENT_PATH_MAX];
int         agent_set_workspace_root(const char* path);
const char* agent_get_workspace_root(void);

/* Resolve a user-provided relative path inside the configured workspace root. */
int agent_resolve_workspace_path(const char* user_path, char* out, size_t out_size);

/* ---------------------------------------------------------------
 * URL whitelist management (loaded from url_whitelist.json)
 * --------------------------------------------------------------- */
int         agent_load_url_whitelist(const char* json_path);
int         agent_is_host_allowed(const char* host);
int         agent_get_whitelist_count(void);
const char* agent_get_whitelist_host(int index);
const char* agent_get_whitelist_desc(int index);
void        agent_free_url_whitelist(void);

/* API documentation (loaded from api_docs.json) */
int         agent_load_api_docs(const char* json_path);
char*       agent_get_api_doc_for_host(const char* host, size_t* out_size);
char*       agent_filter_api_docs(const char* host_json_str, const char* filter_pattern, size_t* out_size);
void        agent_free_api_docs(void);

#endif /* AGENT_CORE_H */
