/*
 * Copyright (c) Katsuya Owari
 *
 * http_param_test
 *   HTTPパラメータ解析APIをテストする。
 *
 * 検証内容:
 *   - api_qs_get_http_get_parameter（GETクエリ・URLデコード）
 *   - api_qs_get_http_post_parameter（POST form）
 *   - api_qs_get_http_post_json_object（POST JSON）
 *   - api_qs_get_http_cookie_parameter（Cookie）
 *   - api_qs_http_response_json（JSONレスポンス）
 *
 * 方式:
 *   - HTTPサーバー（高番ポート）を別スレッドで api_qs_update ループ実行
 *   - on_http_event コールバック内で各パラメータAPIを検証し、
 *     api_qs_http_response_json でJSONレスポンスを返す
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

#define TEST_PORT 18603

static QS_SERVER_CONTEXT* g_context = 0;
static volatile int g_server_running = 0;
static QS_MEMORY_CONTEXT g_temporary_memory;

/* サーバーupdateループを別スレッドで実行 */
static void* server_thread(void* arg)
{
    (void)arg;
    while (g_server_running) {
        api_qs_update(g_context);
        usleep(1000);
    }
    return NULL;
}

/* --- サーバー側コールバック --- */

static int on_http_event(QS_EVENT_PARAMETER params)
{
    int http_status_code = 404;
    const char* method = api_qs_get_http_method(params);
    const char* path = api_qs_get_http_path(params);
    if (method == NULL || path == NULL) return 404;

    api_qs_memory_clean(&g_temporary_memory);

    /* GET クエリパラメータ */
    if (!strcmp(method, "GET") && !strcmp(path, "/api/echo_get")) {
        char* a = api_qs_get_http_get_parameter(params, "a");
        char* b = api_qs_get_http_get_parameter(params, "b");
        char* sp = api_qs_get_http_get_parameter(params, "sp");
        char* missing = api_qs_get_http_get_parameter(params, "missing");
        QS_JSON_ELEMENT_OBJECT object;
        api_qs_object_create(&g_temporary_memory, &object);
        api_qs_object_push_string(&object, "a", a ? a : "");
        api_qs_object_push_string(&object, "b", b ? b : "");
        api_qs_object_push_string(&object, "sp", sp ? sp : "");
        api_qs_object_push_string(&object, "missing", missing ? missing : "");
        http_status_code = api_qs_http_response_json(params, &object, 1024 * 64);
    }

    /* POST form パラメータ */
    if (!strcmp(method, "POST") && !strcmp(path, "/api/echo_post")) {
        char* k1 = api_qs_get_http_post_parameter(params, "k1");
        char* k2 = api_qs_get_http_post_parameter(params, "k2");
        char* k3 = api_qs_get_http_post_parameter(params, "k3");
        QS_JSON_ELEMENT_OBJECT object;
        api_qs_object_create(&g_temporary_memory, &object);
        api_qs_object_push_string(&object, "k1", k1 ? k1 : "");
        api_qs_object_push_string(&object, "k2", k2 ? k2 : "");
        api_qs_object_push_string(&object, "k3", k3 ? k3 : "");
        http_status_code = api_qs_http_response_json(params, &object, 1024 * 64);
    }

    /* POST JSON パラメータ */
    if (!strcmp(method, "POST") && !strcmp(path, "/api/echo_json")) {
        QS_JSON_ELEMENT_OBJECT post;
        api_qs_get_http_post_json_object(params, &post);
        char* name = api_qs_object_get_string(&post, "name");
        int32_t* count = api_qs_object_get_integer(&post, "count");
        QS_JSON_ELEMENT_OBJECT nested;
        api_qs_object_get_object(&post, "nested", &nested);
        char* nx = api_qs_object_get_string(&nested, "x");

        QS_JSON_ELEMENT_OBJECT object;
        api_qs_object_create(&g_temporary_memory, &object);
        api_qs_object_push_string(&object, "name", name ? name : "");
        api_qs_object_push_integer(&object, "count", count ? *count : -1);
        api_qs_object_push_string(&object, "nx", nx ? nx : "");
        http_status_code = api_qs_http_response_json(params, &object, 1024 * 64);
    }

    /* Cookie パラメータ */
    if (!strcmp(method, "GET") && !strcmp(path, "/api/echo_cookie")) {
        char* c1 = api_qs_get_http_cookie_parameter(params, "c1");
        char* c2 = api_qs_get_http_cookie_parameter(params, "c2");
        char* c3 = api_qs_get_http_cookie_parameter(params, "c3");
        QS_JSON_ELEMENT_OBJECT object;
        api_qs_object_create(&g_temporary_memory, &object);
        api_qs_object_push_string(&object, "c1", c1 ? c1 : "");
        api_qs_object_push_string(&object, "c2", c2 ? c2 : "");
        api_qs_object_push_string(&object, "c3", c3 ? c3 : "");
        http_status_code = api_qs_http_response_json(params, &object, 1024 * 64);
    }

    return http_status_code;
}

/* --- クライアント側ヘルパー --- */

/* 生ソケットで HTTP リクエストを送り、レスポンスを返す。
   成功時 0、失敗時 -1。レスポンスは resp に格納（resp_size 以内）。 */
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

/* ヘッダとボディの境界（\r\n\r\n）の次の位置を返す。見つからなければ NULL */
static const char* get_body(const char* resp)
{
    const char* p = strstr(resp, "\r\n\r\n");
    if (p == NULL) return NULL;
    return p + 4;
}

int main(void)
{
    api_qs_init();
    if (api_qs_memory_alloc(&g_temporary_memory, 1024 * 1024 * 4) != 0) {
        printf("memory alloc failed\n");
        return 1;
    }

    QS_SERVER_CONTEXT* context = 0;
    if (api_qs_server_init(&context, TEST_PORT, 10, QS_SERVER_TYPE_HTTP) < 0) {
        printf("server init failed\n");
        api_qs_memory_free(&g_temporary_memory);
        return 1;
    }
    g_context = context;
    api_qs_set_on_http_event(context, on_http_event);
    g_server_running = 1;

    pthread_t tid;
    if (pthread_create(&tid, NULL, server_thread, NULL) != 0) {
        printf("pthread_create failed\n");
        api_qs_free(context);
        api_qs_memory_free(&g_temporary_memory);
        return 1;
    }

    /* サーバー初期化の完了を待つ */
    usleep(100 * 1000);

    char resp[8192];

    /* --- GET クエリパラメータ --- */
    {
        const char* req =
            "GET /api/echo_get?a=hello&b=world&sp=hello%20world HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Connection: close\r\n"
            "\r\n";
        int rc = http_request(req, resp, sizeof(resp));
        check("GET echo_get returns response", rc == 0);
        if (rc == 0) {
            check("GET echo_get status 200", get_status_code(resp) == 200);
            const char* body = get_body(resp);
            check("GET echo_get has body", body != NULL);
            if (body != NULL) {
                check("GET echo_get a=hello", strstr(body, "\"a\":\"hello\"") != NULL);
                check("GET echo_get b=world", strstr(body, "\"b\":\"world\"") != NULL);
                check("GET echo_get urldecode sp", strstr(body, "\"sp\":\"hello world\"") != NULL);
                check("GET echo_get missing param empty", strstr(body, "\"missing\":\"\"") != NULL);
            }
        }
    }

    /* --- POST form パラメータ --- */
    {
        const char* body_str = "k1=v1&k2=v2";
        char req[1024];
        snprintf(req, sizeof(req),
            "POST /api/echo_post HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Content-Type: application/x-www-form-urlencoded\r\n"
            "Content-Length: %zu\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s", strlen(body_str), body_str);
        int rc = http_request(req, resp, sizeof(resp));
        check("POST echo_post returns response", rc == 0);
        if (rc == 0) {
            check("POST echo_post status 200", get_status_code(resp) == 200);
            const char* body = get_body(resp);
            check("POST echo_post has body", body != NULL);
            if (body != NULL) {
                check("POST echo_post k1=v1", strstr(body, "\"k1\":\"v1\"") != NULL);
                check("POST echo_post k2=v2", strstr(body, "\"k2\":\"v2\"") != NULL);
                check("POST echo_post missing param empty", strstr(body, "\"k3\":\"\"") != NULL);
            }
        }
    }

    /* --- POST JSON パラメータ --- */
    {
        const char* body_str = "{\"name\":\"test\",\"count\":42,\"nested\":{\"x\":\"y\"}}";
        char req[1024];
        snprintf(req, sizeof(req),
            "POST /api/echo_json HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %zu\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s", strlen(body_str), body_str);
        int rc = http_request(req, resp, sizeof(resp));
        check("POST echo_json returns response", rc == 0);
        if (rc == 0) {
            check("POST echo_json status 200", get_status_code(resp) == 200);
            const char* ct = strstr(resp, "Content-Type: ");
            check("POST echo_json Content-Type application/json",
                  ct != NULL && strstr(ct, "application/json") != NULL);
            const char* body = get_body(resp);
            check("POST echo_json has body", body != NULL);
            if (body != NULL) {
                check("POST echo_json name=test", strstr(body, "\"name\":\"test\"") != NULL);
                check("POST echo_json count=42", strstr(body, "\"count\":42") != NULL);
                check("POST echo_json nested.x=y", strstr(body, "\"nx\":\"y\"") != NULL);
            }
        }
    }

    /* --- Cookie パラメータ --- */
    {
        const char* req =
            "GET /api/echo_cookie HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Cookie: c1=abc; c2=xyz\r\n"
            "Connection: close\r\n"
            "\r\n";
        int rc = http_request(req, resp, sizeof(resp));
        check("GET echo_cookie returns response", rc == 0);
        if (rc == 0) {
            check("GET echo_cookie status 200", get_status_code(resp) == 200);
            const char* body = get_body(resp);
            check("GET echo_cookie has body", body != NULL);
            if (body != NULL) {
                check("GET echo_cookie c1=abc", strstr(body, "\"c1\":\"abc\"") != NULL);
                check("GET echo_cookie c2=xyz", strstr(body, "\"c2\":\"xyz\"") != NULL);
                check("GET echo_cookie missing cookie empty", strstr(body, "\"c3\":\"\"") != NULL);
            }
        }
    }

    g_server_running = 0;
    pthread_join(tid, NULL);
    api_qs_free(context);
    api_qs_memory_free(&g_temporary_memory);

    printf("\n=== http_param_test summary ===\n");
    printf("pass: %d, fail: %d\n", pass_count, fail_count);
    return (fail_count > 0) ? 1 : 0;
}
