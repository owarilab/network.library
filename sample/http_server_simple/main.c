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
#include "libqs_api.h"

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
	int32_t max_connection = 100;
	if(0 > api_qs_server_init(&context,server_port,max_connection)){return -1;}
	if(-1==api_qs_server_create_router(context)){return -1;}
	if(-1==api_qs_server_create_kvs(context,QS_KVS_MEMORY_TYPE_B1MB)){return -1;}
	if(-1==api_qs_server_create_logger_access(context,"./access_log.txt")){return -1;}
	if(-1==api_qs_server_create_logger_debug(context,"./debug_log.txt")){return -1;}
	if(-1==api_qs_server_create_logger_error(context,"./error_log.txt")){return -1;}
	api_qs_set_on_connect_event(context, on_connect );
	api_qs_set_on_http_event(context, on_http_event );
	api_qs_set_on_websocket_event(context, on_ws_event );
	api_qs_set_on_close_event(context, on_close );

	// test
	if(0)
	{
		for(int i=0;i<1000;i++){
			char* room_name = api_qs_uniqid(&g_temporary_memory,32);
			QS_JSON_ELEMENT_OBJECT object;
			api_qs_room_create(context,room_name,&g_temporary_memory,&object);
			char* json = api_qs_json_encode_object(&object,1024);
			printf("room_info : %s\n",json);
			api_qs_memory_clean(&g_temporary_memory);
		}
		api_qs_router_memory_info(context);
	}

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
			QS_JSON_ELEMENT_OBJECT object;
			api_qs_json_decode_object(&g_temporary_memory,&object,api_qs_get_http_post_body(params));
			QS_JSON_ELEMENT_ARRAY arr1;
			QS_JSON_ELEMENT_ARRAY arr2;
			api_qs_object_get_array(&object,"arr1",&arr1);
			api_qs_object_get_array(&object,"arr2",&arr2);
			printf("arr1 length : %d\n",api_qs_array_get_length(&arr1));
			printf("arr2 length : %d\n",api_qs_array_get_length(&arr2));
			int i;
			printf("<<<show arr1\n");
			for(i=0;i<api_qs_array_get_length(&arr1);i++){
				QS_JSON_ELEMENT_OBJECT tmpobj;
				api_qs_array_get_object(&arr1,i,&tmpobj);
				int32_t* id = api_qs_object_get_integer(&tmpobj,"id");
				printf("id : %d\n",*id);
				printf("value : %s\n",api_qs_object_get_string(&tmpobj,"value"));
			}
			printf("<<<show arr2\n");
			for(i=0;i<api_qs_array_get_length(&arr2);i++){
				QS_JSON_ELEMENT_OBJECT tmpobj;
				api_qs_array_get_object(&arr2,i,&tmpobj);
				int32_t* id = api_qs_object_get_integer(&tmpobj,"id");
				printf("id : %d\n",*id);
				printf("value : %s\n",api_qs_object_get_string(&tmpobj,"value"));
			}
		}
	}

	return http_status_code;
}

int on_ws_event(QS_EVENT_PARAMETER params)
{
	char* message = api_qs_get_ws_message(params);
	api_qs_send_ws_message(params,message);
	//api_qs_send_ws_message(params,"Hello World!");
	//api_qs_send_ws_message_plane(params,message);
	//api_qs_send_ws_message_plane(params,"Hello World!");
	return 0;
}

int on_close(QS_EVENT_PARAMETER params)
{
	return 0;
}
