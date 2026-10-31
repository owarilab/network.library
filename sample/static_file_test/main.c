/*
 * Copyright (c) Katsuya Owari
 *
 * static_file_test
 *   core の HTTP サーバー組み込み「静的ファイル配信」機能をテストする。
 *
 * 検証内容:
 *   - `/` で index.html が配信される（Content-Type: text/html）
 *   - `.css` の Content-Type / Cache-Control / Last-Modified
 *   - `.css` + Cache-Control: max-age=0 + If-Modified-Since で 304 Not Modified
 *   - `.json` の Content-Type
 *   - 存在しないファイルで 404
 *
 * 方式:
 *   - ./www 配下に一時ファイルを生成し、HTTPサーバー（高番ポート）を起動
 *   - 生ソケットで HTTP リクエストを送り、レスポンスヘッダを検証
 *   - 終了時にサーバー解放と一時ファイルのクリーンアップ
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

#define TEST_PORT 18601
#define WWW_DIR "./www"
#define INDEX_FILE (WWW_DIR "/index.html")
#define CSS_FILE (WWW_DIR "/style.css")
#define JSON_FILE (WWW_DIR "/data.json")
#define MISSING_FILE (WWW_DIR "/no_such_file.css")

static QS_SERVER_CONTEXT* g_context = 0;
static volatile int g_server_running = 0;

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

static void cleanup_files(void)
{
    unlink(INDEX_FILE);
    unlink(CSS_FILE);
    unlink(JSON_FILE);
    rmdir(WWW_DIR);
}

/* 一時 www ファイルを生成 */
static int setup_files(void)
{
    cleanup_files();
    if (mkdir(WWW_DIR, 0755) != 0 && errno != EEXIST) return -1;

    FILE* f = fopen(INDEX_FILE, "w");
    if (!f) return -1;
    fprintf(f, "<html><body>static file test index</body></html>");
    fclose(f);

    f = fopen(CSS_FILE, "w");
    if (!f) return -1;
    fprintf(f, "body { color: red; }");
    fclose(f);

    f = fopen(JSON_FILE, "w");
    if (!f) return -1;
    fprintf(f, "{\"key\":\"value\"}");
    fclose(f);
    return 0;
}

/* 生ソケットで HTTP リクエストを送り、レスポンスを返す。
   成功時 0、失敗時 -1。レスポンスは resp に格納（resp_size 以内）。 */
static int http_request(const char* request, char* resp, size_t resp_size)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    /* 受信タイムアウト 3秒 */
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

/* レスポンス先頭からステータスコードを抽出 */
static int get_status_code(const char* resp)
{
    int code = 0;
    if (sscanf(resp, "HTTP/1.1 %d", &code) != 1) return -1;
    return code;
}

/* ヘッダ値を抽出（"Name: " で始まり次の \r\n まで）。見つからなければ NULL */
static const char* get_header(const char* resp, const char* name)
{
    const char* p = strstr(resp, name);
    if (p == NULL) return NULL;
    return p + strlen(name);
}

int main(void)
{
    if (setup_files() != 0) {
        printf("setup_files failed\n");
        return 1;
    }

    api_qs_init();
    QS_MEMORY_CONTEXT temporary_memory;
    if (api_qs_memory_alloc(&temporary_memory, 1024 * 1024 * 4) != 0) {
        printf("memory alloc failed\n");
        cleanup_files();
        return 1;
    }

    QS_SERVER_CONTEXT* context = 0;
    if (api_qs_server_init(&context, TEST_PORT, 10, QS_SERVER_TYPE_HTTP) < 0) {
        printf("server init failed\n");
        api_qs_memory_free(&temporary_memory);
        cleanup_files();
        return 1;
    }
    g_context = context;
    g_server_running = 1;

    pthread_t tid;
    if (pthread_create(&tid, NULL, server_thread, NULL) != 0) {
        printf("pthread_create failed\n");
        api_qs_free(context);
        api_qs_memory_free(&temporary_memory);
        cleanup_files();
        return 1;
    }

    /* サーバー初期化の完了を待つ */
    usleep(100 * 1000);

    char resp[8192];

    /* --- / で index.html が配信される --- */
    {
        const char* req =
            "GET / HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Connection: close\r\n"
            "\r\n";
        int rc = http_request(req, resp, sizeof(resp));
        check("GET / returns response", rc == 0);
        if (rc == 0) {
            check("GET / status 200", get_status_code(resp) == 200);
            const char* ct = get_header(resp, "Content-Type: ");
            check("GET / Content-Type text/html", ct != NULL && strstr(ct, "text/html") != NULL);
            check("GET / body contains index content",
                  strstr(resp, "static file test index") != NULL);
        }
    }

    /* --- .css の Content-Type / Cache-Control / Last-Modified --- */
    {
        const char* req =
            "GET /style.css HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Connection: close\r\n"
            "\r\n";
        int rc = http_request(req, resp, sizeof(resp));
        check("GET /style.css returns response", rc == 0);
        if (rc == 0) {
            check("GET /style.css status 200", get_status_code(resp) == 200);
            const char* ct = get_header(resp, "Content-Type: ");
            check("GET /style.css Content-Type text/css", ct != NULL && strstr(ct, "text/css") != NULL);
            const char* cc = get_header(resp, "Cache-Control: ");
            check("GET /style.css Cache-Control max-age=30", cc != NULL && strstr(cc, "max-age=30") != NULL);
            const char* lm = get_header(resp, "Last-Modified: ");
            check("GET /style.css has Last-Modified", lm != NULL && strlen(lm) > 0);
            check("GET /style.css body contains css", strstr(resp, "color: red") != NULL);
        }
    }

    /* --- .css + If-Modified-Since で 304 --- */
    {
        /* まず Last-Modified を取得 */
        const char* req1 =
            "GET /style.css HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Connection: close\r\n"
            "\r\n";
        char resp1[8192];
        int rc1 = http_request(req1, resp1, sizeof(resp1));
        check("304 test: first GET /style.css ok", rc1 == 0);

        if (rc1 == 0) {
            const char* lm = get_header(resp1, "Last-Modified: ");
            check("304 test: got Last-Modified", lm != NULL && strlen(lm) > 0);

            if (lm != NULL && strlen(lm) > 0) {
                /* 行末の \r\n を除去 */
                char lm_value[128];
                size_t len = strlen(lm);
                while (len > 0 && (lm[len-1] == '\r' || lm[len-1] == '\n')) len--;
                if (len >= sizeof(lm_value)) len = sizeof(lm_value) - 1;
                memcpy(lm_value, lm, len);
                lm_value[len] = '\0';

                char req2[512];
                snprintf(req2, sizeof(req2),
                    "GET /style.css HTTP/1.1\r\n"
                    "Host: localhost\r\n"
                    "Connection: close\r\n"
                    "Cache-Control: max-age=0\r\n"
                    "If-Modified-Since: %s\r\n"
                    "\r\n", lm_value);

                int rc2 = http_request(req2, resp, sizeof(resp));
                check("304 test: conditional GET returns response", rc2 == 0);
                if (rc2 == 0) {
                    check("304 test: status 304", get_status_code(resp) == 304);
                    check("304 test: no body", strstr(resp, "color: red") == NULL);
                }
            }
        }
    }

    /* --- .json の Content-Type --- */
    {
        const char* req =
            "GET /data.json HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Connection: close\r\n"
            "\r\n";
        int rc = http_request(req, resp, sizeof(resp));
        check("GET /data.json returns response", rc == 0);
        if (rc == 0) {
            check("GET /data.json status 200", get_status_code(resp) == 200);
            const char* ct = get_header(resp, "Content-Type: ");
            check("GET /data.json Content-Type application/json",
                  ct != NULL && strstr(ct, "application/json") != NULL);
            check("GET /data.json body contains json", strstr(resp, "\"key\":\"value\"") != NULL);
        }
    }

    /* --- 存在しないファイルで 404 --- */
    {
        const char* req =
            "GET /no_such_file.css HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Connection: close\r\n"
            "\r\n";
        int rc = http_request(req, resp, sizeof(resp));
        check("GET missing file returns response", rc == 0);
        if (rc == 0) {
            check("GET missing file status 404", get_status_code(resp) == 404);
        }
    }

    g_server_running = 0;
    pthread_join(tid, NULL);
    api_qs_free(context);
    api_qs_memory_free(&temporary_memory);
    cleanup_files();

    printf("\n=== static_file_test summary ===\n");
    printf("pass: %d, fail: %d\n", pass_count, fail_count);
    return (fail_count > 0) ? 1 : 0;
}
