/*
 * Copyright (c) Katsuya Owari
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
	if(0 != (error=api_qs_server_init(&context,server_port,max_connection,QS_SERVER_TYPE_SIMPLE))){
        printf("api_qs_server_init error:%d\n",error);
        return -1;
    }
    api_qs_set_server_session_timeout(context, 0);
	api_qs_set_on_connect_event(context, on_connect );
	
	//api_qs_set_on_plain_event(context, on_recv );
	api_qs_set_on_simple_event(context, on_recv );

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
	uint32_t payload_type = api_qs_get_plain_payload_type(params);
    uint8_t* payload = api_qs_get_plain_payload(params);
    size_t payload_len = api_qs_get_plain_payload_length(params);
	printf("payload_type:%u, payload_len:%zu, payload:%.*s\n", payload_type, payload_len, (int)payload_len, payload);

	// plain response
	//api_qs_send_response(params, "hoge");

	// simple response
	api_qs_send_response_with_payload(params, 0x10, "response with payload");

	//uint32_t connection_offset = api_qs_get_connection_offset(params);
	//time_t create_time = api_qs_get_connection_create_time(params);
	//printf("connection_offset:%d, create_time:%ld\n", connection_offset, create_time);
	//QS_SERVER_CONTEXT* context = api_qs_get_server_context(params);
	//api_qs_send_response_by_connection_offset(context, connection_offset, "send response by connection offset");

    return 0;
}

int on_close(QS_EVENT_PARAMETER params)
{
    printf("on_close\n");
	return 0;
}
