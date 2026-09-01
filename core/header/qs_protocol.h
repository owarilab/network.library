/*
 * Copyright (c) Katsuya Owari
 */

#ifdef __cplusplus
extern "C"{
#endif

#ifndef _QS_PROTOCOL_H_
#define _QS_PROTOCOL_H_

#include "qs_core.h"
#include "qs_memory_allocator.h"
#include "qs_logger.h"
#include "qs_socket.h"
#include "qs_variable.h"

#include "qs_sha1.h"
#include "qs_base64.h"

// HTTP method
#define HTTP_METHOD_GET		1;
#define HTTP_METHOD_HEAD	2;
#define HTTP_METHOD_POST	3;

#define HTTP_INTERNAL_SERVER_ERROR "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n500 Internal Server Error\r\n"
#define HTTP_REQUEST_URI_TOO_LONG_ERROR "HTTP/1.1 414 Request-URI Too Long\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n414 Request-URI Too Long\r\n"
#define HTTP_BAD_REQUEST_ERROR "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n400 Bad Request\r\n"
#define HTTP_NOT_MODIFIED "HTTP/1.1 304 Not Modified\r\nConnection: close\r\n\r\n"
#define HTTP_NOT_FOUND "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n"
#define HTTP_OK "HTTP/1.1 200 OK\r\n"

// HTTP PROTOCOL PHASE
#define QS_HTTP_SOCK_PHASE_RECV_CONTINUE 0
#define QS_HTTP_SOCK_PHASE_PARSE_HTTP_HEADER 1
#define QS_HTTP_SOCK_PHASE_MSG_HTTP 2
#define QS_HTTP_SOCK_PHASE_HANDSHAKE_WEBSOCKET 3
#define QS_HTTP_SOCK_PHASE_MSG_WEBSOCKET 4

// WEBSOCKET MESSAGE MODE
#define WS_MODE_TEXT 1
#define WS_MODE_BINARY 2

typedef struct QS_HTTP_REQUEST_COMMON
{
	QS_MEMORY_POOL * temporary_memory;
	int32_t memid_get_parameter_hash;
	int32_t memid_post_parameter_hash;
	int32_t memid_cookie_hash;
	int32_t http_status_code;
	char *method;
	char *request;
	char *get_params;
	char *content_type;
	char *cache_control;
	char *http_version;
	char *user_agent;
	char *request_path;
	char *extension;
	char *from_ip;
	char *cookie;
	QS_FILE_INFO file_info;
} QS_HTTP_REQUEST_COMMON;

typedef struct QS_HTTP_CLIENT
{
	QS_MEMORY_POOL* memory;
	int32_t memid_http_request_string;
	int32_t memid_http_header_name_array;
	int32_t memid_http_header_hash;
	size_t buffer_size;
	size_t buffer_pos;
	int32_t memid_temp_string;
	size_t temp_buffer_size;
} QS_HTTP_CLIENT;

typedef struct QS_WEBSOCKET_CLIENT
{
	int32_t websocket_handshake_complete;
	QS_WEBSOCKET_STATE websocket_state;
	QS_WEBSOCKET_ERROR websocket_error;
	char websocket_key[64];
	char websocket_accept[64];
	char websocket_host[1024];
	char websocket_path[1024];
	uint8_t websocket_buffer[QS_WEBSOCKET_MAX_MESSAGE_SIZE + 14];
	size_t websocket_buffer_size;
	uint8_t websocket_opcode;
	size_t websocket_payload_size;
	size_t websocket_payload_offset;
	uint8_t* websocket_payload;
	uint8_t websocket_fragment_opcode;
	int32_t websocket_fragment_active;
	size_t websocket_fragment_size;
	uint8_t websocket_fragment_buffer[QS_WEBSOCKET_MAX_MESSAGE_SIZE];
	size_t websocket_max_message_size;
	uint16_t websocket_close_code;
	char websocket_close_reason[124];
} QS_WEBSOCKET_CLIENT;

ssize_t qs_get_protocol_buffer_size(ssize_t payload_size);
uint8_t qs_get_protocol_header_size_byte(ssize_t payload_size);
uint32_t qs_get_protocol_header_size(ssize_t payload_size);
uint32_t qs_make_protocol_header(uint8_t* bin,ssize_t payload_size, uint32_t payload_type, uint32_t seq_num);
ssize_t qs_make_protocol_buffer(uint8_t* bin, uint8_t* payload,ssize_t payload_size, uint32_t payload_type, uint32_t seq_num);

int qs_http_protocol_filter(QS_RECV_INFO* rinfo);
int qs_http_parser( QS_RECV_INFO *rinfo );
int qs_http_parse_header( QS_RECV_INFO *rinfo, int skip_head );
size_t qs_http_add_response_common(char* dest, size_t dest_size, int http_response_code, char* content_type, size_t content_length);
size_t qs_http_add_cache_control(char* dest, size_t dest_size, size_t start, int max_age, QS_FILE_INFO* info);
size_t qs_http_add_cookie(char* dest, size_t dest_size, size_t start, const char* cookie_name, const char* cookie_value, const char* path, const char* domain, int max_age, int secure, int http_only);
size_t qs_http_document_path(char* dest, size_t dest_size,char* document_root, char* default_file, char* path);
int32_t qs_http_parse_request_parameter(QS_MEMORY_POOL * memory,char *get_params, size_t buffer_size);
int32_t qs_http_parse_cookie_parameter(QS_MEMORY_POOL * memory,char *cookie_params, size_t buffer_size);
int32_t http_request_common(QS_RECV_INFO *rinfo, QS_HTTP_REQUEST_COMMON* http_request, QS_MEMORY_POOL* temporary_memory);
int32_t http_json_response_common(QS_SERVER_CONNECTION_INFO * connection, QS_SOCKET_OPTION* option,QS_MEMORY_POOL* temporary_memory,int32_t memid_response_hash, size_t json_buffer_size);
int32_t http_json_response_send(QS_SERVER_CONNECTION_INFO * connection, QS_SOCKET_OPTION* option,QS_MEMORY_POOL* temporary_memory,char* json);

int32_t qs_http_access_log(QS_FILE_INFO* log_file_info,QS_HTTP_REQUEST_COMMON* http_request,int32_t http_status_code);

int32_t qs_http_client_init(QS_MEMORY_POOL * memory, size_t http_request_buffer_size);
QS_HTTP_CLIENT* qs_http_client_get(QS_MEMORY_POOL * memory, int32_t memid_http_client);
int32_t qs_http_make_request_v1_1(QS_HTTP_CLIENT* client, const char* method,const char* host, const char* path, int32_t content_length);

int qs_http_protocol_filter_with_websocket(QS_RECV_INFO *rinfo);
ssize_t qs_parse_websocket_binary( QS_SOCKET_OPTION *option, QS_SOCKPARAM *psockparam, uint8_t* u8buf, size_t size, uint32_t basebuf_munit );
ssize_t qs_make_websocket_msg( void* message_buffer, size_t message_buffer_size,int is_binary, const char* msg, ssize_t size );
ssize_t qs_make_ws_message_simple(QS_MEMORY_POOL * temporary_memory,char* connection_id,char* type,char* message,void* buffer,size_t buffer_size);
int qs_send_handshake_param(QS_SOCKET_ID socket, QS_SOCKET_OPTION *option, QS_SERVER_CONNECTION_INFO* connection );

int qs_websocket_client_create(QS_WEBSOCKET_CLIENT** client, const char* host, int port, const char* path);
void qs_websocket_client_destroy(QS_WEBSOCKET_CLIENT* client);
int qs_websocket_client_on_connect(QS_WEBSOCKET_CLIENT* client, QS_SOCKET_ID socket);
int qs_websocket_client_on_recv(QS_WEBSOCKET_CLIENT* client, QS_SOCKET_ID socket, const uint8_t* payload, size_t payload_len, int (*on_message)(void* user_data), void* user_data);
int qs_websocket_client_send(QS_WEBSOCKET_CLIENT* client, QS_SOCKET_ID socket, int is_binary, const void* payload, size_t payload_len);
int qs_websocket_client_close(QS_WEBSOCKET_CLIENT* client, QS_SOCKET_ID socket);
int qs_websocket_client_close_with_reason(QS_WEBSOCKET_CLIENT* client, QS_SOCKET_ID socket, uint16_t status_code, const char* reason);
int qs_websocket_client_set_max_message_size(QS_WEBSOCKET_CLIENT* client, size_t max_message_size);
int qs_websocket_client_is_handshake_complete(const QS_WEBSOCKET_CLIENT* client);
void qs_websocket_client_set_closed(QS_WEBSOCKET_CLIENT* client);
size_t qs_websocket_client_get_max_message_size(const QS_WEBSOCKET_CLIENT* client);
QS_WEBSOCKET_STATE qs_websocket_client_get_state(const QS_WEBSOCKET_CLIENT* client);
QS_WEBSOCKET_ERROR qs_websocket_client_get_error(const QS_WEBSOCKET_CLIENT* client);
uint16_t qs_websocket_client_get_close_code(const QS_WEBSOCKET_CLIENT* client);
const char* qs_websocket_client_get_close_reason(const QS_WEBSOCKET_CLIENT* client);
uint8_t* qs_websocket_client_get_payload(const QS_WEBSOCKET_CLIENT* client);
size_t qs_websocket_client_get_payload_length(const QS_WEBSOCKET_CLIENT* client);
uint8_t qs_websocket_client_get_opcode(const QS_WEBSOCKET_CLIENT* client);

#endif /*_QS_PROTOCOL_H_*/

#ifdef __cplusplus
}
#endif
