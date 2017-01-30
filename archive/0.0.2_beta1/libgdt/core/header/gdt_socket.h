/*
 * Copyright (c) 2014-2016 Katsuya Owari
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

#ifndef __BSD_UNIX__
#ifndef __WINDOWS__
#define USE_EPOOL
#endif // ifndef __WINDOWS__
#endif // ifndef __BSD_UNIX__

#include "gdt_core.h"
#include "gdt_system.h"
#include "gdt_io.h"
#include "gdt_sha1.h"
#include "gdt_base64.h"
#include "gdt_string.h"
#include "gdt_queue.h"
#include "gdt_timeout.h"

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
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <sys/file.h>
#include <arpa/telnet.h>
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

// client mode
#define SOCKET_MODE_SIMPLE_TERM 100
#define SOCKET_MODE_CLIENT_THREAD 101

// websocket status
#define WEBSOCK_STATUS_ACCEPT 0
#define WEBSOCK_STATUS_SENDHANDSHAKE 1
#define WEBSOCK_STATUS_RECVHANDSHAKEOK 2
#define WEBSOCK_STATUS_OTHER 3

// socket phase
#define SOCK_PHASE_MSG_SOCKET 1
#define SOCK_PHASE_HANDSHAKE_WEBSOCKET 2
#define SOCK_PHASE_MSG_WEBSOCKET 3

// socket type
#define SOCK_TYPE_NORMAL 1			// tcp socket
#define SOCK_TYPE_WEBSOCKET 2		// websocket( tcp only )
#define SOCK_TYPE_NORMAL_UDP 3		// udp socket

// protocol
#define PROTOCOL_PLAIN 1			// plane
#define PROTOCOL_SIMPLE 2			// simple header

// WEBSOCKET MESSAGE MODE
#define WS_MODE_TEXT 1
#define WS_MODE_BINARY 2

// HTTP method
#define HTTP_METHOD_GET		1;
#define HTTP_METHOD_HEAD	2;
#define HTTP_METHOD_POST	3;

// HTTP header offset
#define HTTP_HEADER_LENGTH					8	// ヘッダー配列のサイズ
#define HTTP_HEADER_PARAM_METHOD			0	// httpメソッド
#define HTTP_HEADER_PARAM_REQUEST			1	// httpリクエストパラメータ
#define HTTP_HEADER_PARAM_VERSION			2	// httpバージョン
#define HTTP_HEADER_PARAM_HOST				3	// リクエストホスト名
#define HTTP_HEADER_PARAM_USERAGENT			4	// ユーザーエージェント
#define HTTP_HEADER_PARAM_WEBSOCKETKEY		5	// websocket接続キー
#define HTTP_HEADER_PARAM_WEBSOCKETPROTOCOL	6	// websocketプロトコル

// 受信メッセージの受け取りコールバック
typedef void* (*GDT_CALLBACK)( void* args );

/*
 * ソケットオプション
 */
typedef struct GDT_SOCKET_OPTION
{
	int inetflag;					// ネットワークフラグ( 0:ipv4, 1:ipv6 )
	int32_t host_name_munit;		// 接続ホスト名メモリユニット
	int32_t port_num_munit;			// 接続ポートメモリユニット
	int32_t connection_munit;		// 接続管理情報の配列メモリユニット( GDT_SERVER_CHILD_INFO, GDT_SERVER_FORK_INFO, GDT_SERVER_THREAD_INFO )
	int32_t mq_munit;				// メッセージキューのメモリユニット
	int32_t lock_file_munit;		// ロックファイル名が入っているメモリ番号
	int lock_file_fd;				// ファイルロック機能で使用するロック番号
#ifdef __WINDOWS__
	WSADATA wsdata;
	HOSTENT *phostent;
	IN_ADDR inaddr;
	struct hostent *host;
	struct sockaddr_in serveraddr;
	char hbuf[256];
	char ipbuf[256];
	HANDLE	accept_lock;			// 待ち受け処理用のミューテックス
#else
	pthread_mutex_t accept_lock;	// 待ち受け処理用のミューテックス
#endif
	GDT_SOCKET_ID sockid;					// ソケットファイルディスクリプタ
	GDT_SOCKET_ID sockid6;					// ipv6ソケットファイルディスクリプタ
	int64_t t_lock_id;				// スレッドロックID
	char recvtimeoutmode;			// 受信タイムアウトモード
	size_t maxconnection;			// 最大接続数
	uint8_t socket_type;			// ソケットの種類( SOKET_TYPE_SERVER_TCP:サーバーソケット, SOKET_TYPE_CLIENT_TCP:クライアントソケット )
	uint8_t mode;					// 接続モード(SOCKET_MODE_SINGLE:シングルタスク接続, SOCKET_MODE_PREFORK:prefork同時接続, SOCKET_MODE_THREAD:スレッド同時接続)
	uint8_t protocol;				// 内部プロトコル指定(  )
	int32_t t_usec;					// 接続タイムアウトusec
	int32_t t_sec;					// 接続タイムアウトsec
	int32_t s_sec;					// select,pool,epool用のタイムアウト
	GDT_CALLBACK connection_start_callback;	// 接続時のコールバック関数ポインタ
	GDT_CALLBACK send_finish_callback;		// データ送信時のコールバック関数ポインタ
	GDT_CALLBACK recv_callback;				// データ受信時のコールバック関数ポインタ
	GDT_CALLBACK close_callback;			// 通信切断時のコールバック関数ポインタ
	size_t recvbufsize;				// 受信バッファのサイズ
	size_t queuebufsize;			// メッセージキューバッファのサイズ
	size_t queuelen;				// メッセージキュー配列のサイズ
	GDT_MEMORY_POOL* __mp;			// プロセスが持ってるメモリプール
	GDT_MEMORY_POOL* __mmapp;		// 共有メモリプール
	int32_t relay_munit;			// relayソケット
} GDT_SOCKET_OPTION;

// Socket管理用の構造体
typedef struct GDT_SOCKPARAM
{
	int32_t http_header_munit;		// httpヘッダー情報
	int32_t wsockkey_munit;			// websocketキー
	int32_t helth;					// 切断チェック用のヘルス値
	int32_t c_status;				// 接続状態
	GDT_SOCKET_ID acc;				// 接続確立したクライアントのソケット
	int32_t type;					// ソケットの種類
	int phase;						// 接続フェーズ
	uint8_t ws_msg_mode;			// メッセージ形式( 1 : text, 2 : binary )
	uint8_t fin;					// FINフラグ( 送信データの終了なら1 )
	uint8_t rsv;					// RSVフラグ
	uint8_t opcode;					// opcode( 0:connection, 1:test, 2:binary, 3-7reserved for further, 8:close, 9:ping, a:pong b-f:reserved for further )
	uint8_t mask;					// マスクフラグ( 1ならマスクあり )
	uint8_t ckpayloadlen;			// payloadサイズ( 1-125:そのままデータサイズ, 126:以降16ビットがデータサイズ, 127:以降64ビットがデータサイズ )
	uint64_t payloadlen;			// 実際のpayloadサイズ
	uint32_t maskindex;				// maskキーの格納場所( 1-125:2, 126:4, 127:8 )
	uint32_t payloadmask;			// マスク値(32bit)
	ssize_t tmpmsglen;				// 受信メッセージサイズ( 受信途中の )
	uint32_t appdata32bit;			// マスク処理で使う32bitのデータ
	int8_t tmpbitsift;				// 途中のマスク処理
	int32_t buf_munit;				// ソケットが持っているバッファ領域
	struct sockaddr_in addr;		// UDP送信先
	struct sockaddr_storage from;	// UDPの受信相手情報
	socklen_t fromlen;				// UDPのfromのサイズ
} GDT_SOCKPARAM;

// selectで多重化した子に渡す情報
typedef struct GDT_SERVER_CHILD_INFO
{
	GDT_SOCKET_ID id;					// id
	char hbuf[NI_MAXHOST];
	char sbuf[NI_MAXHOST];
	int32_t recvbuf_munit;				// 受信バッファの格納メモリユニット
	int32_t recvinfo_munit;				// 受信情報管理構造体の格納メモリユニット
	int32_t recvmsg_munit;				// 受信メッセージ一時格納領域
	int32_t free_munit;					// 自由利用可能なメモリユニット
	GDT_SOCKET_OPTION *option;			// ソケットオプション
	GDT_SOCKPARAM sockParam;			// socketパラメータ
} GDT_SERVER_CHILD_INFO;

#ifdef __WINDOWS__
#else
// forkした子に渡す情報
typedef struct GDT_SERVER_FORK_INFO
{
	char hbuf[NI_MAXHOST];
	char sbuf[NI_MAXHOST];
	pid_t parentid;						// 親プロセスID
	pid_t currentid;					// 自身のプロセスID
	uint32_t index;						// 連番
	int32_t recvbuf_munit;				// 受信バッファの格納メモリユニット
	int32_t recvinfo_munit;				// 受信情報管理構造体の格納メモリユニット
	int32_t recvmsg_munit;				// 受信メッセージ一時格納領域
	int32_t free_munit;					// 自由利用可能なメモリユニット
	GDT_SOCKET_OPTION *option;			// ソケットオプション
	GDT_SOCKPARAM sockParam;			// socketパラメータ
} GDT_SERVER_FORK_INFO;
#endif

// 子スレッドに渡す情報
typedef struct GDT_SERVER_THREAD_INFO
{
	char hbuf[NI_MAXHOST];
	char sbuf[NI_MAXHOST];
#ifdef __WINDOWS__
	HANDLE	parentid;			// 親スレッドID
	HANDLE	currentid;			// 自身のスレッドID
#else
	pthread_t parentid;					// 親スレッドID
	pthread_t currentid;				// 自身のスレッドID
#endif
	uint32_t index;						// 連番
	int32_t recvbuf_munit;				// 受信バッファの格納メモリユニット
	int32_t recvinfo_munit;				// 受信情報管理構造体の格納メモリユニット
	int32_t recvmsg_munit;				// 受信メッセージ一時格納領域
	int32_t free_munit;					// 自由利用可能なメモリユニット
	GDT_SOCKET_OPTION *option;			// ソケットオプション
	GDT_SOCKPARAM sockParam;			// socketパラメータ
} GDT_SERVER_THREAD_INFO;

// 受信コールバックに渡す情報
typedef struct GDT_RECV_INFO
{
	int32_t recvbuf_munit;					// 受信したバッファの格納メモリユニット
	ssize_t recvlen;						// 受信した文字列のサイズ
	GDT_SOCKET_ID recvfrom;					// 受信相手
	void *tinfo;							// 接続管理情報
} GDT_RECV_INFO;

// 送信コールバックに渡す情報
typedef struct GDT_SEND_INFO
{
	int32_t send_munit;					// 送信相手の格納メモリユニット
	ssize_t sendlen;					// 受信した文字列のサイズ
} GDT_SEND_INFO;

int gdt_initialize_socket_option( 
	  GDT_SOCKET_OPTION *option
	, char* hostname
	, char* portnum
	, uint8_t socket_type
	, uint8_t mode
	, uint8_t protocol
	, size_t maxconnection
	, GDT_MEMORY_POOL* __mp
	, GDT_MEMORY_POOL* __mmapp
	, GDT_CALLBACK connection_start_callback
	, GDT_CALLBACK send_finish_callback
	, GDT_CALLBACK recv_callback
	, GDT_CALLBACK close_callback
);

void gdt_initialize_child_info( GDT_SOCKET_OPTION *option, struct GDT_SERVER_CHILD_INFO* tinfo );
#ifdef __WINDOWS__
#else
void gdt_initialize_fork_info( GDT_SOCKET_OPTION *option, struct GDT_SERVER_FORK_INFO* tinfo );
#endif
void gdt_initialize_thread_info( GDT_SOCKET_OPTION *option, struct GDT_SERVER_THREAD_INFO* tinfo );

ssize_t gdt_send( GDT_SOCKET_OPTION *option, GDT_SOCKPARAM *psockparam, char *buf, size_t size, int flag );
ssize_t gdt_send_all(GDT_SOCKET_ID soc, char *buf, size_t size, int flag );
ssize_t gdt_sendto_all(GDT_SOCKET_ID soc, char *buf, size_t size, int flag, struct sockaddr *pfrom, socklen_t fromlen );

GDT_SOCKET_ID gdt_server_socket( GDT_SOCKET_OPTION *option, int inetflag ); /* create server socket */
GDT_SOCKET_ID gdt_client_socket( GDT_SOCKET_OPTION *option ); /* create client socket */

void gdt_set_sock_option( GDT_SOCKET_OPTION *option ); /* set default socket options */
void* gdt_socket( GDT_SOCKET_OPTION *option ); /* create socket */
void gdt_free_socket( GDT_SOCKET_OPTION *option ); /* free socket */

void gdt_single_task_server( GDT_SOCKET_OPTION *option ); /* single task server mode */
void gdt_select_server( GDT_SOCKET_OPTION *option ); /*select multi server mode */

#ifdef __WINDOWS__
#else
void gdt_pool_server( GDT_SOCKET_OPTION *option ); /* pool multi server mode */
void gdt_epool_server( GDT_SOCKET_OPTION *option ); /* epool multi server mode */
void gdt_prefork_server( GDT_SOCKET_OPTION *option ); /* prefork multi server mode */
void gdt_accept_loop_fork( struct GDT_SERVER_FORK_INFO *forkinfo ); /* accept loop for fork */
void gdt_recv_loop_fork( struct GDT_SERVER_FORK_INFO *forkinfo );  /* recv loop for fork */
void gdt_recvfrom_loop_fork( struct GDT_SERVER_FORK_INFO *forkinfo );  /* recvfrom loop for fork */
#endif
void gdt_thread_server( GDT_SOCKET_OPTION *option ); /* prethread multi server mode */

#ifdef __WINDOWS__
unsigned __stdcall gdt_accept_thread( void* arg ); /* accept loop for thread */
unsigned __stdcall gdt_recvfrom_loop_thread( void* arg );  /* recvfrom loop for thread */
#else
void* gdt_accept_thread( void* arg ); /* accept loop for thread */
void* gdt_recvfrom_loop_thread( void* arg );  /* recvfrom loop for thread */
#endif
void gdt_recv_loop_thread( struct GDT_SERVER_THREAD_INFO *tinfo ); /* recv loop for thread */


int gdt_set_block(GDT_SOCKET_ID fd, int flag ); /* set blocking mode */

#ifdef __WINDOWS__
#else
int gdt_get_sockaddr_info( GDT_SOCKET_OPTION *option, struct sockaddr_storage *saddr, socklen_t *addr_len );
#endif
int gdt_pre_packetfilter( GDT_SOCKET_OPTION *option, struct GDT_RECV_INFO *rinfo, GDT_SOCKPARAM *psockparam, int32_t recvmsg_munit );
int gdt_close_socket(GDT_SOCKET_ID* sock, char* error ); /* close socket */

int32_t gdt_socket_phase( GDT_SOCKET_OPTION *option, struct GDT_RECV_INFO *rinfo, GDT_SOCKPARAM *psockparam ); /* handle recv packet */
ssize_t gdt_parse_socket_binary( GDT_SOCKET_OPTION *option, GDT_SOCKPARAM *psockparam, uint8_t* u8buf, size_t size, uint32_t basebuf_munit ); /* parse recv packet */
ssize_t gdt_send_msg( GDT_SOCKET_OPTION *option, GDT_SOCKPARAM *psockparam, const char* msg, ssize_t size ); /* send tcp packet */
ssize_t gdt_send_udpmsg( GDT_SOCKET_OPTION *option, GDT_SOCKPARAM *psockparam, const char* msg, ssize_t size, int flag, struct sockaddr *pfrom, socklen_t fromlen ); /* send udp packet */

ssize_t gdt_parse_websocket_binary( GDT_SOCKET_OPTION *option, GDT_SOCKPARAM *psockparam, uint8_t* u8buf, size_t size, uint32_t basebuf_munit ); /* parse websocket recv packet */
ssize_t gdt_send_websocket_msg( GDT_SOCKET_OPTION *option, GDT_SOCKPARAM *psockparam, const char* msg, ssize_t size ); /* send websocket packet */
int gdt_send_handshake_param(GDT_SOCKET_ID socket, GDT_SOCKET_OPTION *option, GDT_SOCKPARAM *psockparam ); /* send websocket handshake parameter */
void gdt_init_socket_param( GDT_SOCKPARAM *psockparam ); /* initialize GDT_SOCKPARAM status */
int gdt_parse_handshake_param( GDT_SOCKET_OPTION *option, GDT_SOCKPARAM *psockparam, char* target ); /* parse handshake parameter */
void gdt_add_http_header( GDT_SOCKET_OPTION *option, GDT_SOCKPARAM *psockparam, int headerindex, char* pbuf ); /* add http header parameter */
char* gdt_get_http_header( GDT_SOCKET_OPTION *option, GDT_SOCKPARAM *psockparam, int headerindex ); /* get http header parameter */
void gdt_free_sockparam( GDT_SOCKET_OPTION *option, GDT_SOCKPARAM *psockparam ); /* free GDT_SOCKPARAM memory */
void gdt_send_recv_client_term( GDT_SOCKET_OPTION *option ); /* send recv client term */
void gdt_thread_client( GDT_SOCKET_OPTION *option );

#ifdef __WINDOWS__
unsigned __stdcall gdt_input_thread_client( void* arg ); /* input thread */
unsigned __stdcall gdt_recv_thread_client( void* arg ); /* input thread */
#else
void* gdt_input_thread_client( void* arg ); /* input thread client */
void* gdt_recv_thread_client( void* arg ); /* recv thread client */
#endif

void gdt_inetdump(); /* dump inet status */

#endif /*_GDT_SOCKET_H_*/

#ifdef __cplusplus
}
#endif
