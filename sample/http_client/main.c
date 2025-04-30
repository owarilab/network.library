/*
 * Copyright (c) 2014-2024 Katsuya Owari
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * * Redistributions of source code must retain the above copyright notice, 
 *   this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice, 
 *   this list of conditions and the following disclaimer in the documentation 
 *   and/or other materials provided with the distribution.
 * * Neither the name of the <organization> nor the names of its contributors 
 *   may be used to endorse or promote products derived from this software 
 *   without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "qs_api.h"
#include "qs_openssl_module.h"

int on_connect(QS_EVENT_PARAMETER params);
int on_recv(QS_EVENT_PARAMETER params);
int on_close(QS_EVENT_PARAMETER params);

int main( int argc, char *argv[], char *envp[] )
{
#ifdef __WINDOWS__
	SetConsoleOutputCP(CP_UTF8);
#endif

	QS_HTTP_CLIENT_CONTEXT context;
	memset(&context, 0, sizeof(context));
	// 接続先を example.com に変更
	const char* server_host = "google.com";
	const char* request_path = "/";
	int server_port = 443;
	int is_ssl = 1;
	if(0 != api_qs_http_client_connect(&context,server_host,server_port,is_ssl)){
		printf("api_qs_http_client_connect error\n");
		return -1;
	}

	snprintf(context.request_buffer,sizeof(context.request_buffer),
		"GET %s HTTP/1.1\r\n"
		"Host: %s\r\n"
		"Connection: close\r\n"
		"\r\n",
		request_path,
		server_host);

	printf("connecting start...\n");

	while(1){
		api_qs_http_client_update(&context);
		if(context.phase == QS_SSL_MODULE_PHASE_DISCONNECT){
			break;
		}
		api_qs_client_sleep(context.client_context);
	}
	api_qs_http_client_free(&context);

	printf("qs_client_simple_result\n");
	printf("header:\n%s\n",context.header_buffer);

	char http_version[16];
	int status_code;
	char status_message[1024];
	sscanf(context.header_buffer,"HTTP/%s %d %s\r\n",http_version,&status_code,status_message);
	printf("http_version:%s\n",http_version);
	printf("status_code:%d\n",status_code);
	printf("status_message:%s\n",status_message);

	char content_type[1024];
	api_qs_http_client_get_header(&context,"Content-Type: ",content_type,sizeof(content_type));
	printf("content_type: %s\n", content_type);

	char date[1024];
	api_qs_http_client_get_header(&context,"Date: ",date,sizeof(date));
	printf("date: %s\n", date);

	char content_length[1024];
	memset(content_length,0,sizeof(content_length));
	api_qs_http_client_get_header(&context,"Content-Length: ",content_length,sizeof(content_length));
	printf("content_length: %s\n", content_length);

	//printf("body:\n%s\n",context.body_buffer);

    //const char* server_host = "localhost";
	//int server_port = 52001;
	//QS_CLIENT_CONTEXT* context = 0;
    //int error = 0;
	//if(0 != (error=api_qs_client_init(&context,server_host,server_port))){
    //    printf("api_qs_client_init error:%d\n",error);
    //    return -1;
    //}
	//api_qs_set_client_on_connect_event(context, on_connect );
	//api_qs_set_client_on_plain_event(context, on_recv );
	//api_qs_set_client_on_close_event(context, on_close );
	//int timer = time(0);
	//for(;;){
	//	api_qs_client_update(context);
	//	api_qs_client_sleep(context);
	//	if(time(0) - timer > 3){
	//		api_qs_client_send(context,"test1",5);
	//		timer = time(0);
	//	}
	//}
	//api_qs_client_free(context);
	return 0;
}

int on_connect(QS_EVENT_PARAMETER params)
{
    printf("on_connect\n");
	return 0;
}

int on_recv(QS_EVENT_PARAMETER params)
{
    printf("on_recv\n");
    uint8_t* payload = api_qs_get_plain_payload(params);
    size_t payload_len = api_qs_get_plain_payload_length(params);
    printf("payload:%s, len:%d\n",(char*)payload,(int)payload_len);
    return 0;
}

int on_close(QS_EVENT_PARAMETER params)
{
    printf("on_close\n");
	return 0;
}
