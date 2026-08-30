/*
 * Copyright (c) Katsuya Owari
 */

#include "qs_llama_module.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if QS_LLM_MODULE_ENABLED && QS_LLM_MODULE_HAS_LLAMA_H && defined(QS_LLM_WITH_LLAMA_CPP) && (QS_LLM_WITH_LLAMA_CPP == 1)
#include <llama.h>
#include <pthread.h>

static pthread_mutex_t g_llama_runtime_mutex = PTHREAD_MUTEX_INITIALIZER;
static int g_llama_backend_initialized = 0;
static struct llama_model* g_llama_model = NULL;
static char g_llama_model_path[1024] = {0};

static int qs_llama_debug_enabled(void)
{
	const char* debug_env = getenv("QS_LLM_DEBUG");
	if (debug_env == NULL || debug_env[0] == '\0') {
		return 0;
	}
	if (debug_env[0] == '0' && debug_env[1] == '\0') {
		return 0;
	}
	return 1;
}

#define QS_LLAMA_DEBUG_PRINTF(...) do { if (qs_llama_debug_enabled()) { printf("[qs_llama_module] " __VA_ARGS__); fflush(stdout); } } while (0)

static int qs_llama_runtime_get_n_gpu_layers(void)
{
	const char* n_gpu_layers_env = getenv("QS_LLM_N_GPU_LAYERS");
	if (n_gpu_layers_env == NULL || n_gpu_layers_env[0] == '\0') {
		return 999;
	}
	int n_gpu_layers = atoi(n_gpu_layers_env);
	if (n_gpu_layers < 0) {
		return 0;
	}
	if (n_gpu_layers > 9999) {
		return 9999;
	}
	return n_gpu_layers;
}

static void qs_llama_runtime_unload_model_locked(void)
{
	if (g_llama_model != NULL) {
		llama_model_free(g_llama_model);
		g_llama_model = NULL;
	}
	g_llama_model_path[0] = '\0';
}

static int qs_llama_runtime_load_model_locked(const char* model_path)
{
	QS_LLAMA_DEBUG_PRINTF("load_model: requested path=%s\n", model_path != NULL ? model_path : "(null)");

	if (model_path == NULL || model_path[0] == '\0') {
		QS_LLAMA_DEBUG_PRINTF("load_model: invalid model path\n");
		return -1;
	}

	if (g_llama_model != NULL && strcmp(g_llama_model_path, model_path) == 0) {
		QS_LLAMA_DEBUG_PRINTF("load_model: already loaded, reusing model\n");
		return 0;
	}

	if (g_llama_backend_initialized == 0) {
		QS_LLAMA_DEBUG_PRINTF("load_model: llama_backend_init\n");
		llama_backend_init();
		g_llama_backend_initialized = 1;
	}

	qs_llama_runtime_unload_model_locked();

	struct llama_model_params mparams = llama_model_default_params();
	mparams.n_gpu_layers = qs_llama_runtime_get_n_gpu_layers();
	QS_LLAMA_DEBUG_PRINTF("load_model: n_gpu_layers=%d\n", mparams.n_gpu_layers);
	g_llama_model = llama_model_load_from_file(model_path, mparams);
	if (g_llama_model == NULL) {
		QS_LLAMA_DEBUG_PRINTF("load_model: llama_model_load_from_file failed\n");
		return -1;
	}

	snprintf(g_llama_model_path, sizeof(g_llama_model_path), "%s", model_path);
	QS_LLAMA_DEBUG_PRINTF("load_model: model loaded successfully\n");
	return 0;
}

static int qs_llama_runtime_prepare_internal(char* error_message, size_t error_message_size)
{
	const char* model_path = getenv("QS_LLM_MODEL_PATH");
	QS_LLAMA_DEBUG_PRINTF("prepare: QS_LLM_MODEL_PATH=%s\n", model_path != NULL ? model_path : "(null)");
	if (model_path == NULL || model_path[0] == '\0') {
		if (error_message != NULL && error_message_size > 0) {
			snprintf(error_message, error_message_size, "error: set QS_LLM_MODEL_PATH to a GGUF model");
		}
		return -1;
	}

	pthread_mutex_lock(&g_llama_runtime_mutex);
	int load_result = qs_llama_runtime_load_model_locked(model_path);
	pthread_mutex_unlock(&g_llama_runtime_mutex);

	if (load_result != 0) {
		QS_LLAMA_DEBUG_PRINTF("prepare: load model failed\n");
		if (error_message != NULL && error_message_size > 0) {
			snprintf(error_message, error_message_size, "error: failed to load model: %s", model_path);
		}
		return -1;
	}

	QS_LLAMA_DEBUG_PRINTF("prepare: model ready\n");
	return 0;
}
#endif

int qs_llama_module_is_available(void)
{
#if QS_LLM_MODULE_ENABLED && QS_LLM_MODULE_HAS_LLAMA_H && defined(QS_LLM_WITH_LLAMA_CPP) && (QS_LLM_WITH_LLAMA_CPP == 1)
	return 1;
#else
	return 0;
#endif
}

const char* qs_llama_module_build_info(void)
{
#if !QS_LLM_MODULE_ENABLED
	return "qs_llama_module: disabled (QS_LLM_MODULE_ENABLED=0)";
#elif QS_LLM_MODULE_HAS_LLAMA_H && defined(QS_LLM_WITH_LLAMA_CPP) && (QS_LLM_WITH_LLAMA_CPP == 1)
	return "qs_llama_module: llama.cpp runtime enabled";
#elif QS_LLM_MODULE_HAS_LLAMA_H
	return "qs_llama_module: llama.h found (build with QS_LLM_WITH_LLAMA_CPP=1 to enable runtime)";
#else
	return "qs_llama_module: llama.h not found (place llama.cpp and add include path)";
#endif
}

int qs_llama_module_prepare(void)
{
#if QS_LLM_MODULE_ENABLED && QS_LLM_MODULE_HAS_LLAMA_H && defined(QS_LLM_WITH_LLAMA_CPP) && (QS_LLM_WITH_LLAMA_CPP == 1)
	return qs_llama_runtime_prepare_internal(NULL, 0);
#else
	return 0;
#endif
}

void qs_llama_module_shutdown(void)
{
#if QS_LLM_MODULE_ENABLED && QS_LLM_MODULE_HAS_LLAMA_H && defined(QS_LLM_WITH_LLAMA_CPP) && (QS_LLM_WITH_LLAMA_CPP == 1)
	pthread_mutex_lock(&g_llama_runtime_mutex);
	qs_llama_runtime_unload_model_locked();
	if (g_llama_backend_initialized != 0) {
		llama_backend_free();
		g_llama_backend_initialized = 0;
	}
	pthread_mutex_unlock(&g_llama_runtime_mutex);
#endif
}

int qs_llm_http_stream_open(QS_EVENT_PARAMETER params, QS_LLM_HTTP_STREAM_CONTEXT* stream_context)
{
	if (params == NULL || stream_context == NULL) {
		return -1;
	}
	memset(stream_context, 0, sizeof(QS_LLM_HTTP_STREAM_CONTEXT));
	stream_context->server_context = api_qs_get_server_context(params);
	stream_context->connection_offset = api_qs_get_connection_offset(params);

	api_qs_send_response(
		params,
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/event-stream\r\n"
		"Cache-Control: no-cache\r\n"
		"Connection: keep-alive\r\n"
		"X-Accel-Buffering: no\r\n"
		"\r\n"
	);

	stream_context->is_open = 1;
	return 0;
}

static int qs_llm_http_stream_send_raw(QS_LLM_HTTP_STREAM_CONTEXT* stream_context, const char* payload)
{
	if (stream_context == NULL || payload == NULL) {
		return -1;
	}
	if (stream_context->is_open == 0 || stream_context->server_context == NULL) {
		return -1;
	}
	api_qs_send_response_by_connection_offset(stream_context->server_context, stream_context->connection_offset, payload);
	return 0;
}

static int qs_llm_http_stream_send_data_chunks(
	QS_LLM_HTTP_STREAM_CONTEXT* stream_context,
	const char* event_name,
	const char* data,
	size_t data_len,
	int* event_sent,
	int* wrote_data_line)
{
	while (data_len > 0) {
		size_t chunk_len = data_len;
		char line_buffer[4096];

		if (chunk_len > 3800) {
			chunk_len = 3800;
		}
		if (*event_sent == 0 && event_name != NULL && event_name[0] != '\0') {
			char event_buffer[128];
			snprintf(event_buffer, sizeof(event_buffer), "event: %s\n", event_name);
			if (-1 == qs_llm_http_stream_send_raw(stream_context, event_buffer)) {
				return -1;
			}
			*event_sent = 1;
		}
		memset(line_buffer, 0, sizeof(line_buffer));
		memcpy(line_buffer, "data: ", 6);
		memcpy(line_buffer + 6, data, chunk_len);
		memcpy(line_buffer + 6 + chunk_len, "\n", 1);
		if (-1 == qs_llm_http_stream_send_raw(stream_context, line_buffer)) {
			return -1;
		}
		*wrote_data_line = 1;
		data += chunk_len;
		data_len -= chunk_len;
	}

	return 0;
}

int qs_llm_http_stream_send_event(QS_LLM_HTTP_STREAM_CONTEXT* stream_context, const char* event_name, const char* data)
{
	if (stream_context == NULL || data == NULL) {
		return -1;
	}
	int wrote_data_line = 0;
	int event_sent = 0;

	const char* current = data;
	while (1) {
		const char* newline = strchr(current, '\n');
		if (newline == NULL) {
			if (current[0] != '\0') {
				if (-1 == qs_llm_http_stream_send_data_chunks(stream_context, event_name, current, strlen(current), &event_sent, &wrote_data_line)) {
					return -1;
				}
			}
			break;
		}

		size_t line_len = (size_t)(newline - current);
		if (line_len > 0) {
			if (-1 == qs_llm_http_stream_send_data_chunks(stream_context, event_name, current, line_len, &event_sent, &wrote_data_line)) {
				return -1;
			}
		}
		current = newline + 1;
	}

	if (wrote_data_line == 0) {
		return 0;
	}

	if (-1 == qs_llm_http_stream_send_raw(stream_context, "\n")) {
		return -1;
	}
	return 0;
}

int qs_llm_http_stream_send_token(QS_LLM_HTTP_STREAM_CONTEXT* stream_context, const char* token)
{
	if (token == NULL) {
		return -1;
	}
	return qs_llm_http_stream_send_event(stream_context, "token", token);
}

int qs_llm_http_stream_send_done(QS_LLM_HTTP_STREAM_CONTEXT* stream_context)
{
	if (-1 == qs_llm_http_stream_send_event(stream_context, "done", "[DONE]")) {
		return -1;
	}
	return 0;
}

int qs_llm_http_stream_close(QS_LLM_HTTP_STREAM_CONTEXT* stream_context)
{
	if (stream_context == NULL) {
		return -1;
	}
	stream_context->is_open = 0;
	return 0;
}

int qs_llama_module_stream_text(const char* prompt, QS_LLM_STREAM_TOKEN_CALLBACK callback, void* user_data)
{
	if (prompt == NULL || callback == NULL) {
		return -1;
	}

#if QS_LLM_MODULE_ENABLED && QS_LLM_MODULE_HAS_LLAMA_H && defined(QS_LLM_WITH_LLAMA_CPP) && (QS_LLM_WITH_LLAMA_CPP == 1)
	char prepare_error[1024];
	if (-1 == qs_llama_runtime_prepare_internal(prepare_error, sizeof(prepare_error))) {
		QS_LLAMA_DEBUG_PRINTF("stream_text: prepare failed: %s\n", prepare_error);
		if (-1 == callback(user_data, prepare_error, 1)) {
			return -1;
		}
		return 0;
	}
	QS_LLAMA_DEBUG_PRINTF("stream_text: start prompt_len=%zu\n", strlen(prompt));

	int max_tokens = 512;
	const char* max_tokens_env = getenv("QS_LLM_MAX_TOKENS");
	if (max_tokens_env != NULL && max_tokens_env[0] != '\0') {
		int value = atoi(max_tokens_env);
		if (value > 0 && value <= 4096) {
			max_tokens = value;
		}
	}

	int n_ctx = 8192;
	const char* n_ctx_env = getenv("QS_LLM_N_CTX");
	if (n_ctx_env != NULL && n_ctx_env[0] != '\0') {
		int value = atoi(n_ctx_env);
		if (value >= 256 && value <= 32768) {
			n_ctx = value;
		}
	}
	QS_LLAMA_DEBUG_PRINTF("stream_text: max_tokens=%d n_ctx=%d\n", max_tokens, n_ctx);

	pthread_mutex_lock(&g_llama_runtime_mutex);
	struct llama_model* model = g_llama_model;
	if (model == NULL) {
		QS_LLAMA_DEBUG_PRINTF("stream_text: model is NULL\n");
		pthread_mutex_unlock(&g_llama_runtime_mutex);
		if (-1 == callback(user_data, "error: model is not prepared", 1)) {
			return -1;
		}
		return 0;
	}

	const struct llama_vocab* vocab = llama_model_get_vocab(model);
	int32_t token_cap = (int32_t)strlen(prompt) + 8;
	if (token_cap < 64) {
		token_cap = 64;
	}
	llama_token* prompt_tokens = (llama_token*)malloc(sizeof(llama_token) * (size_t)token_cap);
	if (prompt_tokens == NULL) {
		QS_LLAMA_DEBUG_PRINTF("stream_text: malloc prompt_tokens failed, token_cap=%d\n", token_cap);
		pthread_mutex_unlock(&g_llama_runtime_mutex);
		if (-1 == callback(user_data, "error: memory allocation failed", 1)) {
			return -1;
		}
		return 0;
	}

	int32_t n_prompt = llama_tokenize(vocab, prompt, (int32_t)strlen(prompt), prompt_tokens, token_cap, true, true);
	QS_LLAMA_DEBUG_PRINTF("stream_text: tokenized initial n_prompt=%d token_cap=%d\n", n_prompt, token_cap);
	if (n_prompt < 0) {
		token_cap = -n_prompt;
		QS_LLAMA_DEBUG_PRINTF("stream_text: retry tokenization with token_cap=%d\n", token_cap);
		llama_token* retry_tokens = (llama_token*)realloc(prompt_tokens, sizeof(llama_token) * (size_t)token_cap);
		if (retry_tokens == NULL) {
			QS_LLAMA_DEBUG_PRINTF("stream_text: realloc prompt_tokens failed\n");
			free(prompt_tokens);
			pthread_mutex_unlock(&g_llama_runtime_mutex);
			if (-1 == callback(user_data, "error: memory allocation failed", 1)) {
				return -1;
			}
			return 0;
		}
		prompt_tokens = retry_tokens;
		n_prompt = llama_tokenize(vocab, prompt, (int32_t)strlen(prompt), prompt_tokens, token_cap, true, true);
		QS_LLAMA_DEBUG_PRINTF("stream_text: tokenized retry n_prompt=%d\n", n_prompt);
	}
	if (n_prompt <= 0) {
		QS_LLAMA_DEBUG_PRINTF("stream_text: tokenization failed n_prompt=%d\n", n_prompt);
		free(prompt_tokens);
		pthread_mutex_unlock(&g_llama_runtime_mutex);
		if (-1 == callback(user_data, "error: tokenization failed", 1)) {
			return -1;
		}
		return 0;
	}

	if (n_prompt >= n_ctx) {
		QS_LLAMA_DEBUG_PRINTF("stream_text: prompt too long n_prompt=%d n_ctx=%d, truncating\n", n_prompt, n_ctx);
		int32_t keep_tokens = n_ctx - 8;
		if (keep_tokens <= 0) {
			free(prompt_tokens);
			pthread_mutex_unlock(&g_llama_runtime_mutex);
			if (-1 == callback(user_data, "error: invalid context size", 1)) {
				return -1;
			}
			return 0;
		}
		int32_t drop_tokens = n_prompt - keep_tokens;
		memmove(prompt_tokens, prompt_tokens + drop_tokens, sizeof(llama_token) * (size_t)keep_tokens);
		n_prompt = keep_tokens;
	}

	struct llama_context_params cparams = llama_context_default_params();
	cparams.n_ctx = n_ctx;
	cparams.n_batch = n_ctx;
	cparams.n_threads = 4;
	cparams.n_threads_batch = 4;
	struct llama_context* ctx = llama_init_from_model(model, cparams);
	if (ctx == NULL) {
		QS_LLAMA_DEBUG_PRINTF("stream_text: llama_init_from_model failed\n");
		free(prompt_tokens);
		pthread_mutex_unlock(&g_llama_runtime_mutex);
		if (-1 == callback(user_data, "error: failed to create context", 1)) {
			return -1;
		}
		return 0;
	}

	struct llama_batch batch = llama_batch_get_one(prompt_tokens, n_prompt);
	if (llama_decode(ctx, batch) != 0) {
		QS_LLAMA_DEBUG_PRINTF("stream_text: initial llama_decode failed\n");
		free(prompt_tokens);
		pthread_mutex_unlock(&g_llama_runtime_mutex);
		llama_free(ctx);
		if (-1 == callback(user_data, "error: initial decode failed", 1)) {
			return -1;
		}
		return 0;
	}

	struct llama_sampler_chain_params sp = llama_sampler_chain_default_params();
	struct llama_sampler* sampler = llama_sampler_chain_init(sp);
	llama_sampler_chain_add(sampler, llama_sampler_init_top_k(40));
	llama_sampler_chain_add(sampler, llama_sampler_init_top_p(0.9f, 1));
	llama_sampler_chain_add(sampler, llama_sampler_init_temp(0.8f));
	llama_sampler_chain_add(sampler, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));

	int result = 0;
	for (int i = 0; i < max_tokens; i++) {
		llama_token token = llama_sampler_sample(sampler, ctx, -1);
		if (llama_vocab_is_eog(vocab, token)) {
			QS_LLAMA_DEBUG_PRINTF("stream_text: reached EOG at step=%d\n", i);
			break;
		}

		char piece[256];
		int32_t piece_len = llama_token_to_piece(vocab, token, piece, (int32_t)sizeof(piece) - 1, 0, true);
		if (piece_len < 0) {
			QS_LLAMA_DEBUG_PRINTF("stream_text: llama_token_to_piece failed at step=%d token=%d\n", i, (int)token);
			result = -1;
			break;
		}
		piece[piece_len] = '\0';

		if (-1 == callback(user_data, piece, 0)) {
			QS_LLAMA_DEBUG_PRINTF("stream_text: callback returned -1 at step=%d\n", i);
			result = -1;
			break;
		}

		llama_sampler_accept(sampler, token);
		int n_ctx_current = llama_n_ctx(ctx);
		int n_ctx_used = llama_memory_seq_pos_max(llama_get_memory(ctx), 0) + 1;
		if (n_ctx_used + 1 > n_ctx_current) {
			QS_LLAMA_DEBUG_PRINTF("stream_text: context full n_ctx_used=%d n_ctx_current=%d\n", n_ctx_used, n_ctx_current);
			break;
		}
		batch = llama_batch_get_one(&token, 1);
		if (llama_decode(ctx, batch) != 0) {
			QS_LLAMA_DEBUG_PRINTF("stream_text: llama_decode failed during generation at step=%d\n", i);
			result = -1;
			break;
		}
	}

	if (-1 == callback(user_data, "", 1)) {
		QS_LLAMA_DEBUG_PRINTF("stream_text: end callback returned -1\n");
		result = -1;
	}
	QS_LLAMA_DEBUG_PRINTF("stream_text: finished result=%d\n", result);

	llama_sampler_free(sampler);
	free(prompt_tokens);
	llama_free(ctx);
	pthread_mutex_unlock(&g_llama_runtime_mutex);
	return result;
#endif

	if (-1 == callback(user_data, "stub-response:", 0)) {
		return -1;
	}

	const char* current = prompt;
	char token[256];
	while (*current != '\0') {
		while (*current == ' ' || *current == '\t' || *current == '\r' || *current == '\n') {
			current++;
		}
		if (*current == '\0') {
			break;
		}
		size_t length = 0;
		while (current[length] != '\0' && current[length] != ' ' && current[length] != '\t' && current[length] != '\r' && current[length] != '\n') {
			length++;
			if (length >= sizeof(token) - 1) {
				break;
			}
		}
		memcpy(token, current, length);
		token[length] = '\0';
		if (-1 == callback(user_data, token, 0)) {
			return -1;
		}
		current += length;
	}

	if (-1 == callback(user_data, "", 1)) {
		return -1;
	}
	return 0;
}
