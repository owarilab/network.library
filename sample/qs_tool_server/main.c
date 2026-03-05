/*
 * Copyright (c) Katsuya Owari
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "qs_api.h"

int on_connect(QS_EVENT_PARAMETER params);
int on_http_event(QS_EVENT_PARAMETER params);
int on_ws_event(QS_EVENT_PARAMETER params);
int on_close(QS_EVENT_PARAMETER params);

QS_MEMORY_CONTEXT g_temporary_memory;

int main( int argc, char *argv[], char *envp[] )
{
#ifdef __WINDOWS__
	SetConsoleOutputCP(CP_UTF8);
#endif
	api_qs_memory_alloc(&g_temporary_memory,1024*1024*4);
	int server_port = 8080;
	{
		QS_SERVER_SCRIPT_CONTEXT script;
		if(-1==api_qs_script_read_file(&g_temporary_memory, &script, "./server.conf")){return -1;}
		if(-1==api_qs_script_run(&script)){return -1;}
		if(0!=api_qs_script_get_parameter(&script,"server_port")){
			server_port = atoi(api_qs_script_get_parameter(&script,"server_port"));
		}
		api_qs_memory_clean(&g_temporary_memory);
	}
	QS_SERVER_CONTEXT* context = 0;
	int32_t max_connection = 10;
	if(0 > api_qs_server_init(&context,server_port,max_connection,QS_SERVER_TYPE_HTTP)){return -1;}
	if(-1==api_qs_set_scheduler(context,QS_SCHEDULER_MODE_LOW)){return -1;}
	if(-1==api_qs_server_create_router(context)){return -1;}
	if(-1==api_qs_server_create_kvs(context,QS_KVS_MEMORY_TYPE_B1MB)){return -1;}
	//if(-1==api_qs_server_create_logger_access(context,"./access_log.txt")){return -1;}
	//if(-1==api_qs_server_create_logger_debug(context,"./debug_log.txt")){return -1;}
	//if(-1==api_qs_server_create_logger_error(context,"./error_log.txt")){return -1;}
	api_qs_set_on_connect_event(context, on_connect );
	api_qs_set_on_http_event(context, on_http_event );
	api_qs_set_on_websocket_event(context, on_ws_event );
	api_qs_set_on_close_event(context, on_close );
	for(;;){
		api_qs_update(context);
		api_qs_sleep(context);
	}
	api_qs_free(context);
	api_qs_memory_free(&g_temporary_memory);
	return 0;
}

int on_connect(QS_EVENT_PARAMETER params)
{
	return 0;
}

int on_http_event(QS_EVENT_PARAMETER params)
{
	int http_status_code = 404;
	//QS_SERVER_CONTEXT* context = api_qs_get_server_context(params);
	api_qs_memory_clean(&g_temporary_memory);
	// curl -X POST  -H "Content-Type: application/json" -d '{"arr1":[{"id":1,"value":"arr1_v1"},{"id":2,"value":"arr1_v2"}],"arr2":[{"id":1,"value":"arr2_v1"},{"id":2,"value":"arr2_v2"}]}' "http://localhost:4444/api/json_test"
	if(!strcmp(api_qs_get_http_method(params),"POST")){
		if(!strcmp(api_qs_get_http_path(params),"/api/json_test")){
			printf("body : %s\n",api_qs_get_http_post_body(params));
			http_status_code = 200;
		}
	}
	return http_status_code;
}

int on_ws_event(QS_EVENT_PARAMETER params)
{
	char* message = api_qs_get_ws_message(params);
	api_qs_send_ws_message(params,message);
	return 0;
}

int on_close(QS_EVENT_PARAMETER params)
{
	return 0;
}
