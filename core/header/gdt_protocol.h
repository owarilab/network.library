#ifdef __cplusplus
extern "C"{
#endif

#ifndef _GDT_PROTOCOL_H_
#define _GDT_PROTOCOL_H_

#include "gdt_core.h"
#include "gdt_memory_allocator.h"
#include "gdt_socket.h"

// HTTP method
#define HTTP_METHOD_GET		1;
#define HTTP_METHOD_HEAD	2;
#define HTTP_METHOD_POST	3;

#define HTTP_INTERNAL_SERVER_ERROR "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n500 Internal Server Error\r\n"
#define HTTP_REQUEST_URI_TOO_LONG_ERROR "HTTP/1.1 414 Request-URI Too Long\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n414 Request-URI Too Long\r\n"
#define HTTP_BAD_REQUEST_ERROR "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n400 Bad Request\r\n"
#define HTTP_NOT_MODIFIED "HTTP/1.1 304 Not Modified\r\nConnection: close\r\n\r\n"
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


ssize_t gdt_get_protocol_buffer_size(ssize_t payload_size);
uint8_t gdt_get_protocol_header_size_byte(ssize_t payload_size);
uint32_t gdt_get_protocol_header_size(ssize_t payload_size);
uint32_t gdt_make_protocol_header(uint8_t* bin,ssize_t payload_size, uint32_t payload_type, uint32_t seq_num);
ssize_t gdt_make_protocol_buffer(uint8_t* bin, uint8_t* payload,ssize_t payload_size, uint32_t payload_type, uint32_t seq_num);

int gdt_http_protocol_filter(GDT_RECV_INFO* rinfo);
int gdt_http_parser( GDT_RECV_INFO *rinfo );
int gdt_http_parse_header( GDT_RECV_INFO *rinfo );
size_t gdt_http_add_response_common(char* dest, size_t dest_size, int http_response_code, char* content_type, size_t content_length);
size_t gdt_http_add_cache_control(char* dest, size_t dest_size, size_t start, int max_age, GDT_FILE_INFO* info);

#endif /*_GDT_PROTOCOL_H_*/

#ifdef __cplusplus
}
#endif
