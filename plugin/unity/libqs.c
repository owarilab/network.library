#include "libqs.h"
#include <stdio.h>
#include <string.h>

int on_connect(QS_EVENT_PARAMETER params);
int on_http_event(QS_EVENT_PARAMETER params);
int on_ws_event(QS_EVENT_PARAMETER params);
int on_close(QS_EVENT_PARAMETER params);

static LIBQS_WORKING_DATA libqs_working_data;

int on_connect(QS_EVENT_PARAMETER params)
{
	libqs_api_debug_log("on_connect");
	return 0;
}

int on_http_event(QS_EVENT_PARAMETER params)
{
	libqs_api_debug_log("on_http_event");
	int http_status_code = 404;
	api_qs_memory_clean(&libqs_working_data.temporary_memory);
	if(libqs_working_data.on_recv_http_request){
		char* http_response = libqs_working_data.on_recv_http_request(params);
		http_status_code = api_qs_http_response_raw_json(params,&libqs_working_data.temporary_memory,http_response);
	}
	return http_status_code;
}

int on_ws_event(QS_EVENT_PARAMETER params)
{
	char* message = api_qs_get_ws_message(params);
	api_qs_send_ws_message(params,message);
	//api_qs_send_ws_message_plane(params,message);
	return 0;
}

int on_close(QS_EVENT_PARAMETER params)
{
	libqs_api_debug_log("on_close");
	return 0;
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_init()
{
	memset(&libqs_working_data,0,sizeof(LIBQS_WORKING_DATA));
    libqs_working_data.main_memory.memory = 0;
    libqs_working_data.temporary_memory.memory = 0;
	libqs_working_data.server_context = 0;
    libqs_working_data.debug_log = 0;
	libqs_working_data.on_recv_http_request = 0;
    api_qs_memory_alloc(&libqs_working_data.temporary_memory,1024*1024*32);
    api_qs_memory_alloc(&libqs_working_data.main_memory,1024*1024*64);
    return api_qs_init();
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_init_http_server(uint32_t server_port, uint32_t max_connection)
{
	libqs_api_debug_log("libqs_api_init_http_server");
	if(0 > api_qs_server_init(&libqs_working_data.server_context,server_port,max_connection)){return -1;}
	//if(-1==api_qs_server_create_router(libqs_working_data.server_context)){return -1;}
	//if(-1==api_qs_server_create_kvs(libqs_working_data.server_context,QS_KVS_MEMORY_TYPE_B1MB)){return -1;}
	//if(-1==api_qs_server_create_logger_access(libqs_working_data.server_context,"./access_log.txt")){return -1;}
	//if(-1==api_qs_server_create_logger_debug(libqs_working_data.server_context,"./debug_log.txt")){return -1;}
	//if(-1==api_qs_server_create_logger_error(libqs_working_data.server_context,"./error_log.txt")){return -1;}
	api_qs_set_on_connect_event(libqs_working_data.server_context, on_connect );
	api_qs_set_on_http_event(libqs_working_data.server_context, on_http_event );
	api_qs_set_on_websocket_event(libqs_working_data.server_context, on_ws_event );
	api_qs_set_on_close_event(libqs_working_data.server_context, on_close );
	return 0;
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_update()
{
	api_qs_update(libqs_working_data.server_context);
	return 0;
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_free()
{
	if(libqs_working_data.server_context){
		api_qs_free(libqs_working_data.server_context);
	}
	if(libqs_working_data.temporary_memory.memory){
		api_qs_memory_free(&libqs_working_data.temporary_memory);
	}
	if(libqs_working_data.main_memory.memory){
		api_qs_memory_free(&libqs_working_data.main_memory);
	}
    libqs_working_data.debug_log = 0;
	libqs_working_data.on_recv_http_request = 0;
	return 0;
}

UNITY_INTERFACE_EXPORT uint32_t UNITY_INTERFACE_API libqs_api_rand()
{
    libqs_api_debug_log("libqs_api_rand");
    return api_qs_rand();
}

UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_set_debug_log(libqs_api_callback_unity_debug_log callback)
{
    libqs_working_data.debug_log = callback;
}

UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_set_on_recv_http_request(libqs_api_callback_unity_on_recv_http_request callback)
{
	libqs_working_data.on_recv_http_request = callback;
}

void libqs_api_debug_log(const char* str)
{
    if (libqs_working_data.debug_log)
    {
        libqs_working_data.debug_log(str);
    }
}

