/*
 * Copyright (c) Katsuya Owari
 */

#ifdef __cplusplus
extern "C"{
#endif

#ifndef _QS_API_H_
#define _QS_API_H_

#include <sys/types.h>
#include <inttypes.h>

#ifdef _WINDOWS
#ifndef __WINDOWS__
#define __WINDOWS__
#endif
#endif

#define QS_SERVER_TYPE_PLAIN 100
#define QS_SERVER_TYPE_SIMPLE 200
#define QS_SERVER_TYPE_HTTP 300

#define QS_SCHEDULER_MODE_HIGH 1
#define QS_SCHEDULER_MODE_MIDDLE 2
#define QS_SCHEDULER_MODE_LOW 3

#define QS_KVS_MEMORY_TYPE_B1MB 0
#define QS_KVS_MEMORY_TYPE_B128MB 1
#define QS_KVS_MEMORY_TYPE_B256MB 2
#define QS_KVS_MEMORY_TYPE_B512MB 3
#define QS_KVS_MEMORY_TYPE_B1024MB 4

#define QS_EVENT_PARAMETER_TYPE_RECV 1
#define QS_EVENT_PARAMETER_TYPE_CONNECTION 2
#define QS_WEBSOCKET_MAX_MESSAGE_SIZE 65535

#define QS_DB_CAPACITY_B1MB    1
#define QS_DB_CAPACITY_B8MB    8
#define QS_DB_CAPACITY_B16MB   16
#define QS_DB_CAPACITY_B32MB   32
#define QS_DB_CAPACITY_B64MB   64
#define QS_DB_CAPACITY_B128MB  128
#define QS_DB_CAPACITY_B256MB  256
#define QS_DB_CAPACITY_B512MB  512
#define QS_DB_CAPACITY_B1024MB 1024
#define QS_DB_SCAN_FULL   0
#define QS_DB_SCAN_INDEX  1

typedef enum QS_WEBSOCKET_STATE
{
	QS_WEBSOCKET_STATE_CONNECTING = 0,
	QS_WEBSOCKET_STATE_HANDSHAKING,
	QS_WEBSOCKET_STATE_OPEN,
	QS_WEBSOCKET_STATE_CLOSING,
	QS_WEBSOCKET_STATE_CLOSED,
	QS_WEBSOCKET_STATE_ERROR
} QS_WEBSOCKET_STATE;

typedef enum QS_WEBSOCKET_ERROR
{
	QS_WEBSOCKET_ERROR_NONE = 0,
	QS_WEBSOCKET_ERROR_HANDSHAKE_SEND,
	QS_WEBSOCKET_ERROR_HANDSHAKE_RESPONSE,
	QS_WEBSOCKET_ERROR_HANDSHAKE_ACCEPT,
	QS_WEBSOCKET_ERROR_RECEIVE_BUFFER,
	QS_WEBSOCKET_ERROR_FRAME,
	QS_WEBSOCKET_ERROR_SEND
} QS_WEBSOCKET_ERROR;

typedef struct QS_WEBSOCKET_CLIENT QS_WEBSOCKET_CLIENT;

#ifndef _QS_SOCKET_ID_DEFINED_
#define _QS_SOCKET_ID_DEFINED_
#ifdef __WINDOWS__
typedef intptr_t QS_SOCKET_ID;
#else
typedef int QS_SOCKET_ID;
#endif
#endif

typedef ssize_t (*QS_SOCKET_SEND_HOOK)(QS_SOCKET_ID soc, const char *buf, size_t size, int flag);
typedef ssize_t (*QS_SOCKET_RECV_HOOK)(QS_SOCKET_ID soc, char *buf, size_t size, int flag);

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

	// websocket binary support
	uint8_t ws_opcode;		// last received WS opcode (1:text, 2:binary)
	ssize_t ws_message_size;	// last received WS payload size in bytes
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
	int32_t websocket_mode;
	QS_WEBSOCKET_CLIENT* websocket_client;
	QS_EVENT_FUNCTION on_websocket_event;
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

typedef struct QS_RANKING_CONTEXT
{
	int32_t memid_ranking;
	void* memory;
} QS_RANKING_CONTEXT;

typedef struct QS_DB_API_CONTEXT
{
	QS_MEMORY_CONTEXT memory;
	int32_t memid_db;
	int32_t auto_index_update; /* 1: レコード CRUD で index を自動更新する */
} QS_DB_API_CONTEXT;

#ifdef __GNUC__
#define QS_LOG_PRINTF_CHECK(fmt_index, arg_index) __attribute__((format(printf, fmt_index, arg_index)))
#else
#define QS_LOG_PRINTF_CHECK(fmt_index, arg_index)
#endif

typedef enum QS_LOG_LEVEL
{
	QS_LOG_LEVEL_DEBUG = 0,
	QS_LOG_LEVEL_INFO,
	QS_LOG_LEVEL_WARN,
	QS_LOG_LEVEL_ERROR,
	QS_LOG_LEVEL_NONE,			// ロガーの閾値に指定すると全出力を抑制
} QS_LOG_LEVEL;

typedef struct QS_LOGGER_CONTEXT
{
	int32_t memid_logger;		// QS_LOGGER を格納した munit（-1 は無効）
	void* memory;				// QS_MEMORY_POOL
} QS_LOGGER_CONTEXT;

int api_qs_init();
uint32_t api_qs_rand();
int api_qs_memory_alloc(QS_MEMORY_CONTEXT* context, size_t alloc_size);
int api_qs_memory_clean(QS_MEMORY_CONTEXT* context);
void api_qs_memory_info(QS_MEMORY_CONTEXT* context);
int api_qs_memory_free(QS_MEMORY_CONTEXT* context);
size_t api_qs_memory_available_size(QS_MEMORY_CONTEXT* context);
int32_t api_qs_memory_create_block(QS_MEMORY_CONTEXT* context, size_t size);
void* api_qs_memory_get_pointer(QS_MEMORY_CONTEXT* context, int32_t memid);
void* api_qs_memory_get_offset_pointer(QS_MEMORY_CONTEXT* context, int32_t memid, size_t size, int32_t offset);
size_t api_qs_memory_get_size(QS_MEMORY_CONTEXT* context, int32_t memid);
size_t api_qs_memory_block_used(QS_MEMORY_CONTEXT* context, int32_t memid);
size_t api_qs_memory_block_remaining(QS_MEMORY_CONTEXT* context, int32_t memid);
int api_qs_memory_block_append(QS_MEMORY_CONTEXT* context, int32_t memid, const void* src, size_t len);
int api_qs_memory_block_write(QS_MEMORY_CONTEXT* context, int32_t memid, size_t offset, const void* src, size_t len);
int api_qs_memory_block_read(QS_MEMORY_CONTEXT* context, int32_t memid, size_t offset, void* dst, size_t len);
int api_qs_memory_block_clear(QS_MEMORY_CONTEXT* context, int32_t memid);
int api_qs_memory_free_block(QS_MEMORY_CONTEXT* context, int32_t* memid);
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
int api_qs_websocket_client_init(QS_CLIENT_CONTEXT** ppcontext, const char* host, int port, const char* path);
int api_qs_client_get_socket(QS_CLIENT_CONTEXT* context);
void api_qs_set_client_on_connect_event(QS_CLIENT_CONTEXT* context, QS_EVENT_FUNCTION on_connect );
void api_qs_set_client_on_plain_event(QS_CLIENT_CONTEXT* context, QS_EVENT_FUNCTION on_plain_event );
void api_qs_set_client_on_simple_event(QS_CLIENT_CONTEXT* context, QS_EVENT_FUNCTION on_simple_event );
void api_qs_set_client_on_close_event(QS_CLIENT_CONTEXT* context, QS_EVENT_FUNCTION on_close );
void api_qs_set_websocket_client_on_event(QS_CLIENT_CONTEXT* context, QS_EVENT_FUNCTION on_event);
void api_qs_client_update(QS_CLIENT_CONTEXT* context);
void api_qs_client_sleep(QS_CLIENT_CONTEXT* context);
QS_CLIENT_CONTEXT* api_qs_client_get_context(QS_EVENT_PARAMETER params);
int api_qs_client_send(QS_CLIENT_CONTEXT* context, const char* payload, size_t payload_len);
int api_qs_client_send_message(QS_CLIENT_CONTEXT* context,uint32_t payload_type, const char* payload, size_t payload_len);
int api_qs_websocket_client_send(QS_CLIENT_CONTEXT* context, int is_binary, const void* payload, size_t payload_len);
int api_qs_websocket_client_close(QS_CLIENT_CONTEXT* context);
int api_qs_websocket_client_close_with_reason(QS_CLIENT_CONTEXT* context, uint16_t status_code, const char* reason);
int api_qs_websocket_client_set_max_message_size(QS_CLIENT_CONTEXT* context, size_t max_message_size);
size_t api_qs_websocket_client_get_max_message_size(QS_CLIENT_CONTEXT* context);
QS_WEBSOCKET_STATE api_qs_websocket_client_get_state(QS_CLIENT_CONTEXT* context);
QS_WEBSOCKET_ERROR api_qs_websocket_client_get_error(QS_CLIENT_CONTEXT* context);
uint16_t api_qs_websocket_client_get_close_code(QS_CLIENT_CONTEXT* context);
const char* api_qs_websocket_client_get_close_reason(QS_CLIENT_CONTEXT* context);
uint8_t* api_qs_get_websocket_payload(QS_EVENT_PARAMETER params);
size_t api_qs_get_websocket_payload_length(QS_EVENT_PARAMETER params);
uint8_t api_qs_get_websocket_opcode(QS_EVENT_PARAMETER params);
void api_qs_client_free(QS_CLIENT_CONTEXT* context);
void api_qs_client_set_lowlevel_send_hook(QS_CLIENT_CONTEXT* context, QS_SOCKET_SEND_HOOK func);
void api_qs_client_set_lowlevel_recv_hook(QS_CLIENT_CONTEXT* context, QS_SOCKET_RECV_HOOK func);


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
void api_qs_server_set_lowlevel_send_hook(QS_SERVER_CONTEXT* context, QS_SOCKET_SEND_HOOK func);
void api_qs_server_set_lowlevel_recv_hook(QS_SERVER_CONTEXT* context, QS_SOCKET_RECV_HOOK func);

char* api_qs_get_ws_message(QS_EVENT_PARAMETER params);
ssize_t api_qs_get_ws_message_size(QS_EVENT_PARAMETER params);
uint8_t api_qs_get_ws_opcode(QS_EVENT_PARAMETER params);
int api_qs_send_ws_message(QS_EVENT_PARAMETER params,const char* message);
int api_qs_send_ws_message_plane(QS_EVENT_PARAMETER params,const char* message);
/* Call from the server event-loop thread; connection_offset is a current slot index. */
int api_qs_send_ws_text_by_connection_offset(QS_SERVER_CONTEXT* context,uint32_t connection_offset,const char* message);
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
int api_qs_ranking_create(QS_MEMORY_CONTEXT* memory_context, QS_RANKING_CONTEXT* ranking_context, size_t size, int32_t key_size, int32_t get_max, int32_t refresh_size);
int api_qs_ranking_entry(QS_RANKING_CONTEXT* ranking_context, const char* id);
int api_qs_ranking_set_value(QS_RANKING_CONTEXT* ranking_context, const char* id, uint32_t value);
int api_qs_ranking_add_value(QS_RANKING_CONTEXT* ranking_context, const char* id, uint32_t value);
int api_qs_ranking_sort(QS_RANKING_CONTEXT* ranking_context);
char* api_qs_ranking_get(QS_RANKING_CONTEXT* ranking_context, int32_t offset, int32_t length, QS_MEMORY_CONTEXT* dest_memory);
int api_qs_db_init(QS_DB_API_CONTEXT* context);
/* Context is non-copyable while initialized. Free closes an open database first.
 * Returns 1 and retains context if table handles remain. */
int api_qs_db_free(QS_DB_API_CONTEXT* context);
/* Enable/disable automatic index updates in the record CRUD functions.
 * Disabled by default. The flag does not affect direct index API calls
 * (api_qs_db_index_add / api_qs_db_index_remove). */
int api_qs_db_set_auto_index_update(QS_DB_API_CONTEXT* context, int32_t enable);
int api_qs_db_create_database(const char* base_dir, const char* database);
int api_qs_db_drop_database(const char* base_dir, const char* database);
int api_qs_db_show_database(QS_MEMORY_CONTEXT* memory_context, QS_JSON_ELEMENT_ARRAY* array, const char* base_dir);
int api_qs_db_open_database(QS_DB_API_CONTEXT* context, const char* base_dir, const char* database);
int api_qs_db_close_database(QS_DB_API_CONTEXT* context);
int api_qs_db_create_table(QS_DB_API_CONTEXT* context, const char* table, int capacity_mb);
int api_qs_db_drop_table(QS_DB_API_CONTEXT* context, const char* table);
int api_qs_db_show_table(QS_MEMORY_CONTEXT* memory_context, QS_DB_API_CONTEXT* context, QS_JSON_ELEMENT_ARRAY* array);
/* Open at most 64 named handles. Close the handle before record/list/index operations on that table. */
int api_qs_db_table_open(QS_DB_API_CONTEXT* context, const char* table);
int api_qs_db_table_close(QS_DB_API_CONTEXT* context, const char* table);
int api_qs_db_schema_define(QS_DB_API_CONTEXT* context, const char* table, const char* schema_json);
int api_qs_db_schema_get(QS_DB_API_CONTEXT* context, const char* table, char* out_buf, size_t out_size);
int api_qs_db_schema_validate(QS_MEMORY_CONTEXT* memory_context, QS_DB_API_CONTEXT* context, const char* table, const char* record_json);
/* Insert/update validate records when a schema exists and apply defaults to absent fields.
 * When auto_index_update is enabled (api_qs_db_set_auto_index_update), fields with
 * "is_index":1 in the schema are indexed automatically. Index entries are added or
 * removed after the record write succeeds; a later index failure keeps the written
 * record and returns -1 (use api_qs_db_index_rebuild to resynchronize). */
int api_qs_db_record_insert(QS_DB_API_CONTEXT* context, const char* table, const char* id, const char* record_json);
int api_qs_db_record_update(QS_DB_API_CONTEXT* context, const char* table, const char* id, const char* record_json);
int api_qs_db_record_delete(QS_DB_API_CONTEXT* context, const char* table, const char* id);
int api_qs_db_record_get(QS_DB_API_CONTEXT* context, const char* table, const char* id, char* out_buf, size_t out_size);
char* api_qs_db_record_list(QS_MEMORY_CONTEXT* memory_context, QS_DB_API_CONTEXT* context, const char* table, const char* filter_json, int scan_mode);
int api_qs_db_index_add(QS_DB_API_CONTEXT* context, const char* table, const char* field, const char* value, const char* record_id);
int api_qs_db_index_remove(QS_DB_API_CONTEXT* context, const char* table, const char* field, const char* value, const char* record_id);
int api_qs_db_index_rebuild(QS_MEMORY_CONTEXT* memory_context, QS_DB_API_CONTEXT* context, const char* table, const char* indexed_fields_json);
int api_qs_db_next_id(QS_DB_API_CONTEXT* context, const char* table, const char* prefix, char* out_id, size_t out_size);
int api_qs_http_response_json(QS_EVENT_PARAMETER params, QS_JSON_ELEMENT_OBJECT* object, size_t buffer_size);
int api_qs_http_response_raw_json(QS_EVENT_PARAMETER params, QS_MEMORY_CONTEXT* temporary_memory,char* json);

int api_qs_logger_create(QS_MEMORY_CONTEXT* context, QS_LOGGER_CONTEXT* logger, int32_t level);
int api_qs_logger_open(QS_LOGGER_CONTEXT* logger, const char* file_path);
int api_qs_logger_close(QS_LOGGER_CONTEXT* logger);
int api_qs_logger_free(QS_LOGGER_CONTEXT* logger);
int api_qs_logger_set_level(QS_LOGGER_CONTEXT* logger, int32_t level);
int32_t api_qs_logger_get_level(QS_LOGGER_CONTEXT* logger);
int api_qs_logger_write(QS_LOGGER_CONTEXT* logger, int32_t level, const char* file, int32_t line, const char* format, ...) QS_LOG_PRINTF_CHECK(5, 6);
int api_qs_logger_debug(QS_LOGGER_CONTEXT* logger, const char* format, ...) QS_LOG_PRINTF_CHECK(2, 3);
int api_qs_logger_info(QS_LOGGER_CONTEXT* logger, const char* format, ...) QS_LOG_PRINTF_CHECK(2, 3);
int api_qs_logger_warn(QS_LOGGER_CONTEXT* logger, const char* format, ...) QS_LOG_PRINTF_CHECK(2, 3);
int api_qs_logger_error(QS_LOGGER_CONTEXT* logger, const char* format, ...) QS_LOG_PRINTF_CHECK(2, 3);

// マクロ版（__FILE__ / __LINE__ を付与）
#define API_QS_LOG_DEBUG(logger, ...) api_qs_logger_write(logger, QS_LOG_LEVEL_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define API_QS_LOG_INFO(logger, ...)  api_qs_logger_write(logger, QS_LOG_LEVEL_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define API_QS_LOG_WARN(logger, ...)  api_qs_logger_write(logger, QS_LOG_LEVEL_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define API_QS_LOG_ERROR(logger, ...) api_qs_logger_write(logger, QS_LOG_LEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__)

char* api_qs_uniqid(QS_MEMORY_CONTEXT* memory_context, int32_t length);
char* api_qs_base64_encode(QS_MEMORY_CONTEXT* memory_context, const void* data, size_t length);
char* api_qs_base64_decode(QS_MEMORY_CONTEXT* memory_context, const char* data);
char* api_qs_sha1_encode(QS_MEMORY_CONTEXT* memory_context, const void* data, size_t length);

/* JSON string escape utility */
int api_qs_json_escape(const char* src, char* dst, size_t dst_size);

#endif /*_QS_API_H_*/

#ifdef __cplusplus
}
#endif
