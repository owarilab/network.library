/*
 * Copyright (c) Katsuya Owari
 *
 * docker_cli: qs core + ssl モジュール（QS_HTTP_CLIENT_CONTEXT）のみで
 *             Docker Engine API（plain HTTP / TCP）をコマンド引数から操作する CLI サンプル
 *
 * 使い方:
 *   ./qs_docker_cli [--host HOST] [--port PORT] <command> [args]
 *
 * 詳細は README.md を参照
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "qs_api.h"
#include "qs_openssl_module.h"

#define DOCKER_CLI_HOST_SIZE 256
#define DOCKER_CLI_JSON_MEMORY_SIZE (1024 * 1024 * 16)
#define DOCKER_CLI_CREATE_BODY_SIZE (1024 * 64)
#define DOCKER_CLI_ID_SIZE 256
#define DOCKER_CLI_DEFAULT_SOCKET "/var/run/docker.sock"

typedef struct DOCKER_CLI_CONTEXT
{
    char host[DOCKER_CLI_HOST_SIZE];
    int port;
    char unix_path[DOCKER_CLI_HOST_SIZE];
    int use_unix;
    QS_HTTP_CLIENT_CONTEXT http;
    QS_MEMORY_CONTEXT json_memory;
} DOCKER_CLI_CONTEXT;

static void print_usage(void)
{
    printf("docker_cli (qs core + qs ssl module sample)\n");
    printf("Usage:\n");
    printf("  docker_cli [--host HOST] [--port PORT] <command> [args]\n");
    printf("\n");
    printf("Options:\n");
    printf("  --host HOST           Docker Engine host for TCP (default: 127.0.0.1)\n");
    printf("  --port PORT           Docker Engine port for TCP (default: 2375)\n");
    printf("  --unix-socket PATH    Connect via unix domain socket\n");
    printf("                        (default: %s if it exists, otherwise TCP 127.0.0.1:2375)\n",
           DOCKER_CLI_DEFAULT_SOCKET);
    printf("\n");
    printf("Commands:\n");
    printf("  version                  Show engine version info (GET /version)\n");
    printf("  info                     Show engine info (GET /info)\n");
    printf("  ps [--all]               List containers (GET /containers/json)\n");
    printf("  images                   List images (GET /images/json)\n");
    printf("  run IMAGE [CMD...]       Create and start a container\n");
    printf("  start <container>        Start a container\n");
    printf("  stop <container>         Stop a container\n");
    printf("  restart <container>      Restart a container\n");
    printf("  kill <container> [--signal NAME]  Send a signal (default: KILL)\n");
    printf("  rm <container> [--force] Remove a container\n");
    printf("  logs <container>         Show container logs\n");
    printf("  top <container>          List processes in a container\n");
    printf("  port <container>         List port mappings of a container\n");
    printf("  wait <container>         Block until it stops, then print the exit code\n");
    printf("  stats <container>        Show a one-shot resource usage sample\n");
    printf("  rmi <image> [--force]    Remove an image\n");
    printf("  tag <SOURCE> <TARGET>    Tag an image\n");
    printf("  history <image>          Show the image history\n");
    printf("  network ls               List networks (GET /networks)\n");
    printf("  volume ls                List volumes (GET /volumes)\n");
    printf("  prune <kind> [--filter key=value]  Prune images/containers/volumes\n");
}

/* HTTP パスに埋め込むトークン（コンテナ ID・名前・イメージ名）の文字種チェック */
static int is_safe_path_token(const char* token)
{
    if(token == NULL || *token == 0){
        return 0;
    }
    for(const char* p = token; *p != 0; p++){
        char c = *p;
        int ok = (c >= '0' && c <= '9') ||
                 (c >= 'a' && c <= 'z') ||
                 (c >= 'A' && c <= 'Z') ||
                 c == '.' || c == ':' || c == '-' || c == '_' || c == '/';
        if(!ok){
            return 0;
        }
    }
    return 1;
}

/*
 * HTTP リクエストを実行してレスポンスを待つ（ワンショット）
 *
 * body が NULL の場合は Content-Length なし（GET/DELETE）
 * 成功時は out_body に malloc されたボディコピー（NULL も可）を返す
 *
 * @return 0: HTTP レスポンス取得成功（ステータスコードは out_status）
 *         -1: 接続失敗・パース失敗
 */
static int docker_request(DOCKER_CLI_CONTEXT* cli,
                          const char* method,
                          const char* path,
                          const char* body,
                          char** out_body,
                          size_t* out_body_length,
                          int* out_status)
{
    *out_body = NULL;
    *out_body_length = 0;
    *out_status = 0;

    QS_HTTP_CLIENT_CONTEXT* http = &cli->http;

    if(cli->use_unix){
        if(0 != qs_ssl_module_http_client_connect_unix(http, cli->unix_path, 0)){
            fprintf(stderr, "docker_cli: connect to unix:%s failed\n", cli->unix_path);
            return -1;
        }
    }
    else{
        if(0 != qs_ssl_module_http_client_connect(http, cli->host, cli->port, 0)){
            fprintf(stderr, "docker_cli: connect to %s:%d failed\n", cli->host, cli->port);
            return -1;
        }
    }

    char* request_buffer = qs_ssl_module_http_client_get_request_buffer(http);
    if(request_buffer == NULL){
        fprintf(stderr, "docker_cli: get request buffer failed\n");
        qs_ssl_module_http_client_dispose(http);
        return -1;
    }

    /* Host ヘッダ（Docker Engine は検証しないが、unix 時は localhost を使う） */
    char host_header[DOCKER_CLI_HOST_SIZE + 16];
    if(cli->use_unix){
        snprintf(host_header, sizeof(host_header), "localhost");
    }
    else{
        snprintf(host_header, sizeof(host_header), "%s:%d", cli->host, cli->port);
    }

    if(body != NULL){
        snprintf(request_buffer, QS_HTTP_CLIENT_REQUEST_BUFFER_SIZE,
                 "%s %s HTTP/1.1\r\n"
                 "Host: %s\r\n"
                 "Content-Type: application/json\r\n"
                 "Content-Length: %zu\r\n"
                 "Connection: close\r\n"
                 "\r\n"
                 "%s",
                 method, path, host_header,
                 strlen(body), body);
    }
    else{
        snprintf(request_buffer, QS_HTTP_CLIENT_REQUEST_BUFFER_SIZE,
                 "%s %s HTTP/1.1\r\n"
                 "Host: %s\r\n"
                 "Connection: close\r\n"
                 "\r\n",
                 method, path, host_header);
    }

    while(1){
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
    const char* body_buffer = qs_ssl_module_http_client_get_body_buffer(http);
    if(header_buffer == NULL || body_buffer == NULL){
        fprintf(stderr, "docker_cli: get response buffer failed\n");
        qs_ssl_module_http_client_dispose(http);
        return -1;
    }

    char http_version[16] = {0};
    char status_message[256] = {0};
    int parsed = sscanf(header_buffer, "HTTP/%15s %d %255[^\r\n]",
                        http_version, out_status, status_message);
    if(parsed < 2){
        if(cli->use_unix){
            fprintf(stderr, "docker_cli: no response from unix:%s (connection failed?)\n", cli->unix_path);
        }
        else{
            fprintf(stderr, "docker_cli: no response from %s:%d (connection failed?)\n",
                    cli->host, cli->port);
        }
        qs_ssl_module_http_client_dispose(http);
        return -1;
    }

    if(body_length > 0){
        char* copy = malloc(body_length + 1);
        if(copy != NULL){
            memcpy(copy, body_buffer, body_length);
            copy[body_length] = 0;
            *out_body = copy;
            *out_body_length = body_length;
        }
    }

    qs_ssl_module_http_client_dispose(http);
    return 0;
}

/* JSON 解析前の一時メモリリセット */
static void json_reset(DOCKER_CLI_CONTEXT* cli)
{
    api_qs_memory_clean(&cli->json_memory);
}

/*
 * HTTP エラー（400 以上）を表示する
 * Docker エラー JSON の "message" を取り出せる場合はそれを表示する
 */
static void docker_check_error(DOCKER_CLI_CONTEXT* cli, int status_code, const char* body, size_t body_length)
{
    const char* message = NULL;
    if(body != NULL && body[0] != 0){
        json_reset(cli);
        QS_JSON_ELEMENT_OBJECT object;
        if(0 == api_qs_json_decode_object(&cli->json_memory, &object, body)){
            message = api_qs_object_get_string(&object, "message");
        }
    }
    fprintf(stderr, "docker_cli: error: HTTP %d", status_code);
    if(message != NULL && message[0] != 0){
        fprintf(stderr, ": %s\n", message);
    }
    else{
        fprintf(stderr, "\n");
        if(body != NULL){
            size_t dump = body_length < 1024 ? body_length : 1024;
            fwrite(body, 1, dump, stderr);
            if(dump > 0 && body[dump - 1] != '\n'){
                fputc('\n', stderr);
            }
        }
    }
}

/*
 * Docker のトップレベル配列レスポンスを配列としてデコードする
 * api_qs_json_decode_object はトップレベルがオブジェクト前提のため、
 * ボディを {"items": <body>} でラップしてからデコードする
 */
static int docker_decode_array(DOCKER_CLI_CONTEXT* cli, const char* body, QS_JSON_ELEMENT_ARRAY* out_array)
{
    if(body == NULL || body[0] == 0){
        return -1;
    }
    size_t len = strlen(body);
    char* wrapped = malloc(len + 32);
    if(wrapped == NULL){
        return -1;
    }
    snprintf(wrapped, len + 32, "{\"items\":%s}", body);

    json_reset(cli);
    QS_JSON_ELEMENT_OBJECT object;
    int ret = -1;
    if(0 == api_qs_json_decode_object(&cli->json_memory, &object, wrapped)){
        if(0 == api_qs_object_get_array(&object, "items", out_array)){
            ret = 0;
        }
    }
    free(wrapped);
    return ret;
}

/* 数値フィールドを uint64 として取得する。
 * int32 範囲の値は ID_NUM（int32）として、それ以外は ID_NUM_U64 として格納されるため、
 * 両方の取得関数にフォールバックする。取得できなければ -1 を返す */
static int get_size_u64(QS_JSON_ELEMENT_OBJECT* object, const char* name, uint64_t* out)
{
    uint64_t* v = api_qs_object_get_unsigned_big_integer(object, name);
    if(v != NULL){
        *out = *v;
        return 0;
    }
    int32_t* v32 = api_qs_object_get_integer(object, name);
    if(v32 != NULL){
        *out = (uint64_t)*v32;
        return 0;
    }
    return -1;
}

static void print_human_size(uint64_t bytes, char* buf, size_t buf_size)
{
    if(bytes >= 1024ULL * 1024 * 1024){
        snprintf(buf, buf_size, "%.2f GB", (double)bytes / (1024.0 * 1024 * 1024));
    }
    else if(bytes >= 1024ULL * 1024){
        snprintf(buf, buf_size, "%.2f MB", (double)bytes / (1024.0 * 1024));
    }
    else{
        snprintf(buf, buf_size, "%.2f KB", (double)bytes / 1024.0);
    }
}

/* ------------------------------------------------------------------ */
/* version: GET /version                                              */
/* ------------------------------------------------------------------ */
static int cmd_version(DOCKER_CLI_CONTEXT* cli)
{
    char* body = NULL;
    size_t body_length = 0;
    int status = 0;

    if(0 != docker_request(cli, "GET", "/version", NULL, &body, &body_length, &status)){
        return 1;
    }
    int ret = 0;
    if(200 != status){
        docker_check_error(cli, status, body, body_length);
        ret = 1;
    }
    else{
        json_reset(cli);
        QS_JSON_ELEMENT_OBJECT object;
        if(0 == api_qs_json_decode_object(&cli->json_memory, &object, body)){
            const char* keys[] = {"Version", "ApiVersion", "MinAPIVersion", "GoVersion", "Os", "Arch", "KernelVersion"};
            for(size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++){
                char* value = api_qs_object_get_string(&object, keys[i]);
                if(value != NULL){
                    printf("%-14s %s\n", keys[i], value);
                }
            }
        }
        else{
            fprintf(stderr, "docker_cli: failed to parse /version response\n");
            ret = 1;
        }
    }
    free(body);
    return ret;
}

/* ------------------------------------------------------------------ */
/* info: GET /info                                                    */
/* ------------------------------------------------------------------ */
static int cmd_info(DOCKER_CLI_CONTEXT* cli)
{
    char* body = NULL;
    size_t body_length = 0;
    int status = 0;

    if(0 != docker_request(cli, "GET", "/info", NULL, &body, &body_length, &status)){
        return 1;
    }
    int ret = 0;
    if(200 != status){
        docker_check_error(cli, status, body, body_length);
        ret = 1;
    }
    else{
        json_reset(cli);
        QS_JSON_ELEMENT_OBJECT object;
        if(0 == api_qs_json_decode_object(&cli->json_memory, &object, body)){
            const char* string_keys[] = {"ServerVersion", "DockerRootDir", "Os", "Arch"};
            for(size_t i = 0; i < sizeof(string_keys) / sizeof(string_keys[0]); i++){
                char* value = api_qs_object_get_string(&object, string_keys[i]);
                if(value != NULL){
                    printf("%-16s %s\n", string_keys[i], value);
                }
            }
            int32_t* ncpu = api_qs_object_get_integer(&object, "NCPU");
            if(ncpu != NULL){
                printf("%-16s %d\n", "NCPU", *ncpu);
            }
            /* MemTotal は int32 範囲を越える値のため ID_NUM_U64 として格納される */
            uint64_t* mem_total = api_qs_object_get_unsigned_big_integer(&object, "MemTotal");
            if(mem_total != NULL){
                char size_buf[32];
                print_human_size(*mem_total, size_buf, sizeof(size_buf));
                printf("%-16s %s\n", "MemTotal", size_buf);
            }
            int32_t* containers = api_qs_object_get_integer(&object, "Containers");
            int32_t* running = api_qs_object_get_integer(&object, "ContainersRunning");
            int32_t* paused = api_qs_object_get_integer(&object, "ContainersPaused");
            int32_t* stopped = api_qs_object_get_integer(&object, "ContainersStopped");
            int32_t* images = api_qs_object_get_integer(&object, "Images");
            if(containers != NULL){
                printf("%-16s %d (running: %d, paused: %d, stopped: %d)\n",
                       "Containers", *containers,
                       running != NULL ? *running : 0,
                       paused != NULL ? *paused : 0,
                       stopped != NULL ? *stopped : 0);
            }
            if(images != NULL){
                printf("%-16s %d\n", "Images", *images);
            }
        }
        else{
            fprintf(stderr, "docker_cli: failed to parse /info response\n");
            ret = 1;
        }
    }
    free(body);
    return ret;
}

/* ------------------------------------------------------------------ */
/* ps: GET /containers/json                                           */
/* ------------------------------------------------------------------ */
static int cmd_ps(DOCKER_CLI_CONTEXT* cli, int all)
{
    char* body = NULL;
    size_t body_length = 0;
    int status = 0;

    if(0 != docker_request(cli, "GET", all ? "/containers/json?all=1" : "/containers/json",
                           NULL, &body, &body_length, &status)){
        return 1;
    }
    int ret = 0;
    if(200 != status){
        docker_check_error(cli, status, body, body_length);
        ret = 1;
    }
    else{
        QS_JSON_ELEMENT_ARRAY array;
        if(0 != docker_decode_array(cli, body, &array)){
            fprintf(stderr, "docker_cli: failed to parse /containers/json response\n");
            ret = 1;
        }
        else{
            printf("%-14s %-28s %-24s %s\n", "CONTAINER ID", "IMAGE", "NAME", "STATUS");
            int count = 0;
            int32_t length = api_qs_array_get_length(&array);
            for(int32_t i = 0; i < length; i++){
                QS_JSON_ELEMENT_OBJECT container;
                if(0 != api_qs_array_get_object(&array, i, &container)){
                    continue;
                }
                char* id = api_qs_object_get_string(&container, "Id");
                char* image = api_qs_object_get_string(&container, "Image");
                char* status_str = api_qs_object_get_string(&container, "Status");
                char name_buf[256] = "";
                QS_JSON_ELEMENT_ARRAY names;
                if(0 == api_qs_object_get_array(&container, "Names", &names)){
                    char* first = api_qs_array_get_string(&names, 0);
                    if(first != NULL){
                        snprintf(name_buf, sizeof(name_buf), "%s", first[0] == '/' ? first + 1 : first);
                    }
                }
                printf("%-14s %-28s %-24s %s\n",
                       id != NULL ? id : "",
                       image != NULL ? image : "",
                       name_buf,
                       status_str != NULL ? status_str : "");
                count++;
            }
            if(count == 0){
                printf("(none)\n");
            }
        }
    }
    free(body);
    return ret;
}

/* ------------------------------------------------------------------ */
/* images: GET /images/json                                           */
/* ------------------------------------------------------------------ */
static int cmd_images(DOCKER_CLI_CONTEXT* cli)
{
    char* body = NULL;
    size_t body_length = 0;
    int status = 0;

    if(0 != docker_request(cli, "GET", "/images/json", NULL, &body, &body_length, &status)){
        return 1;
    }
    int ret = 0;
    if(200 != status){
        docker_check_error(cli, status, body, body_length);
        ret = 1;
    }
    else{
        QS_JSON_ELEMENT_ARRAY array;
        if(0 != docker_decode_array(cli, body, &array)){
            fprintf(stderr, "docker_cli: failed to parse /images/json response\n");
            ret = 1;
        }
        else{
            printf("%-14s %-32s %s\n", "IMAGE ID", "TAGS", "SIZE");
            int count = 0;
            int32_t length = api_qs_array_get_length(&array);
            for(int32_t i = 0; i < length; i++){
                QS_JSON_ELEMENT_OBJECT image;
                if(0 != api_qs_array_get_object(&array, i, &image)){
                    continue;
                }
                char* id = api_qs_object_get_string(&image, "Id");
                char id_buf[128] = "";
                if(id != NULL){
                    /* sha256:... 形式の場合は先頭 12 文字を抽出する */
                    const char* sha = strstr(id, ":");
                    snprintf(id_buf, sizeof(id_buf), "%s", sha != NULL ? sha + 1 : id);
                    if(strlen(id_buf) > 12){
                        id_buf[12] = 0;
                    }
                }
                char tags_buf[512] = "";
                QS_JSON_ELEMENT_ARRAY repo_tags;
                if(0 == api_qs_object_get_array(&image, "RepoTags", &repo_tags)){
                    int32_t tag_count = api_qs_array_get_length(&repo_tags);
                    for(int32_t j = 0; j < tag_count; j++){
                        char* tag = api_qs_array_get_string(&repo_tags, j);
                        if(tag == NULL){
                            continue;
                        }
                        if(strlen(tags_buf) > 0){
                            strncat(tags_buf, ", ", sizeof(tags_buf) - strlen(tags_buf) - 1);
                        }
                        strncat(tags_buf, tag, sizeof(tags_buf) - strlen(tags_buf) - 1);
                    }
                }
                if(strlen(tags_buf) == 0){
                    snprintf(tags_buf, sizeof(tags_buf), "<none>");
                }
                /* Size は int32 範囲の値は ID_NUM、それ以外は ID_NUM_U64 として格納される */
                char size_buf[32] = "";
                uint64_t* size = api_qs_object_get_unsigned_big_integer(&image, "Size");
                if(size != NULL){
                    print_human_size(*size, size_buf, sizeof(size_buf));
                }
                else{
                    int32_t* size32 = api_qs_object_get_integer(&image, "Size");
                    if(size32 != NULL && *size32 >= 0){
                        print_human_size((uint64_t)*size32, size_buf, sizeof(size_buf));
                    }
                }
                printf("%-14s %-32s %s\n", id_buf, tags_buf, size_buf);
                count++;
            }
            if(count == 0){
                printf("(none)\n");
            }
        }
    }
    free(body);
    return ret;
}

/* ------------------------------------------------------------------ */
/* run: POST /containers/create + POST /containers/{id}/start        */
/* ------------------------------------------------------------------ */
static int cmd_run(DOCKER_CLI_CONTEXT* cli, int argc, char** argv)
{
    if(argc < 1){
        fprintf(stderr, "docker_cli: run requires an image name\n");
        return 1;
    }
    const char* image = argv[0];
    if(!is_safe_path_token(image)){
        fprintf(stderr, "docker_cli: invalid image name: %s\n", image);
        return 1;
    }

    /* create リクエストボディを JSON で生成 */
    json_reset(cli);
    QS_JSON_ELEMENT_OBJECT object;
    if(0 != api_qs_object_create(&cli->json_memory, &object)){
        fprintf(stderr, "docker_cli: memory alloc failed\n");
        return 1;
    }
    api_qs_object_push_string(&object, "Image", image);
    if(argc > 1){
        QS_JSON_ELEMENT_ARRAY cmd_array;
        api_qs_array_create(&cli->json_memory, &cmd_array);
        for(int i = 1; i < argc; i++){
            api_qs_array_push_string(&cmd_array, argv[i]);
        }
        api_qs_object_push_array(&object, "Cmd", &cmd_array);
    }
    char* create_body = api_qs_json_encode_object(&object, DOCKER_CLI_CREATE_BODY_SIZE);
    if(create_body == NULL){
        fprintf(stderr, "docker_cli: failed to encode create request body\n");
        return 1;
    }

    /* コンテナ作成 */
    char* body = NULL;
    size_t body_length = 0;
    int status = 0;
    if(0 != docker_request(cli, "POST", "/containers/create", create_body, &body, &body_length, &status)){
        return 1;
    }
    int ret = 1;
    if(201 != status){
        docker_check_error(cli, status, body, body_length);
    }
    else{
        json_reset(cli);
        QS_JSON_ELEMENT_OBJECT create_result;
        char id_buf[DOCKER_CLI_ID_SIZE] = "";
        if(0 == api_qs_json_decode_object(&cli->json_memory, &create_result, body)){
            char* id = api_qs_object_get_string(&create_result, "Id");
            if(id != NULL && is_safe_path_token(id)){
                snprintf(id_buf, sizeof(id_buf), "%s", id);
            }
        }
        if(id_buf[0] == 0){
            fprintf(stderr, "docker_cli: failed to parse container id from create response\n");
        }
        else{
            /* コンテナ起動 */
            char start_path[DOCKER_CLI_ID_SIZE + 64];
            snprintf(start_path, sizeof(start_path), "/containers/%s/start", id_buf);
            char* start_body = NULL;
            size_t start_body_length = 0;
            int start_status = 0;
            if(0 != docker_request(cli, "POST", start_path, NULL, &start_body, &start_body_length, &start_status)){
                free(start_body);
                ret = 1;
            }
            else if(start_status == 204 || start_status == 304){
                printf("%s\n", id_buf);
                ret = 0;
            }
            else{
                docker_check_error(cli, start_status, start_body, start_body_length);
            }
            free(start_body);
        }
    }
    free(body);
    return ret;
}

/* ------------------------------------------------------------------ */
/* start / stop: POST /containers/{id}/start | stop                  */
/* ------------------------------------------------------------------ */
static int cmd_container_action(DOCKER_CLI_CONTEXT* cli, const char* action, const char* id)
{
    if(id == NULL || !is_safe_path_token(id)){
        fprintf(stderr, "docker_cli: %s requires a valid container id/name\n", action);
        return 1;
    }

    char path[DOCKER_CLI_ID_SIZE + 64];
    snprintf(path, sizeof(path), "/containers/%s/%s", id, action);

    char* body = NULL;
    size_t body_length = 0;
    int status = 0;
    if(0 != docker_request(cli, "POST", path, NULL, &body, &body_length, &status)){
        return 1;
    }
    int ret = 0;
    if(status == 204 || status == 304){
        printf("%s: %s\n", action, id);
    }
    else{
        docker_check_error(cli, status, body, body_length);
        ret = 1;
    }
    free(body);
    return ret;
}

/* ------------------------------------------------------------------ */
/* kill: POST /containers/{id}/kill?signal=NAME                       */
/* ------------------------------------------------------------------ */
static int is_safe_signal_token(const char* signal)
{
    if(signal == NULL || *signal == 0){
        return 0;
    }
    size_t len = strlen(signal);
    if(len > 16){
        return 0;
    }
    for(size_t i = 0; i < len; i++){
        char c = signal[i];
        if(!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))){
            return 0;
        }
    }
    return 1;
}

static int cmd_kill(DOCKER_CLI_CONTEXT* cli, const char* id, const char* signal)
{
    if(id == NULL || !is_safe_path_token(id)){
        fprintf(stderr, "docker_cli: kill requires a valid container id/name\n");
        return 1;
    }
    if(!is_safe_signal_token(signal)){
        fprintf(stderr, "docker_cli: invalid signal: %s\n", signal);
        return 1;
    }

    char path[DOCKER_CLI_ID_SIZE + 96];
    snprintf(path, sizeof(path), "/containers/%s/kill?signal=%s", id, signal);

    char* body = NULL;
    size_t body_length = 0;
    int status = 0;
    if(0 != docker_request(cli, "POST", path, NULL, &body, &body_length, &status)){
        return 1;
    }
    int ret = 0;
    if(status == 204 || status == 304){
        printf("killed: %s (signal: %s)\n", id, signal);
    }
    else{
        docker_check_error(cli, status, body, body_length);
        ret = 1;
    }
    free(body);
    return ret;
}

/* ------------------------------------------------------------------ */
/* rm: DELETE /containers/{id}                                       */
/* ------------------------------------------------------------------ */
static int cmd_rm(DOCKER_CLI_CONTEXT* cli, const char* id, int force)
{
    if(id == NULL || !is_safe_path_token(id)){
        fprintf(stderr, "docker_cli: rm requires a valid container id/name\n");
        return 1;
    }

    char path[DOCKER_CLI_ID_SIZE + 64];
    snprintf(path, sizeof(path), "/containers/%s%s", id, force ? "?force=1" : "");

    char* body = NULL;
    size_t body_length = 0;
    int status = 0;
    if(0 != docker_request(cli, "DELETE", path, NULL, &body, &body_length, &status)){
        return 1;
    }
    int ret = 0;
    if(204 == status){
        printf("removed: %s\n", id);
    }
    else{
        docker_check_error(cli, status, body, body_length);
        ret = 1;
    }
    free(body);
    return ret;
}

/* ------------------------------------------------------------------ */
/* rmi: DELETE /images/{name}                                         */
/* ------------------------------------------------------------------ */
static int cmd_rmi(DOCKER_CLI_CONTEXT* cli, const char* name, int force)
{
    if(name == NULL || !is_safe_path_token(name)){
        fprintf(stderr, "docker_cli: rmi requires a valid image name\n");
        return 1;
    }

    char path[DOCKER_CLI_HOST_SIZE + 32];
    snprintf(path, sizeof(path), "/images/%s%s", name, force ? "?force=1" : "");

    char* body = NULL;
    size_t body_length = 0;
    int status = 0;
    if(0 != docker_request(cli, "DELETE", path, NULL, &body, &body_length, &status)){
        return 1;
    }
    int ret = 0;
    if(200 == status){
        QS_JSON_ELEMENT_ARRAY array;
        if(0 != docker_decode_array(cli, body, &array)){
            fprintf(stderr, "docker_cli: failed to parse image delete response\n");
            ret = 1;
        }
        else{
            int32_t length = api_qs_array_get_length(&array);
            for(int32_t i = 0; i < length; i++){
                QS_JSON_ELEMENT_OBJECT entry;
                if(0 != api_qs_array_get_object(&array, i, &entry)){
                    continue;
                }
                char* untagged = api_qs_object_get_string(&entry, "Untagged");
                if(untagged != NULL){
                    printf("untagged: %s\n", untagged);
                }
                char* deleted = api_qs_object_get_string(&entry, "Deleted");
                if(deleted != NULL){
                    printf("deleted: %s\n", deleted);
                }
            }
        }
    }
    else{
        docker_check_error(cli, status, body, body_length);
        ret = 1;
    }
    free(body);
    return ret;
}

/* ------------------------------------------------------------------ */
/* tag: POST /images/{name}/tag?repo=REPO&tag=TAG                     */
/* ------------------------------------------------------------------ */
static int cmd_tag(DOCKER_CLI_CONTEXT* cli, const char* source, const char* target)
{
    if(source == NULL || !is_safe_path_token(source)){
        fprintf(stderr, "docker_cli: tag requires a valid source image\n");
        return 1;
    }
    if(target == NULL || !is_safe_path_token(target)){
        fprintf(stderr, "docker_cli: tag requires a valid target image (repo or repo:tag)\n");
        return 1;
    }

    /* ターゲットを最後の '/' より後の ':' を境界に repo / tag に分割 */
    char repo[DOCKER_CLI_HOST_SIZE];
    char tag_name[128];
    const char* colon = NULL;
    const char* last_slash = strrchr(target, '/');
    for(const char* p = target; *p != 0; p++){
        if(*p == ':' && (last_slash == NULL || p > last_slash)){
            colon = p;
        }
    }
    if(colon != NULL){
        size_t repo_len = (size_t)(colon - target);
        if(repo_len >= sizeof(repo)){
            fprintf(stderr, "docker_cli: target repo is too long\n");
            return 1;
        }
        memcpy(repo, target, repo_len);
        repo[repo_len] = 0;
        snprintf(tag_name, sizeof(tag_name), "%s", colon + 1);
    }
    else{
        snprintf(repo, sizeof(repo), "%s", target);
        tag_name[0] = 0;
    }

    char path[DOCKER_CLI_HOST_SIZE * 2 + 128];
    if(tag_name[0] != 0){
        snprintf(path, sizeof(path), "/images/%s/tag?repo=%s&tag=%s", source, repo, tag_name);
    }
    else{
        snprintf(path, sizeof(path), "/images/%s/tag?repo=%s", source, repo);
    }

    char* body = NULL;
    size_t body_length = 0;
    int status = 0;
    if(0 != docker_request(cli, "POST", path, NULL, &body, &body_length, &status)){
        return 1;
    }
    int ret = 0;
    if(status == 201 || status == 204){
        printf("tagged: %s as %s\n", source, target);
    }
    else{
        docker_check_error(cli, status, body, body_length);
        ret = 1;
    }
    free(body);
    return ret;
}

/* ------------------------------------------------------------------ */
/* history: GET /images/{name}/history                                */
/* ------------------------------------------------------------------ */
static int cmd_history(DOCKER_CLI_CONTEXT* cli, const char* name)
{
    if(name == NULL || !is_safe_path_token(name)){
        fprintf(stderr, "docker_cli: history requires a valid image name\n");
        return 1;
    }

    char path[DOCKER_CLI_HOST_SIZE + 32];
    snprintf(path, sizeof(path), "/images/%s/history", name);

    char* body = NULL;
    size_t body_length = 0;
    int status = 0;
    if(0 != docker_request(cli, "GET", path, NULL, &body, &body_length, &status)){
        return 1;
    }
    int ret = 0;
    if(200 != status){
        docker_check_error(cli, status, body, body_length);
        ret = 1;
    }
    else{
        QS_JSON_ELEMENT_ARRAY array;
        if(0 != docker_decode_array(cli, body, &array)){
            fprintf(stderr, "docker_cli: failed to parse image history response\n");
            ret = 1;
        }
        else{
            printf("%-14s %12s %s\n", "IMAGE ID", "SIZE", "CREATED BY");
            int32_t length = api_qs_array_get_length(&array);
            for(int32_t i = 0; i < length; i++){
                QS_JSON_ELEMENT_OBJECT layer;
                if(0 != api_qs_array_get_object(&array, i, &layer)){
                    continue;
                }
                char* id = api_qs_object_get_string(&layer, "Id");
                char id_buf[128] = "";
                if(id != NULL){
                    const char* sha = strstr(id, ":");
                    snprintf(id_buf, sizeof(id_buf), "%s", sha != NULL ? sha + 1 : id);
                    if(strlen(id_buf) > 12){
                        id_buf[12] = 0;
                    }
                }
                char size_buf[32] = "";
                uint64_t* size64 = api_qs_object_get_unsigned_big_integer(&layer, "Size");
                if(size64 != NULL){
                    print_human_size(*size64, size_buf, sizeof(size_buf));
                }
                else{
                    int32_t* size32 = api_qs_object_get_integer(&layer, "Size");
                    if(size32 != NULL){
                        if(*size32 >= 0){
                            print_human_size((uint64_t)*size32, size_buf, sizeof(size_buf));
                        }
                        else{
                            snprintf(size_buf, sizeof(size_buf), "%d", *size32);
                        }
                    }
                }
                char* created_by = api_qs_object_get_string(&layer, "CreatedBy");
                char created_buf[128] = "";
                if(created_by != NULL){
                    snprintf(created_buf, sizeof(created_buf), "%.118s", created_by);
                }
                printf("%-14s %12s %s\n", id_buf, size_buf, created_buf);
            }
        }
    }
    free(body);
    return ret;
}

/* ------------------------------------------------------------------ */
/* logs: GET /containers/{id}/logs?stdout=1&stderr=1                 */
/* Docker raw stream（8 バイトヘッダ + データのフレーム列）をデミュークスして表示        */
/* ------------------------------------------------------------------ */
static void print_demuxed(const char* body, size_t length)
{
    if(body == NULL || length < 8){
        if(body != NULL){
            fwrite(body, 1, length, stdout);
        }
        return;
    }

    /* 最初のフレームがバッファ内に収まらない場合はデミュークスされていないとみなす */
    uint32_t first_len = ((uint32_t)(uint8_t)body[4] << 24) |
                         ((uint32_t)(uint8_t)body[5] << 16) |
                         ((uint32_t)(uint8_t)body[6] << 8) |
                         ((uint32_t)(uint8_t)body[7]);
    if(8 + first_len > length){
        fwrite(body, 1, length, stdout);
        return;
    }

    size_t offset = 0;
    while(offset + 8 <= length){
        uint8_t stream = (uint8_t)body[offset];
        uint32_t frame_len = ((uint32_t)(uint8_t)body[offset + 4] << 24) |
                             ((uint32_t)(uint8_t)body[offset + 5] << 16) |
                             ((uint32_t)(uint8_t)body[offset + 6] << 8) |
                             ((uint32_t)(uint8_t)body[offset + 7]);
        if(offset + 8 + frame_len > length){
            break;
        }
        if(1 == stream){
            fwrite(body + offset + 8, 1, frame_len, stdout);
        }
        else if(2 == stream){
            fwrite(body + offset + 8, 1, frame_len, stderr);
        }
        offset += 8 + frame_len;
    }
}

static int cmd_logs(DOCKER_CLI_CONTEXT* cli, const char* id)
{
    if(id == NULL || !is_safe_path_token(id)){
        fprintf(stderr, "docker_cli: logs requires a valid container id/name\n");
        return 1;
    }

    char path[DOCKER_CLI_ID_SIZE + 64];
    snprintf(path, sizeof(path), "/containers/%s/logs?stdout=1&stderr=1", id);

    char* body = NULL;
    size_t body_length = 0;
    int status = 0;
    if(0 != docker_request(cli, "GET", path, NULL, &body, &body_length, &status)){
        return 1;
    }
    int ret = 0;
    if(200 != status){
        docker_check_error(cli, status, body, body_length);
        ret = 1;
    }
    else{
        print_demuxed(body, body_length);
    }
    free(body);
    return ret;
}

/* ------------------------------------------------------------------ */
/* top: GET /containers/{id}/top                                      */
/* ------------------------------------------------------------------ */
static void print_string_array_line(QS_JSON_ELEMENT_ARRAY* array)
{
    int32_t length = api_qs_array_get_length(array);
    for(int32_t i = 0; i < length; i++){
        char* s = api_qs_array_get_string(array, i);
        if(i > 0){
            putchar(' ');
        }
        if(s != NULL){
            fputs(s, stdout);
        }
    }
    putchar('\n');
}

static int cmd_top(DOCKER_CLI_CONTEXT* cli, const char* id)
{
    if(id == NULL || !is_safe_path_token(id)){
        fprintf(stderr, "docker_cli: top requires a valid container id/name\n");
        return 1;
    }

    char path[DOCKER_CLI_ID_SIZE + 64];
    snprintf(path, sizeof(path), "/containers/%s/top", id);

    char* body = NULL;
    size_t body_length = 0;
    int status = 0;
    if(0 != docker_request(cli, "GET", path, NULL, &body, &body_length, &status)){
        return 1;
    }
    int ret = 0;
    if(200 != status){
        docker_check_error(cli, status, body, body_length);
        ret = 1;
    }
    else{
        json_reset(cli);
        QS_JSON_ELEMENT_OBJECT object;
        if(0 != api_qs_json_decode_object(&cli->json_memory, &object, body)){
            fprintf(stderr, "docker_cli: failed to parse /top response\n");
            ret = 1;
        }
        else{
            QS_JSON_ELEMENT_ARRAY titles;
            QS_JSON_ELEMENT_ARRAY processes;
            if(0 == api_qs_object_get_array(&object, "Titles", &titles)){
                print_string_array_line(&titles);
            }
            if(0 == api_qs_object_get_array(&object, "Processes", &processes)){
                int32_t length = api_qs_array_get_length(&processes);
                for(int32_t i = 0; i < length; i++){
                    QS_JSON_ELEMENT_ARRAY process;
                    if(0 != api_qs_array_get_array(&processes, i, &process)){
                        continue;
                    }
                    print_string_array_line(&process);
                }
            }
        }
    }
    free(body);
    return ret;
}

/* ------------------------------------------------------------------ */
/* port: GET /containers/{id}/json -> NetworkSettings.Ports            */
/* （Engine API に専用の ports エンドポイントはないため inspect を使う） */
/* ------------------------------------------------------------------ */
static int cmd_port(DOCKER_CLI_CONTEXT* cli, const char* id)
{
    if(id == NULL || !is_safe_path_token(id)){
        fprintf(stderr, "docker_cli: port requires a valid container id/name\n");
        return 1;
    }

    char path[DOCKER_CLI_ID_SIZE + 64];
    snprintf(path, sizeof(path), "/containers/%s/json", id);

    char* body = NULL;
    size_t body_length = 0;
    int status = 0;
    if(0 != docker_request(cli, "GET", path, NULL, &body, &body_length, &status)){
        return 1;
    }
    int ret = 0;
    if(200 != status){
        docker_check_error(cli, status, body, body_length);
        ret = 1;
    }
    else{
        json_reset(cli);
        QS_JSON_ELEMENT_OBJECT object;
        if(0 != api_qs_json_decode_object(&cli->json_memory, &object, body)){
            fprintf(stderr, "docker_cli: failed to parse container inspect response\n");
            ret = 1;
        }
        else{
            int count = 0;
            QS_JSON_ELEMENT_OBJECT network_settings;
            QS_JSON_ELEMENT_OBJECT ports;
            if(0 == api_qs_object_get_object(&object, "NetworkSettings", &network_settings) &&
               0 == api_qs_object_get_object(&network_settings, "Ports", &ports)){
                QS_JSON_ELEMENT_ARRAY keys;
                if(0 == api_qs_object_get_keys(&ports, &keys)){
                    int32_t key_count = api_qs_array_get_length(&keys);
                    for(int32_t i = 0; i < key_count; i++){
                        char* key = api_qs_array_get_string(&keys, i);
                        QS_JSON_ELEMENT_ARRAY bindings;
                        if(key == NULL || 0 != api_qs_object_get_array(&ports, key, &bindings)){
                            continue;
                        }
                        int32_t bind_count = api_qs_array_get_length(&bindings);
                        for(int32_t j = 0; j < bind_count; j++){
                            QS_JSON_ELEMENT_OBJECT binding;
                            if(0 != api_qs_array_get_object(&bindings, j, &binding)){
                                continue;
                            }
                            char* host_ip = api_qs_object_get_string(&binding, "HostIp");
                            char* host_port = api_qs_object_get_string(&binding, "HostPort");
                            printf("%s -> %s:%s\n", key,
                                   host_ip != NULL ? host_ip : "?",
                                   host_port != NULL ? host_port : "?");
                            count++;
                        }
                    }
                }
            }
            if(count == 0){
                printf("(none)\n");
            }
        }
    }
    free(body);
    return ret;
}

/* ------------------------------------------------------------------ */
/* wait: POST /containers/{id}/wait                                   */
/* ------------------------------------------------------------------ */
static int cmd_wait(DOCKER_CLI_CONTEXT* cli, const char* id)
{
    if(id == NULL || !is_safe_path_token(id)){
        fprintf(stderr, "docker_cli: wait requires a valid container id/name\n");
        return 1;
    }

    char path[DOCKER_CLI_ID_SIZE + 64];
    snprintf(path, sizeof(path), "/containers/%s/wait", id);

    char* body = NULL;
    size_t body_length = 0;
    int status = 0;
    if(0 != docker_request(cli, "POST", path, NULL, &body, &body_length, &status)){
        return 1;
    }
    int ret = 0;
    if(200 != status){
        docker_check_error(cli, status, body, body_length);
        ret = 1;
    }
    else{
        json_reset(cli);
        QS_JSON_ELEMENT_OBJECT object;
        int32_t* code = NULL;
        if(0 == api_qs_json_decode_object(&cli->json_memory, &object, body)){
            code = api_qs_object_get_integer(&object, "StatusCode");
        }
        if(code != NULL){
            printf("exit code: %d\n", *code);
        }
        else{
            fprintf(stderr, "docker_cli: failed to parse /wait response\n");
            ret = 1;
        }
    }
    free(body);
    return ret;
}

/* ------------------------------------------------------------------ */
/* stats: GET /containers/{id}/stats?stream=false                     */
/* ------------------------------------------------------------------ */
static int cmd_stats(DOCKER_CLI_CONTEXT* cli, const char* id)
{
    if(id == NULL || !is_safe_path_token(id)){
        fprintf(stderr, "docker_cli: stats requires a valid container id/name\n");
        return 1;
    }

    char path[DOCKER_CLI_ID_SIZE + 64];
    snprintf(path, sizeof(path), "/containers/%s/stats?stream=false", id);

    char* body = NULL;
    size_t body_length = 0;
    int status = 0;
    if(0 != docker_request(cli, "GET", path, NULL, &body, &body_length, &status)){
        return 1;
    }
    int ret = 0;
    if(200 != status){
        docker_check_error(cli, status, body, body_length);
        ret = 1;
    }
    else{
        json_reset(cli);
        QS_JSON_ELEMENT_OBJECT object;
        if(0 != api_qs_json_decode_object(&cli->json_memory, &object, body)){
            fprintf(stderr, "docker_cli: failed to parse /stats response\n");
            ret = 1;
        }
        else{
            int printed = 0;

            QS_JSON_ELEMENT_OBJECT memory_stats;
            if(0 == api_qs_object_get_object(&object, "memory_stats", &memory_stats)){
                uint64_t usage = 0;
                uint64_t limit = 0;
                char usage_buf[32] = "";
                char limit_buf[32] = "";
                if(0 == get_size_u64(&memory_stats, "usage", &usage)){
                    print_human_size(usage, usage_buf, sizeof(usage_buf));
                }
                if(0 == get_size_u64(&memory_stats, "limit", &limit) && limit > 0){
                    print_human_size(limit, limit_buf, sizeof(limit_buf));
                }
                printf("Memory:     %s / %s\n", usage_buf, limit_buf);
                printed = 1;
            }

            QS_JSON_ELEMENT_OBJECT cpu_stats;
            if(0 == api_qs_object_get_object(&object, "cpu_stats", &cpu_stats)){
                QS_JSON_ELEMENT_OBJECT cpu_usage;
                if(0 == api_qs_object_get_object(&cpu_stats, "cpu_usage", &cpu_usage)){
                    uint64_t total = 0;
                    if(0 == get_size_u64(&cpu_usage, "total_usage", &total)){
                        printf("CPU total:  %.3f s\n", (double)total / 1e9);
                        printed = 1;
                    }
                }
            }

            QS_JSON_ELEMENT_OBJECT networks;
            if(0 == api_qs_object_get_object(&object, "networks", &networks)){
                QS_JSON_ELEMENT_ARRAY keys;
                if(0 == api_qs_object_get_keys(&networks, &keys)){
                    uint64_t rx = 0;
                    uint64_t tx = 0;
                    int have_rx = 0;
                    int have_tx = 0;
                    int32_t key_count = api_qs_array_get_length(&keys);
                    for(int32_t i = 0; i < key_count; i++){
                        char* key = api_qs_array_get_string(&keys, i);
                        QS_JSON_ELEMENT_OBJECT net;
                        if(key == NULL || 0 != api_qs_object_get_object(&networks, key, &net)){
                            continue;
                        }
                        uint64_t net_v = 0;
                        if(0 == get_size_u64(&net, "rx_bytes", &net_v)){
                            rx += net_v;
                            have_rx = 1;
                        }
                        if(0 == get_size_u64(&net, "tx_bytes", &net_v)){
                            tx += net_v;
                            have_tx = 1;
                        }
                    }
                    char rx_buf[32] = "";
                    char tx_buf[32] = "";
                    if(have_rx){
                        print_human_size(rx, rx_buf, sizeof(rx_buf));
                        printf("Net RX:     %s\n", rx_buf);
                        printed = 1;
                    }
                    if(have_tx){
                        print_human_size(tx, tx_buf, sizeof(tx_buf));
                        printf("Net TX:     %s\n", tx_buf);
                        printed = 1;
                    }
                }
            }

            if(!printed){
                fprintf(stderr, "docker_cli: failed to parse /stats response\n");
                ret = 1;
            }
        }
    }
    free(body);
    return ret;
}

/* ------------------------------------------------------------------ */
/* network ls: GET /networks                                          */
/* ------------------------------------------------------------------ */
static int cmd_network_ls(DOCKER_CLI_CONTEXT* cli)
{
    char* body = NULL;
    size_t body_length = 0;
    int status = 0;

    if(0 != docker_request(cli, "GET", "/networks", NULL, &body, &body_length, &status)){
        return 1;
    }
    int ret = 0;
    if(200 != status){
        docker_check_error(cli, status, body, body_length);
        ret = 1;
    }
    else{
        QS_JSON_ELEMENT_ARRAY array;
        if(0 != docker_decode_array(cli, body, &array)){
            fprintf(stderr, "docker_cli: failed to parse /networks response\n");
            ret = 1;
        }
        else{
            printf("%-24s %-10s %-10s %s\n", "NAME", "DRIVER", "SCOPE", "NETWORK ID");
            int count = 0;
            int32_t length = api_qs_array_get_length(&array);
            for(int32_t i = 0; i < length; i++){
                QS_JSON_ELEMENT_OBJECT network;
                if(0 != api_qs_array_get_object(&array, i, &network)){
                    continue;
                }
                char* name = api_qs_object_get_string(&network, "Name");
                char* driver = api_qs_object_get_string(&network, "Driver");
                char* scope = api_qs_object_get_string(&network, "Scope");
                char* id = api_qs_object_get_string(&network, "Id");
                char id_buf[128] = "";
                if(id != NULL && strlen(id) > 12){
                    snprintf(id_buf, sizeof(id_buf), "%.12s", id);
                }
                printf("%-24s %-10s %-10s %s\n",
                       name != NULL ? name : "",
                       driver != NULL ? driver : "",
                       scope != NULL ? scope : "",
                       id_buf);
                count++;
            }
            if(count == 0){
                printf("(none)\n");
            }
        }
    }
    free(body);
    return ret;
}

/* ------------------------------------------------------------------ */
/* volume ls: GET /volumes                                            */
/* ------------------------------------------------------------------ */
static int cmd_volume_ls(DOCKER_CLI_CONTEXT* cli)
{
    char* body = NULL;
    size_t body_length = 0;
    int status = 0;

    if(0 != docker_request(cli, "GET", "/volumes", NULL, &body, &body_length, &status)){
        return 1;
    }
    int ret = 0;
    if(200 != status){
        docker_check_error(cli, status, body, body_length);
        ret = 1;
    }
    else{
        json_reset(cli);
        QS_JSON_ELEMENT_OBJECT object;
        if(0 != api_qs_json_decode_object(&cli->json_memory, &object, body)){
            fprintf(stderr, "docker_cli: failed to parse /volumes response\n");
            ret = 1;
        }
        else{
            QS_JSON_ELEMENT_ARRAY volumes;
            printf("%-32s %s\n", "NAME", "DRIVER");
            int count = 0;
            if(0 == api_qs_object_get_array(&object, "Volumes", &volumes)){
                int32_t length = api_qs_array_get_length(&volumes);
                for(int32_t i = 0; i < length; i++){
                    QS_JSON_ELEMENT_OBJECT volume;
                    if(0 != api_qs_array_get_object(&volumes, i, &volume)){
                        continue;
                    }
                    char* name = api_qs_object_get_string(&volume, "Name");
                    char* driver = api_qs_object_get_string(&volume, "Driver");
                    printf("%-32s %s\n",
                           name != NULL ? name : "",
                           driver != NULL ? driver : "local");
                    count++;
                }
            }
            if(count == 0){
                printf("(none)\n");
            }
        }
    }
    free(body);
    return ret;
}

/* ------------------------------------------------------------------ */
/* prune: POST /images|containers|volumes/prune [--filter key=value]  */
/* ------------------------------------------------------------------ */
static const char* url_encode_str(const char* src, char* dst, size_t dst_size)
{
    size_t o = 0;
    for(const unsigned char* p = (const unsigned char*)src; *p != 0 && o + 3 < dst_size; p++){
        char c = (char)*p;
        if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
           c == '-' || c == '_' || c == '.' || c == '~'){
            dst[o++] = c;
        }
        else{
            o += (size_t)snprintf(dst + o, dst_size - o, "%%%02X", (unsigned int)*p);
        }
    }
    dst[o] = 0;
    return dst;
}

static int cmd_prune(DOCKER_CLI_CONTEXT* cli, const char* kind, const char* filter)
{
    const char* prune_path;
    const char* deleted_key;
    if(kind != NULL && 0 == strcmp(kind, "images")){
        prune_path = "/images/prune";
        deleted_key = "ImagesDeleted";
    }
    else if(kind != NULL && 0 == strcmp(kind, "containers")){
        prune_path = "/containers/prune";
        deleted_key = "ContainersDeleted";
    }
    else if(kind != NULL && 0 == strcmp(kind, "volumes")){
        prune_path = "/volumes/prune";
        deleted_key = "VolumesDeleted";
    }
    else{
        fprintf(stderr, "docker_cli: prune kind must be one of: images, containers, volumes\n");
        return 1;
    }

    char path[2048];
    if(filter == NULL){
        snprintf(path, sizeof(path), "%s", prune_path);
    }
    else{
        /* フィルタ値が JSON を壊す文字を含んでいれば拒否 */
        if(strpbrk(filter, "\"\\{}[]") != NULL){
            fprintf(stderr, "docker_cli: invalid --filter value\n");
            return 1;
        }
        if(strlen(filter) > 200){
            fprintf(stderr, "docker_cli: --filter value is too long\n");
            return 1;
        }
        /* Engine API の filters パラメータ: {"key":["value"]} のリスト形式。
         * label は label=KEY=VALUE として値全体を渡す（2 段形式は 400 になる）。
         * 例: --filter label=qs_test=1 -> {"label":["qs_test=1"]} */
        char filter_json[512];
        const char* eq = strchr(filter, '=');
        if(eq == NULL || eq == filter){
            fprintf(stderr, "docker_cli: --filter expects key=value\n");
            return 1;
        }
        snprintf(filter_json, sizeof(filter_json),
                 "{\"%.*s\":[\"%s\"]}", (int)(eq - filter), filter, eq + 1);
        char encoded[1024];
        url_encode_str(filter_json, encoded, sizeof(encoded));
        snprintf(path, sizeof(path), "%s?filters=%s", prune_path, encoded);
    }

    char* body = NULL;
    size_t body_length = 0;
    int status = 0;
    if(0 != docker_request(cli, "POST", path, NULL, &body, &body_length, &status)){
        return 1;
    }
    int ret = 0;
    if(200 != status){
        docker_check_error(cli, status, body, body_length);
        ret = 1;
    }
    else{
        json_reset(cli);
        QS_JSON_ELEMENT_OBJECT object;
        if(0 != api_qs_json_decode_object(&cli->json_memory, &object, body)){
            fprintf(stderr, "docker_cli: failed to parse /prune response\n");
            ret = 1;
        }
        else{
            QS_JSON_ELEMENT_ARRAY deleted;
            int count = 0;
            if(0 == api_qs_object_get_array(&object, deleted_key, &deleted)){
                count = api_qs_array_get_length(&deleted);
            }
            uint64_t reclaimed = 0;
            if(0 == get_size_u64(&object, "SpaceReclaimed", &reclaimed)){
                char size_buf[32];
                print_human_size(reclaimed, size_buf, sizeof(size_buf));
                printf("Deleted: %d item(s)\n", count);
                printf("Reclaimed: %s\n", size_buf);
            }
            else{
                printf("Deleted: %d item(s)\n", count);
            }
        }
    }
    free(body);
    return ret;
}

/* ------------------------------------------------------------------ */
/* main                                                              */
/* ------------------------------------------------------------------ */
int main(int argc, char** argv)
{
    DOCKER_CLI_CONTEXT cli;
    memset(&cli, 0, sizeof(cli));
    snprintf(cli.host, sizeof(cli.host), "127.0.0.1");
    cli.port = 2375;

    /* コマンド引数のパース（コマンド名より前にある --host / --port / --unix-socket のみ） */
    int has_host_option = 0;
    int i = 1;
    for(; i < argc; i++){
        if(0 == strcmp(argv[i], "--host") && i + 1 < argc){
            snprintf(cli.host, sizeof(cli.host), "%s", argv[++i]);
            has_host_option = 1;
        }
        else if(0 == strcmp(argv[i], "--port") && i + 1 < argc){
            cli.port = atoi(argv[++i]);
            has_host_option = 1;
        }
        else if(0 == strcmp(argv[i], "--unix-socket") && i + 1 < argc){
            if(strlen(argv[i + 1]) >= sizeof(cli.unix_path)){
                fprintf(stderr, "docker_cli: unix socket path is too long\n");
                return 1;
            }
            snprintf(cli.unix_path, sizeof(cli.unix_path), "%s", argv[++i]);
            cli.use_unix = 1;
        }
        else if(0 == strcmp(argv[i], "-h") || 0 == strcmp(argv[i], "--help")){
            print_usage();
            return 0;
        }
        else{
            break;
        }
    }
    if(i >= argc){
        print_usage();
        return 1;
    }

    /* エンドポイント解決: --unix-socket > --host/--port (TCP) > 既定（/var/run/docker.sock があれば Unix、なければ TCP） */
    if(!cli.use_unix && !has_host_option){
        if(0 == access(DOCKER_CLI_DEFAULT_SOCKET, F_OK)){
            cli.use_unix = 1;
            snprintf(cli.unix_path, sizeof(cli.unix_path), "%s", DOCKER_CLI_DEFAULT_SOCKET);
        }
    }

    const char* command = argv[i];
    i++;

    /* 初期化 */
    api_qs_init();
    if(0 != api_qs_memory_alloc(&cli.json_memory, DOCKER_CLI_JSON_MEMORY_SIZE)){
        fprintf(stderr, "docker_cli: json memory alloc failed\n");
        return 1;
    }

    int ret;
    if(0 == strcmp(command, "version")){
        ret = cmd_version(&cli);
    }
    else if(0 == strcmp(command, "info")){
        ret = cmd_info(&cli);
    }
    else if(0 == strcmp(command, "ps")){
        int all = 0;
        for(; i < argc; i++){
            if(0 == strcmp(argv[i], "--all")){
                all = 1;
            }
        }
        ret = cmd_ps(&cli, all);
    }
    else if(0 == strcmp(command, "images")){
        ret = cmd_images(&cli);
    }
    else if(0 == strcmp(command, "rmi")){
        const char* name = NULL;
        int force = 0;
        for(; i < argc; i++){
            if(0 == strcmp(argv[i], "--force")){
                force = 1;
            }
            else if(name == NULL){
                name = argv[i];
            }
        }
        ret = cmd_rmi(&cli, name, force);
    }
    else if(0 == strcmp(command, "tag")){
        ret = cmd_tag(&cli, i < argc ? argv[i] : NULL, i + 1 < argc ? argv[i + 1] : NULL);
    }
    else if(0 == strcmp(command, "history")){
        ret = cmd_history(&cli, i < argc ? argv[i] : NULL);
    }
    else if(0 == strcmp(command, "run")){
        ret = cmd_run(&cli, argc - i, &argv[i]);
    }
    else if(0 == strcmp(command, "start")){
        ret = cmd_container_action(&cli, "start", i < argc ? argv[i] : NULL);
    }
    else if(0 == strcmp(command, "stop")){
        ret = cmd_container_action(&cli, "stop", i < argc ? argv[i] : NULL);
    }
    else if(0 == strcmp(command, "restart")){
        ret = cmd_container_action(&cli, "restart", i < argc ? argv[i] : NULL);
    }
    else if(0 == strcmp(command, "kill")){
        const char* id = NULL;
        const char* signal_name = "KILL";
        for(; i < argc; i++){
            if(0 == strcmp(argv[i], "--signal") && i + 1 < argc){
                signal_name = argv[++i];
            }
            else if(id == NULL){
                id = argv[i];
            }
        }
        ret = cmd_kill(&cli, id, signal_name);
    }
    else if(0 == strcmp(command, "rm")){
        int force = 0;
        const char* id = NULL;
        for(; i < argc; i++){
            if(0 == strcmp(argv[i], "--force")){
                force = 1;
            }
            else if(id == NULL){
                id = argv[i];
            }
        }
        ret = cmd_rm(&cli, id, force);
    }
    else if(0 == strcmp(command, "logs")){
        ret = cmd_logs(&cli, i < argc ? argv[i] : NULL);
    }
    else if(0 == strcmp(command, "top")){
        ret = cmd_top(&cli, i < argc ? argv[i] : NULL);
    }
    else if(0 == strcmp(command, "port")){
        ret = cmd_port(&cli, i < argc ? argv[i] : NULL);
    }
    else if(0 == strcmp(command, "wait")){
        ret = cmd_wait(&cli, i < argc ? argv[i] : NULL);
    }
    else if(0 == strcmp(command, "stats")){
        ret = cmd_stats(&cli, i < argc ? argv[i] : NULL);
    }
    else if(0 == strcmp(command, "network")){
        const char* sub = (i < argc) ? argv[i] : NULL;
        if(sub != NULL && 0 == strcmp(sub, "ls")){
            ret = cmd_network_ls(&cli);
        }
        else{
            fprintf(stderr, "docker_cli: network currently supports only 'ls'\n");
            ret = 1;
        }
    }
    else if(0 == strcmp(command, "volume")){
        const char* sub = (i < argc) ? argv[i] : NULL;
        if(sub != NULL && 0 == strcmp(sub, "ls")){
            ret = cmd_volume_ls(&cli);
        }
        else{
            fprintf(stderr, "docker_cli: volume currently supports only 'ls'\n");
            ret = 1;
        }
    }
    else if(0 == strcmp(command, "prune")){
        const char* kind = NULL;
        const char* filter = NULL;
        for(; i < argc; i++){
            if(0 == strcmp(argv[i], "--filter") && i + 1 < argc){
                filter = argv[++i];
            }
            else if(kind == NULL){
                kind = argv[i];
            }
        }
        ret = cmd_prune(&cli, kind, filter);
    }
    else{
        fprintf(stderr, "docker_cli: unknown command: %s\n", command);
        print_usage();
        ret = 1;
    }

    api_qs_memory_free(&cli.json_memory);
    return ret;
}
