/*
 * Copyright (c) 2014-2025 Katsuya Owari
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

#ifndef _LIBQS_H_
#define _LIBQS_H_

#include <sys/types.h>
#include <inttypes.h>

// sudo apt install libssl-dev
#include <openssl/ssl.h>
#include <openssl/err.h>

#define QS_SSL_MODULE_PHASE_CONNECT 0
#define QS_SSL_MODULE_PHASE_READ_HEADER 1
#define QS_SSL_MODULE_PHASE_READ_BODY 2
#define QS_SSL_MODULE_PHASE_READ_CHUNKED_BODY 3
#define QS_SSL_MODULE_PHASE_DISCONNECT 4

#define QS_SERVER_TYPE_PLAIN 100
#define QS_SERVER_TYPE_SIMPLE 200
#define QS_SERVER_TYPE_HTTP 300

#define QS_KVS_MEMORY_TYPE_B1MB 0
#define QS_KVS_MEMORY_TYPE_B128MB 1
#define QS_KVS_MEMORY_TYPE_B256MB 2
#define QS_KVS_MEMORY_TYPE_B512MB 3
#define QS_KVS_MEMORY_TYPE_B1024MB 4

#define QS_EVENT_PARAMETER_TYPE_RECV 1
#define QS_EVENT_PARAMETER_TYPE_CONNECTION 2

typedef struct QS_EVENT_PARAMETER_STRUCT
{
    int32_t parameter_type;
    void* params;
} QS_EVENT_PARAMETER_STRUCT;

#define QS_EVENT_PARAMETER QS_EVENT_PARAMETER_STRUCT*
typedef int (*QS_EVENT_FUNCTION)( QS_EVENT_PARAMETER params );

typedef struct QS_MEMORY_CONTEXT
{
	void* memory;
} QS_MEMORY_CONTEXT;

typedef struct QS_KVS_CONTEXT
{
	int32_t is_persistence;
	int32_t memid_kvs;
	int32_t memid_kvs_memory;
	void* memory;
	char persistence_file_path[2048];
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
	QS_EVENT_FUNCTION on_plain_event;
	QS_EVENT_FUNCTION on_simple_event;
	QS_EVENT_FUNCTION on_http_event;
	QS_EVENT_FUNCTION on_ws_event;
	QS_EVENT_FUNCTION on_close;

	int32_t server_type;

	void* router_memory;
	int32_t memid_router;

	void* kvs_memory;
	int32_t memid_kvs_memory;
	int32_t memid_kvs;
} QS_SERVER_CONTEXT;

typedef struct QS_CLIENT_CONTEXT
{
	void* memory;
	int32_t memid_temporary_memory;
	int32_t memid_client;
	time_t current_time;
	time_t update_time;
	QS_EVENT_FUNCTION on_connect;
	QS_EVENT_FUNCTION on_plain_event;
	QS_EVENT_FUNCTION on_simple_event;
	QS_EVENT_FUNCTION on_close;
	void* client_data;
} QS_CLIENT_CONTEXT;

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

typedef struct QS_CSV_CONTEXT
{
	int32_t memid_csv;
	void* memory;
} QS_CSV_CONTEXT;

typedef struct QS_HTTP_CLIENT_CONTEXT
{
    SSL_CTX *ctx;
    SSL *ssl;
    QS_CLIENT_CONTEXT* client_context;
    char host[1024];
    char port[16];
    char request_buffer[1024 * 1024];
    char read_buffer[1024 * 1024];
    int socket;
    int is_ssl;
    
    // working data
    int phase;
	size_t body_length;
	size_t total_read_body_length;
    size_t temp_max_body_length;
	size_t temp_chunked_size;
	size_t temp_chunked_read_size;
	char header_buffer[1024 * 1024];
	char body_buffer[1024 * 1024 * 4];
    char* body_buffer_ptr;
    char chunk_size_buffer[32];
    int chunk_size_buffer_len;
    int waiting_for_chunk_trailer;
} QS_HTTP_CLIENT_CONTEXT;

int qs_ssl_module_http_client_connect(QS_HTTP_CLIENT_CONTEXT* context,const char* server_host, int server_port, int is_ssl);
SSL_CTX* qs_ssl_module_http_client_ssl_create_context();
SSL* qs_ssl_module_http_client_ssl_create(SSL_CTX* ctx, int sock);
int qs_ssl_module_http_client_update(QS_HTTP_CLIENT_CONTEXT* context);
int qs_ssl_module_http_client_recv(QS_HTTP_CLIENT_CONTEXT* context, char* payload, size_t payload_size);
int qs_ssl_module_http_client_free(QS_HTTP_CLIENT_CONTEXT* context);
int qs_ssl_module_http_client_get_header(QS_HTTP_CLIENT_CONTEXT* context, const char* key, char* value, size_t value_size);

int api_qs_init();
uint32_t api_qs_rand();
int api_qs_memory_alloc(QS_MEMORY_CONTEXT* context, size_t alloc_size);
int api_qs_memory_clean(QS_MEMORY_CONTEXT* context);
void api_qs_memory_info(QS_MEMORY_CONTEXT* context);
int api_qs_memory_free(QS_MEMORY_CONTEXT* context);
size_t api_qs_memory_available_size(QS_MEMORY_CONTEXT* context);
int api_qs_array_create(QS_MEMORY_CONTEXT* context, QS_JSON_ELEMENT_ARRAY* array);
int api_qs_array_push_integer(QS_JSON_ELEMENT_ARRAY* array,int32_t value);
int api_qs_array_push_big_integer(QS_JSON_ELEMENT_ARRAY* array,int64_t value);
int api_qs_array_push_unsigned_big_integer(QS_JSON_ELEMENT_ARRAY* array,uint64_t value);
int api_qs_array_push_string(QS_JSON_ELEMENT_ARRAY* array,const char* value);
int api_qs_array_push_object(QS_JSON_ELEMENT_ARRAY* array, QS_JSON_ELEMENT_OBJECT* object);
int api_qs_array_push_array(QS_JSON_ELEMENT_ARRAY* array, QS_JSON_ELEMENT_ARRAY* push_array);
int api_qs_object_create(QS_MEMORY_CONTEXT* context, QS_JSON_ELEMENT_OBJECT* object);
int api_qs_object_push_integer(QS_JSON_ELEMENT_OBJECT* object,const char* name,int32_t value);
int api_qs_object_push_big_integer(QS_JSON_ELEMENT_OBJECT* object,const char* name,int64_t value);
int api_qs_object_push_unsigned_big_integer(QS_JSON_ELEMENT_OBJECT* object,const char* name,uint64_t value);
int api_qs_object_push_string(QS_JSON_ELEMENT_OBJECT* object,const char* name,const char* value);
int api_qs_object_push_array(QS_JSON_ELEMENT_OBJECT* object,const char* name,QS_JSON_ELEMENT_ARRAY* array);
int api_qs_object_push_object(QS_JSON_ELEMENT_OBJECT* object,const char* name,QS_JSON_ELEMENT_OBJECT* push_object);
char* api_qs_json_encode_object(QS_JSON_ELEMENT_OBJECT* object,size_t buffer_size);
char* api_qs_json_encode_array(QS_JSON_ELEMENT_ARRAY* array,size_t buffer_size);

int api_qs_json_decode_object(QS_MEMORY_CONTEXT* context, QS_JSON_ELEMENT_OBJECT* object, const char* json);
int api_qs_object_exist(QS_JSON_ELEMENT_OBJECT* object,const char* name);
int32_t* api_qs_object_get_integer(QS_JSON_ELEMENT_OBJECT* object,const char* name);
int64_t* api_qs_object_get_big_integer(QS_JSON_ELEMENT_OBJECT* object,const char* name);
uint64_t* api_qs_object_get_unsigned_big_integer(QS_JSON_ELEMENT_OBJECT* object,const char* name);

int32_t api_qs_object_get_integer_val(QS_JSON_ELEMENT_OBJECT* object,const char* name);
int64_t api_qs_object_get_big_integer_val(QS_JSON_ELEMENT_OBJECT* object,const char* name);
uint64_t api_qs_object_get_unsigned_big_integer_val(QS_JSON_ELEMENT_OBJECT* object,const char* name);

char* api_qs_object_get_string(QS_JSON_ELEMENT_OBJECT* object,const char* name);
int api_qs_object_get_array(QS_JSON_ELEMENT_OBJECT* object,const char* name,QS_JSON_ELEMENT_ARRAY* dst_array);
int api_qs_object_get_object(QS_JSON_ELEMENT_OBJECT* object,const char* name,QS_JSON_ELEMENT_OBJECT* dst_object);
int api_qs_object_get_keys(QS_JSON_ELEMENT_OBJECT* object,QS_JSON_ELEMENT_ARRAY* dst_array);
int32_t api_qs_array_get_length(QS_JSON_ELEMENT_ARRAY* object);
int32_t* api_qs_array_get_integer(QS_JSON_ELEMENT_ARRAY* object,int32_t offset);
int64_t* api_qs_array_get_big_integer(QS_JSON_ELEMENT_ARRAY* object,int32_t offset);
uint64_t* api_qs_array_get_unsigned_big_integer(QS_JSON_ELEMENT_ARRAY* object,int32_t offset);
char* api_qs_array_get_string(QS_JSON_ELEMENT_ARRAY* object,int32_t offset);
int api_qs_array_get_array(QS_JSON_ELEMENT_ARRAY* object,int32_t offset,QS_JSON_ELEMENT_ARRAY* dst_array);
int api_qs_array_get_object(QS_JSON_ELEMENT_ARRAY* object,int32_t offset,QS_JSON_ELEMENT_OBJECT* dst_object);


int api_qs_csv_read_file(QS_MEMORY_CONTEXT* context, QS_CSV_CONTEXT* csv, const char* csv_file_path);
int api_qs_csv_parse(QS_MEMORY_CONTEXT* context, QS_CSV_CONTEXT* csv, const char * src_csv);
int32_t api_qs_csv_get_line_length(QS_CSV_CONTEXT* csv);
int32_t api_qs_csv_get_row_length(QS_CSV_CONTEXT* csv, int32_t line_pos);
char* api_qs_csv_get_row(QS_CSV_CONTEXT* csv, int32_t line_pos, int32_t row_pos);

int api_qs_client_init(QS_CLIENT_CONTEXT** ppcontext, const char* host, int port, int32_t server_type);
int api_qs_client_get_socket(QS_CLIENT_CONTEXT* context);
void api_qs_set_client_on_connect_event(QS_CLIENT_CONTEXT* context, QS_EVENT_FUNCTION on_connect );
void api_qs_set_client_on_plain_event(QS_CLIENT_CONTEXT* context, QS_EVENT_FUNCTION on_plain_event );
void api_qs_set_client_on_simple_event(QS_CLIENT_CONTEXT* context, QS_EVENT_FUNCTION on_simple_event );
void api_qs_set_client_on_close_event(QS_CLIENT_CONTEXT* context, QS_EVENT_FUNCTION on_close );
void api_qs_client_update(QS_CLIENT_CONTEXT* context);
void api_qs_client_sleep(QS_CLIENT_CONTEXT* context);
QS_CLIENT_CONTEXT* api_qs_client_get_context(QS_EVENT_PARAMETER params);
int api_qs_client_send(QS_CLIENT_CONTEXT* context, const char* payload, size_t payload_len);
int api_qs_client_send_message(QS_CLIENT_CONTEXT* context,uint32_t payload_type, const char* payload, size_t payload_len);
void api_qs_client_free(QS_CLIENT_CONTEXT* context);


int api_qs_server_init(QS_SERVER_CONTEXT** ppcontext, int port, int32_t max_connection, int32_t server_type);
int api_qs_server_get_socket(QS_SERVER_CONTEXT* context);
void api_qs_set_server_session_timeout(QS_SERVER_CONTEXT* context, int32_t timeout);
int api_qs_server_create_router(QS_SERVER_CONTEXT* context);
void api_qs_router_memory_info(QS_SERVER_CONTEXT* context);
int api_qs_server_create_kvs(QS_SERVER_CONTEXT* context, int kvs_memory_type);
int api_qs_server_get_kvs(QS_SERVER_CONTEXT* context,QS_KVS_CONTEXT* kvs_context);
void api_qs_set_on_connect_event(QS_SERVER_CONTEXT* context, QS_EVENT_FUNCTION on_connect );
void api_qs_set_on_plain_event(QS_SERVER_CONTEXT* context, QS_EVENT_FUNCTION on_plain_event );
void api_qs_set_on_simple_event(QS_SERVER_CONTEXT* context, QS_EVENT_FUNCTION on_simple_event );
void api_qs_set_on_http_event(QS_SERVER_CONTEXT* context, QS_EVENT_FUNCTION on_http_event );
void api_qs_set_on_websocket_event(QS_SERVER_CONTEXT* context, QS_EVENT_FUNCTION on_ws_event );
void api_qs_set_on_close_event(QS_SERVER_CONTEXT* context, QS_EVENT_FUNCTION on_close );
int api_qs_server_create_logger_access(QS_SERVER_CONTEXT* context,const char* log_file_path);
int api_qs_server_create_logger_debug(QS_SERVER_CONTEXT* context,const char* log_file_path);
int api_qs_server_create_logger_error(QS_SERVER_CONTEXT* context,const char* log_file_path);
void api_qs_update(QS_SERVER_CONTEXT* context);
void api_qs_sleep(QS_SERVER_CONTEXT* context);
void api_qs_free(QS_SERVER_CONTEXT* context);

char* api_qs_get_ws_message(QS_EVENT_PARAMETER params);
int api_qs_send_ws_message(QS_EVENT_PARAMETER params,const char* message);
int api_qs_send_ws_message_plane(QS_EVENT_PARAMETER params,const char* message);

char* api_qs_get_http_method(QS_EVENT_PARAMETER params);
char* api_qs_get_http_path(QS_EVENT_PARAMETER params);
char* api_qs_get_http_get_parameter(QS_EVENT_PARAMETER params, const char* name);
char* api_qs_get_http_post_parameter(QS_EVENT_PARAMETER params, const char* name);
char* api_qs_get_http_post_body(QS_EVENT_PARAMETER params);
void api_qs_get_http_post_json_object(QS_EVENT_PARAMETER params, QS_JSON_ELEMENT_OBJECT* object);

void api_qs_send_response(QS_EVENT_PARAMETER params, const char* response);
void api_qs_send_response_with_payload(QS_EVENT_PARAMETER params, uint32_t payload_type, const char* payload);

uint32_t api_qs_get_plain_payload_type(QS_EVENT_PARAMETER params);
uint8_t* api_qs_get_plain_payload(QS_EVENT_PARAMETER params);
size_t api_qs_get_plain_payload_length(QS_EVENT_PARAMETER params);

QS_SERVER_CONTEXT* api_qs_get_server_context(QS_EVENT_PARAMETER params);
uint32_t api_qs_get_connection_offset(QS_EVENT_PARAMETER params);
time_t api_qs_get_connection_create_time(QS_EVENT_PARAMETER params);
void api_qs_send_response_by_connection_offset(QS_SERVER_CONTEXT* context, uint32_t connection_offset, const char* response);
int api_qs_script_read_file(QS_MEMORY_CONTEXT* memory_context, QS_SERVER_SCRIPT_CONTEXT* script_context,const char* file_path);
int api_qs_script_set_argv_object(QS_SERVER_SCRIPT_CONTEXT* script_context,const char* name, QS_JSON_ELEMENT_OBJECT* object);
int api_qs_script_set_argv_string(QS_SERVER_SCRIPT_CONTEXT* script_context,const char* name, const char* value);
int api_qs_script_set_argv_integer(QS_SERVER_SCRIPT_CONTEXT* script_context,const char* name, int32_t value);
int api_qs_script_run(QS_SERVER_SCRIPT_CONTEXT* script_context);
char* api_qs_script_get_parameter(QS_SERVER_SCRIPT_CONTEXT* script_context, const char* name);

int api_qs_kvs_create_b1mb(QS_MEMORY_CONTEXT* memory_context, QS_KVS_CONTEXT* kvs_context);
int api_qs_kvs_create_b8mb(QS_MEMORY_CONTEXT* memory_context, QS_KVS_CONTEXT* kvs_context);
int api_qs_kvs_create_b1mb_persistence(QS_KVS_CONTEXT* kvs_context, const char* file_path);
int api_qs_kvs_create_b8mb_persistence(QS_KVS_CONTEXT* kvs_context, const char* file_path);
int api_qs_kvs_set(QS_KVS_CONTEXT* kvs_context,const char* key, const char* value, int32_t life_time);
char* api_qs_kvs_get(QS_KVS_CONTEXT* kvs_context,const char* key);
size_t api_qs_kvs_get_buffer_size(QS_KVS_CONTEXT* kvs_context,const char* key);
int api_qs_kvs_delete(QS_KVS_CONTEXT* kvs_context,const char* key);
int32_t api_qs_kvs_keys(QS_JSON_ELEMENT_ARRAY* array, QS_KVS_CONTEXT* kvs_context);
int32_t api_qs_kvs_sorted_keys(QS_JSON_ELEMENT_ARRAY* array, QS_KVS_CONTEXT* kvs_context, int32_t is_sort_asc);
int api_qs_persistence_kvs_memory_free(QS_KVS_CONTEXT* kvs_context);

int api_qs_room_create(QS_SERVER_CONTEXT* context, const char* name, QS_MEMORY_CONTEXT* dest_memory, QS_JSON_ELEMENT_OBJECT* dest_object);
int api_qs_room_list(QS_SERVER_CONTEXT* context, QS_MEMORY_CONTEXT* dest_memory, QS_JSON_ELEMENT_OBJECT* dest_object);
int api_qs_room_join(QS_SERVER_CONTEXT* context, const char* room_id, const char* connection_id, QS_MEMORY_CONTEXT* dest_memory, QS_JSON_ELEMENT_OBJECT* dest_object);
int api_qs_http_response_json(QS_EVENT_PARAMETER params, QS_JSON_ELEMENT_OBJECT* object, size_t buffer_size);
int api_qs_http_response_raw_json(QS_EVENT_PARAMETER params, QS_MEMORY_CONTEXT* temporary_memory,char* json);

char* api_qs_uniqid(QS_MEMORY_CONTEXT* memory_context, int32_t length);
char* api_qs_base64_encode(QS_MEMORY_CONTEXT* memory_context, const void* data, size_t length);
char* api_qs_base64_decode(QS_MEMORY_CONTEXT* memory_context, const char* data);
char* api_qs_sha1_encode(QS_MEMORY_CONTEXT* memory_context, const void* data, size_t length);

#endif /*_LIBQS_H_*/

#ifdef __cplusplus
}
#endif
