/*
 * Copyright (c) 2014-2017 Katsuya Owari
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
#include "libqs_api.h"

int on_connect(QS_EVENT_PARAMETER params);
int on_recv(QS_EVENT_PARAMETER params);
int on_close(QS_EVENT_PARAMETER params);

int main( int argc, char *argv[], char *envp[] )
{
#ifdef __WINDOWS__
	SetConsoleOutputCP(CP_UTF8);
#endif

	QS_SERVER_SCRIPT_CONTEXT script;
	if(-1==api_qs_script_init(&script, "./server.conf")){
		return -1;
	}
	if(-1==api_qs_script_run(&script)){
		return -1;
	}
	if(0!=api_qs_script_get_parameter(&script,"server_port")){
		printf("config.server_port : %s\n" , api_qs_script_get_parameter(&script,"server_port"));
	}
	api_qs_script_free(&script);

	QS_SERVER_CONTEXT* context = 0;
	if(0 > api_qs_init(&context,8080)){
		return -1;
	}
	api_qs_set_on_connect_event(context, on_connect );
	api_qs_set_on_packet_recv_event(context, on_recv );
	api_qs_set_on_close_event(context, on_close );
	for(;;){
		api_qs_update(context);
		api_qs_sleep(context);
	}
	api_qs_free(context);
	return 0;
}

int on_connect(QS_EVENT_PARAMETER params)
{
	return 0;
}

int on_recv(QS_EVENT_PARAMETER params)
{
	int http_status_code = 500;
	printf("[app] on_recv method:%s, path:%s\n",api_qs_get_http_method(params),api_qs_get_http_path(params));
	if(!strcmp(api_qs_get_http_method(params),"GET")){
		if(!strcmp(api_qs_get_http_path(params),"/api/get_test")){
			char* v = api_qs_get_http_get_parameter(params,"v");
			if(v!=0){
				printf("v = %s\n",v);
				char response[1024];
				memset(response,0,sizeof(response));
				snprintf(response,sizeof(response),"HTTP/1.1 200 OK\r\n\r\nv=%s",v);
				api_qs_send_response(params,response);
				http_status_code = 200;
			}
		}
	}
	return http_status_code;
}

int on_close(QS_EVENT_PARAMETER params)
{
	return 0;
}
