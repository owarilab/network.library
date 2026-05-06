#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "qs_api.h"
#include "qs_llama_module.h"

static int on_http_event(QS_EVENT_PARAMETER params);
static int load_system_prompt(void);
static char* make_effective_prompt(const char* user_prompt);
static char* make_json_mode_prompt(const char* user_prompt);
static char* make_json_repair_prompt(const char* broken_text);
static int send_json_http_response(QS_EVENT_PARAMETER params, int status_code, const char* body);
static int validate_json_object_text(const char* json_text);
static char* extract_json_object_text(const char* text);
static char* json_escape_string(const char* text);

typedef struct JSON_GEN_CONTEXT {
	char* buffer;
	size_t length;
	size_t capacity;
} JSON_GEN_CONTEXT;

static int on_json_token(void* user_data, const char* token, int is_last);

static char* g_system_prompt = NULL;

static int on_stream_token(void* user_data, const char* token, int is_last)
{
	QS_LLM_HTTP_STREAM_CONTEXT* stream = (QS_LLM_HTTP_STREAM_CONTEXT*)user_data;
	if (is_last) {
		return qs_llm_http_stream_send_done(stream);
	}
	if (token == NULL || token[0] == '\0') {
		return 0;
	}
	if (-1 == qs_llm_http_stream_send_token(stream, token)) {
		return -1;
	}
	return 0;
}

int main(int argc, char* argv[], char* envp[])
{
	(void)argc;
	(void)argv;
	(void)envp;

	if (-1 == load_system_prompt()) {
		return -1;
	}

	if (-1 == qs_llama_module_prepare()) {
		return -1;
	}

	QS_SERVER_CONTEXT* context = 0;
	if (0 > api_qs_server_init(&context, 8080, 100, QS_SERVER_TYPE_HTTP)) {
		qs_llama_module_shutdown();
		return -1;
	}
	if (-1 == api_qs_server_create_router(context)) {
		qs_llama_module_shutdown();
		return -1;
	}

	api_qs_set_on_http_event(context, on_http_event);

	for (;;) {
		api_qs_update(context);
		api_qs_sleep(context);
	}

	api_qs_free(context);
	qs_llama_module_shutdown();
	if (g_system_prompt != NULL) {
		free(g_system_prompt);
		g_system_prompt = NULL;
	}
	return 0;
}

static int load_system_prompt(void)
{
	const char* prompt_file_path = getenv("QS_LLM_SYSTEM_PROMPT_FILE");
	if (prompt_file_path == NULL || prompt_file_path[0] == '\0') {
		prompt_file_path = "./system_prompt.md";
	}

	FILE* fp = fopen(prompt_file_path, "rb");
	if (fp == NULL) {
		if (strcmp(prompt_file_path, "./system_prompt.md") == 0) {
			prompt_file_path = "./system_prompt.txt";
			fp = fopen(prompt_file_path, "rb");
		}
	}
	if (fp == NULL) {
		return 0;
	}
	if (fseek(fp, 0, SEEK_END) != 0) {
		fclose(fp);
		return -1;
	}
	long file_size = ftell(fp);
	if (file_size <= 0) {
		fclose(fp);
		return 0;
	}
	if (fseek(fp, 0, SEEK_SET) != 0) {
		fclose(fp);
		return -1;
	}

	g_system_prompt = (char*)malloc((size_t)file_size + 1);
	if (g_system_prompt == NULL) {
		fclose(fp);
		return -1;
	}
	size_t read_size = fread(g_system_prompt, 1, (size_t)file_size, fp);
	fclose(fp);
	if (read_size != (size_t)file_size) {
		free(g_system_prompt);
		g_system_prompt = NULL;
		return -1;
	}
	g_system_prompt[file_size] = '\0';

	while (file_size > 0 && (g_system_prompt[file_size - 1] == '\n' || g_system_prompt[file_size - 1] == '\r')) {
		g_system_prompt[file_size - 1] = '\0';
		file_size--;
	}

	if (g_system_prompt[0] == '\0') {
		free(g_system_prompt);
		g_system_prompt = NULL;
	}

	return 0;
}

static char* make_effective_prompt(const char* user_prompt)
{
	if (user_prompt == NULL) {
		return NULL;
	}
	if (g_system_prompt == NULL || g_system_prompt[0] == '\0') {
		char* copied = (char*)malloc(strlen(user_prompt) + 1);
		if (copied == NULL) {
			return NULL;
		}
		strcpy(copied, user_prompt);
		return copied;
	}

	const char* prefix1 = "[SYSTEM]\n";
	const char* prefix2 = "\n\n[USER]\n";
	const char* suffix = "\n\n[ASSISTANT]\n";
	const size_t total_size = strlen(prefix1) + strlen(g_system_prompt) + strlen(prefix2) + strlen(user_prompt) + strlen(suffix) + 1;
	char* prompt = (char*)malloc(total_size);
	if (prompt == NULL) {
		return NULL;
	}
	prompt[0] = '\0';
	strcat(prompt, prefix1);
	strcat(prompt, g_system_prompt);
	strcat(prompt, prefix2);
	strcat(prompt, user_prompt);
	strcat(prompt, suffix);
	return prompt;
}

static char* make_json_mode_prompt(const char* user_prompt)
{
	char* base_prompt = make_effective_prompt(user_prompt);
	if (base_prompt == NULL) {
		return NULL;
	}
	// old prompt:
	// const char* suffix = "\n\n[FORMAT]\nReturn ONLY one valid JSON object. No markdown, no explanation, no code fence.";
	const char* suffix =
		"\n\n[FORMAT]\n"
		"Return ONLY one valid JSON object.\n"
		"Do not output markdown. Do not output code fences. Do not output comments.\n"
		"The first character must be '{' and the last character must be '}'.\n"
		"All keys and string values must use double quotes.\n"
		"Do not include any explanation before or after the JSON object.";
	size_t total = strlen(base_prompt) + strlen(suffix) + 1;
	char* json_prompt = (char*)malloc(total);
	if (json_prompt == NULL) {
		free(base_prompt);
		return NULL;
	}
	json_prompt[0] = '\0';
	strcat(json_prompt, base_prompt);
	strcat(json_prompt, suffix);
	free(base_prompt);
	return json_prompt;
}

static char* make_json_repair_prompt(const char* broken_text)
{
	if (broken_text == NULL) {
		return NULL;
	}
	// old prompt:
	// const char* prefix = "[TASK]\nConvert the following text to one valid JSON object only. No markdown, no explanation.\n\n[INPUT]\n";
	const char* prefix =
		"[TASK]\n"
		"Convert the following text to one valid JSON object only.\n"
		"Output only the JSON object.\n"
		"Do not output markdown/code fences/comments/explanations.\n"
		"The first character must be '{' and the last character must be '}'.\n\n"
		"[INPUT]\n";
	const size_t total = strlen(prefix) + strlen(broken_text) + 1;
	char* prompt = (char*)malloc(total);
	if (prompt == NULL) {
		return NULL;
	}
	prompt[0] = '\0';
	strcat(prompt, prefix);
	strcat(prompt, broken_text);
	return prompt;
}

static int send_json_http_response(QS_EVENT_PARAMETER params, int status_code, const char* body)
{
	if (body == NULL) {
		return -1;
	}
	const char* status_text = "OK";
	if (status_code == 400) {
		status_text = "Bad Request";
	} else if (status_code == 500) {
		status_text = "Internal Server Error";
	}
	char header[512];
	snprintf(
		header,
		sizeof(header),
		"HTTP/1.1 %d %s\r\n"
		"Content-Type: application/json; charset=utf-8\r\n"
		"Content-Length: %zu\r\n"
		"Connection: close\r\n"
		"\r\n",
		status_code,
		status_text,
		strlen(body)
	);
	size_t response_size = strlen(header) + strlen(body) + 1;
	char* response = (char*)malloc(response_size);
	if (response == NULL) {
		return -1;
	}
	response[0] = '\0';
	strcat(response, header);
	strcat(response, body);
	api_qs_send_response(params, response);
	free(response);
	return 0;
}

static int validate_json_object_text(const char* json_text)
{
	if (json_text == NULL || json_text[0] == '\0') {
		return -1;
	}
	QS_MEMORY_CONTEXT memory;
	if (-1 == api_qs_memory_alloc(&memory, 1024 * 1024)) {
		return -1;
	}
	QS_JSON_ELEMENT_OBJECT object;
	int result = api_qs_json_decode_object(&memory, &object, json_text);
	api_qs_memory_free(&memory);
	return (result == 0) ? 0 : -1;
}

static char* extract_json_object_text(const char* text)
{
	if (text == NULL) {
		return NULL;
	}
	const char* start = NULL;
	const char* end = NULL;
	int depth = 0;
	int in_string = 0;
	int escaped = 0;
	for (const char* p = text; *p != '\0'; p++) {
		char c = *p;
		if (in_string) {
			if (escaped) {
				escaped = 0;
				continue;
			}
			if (c == '\\') {
				escaped = 1;
				continue;
			}
			if (c == '"') {
				in_string = 0;
			}
			continue;
		}

		if (c == '"') {
			in_string = 1;
			continue;
		}
		if (c == '{') {
			if (depth == 0) {
				start = p;
			}
			depth++;
			continue;
		}
		if (c == '}') {
			if (depth <= 0) {
				return NULL;
			}
			depth--;
			if (depth == 0 && start != NULL) {
				end = p;
				break;
			}
		}
	}

	if (start == NULL || end == NULL || end < start) {
		return NULL;
	}
	size_t size = (size_t)(end - start + 1);
	char* out = (char*)malloc(size + 1);
	if (out == NULL) {
		return NULL;
	}
	memcpy(out, start, size);
	out[size] = '\0';
	return out;
}

static char* json_escape_string(const char* text)
{
	if (text == NULL) {
		return NULL;
	}
	size_t src_len = strlen(text);
	size_t cap = src_len * 2 + 1;
	char* out = (char*)malloc(cap);
	if (out == NULL) {
		return NULL;
	}
	size_t w = 0;
	for (size_t i = 0; i < src_len; i++) {
		char c = text[i];
		if (c == '"' || c == '\\') {
			out[w++] = '\\';
			out[w++] = c;
		} else if (c == '\n') {
			out[w++] = '\\';
			out[w++] = 'n';
		} else if (c == '\r') {
			out[w++] = '\\';
			out[w++] = 'r';
		} else if (c == '\t') {
			out[w++] = '\\';
			out[w++] = 't';
		} else {
			out[w++] = c;
		}
	}
	out[w] = '\0';
	return out;
}

static int on_json_token(void* user_data, const char* token, int is_last)
{
	JSON_GEN_CONTEXT* context = (JSON_GEN_CONTEXT*)user_data;
	if (context == NULL) {
		return -1;
	}
	if (is_last || token == NULL || token[0] == '\0') {
		return 0;
	}
	size_t token_len = strlen(token);
	size_t required = context->length + token_len + 1;
	if (required > context->capacity) {
		size_t new_capacity = context->capacity;
		while (new_capacity < required) {
			new_capacity *= 2;
		}
		char* resized = (char*)realloc(context->buffer, new_capacity);
		if (resized == NULL) {
			return -1;
		}
		context->buffer = resized;
		context->capacity = new_capacity;
	}
	memcpy(context->buffer + context->length, token, token_len);
	context->length += token_len;
	context->buffer[context->length] = '\0';
	return 0;
}

static int on_http_event(QS_EVENT_PARAMETER params)
{
	const char* method = api_qs_get_http_method(params);
	const char* path = api_qs_get_http_path(params);

	if (method == NULL || path == NULL) {
		return 404;
	}

	if (strcmp(method, "POST") == 0 && strcmp(path, "/api/llm/stream") == 0) {
		const char* prompt = api_qs_get_http_post_parameter(params, "q");
		if (prompt == NULL || prompt[0] == '\0') {
			prompt = api_qs_get_http_post_body(params);
		}
		if (prompt == NULL || prompt[0] == '\0') {
			prompt = "hello from llm stream";
		}

		char* effective_prompt = make_effective_prompt(prompt);
		if (effective_prompt == NULL) {
			return 500;
		}

		QS_LLM_HTTP_STREAM_CONTEXT stream;
		if (-1 == qs_llm_http_stream_open(params, &stream)) {
			free(effective_prompt);
			return 500;
		}

		const char* meta = (g_system_prompt != NULL) ? "system_prompt=on" : "system_prompt=off";
		if (-1 == qs_llm_http_stream_send_event(&stream, "meta", meta)) {
			qs_llm_http_stream_close(&stream);
			free(effective_prompt);
			return 500;
		}

		if (-1 == qs_llama_module_stream_text(effective_prompt, on_stream_token, &stream)) {
			qs_llm_http_stream_send_event(&stream, "error", "stream failed");
			qs_llm_http_stream_send_done(&stream);
		}

		free(effective_prompt);
		qs_llm_http_stream_close(&stream);
		return 200;
	}

	if (strcmp(method, "POST") == 0 && strcmp(path, "/api/llm") == 0) {
		const char* prompt = api_qs_get_http_post_parameter(params, "q");
		if (prompt == NULL || prompt[0] == '\0') {
			prompt = api_qs_get_http_post_body(params);
		}
		if (prompt == NULL || prompt[0] == '\0') {
			prompt = "hello from llm";
		}

		char* effective_prompt = make_effective_prompt(prompt);
		if (effective_prompt == NULL) {
			return 500;
		}

		JSON_GEN_CONTEXT* ctx = (JSON_GEN_CONTEXT*)malloc(sizeof(JSON_GEN_CONTEXT));
		if (ctx == NULL) {
			free(effective_prompt);
			send_json_http_response(params, 500, "{\"ok\":false,\"error\":\"memory allocation failed\"}");
			return 500;
		}
		ctx->capacity = 4096;
		ctx->length = 0;
		ctx->buffer = (char*)malloc(ctx->capacity);
		if (ctx->buffer == NULL) {
			free(ctx);
			free(effective_prompt);
			send_json_http_response(params, 500, "{\"ok\":false,\"error\":\"memory allocation failed\"}");
			return 500;
		}
		ctx->buffer[0] = '\0';

		int stream_result = qs_llama_module_stream_text(effective_prompt, on_json_token, ctx);
		free(effective_prompt);

		if (stream_result == -1) {
			free(ctx->buffer);
			free(ctx);
			send_json_http_response(params, 500, "{\"ok\":false,\"error\":\"generation failed\"}");
			return 500;
		}

		const char* status_text = "OK";
		char header[512];
		snprintf(
			header,
			sizeof(header),
			"HTTP/1.1 200 %s\r\n"
			"Content-Type: text/plain; charset=utf-8\r\n"
			"Content-Length: %zu\r\n"
			"Connection: close\r\n"
			"\r\n",
			status_text,
			ctx->length
		);
		size_t response_size = strlen(header) + ctx->length + 1;
		char* response = (char*)malloc(response_size);
		if (response == NULL) {
			free(ctx->buffer);
			free(ctx);
			return -1;
		}
		response[0] = '\0';
		strcat(response, header);
		memcpy(response + strlen(header), ctx->buffer, ctx->length);
		api_qs_send_response(params, response);
		free(response);
		free(ctx->buffer);
		free(ctx);
		return 200;
	}

	if (strcmp(method, "POST") == 0 && strcmp(path, "/api/llm/json") == 0) {
		const char* prompt = api_qs_get_http_post_parameter(params, "q");
		if (prompt == NULL || prompt[0] == '\0') {
			prompt = api_qs_get_http_post_body(params);
		}
		if (prompt == NULL || prompt[0] == '\0') {
			send_json_http_response(params, 400, "{\"ok\":false,\"error\":\"empty prompt\"}");
			return 400;
		}

		char* json_prompt = make_json_mode_prompt(prompt);
		if (json_prompt == NULL) {
			send_json_http_response(params, 500, "{\"ok\":false,\"error\":\"failed to build prompt\"}");
			return 500;
		}

		JSON_GEN_CONTEXT context;
		context.capacity = 1024 * 4;
		context.length = 0;
		context.buffer = (char*)malloc(context.capacity);
		if (context.buffer == NULL) {
			free(json_prompt);
			send_json_http_response(params, 500, "{\"ok\":false,\"error\":\"memory allocation failed\"}");
			return 500;
		}
		context.buffer[0] = '\0';

		int stream_result = qs_llama_module_stream_text(json_prompt, on_json_token, &context);
		free(json_prompt);

		if (stream_result == -1) {
			free(context.buffer);
			send_json_http_response(params, 500, "{\"ok\":false,\"error\":\"generation failed\"}");
			return 500;
		}

		char* json_body = context.buffer;
		int is_valid = 0;
		char* extracted = extract_json_object_text(json_body);
		if (extracted != NULL && validate_json_object_text(extracted) == 0) {
			free(context.buffer);
			json_body = extracted;
			is_valid = 1;
		} else if (extracted != NULL) {
			free(extracted);
		}

		if (!is_valid) {
			char* repair_prompt = make_json_repair_prompt(json_body);
			if (repair_prompt != NULL) {
				JSON_GEN_CONTEXT repair_context;
				repair_context.capacity = 1024 * 4;
				repair_context.length = 0;
				repair_context.buffer = (char*)malloc(repair_context.capacity);
				if (repair_context.buffer != NULL) {
					repair_context.buffer[0] = '\0';
					if (0 == qs_llama_module_stream_text(repair_prompt, on_json_token, &repair_context)) {
						char* repair_extracted = extract_json_object_text(repair_context.buffer);
						if (repair_extracted != NULL && validate_json_object_text(repair_extracted) == 0) {
							free(json_body);
							free(repair_context.buffer);
							json_body = repair_extracted;
							is_valid = 1;
						} else {
							if (repair_extracted != NULL) {
								free(repair_extracted);
							}
							free(repair_context.buffer);
						}
					} else {
						free(repair_context.buffer);
					}
				}
				free(repair_prompt);
			}
		}

		if (!is_valid) {
			char* escaped = json_escape_string(json_body);
			free(json_body);
			if (escaped == NULL) {
				send_json_http_response(params, 500, "{\"ok\":false,\"error\":\"invalid json output\"}");
				return 500;
			}
			size_t fallback_size = strlen(escaped) + 128;
			char* fallback = (char*)malloc(fallback_size);
			if (fallback == NULL) {
				free(escaped);
				send_json_http_response(params, 500, "{\"ok\":false,\"error\":\"invalid json output\"}");
				return 500;
			}
			snprintf(fallback, fallback_size, "{\"ok\":false,\"mode\":\"fallback_raw_text\",\"text\":\"%s\"}", escaped);
			free(escaped);
			send_json_http_response(params, 200, fallback);
			free(fallback);
			return 200;
		}

		send_json_http_response(params, 200, json_body);
		free(json_body);
		return 200;
	}

	if (strcmp(method, "GET") == 0 && strcmp(path, "/healthz") == 0) {
		api_qs_send_response(params, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 2\r\n\r\nok");
		return 200;
	}

	return 404;
}
