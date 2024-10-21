#include "qs_openssl_module.h"

//#define QS_OPENSSL_MODULE_DEBUG 1

SSL_CTX* qs_openssl_module_ssl_create_context()
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

SSL* qs_openssl_module_ssl_create(SSL_CTX* ctx, int sock)
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

int qs_openssl_module_connect(QS_SSL_MODULE_CONTEXT* context,const char* server_host, int server_port)
{
    context->ssl = NULL;
    context->ctx = NULL;
    context->client_context = NULL;
    memset(context->host, 0, sizeof(context->host));
    memset(context->port, 0, sizeof(context->port));
    memset(context->request_buffer, 0, sizeof(context->request_buffer));
    memset(context->read_buffer, 0, sizeof(context->read_buffer));

    context->phase = QS_SSL_MODULE_PHASE_CONNECT;
    context->body_length = 0;
    context->total_read_body_length = 0;
    context->temp_chunked_size = 0;
    context->temp_chunked_read_size = 0;
    memset(context->header_buffer, 0, sizeof(context->header_buffer));
    memset(context->body_buffer, 0, sizeof(context->body_buffer));
    context->body_buffer_ptr = context->body_buffer;

    int error = 0;
	if(0 != (error=api_qs_client_init(&context->client_context,server_host,server_port))){
        printf("api_qs_client_init error:%d\n",error);
        return -1;
    }

    context->socket = api_qs_client_get_socket(context->client_context);

    context->ctx = qs_openssl_module_ssl_create_context();
    if(context->ctx == NULL)
    {
        return -1;
    }

    context->ssl = qs_openssl_module_ssl_create(context->ctx, context->socket);
    if(context->ssl == NULL)
    {
        return -1;
    }

    // call connect()
    api_qs_client_update(context->client_context);

    return 0;
}

int qs_openssl_module_update(QS_SSL_MODULE_CONTEXT* context)
{
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
    }
    else if(context->phase == QS_SSL_MODULE_PHASE_READ_HEADER){
        // read header
        do{
            int ret = SSL_read(context->ssl, context->read_buffer, sizeof(context->read_buffer));
            if(ret > 0){
                int read_bytes = ret;
                char*header_end = strstr(context->read_buffer,"\r\n\r\n");
                if(header_end != 0){
                    int header_length = header_end - context->read_buffer;
                    if(header_length > sizeof(context->header_buffer)){
                        printf("header_length is too long %d > %d\n",header_length,(int)sizeof(context->header_buffer));
                        context->phase = QS_SSL_MODULE_PHASE_DISCONNECT;
                        break;
                    }
                    memcpy(context->header_buffer,context->read_buffer,header_length);
                    context->header_buffer[header_length] = 0;
#if QS_OPENSSL_MODULE_DEBUG
                    printf("header(%d):\n%s\n",header_length,context->header_buffer);
#endif
                    char* body = header_end + 4;

                    int read_body_length = 0;

                    // if Transfer-Encoding: chunked
                    char* chunked = strstr(context->header_buffer,"Transfer-Encoding: chunked");
                    if(chunked != 0){
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
                            read_body_length = read_bytes - (body - context->read_buffer);
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
                        read_body_length = read_bytes - (body - context->read_buffer);
                        context->phase = QS_SSL_MODULE_PHASE_READ_BODY;
                    }

                    context->total_read_body_length += read_body_length;
                    context->temp_chunked_read_size += read_body_length;
#if QS_OPENSSL_MODULE_DEBUG
                    printf("body total(%ld):\n",context->total_read_body_length);
                    printf("<<<<<<<<<<<<<<<<<<<<\n");
                    printf("%s\n",body);
                    printf("<<<<<<<<<<<<<<<<<<<<\n");
#endif
                    memcpy(context->body_buffer_ptr,body,read_body_length);
                    context->body_buffer_ptr += read_body_length;
                }

                memset(context->read_buffer, 0, sizeof(context->read_buffer));
            }
            int err = SSL_get_error(context->ssl, ret);
            if(err != 0){
                if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE) {
#if QS_OPENSSL_MODULE_DEBUG
                    printf("SSL_read error %d\n",err);
#endif
                    ERR_print_errors_fp(stderr);
                    context->phase = QS_SSL_MODULE_PHASE_DISCONNECT;
                    break;
                }
            }

        }while(0);
    }
    else if(context->phase == QS_SSL_MODULE_PHASE_READ_BODY){
        printf("QS_SSL_MODULE_PHASE_READ_BODY\n");
    }
    else if(context->phase == QS_SSL_MODULE_PHASE_READ_CHUNKED_BODY){
        // read chunked body
        do{
            int ret = SSL_read(context->ssl, context->read_buffer, sizeof(context->read_buffer));
            if(ret > 0){
                int read_bytes = ret;

                char* p_read_pos = context->read_buffer;
                char* body = context->read_buffer;
                int read_body_length = 0;
                int current_read_bytes = read_bytes;
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
            }
            memset(context->read_buffer, 0, sizeof(context->read_buffer));

            int err = SSL_get_error(context->ssl, ret);
            if(err != 0){
                if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE) {
#if QS_OPENSSL_MODULE_DEBUG
                    printf("SSL_read error %d\n",err);
#endif
                    ERR_print_errors_fp(stderr);
                    context->phase = QS_SSL_MODULE_PHASE_DISCONNECT;
                    break;
                }
            }

        }while(0);
    }
    else if(context->phase == QS_SSL_MODULE_PHASE_DISCONNECT){
        return 0;
    }

    return 0;
}

int qs_openssl_module_free(QS_SSL_MODULE_CONTEXT* context)
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

