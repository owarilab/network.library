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

UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_sleep(QS_SERVER_CONTEXT* context)
{
	api_qs_sleep(context);
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
UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_get_http_cookie_parameter(void* http_request_parameter, char* name)
{
	return api_qs_get_http_cookie_parameter(http_request_parameter, name);
}
UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_get_http_get_parameter(void* http_request_parameter, char* name)
{
	return api_qs_get_http_get_parameter(http_request_parameter, name);
}
UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_get_http_post_parameter(void* http_request_parameter, char* name)
{
	return api_qs_get_http_post_parameter(http_request_parameter, name);
}
UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_get_http_post_body(void* http_request_parameter)
{
	return api_qs_get_http_post_body(http_request_parameter);
}

UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_set_on_connect_event(QS_SERVER_CONTEXT* context, QS_EVENT_FUNCTION on_connect)
{
	api_qs_set_on_connect_event(context, on_connect);
}

UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_set_on_plain_event(QS_SERVER_CONTEXT* context, QS_EVENT_FUNCTION on_plain_event)
{
	api_qs_set_on_plain_event(context, on_plain_event);
}

UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_set_on_simple_event(QS_SERVER_CONTEXT* context, QS_EVENT_FUNCTION on_simple_event)
{
	api_qs_set_on_simple_event(context, on_simple_event);
}

UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_set_on_http_event(QS_SERVER_CONTEXT* context, QS_EVENT_FUNCTION on_http_event)
{
	api_qs_set_on_http_event(context, on_http_event);
}

UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_set_on_websocket_event(QS_SERVER_CONTEXT* context, QS_EVENT_FUNCTION on_ws_event)
{
	api_qs_set_on_websocket_event(context, on_ws_event);
}

UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_set_on_close_event(QS_SERVER_CONTEXT* context, QS_EVENT_FUNCTION on_close)
{
	api_qs_set_on_close_event(context, on_close);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_set_scheduler(QS_SERVER_CONTEXT* context, int32_t scheduler_mode)
{
	return api_qs_set_scheduler(context, scheduler_mode);
}

UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_set_server_session_timeout(QS_SERVER_CONTEXT* context, int32_t timeout)
{
	api_qs_set_server_session_timeout(context, timeout);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_server_create_router(QS_SERVER_CONTEXT* context)
{
	return api_qs_server_create_router(context);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_server_create_kvs(QS_SERVER_CONTEXT* context, int kvs_memory_type)
{
	return api_qs_server_create_kvs(context, kvs_memory_type);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_server_get_kvs(QS_SERVER_CONTEXT* context, QS_KVS_CONTEXT* kvs_context)
{
	return api_qs_server_get_kvs(context, kvs_context);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_server_create_logger_access(QS_SERVER_CONTEXT* context, const char* log_file_path)
{
	return api_qs_server_create_logger_access(context, log_file_path);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_server_create_logger_debug(QS_SERVER_CONTEXT* context, const char* log_file_path)
{
	return api_qs_server_create_logger_debug(context, log_file_path);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_server_create_logger_error(QS_SERVER_CONTEXT* context, const char* log_file_path)
{
	return api_qs_server_create_logger_error(context, log_file_path);
}

UNITY_INTERFACE_EXPORT QS_SERVER_CONTEXT* UNITY_INTERFACE_API libqs_api_get_server_context(void* event_parameter)
{
	return api_qs_get_server_context(event_parameter);
}

UNITY_INTERFACE_EXPORT uint32_t UNITY_INTERFACE_API libqs_api_get_connection_offset(void* event_parameter)
{
	return api_qs_get_connection_offset(event_parameter);
}

UNITY_INTERFACE_EXPORT time_t UNITY_INTERFACE_API libqs_api_get_connection_create_time(void* event_parameter)
{
	return api_qs_get_connection_create_time(event_parameter);
}

UNITY_INTERFACE_EXPORT uint8_t* UNITY_INTERFACE_API libqs_api_get_connection_data(void* event_parameter)
{
	return api_qs_get_connection_data(event_parameter);
}

UNITY_INTERFACE_EXPORT size_t UNITY_INTERFACE_API libqs_api_get_connection_data_size(void* event_parameter)
{
	return api_qs_get_connection_data_size(event_parameter);
}

UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_get_connection_id(void* event_parameter)
{
	return api_qs_get_connection_id(event_parameter);
}

UNITY_INTERFACE_EXPORT int32_t UNITY_INTERFACE_API libqs_api_set_connection_data(void* event_parameter, uint8_t* data, size_t data_size)
{
	return api_qs_set_connection_data(event_parameter, data, data_size);
}

UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_send_response_with_payload(void* event_parameter, uint32_t payload_type, const char* payload)
{
	api_qs_send_response_with_payload(event_parameter, payload_type, payload);
}

UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_send_response_by_connection_offset(QS_SERVER_CONTEXT* context, uint32_t connection_offset, const char* response)
{
	api_qs_send_response_by_connection_offset(context, connection_offset, response);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_send_http_response_object_json(void* http_request_parameter, QS_JSON_ELEMENT_OBJECT* object, size_t buffer_size)
{
	return api_qs_http_response_json(http_request_parameter, object, buffer_size);
}

UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_get_http_post_json_object(void* event_parameter, QS_JSON_ELEMENT_OBJECT* object)
{
	api_qs_get_http_post_json_object(event_parameter, object);
}

UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_get_ws_message(void* event_parameter)
{
	return api_qs_get_ws_message(event_parameter);
}

UNITY_INTERFACE_EXPORT ssize_t UNITY_INTERFACE_API libqs_api_get_ws_message_size(void* event_parameter)
{
	return api_qs_get_ws_message_size(event_parameter);
}

UNITY_INTERFACE_EXPORT uint8_t UNITY_INTERFACE_API libqs_api_get_ws_opcode(void* event_parameter)
{
	return api_qs_get_ws_opcode(event_parameter);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_send_ws_message(void* event_parameter, const char* message)
{
	return api_qs_send_ws_message(event_parameter, message);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_send_ws_message_plane(void* event_parameter, const char* message)
{
	return api_qs_send_ws_message_plane(event_parameter, message);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_send_ws_text_by_connection_offset(QS_SERVER_CONTEXT* context, uint32_t connection_offset, const char* message)
{
	return api_qs_send_ws_text_by_connection_offset(context, connection_offset, message);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_send_ws_binary(void* event_parameter, const void* data, size_t size)
{
	return api_qs_send_ws_binary(event_parameter, data, size);
}

UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_send_ws_binary_by_connection_offset(QS_SERVER_CONTEXT* context, uint32_t connection_offset, const void* data, size_t size)
{
	api_qs_send_ws_binary_by_connection_offset(context, connection_offset, data, size);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_memory_alloc(QS_MEMORY_CONTEXT* context, size_t alloc_size)
{
	return api_qs_memory_alloc(context, alloc_size);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_memory_clean(QS_MEMORY_CONTEXT* context)
{
	return api_qs_memory_clean(context);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_memory_free(QS_MEMORY_CONTEXT* context)
{
	return api_qs_memory_free(context);
}

UNITY_INTERFACE_EXPORT size_t UNITY_INTERFACE_API libqs_api_memory_available_size(QS_MEMORY_CONTEXT* context)
{
	return api_qs_memory_available_size(context);
}

UNITY_INTERFACE_EXPORT int32_t UNITY_INTERFACE_API libqs_api_memory_create_block(QS_MEMORY_CONTEXT* context, size_t size)
{
	return api_qs_memory_create_block(context, size);
}

UNITY_INTERFACE_EXPORT void* UNITY_INTERFACE_API libqs_api_memory_get_pointer(QS_MEMORY_CONTEXT* context, int32_t memid)
{
	return api_qs_memory_get_pointer(context, memid);
}

UNITY_INTERFACE_EXPORT void* UNITY_INTERFACE_API libqs_api_memory_get_offset_pointer(QS_MEMORY_CONTEXT* context, int32_t memid, size_t size, int32_t offset)
{
	return api_qs_memory_get_offset_pointer(context, memid, size, offset);
}

UNITY_INTERFACE_EXPORT size_t UNITY_INTERFACE_API libqs_api_memory_get_size(QS_MEMORY_CONTEXT* context, int32_t memid)
{
	return api_qs_memory_get_size(context, memid);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_memory_free_block(QS_MEMORY_CONTEXT* context, int32_t* memid)
{
	return api_qs_memory_free_block(context, memid);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_array_create(QS_MEMORY_CONTEXT* context, QS_JSON_ELEMENT_ARRAY* array)
{
	return api_qs_array_create(context, array);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_array_push_integer(QS_JSON_ELEMENT_ARRAY* array, int32_t value)
{
	return api_qs_array_push_integer(array, value);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_array_push_big_integer(QS_JSON_ELEMENT_ARRAY* array, int64_t value)
{
	return api_qs_array_push_big_integer(array, value);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_array_push_unsigned_big_integer(QS_JSON_ELEMENT_ARRAY* array, uint64_t value)
{
	return api_qs_array_push_unsigned_big_integer(array, value);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_array_push_string(QS_JSON_ELEMENT_ARRAY* array, const char* value)
{
	return api_qs_array_push_string(array, value);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_array_push_object(QS_JSON_ELEMENT_ARRAY* array, QS_JSON_ELEMENT_OBJECT* object)
{
	return api_qs_array_push_object(array, object);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_array_push_array(QS_JSON_ELEMENT_ARRAY* array, QS_JSON_ELEMENT_ARRAY* push_array)
{
	return api_qs_array_push_array(array, push_array);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_object_create(QS_MEMORY_CONTEXT* context, QS_JSON_ELEMENT_OBJECT* object)
{
	return api_qs_object_create(context, object);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_object_push_integer(QS_JSON_ELEMENT_OBJECT* object, const char* name, int32_t value)
{
	return api_qs_object_push_integer(object, name, value);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_object_push_big_integer(QS_JSON_ELEMENT_OBJECT* object, const char* name, int64_t value)
{
	return api_qs_object_push_big_integer(object, name, value);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_object_push_unsigned_big_integer(QS_JSON_ELEMENT_OBJECT* object, const char* name, uint64_t value)
{
	return api_qs_object_push_unsigned_big_integer(object, name, value);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_object_push_string(QS_JSON_ELEMENT_OBJECT* object, const char* name, const char* value)
{
	return api_qs_object_push_string(object, name, value);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_object_push_array(QS_JSON_ELEMENT_OBJECT* object, const char* name, QS_JSON_ELEMENT_ARRAY* array)
{
	return api_qs_object_push_array(object, name, array);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_object_push_object(QS_JSON_ELEMENT_OBJECT* object, const char* name, QS_JSON_ELEMENT_OBJECT* push_object)
{
	return api_qs_object_push_object(object, name, push_object);
}

UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_json_encode_object(QS_JSON_ELEMENT_OBJECT* object, size_t buffer_size)
{
	return api_qs_json_encode_object(object, buffer_size);
}

UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_json_encode_array(QS_JSON_ELEMENT_ARRAY* array, size_t buffer_size)
{
	return api_qs_json_encode_array(array, buffer_size);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_json_decode_object(QS_MEMORY_CONTEXT* context, QS_JSON_ELEMENT_OBJECT* object, const char* json)
{
	return api_qs_json_decode_object(context, object, json);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_object_exist(QS_JSON_ELEMENT_OBJECT* object, const char* name)
{
	return api_qs_object_exist(object, name);
}

UNITY_INTERFACE_EXPORT int32_t UNITY_INTERFACE_API libqs_api_object_get_integer_val(QS_JSON_ELEMENT_OBJECT* object, const char* name)
{
	return api_qs_object_get_integer_val(object, name);
}

UNITY_INTERFACE_EXPORT int64_t UNITY_INTERFACE_API libqs_api_object_get_big_integer_val(QS_JSON_ELEMENT_OBJECT* object, const char* name)
{
	return api_qs_object_get_big_integer_val(object, name);
}

UNITY_INTERFACE_EXPORT uint64_t UNITY_INTERFACE_API libqs_api_object_get_unsigned_big_integer_val(QS_JSON_ELEMENT_OBJECT* object, const char* name)
{
	return api_qs_object_get_unsigned_big_integer_val(object, name);
}

UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_object_get_string(QS_JSON_ELEMENT_OBJECT* object, const char* name)
{
	return api_qs_object_get_string(object, name);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_object_get_array(QS_JSON_ELEMENT_OBJECT* object, const char* name, QS_JSON_ELEMENT_ARRAY* dst_array)
{
	return api_qs_object_get_array(object, name, dst_array);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_object_get_object(QS_JSON_ELEMENT_OBJECT* object, const char* name, QS_JSON_ELEMENT_OBJECT* dst_object)
{
	return api_qs_object_get_object(object, name, dst_object);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_object_get_keys(QS_JSON_ELEMENT_OBJECT* object, QS_JSON_ELEMENT_ARRAY* dst_array)
{
	return api_qs_object_get_keys(object, dst_array);
}

UNITY_INTERFACE_EXPORT int32_t UNITY_INTERFACE_API libqs_api_array_get_length(QS_JSON_ELEMENT_ARRAY* object)
{
	return api_qs_array_get_length(object);
}

UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_array_get_string(QS_JSON_ELEMENT_ARRAY* object, int32_t offset)
{
	return api_qs_array_get_string(object, offset);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_array_get_array(QS_JSON_ELEMENT_ARRAY* object, int32_t offset, QS_JSON_ELEMENT_ARRAY* dst_array)
{
	return api_qs_array_get_array(object, offset, dst_array);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_array_get_object(QS_JSON_ELEMENT_ARRAY* object, int32_t offset, QS_JSON_ELEMENT_OBJECT* dst_object)
{
	return api_qs_array_get_object(object, offset, dst_object);
}

UNITY_INTERFACE_EXPORT int32_t* UNITY_INTERFACE_API libqs_api_array_get_integer(QS_JSON_ELEMENT_ARRAY* object, int32_t offset)
{
	return api_qs_array_get_integer(object, offset);
}

UNITY_INTERFACE_EXPORT int64_t* UNITY_INTERFACE_API libqs_api_array_get_big_integer(QS_JSON_ELEMENT_ARRAY* object, int32_t offset)
{
	return api_qs_array_get_big_integer(object, offset);
}

UNITY_INTERFACE_EXPORT uint64_t* UNITY_INTERFACE_API libqs_api_array_get_unsigned_big_integer(QS_JSON_ELEMENT_ARRAY* object, int32_t offset)
{
	return api_qs_array_get_unsigned_big_integer(object, offset);
}

UNITY_INTERFACE_EXPORT int32_t* UNITY_INTERFACE_API libqs_api_object_get_integer(QS_JSON_ELEMENT_OBJECT* object, const char* name)
{
	return api_qs_object_get_integer(object, name);
}

UNITY_INTERFACE_EXPORT int64_t* UNITY_INTERFACE_API libqs_api_object_get_big_integer(QS_JSON_ELEMENT_OBJECT* object, const char* name)
{
	return api_qs_object_get_big_integer(object, name);
}

UNITY_INTERFACE_EXPORT uint64_t* UNITY_INTERFACE_API libqs_api_object_get_unsigned_big_integer(QS_JSON_ELEMENT_OBJECT* object, const char* name)
{
	return api_qs_object_get_unsigned_big_integer(object, name);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_csv_read_file(QS_MEMORY_CONTEXT* context, QS_CSV_CONTEXT* csv, const char* csv_file_path)
{
	return api_qs_csv_read_file(context, csv, csv_file_path);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_csv_parse(QS_MEMORY_CONTEXT* context, QS_CSV_CONTEXT* csv, const char* src_csv)
{
	return api_qs_csv_parse(context, csv, src_csv);
}

UNITY_INTERFACE_EXPORT int32_t UNITY_INTERFACE_API libqs_api_csv_get_line_length(QS_CSV_CONTEXT* csv)
{
	return api_qs_csv_get_line_length(csv);
}

UNITY_INTERFACE_EXPORT int32_t UNITY_INTERFACE_API libqs_api_csv_get_row_length(QS_CSV_CONTEXT* csv, int32_t line_pos)
{
	return api_qs_csv_get_row_length(csv, line_pos);
}

UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_csv_get_row(QS_CSV_CONTEXT* csv, int32_t line_pos, int32_t row_pos)
{
	return api_qs_csv_get_row(csv, line_pos, row_pos);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_create_b1mb(QS_MEMORY_CONTEXT* memory_context, QS_KVS_CONTEXT* kvs_context)
{
	return api_qs_kvs_create_b1mb(memory_context, kvs_context);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_create_b8mb(QS_MEMORY_CONTEXT* memory_context, QS_KVS_CONTEXT* kvs_context)
{
	return api_qs_kvs_create_b8mb(memory_context, kvs_context);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_create_b16mb(QS_MEMORY_CONTEXT* memory_context, QS_KVS_CONTEXT* kvs_context)
{
	return api_qs_kvs_create_b16mb(memory_context, kvs_context);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_create_b32mb(QS_MEMORY_CONTEXT* memory_context, QS_KVS_CONTEXT* kvs_context)
{
	return api_qs_kvs_create_b32mb(memory_context, kvs_context);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_create_b64mb(QS_MEMORY_CONTEXT* memory_context, QS_KVS_CONTEXT* kvs_context)
{
	return api_qs_kvs_create_b64mb(memory_context, kvs_context);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_create_b128mb(QS_MEMORY_CONTEXT* memory_context, QS_KVS_CONTEXT* kvs_context)
{
	return api_qs_kvs_create_b128mb(memory_context, kvs_context);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_create_b256mb(QS_MEMORY_CONTEXT* memory_context, QS_KVS_CONTEXT* kvs_context)
{
	return api_qs_kvs_create_b256mb(memory_context, kvs_context);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_create_b512mb(QS_MEMORY_CONTEXT* memory_context, QS_KVS_CONTEXT* kvs_context)
{
	return api_qs_kvs_create_b512mb(memory_context, kvs_context);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_create_b1024mb(QS_MEMORY_CONTEXT* memory_context, QS_KVS_CONTEXT* kvs_context)
{
	return api_qs_kvs_create_b1024mb(memory_context, kvs_context);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_create_b1mb_persistence(QS_KVS_CONTEXT* kvs_context, const char* file_path)
{
	return api_qs_kvs_create_b1mb_persistence(kvs_context, file_path);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_create_b8mb_persistence(QS_KVS_CONTEXT* kvs_context, const char* file_path)
{
	return api_qs_kvs_create_b8mb_persistence(kvs_context, file_path);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_create_b16mb_persistence(QS_KVS_CONTEXT* kvs_context, const char* file_path)
{
	return api_qs_kvs_create_b16mb_persistence(kvs_context, file_path);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_create_b32mb_persistence(QS_KVS_CONTEXT* kvs_context, const char* file_path)
{
	return api_qs_kvs_create_b32mb_persistence(kvs_context, file_path);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_create_b64mb_persistence(QS_KVS_CONTEXT* kvs_context, const char* file_path)
{
	return api_qs_kvs_create_b64mb_persistence(kvs_context, file_path);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_create_b128mb_persistence(QS_KVS_CONTEXT* kvs_context, const char* file_path)
{
	return api_qs_kvs_create_b128mb_persistence(kvs_context, file_path);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_create_b256mb_persistence(QS_KVS_CONTEXT* kvs_context, const char* file_path)
{
	return api_qs_kvs_create_b256mb_persistence(kvs_context, file_path);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_create_b512mb_persistence(QS_KVS_CONTEXT* kvs_context, const char* file_path)
{
	return api_qs_kvs_create_b512mb_persistence(kvs_context, file_path);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_create_b1024mb_persistence(QS_KVS_CONTEXT* kvs_context, const char* file_path)
{
	return api_qs_kvs_create_b1024mb_persistence(kvs_context, file_path);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_set(QS_KVS_CONTEXT* kvs_context, const char* key, const char* value, int32_t life_time)
{
	return api_qs_kvs_set(kvs_context, key, value, life_time);
}

UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_kvs_get(QS_KVS_CONTEXT* kvs_context, const char* key)
{
	return api_qs_kvs_get(kvs_context, key);
}

UNITY_INTERFACE_EXPORT size_t UNITY_INTERFACE_API libqs_api_kvs_get_buffer_size(QS_KVS_CONTEXT* kvs_context, const char* key)
{
	return api_qs_kvs_get_buffer_size(kvs_context, key);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_kvs_delete(QS_KVS_CONTEXT* kvs_context, const char* key)
{
	return api_qs_kvs_delete(kvs_context, key);
}

UNITY_INTERFACE_EXPORT int32_t UNITY_INTERFACE_API libqs_api_kvs_key_length(QS_KVS_CONTEXT* kvs_context)
{
	return api_qs_kvs_key_length(kvs_context);
}

UNITY_INTERFACE_EXPORT int32_t UNITY_INTERFACE_API libqs_api_kvs_keys(QS_JSON_ELEMENT_ARRAY* array, QS_KVS_CONTEXT* kvs_context)
{
	return api_qs_kvs_keys(array, kvs_context);
}

UNITY_INTERFACE_EXPORT int32_t UNITY_INTERFACE_API libqs_api_kvs_sorted_keys(QS_JSON_ELEMENT_ARRAY* array, QS_KVS_CONTEXT* kvs_context, int32_t is_sort_asc)
{
	return api_qs_kvs_sorted_keys(array, kvs_context, is_sort_asc);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_persistence_kvs_memory_free(QS_KVS_CONTEXT* kvs_context)
{
	return api_qs_persistence_kvs_memory_free(kvs_context);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_logger_create(QS_MEMORY_CONTEXT* context, QS_LOGGER_CONTEXT* logger, int32_t level)
{
	return api_qs_logger_create(context, logger, level);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_logger_open(QS_LOGGER_CONTEXT* logger, const char* file_path)
{
	return api_qs_logger_open(logger, file_path);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_logger_close(QS_LOGGER_CONTEXT* logger)
{
	return api_qs_logger_close(logger);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_logger_free(QS_LOGGER_CONTEXT* logger)
{
	return api_qs_logger_free(logger);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_logger_set_level(QS_LOGGER_CONTEXT* logger, int32_t level)
{
	return api_qs_logger_set_level(logger, level);
}

UNITY_INTERFACE_EXPORT int32_t UNITY_INTERFACE_API libqs_api_logger_get_level(QS_LOGGER_CONTEXT* logger)
{
	return api_qs_logger_get_level(logger);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_logger_write(QS_LOGGER_CONTEXT* logger, int32_t level, const char* file, int32_t line, const char* message)
{
	return api_qs_logger_write(logger, level, file, line, "%s", message);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_logger_debug(QS_LOGGER_CONTEXT* logger, const char* message)
{
	return api_qs_logger_debug(logger, "%s", message);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_logger_info(QS_LOGGER_CONTEXT* logger, const char* message)
{
	return api_qs_logger_info(logger, "%s", message);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_logger_warn(QS_LOGGER_CONTEXT* logger, const char* message)
{
	return api_qs_logger_warn(logger, "%s", message);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_logger_error(QS_LOGGER_CONTEXT* logger, const char* message)
{
	return api_qs_logger_error(logger, "%s", message);
}

UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_uniqid(QS_MEMORY_CONTEXT* memory_context, int32_t length)
{
	return api_qs_uniqid(memory_context, length);
}

UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_base64_encode(QS_MEMORY_CONTEXT* memory_context, const void* data, size_t length)
{
	return api_qs_base64_encode(memory_context, data, length);
}

UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_base64_decode(QS_MEMORY_CONTEXT* memory_context, const char* data)
{
	return api_qs_base64_decode(memory_context, data);
}

UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_sha1_encode(QS_MEMORY_CONTEXT* memory_context, const void* data, size_t length)
{
	return api_qs_sha1_encode(memory_context, data, length);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_json_escape(const char* src, char* dst, size_t dst_size)
{
	return api_qs_json_escape(src, dst, dst_size);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_server_init(QS_SERVER_CONTEXT** ppcontext, int port, int32_t max_connection, int32_t server_type)
{
	return api_qs_server_init(ppcontext, port, max_connection, server_type);
}

UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_server_memory_info(QS_SERVER_CONTEXT* context)
{
	api_qs_server_memory_info(context);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_server_get_socket(QS_SERVER_CONTEXT* context)
{
	return api_qs_server_get_socket(context);
}

UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_router_memory_info(QS_SERVER_CONTEXT* context)
{
	api_qs_router_memory_info(context);
}

UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_kvs_memory_info(QS_SERVER_CONTEXT* context)
{
	api_qs_kvs_memory_info(context);
}

UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_server_set_lowlevel_send_hook(QS_SERVER_CONTEXT* context, QS_SOCKET_SEND_HOOK func)
{
	api_qs_server_set_lowlevel_send_hook(context, func);
}

UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_server_set_lowlevel_recv_hook(QS_SERVER_CONTEXT* context, QS_SOCKET_RECV_HOOK func)
{
	api_qs_server_set_lowlevel_recv_hook(context, func);
}

UNITY_INTERFACE_EXPORT uint32_t UNITY_INTERFACE_API libqs_api_get_plain_payload_type(void* event_parameter)
{
	return api_qs_get_plain_payload_type(event_parameter);
}

UNITY_INTERFACE_EXPORT uint8_t* UNITY_INTERFACE_API libqs_api_get_plain_payload(void* event_parameter)
{
	return api_qs_get_plain_payload(event_parameter);
}

UNITY_INTERFACE_EXPORT size_t UNITY_INTERFACE_API libqs_api_get_plain_payload_length(void* event_parameter)
{
	return api_qs_get_plain_payload_length(event_parameter);
}

UNITY_INTERFACE_EXPORT uint8_t* UNITY_INTERFACE_API libqs_api_get_websocket_payload(void* event_parameter)
{
	return api_qs_get_websocket_payload(event_parameter);
}

UNITY_INTERFACE_EXPORT size_t UNITY_INTERFACE_API libqs_api_get_websocket_payload_length(void* event_parameter)
{
	return api_qs_get_websocket_payload_length(event_parameter);
}

UNITY_INTERFACE_EXPORT uint8_t UNITY_INTERFACE_API libqs_api_get_websocket_opcode(void* event_parameter)
{
	return api_qs_get_websocket_opcode(event_parameter);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_http_response_json(void* event_parameter, QS_JSON_ELEMENT_OBJECT* object, size_t buffer_size)
{
	return api_qs_http_response_json(event_parameter, object, buffer_size);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_http_response_raw_json(void* event_parameter, QS_MEMORY_CONTEXT* temporary_memory_ptr, char* json)
{
	return api_qs_http_response_raw_json(event_parameter, temporary_memory_ptr, json);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_client_init(QS_CLIENT_CONTEXT** ppcontext, const char* host, int port, int32_t server_type)
{
	return api_qs_client_init(ppcontext, host, port, server_type);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_websocket_client_init(QS_CLIENT_CONTEXT** ppcontext, const char* host, int port, const char* path)
{
	return api_qs_websocket_client_init(ppcontext, host, port, path);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_client_get_socket(QS_CLIENT_CONTEXT* context)
{
	return api_qs_client_get_socket(context);
}

UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_set_client_on_connect_event(QS_CLIENT_CONTEXT* context, QS_EVENT_FUNCTION on_connect)
{
	api_qs_set_client_on_connect_event(context, on_connect);
}

UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_set_client_on_plain_event(QS_CLIENT_CONTEXT* context, QS_EVENT_FUNCTION on_plain_event)
{
	api_qs_set_client_on_plain_event(context, on_plain_event);
}

UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_set_client_on_simple_event(QS_CLIENT_CONTEXT* context, QS_EVENT_FUNCTION on_simple_event)
{
	api_qs_set_client_on_simple_event(context, on_simple_event);
}

UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_set_client_on_close_event(QS_CLIENT_CONTEXT* context, QS_EVENT_FUNCTION on_close)
{
	api_qs_set_client_on_close_event(context, on_close);
}

UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_set_websocket_client_on_event(QS_CLIENT_CONTEXT* context, QS_EVENT_FUNCTION on_event)
{
	api_qs_set_websocket_client_on_event(context, on_event);
}

UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_client_update(QS_CLIENT_CONTEXT* context)
{
	api_qs_client_update(context);
}

UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_client_sleep(QS_CLIENT_CONTEXT* context)
{
	api_qs_client_sleep(context);
}

UNITY_INTERFACE_EXPORT QS_CLIENT_CONTEXT* UNITY_INTERFACE_API libqs_api_client_get_context(void* event_parameter)
{
	return api_qs_client_get_context(event_parameter);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_client_send(QS_CLIENT_CONTEXT* context, const char* payload, size_t payload_len)
{
	return api_qs_client_send(context, payload, payload_len);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_client_send_message(QS_CLIENT_CONTEXT* context, uint32_t payload_type, const char* payload, size_t payload_len)
{
	return api_qs_client_send_message(context, payload_type, payload, payload_len);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_websocket_client_send(QS_CLIENT_CONTEXT* context, int is_binary, const void* payload, size_t payload_len)
{
	return api_qs_websocket_client_send(context, is_binary, payload, payload_len);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_websocket_client_close(QS_CLIENT_CONTEXT* context)
{
	return api_qs_websocket_client_close(context);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_websocket_client_close_with_reason(QS_CLIENT_CONTEXT* context, uint16_t status_code, const char* reason)
{
	return api_qs_websocket_client_close_with_reason(context, status_code, reason);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_websocket_client_set_max_message_size(QS_CLIENT_CONTEXT* context, size_t max_message_size)
{
	return api_qs_websocket_client_set_max_message_size(context, max_message_size);
}

UNITY_INTERFACE_EXPORT size_t UNITY_INTERFACE_API libqs_api_websocket_client_get_max_message_size(QS_CLIENT_CONTEXT* context)
{
	return api_qs_websocket_client_get_max_message_size(context);
}

UNITY_INTERFACE_EXPORT QS_WEBSOCKET_STATE UNITY_INTERFACE_API libqs_api_websocket_client_get_state(QS_CLIENT_CONTEXT* context)
{
	return api_qs_websocket_client_get_state(context);
}

UNITY_INTERFACE_EXPORT QS_WEBSOCKET_ERROR UNITY_INTERFACE_API libqs_api_websocket_client_get_error(QS_CLIENT_CONTEXT* context)
{
	return api_qs_websocket_client_get_error(context);
}

UNITY_INTERFACE_EXPORT uint16_t UNITY_INTERFACE_API libqs_api_websocket_client_get_close_code(QS_CLIENT_CONTEXT* context)
{
	return api_qs_websocket_client_get_close_code(context);
}

UNITY_INTERFACE_EXPORT const char* UNITY_INTERFACE_API libqs_api_websocket_client_get_close_reason(QS_CLIENT_CONTEXT* context)
{
	return api_qs_websocket_client_get_close_reason(context);
}

UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_client_free(QS_CLIENT_CONTEXT* context)
{
	api_qs_client_free(context);
}

UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_client_set_lowlevel_send_hook(QS_CLIENT_CONTEXT* context, QS_SOCKET_SEND_HOOK func)
{
	api_qs_client_set_lowlevel_send_hook(context, func);
}

UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_client_set_lowlevel_recv_hook(QS_CLIENT_CONTEXT* context, QS_SOCKET_RECV_HOOK func)
{
	api_qs_client_set_lowlevel_recv_hook(context, func);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_script_read_file(QS_MEMORY_CONTEXT* memory_context, QS_SERVER_SCRIPT_CONTEXT* script_context, const char* file_path)
{
	return api_qs_script_read_file(memory_context, script_context, file_path);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_script_set_argv_object(QS_SERVER_SCRIPT_CONTEXT* script_context, const char* name, QS_JSON_ELEMENT_OBJECT* object)
{
	return api_qs_script_set_argv_object(script_context, name, object);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_script_set_argv_string(QS_SERVER_SCRIPT_CONTEXT* script_context, const char* name, const char* value)
{
	return api_qs_script_set_argv_string(script_context, name, value);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_script_set_argv_integer(QS_SERVER_SCRIPT_CONTEXT* script_context, const char* name, int32_t value)
{
	return api_qs_script_set_argv_integer(script_context, name, value);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_script_run(QS_SERVER_SCRIPT_CONTEXT* script_context)
{
	return api_qs_script_run(script_context);
}

UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_script_get_parameter(QS_SERVER_SCRIPT_CONTEXT* script_context, const char* name)
{
	return api_qs_script_get_parameter(script_context, name);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_room_create(QS_SERVER_CONTEXT* context, const char* name, QS_MEMORY_CONTEXT* dest_memory, QS_JSON_ELEMENT_OBJECT* dest_object)
{
	return api_qs_room_create(context, name, dest_memory, dest_object);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_room_list(QS_SERVER_CONTEXT* context, QS_MEMORY_CONTEXT* dest_memory, QS_JSON_ELEMENT_OBJECT* dest_object)
{
	return api_qs_room_list(context, dest_memory, dest_object);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_room_join(QS_SERVER_CONTEXT* context, const char* room_id, const char* connection_id, QS_MEMORY_CONTEXT* dest_memory, QS_JSON_ELEMENT_OBJECT* dest_object)
{
	return api_qs_room_join(context, room_id, connection_id, dest_memory, dest_object);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_room_leave(QS_SERVER_CONTEXT* context, const char* room_id, const char* connection_id, QS_MEMORY_CONTEXT* dest_memory, QS_JSON_ELEMENT_OBJECT* dest_object)
{
	return api_qs_room_leave(context, room_id, connection_id, dest_memory, dest_object);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_ranking_create(QS_MEMORY_CONTEXT* memory_context, QS_RANKING_CONTEXT* ranking_context, size_t size, int32_t key_size, int32_t get_max, int32_t refresh_size)
{
	return api_qs_ranking_create(memory_context, ranking_context, size, key_size, get_max, refresh_size);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_ranking_entry(QS_RANKING_CONTEXT* ranking_context, const char* id)
{
	return api_qs_ranking_entry(ranking_context, id);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_ranking_set_value(QS_RANKING_CONTEXT* ranking_context, const char* id, uint32_t value)
{
	return api_qs_ranking_set_value(ranking_context, id, value);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_ranking_add_value(QS_RANKING_CONTEXT* ranking_context, const char* id, uint32_t value)
{
	return api_qs_ranking_add_value(ranking_context, id, value);
}

UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API libqs_api_ranking_sort(QS_RANKING_CONTEXT* ranking_context)
{
	return api_qs_ranking_sort(ranking_context);
}

UNITY_INTERFACE_EXPORT char* UNITY_INTERFACE_API libqs_api_ranking_get(QS_RANKING_CONTEXT* ranking_context, int32_t offset, int32_t length, QS_MEMORY_CONTEXT* dest_memory)
{
	return api_qs_ranking_get(ranking_context, offset, length, dest_memory);
}

UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API libqs_api_memory_info(QS_MEMORY_CONTEXT* context)
{
	api_qs_memory_info(context);
}


void libqs_api_debug_log(char* str)
{
    if (debug_log)
    {
        debug_log(str);
    }
}

