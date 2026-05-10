#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include "qs_api.h"
#include "qs_llama_module.h"
#include "qs_embedding_module.h"

static int on_http_event(QS_EVENT_PARAMETER params);
static int load_system_prompt(void);
static char* make_effective_prompt(const char* user_prompt);
static char* make_json_mode_prompt(const char* user_prompt);
static char* make_json_repair_prompt(const char* broken_text);
static int send_json_http_response(QS_EVENT_PARAMETER params, int status_code, const char* body);
static int validate_json_object_text(const char* json_text);
static char* extract_json_object_text(const char* text);
static char* json_escape_string(const char* text);
static void strip_markdown_code_fences_in_place(char* text);
static int handle_embed_store(QS_EVENT_PARAMETER params);
static int handle_embed_search(QS_EVENT_PARAMETER params);
static int handle_embed_delete(QS_EVENT_PARAMETER params);
static char* make_rag_prompt(const char* user_query, const int64_t* source_ids,
                             const float* source_scores, char** source_texts,
                             int source_count, const char* ctx_prefix, const char* ctx_suffix);
static int handle_rag(QS_EVENT_PARAMETER params);
static int handle_rag_stream(QS_EVENT_PARAMETER params);

typedef struct JSON_GEN_CONTEXT {
	char* buffer;
	size_t length;
	size_t capacity;
} JSON_GEN_CONTEXT;

typedef struct RAG_GEN_CONTEXT {
	char* buffer;
	size_t length;
	size_t capacity;
} RAG_GEN_CONTEXT;

static int on_json_token(void* user_data, const char* token, int is_last);
static int on_rag_token(void* user_data, const char* token, int is_last);
static int on_rag_stream_token(void* user_data, const char* token, int is_last);

static char* g_system_prompt = NULL;
static int g_embedding_enabled = 0;
static QS_EMBEDDING_STORE* g_embedding_store = NULL;

QS_MEMORY_CONTEXT g_temporary_memory;

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

static int on_rag_stream_token(void* user_data, const char* token, int is_last)
{
	QS_LLM_HTTP_STREAM_CONTEXT* stream = (QS_LLM_HTTP_STREAM_CONTEXT*)user_data;
	if (is_last || token == NULL || token[0] == '\0') {
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

	if(-1==api_qs_memory_alloc(&g_temporary_memory,1024*1024*8)){
		printf("api_qs_memory_alloc failed\n");
		return -1;
	}

	int server_port = 8080;
	int scheduler_mode = QS_SCHEDULER_MODE_LOW;
	int32_t max_connection = 10;

	QS_SERVER_SCRIPT_CONTEXT script;
	if(-1==api_qs_script_read_file(&g_temporary_memory, &script, "./settings.conf")){return -1;}
	if(-1==api_qs_script_run(&script)){return -1;}
	if(0!=api_qs_script_get_parameter(&script,"server_port")){
		server_port = atoi(api_qs_script_get_parameter(&script,"server_port"));
	}
	if(0!=api_qs_script_get_parameter(&script,"scheduler_mode")){
		const char* sm = api_qs_script_get_parameter(&script,"scheduler_mode");
		if(!strcmp(sm,"high"))       scheduler_mode = QS_SCHEDULER_MODE_HIGH;
		else if(!strcmp(sm,"middle")) scheduler_mode = QS_SCHEDULER_MODE_MIDDLE;
		else                          scheduler_mode = QS_SCHEDULER_MODE_LOW;
	}
	if(0!=api_qs_script_get_parameter(&script,"max_connection")){
		int v = atoi(api_qs_script_get_parameter(&script,"max_connection"));
		if(v < 10) v = 10;
		if(v > 1000) v = 1000;
		max_connection = (int32_t)v;
	}

	if (-1 == load_system_prompt()) {
		return -1;
	}

	if (-1 == qs_llama_module_prepare()) {
		return -1;
	}

	/* Initialize embedding module if model and database paths are provided */
	const char* embedding_model = getenv("QS_EMBEDDING_MODEL_PATH");
	const char* embedding_db = getenv("QS_EMBEDDING_DB_PATH");
	if (embedding_model != NULL && embedding_model[0] != '\0' && 
		embedding_db != NULL && embedding_db[0] != '\0') {
		int embed_result = qs_embedding_prepare(embedding_model, embedding_db, &g_embedding_store);
		if (0 == embed_result) {
			g_embedding_enabled = 1;
			printf("[Main] Embedding module initialized: model=%s db=%s\n", embedding_model, embedding_db);
		} else {
			printf("[Main] Failed to initialize embedding module\n");
		}
	}

	printf("[Main] Starting server on port %d...\n", server_port);

	QS_SERVER_CONTEXT* context = 0;
	int init_error = 1;
	do{
		if (0 > api_qs_server_init(&context, server_port, max_connection, QS_SERVER_TYPE_HTTP)) { break; }
		if(-1==api_qs_set_scheduler(context,scheduler_mode)){ break; }
		if (-1 == api_qs_server_create_router(context)) { break; }
		init_error = 0;
	}while(0);

	if(init_error){
		printf("[Main] Server initialization failed\n");
		qs_llama_module_shutdown();
		if (g_embedding_enabled) {
			qs_embedding_shutdown(g_embedding_store);
			g_embedding_store = NULL;
		}
		return -1;
	}

	api_qs_set_on_http_event(context, on_http_event);

	api_qs_memory_clean(&g_temporary_memory);

	for (;;) {
		api_qs_update(context);
		api_qs_sleep(context);
	}

	api_qs_free(context);
	qs_llama_module_shutdown();
	if (g_embedding_enabled) {
		qs_embedding_shutdown(g_embedding_store);
		g_embedding_store = NULL;
	}
	if (g_system_prompt != NULL) {
		free(g_system_prompt);
		g_system_prompt = NULL;
	}
	api_qs_memory_free(&g_temporary_memory);
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

static void strip_markdown_code_fences_in_place(char* text)
{
	char* content_start;
	char* end;

	if (text == NULL || strncmp(text, "```", 3) != 0) {
		return;
	}

	content_start = strchr(text, '\n');
	if (content_start == NULL) {
		return;
	}
	content_start++;
	end = strstr(content_start, "```");
	if (end != NULL) {
		size_t content_len = (size_t)(end - content_start);
		memmove(text, content_start, content_len);
		text[content_len] = '\0';
	} else {
		memmove(text, content_start, strlen(content_start) + 1);
	}
}

static int parse_positive_int64_strict(const char* text, int64_t* out_value)
{
	char* end_ptr;
	long long value;

	if (text == NULL || out_value == NULL || text[0] == '\0') {
		return -1;
	}
	errno = 0;
	value = strtoll(text, &end_ptr, 10);
	if (errno != 0 || end_ptr == text || end_ptr[0] != '\0' || value <= 0) {
		return -1;
	}
	*out_value = (int64_t)value;
	return 0;
}

static int parse_int_in_range_strict(const char* text, int min_value, int max_value, int* out_value)
{
	char* end_ptr;
	long value;

	if (text == NULL || out_value == NULL || text[0] == '\0') {
		return -1;
	}
	errno = 0;
	value = strtol(text, &end_ptr, 10);
	if (errno != 0 || end_ptr == text || end_ptr[0] != '\0') {
		return -1;
	}
	if (value < (long)min_value || value > (long)max_value) {
		return -1;
	}
	*out_value = (int)value;
	return 0;
}

#ifdef QS_LLM_HTTP_STREAM_TESTING
int qs_test_parse_positive_int64_strict(const char* text, int64_t* out_value)
{
	return parse_positive_int64_strict(text, out_value);
}

int qs_test_parse_int_in_range_strict(const char* text, int min_value, int max_value, int* out_value)
{
	return parse_int_in_range_strict(text, min_value, max_value, out_value);
}

void qs_test_strip_markdown_code_fences_in_place(char* text)
{
	strip_markdown_code_fences_in_place(text);
}
#endif

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

static int on_rag_token(void* user_data, const char* token, int is_last)
{
	RAG_GEN_CONTEXT* ctx = (RAG_GEN_CONTEXT*)user_data;
	if (ctx == NULL) return -1;
	if (is_last || token == NULL || token[0] == '\0') {
		return 0;
	}
	size_t token_len = strlen(token);
	size_t required = ctx->length + token_len + 1;
	if (required > ctx->capacity) {
		size_t new_capacity = ctx->capacity;
		while (new_capacity < required) {
			new_capacity *= 2;
		}
		char* resized = (char*)realloc(ctx->buffer, new_capacity);
		if (resized == NULL) return -1;
		ctx->buffer = resized;
		ctx->capacity = new_capacity;
	}
	memcpy(ctx->buffer + ctx->length, token, token_len);
	ctx->length += token_len;
	ctx->buffer[ctx->length] = '\0';
	return 0;
}

static int handle_embed_store(QS_EVENT_PARAMETER params)
{
	if (!g_embedding_enabled) {
		send_json_http_response(params, 400, "{\"ok\":false,\"error\":\"embedding module not enabled\"}");
		return 400;
	}

	const char* text = api_qs_get_http_post_parameter(params, "text");
	if (text == NULL || text[0] == '\0') {
		text = api_qs_get_http_post_body(params);
	}
	if (text == NULL || text[0] == '\0') {
		send_json_http_response(params, 400, "{\"ok\":false,\"error\":\"missing text parameter\"}");
		return 400;
	}

	const char* id_str = api_qs_get_http_post_parameter(params, "id");
	if (id_str == NULL || id_str[0] == '\0') {
		send_json_http_response(params, 400, "{\"ok\":false,\"error\":\"missing id parameter\"}");
		return 400;
	}

	int64_t id;
	if (parse_positive_int64_strict(id_str, &id) != 0) {
		send_json_http_response(params, 400, "{\"ok\":false,\"error\":\"invalid id (must be positive integer)\"}");
		return 400;
	}

	int result = qs_embedding_store(g_embedding_store, text, text, id);
	if (result == 0) {
		char response[256];
		snprintf(response, sizeof(response), "{\"ok\":true,\"id\":%lld,\"text_length\":%zu}", (long long)id, strlen(text));
		send_json_http_response(params, 200, response);
		return 200;
	} else {
		send_json_http_response(params, 500, "{\"ok\":false,\"error\":\"failed to store embedding\"}");
		return 500;
	}
}

static int handle_embed_search(QS_EVENT_PARAMETER params)
{
	if (!g_embedding_enabled) {
		send_json_http_response(params, 400, "{\"ok\":false,\"error\":\"embedding module not enabled\"}");
		return 400;
	}

	const char* query = api_qs_get_http_post_parameter(params, "q");
	if (query == NULL || query[0] == '\0') {
		query = api_qs_get_http_post_body(params);
	}
	if (query == NULL || query[0] == '\0') {
		send_json_http_response(params, 400, "{\"ok\":false,\"error\":\"missing query parameter\"}");
		return 400;
	}

	const char* top_k_str = api_qs_get_http_post_parameter(params, "top_k");
	int top_k = 5;
	if (top_k_str != NULL && top_k_str[0] != '\0') {
		int val;
		if (parse_int_in_range_strict(top_k_str, 1, 100, &val) == 0) {
			top_k = val;
		}
	}

	int64_t result_ids[100];
	float result_scores[100];
	char* result_texts[100] = {NULL};
	int count = qs_embedding_search(g_embedding_store, query, top_k, result_ids, result_scores, result_texts, 100);

	/* Calculate required buffer size based on actual content */
	char* escaped_query = json_escape_string(query);
	if (!escaped_query) {
		for (int i = 0; i < count; i++) free(result_texts[i]);
		send_json_http_response(params, 500, "{\"ok\":false,\"error\":\"memory allocation failed\"}");
		return 500;
	}
	size_t resp_cap = strlen(escaped_query) + 64;
	for (int i = 0; i < count; i++) {
		resp_cap += 64 + (result_texts[i] ? strlen(result_texts[i]) * 2 : 0);
	}
	char* response = (char*)malloc(resp_cap);
	if (response == NULL) {
		free(escaped_query);
		for (int i = 0; i < count; i++) free(result_texts[i]);
		send_json_http_response(params, 500, "{\"ok\":false,\"error\":\"memory allocation failed\"}");
		return 500;
	}

	int pos = snprintf(response, resp_cap, "{\"ok\":true,\"query\":\"%s\",\"results\":[", escaped_query);
	free(escaped_query);
	for (int i = 0; i < count; i++) {
		if (i > 0) {
			pos += snprintf(response + pos, resp_cap - (size_t)pos, ",");
		}
		char* escaped_text = json_escape_string(result_texts[i] ? result_texts[i] : "");
		pos += snprintf(response + pos, resp_cap - (size_t)pos, "{\"id\":%lld,\"distance\":%.6f,\"text\":\"%s\"}",
						 (long long)result_ids[i], result_scores[i], escaped_text);
		free(escaped_text);
	}
	pos += snprintf(response + pos, resp_cap - (size_t)pos, "],\"count\":%d}", count);

	send_json_http_response(params, 200, response);
	free(response);
	for (int i = 0; i < count; i++) free(result_texts[i]);
	return 200;
}

static int handle_embed_delete(QS_EVENT_PARAMETER params)
{
	if (!g_embedding_enabled) {
		send_json_http_response(params, 400, "{\"ok\":false,\"error\":\"embedding module not enabled\"}");
		return 400;
	}

	const char* id_str = api_qs_get_http_post_parameter(params, "id");
	if (id_str == NULL || id_str[0] == '\0') {
		send_json_http_response(params, 400, "{\"ok\":false,\"error\":\"missing id parameter\"}");
		return 400;
	}

	int64_t id;
	if (parse_positive_int64_strict(id_str, &id) != 0) {
		send_json_http_response(params, 400, "{\"ok\":false,\"error\":\"invalid id (must be positive integer)\"}");
		return 400;
	}

	int result = qs_embedding_delete(g_embedding_store, id);
	if (result == 0) {
		char response[256];
		snprintf(response, sizeof(response), "{\"ok\":true,\"id\":%lld}", (long long)id);
		send_json_http_response(params, 200, response);
		return 200;
	} else {
		send_json_http_response(params, 500, "{\"ok\":false,\"error\":\"failed to delete embedding\"}");
		return 500;
	}
}

static char* make_rag_prompt(const char* user_query, const int64_t* source_ids,
                             const float* source_scores, char** source_texts,
                             int source_count, const char* ctx_prefix, const char* ctx_suffix)
{
	(void)ctx_prefix; (void)ctx_suffix; /* use fixed format for cleaner output */

	size_t total_size = 512;
	for (int i = 0; i < source_count; i++) {
		total_size += 64 + (source_texts[i] ? strlen(source_texts[i]) : 0);
	}

	char* prompt = (char*)malloc(total_size);
	if (!prompt) return NULL;

	int pos = 0;
	pos += snprintf(prompt + pos, total_size - (size_t)pos, "Retrieved context:\n");
	for (int i = 0; i < source_count; i++) {
		pos += snprintf(prompt + pos, total_size - (size_t)pos, "- Doc %d (distance=%.4f): ",
		                i + 1, source_scores[i]);
		if (source_texts[i]) {
			pos += snprintf(prompt + pos, total_size - (size_t)pos, "%s", source_texts[i]);
		}
		pos += snprintf(prompt + pos, total_size - (size_t)pos, "\n");
	}

	return prompt;
}

static int handle_rag(QS_EVENT_PARAMETER params)
{
	if (!g_embedding_enabled) {
		send_json_http_response(params, 400, "{\"ok\":false,\"error\":\"embedding module not enabled\"}");
		return 400;
	}

	const char* query = api_qs_get_http_post_parameter(params, "q");
	if (query == NULL || query[0] == '\0') {
		query = api_qs_get_http_post_body(params);
	}
	if (query == NULL || query[0] == '\0') {
		send_json_http_response(params, 400, "{\"ok\":false,\"error\":\"missing q parameter\"}");
		return 400;
	}

	const char* top_k_str = api_qs_get_http_post_parameter(params, "top_k");
	int top_k = 3;
	if (top_k_str != NULL && top_k_str[0] != '\0') {
		int val;
		if (parse_int_in_range_strict(top_k_str, 1, 100, &val) == 0) {
			top_k = val;
		}
	}

	const char* ctx_prefix = api_qs_get_http_post_parameter(params, "context_prefix");
	if (ctx_prefix == NULL || ctx_prefix[0] == '\0') ctx_prefix = "### Doc";
	const char* ctx_suffix = api_qs_get_http_post_parameter(params, "context_suffix");
	if (ctx_suffix == NULL || ctx_suffix[0] == '\0') ctx_suffix = "---";

	int64_t result_ids[100];
	float result_scores[100];
	char* result_texts[100] = {NULL};
	int count = qs_embedding_search(g_embedding_store, query, top_k, result_ids, result_scores, result_texts, 100);

	/* Build effective prompt with context injected into SYSTEM portion */
	const char* prefix = "[SYSTEM]\n";

	size_t base_size = strlen(prefix) + 200;
	if (g_system_prompt != NULL && g_system_prompt[0] != '\0') {
		base_size += strlen(g_system_prompt) + 8;
	}

	char* system_part = (char*)malloc(base_size);
	if (!system_part) {
		for (int i = 0; i < count; i++) free(result_texts[i]);
		send_json_http_response(params, 500, "{\"ok\":false,\"error\":\"memory allocation failed\"}");
		return 500;
	}
	system_part[0] = '\0';

	strcat(system_part, prefix);
	if (g_system_prompt != NULL && g_system_prompt[0] != '\0') {
		strcat(system_part, g_system_prompt);
		strcat(system_part, "\n\n");
	}
	strcat(system_part, "You are a helpful assistant. Answer questions based on the retrieved context below. Be concise and accurate.\n\nRetrieved context:");

	char* rag_context = make_rag_prompt(query, result_ids, result_scores, result_texts, count, ctx_prefix, ctx_suffix);
	if (!rag_context) {
		free(system_part);
		for (int i = 0; i < count; i++) free(result_texts[i]);
		send_json_http_response(params, 500, "{\"ok\":false,\"error\":\"failed to build prompt\"}");
		return 500;
	}

	size_t total = strlen(system_part) + strlen(rag_context) + strlen(query) + 64;
	char* final_prompt = (char*)malloc(total);
	if (!final_prompt) {
		free(system_part);
		free(rag_context);
		for (int i = 0; i < count; i++) free(result_texts[i]);
		send_json_http_response(params, 500, "{\"ok\":false,\"error\":\"memory allocation failed\"}");
		return 500;
	}

	snprintf(final_prompt, total, "%s\n%s\n\n[USER]\n%s\n\n[ASSISTANT]\n", system_part, rag_context, query);
	free(system_part);
	free(rag_context);

	/* Generate answer */
	RAG_GEN_CONTEXT* ctx = (RAG_GEN_CONTEXT*)malloc(sizeof(RAG_GEN_CONTEXT));
	if (ctx == NULL) {
		free(final_prompt);
		for (int i = 0; i < count; i++) free(result_texts[i]);
		send_json_http_response(params, 500, "{\"ok\":false,\"error\":\"memory allocation failed\"}");
		return 500;
	}
	ctx->capacity = 4096;
	ctx->length = 0;
	ctx->buffer = (char*)malloc(ctx->capacity);
	if (ctx->buffer == NULL) {
		free(ctx);
		free(final_prompt);
		for (int i = 0; i < count; i++) free(result_texts[i]);
		send_json_http_response(params, 500, "{\"ok\":false,\"error\":\"memory allocation failed\"}");
		return 500;
	}
	ctx->buffer[0] = '\0';

	int stream_result = qs_llama_module_stream_text(final_prompt, on_rag_token, ctx);
	free(final_prompt);

	if (stream_result == -1) {
		free(ctx->buffer);
		free(ctx);
		for (int i = 0; i < count; i++) free(result_texts[i]);
		send_json_http_response(params, 500, "{\"ok\":false,\"error\":\"generation failed\"}");
		return 500;
	}

	/* Clean up answer: strip leading/trailing whitespace and markdown code fences */
	char* answer_text = ctx->buffer;
	size_t len = strlen(answer_text);
	while (len > 0 && (answer_text[0] == ' ' || answer_text[0] == '\n' || answer_text[0] == '\r' || answer_text[0] == '\t')) {
		answer_text++;
		len--;
	}
	while (len > 0 && (answer_text[len-1] == ' ' || answer_text[len-1] == '\n' || answer_text[len-1] == '\r' || answer_text[len-1] == '\t')) {
		len--;
	}
	answer_text[len] = '\0';

	/* Strip markdown code fences */
	strip_markdown_code_fences_in_place(answer_text);

	const char* final_answer = answer_text;

	/* Build sources array: calculate required size from actual content */
	size_t src_cap = 16;
	for (int i = 0; i < count; i++) {
		src_cap += 64 + (result_texts[i] ? strlen(result_texts[i]) * 2 : 0);
	}
	char* src_json = (char*)malloc(src_cap);
	if (!src_json) {
		free(ctx->buffer);
		free(ctx);
		for (int i = 0; i < count; i++) free(result_texts[i]);
		send_json_http_response(params, 500, "{\"ok\":false,\"error\":\"memory allocation failed\"}");
		return 500;
	}
	int sp = snprintf(src_json, src_cap, "[");
	for (int i = 0; i < count; i++) {
		char* escaped_text = json_escape_string(result_texts[i] ? result_texts[i] : "");
		sp += snprintf(src_json + sp, src_cap - (size_t)sp, "{\"id\":%lld,\"distance\":%.6f,\"text\":\"%s\"}",
			               (long long)result_ids[i], result_scores[i], escaped_text);
		free(escaped_text);
		if (i < count - 1) {
			sp += snprintf(src_json + sp, src_cap - (size_t)sp, ",");
		}
	}
	sp += snprintf(src_json + sp, src_cap - (size_t)sp, "]");
	for (int i = 0; i < count; i++) free(result_texts[i]);

	char* escaped_answer = json_escape_string(final_answer);

	size_t resp_cap = strlen(escaped_answer) + src_cap + 256;
	char* response = (char*)malloc(resp_cap);
	if (response == NULL) {
		free(src_json);
		free(escaped_answer);
		free(ctx->buffer);
		free(ctx);
		send_json_http_response(params, 500, "{\"ok\":false,\"error\":\"memory allocation failed\"}");
		return 500;
	}
	snprintf(response, resp_cap, "{\"ok\":true,\"answer\":\"%s\",\"sources\":%s,\"count\":%d}", escaped_answer, src_json, count);

	send_json_http_response(params, 200, response);
	free(response);
	free(src_json);
	free(escaped_answer);
	free(ctx->buffer);
	free(ctx);
	return 200;
}

static int handle_rag_stream(QS_EVENT_PARAMETER params)
{
	if (!g_embedding_enabled) {
		send_json_http_response(params, 400, "{\"ok\":false,\"error\":\"embedding module not enabled\"}");
		return 400;
	}

	const char* query = api_qs_get_http_post_parameter(params, "q");
	if (query == NULL || query[0] == '\0') {
		query = api_qs_get_http_post_body(params);
	}
	if (query == NULL || query[0] == '\0') {
		send_json_http_response(params, 400, "{\"ok\":false,\"error\":\"missing q parameter\"}");
		return 400;
	}

	const char* top_k_str = api_qs_get_http_post_parameter(params, "top_k");
	int top_k = 3;
	if (top_k_str != NULL && top_k_str[0] != '\0') {
		int val;
		if (parse_int_in_range_strict(top_k_str, 1, 100, &val) == 0) {
			top_k = val;
		}
	}

	const char* ctx_prefix = api_qs_get_http_post_parameter(params, "context_prefix");
	if (ctx_prefix == NULL || ctx_prefix[0] == '\0') ctx_prefix = "### Doc";
	const char* ctx_suffix = api_qs_get_http_post_parameter(params, "context_suffix");
	if (ctx_suffix == NULL || ctx_suffix[0] == '\0') ctx_suffix = "---";

	int64_t result_ids[100];
	float result_scores[100];
	char* result_texts[100] = {NULL};
	int count = qs_embedding_search(g_embedding_store, query, top_k, result_ids, result_scores, result_texts, 100);

	/* Build effective prompt with context injected into SYSTEM portion */
	const char* prefix = "[SYSTEM]\n";

	size_t base_size = strlen(prefix) + 200;
	if (g_system_prompt != NULL && g_system_prompt[0] != '\0') {
		base_size += strlen(g_system_prompt) + 8;
	}

	char* system_part = (char*)malloc(base_size);
	if (!system_part) {
		for (int i = 0; i < count; i++) free(result_texts[i]);
		send_json_http_response(params, 500, "{\"ok\":false,\"error\":\"memory allocation failed\"}");
		return 500;
	}
	system_part[0] = '\0';

	strcat(system_part, prefix);
	if (g_system_prompt != NULL && g_system_prompt[0] != '\0') {
		strcat(system_part, g_system_prompt);
		strcat(system_part, "\n\n");
	}
	strcat(system_part, "You are a helpful assistant. Answer questions based on the retrieved context below. Be concise and accurate.\n\nRetrieved context:");

	char* rag_context = make_rag_prompt(query, result_ids, result_scores, result_texts, count, ctx_prefix, ctx_suffix);
	if (!rag_context) {
		free(system_part);
		for (int i = 0; i < count; i++) free(result_texts[i]);
		send_json_http_response(params, 500, "{\"ok\":false,\"error\":\"failed to build prompt\"}");
		return 500;
	}

	size_t total = strlen(system_part) + strlen(rag_context) + strlen(query) + 64;
	char* final_prompt = (char*)malloc(total);
	if (!final_prompt) {
		free(system_part);
		free(rag_context);
		for (int i = 0; i < count; i++) free(result_texts[i]);
		send_json_http_response(params, 500, "{\"ok\":false,\"error\":\"memory allocation failed\"}");
		return 500;
	}

	snprintf(final_prompt, total, "%s\n%s\n\n[USER]\n%s\n\n[ASSISTANT]\n", system_part, rag_context, query);
	free(system_part);
	free(rag_context);

	/* Stream answer */
	QS_LLM_HTTP_STREAM_CONTEXT stream;
	if (-1 == qs_llm_http_stream_open(params, &stream)) {
		free(final_prompt);
		for (int i = 0; i < count; i++) free(result_texts[i]);
		return 500;
	}

	const char* meta = (g_system_prompt != NULL) ? "system_prompt=on" : "system_prompt=off";
	if (-1 == qs_llm_http_stream_send_event(&stream, "meta", meta)) {
		qs_llm_http_stream_close(&stream);
		free(final_prompt);
		for (int i = 0; i < count; i++) free(result_texts[i]);
		return 500;
	}

	if (-1 == qs_llama_module_stream_text(final_prompt, on_rag_stream_token, &stream)) {
		qs_llm_http_stream_send_event(&stream, "error", "stream failed");
		qs_llm_http_stream_send_done(&stream);
		qs_llm_http_stream_close(&stream);
		free(final_prompt);
		for (int i = 0; i < count; i++) free(result_texts[i]);
		return 500;
	}
	free(final_prompt);

	/* Send sources as final event: calculate required size from actual content */
	size_t src_cap = 16;
	for (int i = 0; i < count; i++) {
		src_cap += 64 + (result_texts[i] ? strlen(result_texts[i]) * 2 : 0);
	}
	char* src_json = (char*)malloc(src_cap);
	if (!src_json) {
		qs_llm_http_stream_send_event(&stream, "error", "memory allocation failed");
		qs_llm_http_stream_send_done(&stream);
		qs_llm_http_stream_close(&stream);
		for (int i = 0; i < count; i++) free(result_texts[i]);
		return 500;
	}
	int sp = snprintf(src_json, src_cap, "[");
	for (int i = 0; i < count; i++) {
		char* escaped_text = json_escape_string(result_texts[i] ? result_texts[i] : "");
		sp += snprintf(src_json + sp, src_cap - (size_t)sp, "{\"id\":%lld,\"distance\":%.6f,\"text\":\"%s\"}",
			               (long long)result_ids[i], result_scores[i], escaped_text);
		free(escaped_text);
		if (i < count - 1) {
			sp += snprintf(src_json + sp, src_cap - (size_t)sp, ",");
		}
	}
	sp += snprintf(src_json + sp, src_cap - (size_t)sp, "]");

	qs_llm_http_stream_send_event(&stream, "sources", src_json);
	qs_llm_http_stream_send_done(&stream);
	qs_llm_http_stream_close(&stream);
	free(src_json);
	for (int i = 0; i < count; i++) free(result_texts[i]);
	return 200;
}

static int on_http_event(QS_EVENT_PARAMETER params)
{
	const char* method = api_qs_get_http_method(params);
	const char* path = api_qs_get_http_path(params);

	if (method == NULL || path == NULL) {
		return 404;
	}

	/* Embedding endpoints */
	if (strcmp(method, "POST") == 0 && strcmp(path, "/api/embed") == 0) {
		return handle_embed_store(params);
	}

	if (strcmp(method, "POST") == 0 && strcmp(path, "/api/embed/search") == 0) {
		return handle_embed_search(params);
	}

	if (strcmp(method, "POST") == 0 && strcmp(path, "/api/embed/delete") == 0) {
		return handle_embed_delete(params);
	}

	/* RAG endpoints */
	if (strcmp(method, "POST") == 0 && strcmp(path, "/api/llm/rag/stream") == 0) {
		return handle_rag_stream(params);
	}

	if (strcmp(method, "POST") == 0 && strcmp(path, "/api/llm/rag") == 0) {
		return handle_rag(params);
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

	if (strcmp(method, "GET") == 0 && strcmp(path, "/api/status") == 0) {
		char response[256];
		snprintf(response, sizeof(response), 
			"{\"ok\":true,\"llm_enabled\":true,\"embedding_enabled\":%s}", 
			g_embedding_enabled ? "true" : "false");
		send_json_http_response(params, 200, response);
		return 200;
	}

	return 404;
}
