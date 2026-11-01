/*
 * Copyright (c) Katsuya Owari
 *
 * ab 風 HTTP ロードテストツール
 *
 * N 並列の同時接続を連続してリクエストし、スループット・応答時間・
 * エラー率・ステータスコード集計を計測する。
 *
 * 各ワーカーは QS_HTTP_CLIENT_CONTEXT を 1 つ持ち、独立した接続を持つ。
 * メインループで全ワーカーの update を 1 パス回すノンブロッキング方式で
 * N 並列を実現する。
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "qs_api.h"
#include "qs_openssl_module.h"

/* ワーカー状態 */
#define WORKER_STATE_IDLE    0
#define WORKER_STATE_ACTIVE  1

typedef struct
{
    long total_requests;   /* 総リクエスト数 */
    int  concurrency;      /* 同時接続数 */
    const char* host;
    int  port;
    const char* path;
    int  is_ssl;
    int  timeout_sec;      /* 全体タイムアウト（秒） */
} CONFIG;

typedef struct
{
    QS_HTTP_CLIENT_CONTEXT ctx;
    int    state;          /* WORKER_STATE_* */
    double start_time_ms;  /* リクエスト開始時刻 */
} WORKER;

typedef struct
{
    long completed;        /* レスポンスを受信したリクエスト数 */
    long failed;           /* 接続エラー等でレスポンス未取得 */
    long non_2xx;          /* 2xx 以外のレスポンス */
    long non_404;          /* 404 以外のレスポンス */
    long status_200;
    long status_404;
    long status_other;
    long total_bytes;      /* 受信ボディ合計バイト数 */
    double min_ms;
    double max_ms;
    double sum_ms;
    long   count;          /* 応答時間のサンプル数 */
} STATS;

/* 現在時刻（ミリ秒、単調増加） */
static double now_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec * 1000.0 + (double)tv.tv_usec / 1000.0;
}

static void print_usage(const char* prog)
{
    printf("Usage: %s [options]\n", prog);
    printf("  -n <count>        total requests (default 100)\n");
    printf("  -c <concurrency>  concurrent connections (default 10)\n");
    printf("  -H <host>         host (default localhost)\n");
    printf("  -p <port>         port (default 4444)\n");
    printf("  -P <path>         path (default /index.html)\n");
    printf("  -s                use SSL\n");
    printf("  -t <sec>          overall timeout seconds (default 30)\n");
}

/* ワーカー 1 つのリクエストを開始する（接続＋リクエストバッファ設定） */
static int start_request(WORKER* w, const CONFIG* cfg)
{
    if(0 != qs_ssl_module_http_client_connect(&w->ctx, cfg->host, cfg->port, cfg->is_ssl)){
        return -1;
    }
    char* req = qs_ssl_module_http_client_get_request_buffer(&w->ctx);
    if(req == NULL){
        qs_ssl_module_http_client_free(&w->ctx);
        return -1;
    }
    snprintf(req, QS_HTTP_CLIENT_REQUEST_BUFFER_SIZE,
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n"
        "\r\n",
        cfg->path,
        cfg->host);
    w->start_time_ms = now_ms();
    w->state = WORKER_STATE_ACTIVE;
    return 0;
}

/* ワーカー 1 つのリクエストを完了処理する（計測＋解放） */
static void finish_request(WORKER* w, STATS* stats)
{
    double elapsed = now_ms() - w->start_time_ms;
    const char* header = qs_ssl_module_http_client_get_header_buffer(&w->ctx);
    int status_code = 0;

    if(header != NULL){
        char http_version[16];
        char status_message[1024];
        memset(http_version, 0, sizeof(http_version));
        memset(status_message, 0, sizeof(status_message));
        if(sscanf(header, "HTTP/%15s %d %1023[^\r\n]", http_version, &status_code, status_message) >= 2){
            /* ステータス行を正常に取得 */
        }
    }

    if(header != NULL && status_code > 0){
        stats->completed++;
        if(status_code >= 200 && status_code < 300){
            stats->status_200++;
        }else if(status_code == 404){
            stats->status_404++;
        }else{
            stats->status_other++;
        }
        if(!(status_code >= 200 && status_code < 300)){
            stats->non_2xx++;
        }
        if(status_code != 404){
            stats->non_404++;
        }
        stats->total_bytes += (long)w->ctx.body_length;
        stats->sum_ms += elapsed;
        stats->count++;
        if(stats->count == 1 || elapsed < stats->min_ms){
            stats->min_ms = elapsed;
        }
        if(elapsed > stats->max_ms){
            stats->max_ms = elapsed;
        }
    }else{
        stats->failed++;
    }

    qs_ssl_module_http_client_free(&w->ctx);
    w->state = WORKER_STATE_IDLE;
}

static void print_result(const CONFIG* cfg, const STATS* stats, double elapsed_sec, int timed_out)
{
    double rps = (elapsed_sec > 0.0) ? (double)stats->completed / elapsed_sec : 0.0;
    double avg = (stats->count > 0) ? stats->sum_ms / (double)stats->count : 0.0;
    double transfer_kbps = (elapsed_sec > 0.0) ? ((double)stats->total_bytes / 1024.0) / elapsed_sec : 0.0;

    printf("\n");
    printf("Server Hostname:      %s\n", cfg->host);
    printf("Server Port:          %d\n", cfg->port);
    printf("SSL:                  %s\n", cfg->is_ssl ? "yes" : "no");
    printf("Document Path:        %s\n", cfg->path);
    printf("\n");
    printf("Concurrency Level:    %d\n", cfg->concurrency);
    printf("Time taken for tests: %.4f seconds\n", elapsed_sec);
    printf("Complete requests:    %ld\n", stats->completed);
    printf("Failed requests:      %ld\n", stats->failed);
    printf("Non-2xx responses:    %ld\n", stats->non_2xx);
    printf("Non-404 responses:    %ld\n", stats->non_404);
    printf("\n");
    printf("Status code distribution:\n");
    printf("  200:   %ld\n", stats->status_200);
    printf("  404:   %ld\n", stats->status_404);
    printf("  other: %ld\n", stats->status_other);
    printf("\n");
    printf("Connection times (ms)\n");
    printf("  min: %.2f\n", stats->min_ms);
    printf("  avg: %.2f\n", avg);
    printf("  max: %.2f\n", stats->max_ms);
    printf("\n");
    printf("Requests per second:  %.2f [#/sec] (mean)\n", rps);
    printf("Transfer rate:        %.2f [Kbytes/sec] received\n", transfer_kbps);
    if(timed_out){
        printf("\n[TIMEOUT] overall timeout (%d sec) reached\n", cfg->timeout_sec);
    }
}

int main(int argc, char *argv[], char *envp[])
{
#ifdef __WINDOWS__
	SetConsoleOutputCP(CP_UTF8);
#endif

    CONFIG cfg;
    cfg.total_requests = 100;
    cfg.concurrency    = 10;
    cfg.host           = "localhost";
    cfg.port           = 4444;
    cfg.path           = "/index.html";
    cfg.is_ssl         = 0;
    cfg.timeout_sec    = 30;

    int opt;
    while((opt = getopt(argc, argv, "n:c:H:p:P:st:")) != -1){
        switch(opt){
            case 'n': cfg.total_requests = atol(optarg); break;
            case 'c': cfg.concurrency    = atoi(optarg); break;
            case 'H': cfg.host           = optarg; break;
            case 'p': cfg.port           = atoi(optarg); break;
            case 'P': cfg.path           = optarg; break;
            case 's': cfg.is_ssl         = 1; break;
            case 't': cfg.timeout_sec    = atoi(optarg); break;
            default:
                print_usage(argv[0]);
                return -1;
        }
    }

    if(cfg.total_requests <= 0 || cfg.concurrency <= 0 || cfg.port <= 0){
        print_usage(argv[0]);
        return -1;
    }

    WORKER* workers = (WORKER*)calloc((size_t)cfg.concurrency, sizeof(WORKER));
    if(workers == NULL){
        printf("failed to allocate workers\n");
        return -1;
    }

    STATS stats;
    memset(&stats, 0, sizeof(stats));

    long remaining = cfg.total_requests;  /* 未開始 */
    long in_flight = 0;                   /* 開始済み・未完了 */
    int  timed_out = 0;
    double start = now_ms();

    printf("Starting load test: %ld requests, concurrency %d, %s:%d%s\n",
        cfg.total_requests, cfg.concurrency, cfg.host, cfg.port, cfg.path);

    while(remaining > 0 || in_flight > 0){
        /* タイムアウトチェック */
        if(cfg.timeout_sec > 0 && (now_ms() - start) > (double)cfg.timeout_sec * 1000.0){
            timed_out = 1;
            break;
        }

        /* アイドルなワーカーに新しいリクエストを開始 */
        for(int i = 0; i < cfg.concurrency && remaining > 0; i++){
            if(workers[i].state == WORKER_STATE_IDLE){
                if(start_request(&workers[i], &cfg) == 0){
                    remaining--;
                    in_flight++;
                }else{
                    stats.failed++;
                    remaining--;
                }
            }
        }

        /* アクティブなワーカーを 1 パス update */
        for(int i = 0; i < cfg.concurrency; i++){
            if(workers[i].state == WORKER_STATE_ACTIVE){
                qs_ssl_module_http_client_update(&workers[i].ctx);
                if(workers[i].ctx.phase == QS_SSL_MODULE_PHASE_DISCONNECT){
                    finish_request(&workers[i], &stats);
                    in_flight--;
                }
            }
        }

        /* 短い sleep（api_qs_client_sleep は qs_sleep(10) を呼ぶ） */
        QS_CLIENT_CONTEXT* sleep_ctx = NULL;
        for(int i = 0; i < cfg.concurrency; i++){
            if(workers[i].state == WORKER_STATE_ACTIVE){
                sleep_ctx = workers[i].ctx.client_context;
                break;
            }
        }
        api_qs_client_sleep(sleep_ctx);
    }

    double elapsed_sec = (now_ms() - start) / 1000.0;

    /* 未完了のワーカー（タイムアウト等）を解放 */
    for(int i = 0; i < cfg.concurrency; i++){
        if(workers[i].state == WORKER_STATE_ACTIVE){
            qs_ssl_module_http_client_dispose(&workers[i].ctx);
        }
    }
    free(workers);

    print_result(&cfg, &stats, elapsed_sec, timed_out);
    return 0;
}