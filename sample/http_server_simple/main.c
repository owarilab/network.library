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
	QS_SERVER_CONTEXT* context = 0;
	if(0 > api_qs_server_init(&context,8080)){
		return -1;
	}
	if(-1==api_qs_server_create_router(context)){
		return -1;
	}
	if(-1==api_qs_server_create_kvs(context)){
		return -1;
	}
	if(-1==api_qs_server_create_logger_access(context,"./access_log.txt")){
		return -1;
	}
	if(-1==api_qs_server_create_logger_debug(context,"./debug_log.txt")){
		return -1;
	}
	if(-1==api_qs_server_create_logger_error(context,"./error_log.txt")){
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
	QS_SERVER_CONTEXT* context = api_qs_get_server_context(params);
	QS_MEMORY_CONTEXT memory;
	QS_JSON_ELEMENT_OBJECT object;

	// curl -X GET "http://localhost:8080/api/get_test?v1=v-1&v2=v-2&v3=v-3"
	if(!strcmp(api_qs_get_http_method(params),"GET")){
		if(!strcmp(api_qs_get_http_path(params),"/api/get_test")){
			char* v1 = api_qs_get_http_get_parameter(params,"v1");
			char* v2 = api_qs_get_http_get_parameter(params,"v2");
			char* v3 = api_qs_get_http_get_parameter(params,"v3");
			if(v1!=0&&v2!=0&&v3!=0){
				api_qs_memory_alloc(&memory,1024 * 1024);
				api_qs_object_create(&memory,&object);
				api_qs_object_push_string(&object,"v1",v1);
				api_qs_object_push_string(&object,"v2",v2);
				api_qs_object_push_string(&object,"v3",v3);
				http_status_code = api_qs_http_response_json(params,&object,1024*8);
				api_qs_memory_free(&memory);
			}
		}
	}

	// curl -X POST http://localhost:8080/api/v1/mem/set -d 'k=memk1&v=memvalue1'
	if(!strcmp(api_qs_get_http_method(params),"POST")){
		if(!strcmp(api_qs_get_http_path(params),"/api/v1/mem/set")){
			char* key = api_qs_get_http_post_parameter(params,"k");
			char* value = api_qs_get_http_post_parameter(params,"v");
			if(key!=0&&value!=0){
				QS_KVS_CONTEXT kvs;
				api_qs_memory_alloc(&memory,1024 * 1024 * 4);
				api_qs_server_get_kvs(context,&kvs);
				int result = api_qs_kvs_set(&kvs,key,value,0);
				api_qs_object_create(&memory,&object);
				api_qs_object_push_string(&object,"k",key);
				api_qs_object_push_string(&object,"v",value);
				api_qs_object_push_integer(&object,"result",result);
				http_status_code = api_qs_http_response_json(params,&object,1024 * 1024);
				api_qs_memory_free(&memory);
			}
		}
	}

	// curl -X POST http://localhost:8080/api/v1/mem/get -d 'k=memk1'
	if(!strcmp(api_qs_get_http_method(params),"POST")){
		if(!strcmp(api_qs_get_http_path(params),"/api/v1/mem/get")){
			char* key = api_qs_get_http_post_parameter(params,"k");
			if(key!=0){
				QS_KVS_CONTEXT kvs;
				api_qs_memory_alloc(&memory,1024 * 1024 * 4);
				api_qs_server_get_kvs(context,&kvs);
				char* value = api_qs_kvs_get(&kvs,key);
				api_qs_object_create(&memory,&object);
				api_qs_object_push_string(&object,"k",key);
				if(value==0){
					api_qs_object_push_string(&object,"v","");
				}else{
					api_qs_object_push_string(&object,"v",value);
				}
				http_status_code = api_qs_http_response_json(params,&object,1024 * 1024);
				api_qs_memory_free(&memory);
			}
		}
	}

	// curl -X POST http://localhost:8080/api/v1/mem/delete -d 'k=memk1'
	if(!strcmp(api_qs_get_http_method(params),"POST")){
		if(!strcmp(api_qs_get_http_path(params),"/api/v1/mem/delete")){
			char* key = api_qs_get_http_post_parameter(params,"k");
			if(key!=0){
				QS_KVS_CONTEXT kvs;
				api_qs_memory_alloc(&memory,1024 * 1024);
				api_qs_server_get_kvs(context,&kvs);
				api_qs_kvs_delete(&kvs,key);
				api_qs_object_create(&memory,&object);
				api_qs_object_push_string(&object,"k",key);
				http_status_code = api_qs_http_response_json(params,&object,1024*64);
				api_qs_memory_free(&memory);
			}
		}
	}

	// curl -X POST http://localhost:8080/api/v1/mem/keys
	if(!strcmp(api_qs_get_http_method(params),"POST")){
		if(!strcmp(api_qs_get_http_path(params),"/api/v1/mem/keys")){
			QS_KVS_CONTEXT kvs;
			QS_JSON_ELEMENT_ARRAY array;
			api_qs_memory_alloc(&memory,1024 * 1024);
			api_qs_server_get_kvs(context,&kvs);
			api_qs_object_create(&memory,&object);
			api_qs_array_create(&memory,&array);
			int32_t key_length = api_qs_kvs_keys(&array,&kvs);
			api_qs_object_push_integer(&object,"len",key_length);
			api_qs_object_push_array(&object,"keys",&array);
			http_status_code = api_qs_http_response_json(params,&object,1024*64);
			api_qs_memory_free(&memory);
		}
	}

	// curl -X POST http://localhost:8080/api/v1/room/create -d 'name=testroom1'
	if(!strcmp(api_qs_get_http_method(params),"POST")){
		if(!strcmp(api_qs_get_http_path(params),"/api/v1/room/create")){
			char* room_name = api_qs_get_http_post_parameter(params,"name");
			if(room_name!=0){
				api_qs_memory_alloc(&memory,1024 * 1024);
				if(0==api_qs_room_create(context,room_name,&memory,&object)){
					http_status_code = api_qs_http_response_json(params,&object,1024*8);
				}
				api_qs_memory_free(&memory);
			}
		}
	}

	// curl -X POST http://localhost:8080/api/v1/room/list
	if(!strcmp(api_qs_get_http_method(params),"POST")){
		if(!strcmp(api_qs_get_http_path(params),"/api/v1/room/list")){
			api_qs_memory_alloc(&memory,1024 * 1024);
			if(0==api_qs_room_list(context,&memory,&object)){
				http_status_code = api_qs_http_response_json(params,&object,1024*8);
			}
			api_qs_memory_free(&memory);
		}
	}

	// curl -X POST http://localhost:8080/api/v1/room/join -d 'room_id=WTeiylPnFIDeyPOnTnBU&connection_id=klV7RRk1vgpshZfdzzPjcC4PM7sDBCd8'
	if(!strcmp(api_qs_get_http_method(params),"POST")){
		if(!strcmp(api_qs_get_http_path(params),"/api/v1/room/join")){
			char* room_id = api_qs_get_http_post_parameter(params,"room_id");
			char* connection_id = api_qs_get_http_post_parameter(params,"connection_id");
			if(room_id!=0&&connection_id!=0){
				api_qs_memory_alloc(&memory,1024 * 1024);
				if(0==api_qs_room_join(context,room_id,connection_id,&memory,&object)){
					http_status_code = api_qs_http_response_json(params,&object,1024*8);
				} else{
					http_status_code = 404;
				}
				api_qs_memory_free(&memory);
			}
		}
	}

	return http_status_code;
}

int on_close(QS_EVENT_PARAMETER params)
{
	return 0;
}
