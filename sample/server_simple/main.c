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

int on_connect(QS_EVENT_PARAMETER params);
int on_recv(QS_EVENT_PARAMETER params);
int on_close(QS_EVENT_PARAMETER params);

QS_MEMORY_CONTEXT g_temporary_memory;

int main( int argc, char *argv[], char *envp[] )
{
#ifdef __WINDOWS__
	SetConsoleOutputCP(CP_UTF8);
#endif
	api_qs_memory_alloc(&g_temporary_memory,1024*1024*4);
	int server_port = 52001;
	QS_SERVER_CONTEXT* context = 0;
	int32_t max_connection = 100;
    int error = 0;
	if(0 != (error=api_qs_server_init(&context,server_port,max_connection,QS_SERVER_TYPE_PLAIN))){
        printf("api_qs_server_init error:%d\n",error);
        return -1;
    }
    api_qs_set_server_session_timeout(context, 0);
	api_qs_set_on_connect_event(context, on_connect );
	api_qs_set_on_plain_event(context, on_recv );
	api_qs_set_on_close_event(context, on_close );
	int timer = time(0);
	for(;;){
		api_qs_update(context);
		api_qs_sleep(context);
		if(time(0) - timer > 10){
			timer = time(0);
		}
	}
	api_qs_free(context);
	api_qs_memory_free(&g_temporary_memory);
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

	api_qs_send_response(params, "hoge");

    return 0;
}

int on_close(QS_EVENT_PARAMETER params)
{
    printf("on_close\n");
	return 0;
}
