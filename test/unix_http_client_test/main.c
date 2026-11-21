/*
 * Copyright (c) Katsuya Owari
 *
 * unix_http_client_test
 * ssl モジュールの qs_ssl_module_http_client_connect_unix 検証:
 *   case 1: 同一プロセスの AF_UNIX HTTP サーバーに対する GET
 *           -> status 200 / body 一致 / ヘッダ取得
 *   case 2: 存在しないソケットパス -> DISCONNECT まで進み、ハングしない
 */

#include "qs_api.h"
#include "qs_openssl_module.h"

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

#define TEST_SOCKET_PATH "/tmp/opencode/qs_unix_http_client_test.sock"
#define TEST_EXPECT_BODY "hello_unix_http"
#define TEST_EXPECT_CONTENT_TYPE "text/plain"
#define TEST_LOOP_LIMIT 3000 /* api_qs_client_sleep は 10ms 相当 -> 最大 30 秒 */

/* ---------------- AF_UNIX HTTP サーバー（raw socket） ---------------- */

static const char* SERVER_RESPONSE =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 15\r\n"
    "Connection: close\r\n"
    "\r\n"
    "hello_unix_http";

static int g_server_listen_fd = -1;

static void* server_thread(void* arg)
{
    (void)arg;
    int client_fd = accept(g_server_listen_fd, NULL, NULL);
    if(client_fd == -1){
        return NULL;
    }
    /* リクエストヘッダ終端（\r\n\r\n）が揃うまで受信 */
    char buf[8192];
    size_t total = 0;
    int request_done = 0;
    while(!request_done){
        ssize_t n = recv(client_fd, buf + total, sizeof(buf) - 1 - total, 0);
        if(n <= 0){
            break;
        }
        total += n;
        buf[total] = 0;
        if(strstr(buf, "\r\n\r\n") != NULL){
            request_done = 1;
        }
    }
    if(request_done){
        (void)send(client_fd, SERVER_RESPONSE, strlen(SERVER_RESPONSE), 0);
    }
    close(client_fd);
    return NULL;
}

static int start_http_server(void)
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

static void stop_http_server(void)
{
    if(g_server_listen_fd != -1){
        close(g_server_listen_fd);
        g_server_listen_fd = -1;
    }
    unlink(TEST_SOCKET_PATH);
}

/* ---------------- HTTP クライアント実行（sample/http_client と同じパターン） ---------------- */

typedef struct TEST_HTTP_RESULT
{
    int status_code;    /* 0: ステータス行なし（レスポンスなし） */
    char body[256];
    size_t body_length;
    char content_type[128];
} TEST_HTTP_RESULT;

static void run_get(QS_HTTP_CLIENT_CONTEXT* http, TEST_HTTP_RESULT* result)
{
    memset(result, 0, sizeof(*result));

    char* request_buffer = qs_ssl_module_http_client_get_request_buffer(http);
    if(request_buffer == NULL){
        return;
    }
    snprintf(request_buffer, QS_HTTP_CLIENT_REQUEST_BUFFER_SIZE,
             "GET /test HTTP/1.1\r\n"
             "Host: localhost\r\n"
             "Connection: close\r\n"
             "\r\n");

    for(int i = 0; i < TEST_LOOP_LIMIT; i++){
        qs_ssl_module_http_client_update(http);
        if(http->phase == QS_SSL_MODULE_PHASE_DISCONNECT){
            break;
        }
        api_qs_client_sleep(http->client_context);
    }

    /* free 後に body_length はリセットされるため、先に取得する */
    size_t body_length = http->body_length;
    qs_ssl_module_http_client_free(http);

    const char* header_buffer = qs_ssl_module_http_client_get_header_buffer(http);
    if(header_buffer != NULL && header_buffer[0] != 0){
        char http_version[16];
        char status_message[256];
        if(sscanf(header_buffer, "HTTP/%15s %d %255[^\r\n]", http_version, &result->status_code, status_message) >= 2){
            char* body_buffer = (char*)qs_ssl_module_http_client_get_body_buffer(http);
            if(body_buffer != NULL && body_length > 0){
                size_t copy = (body_length < sizeof(result->body) - 1) ? body_length : sizeof(result->body) - 1;
                memcpy(result->body, body_buffer, copy);
                result->body[copy] = 0;
                result->body_length = body_length;
            }
            qs_ssl_module_http_client_get_header(http, "Content-Type: ", result->content_type, sizeof(result->content_type));
        }
    }
}

/* ---------------- ケース ---------------- */

static int test_get(void)
{
    printf("case 1: GET via %s\n", TEST_SOCKET_PATH);
    if(0 != start_http_server()){
        printf("  fail: server start failed\n");
        return -1;
    }
    QS_HTTP_CLIENT_CONTEXT http;
    memset(&http, 0, sizeof(http));
    if(0 != qs_ssl_module_http_client_connect_unix(&http, TEST_SOCKET_PATH, 0)){
        printf("  fail: connect_unix failed\n");
        stop_http_server();
        return -1;
    }
    TEST_HTTP_RESULT result;
    run_get(&http, &result);
    qs_ssl_module_http_client_dispose(&http);
    stop_http_server();

    int result_ret = 0;
    if(result.status_code == 200 &&
       result.body_length == strlen(TEST_EXPECT_BODY) &&
       0 == strncmp(result.body, TEST_EXPECT_BODY, sizeof(result.body) - 1) &&
       0 == strcmp(result.content_type, TEST_EXPECT_CONTENT_TYPE)){
        printf("  ok: status=%d body='%s' content_type='%s'\n",
               result.status_code, result.body, result.content_type);
    }
    else{
        printf("  fail: status=%d body='%s' (len=%zu) content_type='%s'\n",
               result.status_code, result.body, result.body_length, result.content_type);
        result_ret = -1;
    }
    return result_ret;
}

static int test_missing_path(void)
{
    printf("case 2: missing socket path\n");
    const char* missing = "/tmp/opencode/qs_unix_http_missing.sock";
    unlink(missing);
    QS_HTTP_CLIENT_CONTEXT http;
    memset(&http, 0, sizeof(http));
    if(0 != qs_ssl_module_http_client_connect_unix(&http, missing, 0)){
        printf("  fail: connect_unix unexpectedly failed\n");
        return -1;
    }
    TEST_HTTP_RESULT result;
    run_get(&http, &result);
    qs_ssl_module_http_client_dispose(&http);

    int result_ret = 0;
    if(result.status_code == 0){
        printf("  ok: no response, reached DISCONNECT without hang\n");
    }
    else{
        printf("  fail: unexpected status %d\n", result.status_code);
        result_ret = -1;
    }
    return result_ret;
}

int main(void)
{
    api_qs_init();
    int failed = 0;
    failed |= test_get();
    failed |= test_missing_path();
    if(failed){
        printf("UNIX HTTP CLIENT TEST: FAILED\n");
        return 1;
    }
    printf("UNIX HTTP CLIENT TEST: ALL PASSED\n");
    return 0;
}
