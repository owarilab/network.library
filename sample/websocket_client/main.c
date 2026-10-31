#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "qs_api.h"
#include "qs_openssl_module.h"

static QS_CLIENT_CONTEXT* g_client;
static QS_MEMORY_CONTEXT g_memory;
static QS_HTTP_CLIENT_CONTEXT g_http_client;
static char g_room_id[128];
static char g_connection_id[128];
static int g_joined;
static int g_message_received;
static int g_failed;

static int run_room_request(const char* method, const char* path, const char* body, char* response, size_t response_size)
{
	char* request_buffer;
	const char* body_buffer;
	int status_code;
	char status_message[256];
	char http_version[16];

	qs_ssl_module_http_client_dispose(&g_http_client);
	if(0 != qs_ssl_module_http_client_connect(&g_http_client, "127.0.0.1", 4444, 0)){
		printf("HTTP %s %s -> connect failed\n", method, path);
		return -1;
	}

	request_buffer = qs_ssl_module_http_client_get_request_buffer(&g_http_client);
	if(request_buffer == NULL){
		qs_ssl_module_http_client_dispose(&g_http_client);
		printf("HTTP %s %s -> request buffer null\n", method, path);
		return -1;
	}

	if(body != NULL){
		snprintf(request_buffer, QS_HTTP_CLIENT_REQUEST_BUFFER_SIZE,
			"%s %s HTTP/1.1\r\n"
			"Host: 127.0.0.1:4444\r\n"
			"Content-Type: application/x-www-form-urlencoded\r\n"
			"Content-Length: %zu\r\n"
			"Connection: close\r\n"
			"\r\n"
			"%s",
			method, path, strlen(body), body);
	}else{
		snprintf(request_buffer, QS_HTTP_CLIENT_REQUEST_BUFFER_SIZE,
			"%s %s HTTP/1.1\r\n"
			"Host: 127.0.0.1:4444\r\n"
			"Connection: close\r\n"
			"\r\n",
			method, path);
	}

	while(1){
		qs_ssl_module_http_client_update(&g_http_client);
		if(g_http_client.phase == QS_SSL_MODULE_PHASE_DISCONNECT){
			break;
		}
		if(g_http_client.client_context == NULL){
			qs_ssl_module_http_client_dispose(&g_http_client);
			printf("HTTP %s %s -> client context lost\n", method, path);
			return -1;
		}
		api_qs_client_sleep(g_http_client.client_context);
	}

	body_buffer = qs_ssl_module_http_client_get_body_buffer(&g_http_client);
	if(body_buffer == NULL){
		qs_ssl_module_http_client_dispose(&g_http_client);
		printf("HTTP %s %s -> body buffer null\n", method, path);
		return -1;
	}

	memset(http_version, 0, sizeof(http_version));
	memset(status_message, 0, sizeof(status_message));
	if(sscanf(qs_ssl_module_http_client_get_header_buffer(&g_http_client),
		"HTTP/%15s %d %255[^\r\n]", http_version, &status_code, status_message) < 2){
		qs_ssl_module_http_client_dispose(&g_http_client);
		printf("HTTP %s %s -> failed to parse status line\n", method, path);
		return -1;
	}

	snprintf(response, response_size, "%s", body_buffer);
	printf("HTTP %s %s -> %s\n", method, path, response);
	qs_ssl_module_http_client_dispose(&g_http_client);
	return 0;
}

static int extract_json_string(const char* json, const char* key, char* value, size_t value_size)
{
	QS_JSON_ELEMENT_OBJECT object;
	char* result;

	api_qs_memory_clean(&g_memory);
	if(0 != api_qs_json_decode_object(&g_memory, &object, json)){
		return -1;
	}
	result = api_qs_object_get_string(&object, key);
	if(result == NULL) return -1;
	if(strlen(result) >= value_size) return -1;
	snprintf(value, value_size, "%s", result);
	return 0;
}

static int on_connect(QS_EVENT_PARAMETER params)
{
	const char* hello = "native-websocket-client";
	int send_result;
	(void)params;
	printf("WebSocket upgrade complete\n");
	if(api_qs_websocket_client_get_state(g_client) != QS_WEBSOCKET_STATE_OPEN){
		fprintf(stderr, "WebSocket did not enter OPEN state\n");
		g_failed = 1;
		return -1;
	}
	send_result = api_qs_websocket_client_send(g_client, 0, hello, strlen(hello));
	printf("initial WebSocket send result=%d\n", send_result);
	if(send_result != 0){
		g_failed = 1;
	}
	return 0;
}

static int on_websocket_event(QS_EVENT_PARAMETER params)
{
	uint8_t* payload = api_qs_get_websocket_payload(params);
	size_t payload_size = api_qs_get_websocket_payload_length(params);
	uint8_t opcode = api_qs_get_websocket_opcode(params);
	char message[256];
	printf("WebSocket frame opcode=%u payload=%.*s\n", opcode, (int)payload_size, payload);
	if(opcode != 1 || payload_size >= sizeof(message)) return 0;
	memcpy(message, payload, payload_size);
	message[payload_size] = '\0';
	if(g_connection_id[0] == '\0' && strstr(message, "native-websocket-client") != NULL){
		if(extract_json_string(message, "id", g_connection_id, sizeof(g_connection_id)) != 0){
			g_failed = 1;
			return 0;
		}
		printf("WebSocket connection_id=%s\n", g_connection_id);
		return 0;
	}
	if(strstr(message, "native-test-message") != NULL){
		g_message_received = 1;
	}
	return 0;
}

static int on_close(QS_EVENT_PARAMETER params)
{
	printf("WebSocket closed\n");
	return 0;
}

int main(int argc, char** argv)
{
	char response[4096];
	char body[512];
	time_t deadline;
	int memory_allocated = 0;
	int result = 0;
	(void)argc;
	(void)argv;

	if(api_qs_init() != 0){
		fprintf(stderr, "api_qs_init failed\n");
		return 1;
	}
	if(api_qs_memory_alloc(&g_memory, 1024 * 1024 * 4) != 0){
		fprintf(stderr, "api_qs_memory_alloc failed\n");
		return 1;
	}
	memory_allocated = 1;

	snprintf(body, sizeof(body), "name=native_ws_test_%ld", (long)time(NULL));
	if(run_room_request("POST", "/api/v1/room/create", body, response, sizeof(response)) != 0 || extract_json_string(response, "id", g_room_id, sizeof(g_room_id)) != 0){
		fprintf(stderr, "room create failed\n");
		result = 1;
		goto cleanup;
	}
	if(api_qs_websocket_client_init(&g_client, "127.0.0.1", 4444, "/") != 0){
		fprintf(stderr, "websocket client init failed\n");
		result = 1;
		goto cleanup;
	}
	if(api_qs_websocket_client_set_max_message_size(g_client, 4096) != 0 ||
		api_qs_websocket_client_get_max_message_size(g_client) != 4096){
		fprintf(stderr, "websocket message size configuration failed\n");
		api_qs_client_free(g_client);
		result = 1;
		goto cleanup;
	}
	printf("WebSocket client initialized, socket=%d\n", api_qs_client_get_socket(g_client));
	api_qs_set_client_on_connect_event(g_client, on_connect);
	api_qs_set_websocket_client_on_event(g_client, on_websocket_event);
	api_qs_set_client_on_close_event(g_client, on_close);
	deadline = time(NULL) + 10;
	while(time(NULL) < deadline && !g_failed && !g_message_received){
		api_qs_client_update(g_client);
		if(g_connection_id[0] != '\0' && !g_joined){
			snprintf(body, sizeof(body), "room_id=%s&connection_id=%s", g_room_id, g_connection_id);
			if(run_room_request("POST", "/api/v1/room/join", body, response, sizeof(response)) != 0){
				g_failed = 1;
			}else{
				g_joined = 1;
				printf("room joined, sending room message result=%d\n", api_qs_websocket_client_send(g_client, 0, "native-test-message", strlen("native-test-message")));
			}
		}
		api_qs_client_sleep(g_client);
	}
	if(g_joined){
		snprintf(body, sizeof(body), "room_id=%s&connection_id=%s", g_room_id, g_connection_id);
		run_room_request("POST", "/api/v1/room/leave", body, response, sizeof(response));
	}
	if(api_qs_websocket_client_close_with_reason(g_client, 1000, "test complete") != 0 ||
		api_qs_websocket_client_get_state(g_client) != QS_WEBSOCKET_STATE_CLOSING ||
		api_qs_websocket_client_get_close_code(g_client) != 1000 ||
		strcmp(api_qs_websocket_client_get_close_reason(g_client), "test complete") != 0 ||
		api_qs_websocket_client_send(g_client, 0, "after-close", strlen("after-close")) == 0){
		fprintf(stderr, "WebSocket close state handling failed\n");
		g_failed = 1;
	}
	api_qs_client_free(g_client);
	if(g_failed || !g_message_received){
		fprintf(stderr, "WEBSOCKET TEST FAILED\n");
		result = 1;
	}else{
		printf("WEBSOCKET TEST PASSED\n");
	}

cleanup:
	if(memory_allocated){
		api_qs_memory_free(&g_memory);
	}
	return result;
}