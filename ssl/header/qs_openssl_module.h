/*
 * Copyright (c) Katsuya Owari
 */

#define _GNU_SOURCE

#ifdef __cplusplus
extern "C"{
#endif

#ifndef _QS_OPENSSL_MODULE_H_
#define _QS_OPENSSL_MODULE_H_

#include <sys/types.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "qs_api.h"

#define QS_SSL_MODULE_ENABLED 1

# ifdef QS_SSL_MODULE_ENABLED
// sudo apt install libssl-dev
#include <openssl/ssl.h>
#include <openssl/err.h>
# endif

#define QS_SSL_MODULE_PHASE_CONNECT 0
#define QS_SSL_MODULE_PHASE_READ_HEADER 1
#define QS_SSL_MODULE_PHASE_READ_BODY 2
#define QS_SSL_MODULE_PHASE_READ_CHUNKED_BODY 3
#define QS_SSL_MODULE_PHASE_DISCONNECT 4

#define QS_HTTP_CLIENT_HOST_SIZE 1024
#define QS_HTTP_CLIENT_PORT_SIZE 16
#define QS_HTTP_CLIENT_REQUEST_BUFFER_SIZE (1024 * 1024)
#define QS_HTTP_CLIENT_READ_BUFFER_SIZE (1024 * 1024)
#define QS_HTTP_CLIENT_TRANSIENT_MEMORY_SIZE (1024 * 1024 * 3)
#define QS_HTTP_CLIENT_HEADER_BUFFER_SIZE (1024 * 1024)
#define QS_HTTP_CLIENT_BODY_BUFFER_SIZE (1024 * 1024 * 4)
#define QS_HTTP_CLIENT_CHUNK_SIZE_BUFFER_SIZE 32
#define QS_HTTP_CLIENT_RESPONSE_MEMORY_SIZE (QS_HTTP_CLIENT_HEADER_BUFFER_SIZE + QS_HTTP_CLIENT_BODY_BUFFER_SIZE + 4096)

typedef struct QS_HTTP_CLIENT_CONTEXT
{
# ifdef QS_SSL_MODULE_ENABLED
    SSL_CTX *ctx;
    SSL *ssl;
# endif
    QS_CLIENT_CONTEXT* client_context;
    QS_MEMORY_CONTEXT transient_memory_context;
    QS_MEMORY_CONTEXT response_memory_context;
    int32_t memid_host;
    int32_t memid_port;
    int32_t memid_request_buffer;
    int32_t memid_read_buffer;
    int32_t memid_header_buffer;
    int32_t memid_body_buffer;
    int32_t memid_chunk_size_buffer;
    int socket;
    int is_ssl;
    
    // working data
    int phase;
	size_t body_length;
	size_t total_read_body_length;
    size_t temp_max_body_length;
	size_t temp_chunked_size;
	size_t temp_chunked_read_size;
    size_t body_write_offset;
    int chunk_size_buffer_len;
    int waiting_for_chunk_trailer;
} QS_HTTP_CLIENT_CONTEXT;

int qs_ssl_module_http_client_connect(QS_HTTP_CLIENT_CONTEXT* context,const char* server_host, int server_port, int is_ssl);
# ifdef QS_SSL_MODULE_ENABLED
SSL_CTX* qs_ssl_module_http_client_ssl_create_context();
SSL* qs_ssl_module_http_client_ssl_create(SSL_CTX* ctx, int sock);
# endif
int qs_ssl_module_http_client_update(QS_HTTP_CLIENT_CONTEXT* context);
int qs_ssl_module_http_client_recv(QS_HTTP_CLIENT_CONTEXT* context, char* payload, size_t payload_size);
int qs_ssl_module_http_client_free(QS_HTTP_CLIENT_CONTEXT* context);
int qs_ssl_module_http_client_dispose(QS_HTTP_CLIENT_CONTEXT* context);
int qs_ssl_module_http_client_get_header(QS_HTTP_CLIENT_CONTEXT* context, const char* key, char* value, size_t value_size);
char* qs_ssl_module_http_client_get_request_buffer(QS_HTTP_CLIENT_CONTEXT* context);
const char* qs_ssl_module_http_client_get_header_buffer(QS_HTTP_CLIENT_CONTEXT* context);
const char* qs_ssl_module_http_client_get_body_buffer(QS_HTTP_CLIENT_CONTEXT* context);

#endif /*_QS_OPENSSL_MODULE_H_*/

#ifdef __cplusplus
}
#endif
