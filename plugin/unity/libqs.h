/*
 * Copyright (c) Katsuya Owari
 */

#ifdef _WINDOWS
#define UNITY_INTERFACE_API __stdcall
#define UNITY_INTERFACE_EXPORT __declspec(dllexport)
#else
//#define UNITY_INTERFACE_API __attribute__((cdecl))
#define UNITY_INTERFACE_API 
#define UNITY_INTERFACE_EXPORT 
#endif


#ifdef __cplusplus
extern "C"
{
#endif

#ifdef _WINDOWS
#pragma comment(lib,"ws2_32.lib")
#endif

#include "qs_api.h"

// unity c#側にchar*を受け取る関数の関数ポインタの宣言
typedef void(UNITY_INTERFACE_API *libqs_api_callback_unity_debug_log)(char* str);

// unity c#側にvoid*を受け取り、intを返す関数の関数ポインタの宣言
typedef int (UNITY_INTERFACE_API *libqs_api_callback_unity_on_recv_http_request)(void* http_request_parameter);

typedef struct {
    QS_MEMORY_CONTEXT main_memory;
    QS_MEMORY_CONTEXT temporary_memory;
    QS_SERVER_CONTEXT* server_context;
    libqs_api_callback_unity_debug_log debug_log;
    libqs_api_callback_unity_on_recv_http_request on_recv_http_request;
} LIBQS_WORKING_DATA;

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_init();
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_init_http_server(uint32_t server_port, uint32_t max_connection);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_update();
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_free();
UNITY_INTERFACE_EXPORT uint32_t UNITY_INTERFACE_API libqs_api_rand();
UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_sleep(QS_SERVER_CONTEXT* context);
UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_set_debug_log(libqs_api_callback_unity_debug_log callback);
UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_set_on_recv_http_request(libqs_api_callback_unity_on_recv_http_request callback);
UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_set_on_connect_event(QS_SERVER_CONTEXT* context, QS_EVENT_FUNCTION on_connect);
UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_set_on_plain_event(QS_SERVER_CONTEXT* context, QS_EVENT_FUNCTION on_plain_event);
UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_set_on_simple_event(QS_SERVER_CONTEXT* context, QS_EVENT_FUNCTION on_simple_event);
UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_set_on_http_event(QS_SERVER_CONTEXT* context, QS_EVENT_FUNCTION on_http_event);
UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_set_on_websocket_event(QS_SERVER_CONTEXT* context, QS_EVENT_FUNCTION on_ws_event);
UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_set_on_close_event(QS_SERVER_CONTEXT* context, QS_EVENT_FUNCTION on_close);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_set_scheduler(QS_SERVER_CONTEXT* context, int32_t scheduler_mode);
UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_set_server_session_timeout(QS_SERVER_CONTEXT* context, int32_t timeout);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_server_create_router(QS_SERVER_CONTEXT* context);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_server_create_kvs(QS_SERVER_CONTEXT* context, int kvs_memory_type);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_server_get_kvs(QS_SERVER_CONTEXT* context, QS_KVS_CONTEXT* kvs_context);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_server_create_logger_access(QS_SERVER_CONTEXT* context, const char* log_file_path);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_server_create_logger_debug(QS_SERVER_CONTEXT* context, const char* log_file_path);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_server_create_logger_error(QS_SERVER_CONTEXT* context, const char* log_file_path);
UNITY_INTERFACE_EXPORT QS_SERVER_CONTEXT* UNITY_INTERFACE_API libqs_api_get_server_context(void* event_parameter);
UNITY_INTERFACE_EXPORT uint32_t UNITY_INTERFACE_API libqs_api_get_connection_offset(void* event_parameter);
UNITY_INTERFACE_EXPORT time_t UNITY_INTERFACE_API libqs_api_get_connection_create_time(void* event_parameter);
UNITY_INTERFACE_EXPORT uint8_t* UNITY_INTERFACE_API libqs_api_get_connection_data(void* event_parameter);
UNITY_INTERFACE_EXPORT size_t UNITY_INTERFACE_API libqs_api_get_connection_data_size(void* event_parameter);
UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_get_connection_id(void* event_parameter);
UNITY_INTERFACE_EXPORT int32_t UNITY_INTERFACE_API libqs_api_set_connection_data(void* event_parameter, uint8_t* data, size_t data_size);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_send_response(void* http_request_parameter,char* response);
UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_send_response_with_payload(void* event_parameter, uint32_t payload_type, const char* payload);
UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_send_response_by_connection_offset(QS_SERVER_CONTEXT* context, uint32_t connection_offset, const char* response);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_send_http_response_json(void* http_request_parameter,char* json);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_send_http_response_object_json(void* http_request_parameter, QS_JSON_ELEMENT_OBJECT* object, size_t buffer_size);

UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_get_http_method(void* http_request_parameter);
UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_get_http_path(void* http_request_parameter);
UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_get_http_cookie_parameter(void* http_request_parameter, char* name);
UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_get_http_get_parameter(void* http_request_parameter, char* name);
UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_get_http_post_parameter(void* http_request_parameter, char* name);
UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_get_http_post_body(void* http_request_parameter);
UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_get_http_post_json_object(void* event_parameter, QS_JSON_ELEMENT_OBJECT* object);

UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_get_ws_message(void* event_parameter);
UNITY_INTERFACE_EXPORT ssize_t UNITY_INTERFACE_API libqs_api_get_ws_message_size(void* event_parameter);
UNITY_INTERFACE_EXPORT uint8_t UNITY_INTERFACE_API libqs_api_get_ws_opcode(void* event_parameter);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_send_ws_message(void* event_parameter, const char* message);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_send_ws_message_plane(void* event_parameter, const char* message);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_send_ws_text_by_connection_offset(QS_SERVER_CONTEXT* context, uint32_t connection_offset, const char* message);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_send_ws_binary(void* event_parameter, const void* data, size_t size);
UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_send_ws_binary_by_connection_offset(QS_SERVER_CONTEXT* context, uint32_t connection_offset, const void* data, size_t size);

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_memory_alloc(QS_MEMORY_CONTEXT* context, size_t alloc_size);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_memory_clean(QS_MEMORY_CONTEXT* context);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_memory_free(QS_MEMORY_CONTEXT* context);
UNITY_INTERFACE_EXPORT size_t UNITY_INTERFACE_API libqs_api_memory_available_size(QS_MEMORY_CONTEXT* context);
UNITY_INTERFACE_EXPORT int32_t UNITY_INTERFACE_API libqs_api_memory_create_block(QS_MEMORY_CONTEXT* context, size_t size);
UNITY_INTERFACE_EXPORT void* UNITY_INTERFACE_API libqs_api_memory_get_pointer(QS_MEMORY_CONTEXT* context, int32_t memid);
UNITY_INTERFACE_EXPORT void* UNITY_INTERFACE_API libqs_api_memory_get_offset_pointer(QS_MEMORY_CONTEXT* context, int32_t memid, size_t size, int32_t offset);
UNITY_INTERFACE_EXPORT size_t UNITY_INTERFACE_API libqs_api_memory_get_size(QS_MEMORY_CONTEXT* context, int32_t memid);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_memory_free_block(QS_MEMORY_CONTEXT* context, int32_t* memid);

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_array_create(QS_MEMORY_CONTEXT* context, QS_JSON_ELEMENT_ARRAY* array);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_array_push_integer(QS_JSON_ELEMENT_ARRAY* array, int32_t value);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_array_push_big_integer(QS_JSON_ELEMENT_ARRAY* array, int64_t value);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_array_push_unsigned_big_integer(QS_JSON_ELEMENT_ARRAY* array, uint64_t value);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_array_push_string(QS_JSON_ELEMENT_ARRAY* array, const char* value);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_array_push_object(QS_JSON_ELEMENT_ARRAY* array, QS_JSON_ELEMENT_OBJECT* object);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_array_push_array(QS_JSON_ELEMENT_ARRAY* array, QS_JSON_ELEMENT_ARRAY* push_array);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_object_create(QS_MEMORY_CONTEXT* context, QS_JSON_ELEMENT_OBJECT* object);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_object_push_integer(QS_JSON_ELEMENT_OBJECT* object, const char* name, int32_t value);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_object_push_big_integer(QS_JSON_ELEMENT_OBJECT* object, const char* name, int64_t value);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_object_push_unsigned_big_integer(QS_JSON_ELEMENT_OBJECT* object, const char* name, uint64_t value);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_object_push_string(QS_JSON_ELEMENT_OBJECT* object, const char* name, const char* value);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_object_push_array(QS_JSON_ELEMENT_OBJECT* object, const char* name, QS_JSON_ELEMENT_ARRAY* array);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_object_push_object(QS_JSON_ELEMENT_OBJECT* object, const char* name, QS_JSON_ELEMENT_OBJECT* push_object);
UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_json_encode_object(QS_JSON_ELEMENT_OBJECT* object, size_t buffer_size);
UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_json_encode_array(QS_JSON_ELEMENT_ARRAY* array, size_t buffer_size);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_json_decode_object(QS_MEMORY_CONTEXT* context, QS_JSON_ELEMENT_OBJECT* object, const char* json);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_object_exist(QS_JSON_ELEMENT_OBJECT* object, const char* name);
UNITY_INTERFACE_EXPORT int32_t UNITY_INTERFACE_API libqs_api_object_get_integer_val(QS_JSON_ELEMENT_OBJECT* object, const char* name);
UNITY_INTERFACE_EXPORT int64_t UNITY_INTERFACE_API libqs_api_object_get_big_integer_val(QS_JSON_ELEMENT_OBJECT* object, const char* name);
UNITY_INTERFACE_EXPORT uint64_t UNITY_INTERFACE_API libqs_api_object_get_unsigned_big_integer_val(QS_JSON_ELEMENT_OBJECT* object, const char* name);
UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_object_get_string(QS_JSON_ELEMENT_OBJECT* object, const char* name);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_object_get_array(QS_JSON_ELEMENT_OBJECT* object, const char* name, QS_JSON_ELEMENT_ARRAY* dst_array);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_object_get_object(QS_JSON_ELEMENT_OBJECT* object, const char* name, QS_JSON_ELEMENT_OBJECT* dst_object);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_object_get_keys(QS_JSON_ELEMENT_OBJECT* object, QS_JSON_ELEMENT_ARRAY* dst_array);
UNITY_INTERFACE_EXPORT int32_t UNITY_INTERFACE_API libqs_api_array_get_length(QS_JSON_ELEMENT_ARRAY* object);
UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_array_get_string(QS_JSON_ELEMENT_ARRAY* object, int32_t offset);
UNITY_INTERFACE_EXPORT int32_t* UNITY_INTERFACE_API libqs_api_array_get_integer(QS_JSON_ELEMENT_ARRAY* object, int32_t offset);
UNITY_INTERFACE_EXPORT int64_t* UNITY_INTERFACE_API libqs_api_array_get_big_integer(QS_JSON_ELEMENT_ARRAY* object, int32_t offset);
UNITY_INTERFACE_EXPORT uint64_t* UNITY_INTERFACE_API libqs_api_array_get_unsigned_big_integer(QS_JSON_ELEMENT_ARRAY* object, int32_t offset);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_array_get_array(QS_JSON_ELEMENT_ARRAY* object, int32_t offset, QS_JSON_ELEMENT_ARRAY* dst_array);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_array_get_object(QS_JSON_ELEMENT_ARRAY* object, int32_t offset, QS_JSON_ELEMENT_OBJECT* dst_object);
UNITY_INTERFACE_EXPORT int32_t* UNITY_INTERFACE_API libqs_api_object_get_integer(QS_JSON_ELEMENT_OBJECT* object, const char* name);
UNITY_INTERFACE_EXPORT int64_t* UNITY_INTERFACE_API libqs_api_object_get_big_integer(QS_JSON_ELEMENT_OBJECT* object, const char* name);
UNITY_INTERFACE_EXPORT uint64_t* UNITY_INTERFACE_API libqs_api_object_get_unsigned_big_integer(QS_JSON_ELEMENT_OBJECT* object, const char* name);

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_csv_read_file(QS_MEMORY_CONTEXT* context, QS_CSV_CONTEXT* csv, const char* csv_file_path);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_csv_parse(QS_MEMORY_CONTEXT* context, QS_CSV_CONTEXT* csv, const char* src_csv);
UNITY_INTERFACE_EXPORT int32_t UNITY_INTERFACE_API libqs_api_csv_get_line_length(QS_CSV_CONTEXT* csv);
UNITY_INTERFACE_EXPORT int32_t UNITY_INTERFACE_API libqs_api_csv_get_row_length(QS_CSV_CONTEXT* csv, int32_t line_pos);
UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_csv_get_row(QS_CSV_CONTEXT* csv, int32_t line_pos, int32_t row_pos);

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_create_b1mb(QS_MEMORY_CONTEXT* memory_context, QS_KVS_CONTEXT* kvs_context);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_create_b8mb(QS_MEMORY_CONTEXT* memory_context, QS_KVS_CONTEXT* kvs_context);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_create_b16mb(QS_MEMORY_CONTEXT* memory_context, QS_KVS_CONTEXT* kvs_context);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_create_b32mb(QS_MEMORY_CONTEXT* memory_context, QS_KVS_CONTEXT* kvs_context);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_create_b64mb(QS_MEMORY_CONTEXT* memory_context, QS_KVS_CONTEXT* kvs_context);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_create_b128mb(QS_MEMORY_CONTEXT* memory_context, QS_KVS_CONTEXT* kvs_context);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_create_b256mb(QS_MEMORY_CONTEXT* memory_context, QS_KVS_CONTEXT* kvs_context);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_create_b512mb(QS_MEMORY_CONTEXT* memory_context, QS_KVS_CONTEXT* kvs_context);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_create_b1024mb(QS_MEMORY_CONTEXT* memory_context, QS_KVS_CONTEXT* kvs_context);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_create_b1mb_persistence(QS_KVS_CONTEXT* kvs_context, const char* file_path);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_create_b8mb_persistence(QS_KVS_CONTEXT* kvs_context, const char* file_path);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_create_b16mb_persistence(QS_KVS_CONTEXT* kvs_context, const char* file_path);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_create_b32mb_persistence(QS_KVS_CONTEXT* kvs_context, const char* file_path);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_create_b64mb_persistence(QS_KVS_CONTEXT* kvs_context, const char* file_path);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_create_b128mb_persistence(QS_KVS_CONTEXT* kvs_context, const char* file_path);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_create_b256mb_persistence(QS_KVS_CONTEXT* kvs_context, const char* file_path);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_create_b512mb_persistence(QS_KVS_CONTEXT* kvs_context, const char* file_path);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_create_b1024mb_persistence(QS_KVS_CONTEXT* kvs_context, const char* file_path);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_set(QS_KVS_CONTEXT* kvs_context, const char* key, const char* value, int32_t life_time);
UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_kvs_get(QS_KVS_CONTEXT* kvs_context, const char* key);
UNITY_INTERFACE_EXPORT size_t UNITY_INTERFACE_API libqs_api_kvs_get_buffer_size(QS_KVS_CONTEXT* kvs_context, const char* key);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_delete(QS_KVS_CONTEXT* kvs_context, const char* key);
UNITY_INTERFACE_EXPORT int32_t UNITY_INTERFACE_API libqs_api_kvs_key_length(QS_KVS_CONTEXT* kvs_context);
UNITY_INTERFACE_EXPORT int32_t UNITY_INTERFACE_API libqs_api_kvs_keys(QS_JSON_ELEMENT_ARRAY* array, QS_KVS_CONTEXT* kvs_context);
UNITY_INTERFACE_EXPORT int32_t UNITY_INTERFACE_API libqs_api_kvs_sorted_keys(QS_JSON_ELEMENT_ARRAY* array, QS_KVS_CONTEXT* kvs_context, int32_t is_sort_asc);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_persistence_kvs_memory_free(QS_KVS_CONTEXT* kvs_context);

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_logger_create(QS_MEMORY_CONTEXT* context, QS_LOGGER_CONTEXT* logger, int32_t level);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_logger_open(QS_LOGGER_CONTEXT* logger, const char* file_path);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_logger_close(QS_LOGGER_CONTEXT* logger);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_logger_free(QS_LOGGER_CONTEXT* logger);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_logger_set_level(QS_LOGGER_CONTEXT* logger, int32_t level);
UNITY_INTERFACE_EXPORT int32_t UNITY_INTERFACE_API libqs_api_logger_get_level(QS_LOGGER_CONTEXT* logger);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_logger_write(QS_LOGGER_CONTEXT* logger, int32_t level, const char* file, int32_t line, const char* message);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_logger_debug(QS_LOGGER_CONTEXT* logger, const char* message);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_logger_info(QS_LOGGER_CONTEXT* logger, const char* message);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_logger_warn(QS_LOGGER_CONTEXT* logger, const char* message);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_logger_error(QS_LOGGER_CONTEXT* logger, const char* message);

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_server_init(QS_SERVER_CONTEXT** ppcontext, int port, int32_t max_connection, int32_t server_type);
UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_server_memory_info(QS_SERVER_CONTEXT* context);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_server_get_socket(QS_SERVER_CONTEXT* context);
UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_router_memory_info(QS_SERVER_CONTEXT* context);
UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_kvs_memory_info(QS_SERVER_CONTEXT* context);
UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_server_set_lowlevel_send_hook(QS_SERVER_CONTEXT* context, QS_SOCKET_SEND_HOOK func);
UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_server_set_lowlevel_recv_hook(QS_SERVER_CONTEXT* context, QS_SOCKET_RECV_HOOK func);

UNITY_INTERFACE_EXPORT uint32_t UNITY_INTERFACE_API libqs_api_get_plain_payload_type(void* event_parameter);
UNITY_INTERFACE_EXPORT uint8_t* UNITY_INTERFACE_API libqs_api_get_plain_payload(void* event_parameter);
UNITY_INTERFACE_EXPORT size_t UNITY_INTERFACE_API libqs_api_get_plain_payload_length(void* event_parameter);
UNITY_INTERFACE_EXPORT uint8_t* UNITY_INTERFACE_API libqs_api_get_websocket_payload(void* event_parameter);
UNITY_INTERFACE_EXPORT size_t UNITY_INTERFACE_API libqs_api_get_websocket_payload_length(void* event_parameter);
UNITY_INTERFACE_EXPORT uint8_t UNITY_INTERFACE_API libqs_api_get_websocket_opcode(void* event_parameter);

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_http_response_json(void* event_parameter, QS_JSON_ELEMENT_OBJECT* object, size_t buffer_size);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_http_response_raw_json(void* event_parameter, QS_MEMORY_CONTEXT* temporary_memory, char* json);

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_client_init(QS_CLIENT_CONTEXT** ppcontext, const char* host, int port, int32_t server_type);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_websocket_client_init(QS_CLIENT_CONTEXT** ppcontext, const char* host, int port, const char* path);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_client_get_socket(QS_CLIENT_CONTEXT* context);
UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_set_client_on_connect_event(QS_CLIENT_CONTEXT* context, QS_EVENT_FUNCTION on_connect);
UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_set_client_on_plain_event(QS_CLIENT_CONTEXT* context, QS_EVENT_FUNCTION on_plain_event);
UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_set_client_on_simple_event(QS_CLIENT_CONTEXT* context, QS_EVENT_FUNCTION on_simple_event);
UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_set_client_on_close_event(QS_CLIENT_CONTEXT* context, QS_EVENT_FUNCTION on_close);
UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_set_websocket_client_on_event(QS_CLIENT_CONTEXT* context, QS_EVENT_FUNCTION on_event);
UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_client_update(QS_CLIENT_CONTEXT* context);
UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_client_sleep(QS_CLIENT_CONTEXT* context);
UNITY_INTERFACE_EXPORT QS_CLIENT_CONTEXT* UNITY_INTERFACE_API libqs_api_client_get_context(void* event_parameter);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_client_send(QS_CLIENT_CONTEXT* context, const char* payload, size_t payload_len);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_client_send_message(QS_CLIENT_CONTEXT* context, uint32_t payload_type, const char* payload, size_t payload_len);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_websocket_client_send(QS_CLIENT_CONTEXT* context, int is_binary, const void* payload, size_t payload_len);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_websocket_client_close(QS_CLIENT_CONTEXT* context);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_websocket_client_close_with_reason(QS_CLIENT_CONTEXT* context, uint16_t status_code, const char* reason);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_websocket_client_set_max_message_size(QS_CLIENT_CONTEXT* context, size_t max_message_size);
UNITY_INTERFACE_EXPORT size_t UNITY_INTERFACE_API libqs_api_websocket_client_get_max_message_size(QS_CLIENT_CONTEXT* context);
UNITY_INTERFACE_EXPORT QS_WEBSOCKET_STATE UNITY_INTERFACE_API libqs_api_websocket_client_get_state(QS_CLIENT_CONTEXT* context);
UNITY_INTERFACE_EXPORT QS_WEBSOCKET_ERROR UNITY_INTERFACE_API libqs_api_websocket_client_get_error(QS_CLIENT_CONTEXT* context);
UNITY_INTERFACE_EXPORT uint16_t UNITY_INTERFACE_API libqs_api_websocket_client_get_close_code(QS_CLIENT_CONTEXT* context);
UNITY_INTERFACE_EXPORT const char* UNITY_INTERFACE_API libqs_api_websocket_client_get_close_reason(QS_CLIENT_CONTEXT* context);
UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_client_free(QS_CLIENT_CONTEXT* context);
UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_client_set_lowlevel_send_hook(QS_CLIENT_CONTEXT* context, QS_SOCKET_SEND_HOOK func);
UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_client_set_lowlevel_recv_hook(QS_CLIENT_CONTEXT* context, QS_SOCKET_RECV_HOOK func);

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_script_read_file(QS_MEMORY_CONTEXT* memory_context, QS_SERVER_SCRIPT_CONTEXT* script_context, const char* file_path);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_script_set_argv_object(QS_SERVER_SCRIPT_CONTEXT* script_context, const char* name, QS_JSON_ELEMENT_OBJECT* object);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_script_set_argv_string(QS_SERVER_SCRIPT_CONTEXT* script_context, const char* name, const char* value);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_script_set_argv_integer(QS_SERVER_SCRIPT_CONTEXT* script_context, const char* name, int32_t value);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_script_run(QS_SERVER_SCRIPT_CONTEXT* script_context);
UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_script_get_parameter(QS_SERVER_SCRIPT_CONTEXT* script_context, const char* name);

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_room_create(QS_SERVER_CONTEXT* context, const char* name, QS_MEMORY_CONTEXT* dest_memory, QS_JSON_ELEMENT_OBJECT* dest_object);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_room_list(QS_SERVER_CONTEXT* context, QS_MEMORY_CONTEXT* dest_memory, QS_JSON_ELEMENT_OBJECT* dest_object);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_room_join(QS_SERVER_CONTEXT* context, const char* room_id, const char* connection_id, QS_MEMORY_CONTEXT* dest_memory, QS_JSON_ELEMENT_OBJECT* dest_object);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_room_leave(QS_SERVER_CONTEXT* context, const char* room_id, const char* connection_id, QS_MEMORY_CONTEXT* dest_memory, QS_JSON_ELEMENT_OBJECT* dest_object);

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_ranking_create(QS_MEMORY_CONTEXT* memory_context, QS_RANKING_CONTEXT* ranking_context, size_t size, int32_t key_size, int32_t get_max, int32_t refresh_size);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_ranking_entry(QS_RANKING_CONTEXT* ranking_context, const char* id);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_ranking_set_value(QS_RANKING_CONTEXT* ranking_context, const char* id, uint32_t value);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_ranking_add_value(QS_RANKING_CONTEXT* ranking_context, const char* id, uint32_t value);
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_ranking_sort(QS_RANKING_CONTEXT* ranking_context);
UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_ranking_get(QS_RANKING_CONTEXT* ranking_context, int32_t offset, int32_t length, QS_MEMORY_CONTEXT* dest_memory);

UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_memory_info(QS_MEMORY_CONTEXT* context);

UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_uniqid(QS_MEMORY_CONTEXT* memory_context, int32_t length);
UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_base64_encode(QS_MEMORY_CONTEXT* memory_context, const void* data, size_t length);
UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_base64_decode(QS_MEMORY_CONTEXT* memory_context, const char* data);
UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_sha1_encode(QS_MEMORY_CONTEXT* memory_context, const void* data, size_t length);

/* JSON string escape utility */
UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_json_escape(const char* src, char* dst, size_t dst_size);

void libqs_api_debug_log(char* str);

#ifdef __cplusplus
}
#endif
