/*
 * Copyright (c) Katsuya Owari
 *
 * room_test
 *   room機能（api_qs_room_create/list/join/leave）をHTTP API経由で検証し、
 *   WebSocketブロードキャストを確認する。
 *
 * 検証内容:
 *   - POST /api/v1/room/create でルーム作成（room_id取得）
 *   - GET  /api/v1/room/list でルームが列挙される
 *   - WS接続で connection_id を取得（サーバーがechoで返すJSONのid）
 *   - POST /api/v1/room/join でルーム参加
 *   - WSメッセージ送信でブロードキャスト受信を確認
 *   - POST /api/v1/room/leave でルーム退出
 *
 * 方式:
 *   - ルーター有効のHTTPサーバーを別スレッドで api_qs_update ループ実行
 *   - WSクライアントを別スレッドで api_qs_client_update ループ実行
 *   - メインスレッドから生ソケットで HTTP リクエストを送り、レスポンスを検証
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

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

#define TEST_PORT 18604

static QS_SERVER_CONTEXT* g_context = 0;
static QS_CLIENT_CONTEXT* g_client = 0;
static volatile int g_server_running = 0;
static volatile int g_client_running = 0;

static QS_MEMORY_CONTEXT g_main_memory;      /* メインスレッド用（HTTPレスポンス解析） */
static QS_MEMORY_CONTEXT g_client_memory;    /* WSクライアントコールバック用 */

static char g_room_id[128];
static char g_connection_id[128];
static volatile int g_joined = 0;
static volatile int g_message_received = 0;
static volatile int g_failed = 0;

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
    /* 受信メッセージをecho（ルーター有効のためJSONラップされて返る） */
    char* message = api_qs_get_ws_message(params);
    if (message != NULL) {
        api_qs_send_ws_message(params, message);
    }
    return 0;
}

/* --- WSクライアント側コールバック --- */

static int extract_json_string(const char* json, const char* key, char* value, size_t value_size)
{
    api_qs_memory_clean(&g_client_memory);
    QS_JSON_ELEMENT_OBJECT object;
    if (0 != api_qs_json_decode_object(&g_client_memory, &object, json)) {
        return -1;
    }
    char* result = api_qs_object_get_string(&object, key);
    if (result == NULL) return -1;
    if (strlen(result) >= value_size) return -1;
    snprintf(value, value_size, "%s", result);
    return 0;
}

static int on_client_connect(QS_EVENT_PARAMETER params)
{
    (void)params;
    if (api_qs_websocket_client_get_state(g_client) != QS_WEBSOCKET_STATE_OPEN) {
        g_failed = 1;
        return -1;
    }
    /* connection_id を取得するためhelloを送信（サーバーがechoでidを返す） */
    api_qs_websocket_client_send(g_client, 0, "hello", 5);
    return 0;
}

static int on_client_ws_event(QS_EVENT_PARAMETER params)
{
    uint8_t* payload = api_qs_get_websocket_payload(params);
    size_t payload_len = api_qs_get_websocket_payload_length(params);
    uint8_t opcode = api_qs_get_websocket_opcode(params);
    if (opcode != 1 || payload == NULL || payload_len == 0) return 0;

    char message[512];
    if (payload_len >= sizeof(message)) return 0;
    memcpy(message, payload, payload_len);
    message[payload_len] = '\0';

    /* 初回: echoのJSONからconnection_idを取得 */
    if (g_connection_id[0] == '\0' && strstr(message, "hello") != NULL) {
        if (extract_json_string(message, "id", g_connection_id, sizeof(g_connection_id)) != 0) {
            g_failed = 1;
        }
        return 0;
    }
    /* room参加後のテストメッセージ受信 */
    if (strstr(message, "room-test-msg") != NULL) {
        g_message_received = 1;
    }
    return 0;
}

static int on_client_close(QS_EVENT_PARAMETER params)
{
    (void)params;
    return 0;
}

/* --- HTTPクライアントヘルパー（生ソケット） --- */

static int http_request(const char* request, char* resp, size_t resp_size)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(TEST_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }

    size_t total_sent = 0;
    size_t req_len = strlen(request);
    while (total_sent < req_len) {
        ssize_t n = send(sock, request + total_sent, req_len - total_sent, 0);
        if (n <= 0) { close(sock); return -1; }
        total_sent += n;
    }

    size_t total = 0;
    for (;;) {
        if (total + 1 >= resp_size) break;
        ssize_t n = recv(sock, resp + total, resp_size - total - 1, 0);
        if (n <= 0) break;
        total += n;
    }
    resp[total] = '\0';
    close(sock);
    return (total > 0) ? 0 : -1;
}

static int get_status_code(const char* resp)
{
    int code = 0;
    if (sscanf(resp, "HTTP/1.1 %d", &code) != 1) return -1;
    return code;
}

static const char* get_body(const char* resp)
{
    const char* p = strstr(resp, "\r\n\r\n");
    if (p == NULL) return NULL;
    return p + 4;
}

/* メインスレッド用のJSON文字列抽出 */
static int main_extract_json_string(const char* json, const char* key, char* value, size_t value_size)
{
    api_qs_memory_clean(&g_main_memory);
    QS_JSON_ELEMENT_OBJECT object;
    if (0 != api_qs_json_decode_object(&g_main_memory, &object, json)) {
        return -1;
    }
    char* result = api_qs_object_get_string(&object, key);
    if (result == NULL) return -1;
    if (strlen(result) >= value_size) return -1;
    snprintf(value, value_size, "%s", result);
    return 0;
}

int main(void)
{
    api_qs_init();
    if (api_qs_memory_alloc(&g_main_memory, 1024 * 1024 * 4) != 0) {
        printf("main memory alloc failed\n");
        return 1;
    }
    if (api_qs_memory_alloc(&g_client_memory, 1024 * 1024 * 4) != 0) {
        printf("client memory alloc failed\n");
        api_qs_memory_free(&g_main_memory);
        return 1;
    }

    QS_SERVER_CONTEXT* context = 0;
    if (api_qs_server_init(&context, TEST_PORT, 10, QS_SERVER_TYPE_HTTP) < 0) {
        printf("server init failed\n");
        api_qs_memory_free(&g_main_memory);
        api_qs_memory_free(&g_client_memory);
        return 1;
    }
    if (-1 == api_qs_server_create_router(context)) {
        printf("server create router failed\n");
        api_qs_free(context);
        api_qs_memory_free(&g_main_memory);
        api_qs_memory_free(&g_client_memory);
        return 1;
    }
    g_context = context;
    api_qs_set_on_websocket_event(context, on_ws_event);
    g_server_running = 1;

    pthread_t server_tid;
    if (pthread_create(&server_tid, NULL, server_thread, NULL) != 0) {
        printf("server pthread_create failed\n");
        api_qs_free(context);
        api_qs_memory_free(&g_main_memory);
        api_qs_memory_free(&g_client_memory);
        return 1;
    }
    usleep(100 * 1000);

    char resp[8192];
    char req[1024];

    /* --- room/create --- */
    {
        snprintf(req, sizeof(req),
            "POST /api/v1/room/create HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Content-Type: application/x-www-form-urlencoded\r\n"
            "Content-Length: %zu\r\n"
            "Connection: close\r\n"
            "\r\n"
            "name=roomtest", strlen("name=roomtest"));
        int rc = http_request(req, resp, sizeof(resp));
        check("room/create returns response", rc == 0);
        if (rc == 0) {
            check("room/create status 200", get_status_code(resp) == 200);
            const char* body = get_body(resp);
            check("room/create has body", body != NULL);
            if (body != NULL) {
                check("room/create returns room id",
                      main_extract_json_string(body, "id", g_room_id, sizeof(g_room_id)) == 0 && g_room_id[0] != '\0');
            }
        }
    }

    /* --- room/list（作成後） --- */
    {
        const char* r =
            "GET /api/v1/room/list HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Connection: close\r\n"
            "\r\n";
        int rc = http_request(r, resp, sizeof(resp));
        check("room/list returns response", rc == 0);
        if (rc == 0) {
            check("room/list status 200", get_status_code(resp) == 200);
            check("room/list contains created room",
                  g_room_id[0] != '\0' && strstr(resp, g_room_id) != NULL);
        }
    }

    /* --- WSクライアント接続 --- */
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
            /* connection_id 取得を待つ */
            time_t deadline = time(NULL) + 5;
            while (time(NULL) < deadline && g_connection_id[0] == '\0' && !g_failed) {
                usleep(10 * 1000);
            }
            check("WS client got connection_id", g_connection_id[0] != '\0');

            if (g_connection_id[0] != '\0' && !g_failed) {
                /* --- room/join --- */
                snprintf(req, sizeof(req),
                    "POST /api/v1/room/join HTTP/1.1\r\n"
                    "Host: localhost\r\n"
                    "Content-Type: application/x-www-form-urlencoded\r\n"
                    "Content-Length: %zu\r\n"
                    "Connection: close\r\n"
                    "\r\n"
                    "room_id=%s&connection_id=%s",
                    strlen(g_room_id) + strlen(g_connection_id) + 20, g_room_id, g_connection_id);
                int rc = http_request(req, resp, sizeof(resp));
                check("room/join returns response", rc == 0);
                if (rc == 0) {
                    check("room/join status 200", get_status_code(resp) == 200);
                    g_joined = 1;
                }

                /* --- WSメッセージ送信 → ブロードキャスト受信 --- */
                if (g_joined) {
                    api_qs_websocket_client_send(g_client, 0, "room-test-msg", 13);
                    deadline = time(NULL) + 5;
                    while (time(NULL) < deadline && !g_message_received && !g_failed) {
                        usleep(10 * 1000);
                    }
                    check("WS broadcast message received", g_message_received == 1);
                }

                /* --- room/leave --- */
                if (g_joined) {
                    snprintf(req, sizeof(req),
                        "POST /api/v1/room/leave HTTP/1.1\r\n"
                        "Host: localhost\r\n"
                        "Content-Type: application/x-www-form-urlencoded\r\n"
                        "Content-Length: %zu\r\n"
                        "Connection: close\r\n"
                        "\r\n"
                        "room_id=%s&connection_id=%s",
                        strlen(g_room_id) + strlen(g_connection_id) + 20, g_room_id, g_connection_id);
                    int rc = http_request(req, resp, sizeof(resp));
                    check("room/leave returns response", rc == 0);
                    if (rc == 0) {
                        check("room/leave status 200", get_status_code(resp) == 200);
                    }
                }
            }

            g_client_running = 0;
            pthread_join(client_tid, NULL);
            api_qs_client_free(g_client);
        }
    }

    g_server_running = 0;
    pthread_join(server_tid, NULL);
    api_qs_free(context);
    api_qs_memory_free(&g_main_memory);
    api_qs_memory_free(&g_client_memory);

    printf("\n=== room_test summary ===\n");
    printf("pass: %d, fail: %d\n", pass_count, fail_count);
    return (fail_count > 0) ? 1 : 0;
}
