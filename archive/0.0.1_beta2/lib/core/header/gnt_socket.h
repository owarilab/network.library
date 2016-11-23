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

#ifndef _GNT_SOCKET_H_
#define _GNT_SOCKET_H_

#define USE_EPOOL

#include "core.h"
#include "gnt_system.h"
#include "gnt_io.h"
#include "gnt_sha1.h"
#include "gnt_base64.h"
#include "gnt_string.h"
#include "gnt_queue.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <semaphore.h>
#include <pthread.h>
#include <netinet/in.h>
#include <errno.h>
#include <netdb.h>
#include <poll.h>
#include <ctype.h>
#include <sysexits.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <time.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#ifdef USE_EPOOL
#include <sys/epoll.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <arpa/telnet.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <net/if.h>
#include <ifaddrs.h>

// ソケットの種類
#define SOKET_TYPE_SERVER_TCP 1
#define SOKET_TYPE_SERVER_UDP 2
#define SOKET_TYPE_CLIENT_TCP 3
#define SOKET_TYPE_CLIENT_UDP 4

// サーバ接続モード
#define SOCKET_MODE_SINGLE 0
#define SOCKET_MODE_SELECT 1
#define SOCKET_MODE_POOL 2
#define SOCKET_MODE_EPOOL 3
#define SOCKET_MODE_PREFORK 4
#define SOCKET_MODE_THREAD 5

// クライアント接続モード
#define SOCKET_MODE_SIMPLE_TERM 100
#define SOCKET_MODE_CLIENT_THREAD 101

// WebSocket接続使用時のステータス
#define WEBSOCK_STATUS_ACCEPT 0
#define WEBSOCK_STATUS_SENDHANDSHAKE 1
#define WEBSOCK_STATUS_RECVHANDSHAKEOK 2
#define WEBSOCK_STATUS_OTHER 3

// ソケット接続状態
#define SOCK_PHASE_MSG_SOCKET 1
#define SOCK_PHASE_HANDSHAKE_WEBSOCKET 2
#define SOCK_PHASE_MSG_WEBSOCKET 3

// ソケットタイプ
#define SOCK_TYPE_NORMAL 1			// 通常のTCPソケット
#define SOCK_TYPE_WEBSOCKET 2		// websocket(TCPしかない)
#define SOCK_TYPE_NORMAL_UDP 3		// 通常のUDPソケット

// 内部プロトコル
#define PROTOCOL_PLAIN 1			// 平文
#define PROTOCOL_SIMPLE 2			// シンプルなヘッダー付きパケット

// HTTPメソッド
#define HTTP_METHOD_GET		1;
#define HTTP_METHOD_HEAD	2;
#define HTTP_METHOD_POST	3;

// HTTPヘッダの配列格納番号
#define HTTP_HEADER_LENGTH					8	// ヘッダー配列のサイズ
#define HTTP_HEADER_PARAM_METHOD			0	// httpメソッド
#define HTTP_HEADER_PARAM_REQUEST			1	// httpリクエストパラメータ
#define HTTP_HEADER_PARAM_VERSION			2	// httpバージョン
#define HTTP_HEADER_PARAM_HOST				3	// リクエストホスト名
#define HTTP_HEADER_PARAM_USERAGENT			4	// ユーザーエージェント
#define HTTP_HEADER_PARAM_WEBSOCKETKEY		5	// websocket接続キー
#define HTTP_HEADER_PARAM_WEBSOCKETPROTOCOL	6	// websocketプロトコル

#define STATICPARAM
static const char connectionField[] STATICPARAM = "Connection: ";
static const char upgrade[] 		STATICPARAM = "upgrade";
static const char upgrade2[] 		STATICPARAM = "Upgrade";
static const char upgradeField[] 	STATICPARAM = "Upgrade: ";
static const char websocket[] 		STATICPARAM = "websocket";
static const char hostField[] 		STATICPARAM = "Host: ";
static const char originField[] 	STATICPARAM = "Origin: ";
static const char keyField[] 		STATICPARAM = "Sec-WebSocket-Key: ";
static const char protocolField[] 	STATICPARAM = "Sec-WebSocket-Protocol: ";
static const char versionField[] 	STATICPARAM = "Sec-WebSocket-Version: ";
static const char version[] 		STATICPARAM = "13";
static const char secret[] 			STATICPARAM = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
static const char protocol_chat[] 	STATICPARAM = "chat";

// 受信メッセージの受け取りコールバック
typedef void* (*GNT_CALLBACK)( void* args );

typedef struct GNT_SOCKET_OPTION GNT_SOCKET_OPTION;
typedef struct GNT_SOCKPARAM GNT_SOCKPARAM;
typedef struct GNT_SEND_INFO GNT_SEND_INFO;

/*
 * ソケットオプション
 */
struct GNT_SOCKET_OPTION
{
	int inetflag;					// ネットワークフラグ( 0:ipv4, 1:ipv6 )
	int32_t host_name_munit;		// 接続ホスト名メモリユニット
	int32_t port_num_munit;			// 接続ポートメモリユニット
	int32_t connection_munit;		// 接続管理情報の配列メモリユニット( GNT_SERVER_CHILD_INFO, GNT_SERVER_FORK_INFO, GNT_SERVER_THREAD_INFO )
	int32_t mq_munit;				// メッセージキューのメモリユニット
	int32_t lock_file_munit;		// ロックファイル名が入っているメモリ番号
	int lock_file_fd;				// ファイルロック機能で使用するロック番号
	pthread_mutex_t accept_lock;	// 待ち受け処理用のミューテックス
	int64_t t_lock_id;				// スレッドロックID
	int sockid;						// ソケットファイルディスクリプタ
	int sockid6;					// ipv6ソケットファイルディスクリプタ
	char recvtimeoutmode;			// 受信タイムアウトモード
	size_t maxconnection;			// 最大接続数
	uint8_t socket_type;			// ソケットの種類( SOKET_TYPE_SERVER_TCP:サーバーソケット, SOKET_TYPE_CLIENT_TCP:クライアントソケット )
	uint8_t mode;					// 接続モード(SOCKET_MODE_SINGLE:シングルタスク接続, SOCKET_MODE_PREFORK:prefork同時接続, SOCKET_MODE_THREAD:スレッド同時接続)
	uint8_t protocol;				// 内部プロトコル指定(  )
	int32_t t_usec;					// 接続タイムアウトusec
	int32_t t_sec;					// 接続タイムアウトsec
	int32_t s_sec;					// select,pool,epool用のタイムアウト
	GNT_CALLBACK connection_start_callback;	// 接続時のコールバック関数ポインタ
	GNT_CALLBACK send_finish_callback;		// データ送信時のコールバック関数ポインタ
	GNT_CALLBACK recv_callback;		// データ受信時のコールバック関数ポインタ
	size_t recvbufsize;				// 受信バッファのサイズ
	size_t queuebufsize;			// メッセージキューバッファのサイズ
	size_t queuelen;				// メッセージキュー配列のサイズ
	GNT_MEMORY_POOL* __mp;			// プロセスが持ってるメモリプール
	GNT_MEMORY_POOL* __mmapp;		// 共有メモリプール
	int32_t relay_munit;			// relayソケット
};

// Socket管理用の構造体
struct GNT_SOCKPARAM
{
	int32_t http_header_munit;		// httpヘッダー情報
	int32_t helth;					// 切断チェック用のヘルス値
	int32_t c_status;				// 接続状態
	int32_t acc;					// 接続確立したクライアントのソケット
	int32_t type;					// ソケットの種類
	int phase;						// 接続フェーズ
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
};

// selectで多重化した子に渡す情報
struct GNT_SERVER_CHILD_INFO
{
	int id;								// id
	int32_t recvbuf_munit;				// 受信バッファの格納メモリユニット
	int32_t recvinfo_munit;				// 受信情報管理構造体の格納メモリユニット
	int32_t recvmsg_munit;				// 受信メッセージ一時格納領域
	int32_t free_munit;					// 自由利用可能なメモリユニット
	GNT_SOCKET_OPTION *option;			// ソケットオプション
	GNT_SOCKPARAM sockParam;			// socketパラメータ
};

// forkした子に渡す情報
struct GNT_SERVER_FORK_INFO
{
	pid_t parentid;						// 親プロセスID
	pid_t currentid;					// 自身のプロセスID
	uint32_t index;						// 連番
	int32_t recvbuf_munit;				// 受信バッファの格納メモリユニット
	int32_t recvinfo_munit;				// 受信情報管理構造体の格納メモリユニット
	int32_t recvmsg_munit;				// 受信メッセージ一時格納領域
	int32_t free_munit;					// 自由利用可能なメモリユニット
	GNT_SOCKET_OPTION *option;			// ソケットオプション
	GNT_SOCKPARAM sockParam;			// socketパラメータ
};

// 子スレッドに渡す情報
struct GNT_SERVER_THREAD_INFO
{
	pthread_t parentid;					// 親スレッドID
	pthread_t currentid;				// 自身のスレッドID
	uint32_t index;						// 連番
	int32_t recvbuf_munit;				// 受信バッファの格納メモリユニット
	int32_t recvinfo_munit;				// 受信情報管理構造体の格納メモリユニット
	int32_t recvmsg_munit;				// 受信メッセージ一時格納領域
	int32_t free_munit;					// 自由利用可能なメモリユニット
	GNT_SOCKET_OPTION *option;			// ソケットオプション
	GNT_SOCKPARAM sockParam;			// socketパラメータ
};

// 受信コールバックに渡す情報
struct GNT_RECV_INFO
{
	int32_t recvbuf_munit;					// 受信したバッファの格納メモリユニット
	ssize_t recvlen;						// 受信した文字列のサイズ
	int recvfrom;							// 受信相手
	void *tinfo;							// 接続管理情報
};

// 送信コールバックに渡す情報
struct GNT_SEND_INFO
{
	int32_t send_munit;					// 送信相手の格納メモリユニット
	ssize_t sendlen;					// 受信した文字列のサイズ
};

// ソケットオプション構造体の初期化
void gnt_initialize_socket_option( 
	  GNT_SOCKET_OPTION *option
	, char* hostname
	, char* portnum
	, uint8_t socket_type
	, uint8_t mode
	, uint8_t protocol
	, size_t maxconnection
	, GNT_MEMORY_POOL* __mp
	, GNT_MEMORY_POOL* __mmapp
	, GNT_CALLBACK connection_start_callback
	, GNT_CALLBACK send_finish_callback
	, GNT_CALLBACK recv_callback
);

// 接続管理情報の初期化
void gnt_initialize_child_info( GNT_SOCKET_OPTION *option, struct GNT_SERVER_CHILD_INFO* tinfo );
// 接続管理情報の初期化
void gnt_initialize_fork_info( GNT_SOCKET_OPTION *option, struct GNT_SERVER_FORK_INFO* tinfo );
// 接続管理情報の初期化
void gnt_initialize_thread_info( GNT_SOCKET_OPTION *option, struct GNT_SERVER_THREAD_INFO* tinfo );

// パケット送信
ssize_t gnt_send( GNT_SOCKET_OPTION *option, GNT_SOCKPARAM *pSockParam, char *buf, size_t size, int flag );
// tcpパケットの送信
ssize_t gnt_send_all( int soc, char *buf, size_t size, int flag );
// udpパケットの送信
ssize_t gnt_sendto_all( int soc, char *buf, size_t size, int flag, struct sockaddr *pfrom, socklen_t fromlen );
// ソケットオプションの設定
void gnt_set_sock_option( GNT_SOCKET_OPTION *option );
// ソケット制御群の生成
void* gnt_socket( GNT_SOCKET_OPTION *option );
// ソケット処理破棄
void gnt_free_socket( GNT_SOCKET_OPTION *option );
// 通信処理( シングルタスクモード )
void gnt_single_task_server( GNT_SOCKET_OPTION *option );
// 通信処理( select多重モード )
void gnt_select_server( GNT_SOCKET_OPTION *option );
// 通信処理( pool多重モード )
void gnt_pool_server( GNT_SOCKET_OPTION *option );
// 通信処理( epool多重モード )
void gnt_epool_server( GNT_SOCKET_OPTION *option );
// 通信処理( preforkモード )
void gnt_prefork_server( GNT_SOCKET_OPTION *option );
// 通信処理( スレッドモード )
void gnt_thread_server( GNT_SOCKET_OPTION *option );
// flockによる同時接続ループ
void gnt_accept_loop_fork( struct GNT_SERVER_FORK_INFO *forkinfo );
// flockによる受信ループ
void gnt_recv_loop_fork( struct GNT_SERVER_FORK_INFO *forkinfo );
// flockによるUDP受信ループ
void gnt_recvfrom_loop_fork( struct GNT_SERVER_FORK_INFO *forkinfo );
// threadによる同時接続ループ
void* gnt_accept_thread( void* arg );
// threadによる受信ループ
void gnt_recv_loop_thread( struct GNT_SERVER_THREAD_INFO *tinfo );
// threadによるUDP受信ループ
void* gnt_recvfrom_loop_thread( void* arg );
// ブロッキングモードの指定
int gnt_set_block( int fd, int flag );
// サーバーソケットの生成
int gnt_server_socket( GNT_SOCKET_OPTION *option, int inetflag );
// タイムアウトなしのソケット接続
int gnt_client_socket( GNT_SOCKET_OPTION *option );
// アドレス情報の取得
int gnt_get_sockaddr_info( GNT_SOCKET_OPTION *option, struct sockaddr_storage *saddr, socklen_t *addr_len );
// パケットの解析
int gnt_pre_packetfilter( GNT_SOCKET_OPTION *option, struct GNT_RECV_INFO *rinfo, GNT_SOCKPARAM *pSockParam, int32_t recvmsg_munit );
// ソケットを閉じる
int closeSocket( int* sock, char* error );
// 受信処理(タイムアウトあり)
ssize_t recv_with_timeout( char mode, int32_t timeoutsec, int soc, char *buf, size_t bufsize, int flag );
ssize_t recv_timeout_by_nonblocking( int soc, char *buf, size_t bufsize, int flag, int32_t timeoutsec );
ssize_t recv_timeout_by_select( int soc, char *buf, size_t bufsize, int flag, int32_t timeoutsec );
ssize_t recv_timeout_by_poll( int soc, char *buf, size_t bufsize, int flag, int32_t timeoutsec );
ssize_t recv_timeout_by_epoll( int soc, char *buf, size_t bufsize, int flag, int32_t timeoutsec );
ssize_t recv_timeout_by_ioctl( int soc, char *buf, size_t bufsize, int flag, int32_t timeoutsec );
ssize_t recv_timeout_by_setsockopt( int soc, char *buf, size_t bufsize, int flag, int32_t timeoutsec );

// 独自ハンドシェイク
int32_t gnt_socket_phase( GNT_SOCKET_OPTION *option, struct GNT_RECV_INFO *rinfo, GNT_SOCKPARAM *pSockParam );
// tcpソケットから受信したデータを取り出す( プロトコルの解析 )
ssize_t gnt_parse_socket_binary( GNT_SOCKET_OPTION *option, GNT_SOCKPARAM *pSockParam, uint8_t* u8buf, size_t size, uint32_t basebuf_munit );
// メッセージの送信
ssize_t gnt_send_msg( GNT_SOCKET_OPTION *option, GNT_SOCKPARAM *pSockParam, const char* msg, ssize_t size );
// UDPメッセージの送信
ssize_t gnt_send_udpmsg( GNT_SOCKET_OPTION *option, GNT_SOCKPARAM *pSockParam, const char* msg, ssize_t size, int flag, struct sockaddr *pfrom, socklen_t fromlen  );

// websocketから受信したデータを取り出す( プロトコルの解析 )
ssize_t gnt_parse_webSocket_binary( GNT_SOCKET_OPTION *option, GNT_SOCKPARAM *pSockParam, uint8_t* u8buf, size_t size, uint32_t basebuf_munit );
// websocketへメッセージの送信
ssize_t gnt_send_websocket_msg( GNT_SOCKET_OPTION *option, GNT_SOCKPARAM *pSockParam, const char* msg, ssize_t size );
// websocketハンドシェイクの送信
int   sendHandShakeParam( int socket, GNT_SOCKET_OPTION *option, GNT_SOCKPARAM *pSockParam );
// socket構造体の初期化
void  initSocketParam( GNT_SOCKPARAM *pSockParam );
// websocketハンドシェイク情報の解析
int   parseHandShakeParam( GNT_SOCKET_OPTION *option, GNT_SOCKPARAM *pSockParam, char* target );
// HTTPヘッダーの追加
void addHttpHeader( GNT_SOCKET_OPTION *option, GNT_SOCKPARAM *pSockParam, int headerindex, char* pbuf );
// HTTPヘッダーの取得
char* getHttpHeader( GNT_SOCKET_OPTION *option, GNT_SOCKPARAM *pSockParam, int headerindex );
// socket構造体の解放
void  freeSocketParam( GNT_SOCKPARAM *pSockParam );
// シンプルなキー入力クライアント送受信処理
void gnt_send_recv_client_term( GNT_SOCKET_OPTION *option );
// 受信専用クライアント送受信処理( リレーからのパケット受け取りなど )
void gnt_thread_client( GNT_SOCKET_OPTION *option );
// 受信専用クライアントスレッド
void* gnt_recv_thread_client( void* arg );

void gnt_inetdump();

#endif /*_GNT_SOCKET_H_*/
