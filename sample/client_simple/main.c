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

int main( int argc, char *argv[], char *envp[] )
{
#ifdef __WINDOWS__
	SetConsoleOutputCP(CP_UTF8);
#endif
    const char* server_host = "localhost";
	int server_port = 52001;
	QS_CLIENT_CONTEXT* context = 0;
    int error = 0;
	if(0 != (error=api_qs_client_init(&context,server_host,server_port,QS_SERVER_TYPE_SIMPLE))){
        printf("api_qs_client_init error:%d\n",error);
        return -1;
    }
	api_qs_set_client_on_connect_event(context, on_connect );
	//api_qs_set_client_on_plain_event(context, on_recv );
	api_qs_set_client_on_simple_event(context, on_recv );
	api_qs_set_client_on_close_event(context, on_close );
	int timer = time(0);
	for(;;){
		api_qs_client_update(context);
		api_qs_client_sleep(context);
		if(time(0) - timer > 3){
			//api_qs_client_send(context,"test1",5);
			api_qs_client_send_message(context,0x05,"test5_data",10);
			timer = time(0);
		}
	}
	api_qs_client_free(context);
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
	printf("payload_type:0x%08x, payload_len:%zu, payload:%.*s\n", payload_type, payload_len, (int)payload_len, payload);
    return 0;
}

int on_close(QS_EVENT_PARAMETER params)
{
    printf("on_close\n");
	return 0;
}
