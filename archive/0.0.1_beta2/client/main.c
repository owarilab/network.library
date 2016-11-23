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

#include "app_define.h"

// メモリプール
GNT_MEMORY_POOL* __mp = NULL;

void* _connection_start_callback( void* args );
void* _send_finish_callback( void* args );
void* _recv_callback( void* args );

/*
 * メイン処理実行
 * ipv6ではnicの指定を追加する : ex) fe80::20c:29ff:fe16:a0ad%eth0
 * ./client -v4 -hlocalhost -p1024
 * ./client -v6 -hlocalhost -p1024
 */
int mainProc()
{
	int exe_code = EX_OK;
	int result;
	char hostname[256];
	char portnum[32];
	int inetflag = 0;
	GNT_SOCKET_OPTION option;
	do{
		// 乱数初期化
		srand_32();
		srand_64();
		
		// h : ホスト名, p : ポート番号
		memset( hostname, 0, sizeof( hostname ) );
		memset( portnum, 0, sizeof( portnum ) );
		snprintf( hostname, sizeof( hostname ) -1, "localhost" );
		snprintf( portnum, sizeof( portnum ) -1, "1024" );
		while( ( result = getopt( (*argc_), (*argv_), "v:h:p:" ) ) != -1 )
		{
			switch(result)
			{
			case 'v':
				fprintf( stdout,"%c %s\n", result, optarg );
				if( optarg[0] == '6' ){
					inetflag = 1;
				}
				else if( optarg[0] == '4' ){
					inetflag = 0;
				}
				break;
			case 'h':
				fprintf( stdout,"%c %s\n", result, optarg );
				snprintf( hostname, sizeof( hostname ) -1, "%s", optarg );
				break;
			case 'p':
				fprintf( stdout,"%c %s\n", result, optarg );
				snprintf( portnum, sizeof( portnum ) -1, "%s", optarg );
				break;
			case ':':
				fprintf( stdout,"%c needs value\n", result );
				break;
			case '?':
				fprintf(stdout,"unknown\n");
				break;
			}
		}
		
		// クライアントソケット生成オプション初期化
		gnt_initialize_socket_option( 
			  &option
			, hostname // "fe80::20c:29ff:fe16:a0ad%eth0"
			, portnum  // "1024"
			, SOKET_TYPE_CLIENT_TCP
			, SOCKET_MODE_SIMPLE_TERM
			, PROTOCOL_PLAIN
			, 1
			, __mp
			, NULL
			, _connection_start_callback
			, _send_finish_callback
			, _recv_callback
		);
		option.recvbufsize		= 4096;
		option.queuebufsize		= 4096;
		option.inetflag = inetflag;
		
		// サーバー接続
		gnt_socket( &option );
		
		// ソケット切断
		if( option.sockid > -1 ){
			(void) close( option.sockid );
		}
	}while( false );
	return exe_code;
}

//------------------------------------------------------------
// main関数
// 
// param - argc : コマンドライン引数の数
//         argv : コマンドライン引数の配列
//         envp : 環境変数の配列
//------------------------------------------------------------
int main( int argc, char *argv[], char *envp[] )
{
	clock_t s_cl,e_cl;	// 処理時間計測用
	s_cl = clock();
	int exe_code = EX_OK;
	do{
		// メモリ確保
		if( gnt_initialize_memory( 
			  &__mp
			, SIZE_KBYTE * 256						// 初期サイズ
			, SIZE_KBYTE * 256						// 最大メモリサイズ
			, MEMORY_ALIGNMENT_SIZE_BIT_64			// 64ビットアラインメント
			, 16									// 固定メモリユニット数
			, 16									// 初期メモリユニット数
			, SIZE_KBYTE * 16						// 最少メモリ確保数
		) <= 0 )
		{
			printf(  "gnt_initialize_memory error\n" );
			break;
		}
		
		// コマンドライン引数をグローバル変数へセット
		(void) gnt_set_argv( &argc, &argv, &envp );
		
		// 実行環境取得
		(void) gnt_get_env();
		
		// デーモン化
		//(void) daemonize( 0, 0 );
		
		// 各アプリケーションのメイン処理実行
		exe_code = mainProc();
		
		// メモリ解放( これ以降は再度メモリ確保しないとポインタ参照がおかしくなる )
		size_t free_size = gnt_free( __mp );
		if( free_size <= 0 ){
			printf(  "gnt_free error\n" );
			break;
		}
	}while( false );
	e_cl = clock();
	printf(  "time = %lf[s]\n", (double)( e_cl - s_cl ) / CLOCKS_PER_SEC );
	return exe_code;
}

/*
 * 接続完了コールバック
 */
void* _connection_start_callback( void* args )
{
#ifdef __GNT_DEBUG__
	printf(  "_connection_start_callback aaa\n" );
#endif
	struct GNT_SERVER_CHILD_INFO * tinfo;
	tinfo = (struct GNT_SERVER_CHILD_INFO *) args;
	gnt_send( tinfo->option, &tinfo->sockParam, "ok", 2, 0 );
	return ( (void *) NULL );
}

/*
 * 送信コールバック
 */
void* _send_finish_callback( void* args )
{
#ifdef __GNT_DEBUG__
	printf(  "_send_finish_callback\n" );
#endif
	return ( (void *) NULL );
}

/*
 * 受信コールバック
 */
void* _recv_callback( void* args )
{
	struct GNT_RECV_INFO *rinfo;
	struct GNT_SERVER_CHILD_INFO * tinfo;
	char *pbuf;
	rinfo = (struct GNT_RECV_INFO *) args;
	tinfo = (struct GNT_SERVER_CHILD_INFO *)rinfo->tinfo;
	pbuf = (char*)gnt_upointer( __mp, rinfo->recvbuf_munit );
	// 文字列整形
	pbuf[rinfo->recvlen] = '\0';
	(void) printf( "[id:%d]>%s\n", tinfo->option->sockid, pbuf );
	return ( (void *) NULL );
}
