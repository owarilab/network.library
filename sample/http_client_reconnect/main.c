/*
 * Copyright (c) Katsuya Owari
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "qs_api.h"
#include "qs_openssl_module.h"

static int run_request(QS_HTTP_CLIENT_CONTEXT* context,
	const char* server_host,
	int server_port,
	int is_ssl,
	const char* request_path,
	const char* label)
{
	char* request_buffer = NULL;
	const char* header_buffer = NULL;
	const char* body_buffer = NULL;
	char content_length[128];
	char content_type[256];
	char status_message[1024];
	char http_version[16];
	int status_code = 0;

	if(0 != qs_ssl_module_http_client_connect(context, server_host, server_port, is_ssl)){
		printf("[%s] qs_ssl_module_http_client_connect error\n", label);
		return -1;
	}

	request_buffer = qs_ssl_module_http_client_get_request_buffer(context);
	if(request_buffer == NULL){
		printf("[%s] request buffer is null\n", label);
		qs_ssl_module_http_client_free(context);
		return -1;
	}

	snprintf(request_buffer, QS_HTTP_CLIENT_REQUEST_BUFFER_SIZE,
		"GET %s HTTP/1.1\r\n"
		"Host: %s\r\n"
		"Connection: close\r\n"
		"\r\n",
		request_path,
		server_host);

	printf("[%s] connecting start...\n", label);
	while(1){
		qs_ssl_module_http_client_update(context);
		if(context->phase == QS_SSL_MODULE_PHASE_DISCONNECT){
			break;
		}
		if(context->client_context == NULL){
			printf("[%s] client_context became null\n", label);
			return -1;
		}
		api_qs_client_sleep(context->client_context);
	}

	header_buffer = qs_ssl_module_http_client_get_header_buffer(context);
	body_buffer = qs_ssl_module_http_client_get_body_buffer(context);
	if(header_buffer == NULL || body_buffer == NULL){
		printf("[%s] response buffer is null\n", label);
		return -1;
	}

	memset(http_version, 0, sizeof(http_version));
	memset(status_message, 0, sizeof(status_message));
	if(sscanf(header_buffer, "HTTP/%15s %d %1023[^\r\n]", http_version, &status_code, status_message) < 2){
		printf("[%s] failed to parse status line\n", label);
		return -1;
	}

	memset(content_length, 0, sizeof(content_length));
	memset(content_type, 0, sizeof(content_type));
	qs_ssl_module_http_client_get_header(context, "Content-Length: ", content_length, sizeof(content_length));
	qs_ssl_module_http_client_get_header(context, "Content-Type: ", content_type, sizeof(content_type));

	printf("[%s] status=%d %s version=%s\n", label, status_code, status_message, http_version);
	printf("[%s] content_type=%s content_length=%s body_length=%ld total_read=%ld\n",
		label,
		content_type[0] ? content_type : "(none)",
		content_length[0] ? content_length : "(none)",
		(long)context->body_length,
		(long)context->total_read_body_length);
	printf("[%s] body preview: %.120s\n", label, body_buffer);
	return 0;
}

int main(int argc, char *argv[], char *envp[])
{
#ifdef __WINDOWS__
	SetConsoleOutputCP(CP_UTF8);
#endif

	QS_HTTP_CLIENT_CONTEXT context;
	const char* server_host = "localhost";
	const char* request_path = "/index.html";
	int server_port = 4444;
	int is_ssl = 0;

	(void)argc;
	(void)argv;
	(void)envp;

	memset(&context, 0, sizeof(context));
	printf("initial connectable: %d\n", qs_ssl_module_http_client_is_connectable(&context));

	if(run_request(&context, server_host, server_port, is_ssl, request_path, "request-1") != 0){
		qs_ssl_module_http_client_dispose(&context);
		return -1;
	}

	qs_ssl_module_http_client_free(&context);
	printf("after free connectable: %d\n", qs_ssl_module_http_client_is_connectable(&context));

	if(run_request(&context, server_host, server_port, is_ssl, request_path, "request-2-after-free") != 0){
		qs_ssl_module_http_client_dispose(&context);
		return -1;
	}

	qs_ssl_module_http_client_dispose(&context);
	printf("after dispose connectable: %d\n", qs_ssl_module_http_client_is_connectable(&context));

	if(run_request(&context, server_host, server_port, is_ssl, request_path, "request-3-after-dispose") != 0){
		qs_ssl_module_http_client_dispose(&context);
		return -1;
	}

	qs_ssl_module_http_client_dispose(&context);
	printf("final connectable: %d\n", qs_ssl_module_http_client_is_connectable(&context));
	printf("reconnect test completed successfully\n");
	return 0;
}