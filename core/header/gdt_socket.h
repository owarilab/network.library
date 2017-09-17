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

#ifndef _GDT_SOCKET_H_
#define _GDT_SOCKET_H_

#include "gdt_core.h"
#include "gdt_system.h"
#include "gdt_io.h"
#include "gdt_sha1.h"
#include "gdt_base64.h"
#include "gdt_string.h"
#include "gdt_queue.h"
#include "gdt_timeout.h"
#include "gdt_hash.h"
#include "gdt_memory_allocator.h"

#ifdef __WINDOWS__
#include <process.h>
#else
#include <stdint.h>
#include <semaphore.h>
#include <pthread.h>
#include <netinet/in.h>
#include <netdb.h>
#include <poll.h>
#include <sysexits.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#ifdef USE_EPOOL
#include <sys/epoll.h>
#endif
#ifdef USE_KQUEUE
#include <sys/event.h>
#endif
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <sys/file.h>
#ifndef __IOS__
#include <arpa/telnet.h>
#endif
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <net/if.h>
#include <ifaddrs.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

// inetflags
#define INET_FLAG_BIT_IPV6 0x00000001
#define INET_FLAG_BIT_CONNECT_UDP 0x00000002

// sokcet type
#define SOKET_TYPE_SERVER_TCP 1
#define SOKET_TYPE_SERVER_UDP 2
#define SOKET_TYPE_CLIENT_TCP 3
#define SOKET_TYPE_CLIENT_UDP 4

// server mode
#define SOCKET_MODE_SINGLE 0
#define SOCKET_MODE_SELECT 1
#define SOCKET_MODE_POOL 2
#define SOCKET_MODE_EPOOL 3
#define SOCKET_MODE_PREFORK 4
#define SOCKET_MODE_THREAD 5
#define SOCKET_MODE_KQUEUE 6
#define SOCKET_MODE_NONBLOCKING 7

// client mode
#define SOCKET_MODE_SIMPLE_TERM 100
#define SOCKET_MODE_CLIENT_THREAD 101
#define SOCKET_MODE_CLIENT_NONBLOCKING 102

// status
#define PROTOCOL_STATUS_DEFAULT 0
#define PROTOCOL_STATUS_WEBSOCKET_SENDHANDSHAKE 1
#define PROTOCOL_STATUS_WEBSOCKET_RECVHANDSHAKEOK 2
#define PROTOCOL_STATUS_HTTP 3
#define PROTOCOL_STATUS_OTHER 10

// socket phase
#define SOCK_PHASE_MSG_SOCKET 1
#define SOCK_PHASE_REQUEST_HTTP 2
#define SOCK_PHASE_HANDSHAKE_WEBSOCKET 3
#define SOCK_PHASE_MSG_WEBSOCKET 4

// socket type
#define SOCK_TYPE_NORMAL 1			// tcp socket
#define SOCK_TYPE_WEBSOCKET 2		// websocket( tcp only )
#define SOCK_TYPE_HTTP 3			// http
#define SOCK_TYPE_NORMAL_UDP 4		// udp socket

// protocol
#define PROTOCOL_PLAIN 1			// plane
#define PROTOCOL_SIMPLE 2			// simple protocol
#define PROTOCOL_HTTP 3				// http protocol

// WEBSOCKET MESSAGE MODE
#define WS_MODE_TEXT 1
#define WS_MODE_BINARY 2

// HTTP method
#define HTTP_METHOD_GET		1;
#define HTTP_METHOD_HEAD	2;
#define HTTP_METHOD_POST	3;

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
	{"Connection:","CONNECTION","64"}, \
}; \


typedef struct GDT_SOCKPARAM
{
	int32_t http_header_munit;		// http header hash( GDT_HASH* )
	int32_t wsockkey_munit;			// websocket key buffer( char* )
	int32_t c_status;				// status( ROTOCOL_STATUS_* )
	GDT_SOCKET_ID acc;				// client id
	int32_t type;					// socket type( SOCK_TYPE_* )
	int phase;						// connect phase( SOCK_PHASE_* )
	uint8_t ws_msg_mode;			// ( 1 : text, 2 : binary )
	uint8_t fin;					// FIN flag( 1:finish )
	uint8_t rsv;					// RSV flag
	uint8_t opcode;					// opcode( 0:connection, 1:test, 2:binary, 3-7reserved for further, 8:close, 9:ping, a:pong b-f:reserved for further )
	uint8_t mask;					// mask flag( 1:mask enable )
	uint8_t ckpayloadlen;			// payloadサイズ( 1-125:そのままデータサイズ, 126:以降16ビットがデータサイズ, 127:以降64ビットがデータサイズ )
	uint32_t payload_type;			// payload type
	uint64_t payloadlen;			// 実際のpayloadサイズ
	uint32_t maskindex;				// maskキーの格納場所( 1-125:2, 126:4, 127:8 )
	uint32_t payloadmask;			// mask(32bit)
	ssize_t tmpmsglen;				// 受信メッセージサイズ( 受信途中の )
	uint32_t appdata32bit;			// マスク処理で使う32bitのデータ
	int8_t tmpbitsift;				// 途中のマスク処理
	size_t continue_pos;
	int32_t buf_munit;				// ソケットが持っているバッファ領域
	struct sockaddr_in addr;		// UDP送信先
	struct sockaddr_storage from;	// UDPの受信相手情報
	socklen_t fromlen;				// UDPのfromのサイズ
} GDT_SOCKPARAM;

typedef struct GDT_SERVER_CONNECTION_INFO
{
	GDT_SOCKET_ID id;
	char hbuf[NI_MAXHOST];
	char sbuf[NI_MAXHOST];
#ifdef __WINDOWS__
	HANDLE	parentid;
	HANDLE	currentid;
#else
	pthread_t parentid;
	pthread_t currentid;
	pid_t parent_processid;
	pid_t current_processid;
#endif
	uint32_t index;	
	int32_t recvbuf_munit;
	int32_t recvinfo_munit;
	int32_t recvmsg_munit;
	void* user_data;
	void* gdt_socket_option;
	GDT_SOCKPARAM sockparam;
} GDT_SERVER_CONNECTION_INFO;

typedef struct GDT_RECV_INFO
{
	int32_t recvbuf_munit;
	ssize_t recvlen;
	GDT_SOCKET_ID recvfrom;
	GDT_SERVER_CONNECTION_INFO *tinfo;
} GDT_RECV_INFO;

typedef struct GDT_SEND_INFO
{
	int32_t send_munit;
	ssize_t sendlen;
} GDT_SEND_INFO;


// event callback
typedef void* (*GDT_CALLBACK)( void* args );
typedef int (*GDT_CONNECTION_EVENT_CALLBACK)( GDT_SERVER_CONNECTION_INFO* connection );
typedef int32_t (*GDT_ON_RECV)(uint32_t payload_type, uint8_t* payload, size_t payload_len, GDT_RECV_INFO *gdt_recv_info );
// recv call callback
typedef int (*GDT_USER_RECV)( void* connection, GDT_SOCKET_ID id, char* buf, size_t buffer_size, int flag );
// send call callback
typedef int (*GDT_USER_SEND)( void* connection, GDT_SOCKET_ID id, char *buf, size_t buffer_size, int flag );

typedef struct GDT_SOCKET_OPTION
{
	int inetflag;					// bit flag( show inetflags )
	int32_t host_name_munit;		// host name( char* )
	int32_t port_num_munit;			// port num( char* )
	int32_t connection_munit;		// connection array ( GDT_SERVER_CONNECTION_INFO* )
	int32_t lock_file_munit;		// lock file name( char* )
	int lock_file_fd;				// accept lock fd
#ifdef __WINDOWS__
	WSADATA wsdata;
	HOSTENT *phostent;
	IN_ADDR inaddr;
	struct hostent *host;
	struct sockaddr_in serveraddr;
	char hbuf[256];
	char ipbuf[256];
	HANDLE	accept_lock;					// mutex
#else
	pthread_mutex_t accept_lock;			// mutex
#endif
	GDT_SOCKET_ID sockid;					// sokcet fd
	GDT_SOCKET_ID sockid6;					// ipv6 socket fd
	int64_t t_lock_id;						// thread lock id
	char recvtimeoutmode;					// recv timeout mode( TIMEOUT_MODE_* )
	size_t maxconnection;					// max connection length
	uint8_t socket_type;					// socket type( SOKET_TYPE_SERVER_TCP: server socket, SOKET_TYPE_CLIENT_TCP: client socket )
	uint8_t mode;							// connect mode( SOCKET_MODE_SINGLE:single task, SOCKET_MODE_PREFORK:prefork, SOCKET_MODE_THREAD:thread)
	uint8_t protocol;						// PROTOCOL_*
	int32_t t_usec;							// connection timeout (usec)
	int32_t t_sec;							// connection timeout (sec)
	int32_t s_sec;							// timeout( select, pool, epool )
	int32_t s_usec;							// timeout( select, pool, epool )
	GDT_CONNECTION_EVENT_CALLBACK connection_start_callback;	// callback pointer
	GDT_CALLBACK send_finish_callback;		// callback pointer
	GDT_CALLBACK plane_recv_callback;		// callback pointer
	GDT_CONNECTION_EVENT_CALLBACK close_callback;			// callback pointer
	GDT_CALLBACK timeout_callback;			// callback pointer
	GDT_ON_RECV payload_recv_callback;
	GDT_USER_RECV user_recv_function;		// user call recv
	GDT_USER_SEND user_send_function;		// user call send
	size_t recvbuffer_size;					// recv buffer size
	size_t sendbuffer_size;					// send buffer size
	size_t msgbuffer_size;					// message buffer size
	GDT_MEMORY_POOL* memory_pool;			// memory pool
	GDT_MEMORY_POOL* mmap_memory_pool;		// mmap memory pool
} GDT_SOCKET_OPTION;

GDT_SOCKET_OPTION* gdt_create_tcp_server(char* hostname, char* portnum);
GDT_SOCKET_OPTION* gdt_create_udp_server(char* hostname, char* portnum);
GDT_SOCKET_OPTION* gdt_create_tcp_client(char* hostname, char* portnum);
GDT_SOCKET_OPTION* gdt_create_udp_client(char* hostname, char* portnum);
ssize_t gdt_send_message(uint32_t payload_type, char* payload, size_t payload_len, GDT_RECV_INFO *gdt_recv_info);
ssize_t gdt_send_message_broadcast(uint32_t payload_type, char* payload, size_t payload_len, GDT_RECV_INFO *gdt_recv_info);
ssize_t gdt_send_message_multicast(uint32_t payload_type, char* payload, size_t payload_len, GDT_RECV_INFO *gdt_recv_info, GDT_MEMORY_POOL* array_memory, int32_t array_munit);
ssize_t gdt_client_send_message(uint32_t payload_type, char* payload, size_t payload_len, GDT_SOCKET_OPTION *option);
int32_t gdt_set_client_id(GDT_SOCKET_OPTION *option,uint32_t id);

int gdt_initialize_socket_option( 
	  GDT_SOCKET_OPTION *option
	, char* hostname
	, char* portnum
	, uint8_t socket_type
	, uint8_t mode
	, uint8_t protocol
	, size_t maxconnection
	, GDT_MEMORY_POOL* memory_pool
	, GDT_MEMORY_POOL* mmap_memory_pool
);

void set_on_connect_event( GDT_SOCKET_OPTION *option, GDT_CONNECTION_EVENT_CALLBACK func );
void set_on_sent_event( GDT_SOCKET_OPTION *option, GDT_CALLBACK func );
void set_on_packet_recv_event( GDT_SOCKET_OPTION *option, GDT_CALLBACK func );
void set_on_payload_recv_event( GDT_SOCKET_OPTION *option, GDT_ON_RECV func );
void set_on_close_event( GDT_SOCKET_OPTION *option, GDT_CONNECTION_EVENT_CALLBACK func );
void set_user_recv_event( GDT_SOCKET_OPTION *option, GDT_USER_RECV func );
void set_user_send_event( GDT_SOCKET_OPTION *option, GDT_USER_SEND func);
void set_message_buffer( GDT_SOCKET_OPTION *option, size_t buffer_size);
void gdt_set_timeout_event( GDT_SOCKET_OPTION *option, GDT_CALLBACK func );
void gdt_set_connection_timeout( GDT_SOCKET_OPTION *option, int32_t sec, int32_t usec );
void gdt_set_select_timeout( GDT_SOCKET_OPTION *option, int32_t sec, int32_t usec );

int32_t gdt_make_connection_info( GDT_SOCKET_OPTION *option );
int32_t gdt_make_connection_info_core( GDT_SOCKET_OPTION *option, GDT_SERVER_CONNECTION_INFO* tinfo, int index );
void gdt_initialize_connection_info( GDT_SOCKET_OPTION *option, GDT_SERVER_CONNECTION_INFO* tinfo );

ssize_t gdt_send_broadcast( GDT_SOCKET_OPTION *option, char *buf, size_t size, uint32_t payload_type );
ssize_t gdt_send( GDT_SOCKET_OPTION *option, GDT_SOCKPARAM *psockparam, char *buf, size_t size, uint32_t payload_type );
ssize_t gdt_send_all(GDT_SOCKET_ID soc, char *buf, size_t size, int flag );
ssize_t gdt_sendto_all(GDT_SOCKET_ID soc, char *buf, size_t size, int flag, struct sockaddr *pfrom, socklen_t fromlen );

int gdt_get_sockaddr_info( GDT_SOCKET_OPTION *option, struct sockaddr_storage *saddr, socklen_t *addr_len );
GDT_SOCKET_ID gdt_server_socket( GDT_SOCKET_OPTION *option, int is_ipv6 );
GDT_SOCKET_ID gdt_client_socket( GDT_SOCKET_OPTION *option );
void gdt_disconnect( GDT_SOCKPARAM *psockparam );
void gdt_set_sock_option( GDT_SOCKET_OPTION *option );
void* gdt_socket( GDT_SOCKET_OPTION *option );
void gdt_free_socket( GDT_SOCKET_OPTION *option );

void gdt_single_task_server( GDT_SOCKET_OPTION *option );
void gdt_select_server( GDT_SOCKET_OPTION *option );
void gdt_nonblocking_server(GDT_SOCKET_OPTION *option);
void gdt_server_update(GDT_SOCKET_OPTION *option);
void gdt_nonblocking_client(GDT_SOCKET_OPTION *option);
void gdt_client_update(GDT_SOCKET_OPTION *option);

#ifdef __WINDOWS__
#else
void gdt_pool_server( GDT_SOCKET_OPTION *option );
void gdt_epool_server( GDT_SOCKET_OPTION *option );
void gdt_kqueue_server( GDT_SOCKET_OPTION *option );
void gdt_prefork_server( GDT_SOCKET_OPTION *option );
void gdt_accept_loop_fork( struct GDT_SERVER_CONNECTION_INFO *forkinfo );
void gdt_recv_loop_fork( struct GDT_SERVER_CONNECTION_INFO *forkinfo );
void gdt_recvfrom_loop_fork( struct GDT_SERVER_CONNECTION_INFO *forkinfo );
#endif

void gdt_thread_server( GDT_SOCKET_OPTION *option );
#ifdef __WINDOWS__
unsigned __stdcall gdt_accept_thread( void* arg );
unsigned __stdcall gdt_recvfrom_loop_thread( void* arg );
#else
void* gdt_accept_thread( void* arg );
void* gdt_recvfrom_loop_thread( void* arg );
#endif
void gdt_recv_loop_thread( struct GDT_SERVER_CONNECTION_INFO *tinfo );

int gdt_set_block(GDT_SOCKET_ID fd, int flag );

void gdt_print_payload(uint32_t payload_type, uint8_t* payload, size_t payload_len,size_t view_max);
int gdt_pre_packetfilter( GDT_SOCKET_OPTION *option, struct GDT_RECV_INFO *rinfo, GDT_SOCKPARAM *psockparam, int32_t recvmsg_munit );
int32_t gdt_socket_phase( GDT_SOCKET_OPTION *option, struct GDT_RECV_INFO *rinfo, GDT_SOCKPARAM *psockparam, int32_t recvmsg_munit );
ssize_t gdt_parse_socket_binary( GDT_SOCKET_OPTION *option, GDT_SOCKPARAM *psockparam, uint8_t* u8buf, size_t size, uint32_t basebuf_munit );
ssize_t gdt_send_msg( GDT_SOCKET_OPTION *option, GDT_SOCKPARAM *psockparam, const char* msg, ssize_t size, uint32_t payload_type );
uint32_t gdt_make_size_header(uint8_t* head_ptr,ssize_t size);
size_t gdt_make_udpmsg( void* sendbin, const char* msg, ssize_t size, uint32_t payload_type );
ssize_t gdt_send_udpmsg( GDT_SOCKET_OPTION *option, GDT_SOCKPARAM *psockparam, const char* msg, ssize_t size, uint32_t payload_type, struct sockaddr *pfrom, socklen_t fromlen );

ssize_t gdt_parse_websocket_binary( GDT_SOCKET_OPTION *option, GDT_SOCKPARAM *psockparam, uint8_t* u8buf, size_t size, uint32_t basebuf_munit );
ssize_t gdt_send_websocket_msg( GDT_SOCKET_OPTION *option, GDT_SOCKPARAM *psockparam, const char* msg, ssize_t size );
int gdt_send_handshake_param(GDT_SOCKET_ID socket, GDT_SOCKET_OPTION *option, GDT_SOCKPARAM *psockparam );

void gdt_init_socket_param( GDT_SOCKPARAM *psockparam );
void gdt_free_sockparam( GDT_SOCKET_OPTION *option, GDT_SOCKPARAM *psockparam );
int gdt_close_socket(GDT_SOCKET_ID* sock, char* error );

int gdt_parse_http( GDT_SOCKET_OPTION *option, GDT_SOCKPARAM *psockparam, char* target, int32_t recvmsg_munit );
int gdt_parse_http_header( GDT_SOCKET_OPTION *option, GDT_SOCKPARAM *psockparam, char* target, int32_t recvmsg_munit );
void gdt_add_http_header( GDT_SOCKET_OPTION *option, GDT_SOCKPARAM *psockparam, int headerindex, char* pbuf );

void gdt_send_recv_client_term( GDT_SOCKET_OPTION *option );
void gdt_thread_client( GDT_SOCKET_OPTION *option );

#ifdef __WINDOWS__
unsigned __stdcall gdt_input_thread_client( void* arg );
unsigned __stdcall gdt_recv_thread_client( void* arg );
#else
void* gdt_input_thread_client( void* arg );
void* gdt_recv_thread_client( void* arg );
#endif

#endif /*_GDT_SOCKET_H_*/

#ifdef __cplusplus
}
#endif
