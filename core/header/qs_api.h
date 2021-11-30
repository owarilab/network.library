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

#ifdef __cplusplus
extern "C"{
#endif

#ifndef _QS_API_H_
#define _QS_API_H_

#include "qs_array.h"
#include "qs_hash.h"
#include "qs_socket.h"
#include "qs_protocol.h"
#include "qs_variable.h"
#include "qs_script.h"
#include "qs_packet_route.h"
#include "qs_random.h"

#define QS_EVENT_PARAMETER void*
typedef int (*QS_EVENT_FUNCTION)( QS_EVENT_PARAMETER params );

typedef struct QS_MEMORY_CONTEXT
{
	void* memory;
} QS_MEMORY_CONTEXT;

typedef struct QS_KVS_CONTEXT
{
	int32_t memid_kvs;
	int32_t memid_kvs_memory;
	void* memory;
} QS_KVS_CONTEXT;

typedef struct QS_SERVER_CONTEXT
{
	void* memory;
	void* system_data;
	int32_t memid_temporary_memory;
	int32_t memid_scheduler;
	int32_t memid_server;
	time_t current_time;
	time_t update_time;
	QS_EVENT_FUNCTION on_connect;
	QS_EVENT_FUNCTION on_recv;
	QS_EVENT_FUNCTION on_close;

	void* router_memory;
	int32_t memid_router;
} QS_SERVER_CONTEXT;

typedef struct QS_SERVER_SCRIPT_CONTEXT
{
	int32_t memid_script;
	void* memory;
} QS_SERVER_SCRIPT_CONTEXT;

typedef struct QS_JSON_ELEMENT_ARRAY
{
	int32_t memid_array;
	void* memory;
} QS_JSON_ELEMENT_ARRAY;

typedef struct QS_JSON_ELEMENT_OBJECT
{
	int32_t memid_object;
	void* memory;
} QS_JSON_ELEMENT_OBJECT;

// private api
ssize_t api_qs_make_ws_message_common(QS_MEMORY_POOL * temporary_memory,char* connection_id,char* type,char* message,void* buffer,size_t buffer_size);

// public api
int api_qs_init();
int api_qs_memory_alloc(QS_MEMORY_CONTEXT* context, size_t alloc_size);
int api_qs_memory_clean(QS_MEMORY_CONTEXT* context);
void api_qs_memory_info(QS_MEMORY_CONTEXT* context);
int api_qs_memory_free(QS_MEMORY_CONTEXT* context);
int api_qs_array_create(QS_MEMORY_CONTEXT* context, QS_JSON_ELEMENT_ARRAY* array);
int api_qs_array_push_integer(QS_JSON_ELEMENT_ARRAY* array,int32_t value);
int api_qs_array_push_string(QS_JSON_ELEMENT_ARRAY* array,const char* value);
int api_qs_object_create(QS_MEMORY_CONTEXT* context, QS_JSON_ELEMENT_OBJECT* object);
int api_qs_object_push_integer(QS_JSON_ELEMENT_OBJECT* object,const char* name,int32_t value);
int api_qs_object_push_string(QS_JSON_ELEMENT_OBJECT* object,const char* name,const char* value);
int api_qs_object_push_array(QS_JSON_ELEMENT_OBJECT* object,const char* name,QS_JSON_ELEMENT_ARRAY* array);
char* api_qs_json_encode_object(QS_JSON_ELEMENT_OBJECT* object,size_t buffer_size);

int api_qs_server_init(QS_SERVER_CONTEXT** ppcontext, int port);
int api_qs_server_create_router(QS_SERVER_CONTEXT* context);
void api_qs_set_on_connect_event(QS_SERVER_CONTEXT* context, QS_EVENT_FUNCTION on_connect );
void api_qs_set_on_packet_recv_event(QS_SERVER_CONTEXT* context, QS_EVENT_FUNCTION on_recv );
void api_qs_set_on_close_event(QS_SERVER_CONTEXT* context, QS_EVENT_FUNCTION on_close );
void api_qs_update(QS_SERVER_CONTEXT* context);
void api_qs_sleep(QS_SERVER_CONTEXT* context);
void api_qs_free(QS_SERVER_CONTEXT* context);
void api_qs_on_recv( void* params );
void api_qs_exec_http(QS_RECV_INFO *rinfo);
void api_qs_exec_websocket(QS_RECV_INFO *rinfo);
int api_qs_core_on_connect(QS_SERVER_CONNECTION_INFO* connection);
void* api_qs_core_on_recv( void* args );
int api_qs_core_on_close(QS_SERVER_CONNECTION_INFO* connection);

char* api_qs_get_http_method(QS_EVENT_PARAMETER params);
char* api_qs_get_http_path(QS_EVENT_PARAMETER params);
char* api_qs_get_http_get_parameter(QS_EVENT_PARAMETER params, const char* name);
char* api_qs_get_http_post_parameter(QS_EVENT_PARAMETER params, const char* name);
void api_qs_send_response(QS_EVENT_PARAMETER params, const char* response);

QS_SERVER_CONTEXT* api_qs_get_server_context(QS_EVENT_PARAMETER params);
int api_qs_script_init(QS_MEMORY_CONTEXT* memory_context, QS_SERVER_SCRIPT_CONTEXT* script_context,const char* file_path);
int api_qs_script_run(QS_SERVER_SCRIPT_CONTEXT* script_context);
char* api_qs_script_get_parameter(QS_SERVER_SCRIPT_CONTEXT* script_context, const char* name);

int api_qs_kvs_create_b1mb(QS_MEMORY_CONTEXT* memory_context, QS_KVS_CONTEXT* kvs_context);
int api_qs_kvs_set(QS_KVS_CONTEXT* kvs_context,const char* key, const char* value, int32_t life_time);
char* api_qs_kvs_get(QS_KVS_CONTEXT* kvs_context,const char* key);
int api_qs_kvs_keys(QS_JSON_ELEMENT_ARRAY* array, QS_KVS_CONTEXT* kvs_context);

int api_qs_room_create(QS_SERVER_CONTEXT* context, const char* name);
int api_qs_room_list(QS_SERVER_CONTEXT* context, QS_MEMORY_CONTEXT* dest_memory, QS_JSON_ELEMENT_OBJECT* dest_object);

#endif /*_QS_API_H_*/

#ifdef __cplusplus
}
#endif
