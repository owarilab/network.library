#include "qs_openssl_module.h"

//#define QS_OPENSSL_MODULE_DEBUG 1


int qs_ssl_module_http_client_on_connect(QS_EVENT_PARAMETER params);
int qs_ssl_module_http_client_on_recv(QS_EVENT_PARAMETER params);
int qs_ssl_module_http_client_on_close(QS_EVENT_PARAMETER params);

int qs_ssl_module_http_client_on_connect(QS_EVENT_PARAMETER params)
{
#if QS_OPENSSL_MODULE_DEBUG
    printf("qs_ssl_module_http_client_on_connect\n");
#endif
    QS_CLIENT_CONTEXT* context = api_qs_client_get_context(params);
    QS_HTTP_CLIENT_CONTEXT* http_client_context = (QS_HTTP_CLIENT_CONTEXT*)context->client_data;
#if QS_OPENSSL_MODULE_DEBUG
    printf("sending request:%s\n",http_client_context->request_buffer);
#endif
    api_qs_send_response(params, http_client_context->request_buffer);
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

int qs_ssl_module_http_client_connect(QS_HTTP_CLIENT_CONTEXT* context,const char* server_host, int server_port, int is_ssl)
{
    context->ssl = NULL;
    context->ctx = NULL;
    context->client_context = NULL;
    memset(context->host, 0, sizeof(context->host));
    memset(context->port, 0, sizeof(context->port));
    memset(context->request_buffer, 0, sizeof(context->request_buffer));
    memset(context->read_buffer, 0, sizeof(context->read_buffer));

    context->is_ssl = is_ssl;

    context->phase = QS_SSL_MODULE_PHASE_CONNECT;
    context->body_length = 0;
    context->total_read_body_length = 0;
    context->temp_chunked_size = 0;
    context->temp_chunked_read_size = 0;
    memset(context->header_buffer, 0, sizeof(context->header_buffer));
    memset(context->body_buffer, 0, sizeof(context->body_buffer));
    context->body_buffer_ptr = context->body_buffer;

    int error = 0;
	if(0 != (error=api_qs_client_init(&context->client_context,server_host,server_port,QS_SERVER_TYPE_HTTP))){
        printf("api_qs_client_init error:%d\n",error);
        return -1;
    }

    context->client_context->client_data = context;

    context->socket = api_qs_client_get_socket(context->client_context);

    if(context->is_ssl)
    {
        context->ctx = qs_ssl_module_http_client_ssl_create_context();
        if(context->ctx == NULL)
        {
            return -1;
        }

        context->ssl = qs_ssl_module_http_client_ssl_create(context->ctx, context->socket);
        if(context->ssl == NULL)
        {
            return -1;
        }

        // call connect()
        api_qs_client_update(context->client_context);
    }else{
        api_qs_set_client_on_connect_event(context->client_context, qs_ssl_module_http_client_on_connect );
        api_qs_set_client_on_plain_event(context->client_context, qs_ssl_module_http_client_on_recv );
        api_qs_set_client_on_close_event(context->client_context, qs_ssl_module_http_client_on_close );
    }

    return 0;
}

int qs_ssl_module_http_client_update(QS_HTTP_CLIENT_CONTEXT* context)
{
    if(context->phase == QS_SSL_MODULE_PHASE_DISCONNECT){
        return 0;
    }
    if(context->is_ssl == 0){
        api_qs_client_update(context->client_context);
        return 0;
    }
    if(context->phase == QS_SSL_MODULE_PHASE_CONNECT){
        // connect
        do{
            int ret = SSL_connect(context->ssl);
            if(ret == 1){
#if QS_OPENSSL_MODULE_DEBUG
                printf("Connected with %s encryption\n", SSL_get_cipher(context->ssl));
#endif

#if QS_OPENSSL_MODULE_DEBUG
                printf("sending request:%s\n",context->request_buffer);
#endif
                SSL_write(context->ssl, context->request_buffer, strlen(context->request_buffer));
                context->phase = QS_SSL_MODULE_PHASE_READ_HEADER;
                break;
            }
            int err = SSL_get_error(context->ssl, ret);
            if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE) {
                printf("SSL_connect error\n");
                ERR_print_errors_fp(stderr);
                context->phase = QS_SSL_MODULE_PHASE_DISCONNECT;
                break;
            }
        }while(0);
    }else{
        int ret = SSL_read(context->ssl, context->read_buffer, sizeof(context->read_buffer));
        int err = SSL_get_error(context->ssl, ret);
        if(err != 0){
            if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE) {
#if QS_OPENSSL_MODULE_DEBUG
                printf("SSL_read error %d\n",err);
#endif
                ERR_print_errors_fp(stderr);
                context->phase = QS_SSL_MODULE_PHASE_DISCONNECT;
                return -1;
            }
        }
        if(ret <= 0){
            return 0;
        }

        int read_bytes = ret;
        char* payload = context->read_buffer;
        int qs_ssl_module_http_client_recv_ret = qs_ssl_module_http_client_recv(context,payload,read_bytes);
        memset(context->read_buffer, 0, sizeof(context->read_buffer));
        return qs_ssl_module_http_client_recv_ret;
    }
    return 0;
}

int qs_ssl_module_http_client_recv(QS_HTTP_CLIENT_CONTEXT* context, char* payload, size_t payload_size)
{
#if QS_OPENSSL_MODULE_DEBUG
    printf("qs_ssl_module_http_client_recv. phase:%d, payload_size:%ld\n",context->phase,payload_size);
#endif
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
                if(header_length > sizeof(context->header_buffer)){
                    printf("header_length is too long %d > %d\n",header_length,(int)sizeof(context->header_buffer));
                    context->phase = QS_SSL_MODULE_PHASE_DISCONNECT;
                    break;
                }
                memcpy(context->header_buffer,payload,header_length);
                context->header_buffer[header_length] = 0;
#if QS_OPENSSL_MODULE_DEBUG
                printf("header(%d):\n%.*s\n",header_length,header_length,context->header_buffer);
#endif
                char* body = header_end + new_line_size;
                size_t body_size = payload_size - (body - payload);
                int read_body_length = 0;

                // if Transfer-Encoding: chunked
                char* chunked = strstr(context->header_buffer,"Transfer-Encoding: chunked");
                if(chunked != 0){
#if QS_OPENSSL_MODULE_DEBUG
                    printf("chunked\n");
#endif
                    // チャンクサイズ行の検出（バイナリセーフ）
                    char* chunked_body = body;
                    size_t remain = body_size;
                    char* rn = memchr(chunked_body, '\r', remain);
                    char* n = memchr(chunked_body, '\n', remain);
                    char* line_end = NULL;
                    if (rn && (n == NULL || rn < n)) {
                        line_end = rn;
                    } else if (n) {
                        line_end = n;
                    }
                    if(line_end != 0){
                        int chunked_length = line_end - chunked_body;
                        char chunked_bytes[32];
                        int copy_len = chunked_length < 31 ? chunked_length : 31;
                        memcpy(chunked_bytes,chunked_body,copy_len);
                        chunked_bytes[copy_len] = 0;
                        context->temp_chunked_size = strtol(chunked_bytes,0,16);
#if QS_OPENSSL_MODULE_DEBUG
                        printf("chunked_bytes(%d):\n%.*s\n",chunked_length,chunked_length,chunked_bytes);
                        printf("chunked_size:%ld bytes\n",context->temp_chunked_size);
                        printf("\n");
#endif
                        // 行末の\r\nまたは\nをスキップ
                        int skip = 1;
                        if(line_end[0] == '\r' && line_end[1] == '\n') skip = 2;
                        body = line_end + skip;
                        body_size = payload_size - (body - payload);
                        context->body_length += context->temp_chunked_size;
                        read_body_length = body_size;
                        context->phase = QS_SSL_MODULE_PHASE_READ_CHUNKED_BODY;
                    }
                }

                // Content-Length
                char* content_length = strstr(context->header_buffer,"Content-Length:");
                if(content_length != 0){
#if QS_OPENSSL_MODULE_DEBUG
                    printf("content_length\n");
#endif
                    int content_length_size = 0;
                    sscanf(content_length,"Content-Length: %d",&content_length_size);
#if QS_OPENSSL_MODULE_DEBUG
                    printf("content_length_size:%d bytes\n",content_length_size);
                    printf("\n");
#endif
                    context->body_length += content_length_size;
                    read_body_length = body_size;
                    context->phase = QS_SSL_MODULE_PHASE_READ_BODY;
                }

                context->total_read_body_length += read_body_length;
                context->temp_chunked_read_size += read_body_length;
#if QS_OPENSSL_MODULE_DEBUG
                printf("body total(%ld):\n",context->total_read_body_length);
                printf("<<<<<<<<<<<<<<<<<<<<\n");
                fwrite(body, 1, read_body_length, stdout);
                printf("\n<<<<<<<<<<<<<<<<<<<<\n");
#endif
                memcpy(context->body_buffer_ptr,body,read_body_length);
                context->body_buffer_ptr += read_body_length;

                if(context->total_read_body_length >= context->body_length){
#if QS_OPENSSL_MODULE_DEBUG
                    printf("body total(%ld) >= body_length(%ld)\n",context->total_read_body_length,context->body_length);
#endif
                    context->phase = QS_SSL_MODULE_PHASE_DISCONNECT;
                }
            }
        }while(0);
    }
    else if(context->phase == QS_SSL_MODULE_PHASE_READ_BODY){
#if QS_OPENSSL_MODULE_DEBUG
        printf("QS_SSL_MODULE_PHASE_READ_BODY\n");
#endif
        context->total_read_body_length += payload_size;
        memcpy(context->body_buffer_ptr,payload,payload_size);
        context->body_buffer_ptr += payload_size;

        if(context->total_read_body_length >= context->body_length){
#if QS_OPENSSL_MODULE_DEBUG
            printf("body total(%ld) >= body_length(%ld)\n",context->total_read_body_length,context->body_length);
#endif
            context->phase = QS_SSL_MODULE_PHASE_DISCONNECT;
        }
    }
    else if(context->phase == QS_SSL_MODULE_PHASE_READ_CHUNKED_BODY){
        // read chunked body
        do{
            char* p_read_pos = payload;
            char* body = payload;
            int read_body_length = 0;
            int current_read_bytes = payload_size;
            int continue_read = 0;
            do{
                continue_read = 0;
                if(context->temp_chunked_read_size >= context->temp_chunked_size){
                    context->temp_chunked_read_size = 0;
#if QS_OPENSSL_MODULE_DEBUG
                    printf("chunked\n");
#endif
                    char* chunked_body = strstr(body,"\r\n");
                    if(chunked_body != 0){
                        int chunked_length = chunked_body - body;
                        char chunked_bytes[chunked_length+1];
                        memcpy(chunked_bytes,body,chunked_length);
                        chunked_bytes[chunked_length] = 0;
                        context->temp_chunked_size = strtol(chunked_bytes,0,16);
#if QS_OPENSSL_MODULE_DEBUG
                        printf("chunked_bytes(%d):\n%s\n",chunked_length,chunked_bytes);
                        printf("chunked_size:%ld bytes\n",context->temp_chunked_size);
                        printf("\n");
#endif
                        body = chunked_body + 2;
                        context->body_length += context->temp_chunked_size;
                        read_body_length = current_read_bytes - (body - p_read_pos);
                        context->total_read_body_length += read_body_length;
                        context->temp_chunked_read_size += read_body_length;
                    }
                }else{
                    read_body_length = current_read_bytes;
                    context->total_read_body_length += read_body_length;
                    context->temp_chunked_read_size += read_body_length;
                }

#if QS_OPENSSL_MODULE_DEBUG
                printf("body total(%ld):\n",context->total_read_body_length);
                printf("read_body_length:(%d) ,	strlen:%d\n",read_body_length,(int)strlen(body));
                printf("<<<<<<<<<<<<<<<<<<<<\n");
                printf("%s\n",body);
                printf("<<<<<<<<<<<<<<<<<<<<\n");
#endif

                if(context->temp_chunked_read_size>context->temp_chunked_size){
#if QS_OPENSSL_MODULE_DEBUG
                    printf("chunk size is over\n");
                    printf("temp_chunked_read_size:%ld\n",context->temp_chunked_read_size);
                    printf("temp_chunked_size:%ld\n",context->temp_chunked_size);
#endif
                    int over_size = context->temp_chunked_read_size - context->temp_chunked_size;
                    read_body_length -= over_size;
                    context->total_read_body_length -= over_size;
                    memcpy(context->body_buffer_ptr,body,read_body_length);
                    context->body_buffer_ptr += read_body_length;

                    body = body + read_body_length;
                    p_read_pos = body;
                    current_read_bytes = over_size;
#if QS_OPENSSL_MODULE_DEBUG
                    printf("next body %s\n",body);
#endif
                    if(over_size > 2){
#if QS_OPENSSL_MODULE_DEBUG
                        printf("over_size > 2. next chunk read\n");
#endif
                        continue_read = 1;
                    }
                }else{
                    memcpy(context->body_buffer_ptr,body,read_body_length);
                    context->body_buffer_ptr += read_body_length;
                }
            }while(continue_read);
        }while(0);
    }
    return 0;
}

int qs_ssl_module_http_client_free(QS_HTTP_CLIENT_CONTEXT* context)
{
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
    if(context->client_context != NULL)
    {
        api_qs_client_free(context->client_context);
        context->client_context = NULL;
    }
    memset(context->host, 0, sizeof(context->host));
    memset(context->port, 0, sizeof(context->port));
    memset(context->read_buffer, 0, sizeof(context->read_buffer));
    context->socket = 0;
    return 0;
}

int qs_ssl_module_http_client_get_header(QS_HTTP_CLIENT_CONTEXT* context, const char* key, char* value, size_t value_size)
{
    const char* start = strstr(context->header_buffer, key);
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