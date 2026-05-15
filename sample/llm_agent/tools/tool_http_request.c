#include "tool_http_request.h"
#include "tool_common.h"
#include "../agent_core.h"
#include "qs_api.h"
#include "qs_openssl_module.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define HTTP_REQUEST_DEFAULT_TIMEOUT_MS 30000
#define HTTP_REQUEST_DEFAULT_MAX_BODY   (1024 * 1024)
#define HTTP_REQUEST_HARD_MAX_BODY      (1024 * 1024 * 4)

typedef struct {
    QS_HTTP_CLIENT_CONTEXT ctx;
    int         done;
    int         status_code;
    long        elapsed_ms;
} HTTP_SYNC_CTX;

static long get_time_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static int alloc_json_memory(QS_MEMORY_CONTEXT* memory, size_t json_size)
{
    size_t alloc_size = json_size * 8 + (64 * 1024);
    if (alloc_size < 64 * 1024) alloc_size = 64 * 1024;
    return api_qs_memory_alloc(memory, alloc_size);
}

static int header_contains_invalid_chars(const char* value)
{
    if (!value) return 1;
    while (*value) {
        if (*value == '\r' || *value == '\n') return 1;
        value++;
    }
    return 0;
}

static int parse_url(const char* url, char* host, size_t host_sz,
                     int* port, int* is_ssl,
                     char* path, size_t path_sz)
{
    const char* p = url;
    int default_port = 80;
    int use_ssl = 0;

    if (strncmp(p, "http://", 7) == 0) {
        p += 7;
        default_port = 80;
    } else if (strncmp(p, "https://", 8) == 0) {
        p += 8;
        default_port = 443;
        use_ssl = 1;
    } else {
        return -1;
    }

    if (*p == '[') return -1;

    const char* authority_end = p;
    while (*authority_end && *authority_end != '/' && *authority_end != '?' && *authority_end != '#') {
        authority_end++;
    }

    const char* colon = NULL;
    for (const char* scan = p; scan < authority_end; scan++) {
        if (*scan == ':') {
            colon = scan;
            break;
        }
    }

    const char* host_end = colon ? colon : authority_end;
    size_t host_len = (size_t)(host_end - p);
    if (host_len == 0 || host_len >= host_sz) return -1;
    memcpy(host, p, host_len);
    host[host_len] = '\0';

    if (colon) {
        int parsed_port = 0;
        for (const char* scan = colon + 1; scan < authority_end; scan++) {
            if (!isdigit((unsigned char)*scan)) return -1;
            parsed_port = parsed_port * 10 + (*scan - '0');
        }
        if (parsed_port <= 0 || parsed_port > 65535) return -1;
        *port = parsed_port;
    } else {
        *port = default_port;
    }

    *is_ssl = use_ssl;

    if (*authority_end == '\0') {
        snprintf(path, path_sz, "/");
    } else if (*authority_end == '?' || *authority_end == '#') {
        snprintf(path, path_sz, "/%s", authority_end);
    } else {
        size_t path_len = strlen(authority_end);
        if (path_len >= path_sz) return -1;
        memcpy(path, authority_end, path_len + 1);
    }

    return (path[0] == '/') ? 0 : -1;
}

static int build_extra_headers(QS_JSON_ELEMENT_OBJECT* headers_obj,
                               char* extra_headers, size_t extra_headers_size)
{
    extra_headers[0] = '\0';

    QS_JSON_ELEMENT_ARRAY keys;
    if (api_qs_object_get_keys(headers_obj, &keys) != 0) return 0;

    size_t pos = 0;
    int key_count = api_qs_array_get_length(&keys);
    for (int i = 0; i < key_count; i++) {
        char* key = api_qs_array_get_string(&keys, i);
        char* value = key ? api_qs_object_get_string(headers_obj, key) : NULL;
        if (!key || !value || !key[0] || header_contains_invalid_chars(key) || header_contains_invalid_chars(value)) {
            continue;
        }
        if (strcmp(key, "Host") == 0 || strcmp(key, "Content-Length") == 0) {
            continue;
        }

        char line[4096];
        int line_len = snprintf(line, sizeof(line), "%s: %s\r\n", key, value);
        if (line_len <= 0 || (size_t)line_len >= sizeof(line)) return -1;
        if (tool_buf_append(extra_headers, extra_headers_size, &pos, line) != 0) return -1;
    }

    return 0;
}

static int http_sync_request(HTTP_SYNC_CTX* sync,
                             const char* method,
                             const char* host,
                             int port,
                             int is_ssl,
                             const char* path,
                             const char* extra_headers,
                             const char* body,
                             int timeout_ms)
{
    memset(sync, 0, sizeof(*sync));

    if (qs_ssl_module_http_client_connect(&sync->ctx, host, port, is_ssl) != 0) {
        return -1;
    }

    size_t request_pos = 0;
    char line[4096];
    sync->ctx.request_buffer[0] = '\0';

    snprintf(line, sizeof(line), "%s %s HTTP/1.1\r\n", method, path);
    if (tool_buf_append(sync->ctx.request_buffer, sizeof(sync->ctx.request_buffer), &request_pos, line) != 0 ||
        snprintf(line, sizeof(line), "Host: %s\r\n", host) < 0 ||
        tool_buf_append(sync->ctx.request_buffer, sizeof(sync->ctx.request_buffer), &request_pos, line) != 0 ||
        tool_buf_append(sync->ctx.request_buffer, sizeof(sync->ctx.request_buffer), &request_pos, "Connection: close\r\n") != 0) {
        return -1;
    }

    if (extra_headers && extra_headers[0] &&
        tool_buf_append(sync->ctx.request_buffer, sizeof(sync->ctx.request_buffer), &request_pos, extra_headers) != 0) {
        return -1;
    }

    size_t body_len = body ? strlen(body) : 0;
    if (body_len > 0 && (strcmp(method, "POST") == 0 || strcmp(method, "PUT") == 0)) {
        snprintf(line, sizeof(line), "Content-Length: %zu\r\n", body_len);
        if (tool_buf_append(sync->ctx.request_buffer, sizeof(sync->ctx.request_buffer), &request_pos, line) != 0 ||
            tool_buf_append(sync->ctx.request_buffer, sizeof(sync->ctx.request_buffer), &request_pos, "\r\n") != 0 ||
            tool_buf_append(sync->ctx.request_buffer, sizeof(sync->ctx.request_buffer), &request_pos, body) != 0) {
            return -1;
        }
    } else {
        if (tool_buf_append(sync->ctx.request_buffer, sizeof(sync->ctx.request_buffer), &request_pos, "\r\n") != 0) {
            return -1;
        }
    }

    long start_ms = get_time_ms();
    while (!sync->done) {
        qs_ssl_module_http_client_update(&sync->ctx);
        if (sync->ctx.phase == QS_SSL_MODULE_PHASE_DISCONNECT) {
            sync->done = 1;
            break;
        }

        sync->elapsed_ms = get_time_ms() - start_ms;
        if (sync->elapsed_ms > timeout_ms) {
            qs_ssl_module_http_client_free(&sync->ctx);
            return -2;
        }

        if (!sync->ctx.client_context) {
            qs_ssl_module_http_client_free(&sync->ctx);
            return -1;
        }
        api_qs_client_sleep(sync->ctx.client_context);
    }

    sync->elapsed_ms = get_time_ms() - start_ms;
    return 0;
}

static void http_sync_free(HTTP_SYNC_CTX* sync)
{
    qs_ssl_module_http_client_free(&sync->ctx);
}

static int build_headers_object(QS_MEMORY_CONTEXT* memory,
                                const char* raw_headers,
                                QS_JSON_ELEMENT_OBJECT* headers_obj)
{
    if (api_qs_object_create(memory, headers_obj) != 0) return -1;
    if (!raw_headers || !raw_headers[0]) return 0;

    const char* p = raw_headers;
    while (*p && *p != '\n') p++;
    if (*p == '\n') p++;

    while (*p) {
        while (*p == '\r' || *p == '\n') p++;
        if (!*p) break;

        const char* line_end = p;
        while (*line_end && *line_end != '\r' && *line_end != '\n') line_end++;
        const char* colon = memchr(p, ':', (size_t)(line_end - p));
        if (colon) {
            size_t key_len = (size_t)(colon - p);
            const char* value_start = colon + 1;
            while (value_start < line_end && *value_start == ' ') value_start++;
            size_t value_len = (size_t)(line_end - value_start);

            char key[256];
            char value[4096];
            if (key_len >= sizeof(key)) key_len = sizeof(key) - 1;
            if (value_len >= sizeof(value)) value_len = sizeof(value) - 1;

            memcpy(key, p, key_len);
            key[key_len] = '\0';
            memcpy(value, value_start, value_len);
            value[value_len] = '\0';

            api_qs_object_push_string(headers_obj, key, value);
        }

        p = line_end;
        while (*p == '\r' || *p == '\n') p++;
    }

    return 0;
}

static int copy_json_result(char* output, size_t output_size, const char* json)
{
    if (!json) return -1;
    int written = snprintf(output, output_size, "%s", json);
    return (written > 0 && (size_t)written < output_size) ? 0 : -1;
}

int tool_http_request_execute(const char* json_args, char* output, size_t output_size)
{
    if (!json_args || !output || output_size == 0) return -1;

    QS_MEMORY_CONTEXT req_mem = {0};
    QS_JSON_ELEMENT_OBJECT req_obj;
    char method[16] = "GET";
    char host[1024] = "";
    char path[2048] = "/";
    char extra_headers[8192] = "";
    int port = 80;
    int is_ssl = 0;
    int timeout_ms = HTTP_REQUEST_DEFAULT_TIMEOUT_MS;
    size_t max_body_bytes = HTTP_REQUEST_DEFAULT_MAX_BODY;
    const char* body = "";

    if (alloc_json_memory(&req_mem, strlen(json_args)) != 0) {
        snprintf(output, output_size, "{\"error\":\"out of memory\"}");
        return -1;
    }
    if (api_qs_json_decode_object(&req_mem, &req_obj, json_args) != 0) {
        api_qs_memory_free(&req_mem);
        snprintf(output, output_size, "{\"error\":\"invalid JSON input\"}");
        return -1;
    }

    char* method_in = api_qs_object_get_string(&req_obj, "method");
    char* url = api_qs_object_get_string(&req_obj, "url");
    char* body_in = api_qs_object_get_string(&req_obj, "body");
    int64_t* timeout_in = api_qs_object_get_big_integer(&req_obj, "timeout_ms");
    int64_t* max_body_in = api_qs_object_get_big_integer(&req_obj, "max_body_bytes");

    if (method_in && method_in[0]) {
        strncpy(method, method_in, sizeof(method) - 1);
        method[sizeof(method) - 1] = '\0';
    }
    if (!url || !url[0]) {
        api_qs_memory_free(&req_mem);
        snprintf(output, output_size, "{\"error\":\"url is required\"}");
        return -1;
    }
    if (body_in) body = body_in;
    if (timeout_in && *timeout_in > 0) timeout_ms = (int)*timeout_in;
    if (max_body_in && *max_body_in > 0) max_body_bytes = (size_t)*max_body_in;
    if (max_body_bytes > HTTP_REQUEST_HARD_MAX_BODY) max_body_bytes = HTTP_REQUEST_HARD_MAX_BODY;

    if (strcmp(method, "GET") != 0 && strcmp(method, "POST") != 0 &&
        strcmp(method, "PUT") != 0 && strcmp(method, "DELETE") != 0) {
        api_qs_memory_free(&req_mem);
        snprintf(output, output_size, "{\"error\":\"method must be GET, POST, PUT, or DELETE\"}");
        return -1;
    }
    if (parse_url(url, host, sizeof(host), &port, &is_ssl, path, sizeof(path)) != 0) {
        api_qs_memory_free(&req_mem);
        snprintf(output, output_size, "{\"error\":\"invalid URL scheme (use http:// or https://)\"}");
        return -1;
    }
    if (!agent_is_host_allowed(host)) {
        api_qs_memory_free(&req_mem);
        snprintf(output, output_size, "{\"error\":\"host '%s' is not in the allowed hosts whitelist\"}", host);
        return -1;
    }

    QS_JSON_ELEMENT_OBJECT headers_obj;
    if (api_qs_object_get_object(&req_obj, "headers", &headers_obj) == 0) {
        if (build_extra_headers(&headers_obj, extra_headers, sizeof(extra_headers)) != 0) {
            api_qs_memory_free(&req_mem);
            snprintf(output, output_size, "{\"error\":\"headers are too large\"}");
            return -1;
        }
    }

    HTTP_SYNC_CTX sync;
    int request_ret = http_sync_request(&sync, method, host, port, is_ssl, path, extra_headers, body, timeout_ms);
    api_qs_memory_free(&req_mem);

    if (request_ret == -2) {
        snprintf(output, output_size, "{\"error\":\"request timed out\"}");
        return -1;
    }
    if (request_ret != 0) {
        snprintf(output, output_size, "{\"error\":\"connection failed\"}");
        return -1;
    }

    int status_code = 0;
    sscanf(sync.ctx.header_buffer, "HTTP/%*s %d", &status_code);

    char location[2048] = "";
    if ((status_code >= 300 && status_code < 400) ||
        qs_ssl_module_http_client_get_header(&sync.ctx, "Location:", location, sizeof(location)) == 0) {
        http_sync_free(&sync);
        snprintf(output, output_size, "{\"error\":\"redirect detected (auto-redirect disabled)\"}");
        return -1;
    }
    if ((size_t)sync.ctx.body_length > max_body_bytes) {
        http_sync_free(&sync);
        snprintf(output, output_size,
                 "{\"error\":\"response body too large (%zu bytes, limit %zu)\"}",
                 sync.ctx.body_length, max_body_bytes);
        return -1;
    }

    size_t response_alloc = (sync.ctx.body_length * 2) + (strlen(sync.ctx.header_buffer) * 2) + (64 * 1024);
    if (response_alloc < 128 * 1024) response_alloc = 128 * 1024;

    QS_MEMORY_CONTEXT resp_mem = {0};
    QS_JSON_ELEMENT_OBJECT resp_obj;
    QS_JSON_ELEMENT_OBJECT resp_headers;
    int ret = -1;

    if (api_qs_memory_alloc(&resp_mem, response_alloc) == 0 &&
        api_qs_object_create(&resp_mem, &resp_obj) == 0 &&
        build_headers_object(&resp_mem, sync.ctx.header_buffer, &resp_headers) == 0) {
        api_qs_object_push_big_integer(&resp_obj, "ok", 1);
        api_qs_object_push_big_integer(&resp_obj, "status_code", status_code);
        api_qs_object_push_object(&resp_obj, "headers", &resp_headers);
        api_qs_object_push_string(&resp_obj, "body", sync.ctx.body_buffer);
        api_qs_object_push_big_integer(&resp_obj, "response_time_ms", sync.elapsed_ms);

        char* encoded = api_qs_json_encode_object(&resp_obj, response_alloc / 2);
        if (encoded) ret = copy_json_result(output, output_size, encoded);
    }

    if (ret != 0) {
        snprintf(output, output_size, "{\"error\":\"failed to build response\"}");
    }

    http_sync_free(&sync);
    if (resp_mem.memory) api_qs_memory_free(&resp_mem);
    return ret;
}
