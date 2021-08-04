#ifdef __cplusplus
extern "C"{
#endif

#ifndef _QS_PROTOCOL_H_
#define _QS_PROTOCOL_H_

#include "qs_core.h"
#include "qs_memory_allocator.h"
#include "qs_socket.h"
#include "qs_variable.h"

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

#define HTTP_HEADER_STRINGS { \
	{"GET_PARAMS","GET_PARAMS","2048"}, \
	{"REQUEST","REQUEST","2048"}, \
	{"HTTP_VERSION","HTTP_VERSION","32"}, \
	{"HTTP_METHOD","HTTP_METHOD","32"}, \
	{"Sec-WebSocket-Key:","SEC_WEBSOCKET_KEY","256"}, \
	{"Host:","HOST","512"}, \
	{"User-Agent:","USER_AGENT","512"}, \
	{"Sec-WebSocket-Protocol:","SEC_WEBSOCKET_PROTOCOL","128"}, \
	{"Content-Length:","CONTENT_LENGTH","64"}, \
	{"Content-Type:","CONTENT_TYPE","256"}, \
	{"Connection:","CONNECTION","64"}, \
	{"Cache-Control:","CACHE_CONTROL","256"}, \
	{"Referer:","REFERER","2048"}, \
	{"If-Modified-Since:","IF_MODIFIED_SINCE","64"}, \
}; \

typedef struct QS_HTTP_REQUEST_COMMON
{
	QS_MEMORY_POOL * temporary_memory;
	int32_t memid_get_parameter_hash;
	int32_t memid_post_parameter_hash;
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
	QS_FILE_INFO file_info;
} QS_HTTP_REQUEST_COMMON;

ssize_t qs_get_protocol_buffer_size(ssize_t payload_size);
uint8_t qs_get_protocol_header_size_byte(ssize_t payload_size);
uint32_t qs_get_protocol_header_size(ssize_t payload_size);
uint32_t qs_make_protocol_header(uint8_t* bin,ssize_t payload_size, uint32_t payload_type, uint32_t seq_num);
ssize_t qs_make_protocol_buffer(uint8_t* bin, uint8_t* payload,ssize_t payload_size, uint32_t payload_type, uint32_t seq_num);

int qs_http_protocol_filter(QS_RECV_INFO* rinfo);
int qs_http_parser( QS_RECV_INFO *rinfo );
int qs_http_parse_header( QS_RECV_INFO *rinfo );
size_t qs_http_add_response_common(char* dest, size_t dest_size, int http_response_code, char* content_type, size_t content_length);
size_t qs_http_add_cache_control(char* dest, size_t dest_size, size_t start, int max_age, QS_FILE_INFO* info);
size_t qs_http_document_path(char* dest, size_t dest_size,char* document_root, char* default_file, char* path);
int32_t qs_http_parse_request_parameter(QS_MEMORY_POOL * memory,char *get_params);
int32_t http_request_common(QS_RECV_INFO *rinfo, QS_HTTP_REQUEST_COMMON* http_request, QS_MEMORY_POOL* temporary_memory);

#endif /*_QS_PROTOCOL_H_*/

#ifdef __cplusplus
}
#endif
