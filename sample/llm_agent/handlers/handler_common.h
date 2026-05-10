#ifndef HANDLER_COMMON_H
#define HANDLER_COMMON_H

#include "qs_api.h"
#include "qs_llama_module.h"
#include <stddef.h>

/* Prompt template loaded from prompt.conf at startup (defined in main.c). */
extern char* g_agent_prompt_template;

/* ---------------------------------------------------------------
 * LLM output accumulation buffer
 * Used with llm_token_callback as QS_LLM_STREAM_TOKEN_CALLBACK.
 * --------------------------------------------------------------- */
typedef struct {
    char*  buf;
    size_t len;
    size_t cap;
} LLM_BUFFER;

int  llm_buffer_init(LLM_BUFFER* b, size_t initial_cap);
void llm_buffer_free(LLM_BUFFER* b);
int  llm_token_callback(void* user_data, const char* token, int is_last);

/* ---------------------------------------------------------------
 * HTTP response helper
 * --------------------------------------------------------------- */
int send_json_response(QS_EVENT_PARAMETER params, int status, const char* body);

/* ---------------------------------------------------------------
 * JSON field extractors
 * --------------------------------------------------------------- */
int hc_extract_str(const char* json, const char* key, char* out, size_t out_size);
int hc_extract_int(const char* json, const char* key, int default_val);
int hc_extract_obj_raw(const char* json, const char* key, char* out, size_t out_size);

/* ---------------------------------------------------------------
 * JSON string escape
 * Returns malloc'd string. Caller must free().
 * --------------------------------------------------------------- */
char* hc_json_escape(const char* src);

/* ---------------------------------------------------------------
 * Agent think-prompt builder
 * Uses g_agent_prompt_template if set, falls back to built-in.
 * Returns malloc'd prompt string. Caller must free().
 * --------------------------------------------------------------- */
char* agent_build_think_prompt(const char* query, const char* context);

#endif /* HANDLER_COMMON_H */
