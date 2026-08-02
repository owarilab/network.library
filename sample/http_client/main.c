/*
 * Copyright (c) Katsuya Owari
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "qs_api.h"
#include "qs_openssl_module.h"

int main( int argc, char *argv[], char *envp[] )
{
#ifdef __WINDOWS__
	SetConsoleOutputCP(CP_UTF8);
#endif

	QS_HTTP_CLIENT_CONTEXT context;
	memset(&context, 0, sizeof(context));

	// ssl
	//const char* server_host = "www.google.com";
	//const char* request_path = "/";
	//int server_port = 443;
	//int is_ssl = 1;

	// plain
	const char* server_host = "localhost";
	const char* request_path = "/index.html";
	int server_port = 4444;
	int is_ssl = 0;
	if(0 != qs_ssl_module_http_client_connect(&context,server_host,server_port,is_ssl)){
		printf("qs_ssl_module_http_client_connect error\n");
		return -1;
	}

	char* request_buffer = qs_ssl_module_http_client_get_request_buffer(&context);
	if(request_buffer == NULL){
		qs_ssl_module_http_client_free(&context);
		return -1;
	}

	snprintf(request_buffer,QS_HTTP_CLIENT_REQUEST_BUFFER_SIZE,
		"GET %s HTTP/1.1\r\n"
		"Host: %s\r\n"
		"Connection: close\r\n"
		"\r\n",
		request_path,
		server_host);

	printf("connecting start...\n");

	while(1){
		qs_ssl_module_http_client_update(&context);
		if(context.phase == QS_SSL_MODULE_PHASE_DISCONNECT){
			break;
		}
		api_qs_client_sleep(context.client_context);
	}
	qs_ssl_module_http_client_free(&context);
	const char* header_buffer = qs_ssl_module_http_client_get_header_buffer(&context);
	const char* body_buffer = qs_ssl_module_http_client_get_body_buffer(&context);
	if(header_buffer == NULL || body_buffer == NULL){
		qs_ssl_module_http_client_dispose(&context);
		return -1;
	}

	printf("qs_client_simple_result\n");
	printf("header:\n%s\n\n\n",header_buffer);

	char http_version[16];
	int status_code;
	char status_message[1024];
	sscanf(header_buffer,"HTTP/%s %d %s\r\n",http_version,&status_code,status_message);
	printf("http_version:%s\n",http_version);
	printf("status_code:%d\n",status_code);
	printf("status_message:%s\n",status_message);

	char content_type[1024];
	qs_ssl_module_http_client_get_header(&context,"Content-Type: ",content_type,sizeof(content_type));
	printf("content_type: %s\n", content_type);

	char date[1024];
	qs_ssl_module_http_client_get_header(&context,"Date: ",date,sizeof(date));
	printf("date: %s\n", date);

	char content_length[1024];
	memset(content_length,0,sizeof(content_length));
	qs_ssl_module_http_client_get_header(&context,"Content-Length: ",content_length,sizeof(content_length));
	printf("content_length: %s\n", content_length);

	printf("\n\n");
	printf("body:\n");
	printf("%s\n",body_buffer);

	printf("total_read_body_length: %ld bytes\n", context.total_read_body_length);
	printf("body_length: %ld bytes\n", context.body_length);

	// save body to file
	FILE* file = fopen("response_body.txt", "wb");
	if (file) {
		fwrite(body_buffer, 1, context.body_length, file);
		fclose(file);
		printf("Response body saved to response_body.txt\n");
	} else {
		printf("Failed to open file for writing\n");
	}
	qs_ssl_module_http_client_dispose(&context);
	
	return 0;
}
