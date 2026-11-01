/*
 * Copyright (c) Katsuya Owari
 *
 * ws_binary_test
 *   WebSocketバイナリ送受信APIをテストする。
 *
 * 検証内容:
 *   - テキストメッセージの往復（opcode 1）
 *   - バイナリメッセージの往復（opcode 2・NULバイト含むペイロード）
 *   - サーバー側: api_qs_get_ws_opcode / api_qs_get_ws_message /
 *                 api_qs_get_ws_message_size / api_qs_send_ws_binary
 *   - クライアント側: api_qs_websocket_client_send(is_binary) /
 *                 api_qs_get_websocket_opcode / api_qs_get_websocket_payload
 *
 * 方式:
 *   - ルーター無効のHTTPサーバー（高番ポート）を別スレッドで api_qs_update ループ実行
 *   - on_ws_event で opcode を確認し、テキスト/バイナリをecho
 *   - WSクライアントを別スレッドで api_qs_client_update ループ実行
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "qs_api.h"

static int pass_count = 0;
static int fail_count = 0;

static void check(const char* name, int ok)
{
    if (ok) {
        pass_count++;
        printf("[PASS] %s\n", name);
    } else {
        fail_count++;
        printf("[FAIL] %s\n", name);
    }
}

#define TEST_PORT 18605
#define BINARY_PAYLOAD_SIZE 16

static QS_SERVER_CONTEXT* g_context = 0;
static QS_CLIENT_CONTEXT* g_client = 0;
static volatile int g_server_running = 0;
static volatile int g_client_running = 0;

/* サーバー側で確認したopcode（1=text, 2=binary） */
static volatile uint8_t g_server_seen_text_opcode = 0;
static volatile uint8_t g_server_seen_binary_opcode = 0;
static volatile int g_server_binary_payload_ok = 0;

/* クライアント側で受信したメッセージ */
static volatile int g_text_echo_received = 0;
static volatile uint8_t g_client_text_opcode = 0;
static volatile int g_binary_echo_received = 0;
static volatile uint8_t g_client_binary_opcode = 0;
static uint8_t g_client_binary_payload[BINARY_PAYLOAD_SIZE];
static volatile size_t g_client_binary_payload_len = 0;
static volatile int g_failed = 0;

static uint8_t g_send_payload[BINARY_PAYLOAD_SIZE];

/* --- スレッド --- */

static void* server_thread(void* arg)
{
    (void)arg;
    while (g_server_running) {
        api_qs_update(g_context);
        usleep(1000);
    }
    return NULL;
}

static void* client_thread(void* arg)
{
    (void)arg;
    while (g_client_running) {
        api_qs_client_update(g_client);
        usleep(1000);
    }
    return NULL;
}

/* --- サーバー側コールバック --- */

static int on_ws_event(QS_EVENT_PARAMETER params)
{
    uint8_t opcode = api_qs_get_ws_opcode(params);
    char* payload = api_qs_get_ws_message(params);
    size_t payload_len = (size_t)api_qs_get_ws_message_size(params);
    if (payload == NULL || payload_len == 0) return 0;

    if (opcode == 1) {
        /* テキスト: 文字列としてecho */
        g_server_seen_text_opcode = 1;
        char message[256];
        if (payload_len >= sizeof(message)) return 0;
        memcpy(message, payload, payload_len);
        message[payload_len] = '\0';
        api_qs_send_ws_message(params, message);
    } else if (opcode == 2) {
        /* バイナリ: ペイロードを検証してバイナリでecho */
        g_server_seen_binary_opcode = 2;
        if (payload_len == BINARY_PAYLOAD_SIZE &&
            memcmp(payload, g_send_payload, BINARY_PAYLOAD_SIZE) == 0) {
            g_server_binary_payload_ok = 1;
        }
        api_qs_send_ws_binary(params, payload, payload_len);
    }
    return 0;
}

/* --- WSクライアント側コールバック --- */

static int on_client_connect(QS_EVENT_PARAMETER params)
{
    (void)params;
    if (api_qs_websocket_client_get_state(g_client) != QS_WEBSOCKET_STATE_OPEN) {
        g_failed = 1;
        return -1;
    }
    /* テキストメッセージ送信 */
    api_qs_websocket_client_send(g_client, 0, "ws-binary-test-hello", 20);
    return 0;
}

static int on_client_ws_event(QS_EVENT_PARAMETER params)
{
    uint8_t* payload = api_qs_get_websocket_payload(params);
    size_t payload_len = api_qs_get_websocket_payload_length(params);
    uint8_t opcode = api_qs_get_websocket_opcode(params);
    if (payload == NULL || payload_len == 0) return 0;

    if (opcode == 1 && !g_text_echo_received) {
        /* テキストecho受信 */
        if (payload_len == 20 && memcmp(payload, "ws-binary-test-hello", 20) == 0) {
            g_client_text_opcode = opcode;
            g_text_echo_received = 1;
            /* バイナリメッセージ送信（NULバイト含む） */
            for (int i = 0; i < BINARY_PAYLOAD_SIZE; i++) {
                g_send_payload[i] = (uint8_t)(i * 7 + 1);
            }
            g_send_payload[3] = 0; /* NULバイトを混ぜる */
            api_qs_websocket_client_send(g_client, 1, g_send_payload, BINARY_PAYLOAD_SIZE);
        }
    } else if (opcode == 2 && !g_binary_echo_received) {
        /* バイナリecho受信 */
        if (payload_len == BINARY_PAYLOAD_SIZE &&
            memcmp(payload, g_send_payload, BINARY_PAYLOAD_SIZE) == 0) {
            g_client_binary_opcode = opcode;
            memcpy(g_client_binary_payload, payload, BINARY_PAYLOAD_SIZE);
            g_client_binary_payload_len = payload_len;
            g_binary_echo_received = 1;
        }
    }
    return 0;
}

static int on_client_close(QS_EVENT_PARAMETER params)
{
    (void)params;
    return 0;
}

int main(void)
{
    api_qs_init();

    QS_SERVER_CONTEXT* context = 0;
    if (api_qs_server_init(&context, TEST_PORT, 10, QS_SERVER_TYPE_HTTP) < 0) {
        printf("server init failed\n");
        return 1;
    }
    g_context = context;
    api_qs_set_on_websocket_event(context, on_ws_event);
    g_server_running = 1;

    pthread_t server_tid;
    if (pthread_create(&server_tid, NULL, server_thread, NULL) != 0) {
        printf("server pthread_create failed\n");
        api_qs_free(context);
        return 1;
    }
    usleep(100 * 1000);

    if (api_qs_websocket_client_init(&g_client, "127.0.0.1", TEST_PORT, "/") != 0) {
        printf("websocket client init failed\n");
        g_failed = 1;
    } else {
        api_qs_websocket_client_set_max_message_size(g_client, 4096);
        api_qs_set_client_on_connect_event(g_client, on_client_connect);
        api_qs_set_websocket_client_on_event(g_client, on_client_ws_event);
        api_qs_set_client_on_close_event(g_client, on_client_close);
        g_client_running = 1;
        pthread_t client_tid;
        if (pthread_create(&client_tid, NULL, client_thread, NULL) != 0) {
            printf("client pthread_create failed\n");
            g_failed = 1;
        } else {
            /* バイナリecho受信を待つ */
            time_t deadline = time(NULL) + 10;
            while (time(NULL) < deadline && !g_binary_echo_received && !g_failed) {
                usleep(10 * 1000);
            }

            check("text echo received", g_text_echo_received == 1);
            check("text echo opcode is 1 (text)", g_client_text_opcode == 1);
            check("binary echo received", g_binary_echo_received == 1);
            check("binary echo opcode is 2 (binary)", g_client_binary_opcode == 2);
            check("binary echo payload length", g_client_binary_payload_len == BINARY_PAYLOAD_SIZE);
            check("binary echo payload content (incl. NUL byte)",
                  g_client_binary_payload_len == BINARY_PAYLOAD_SIZE &&
                  memcmp(g_client_binary_payload, g_send_payload, BINARY_PAYLOAD_SIZE) == 0);
            check("server saw text opcode 1", g_server_seen_text_opcode == 1);
            check("server saw binary opcode 2", g_server_seen_binary_opcode == 2);
            check("server verified binary payload", g_server_binary_payload_ok == 1);

            g_client_running = 0;
            pthread_join(client_tid, NULL);
            api_qs_client_free(g_client);
        }
    }

    g_server_running = 0;
    pthread_join(server_tid, NULL);
    api_qs_free(context);

    printf("\n=== ws_binary_test summary ===\n");
    printf("pass: %d, fail: %d\n", pass_count, fail_count);
    return (fail_count > 0) ? 1 : 0;
}
