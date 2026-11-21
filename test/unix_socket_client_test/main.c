/*
 * Copyright (c) Katsuya Owari
 *
 * unix_socket_client_test
 * api_qs_client_init_unix (AF_UNIX クライアント) の検証:
 *   case 1: 同一プロセスの AF_UNIX echo サーバーとのラウンドトリップ（PLAIN）
 *   case 2: 存在しないソケットパス -> close イベント発火、ハングしない
 *   case 3: パス無効（NULL / 128 バイト以上）-> init が -7 を返す
 */

#include "qs_api.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#if defined(__WINDOWS__)
#error "This test requires POSIX (AF_UNIX)"
#endif

#define TEST_SOCKET_PATH "/tmp/opencode/qs_unix_client_test.sock"
#define TEST_PAYLOAD "hello_unix_socket"
#define TEST_LOOP_LIMIT 3000 /* api_qs_client_sleep は 10ms 相当 -> 最大 30 秒 */

/* ---------------- AF_UNIX echo サーバー（raw socket） ---------------- */

static int g_server_listen_fd = -1;

static void* server_thread(void* arg)
{
	(void)arg;
	int client_fd = accept(g_server_listen_fd, NULL, NULL);
	if(client_fd == -1){
		return NULL;
	}
	char buf[4096];
	for(;;){
		ssize_t n = recv(client_fd, buf, sizeof(buf), 0);
		if(n <= 0){
			break;
		}
		if(send(client_fd, buf, n, 0) == -1){
			break;
		}
	}
	close(client_fd);
	return NULL;
}

static int start_echo_server(void)
{
	unlink(TEST_SOCKET_PATH);
	g_server_listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if(g_server_listen_fd == -1){
		return -1;
	}
	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, TEST_SOCKET_PATH, sizeof(addr.sun_path) - 1);
	if(bind(g_server_listen_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1){
		close(g_server_listen_fd);
		g_server_listen_fd = -1;
		return -1;
	}
	if(listen(g_server_listen_fd, 1) == -1){
		close(g_server_listen_fd);
		g_server_listen_fd = -1;
		return -1;
	}
	pthread_t thread;
	if(pthread_create(&thread, NULL, server_thread, NULL) != 0){
		close(g_server_listen_fd);
		g_server_listen_fd = -1;
		return -1;
	}
	pthread_detach(thread);
	return 0;
}

static void stop_echo_server(void)
{
	if(g_server_listen_fd != -1){
		close(g_server_listen_fd);
		g_server_listen_fd = -1;
	}
	unlink(TEST_SOCKET_PATH);
}

/* ---------------- クライアントイベント ---------------- */

typedef struct TEST_CLIENT_STATE
{
	int connected;
	int closed;
	int matched;
	char recv_buf[256];
	size_t recv_len;
} TEST_CLIENT_STATE;

static int on_connect(QS_EVENT_PARAMETER params)
{
	QS_CLIENT_CONTEXT* context = api_qs_client_get_context(params);
	TEST_CLIENT_STATE* state = (TEST_CLIENT_STATE*)context->client_data;
	state->connected = 1;
	api_qs_client_send(context, TEST_PAYLOAD, strlen(TEST_PAYLOAD));
	return 0;
}

static int on_plain_event(QS_EVENT_PARAMETER params)
{
	QS_CLIENT_CONTEXT* context = api_qs_client_get_context(params);
	TEST_CLIENT_STATE* state = (TEST_CLIENT_STATE*)context->client_data;
	uint8_t* payload = api_qs_get_plain_payload(params);
	size_t len = api_qs_get_plain_payload_length(params);
	size_t copy = (len < sizeof(state->recv_buf) - 1) ? len : sizeof(state->recv_buf) - 1;
	memcpy(state->recv_buf, payload, copy);
	state->recv_buf[copy] = 0;
	state->recv_len = len;
	state->matched = (len == strlen(TEST_PAYLOAD)) && (0 == memcmp(payload, TEST_PAYLOAD, len));
	return 0;
}

static int on_close(QS_EVENT_PARAMETER params)
{
	QS_CLIENT_CONTEXT* context = api_qs_client_get_context(params);
	TEST_CLIENT_STATE* state = (TEST_CLIENT_STATE*)context->client_data;
	state->closed = 1;
	return 0;
}

/* ---------------- ケース ---------------- */

static int test_roundtrip(void)
{
	printf("case 1: roundtrip via %s\n", TEST_SOCKET_PATH);
	if(0 != start_echo_server()){
		printf("  fail: server start failed\n");
		return -1;
	}
	TEST_CLIENT_STATE state;
	memset(&state, 0, sizeof(state));
	QS_CLIENT_CONTEXT* context = NULL;
	if(0 != api_qs_client_init_unix(&context, TEST_SOCKET_PATH, QS_SERVER_TYPE_PLAIN)){
		printf("  fail: init failed\n");
		stop_echo_server();
		return -1;
	}
	context->client_data = &state;
	api_qs_set_client_on_connect_event(context, on_connect);
	api_qs_set_client_on_plain_event(context, on_plain_event);
	api_qs_set_client_on_close_event(context, on_close);

	for(int i = 0; i < TEST_LOOP_LIMIT; i++){
		api_qs_client_update(context);
		api_qs_client_sleep(context);
		if(state.matched){
			break;
		}
	}
	int result = 0;
	if(state.matched){
		printf("  ok: received '%s' (%zu bytes)\n", state.recv_buf, state.recv_len);
	}
	else{
		printf("  fail: no matching payload (connected=%d closed=%d)\n", state.connected, state.closed);
		result = -1;
	}
	api_qs_client_free(context);
	stop_echo_server();
	return result;
}

static int test_missing_path(void)
{
	printf("case 2: missing socket path\n");
	const char* missing = "/tmp/opencode/qs_unix_client_missing.sock";
	unlink(missing);
	TEST_CLIENT_STATE state;
	memset(&state, 0, sizeof(state));
	QS_CLIENT_CONTEXT* context = NULL;
	if(0 != api_qs_client_init_unix(&context, missing, QS_SERVER_TYPE_PLAIN)){
		printf("  fail: init unexpectedly failed\n");
		return -1;
	}
	context->client_data = &state;
	api_qs_set_client_on_close_event(context, on_close);

	for(int i = 0; i < TEST_LOOP_LIMIT; i++){
		api_qs_client_update(context);
		api_qs_client_sleep(context);
		if(state.closed){
			break;
		}
	}
	int result = 0;
	if(state.closed){
		printf("  ok: close event fired\n");
	}
	else{
		printf("  fail: no close event (hang risk)\n");
		result = -1;
	}
	api_qs_client_free(context);
	return result;
}

static int test_invalid_path(void)
{
	printf("case 3: invalid path (NULL / >= 128 bytes)\n");
	QS_CLIENT_CONTEXT* context = NULL;
	int ret_null = api_qs_client_init_unix(&context, NULL, QS_SERVER_TYPE_PLAIN);

	char long_path[256];
	memset(long_path, 'a', sizeof(long_path) - 1);
	long_path[sizeof(long_path) - 1] = 0;
	int ret_long = api_qs_client_init_unix(&context, long_path, QS_SERVER_TYPE_PLAIN);

	int result = 0;
	if(ret_null == -7 && ret_long == -7){
		printf("  ok: both returned -7\n");
	}
	else{
		printf("  fail: null=%d long=%d (expected -7)\n", ret_null, ret_long);
		result = -1;
	}
	return result;
}

int main(void)
{
	api_qs_init();
	int failed = 0;
	failed |= test_roundtrip();
	failed |= test_missing_path();
	failed |= test_invalid_path();
	if(failed){
		printf("UNIX SOCKET CLIENT TEST: FAILED\n");
		return 1;
	}
	printf("UNIX SOCKET CLIENT TEST: ALL PASSED\n");
	return 0;
}
