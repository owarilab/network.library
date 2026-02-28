/*
 * Copyright (c) Katsuya Owari
 */

#ifdef __cplusplus
extern "C"{
#endif

#ifndef _QS_LLAMA_MODULE_H_
#define _QS_LLAMA_MODULE_H_

#include "qs_core.h"
#include "qs_api.h"

// build option (can be overridden by compiler definitions)
#ifndef QS_LLM_MODULE_ENABLED
#define QS_LLM_MODULE_ENABLED 1
#endif

// Detect llama.cpp C header availability (llama.h)
#if QS_LLM_MODULE_ENABLED
# if defined(__has_include)
#  if __has_include(<llama.h>)
#   define QS_LLM_MODULE_HAS_LLAMA_H 1
#  else
#   define QS_LLM_MODULE_HAS_LLAMA_H 0
#  endif
# else
#  define QS_LLM_MODULE_HAS_LLAMA_H 0
# endif
#else
# define QS_LLM_MODULE_HAS_LLAMA_H 0
#endif

// Returns 1 if the module was built with llama.h visible, otherwise 0.
int qs_llama_module_is_available(void);

// Human readable build info.
const char* qs_llama_module_build_info(void);

// Prepare runtime resources and resident model (no-op on stub build).
int qs_llama_module_prepare(void);

// Release runtime resources and resident model (safe to call multiple times).
void qs_llama_module_shutdown(void);

typedef struct QS_LLM_HTTP_STREAM_CONTEXT
{
	QS_SERVER_CONTEXT* server_context;
	uint32_t connection_offset;
	int is_open;
} QS_LLM_HTTP_STREAM_CONTEXT;

typedef int (*QS_LLM_STREAM_TOKEN_CALLBACK)(void* user_data, const char* token, int is_last);

// Open SSE stream for current HTTP request.
int qs_llm_http_stream_open(QS_EVENT_PARAMETER params, QS_LLM_HTTP_STREAM_CONTEXT* stream_context);

// Send one SSE event with optional event name.
int qs_llm_http_stream_send_event(QS_LLM_HTTP_STREAM_CONTEXT* stream_context, const char* event_name, const char* data);

// Send token as SSE event (event: token).
int qs_llm_http_stream_send_token(QS_LLM_HTTP_STREAM_CONTEXT* stream_context, const char* token);

// Send done event and terminal marker.
int qs_llm_http_stream_send_done(QS_LLM_HTTP_STREAM_CONTEXT* stream_context);

// Close stream state on server side (HTTP socket remains managed by core loop).
int qs_llm_http_stream_close(QS_LLM_HTTP_STREAM_CONTEXT* stream_context);

// Minimal text streaming entrypoint. Currently a lightweight callback-based implementation.
int qs_llama_module_stream_text(const char* prompt, QS_LLM_STREAM_TOKEN_CALLBACK callback, void* user_data);

#endif /* _QS_LLAMA_MODULE_H_ */

#ifdef __cplusplus
}
#endif
