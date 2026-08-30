#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "qs_api.h"

static QS_CLIENT_CONTEXT* g_client;
static char g_room_id[128];
static char g_connection_id[128];
static int g_joined;
static int g_message_received;
static int g_failed;

static int run_room_request(const char* method, const char* path, const char* body, char* response, size_t response_size)
{
	char command[1024];
	FILE* pipe;
	int result;
	if(body != NULL){
		snprintf(command, sizeof(command), "curl -sS -X %s http://127.0.0.1:4444%s -d '%s'", method, path, body);
	}else{
		snprintf(command, sizeof(command), "curl -sS -X %s http://127.0.0.1:4444%s", method, path);
	}
	pipe = popen(command, "r");
	if(pipe == NULL) return -1;
	response[0] = '\0';
	fread(response, 1, response_size - 1, pipe);
	response[response_size - 1] = '\0';
	result = pclose(pipe);
	printf("HTTP %s %s -> %s\n", method, path, response);
	return result == 0 ? 0 : -1;
}

static int extract_json_string(const char* json, const char* key, char* value, size_t value_size)
{
	char needle[64];
	const char* start;
	const char* end;
	snprintf(needle, sizeof(needle), "\"%s\":\"", key);
	start = strstr(json, needle);
	if(start == NULL) return -1;
	start += strlen(needle);
	end = strchr(start, '"');
	if(end == NULL || (size_t)(end - start) >= value_size) return -1;
	memcpy(value, start, (size_t)(end - start));
	value[end - start] = '\0';
	return 0;
}

static int on_connect(QS_EVENT_PARAMETER params)
{
	const char* hello = "native-websocket-client";
	int send_result;
	printf("WebSocket upgrade complete\n");
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
	(void)argc;
	(void)argv;
	snprintf(body, sizeof(body), "name=native_ws_test_%ld", (long)time(NULL));
	if(run_room_request("POST", "/api/v1/room/create", body, response, sizeof(response)) != 0 || extract_json_string(response, "id", g_room_id, sizeof(g_room_id)) != 0){
		fprintf(stderr, "room create failed\n");
		return 1;
	}
	if(api_qs_websocket_client_init(&g_client, "127.0.0.1", 4444, "/") != 0){
		fprintf(stderr, "websocket client init failed\n");
		return 1;
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
	api_qs_websocket_client_close(g_client);
	api_qs_client_free(g_client);
	if(g_failed || !g_message_received){
		fprintf(stderr, "WEBSOCKET TEST FAILED\n");
		return 1;
	}
	printf("WEBSOCKET TEST PASSED\n");
	return 0;
}