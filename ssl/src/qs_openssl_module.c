/*
 * Copyright (c) Katsuya Owari
 */

#include "qs_openssl_module.h"
#include "qs_socket.h"

//#define QS_OPENSSL_MODULE_DEBUG 1

static char* qs_ssl_module_http_client_get_host_buffer(QS_HTTP_CLIENT_CONTEXT* context);
static char* qs_ssl_module_http_client_get_port_buffer(QS_HTTP_CLIENT_CONTEXT* context);
static char* qs_ssl_module_http_client_get_read_buffer(QS_HTTP_CLIENT_CONTEXT* context);
static char* qs_ssl_module_http_client_get_header_buffer_mutable(QS_HTTP_CLIENT_CONTEXT* context);
static char* qs_ssl_module_http_client_get_body_buffer_mutable(QS_HTTP_CLIENT_CONTEXT* context);
static char* qs_ssl_module_http_client_get_chunk_size_buffer_mutable(QS_HTTP_CLIENT_CONTEXT* context);
static int qs_ssl_module_http_client_alloc_transient_buffers(QS_HTTP_CLIENT_CONTEXT* context, const char* server_host, int server_port);
static void qs_ssl_module_http_client_free_transient_buffers(QS_HTTP_CLIENT_CONTEXT* context);
static int qs_ssl_module_http_client_alloc_response_buffers(QS_HTTP_CLIENT_CONTEXT* context);
static void qs_ssl_module_http_client_free_response_buffers(QS_HTTP_CLIENT_CONTEXT* context);
static int qs_ssl_module_http_client_append_body(QS_HTTP_CLIENT_CONTEXT* context, const char* payload, size_t payload_size);

int qs_ssl_module_http_client_on_connect(QS_EVENT_PARAMETER params);
int qs_ssl_module_http_client_on_recv(QS_EVENT_PARAMETER params);
int qs_ssl_module_http_client_on_close(QS_EVENT_PARAMETER params);

static char* qs_ssl_module_http_client_get_host_buffer(QS_HTTP_CLIENT_CONTEXT* context)
{
    return (char*)api_qs_memory_get_pointer(&context->transient_memory_context, context->memid_host);
}

static char* qs_ssl_module_http_client_get_port_buffer(QS_HTTP_CLIENT_CONTEXT* context)
{
    return (char*)api_qs_memory_get_pointer(&context->transient_memory_context, context->memid_port);
}

char* qs_ssl_module_http_client_get_request_buffer(QS_HTTP_CLIENT_CONTEXT* context)
{
    return (char*)api_qs_memory_get_pointer(&context->transient_memory_context, context->memid_request_buffer);
}

static char* qs_ssl_module_http_client_get_header_buffer_mutable(QS_HTTP_CLIENT_CONTEXT* context)
{
    return (char*)api_qs_memory_get_pointer(&context->response_memory_context, context->memid_header_buffer);
}

const char* qs_ssl_module_http_client_get_header_buffer(QS_HTTP_CLIENT_CONTEXT* context)
{
    return (const char*)qs_ssl_module_http_client_get_header_buffer_mutable(context);
}

static char* qs_ssl_module_http_client_get_body_buffer_mutable(QS_HTTP_CLIENT_CONTEXT* context)
{
    return (char*)api_qs_memory_get_pointer(&context->response_memory_context, context->memid_body_buffer);
}

const char* qs_ssl_module_http_client_get_body_buffer(QS_HTTP_CLIENT_CONTEXT* context)
{
    return (const char*)qs_ssl_module_http_client_get_body_buffer_mutable(context);
}

static char* qs_ssl_module_http_client_get_read_buffer(QS_HTTP_CLIENT_CONTEXT* context)
{
    return (char*)api_qs_memory_get_pointer(&context->transient_memory_context, context->memid_read_buffer);
}

static char* qs_ssl_module_http_client_get_chunk_size_buffer_mutable(QS_HTTP_CLIENT_CONTEXT* context)
{
    return (char*)api_qs_memory_get_pointer(&context->response_memory_context, context->memid_chunk_size_buffer);
}

static int qs_ssl_module_http_client_alloc_transient_buffers(QS_HTTP_CLIENT_CONTEXT* context, const char* server_host, int server_port)
{
    char* host_buffer = NULL;
    char* port_buffer = NULL;
    char* request_buffer = NULL;
    char* read_buffer = NULL;

    context->transient_memory_context.memory = NULL;
    context->memid_host = -1;
    context->memid_port = -1;
    context->memid_request_buffer = -1;
    context->memid_read_buffer = -1;

    if(api_qs_memory_alloc(&context->transient_memory_context, QS_HTTP_CLIENT_TRANSIENT_MEMORY_SIZE) != 0){
        return -1;
    }

    context->memid_host = api_qs_memory_create_block(&context->transient_memory_context, QS_HTTP_CLIENT_HOST_SIZE);
    context->memid_port = api_qs_memory_create_block(&context->transient_memory_context, QS_HTTP_CLIENT_PORT_SIZE);
    context->memid_request_buffer = api_qs_memory_create_block(&context->transient_memory_context, QS_HTTP_CLIENT_REQUEST_BUFFER_SIZE);
    context->memid_read_buffer = api_qs_memory_create_block(&context->transient_memory_context, QS_HTTP_CLIENT_READ_BUFFER_SIZE);
    if(context->memid_host == -1 || context->memid_port == -1 || context->memid_request_buffer == -1 || context->memid_read_buffer == -1){
        qs_ssl_module_http_client_free_transient_buffers(context);
        return -1;
    }

    host_buffer = qs_ssl_module_http_client_get_host_buffer(context);
    port_buffer = qs_ssl_module_http_client_get_port_buffer(context);
    request_buffer = qs_ssl_module_http_client_get_request_buffer(context);
    read_buffer = qs_ssl_module_http_client_get_read_buffer(context);
    if(host_buffer == NULL || port_buffer == NULL || request_buffer == NULL || read_buffer == NULL){
        qs_ssl_module_http_client_free_transient_buffers(context);
        return -1;
    }

    memset(host_buffer, 0, QS_HTTP_CLIENT_HOST_SIZE);
    memset(port_buffer, 0, QS_HTTP_CLIENT_PORT_SIZE);
    memset(request_buffer, 0, QS_HTTP_CLIENT_REQUEST_BUFFER_SIZE);
    memset(read_buffer, 0, QS_HTTP_CLIENT_READ_BUFFER_SIZE);

    strncpy(host_buffer, server_host, QS_HTTP_CLIENT_HOST_SIZE - 1);
    snprintf(port_buffer, QS_HTTP_CLIENT_PORT_SIZE, "%d", server_port);
    return 0;
}

static void qs_ssl_module_http_client_free_transient_buffers(QS_HTTP_CLIENT_CONTEXT* context)
{
    if(context->transient_memory_context.memory == NULL){
        context->memid_host = -1;
        context->memid_port = -1;
        context->memid_request_buffer = -1;
        context->memid_read_buffer = -1;
        return;
    }

    if(context->memid_host != -1){
        api_qs_memory_free_block(&context->transient_memory_context, &context->memid_host);
    }
    if(context->memid_port != -1){
        api_qs_memory_free_block(&context->transient_memory_context, &context->memid_port);
    }
    if(context->memid_request_buffer != -1){
        api_qs_memory_free_block(&context->transient_memory_context, &context->memid_request_buffer);
    }
    if(context->memid_read_buffer != -1){
        api_qs_memory_free_block(&context->transient_memory_context, &context->memid_read_buffer);
    }
    api_qs_memory_free(&context->transient_memory_context);
    context->transient_memory_context.memory = NULL;
}

static int qs_ssl_module_http_client_alloc_response_buffers(QS_HTTP_CLIENT_CONTEXT* context)
{
    char* header_buffer = NULL;
    char* body_buffer = NULL;
    char* chunk_size_buffer = NULL;

    context->response_memory_context.memory = NULL;
    context->memid_header_buffer = -1;
    context->memid_body_buffer = -1;
    context->memid_chunk_size_buffer = -1;

    if(api_qs_memory_alloc(&context->response_memory_context, QS_HTTP_CLIENT_RESPONSE_MEMORY_SIZE) != 0){
        return -1;
    }

    context->memid_header_buffer = api_qs_memory_create_block(&context->response_memory_context, QS_HTTP_CLIENT_HEADER_BUFFER_SIZE);
    context->memid_body_buffer = api_qs_memory_create_block(&context->response_memory_context, QS_HTTP_CLIENT_BODY_BUFFER_SIZE);
    context->memid_chunk_size_buffer = api_qs_memory_create_block(&context->response_memory_context, QS_HTTP_CLIENT_CHUNK_SIZE_BUFFER_SIZE);
    if(context->memid_header_buffer == -1 || context->memid_body_buffer == -1 || context->memid_chunk_size_buffer == -1){
        qs_ssl_module_http_client_free_response_buffers(context);
        return -1;
    }

    header_buffer = qs_ssl_module_http_client_get_header_buffer_mutable(context);
    body_buffer = qs_ssl_module_http_client_get_body_buffer_mutable(context);
    chunk_size_buffer = qs_ssl_module_http_client_get_chunk_size_buffer_mutable(context);
    if(header_buffer == NULL || body_buffer == NULL || chunk_size_buffer == NULL){
        qs_ssl_module_http_client_free_response_buffers(context);
        return -1;
    }

    memset(header_buffer, 0, QS_HTTP_CLIENT_HEADER_BUFFER_SIZE);
    memset(body_buffer, 0, QS_HTTP_CLIENT_BODY_BUFFER_SIZE);
    memset(chunk_size_buffer, 0, QS_HTTP_CLIENT_CHUNK_SIZE_BUFFER_SIZE);
    return 0;
}

static void qs_ssl_module_http_client_free_response_buffers(QS_HTTP_CLIENT_CONTEXT* context)
{
    if(context->response_memory_context.memory == NULL){
        context->memid_header_buffer = -1;
        context->memid_body_buffer = -1;
        context->memid_chunk_size_buffer = -1;
        return;
    }

    if(context->memid_header_buffer != -1){
        api_qs_memory_free_block(&context->response_memory_context, &context->memid_header_buffer);
    }
    if(context->memid_body_buffer != -1){
        api_qs_memory_free_block(&context->response_memory_context, &context->memid_body_buffer);
    }
    if(context->memid_chunk_size_buffer != -1){
        api_qs_memory_free_block(&context->response_memory_context, &context->memid_chunk_size_buffer);
    }
    api_qs_memory_free(&context->response_memory_context);
    context->response_memory_context.memory = NULL;
}

static int qs_ssl_module_http_client_append_body(QS_HTTP_CLIENT_CONTEXT* context, const char* payload, size_t payload_size)
{
    char* body_buffer = qs_ssl_module_http_client_get_body_buffer_mutable(context);
    if(body_buffer == NULL){
        return -1;
    }
    if((context->body_write_offset + payload_size) > QS_HTTP_CLIENT_BODY_BUFFER_SIZE){
        printf("body buffer is too small %zu > %d\n", context->body_write_offset + payload_size, QS_HTTP_CLIENT_BODY_BUFFER_SIZE);
        context->phase = QS_SSL_MODULE_PHASE_DISCONNECT;
        return -1;
    }
    memcpy(body_buffer + context->body_write_offset, payload, payload_size);
    context->body_write_offset += payload_size;
    body_buffer[context->body_write_offset] = 0;
    return 0;
}

int qs_ssl_module_http_client_on_connect(QS_EVENT_PARAMETER params)
{
#if QS_OPENSSL_MODULE_DEBUG
    printf("qs_ssl_module_http_client_on_connect\n");
#endif
    QS_CLIENT_CONTEXT* context = api_qs_client_get_context(params);
    QS_HTTP_CLIENT_CONTEXT* http_client_context = (QS_HTTP_CLIENT_CONTEXT*)context->client_data;
    char* request_buffer = qs_ssl_module_http_client_get_request_buffer(http_client_context);
#if QS_OPENSSL_MODULE_DEBUG
    printf("sending request:%s\n",request_buffer);
#endif
    api_qs_send_response(params, request_buffer);
    http_client_context->phase = QS_SSL_MODULE_PHASE_READ_HEADER;
	return 0;
}

int qs_ssl_module_http_client_on_recv(QS_EVENT_PARAMETER params)
{
#if QS_OPENSSL_MODULE_DEBUG
    printf("qs_ssl_module_http_client_on_recv\n");
#endif
    uint8_t* payload = api_qs_get_plain_payload(params);
    size_t payload_len = api_qs_get_plain_payload_length(params);
#if QS_OPENSSL_MODULE_DEBUG
    printf(">>>>>\npayload:%s\n>>>>>>\n, len:%d\n",(char*)payload,(int)payload_len);
#endif
    QS_CLIENT_CONTEXT* context = api_qs_client_get_context(params);
    QS_HTTP_CLIENT_CONTEXT* http_client_context = (QS_HTTP_CLIENT_CONTEXT*)context->client_data;
    qs_ssl_module_http_client_recv(http_client_context,(char*)payload,payload_len);
    return 0;
}

int qs_ssl_module_http_client_on_close(QS_EVENT_PARAMETER params)
{
#if QS_OPENSSL_MODULE_DEBUG
    printf("qs_ssl_module_http_client_on_close\n");
#endif
    QS_CLIENT_CONTEXT* context = api_qs_client_get_context(params);
    QS_HTTP_CLIENT_CONTEXT* http_client_context = (QS_HTTP_CLIENT_CONTEXT*)context->client_data;
    http_client_context->phase = QS_SSL_MODULE_PHASE_DISCONNECT;
    return 0;
}

#ifdef QS_SSL_MODULE_ENABLED
SSL_CTX* qs_ssl_module_http_client_ssl_create_context()
{
	SSL_CTX *ctx;
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();
	ctx = SSL_CTX_new(SSLv23_client_method());
	if (ctx == NULL)
	{
		ERR_print_errors_fp(stderr);
		return NULL;
	}
	SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2);
	return ctx;
}

SSL* qs_ssl_module_http_client_ssl_create(SSL_CTX* ctx, int sock)
{
	SSL *ssl;
	ssl = SSL_new(ctx);
	if (ssl == NULL)
	{
		ERR_print_errors_fp(stderr);
		return NULL;
	}
	SSL_set_fd(ssl, sock);
	return ssl;
}
#endif // QS_SSL_MODULE_ENABLED

int qs_ssl_module_http_client_connect(QS_HTTP_CLIENT_CONTEXT* context,const char* server_host, int server_port, int is_ssl)
{
#ifdef QS_SSL_MODULE_ENABLED
    context->ssl = NULL;
    context->ctx = NULL;
#endif
    context->client_context = NULL;
    if(qs_ssl_module_http_client_alloc_transient_buffers(context, server_host, server_port) != 0){
        return -1;
    }
    if(qs_ssl_module_http_client_alloc_response_buffers(context) != 0){
        qs_ssl_module_http_client_free_transient_buffers(context);
        return -1;
    }

    context->is_ssl = is_ssl;

    context->phase = QS_SSL_MODULE_PHASE_CONNECT;
    context->body_length = 0;
    context->total_read_body_length = 0;
    context->temp_max_body_length = 0;
    context->temp_chunked_size = 0;
    context->temp_chunked_read_size = 0;
    context->body_write_offset = 0;
    context->chunk_size_buffer_len = 0;
    context->waiting_for_chunk_trailer = 0;

    int error = 0;
	if(0 != (error=api_qs_client_init(&context->client_context,server_host,server_port,QS_SERVER_TYPE_HTTP))){
        printf("api_qs_client_init error:%d\n",error);
        qs_ssl_module_http_client_free_transient_buffers(context);
        qs_ssl_module_http_client_free_response_buffers(context);
        return -1;
    }

    context->client_context->client_data = context;

    context->socket = api_qs_client_get_socket(context->client_context);

    if(context->is_ssl)
    {
#ifdef QS_SSL_MODULE_ENABLED
        context->ctx = qs_ssl_module_http_client_ssl_create_context();
        if(context->ctx == NULL)
        {
            qs_ssl_module_http_client_free(context);
            qs_ssl_module_http_client_free_response_buffers(context);
            return -1;
        }

        context->ssl = qs_ssl_module_http_client_ssl_create(context->ctx, context->socket);
        if(context->ssl == NULL)
        {
            qs_ssl_module_http_client_free(context);
            qs_ssl_module_http_client_free_response_buffers(context);
            return -1;
        }

        // call connect()
        api_qs_client_update(context->client_context);
#else
        printf("SSL module is not enabled.\n");
    qs_ssl_module_http_client_free(context);
    qs_ssl_module_http_client_free_response_buffers(context);
        return -1;
#endif
    }else{
        api_qs_set_client_on_connect_event(context->client_context, qs_ssl_module_http_client_on_connect );
        api_qs_set_client_on_plain_event(context->client_context, qs_ssl_module_http_client_on_recv );
        api_qs_set_client_on_close_event(context->client_context, qs_ssl_module_http_client_on_close );
    }

    return 0;
}

#ifdef QS_SSL_MODULE_ENABLED
static ssize_t qs_ssl_lowlevel_send(QS_SOCKET_ID soc, const char *buf, size_t size, int flag)
{
    QS_SOCKET_OPTION* option = find_socket_option_by_fd(soc);
    if (option != NULL && option->application_data != NULL) {
        QS_CLIENT_CONTEXT* client_context = (QS_CLIENT_CONTEXT*)option->application_data;
        QS_HTTP_CLIENT_CONTEXT* http_ctx = (QS_HTTP_CLIENT_CONTEXT*)client_context->client_data;
        if (http_ctx != NULL && http_ctx->ssl != NULL) {
            int ret = SSL_write(http_ctx->ssl, buf, (int)size);
            if (ret <= 0) {
                int err = SSL_get_error(http_ctx->ssl, ret);
                if (err == SSL_ERROR_WANT_WRITE || err == SSL_ERROR_WANT_READ) {
                    errno = EAGAIN;
                }
                return -1;
            }
            return ret;
        }
    }
    return send(soc, buf, size, flag);
}

static ssize_t qs_ssl_lowlevel_recv(QS_SOCKET_ID soc, char *buf, size_t size, int flag)
{
    QS_SOCKET_OPTION* option = find_socket_option_by_fd(soc);
    if (option != NULL && option->application_data != NULL) {
        QS_CLIENT_CONTEXT* client_context = (QS_CLIENT_CONTEXT*)option->application_data;
        QS_HTTP_CLIENT_CONTEXT* http_ctx = (QS_HTTP_CLIENT_CONTEXT*)client_context->client_data;
        if (http_ctx != NULL && http_ctx->ssl != NULL) {
            int ret = SSL_read(http_ctx->ssl, buf, (int)size);
            if (ret <= 0) {
                int err = SSL_get_error(http_ctx->ssl, ret);
                if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                    errno = EAGAIN;
                }
                return -1;
            }
            return ret;
        }
    }
    return recv(soc, buf, size, flag);
}
#endif

int qs_ssl_module_http_client_update(QS_HTTP_CLIENT_CONTEXT* context)
{
    if(context->phase == QS_SSL_MODULE_PHASE_DISCONNECT){
        return 0;
    }
    if(context->is_ssl == 0){
        api_qs_client_update(context->client_context);
        return 0;
    }
#ifdef QS_SSL_MODULE_ENABLED
    QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)context->client_context->memory;
    QS_SOCKET_OPTION* client = (QS_SOCKET_OPTION*)QS_GET_POINTER(memory, context->client_context->memid_client);

    // 1. TCP接続が完了するまではTCP接続を進める
    if (client->client_connection_status == CLIENT_STATUS_CONNECTING ||
        client->client_connection_status == CLIENT_STATUS_CONNECT_PENDING) {
        api_qs_client_update(context->client_context);
        return 0;
    }

    // 2. SSLハンドシェイク
    if(context->phase == QS_SSL_MODULE_PHASE_CONNECT){
        int ret = SSL_connect(context->ssl);
        if(ret == 1){
#if QS_OPENSSL_MODULE_DEBUG
            printf("Connected with %s encryption\n", SSL_get_cipher(context->ssl));
#endif
            // ハンドシェイク完了後に初めてフックとイベントを登録する
            api_qs_client_set_lowlevel_send_hook(context->client_context, qs_ssl_lowlevel_send);
            api_qs_client_set_lowlevel_recv_hook(context->client_context, qs_ssl_lowlevel_recv);

            api_qs_set_client_on_connect_event(context->client_context, qs_ssl_module_http_client_on_connect );
            api_qs_set_client_on_plain_event(context->client_context, qs_ssl_module_http_client_on_recv );
            api_qs_set_client_on_close_event(context->client_context, qs_ssl_module_http_client_on_close );

            QS_EVENT_PARAMETER_STRUCT params;
            params.parameter_type = QS_EVENT_PARAMETER_TYPE_CONNECTION;
            params.params = QS_GET_POINTER(memory, client->connection_munit);
            qs_ssl_module_http_client_on_connect(&params);
        } else {
            int err = SSL_get_error(context->ssl, ret);
            if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE) {
                printf("SSL_connect error\n");
                ERR_print_errors_fp(stderr);
                context->phase = QS_SSL_MODULE_PHASE_DISCONNECT;
            }
        }
    }
    // 3. 通信実行フェーズ：coreの非同期送受信ハンドラに任せる（透過的にフックされる）
    else {
        api_qs_client_update(context->client_context);
    }
#endif // QS_SSL_MODULE_ENABLED
    return 0;
}

int qs_ssl_module_http_client_recv(QS_HTTP_CLIENT_CONTEXT* context, char* payload, size_t payload_size)
{
    char* header_buffer = qs_ssl_module_http_client_get_header_buffer_mutable(context);
    char* chunk_size_buffer = qs_ssl_module_http_client_get_chunk_size_buffer_mutable(context);
#if QS_OPENSSL_MODULE_DEBUG
    printf("qs_ssl_module_http_client_recv. phase:%d, payload_size:%ld\n",context->phase,payload_size);
#endif
    if(header_buffer == NULL || chunk_size_buffer == NULL){
        context->phase = QS_SSL_MODULE_PHASE_DISCONNECT;
        return -1;
    }
    if(context->phase == QS_SSL_MODULE_PHASE_READ_HEADER){
        // read header
        do{
            int new_line_size = 0;
            // バイナリセーフなヘッダー終端検出
            char *header_end = NULL;
            void *p = memmem(payload, payload_size, "\r\n\r\n", 4);
            if (p) {
                header_end = (char*)p;
                new_line_size = 4;
            } else {
                p = memmem(payload, payload_size, "\n\n", 2);
                if (p) {
                    header_end = (char*)p;
                    new_line_size = 2;
                }
            }
            if(header_end != 0){
                int header_length = header_end - payload;
                if(header_length > QS_HTTP_CLIENT_HEADER_BUFFER_SIZE - 1){
                    printf("header_length is too long %d > %d\n",header_length,QS_HTTP_CLIENT_HEADER_BUFFER_SIZE - 1);
                    context->phase = QS_SSL_MODULE_PHASE_DISCONNECT;
                    break;
                }
                memcpy(header_buffer,payload,header_length);
                header_buffer[header_length] = 0;
#if QS_OPENSSL_MODULE_DEBUG
                printf("header(%d):\n%.*s\n",header_length,header_length,header_buffer);
#endif
                char* body = header_end + new_line_size;
                size_t body_size = payload_size - (body - payload);
                int read_body_length = 0;

                // Content-Length
                char* content_length = strstr(header_buffer,"Content-Length:");
                // if Transfer-Encoding: chunked
                char* chunked = strstr(header_buffer,"Transfer-Encoding: chunked");
                if(chunked != 0){
#if QS_OPENSSL_MODULE_DEBUG
                    printf("Transfer-Encoding: chunked detected\n");
#endif
                    context->phase = QS_SSL_MODULE_PHASE_READ_CHUNKED_BODY;
                    context->temp_chunked_size = 0;
                    context->temp_chunked_read_size = 0;
                    context->chunk_size_buffer_len = 0;
                    context->waiting_for_chunk_trailer = 0;
                    // Process any body data that came with the header
                    if (body_size > 0) {
                        qs_ssl_module_http_client_recv(context, body, body_size);
                    }
                    break;
                }
                else if(content_length != 0){
#if QS_OPENSSL_MODULE_DEBUG
                    printf("content_length\n");
#endif
                    int content_length_size = 0;
                    sscanf(content_length,"Content-Length: %d",&content_length_size);
#if QS_OPENSSL_MODULE_DEBUG
                    printf("content_length_size:%d bytes\n",content_length_size);
                    printf("\n");
#endif
                    context->temp_max_body_length = content_length_size;
                    context->body_length = content_length_size;
                    read_body_length = body_size;
                    context->phase = QS_SSL_MODULE_PHASE_READ_BODY;

                    context->total_read_body_length += read_body_length;
                    context->temp_chunked_read_size += read_body_length;
#if QS_OPENSSL_MODULE_DEBUG
                    printf("body total(%ld):\n",context->total_read_body_length);
                    printf("<<<<<<<<<<<<<<<<<<<<\n");
                    fwrite(body, 1, read_body_length, stdout);
                    printf("\n<<<<<<<<<<<<<<<<<<<<\n");
#endif
                    if(qs_ssl_module_http_client_append_body(context, body, read_body_length) != 0){
                        return -1;
                    }

                    if(context->total_read_body_length >= context->temp_max_body_length){
#if QS_OPENSSL_MODULE_DEBUG
                        printf("body total(%ld) >= body_length(%ld)\n",context->total_read_body_length,context->body_length);
#endif
                        context->phase = QS_SSL_MODULE_PHASE_DISCONNECT;
                    }
                } else{
#if QS_OPENSSL_MODULE_DEBUG
                    printf("No Content-Length or Transfer-Encoding: chunked found, reading until disconnect\n");
#endif
                    context->temp_max_body_length = 1024 * 1024 * 8; // TODO BODY_MAX_SIZE
                    context->body_length = 0;
                    context->phase = QS_SSL_MODULE_PHASE_READ_BODY;
                    read_body_length = body_size;

                    context->total_read_body_length += read_body_length;
                    context->temp_chunked_read_size += read_body_length;
#if QS_OPENSSL_MODULE_DEBUG
                    printf("body total(%ld):\n",context->total_read_body_length);
                    printf("<<<<<<<<<<<<<<<<<<<<\n");
                    fwrite(body, 1, read_body_length, stdout);
                    printf("\n<<<<<<<<<<<<<<<<<<<<\n");
#endif
                    if(qs_ssl_module_http_client_append_body(context, body, read_body_length) != 0){
                        return -1;
                    }
                    context->body_length = read_body_length;
                }
            }
        }while(0);
    }
    else if(context->phase == QS_SSL_MODULE_PHASE_READ_BODY){
#if QS_OPENSSL_MODULE_DEBUG
        printf("QS_SSL_MODULE_PHASE_READ_BODY\n");
#endif
        context->total_read_body_length += payload_size;
    if(qs_ssl_module_http_client_append_body(context, payload, payload_size) != 0){
        return -1;
    }
        context->body_length += payload_size;

        if(context->total_read_body_length >= context->temp_max_body_length){
#if QS_OPENSSL_MODULE_DEBUG
            printf("body total(%ld) >= body_length(%ld)\n",context->total_read_body_length,context->body_length);
#endif
            context->phase = QS_SSL_MODULE_PHASE_DISCONNECT;
        }
    }
    else if(context->phase == QS_SSL_MODULE_PHASE_READ_CHUNKED_BODY){
#if QS_OPENSSL_MODULE_DEBUG
        printf("QS_SSL_MODULE_PHASE_READ_CHUNKED_BODY: payload_size%ld\n", payload_size);
#endif
        char* current_pos = payload;
        size_t remaining = payload_size;

        while(remaining > 0) {
            // If waiting for chunk trailer after chunk data
            if (context->waiting_for_chunk_trailer) {
                if(remaining >= 2 && memcmp(current_pos, "\r\n", 2) == 0) {
                    current_pos += 2;
                    remaining -= 2;
                    context->waiting_for_chunk_trailer = 0;
                } else if(remaining >= 1 && *current_pos == '\n') {
                    current_pos += 1;
                    remaining -= 1;
                    context->waiting_for_chunk_trailer = 0;
                } else {
                    // Need more data for trailing CRLF
                    return 0;                    
                }
                continue;
            }

            // If we haven't read chunk size yet
            if (context->temp_chunked_size == 0) {
                // Look for chunk size line (ends with \r\n or \n)
                char* line_end = NULL;
                int line_end_size = 0;
                char* search_start = current_pos;
                size_t search_len = remaining;

                // If we have partial chunk size from previous call, combine it
                char combined_buffer[64];
                if (context->chunk_size_buffer_len > 0) {
                    memcpy(combined_buffer, chunk_size_buffer, context->chunk_size_buffer_len);
                    size_t copy_len = (remaining < (sizeof(combined_buffer) - context->chunk_size_buffer_len)) ? remaining : (sizeof(combined_buffer) - context->chunk_size_buffer_len);
                    memcpy(combined_buffer + context->chunk_size_buffer_len, current_pos, copy_len);
                    search_start = combined_buffer;
                    search_len = context->chunk_size_buffer_len + copy_len;
                }

                // Search for \r\n
                void* p = memmem(search_start, search_len, "\r\n", 2);
                if (p) {
                    line_end = (char*)p;
                    line_end_size = 2;
                } else {
                    // Search for \n
                    p = memmem(search_start, search_len, "\n", 1);
                    if (p) {
                        line_end = (char*)p;
                        line_end_size = 1;
                    }
                }

                if(line_end) {
                    // Parse chunk size (hexadecimal)
                    char chunk_size_str[32] = {0};
                    size_t chunk_line_len = line_end - search_start;
                    if (chunk_line_len >= sizeof(chunk_size_str)) {
                        printf("Chunk size line too long: %zu\n", chunk_line_len);
                        context->phase = QS_SSL_MODULE_PHASE_DISCONNECT;
                        return -1;
                    }
                    memcpy(chunk_size_str, search_start, chunk_line_len);

                    // Parse hex value (ignore chunk extensions after ';')
                    char* semicolon = strchr(chunk_size_str, ';');
                    if (semicolon) {
                        *semicolon = '\0';
                    }
                    context->temp_chunked_size = strtol(chunk_size_str, NULL, 16);
                    context->temp_chunked_read_size = 0;
#if QS_OPENSSL_MODULE_DEBUG
                    printf("Chunk size: %ld (0x%s)\n", context->temp_chunked_size, chunk_size_str);
#endif
                    // Check for final chunk (size 0)
                    if (context->temp_chunked_size == 0) {
#if QS_OPENSSL_MODULE_DEBUG
                        printf("Final chunk detected, waiting for trailer\n");
#endif
                        context->phase = QS_SSL_MODULE_PHASE_DISCONNECT;
                        return 0;
                    }

                    // Move past chunk size line
                    if(context->chunk_size_buffer_len > 0) {
                        // If we used buffered data, calculate how much of current payload to skip
                        size_t consumed_from_current = (line_end - combined_buffer) - context->chunk_size_buffer_len + line_end_size;
                        current_pos += consumed_from_current;
                        remaining -= consumed_from_current;
                        context->chunk_size_buffer_len = 0;
                    } else {
                        current_pos = line_end + line_end_size;
                        remaining -= (chunk_line_len + line_end_size);
                    }
                } else {
                    // Need more data to read chunk size
                    // Save what we have so far in the buffer
                    if(context->chunk_size_buffer_len == 0) {
                        size_t to_save = (remaining < QS_HTTP_CLIENT_CHUNK_SIZE_BUFFER_SIZE) ? remaining : QS_HTTP_CLIENT_CHUNK_SIZE_BUFFER_SIZE;
                        memcpy(chunk_size_buffer, current_pos, to_save);
                        context->chunk_size_buffer_len = to_save;
                    } else {
                        // Append to existing buffer
                        size_t space_left = QS_HTTP_CLIENT_CHUNK_SIZE_BUFFER_SIZE - context->chunk_size_buffer_len;
                        size_t to_append = (remaining < space_left) ? remaining : space_left;
                        memcpy(chunk_size_buffer + context->chunk_size_buffer_len, current_pos, to_append);
                        context->chunk_size_buffer_len += to_append;
                    }
                    return 0;
                }
            }

            // Read chunk data
            if(context->temp_chunked_size > 0 && remaining > 0) {
                size_t chunk_remaining = context->temp_chunked_size - context->temp_chunked_read_size;
                size_t to_read = (remaining < chunk_remaining) ? remaining : chunk_remaining;
#if QS_OPENSSL_MODULE_DEBUG
                printf("Reading chunk data: %ld bytes (chunk remaining: %ld)\n", to_read, chunk_remaining);
#endif
                // Copy chunk data to body buffer
                if(qs_ssl_module_http_client_append_body(context, current_pos, to_read) != 0){
                    return -1;
                }
                context->total_read_body_length += to_read;
                context->body_length += to_read;
                context->temp_chunked_read_size += to_read;

                current_pos += to_read;
                remaining -= to_read;

                // Check if chunk is complete
                if(context->temp_chunked_read_size >= context->temp_chunked_size) {
#if QS_OPENSSL_MODULE_DEBUG
                    printf("Chunk complete: %ld bytes read\n", context->temp_chunked_read_size);
#endif
                    // Need to consume trailing \r\n after chunk data
                    if(remaining >= 2 && memcmp(current_pos, "\r\n", 2) == 0) {
                        current_pos += 2;
                        remaining -= 2;
                    } else if(remaining >= 1 && *current_pos == '\n') {
                        current_pos += 1;
                        remaining -= 1;
                    } else {
                        // Need more data for trailing CRLF
                        context->waiting_for_chunk_trailer = 1;
                        return 0;
                    }

                    // Reset for next chunk
                    context->temp_chunked_size = 0;
                    context->temp_chunked_read_size = 0;
                }
            }
        }
    }
    return 0;
}

int qs_ssl_module_http_client_free(QS_HTTP_CLIENT_CONTEXT* context)
{
#ifdef QS_SSL_MODULE_ENABLED
    if(context->ssl != NULL)
    {
        SSL_shutdown(context->ssl);
        SSL_free(context->ssl);
        context->ssl = NULL;
    }
    if(context->ctx != NULL)
    {
        SSL_CTX_free(context->ctx);
        context->ctx = NULL;
    }
#endif
    if(context->client_context != NULL)
    {
        api_qs_client_free(context->client_context);
        context->client_context = NULL;
    }
    qs_ssl_module_http_client_free_transient_buffers(context);
    context->socket = 0;
    return 0;
}

int qs_ssl_module_http_client_dispose(QS_HTTP_CLIENT_CONTEXT* context)
{
    qs_ssl_module_http_client_free(context);
    qs_ssl_module_http_client_free_response_buffers(context);
    context->body_write_offset = 0;
    context->body_length = 0;
    context->total_read_body_length = 0;
    context->temp_max_body_length = 0;
    context->temp_chunked_size = 0;
    context->temp_chunked_read_size = 0;
    context->chunk_size_buffer_len = 0;
    context->waiting_for_chunk_trailer = 0;
    return 0;
}

int qs_ssl_module_http_client_get_header(QS_HTTP_CLIENT_CONTEXT* context, const char* key, char* value, size_t value_size)
{
    const char* header_buffer = qs_ssl_module_http_client_get_header_buffer(context);
    if(header_buffer == NULL){
        return -1;
    }
    const char* start = strstr(header_buffer, key);
    if (start) {
        start += strlen(key);
        const char* end = strstr(start, "\r\n");
        if (!end) {
            end = strstr(start, "\n");
        }
        if (end) {
            size_t len = end - start;
            if (len >= value_size) {
                len = value_size - 1;
            }
            strncpy(value, start, len);
            value[len] = '\0';
            return 0;
        }
    }
    return -1;
}
