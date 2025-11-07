/*
 * Copyright (c) Katsuya Owari
 */

#include "libqs.h"
#include <stdio.h>
#include <string.h>

int on_connect(QS_EVENT_PARAMETER params);
int on_http_event(QS_EVENT_PARAMETER params);
int on_ws_event(QS_EVENT_PARAMETER params);
int on_close(QS_EVENT_PARAMETER params);

QS_MEMORY_CONTEXT temporary_memory;
QS_SERVER_CONTEXT* server_context = 0;
libqs_api_callback_unity_debug_log debug_log = 0;
libqs_api_callback_unity_on_recv_http_request on_recv_http_request = 0;

int on_connect(QS_EVENT_PARAMETER params)
{
	return 0;
}

int on_http_event(QS_EVENT_PARAMETER params)
{
	int http_status_code = 404;
	if(on_recv_http_request){
		http_status_code = on_recv_http_request(params);
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

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_init()
{
	api_qs_memory_alloc(&temporary_memory,1024*1024*4);
    return api_qs_init();
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_init_http_server(uint32_t server_port, uint32_t max_connection)
{
	if(0 > api_qs_server_init(&server_context,server_port,max_connection,QS_SERVER_TYPE_HTTP)){return -1;}
	//if(-1==api_qs_server_create_router(libqs_working_data.server_context)){return -1;}
	//if(-1==api_qs_server_create_kvs(libqs_working_data.server_context,QS_KVS_MEMORY_TYPE_B1MB)){return -1;}
	//if(-1==api_qs_server_create_logger_access(libqs_working_data.server_context,"./access_log.txt")){return -1;}
	//if(-1==api_qs_server_create_logger_debug(libqs_working_data.server_context,"./debug_log.txt")){return -1;}
	//if(-1==api_qs_server_create_logger_error(libqs_working_data.server_context,"./error_log.txt")){return -1;}
	api_qs_set_on_connect_event(server_context, on_connect );
	api_qs_set_on_http_event(server_context, on_http_event );
	api_qs_set_on_websocket_event(server_context, on_ws_event );
	api_qs_set_on_close_event(server_context, on_close );
	return 0;
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_update()
{
	api_qs_update(server_context);
	return 0;
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_free()
{
	api_qs_memory_free(&temporary_memory);
	debug_log = 0;
	on_recv_http_request = 0;
	if(server_context!=0){
		api_qs_free(server_context);
	}
	return 0;
}

UNITY_INTERFACE_EXPORT uint32_t UNITY_INTERFACE_API libqs_api_rand()
{
    return api_qs_rand();
}

UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_set_debug_log(libqs_api_callback_unity_debug_log callback)
{
    debug_log = callback;
}

UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_set_on_recv_http_request(libqs_api_callback_unity_on_recv_http_request callback)
{
	on_recv_http_request = callback;
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_send_response(void* http_request_parameter,char* response)
{
	api_qs_send_response(http_request_parameter, response);
	return 0;
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_send_http_response_json(void* http_request_parameter,char* json)
{
	api_qs_memory_clean(&temporary_memory);
	return api_qs_http_response_raw_json(http_request_parameter,&temporary_memory,json);
}

UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_get_http_method(void* http_request_parameter)
{
	return api_qs_get_http_method(http_request_parameter);
}
UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_get_http_path(void* http_request_parameter)
{
	return api_qs_get_http_path(http_request_parameter);
}
UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_get_http_get_parameter(void* http_request_parameter, char* name)
{
	return api_qs_get_http_get_parameter(http_request_parameter, name);
}
UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_APIlibqs_api_get_http_post_parameter(void* http_request_parameter, char* name)
{
	return api_qs_get_http_post_parameter(http_request_parameter, name);
}
UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_get_http_post_body(void* http_request_parameter)
{
	return api_qs_get_http_post_body(http_request_parameter);
}


void libqs_api_debug_log(char* str)
{
    if (debug_log)
    {
        debug_log(str);
    }
}

