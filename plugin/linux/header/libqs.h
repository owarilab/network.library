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

#define QS_SSL_MODULE_ENABLED 1

# ifdef QS_SSL_MODULE_ENABLED
// sudo apt install libssl-dev
#include <openssl/ssl.h>
#include <openssl/err.h>
# endif

#define QS_SSL_MODULE_PHASE_CONNECT 0
#define QS_SSL_MODULE_PHASE_READ_HEADER 1
#define QS_SSL_MODULE_PHASE_READ_BODY 2
#define QS_SSL_MODULE_PHASE_READ_CHUNKED_BODY 3
#define QS_SSL_MODULE_PHASE_DISCONNECT 4

#define QS_HTTP_CLIENT_HOST_SIZE 1024
#define QS_HTTP_CLIENT_PORT_SIZE 16
#define QS_HTTP_CLIENT_REQUEST_BUFFER_SIZE (1024 * 1024)
#define QS_HTTP_CLIENT_READ_BUFFER_SIZE (1024 * 1024)
#define QS_HTTP_CLIENT_TRANSIENT_MEMORY_SIZE (1024 * 1024 * 3)
#define QS_HTTP_CLIENT_HEADER_BUFFER_SIZE (1024 * 1024)
#define QS_HTTP_CLIENT_BODY_BUFFER_SIZE (1024 * 1024 * 4)
#define QS_HTTP_CLIENT_CHUNK_SIZE_BUFFER_SIZE 32
#define QS_HTTP_CLIENT_RESPONSE_MEMORY_SIZE (QS_HTTP_CLIENT_HEADER_BUFFER_SIZE + QS_HTTP_CLIENT_BODY_BUFFER_SIZE + 4096)

/*
 * Server protocol/runtime type used by server/client initialization APIs.
 * - PLAIN  : raw payload mode
 * - SIMPLE : framed simple protocol mode
 * - HTTP   : HTTP/WebSocket server mode
 */
#define QS_SERVER_TYPE_PLAIN 100
#define QS_SERVER_TYPE_SIMPLE 200
#define QS_SERVER_TYPE_HTTP 300

/*
 * Update scheduler preset used by api_qs_set_scheduler().
 * Higher modes update more aggressively, lower modes reduce update frequency.
 */
#define QS_SCHEDULER_MODE_HIGH 1
#define QS_SCHEDULER_MODE_MIDDLE 2
#define QS_SCHEDULER_MODE_LOW 3

/*
 * Built-in KVS memory size preset used by api_qs_server_create_kvs().
 * The suffix indicates the target cache area size.
 */
#define QS_KVS_MEMORY_TYPE_B1MB 0
#define QS_KVS_MEMORY_TYPE_B128MB 1
#define QS_KVS_MEMORY_TYPE_B256MB 2
#define QS_KVS_MEMORY_TYPE_B512MB 3
#define QS_KVS_MEMORY_TYPE_B1024MB 4

/*
 * Event payload source type passed through QS_EVENT_PARAMETER.
 * Handlers use this to determine whether params refers to receive data or a connection.
 */
#define QS_EVENT_PARAMETER_TYPE_RECV 1
#define QS_EVENT_PARAMETER_TYPE_CONNECTION 2

/*
 * Event callback input wrapper.
 * parameter_type identifies whether params points to receive data or connection data.
 * User event handlers receive this through QS_EVENT_FUNCTION.
 */
typedef struct QS_EVENT_PARAMETER_STRUCT
{
    int32_t parameter_type;
    void* params;
} QS_EVENT_PARAMETER_STRUCT;

#define QS_EVENT_PARAMETER QS_EVENT_PARAMETER_STRUCT*
typedef int (*QS_EVENT_FUNCTION)( QS_EVENT_PARAMETER params );

/*
 * Generic memory-pool context used by memory, JSON, CSV, and helper APIs.
 * Create it with api_qs_memory_alloc(), use it for pool-based allocations, then
 * reset with api_qs_memory_clean() or release with api_qs_memory_free().
 */
typedef struct QS_MEMORY_CONTEXT
{
	void* memory;
} QS_MEMORY_CONTEXT;

/*
 * Key-value store context.
 * Obtain and use this when working with the built-in KVS APIs, including optional
 * persistence-backed storage.
 */
typedef struct QS_KVS_CONTEXT
{
	int32_t is_persistence;
	int32_t memid_kvs;
	int32_t memid_kvs_memory;
	void* memory;
	char persistence_file_path[2048];
} QS_KVS_CONTEXT;

/*
 * Server runtime context returned by api_qs_server_init().
 * Holds the server memory pool, event handlers, timing state, and optional router
 * and KVS resources used while running a server.
 */
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

	// websocket binary support
	uint8_t ws_opcode;		// last received WS opcode (1:text, 2:binary)
	ssize_t ws_message_size;	// last received WS payload size in bytes
} QS_SERVER_CONTEXT;

/*
 * Client runtime context returned by api_qs_client_init().
 * Holds the client memory pool, event handlers, timing state, and optional
 * application-specific client_data.
 */
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

/*
 * Script execution context used by the script APIs.
 * It binds one loaded script instance to the memory pool that owns its data.
 */
typedef struct QS_SERVER_SCRIPT_CONTEXT
{
	int32_t memid_script;
	void* memory;
} QS_SERVER_SCRIPT_CONTEXT;

/*
 * Handle for one JSON array stored inside a QS memory pool.
 * Create it with api_qs_array_create() and pass it to array/JSON helper APIs.
 */
typedef struct QS_JSON_ELEMENT_ARRAY
{
	int32_t memid_array;
	void* memory;
} QS_JSON_ELEMENT_ARRAY;

/*
 * Handle for one JSON object stored inside a QS memory pool.
 * Create or decode it, then use object/JSON helper APIs to access properties.
 */
typedef struct QS_JSON_ELEMENT_OBJECT
{
	int32_t memid_object;
	void* memory;
} QS_JSON_ELEMENT_OBJECT;

/*
 * CSV parse/load context stored in a QS memory pool.
 * Fill it with api_qs_csv_read_file() or api_qs_csv_parse(), then read rows
 * through the CSV accessor APIs.
 */
typedef struct QS_CSV_CONTEXT
{
	int32_t memid_csv;
	void* memory;
} QS_CSV_CONTEXT;

typedef struct QS_HTTP_CLIENT_CONTEXT
{
# ifdef QS_SSL_MODULE_ENABLED
    SSL_CTX *ctx;
    SSL *ssl;
# endif
    QS_CLIENT_CONTEXT* client_context;
	QS_MEMORY_CONTEXT transient_memory_context;
	QS_MEMORY_CONTEXT response_memory_context;
	int32_t memid_host;
	int32_t memid_port;
	int32_t memid_request_buffer;
	int32_t memid_read_buffer;
	int32_t memid_header_buffer;
	int32_t memid_body_buffer;
	int32_t memid_chunk_size_buffer;
    int socket;
    int is_ssl;
    
    // working data
    int phase;
	size_t body_length;
	size_t total_read_body_length;
    size_t temp_max_body_length;
	size_t temp_chunked_size;
	size_t temp_chunked_read_size;
	size_t body_write_offset;
    int chunk_size_buffer_len;
    int waiting_for_chunk_trailer;
} QS_HTTP_CLIENT_CONTEXT;

int qs_ssl_module_http_client_connect(QS_HTTP_CLIENT_CONTEXT* context,const char* server_host, int server_port, int is_ssl);
# ifdef QS_SSL_MODULE_ENABLED
SSL_CTX* qs_ssl_module_http_client_ssl_create_context();
SSL* qs_ssl_module_http_client_ssl_create(SSL_CTX* ctx, int sock);
# endif
int qs_ssl_module_http_client_update(QS_HTTP_CLIENT_CONTEXT* context);
int qs_ssl_module_http_client_recv(QS_HTTP_CLIENT_CONTEXT* context, char* payload, size_t payload_size);
int qs_ssl_module_http_client_free(QS_HTTP_CLIENT_CONTEXT* context);
int qs_ssl_module_http_client_dispose(QS_HTTP_CLIENT_CONTEXT* context);
int qs_ssl_module_http_client_get_header(QS_HTTP_CLIENT_CONTEXT* context, const char* key, char* value, size_t value_size);
char* qs_ssl_module_http_client_get_request_buffer(QS_HTTP_CLIENT_CONTEXT* context);
const char* qs_ssl_module_http_client_get_header_buffer(QS_HTTP_CLIENT_CONTEXT* context);
const char* qs_ssl_module_http_client_get_body_buffer(QS_HTTP_CLIENT_CONTEXT* context);

/*
 * Initialize internal library state.
 * Currently this seeds the 32-bit and 64-bit random number generators.
 */
int api_qs_init();

/*
 * Return one pseudo-random 32-bit unsigned integer.
 * Call api_qs_init() before using this function.
 */
uint32_t api_qs_rand();

/*
 * Allocate a memory pool and bind it to the context.
 * alloc_size specifies the total pool size in bytes.
 */
int api_qs_memory_alloc(QS_MEMORY_CONTEXT* context, size_t alloc_size);

/*
 * Reset the memory pool to its initial reusable state.
 * This clears allocations managed by the pool, but does not release the pool itself.
 */
int api_qs_memory_clean(QS_MEMORY_CONTEXT* context);

/*
 * Print debug information about the current memory pool usage.
 */
void api_qs_memory_info(QS_MEMORY_CONTEXT* context);

/*
 * Release the memory pool itself.
 * Call this when the context is no longer needed.
 */
int api_qs_memory_free(QS_MEMORY_CONTEXT* context);

/*
 * Return currently available bytes in the memory pool.
 */
size_t api_qs_memory_available_size(QS_MEMORY_CONTEXT* context);

/*
 * Create one allocation block inside the memory pool and return its memid.
 */
int32_t api_qs_memory_create_block(QS_MEMORY_CONTEXT* context, size_t size);

/*
 * Get a pointer to the start address of a block identified by memid.
 */
void* api_qs_memory_get_pointer(QS_MEMORY_CONTEXT* context, int32_t memid);

/*
 * Get a pointer inside a block using element-based offset access.
 * The internal address calculation is: base + (size * offset).
 *
 * Typical usage:
 * - size   : size of one element (for example sizeof(MyStruct))
 * - offset : element index (0, 1, 2, ...), not a raw byte offset
 *
 * This is useful when one block stores a fixed-size array such as structs.
 */
void* api_qs_memory_get_offset_pointer(QS_MEMORY_CONTEXT* context, int32_t memid, size_t size, int32_t offset);

/*
 * Return the allocated size of a block in bytes.
 */
size_t api_qs_memory_get_size(QS_MEMORY_CONTEXT* context, int32_t memid);

/*
 * Free one allocation block inside the memory pool.
 * This does not release the memory pool itself.
 */
int api_qs_memory_free_block(QS_MEMORY_CONTEXT* context, int32_t* memid);

/*
 * Create a JSON-style array in the same memory pool as context.
 * On success, array->memory is bound to the pool and array->memid_array is set.
 */
int api_qs_array_create(QS_MEMORY_CONTEXT* context, QS_JSON_ELEMENT_ARRAY* array);

/*
 * Append a 32-bit signed integer element to the array.
 * Returns 0 on success, -1 on failure.
 */
int api_qs_array_push_integer(QS_JSON_ELEMENT_ARRAY* array,int32_t value);

/*
 * Append a 64-bit signed integer element to the array.
 */
int api_qs_array_push_big_integer(QS_JSON_ELEMENT_ARRAY* array,int64_t value);

/*
 * Append a 64-bit unsigned integer element to the array.
 */
int api_qs_array_push_unsigned_big_integer(QS_JSON_ELEMENT_ARRAY* array,uint64_t value);

/*
 * Append a string element to the array.
 * The stored string is managed inside the same memory pool.
 */
int api_qs_array_push_string(QS_JSON_ELEMENT_ARRAY* array,const char* value);

/*
 * Append an object element to the array.
 * array and object must belong to the same memory pool.
 */
int api_qs_array_push_object(QS_JSON_ELEMENT_ARRAY* array, QS_JSON_ELEMENT_OBJECT* object);

/*
 * Append another array as a nested array element.
 * Both arrays must belong to the same memory pool.
 */
int api_qs_array_push_array(QS_JSON_ELEMENT_ARRAY* array, QS_JSON_ELEMENT_ARRAY* push_array);

/*
 * Create a JSON-style object in the same memory pool as context.
 * On success, object->memory is bound to the pool and object->memid_object is set.
 */
int api_qs_object_create(QS_MEMORY_CONTEXT* context, QS_JSON_ELEMENT_OBJECT* object);

/*
 * Set a 32-bit signed integer property on the object.
 * Returns 0 on success, -1 on failure.
 */
int api_qs_object_push_integer(QS_JSON_ELEMENT_OBJECT* object,const char* name,int32_t value);

/*
 * Set a 64-bit signed integer property on the object.
 */
int api_qs_object_push_big_integer(QS_JSON_ELEMENT_OBJECT* object,const char* name,int64_t value);

/*
 * Set a 64-bit unsigned integer property on the object.
 */
int api_qs_object_push_unsigned_big_integer(QS_JSON_ELEMENT_OBJECT* object,const char* name,uint64_t value);

/*
 * Set a string property on the object.
 * The stored string is managed inside the same memory pool.
 */
int api_qs_object_push_string(QS_JSON_ELEMENT_OBJECT* object,const char* name,const char* value);

/*
 * Set an array property on the object.
 * The array is referenced from the same memory pool.
 */
int api_qs_object_push_array(QS_JSON_ELEMENT_OBJECT* object,const char* name,QS_JSON_ELEMENT_ARRAY* array);

/*
 * Set a nested object property on the object.
 * The nested object is referenced from the same memory pool.
 */
int api_qs_object_push_object(QS_JSON_ELEMENT_OBJECT* object,const char* name,QS_JSON_ELEMENT_OBJECT* push_object);

/*
 * Encode an object as JSON text in the object's memory pool.
 * The returned pointer is pool-owned and remains valid until the pool is cleaned or freed.
 * buffer_size is the temporary/output buffer size hint used by the encoder.
 */
char* api_qs_json_encode_object(QS_JSON_ELEMENT_OBJECT* object,size_t buffer_size);

/*
 * Encode an array as JSON text in the array's memory pool.
 * The returned pointer is pool-owned and remains valid until the pool is cleaned or freed.
 */
char* api_qs_json_encode_array(QS_JSON_ELEMENT_ARRAY* array,size_t buffer_size);

/*
 * Decode JSON text and bind the root object to the provided context memory pool.
 * This function succeeds only when the JSON root is an object.
 */
int api_qs_json_decode_object(QS_MEMORY_CONTEXT* context, QS_JSON_ELEMENT_OBJECT* object, const char* json);

/*
 * Return non-zero when the named property exists, otherwise 0.
 */
int api_qs_object_exist(QS_JSON_ELEMENT_OBJECT* object,const char* name);

/*
 * Get a pointer to a 32-bit signed integer property.
 * Returns NULL if the key is missing or the stored type does not match.
 * The returned pointer is pool-owned.
 */
int32_t* api_qs_object_get_integer(QS_JSON_ELEMENT_OBJECT* object,const char* name);

/*
 * Get a pointer to a 64-bit signed integer property.
 * Returns NULL if the key is missing or the stored type does not match.
 */
int64_t* api_qs_object_get_big_integer(QS_JSON_ELEMENT_OBJECT* object,const char* name);

/*
 * Get a pointer to a 64-bit unsigned integer property.
 * Returns NULL if the key is missing or the stored type does not match.
 */
uint64_t* api_qs_object_get_unsigned_big_integer(QS_JSON_ELEMENT_OBJECT* object,const char* name);

/*
 * Get a 32-bit signed integer property by value.
 * Returns 0 when the key is missing or the stored type does not match.
 */
int32_t api_qs_object_get_integer_val(QS_JSON_ELEMENT_OBJECT* object,const char* name);

/*
 * Get a numeric property by value as int64_t.
 * Accepts stored int32, int64, and uint64 values and converts them to int64_t.
 * Returns 0 when the key is missing.
 */
int64_t api_qs_object_get_big_integer_val(QS_JSON_ELEMENT_OBJECT* object,const char* name);

/*
 * Get a numeric property by value as uint64_t.
 * Accepts stored int32, int64, and uint64 values and converts them to uint64_t.
 * Returns 0 when the key is missing.
 */
uint64_t api_qs_object_get_unsigned_big_integer_val(QS_JSON_ELEMENT_OBJECT* object,const char* name);

/*
 * Get a string property.
 * Returns NULL if the key is missing or the stored type does not match.
 * The returned pointer is pool-owned.
 */
char* api_qs_object_get_string(QS_JSON_ELEMENT_OBJECT* object,const char* name);

/*
 * Get an array property view.
 * On success, dst_array references the existing nested array in the same memory pool.
 */
int api_qs_object_get_array(QS_JSON_ELEMENT_OBJECT* object,const char* name,QS_JSON_ELEMENT_ARRAY* dst_array);

/*
 * Get an object property view.
 * On success, dst_object references the existing nested object in the same memory pool.
 */
int api_qs_object_get_object(QS_JSON_ELEMENT_OBJECT* object,const char* name,QS_JSON_ELEMENT_OBJECT* dst_object);

/*
 * Get object keys as a new array of strings in the same memory pool.
 * Keys are returned in ascending sort order.
 */
int api_qs_object_get_keys(QS_JSON_ELEMENT_OBJECT* object,QS_JSON_ELEMENT_ARRAY* dst_array);

/*
 * Return the number of elements in the array.
 */
int32_t api_qs_array_get_length(QS_JSON_ELEMENT_ARRAY* object);

/*
 * Get a pointer to a 32-bit signed integer array element.
 * Returns NULL if the index is out of range or the element type does not match.
 */
int32_t* api_qs_array_get_integer(QS_JSON_ELEMENT_ARRAY* object,int32_t offset);

/*
 * Get a pointer to a 64-bit signed integer array element.
 */
int64_t* api_qs_array_get_big_integer(QS_JSON_ELEMENT_ARRAY* object,int32_t offset);

/*
 * Get a pointer to a 64-bit unsigned integer array element.
 */
uint64_t* api_qs_array_get_unsigned_big_integer(QS_JSON_ELEMENT_ARRAY* object,int32_t offset);

/*
 * Get a string array element.
 * Returns NULL if the index is out of range or the element type does not match.
 * The returned pointer is pool-owned.
 */
char* api_qs_array_get_string(QS_JSON_ELEMENT_ARRAY* object,int32_t offset);

/*
 * Get a nested array element view.
 * On success, dst_array references the existing nested array in the same memory pool.
 */
int api_qs_array_get_array(QS_JSON_ELEMENT_ARRAY* object,int32_t offset,QS_JSON_ELEMENT_ARRAY* dst_array);

/*
 * Get a nested object element view.
 * On success, dst_object references the existing nested object in the same memory pool.
 */
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
void api_qs_server_memory_info(QS_SERVER_CONTEXT* context);
int api_qs_server_get_socket(QS_SERVER_CONTEXT* context);
int api_qs_set_scheduler(QS_SERVER_CONTEXT* context, int32_t scheduler_mode);
void api_qs_set_server_session_timeout(QS_SERVER_CONTEXT* context, int32_t timeout);
int api_qs_server_create_router(QS_SERVER_CONTEXT* context);
void api_qs_router_memory_info(QS_SERVER_CONTEXT* context);
int api_qs_server_create_kvs(QS_SERVER_CONTEXT* context, int kvs_memory_type);
void api_qs_kvs_memory_info(QS_SERVER_CONTEXT* context);
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
ssize_t api_qs_get_ws_message_size(QS_EVENT_PARAMETER params);
uint8_t api_qs_get_ws_opcode(QS_EVENT_PARAMETER params);
int api_qs_send_ws_message(QS_EVENT_PARAMETER params,const char* message);
int api_qs_send_ws_message_plane(QS_EVENT_PARAMETER params,const char* message);
int api_qs_send_ws_binary(QS_EVENT_PARAMETER params,const void* data,size_t size);
void api_qs_send_ws_binary_by_connection_offset(QS_SERVER_CONTEXT* context,uint32_t connection_offset,const void* data,size_t size);

char* api_qs_get_http_method(QS_EVENT_PARAMETER params);
char* api_qs_get_http_path(QS_EVENT_PARAMETER params);
char* api_qs_get_http_cookie_parameter(QS_EVENT_PARAMETER params, const char* name);
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
uint8_t* api_qs_get_connection_data(QS_EVENT_PARAMETER params);
size_t api_qs_get_connection_data_size(QS_EVENT_PARAMETER params);
char* api_qs_get_connection_id(QS_EVENT_PARAMETER params);
int32_t api_qs_set_connection_data(QS_EVENT_PARAMETER params, uint8_t* data, size_t data_size);
void api_qs_send_response_by_connection_offset(QS_SERVER_CONTEXT* context, uint32_t connection_offset, const char* response);
int api_qs_script_read_file(QS_MEMORY_CONTEXT* memory_context, QS_SERVER_SCRIPT_CONTEXT* script_context,const char* file_path);
int api_qs_script_set_argv_object(QS_SERVER_SCRIPT_CONTEXT* script_context,const char* name, QS_JSON_ELEMENT_OBJECT* object);
int api_qs_script_set_argv_string(QS_SERVER_SCRIPT_CONTEXT* script_context,const char* name, const char* value);
int api_qs_script_set_argv_integer(QS_SERVER_SCRIPT_CONTEXT* script_context,const char* name, int32_t value);
int api_qs_script_run(QS_SERVER_SCRIPT_CONTEXT* script_context);
char* api_qs_script_get_parameter(QS_SERVER_SCRIPT_CONTEXT* script_context, const char* name);

int api_qs_kvs_create_b1mb(QS_MEMORY_CONTEXT* memory_context, QS_KVS_CONTEXT* kvs_context);
int api_qs_kvs_create_b8mb(QS_MEMORY_CONTEXT* memory_context, QS_KVS_CONTEXT* kvs_context);
int api_qs_kvs_create_b16mb(QS_MEMORY_CONTEXT* memory_context, QS_KVS_CONTEXT* kvs_context);
int api_qs_kvs_create_b32mb(QS_MEMORY_CONTEXT* memory_context, QS_KVS_CONTEXT* kvs_context);
int api_qs_kvs_create_b64mb(QS_MEMORY_CONTEXT* memory_context, QS_KVS_CONTEXT* kvs_context);
int api_qs_kvs_create_b128mb(QS_MEMORY_CONTEXT* memory_context, QS_KVS_CONTEXT* kvs_context);
int api_qs_kvs_create_b256mb(QS_MEMORY_CONTEXT* memory_context, QS_KVS_CONTEXT* kvs_context);
int api_qs_kvs_create_b512mb(QS_MEMORY_CONTEXT* memory_context, QS_KVS_CONTEXT* kvs_context);
int api_qs_kvs_create_b1024mb(QS_MEMORY_CONTEXT* memory_context, QS_KVS_CONTEXT* kvs_context);
int api_qs_kvs_create_b1mb_persistence(QS_KVS_CONTEXT* kvs_context, const char* file_path);
int api_qs_kvs_create_b8mb_persistence(QS_KVS_CONTEXT* kvs_context, const char* file_path);
int api_qs_kvs_create_b16mb_persistence(QS_KVS_CONTEXT* kvs_context, const char* file_path);
int api_qs_kvs_create_b32mb_persistence(QS_KVS_CONTEXT* kvs_context, const char* file_path);
int api_qs_kvs_create_b64mb_persistence(QS_KVS_CONTEXT* kvs_context, const char* file_path);
int api_qs_kvs_create_b128mb_persistence(QS_KVS_CONTEXT* kvs_context, const char* file_path);
int api_qs_kvs_create_b256mb_persistence(QS_KVS_CONTEXT* kvs_context, const char* file_path);
int api_qs_kvs_create_b512mb_persistence(QS_KVS_CONTEXT* kvs_context, const char* file_path);
int api_qs_kvs_create_b1024mb_persistence(QS_KVS_CONTEXT* kvs_context, const char* file_path);
int api_qs_kvs_set(QS_KVS_CONTEXT* kvs_context,const char* key, const char* value, int32_t life_time);
char* api_qs_kvs_get(QS_KVS_CONTEXT* kvs_context,const char* key);
size_t api_qs_kvs_get_buffer_size(QS_KVS_CONTEXT* kvs_context,const char* key);
int api_qs_kvs_delete(QS_KVS_CONTEXT* kvs_context,const char* key);
int32_t api_qs_kvs_key_length(QS_KVS_CONTEXT* kvs_context);
int32_t api_qs_kvs_keys(QS_JSON_ELEMENT_ARRAY* array, QS_KVS_CONTEXT* kvs_context);
int32_t api_qs_kvs_sorted_keys(QS_JSON_ELEMENT_ARRAY* array, QS_KVS_CONTEXT* kvs_context, int32_t is_sort_asc);
int api_qs_persistence_kvs_memory_free(QS_KVS_CONTEXT* kvs_context);

int api_qs_room_create(QS_SERVER_CONTEXT* context, const char* name, QS_MEMORY_CONTEXT* dest_memory, QS_JSON_ELEMENT_OBJECT* dest_object);
int api_qs_room_list(QS_SERVER_CONTEXT* context, QS_MEMORY_CONTEXT* dest_memory, QS_JSON_ELEMENT_OBJECT* dest_object);
int api_qs_room_join(QS_SERVER_CONTEXT* context, const char* room_id, const char* connection_id, QS_MEMORY_CONTEXT* dest_memory, QS_JSON_ELEMENT_OBJECT* dest_object);
int api_qs_room_leave(QS_SERVER_CONTEXT* context, const char* room_id, const char* connection_id, QS_MEMORY_CONTEXT* dest_memory, QS_JSON_ELEMENT_OBJECT* dest_object);
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
