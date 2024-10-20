#include "qs_openssl_module.h"

void qs_openssl_module_hoge()
{
    printf("qs_openssl_module_hoge\n");
}

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
    memset(context->read_buffer, 0, sizeof(context->read_buffer));

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
