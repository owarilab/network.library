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

/*
 * main関数
 * 
 * param - argc : コマンドライン引数の数
 *         argv : コマンドライン引数の配列
 *         envp : 環境変数の配列
 */
int main( int argc, char *argv[], char *envp[] )
{
	clock_t s_cl,e_cl;	// 処理時間計測用
	s_cl = clock();
	int exe_code = EX_OK;
	do{
		// メモリ確保
		if( gnt_initialize_memory( 
			  &__mp
			, SIZE_MBYTE * 8						// 初期サイズ
			, SIZE_MBYTE * 8						// 最大メモリサイズ
			, MEMORY_ALIGNMENT_SIZE_BIT_64			// 64ビットアラインメント
			, 2										// 固定メモリユニット数
			, 16									// 初期メモリユニット数
			, SIZE_KBYTE * 16						// 最少メモリ確保数
		) <= 0 ){
			printf(  "gnt_initialize_memory error\n" );
			break;
		}
		
		// 共有メモリ確保
		if( gnt_initialize_mmapmemory( 
			  &__mmapp
			, SIZE_MBYTE * 32						// 初期サイズ
			, SIZE_MBYTE * 32						// 最大メモリサイズ
			, MEMORY_ALIGNMENT_SIZE_BIT_64			// 64ビットアラインメント
			, 1										// 固定メモリユニット数
			, 16									// 初期メモリユニット数
			, SIZE_KBYTE * 16						// 最少メモリ確保数
		) <= 0 ){
			printf(  "gnt_initialize_mmapmemory error\n" );
			break;
		}
		
		// コマンドライン引数をグローバル変数へセット
		(void) set_global_argv( &argc, &argv, &envp );
		
		// プロセス名の変更
		setProcName( "gnt_server: parent process", 256 );
		
		// 実行環境取得
		(void) getEnv();
		
		// デーモン化
		//(void) daemonize( 0, 0 );
		
		// シグナルハンドラの設定
		(void) setSignalOption();
		
		// 各アプリケーションのメイン処理実行
		exe_code = main_proc();
		
		// メモリ解放
		if( gnt_free( __mp ) <= 0 ){
			printf(  "gnt_free error\n" );
		}
		if( gnt_free( __mmapp ) <= 0 ){
			printf(  "gnt_free error\n" );
		}
	}while( false );
	e_cl = clock();
	printf(  "time = %lf[s]\n", (double)( e_cl - s_cl ) / CLOCKS_PER_SEC );
	return exe_code;
}

/*
 * メイン処理実行
 */
int main_proc()
{
	char* phostname;
	char hostname[256];
	char portnum[32];
	int inetflag = 0;
	int result;
	int exe_code = EX_OK;
	GNT_SOCKET_OPTION option;
	GNT_SOCKET_OPTION option_relay;
	do{
		// 乱数初期化
		srand_32();
		srand_64();
		
		// 初期値設定
		phostname = NULL;
		memset( hostname, 0, sizeof( hostname ) );
		memset( portnum, 0, sizeof( portnum ) );
		snprintf( hostname, sizeof( hostname ) -1, "localhost" );
		snprintf( portnum, sizeof( portnum ) -1, "1024" );
		
		// コマンドライン引数取得
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
				phostname = hostname;
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
		
		// relayに接続
		gnt_initialize_socket_option( 
			  &option_relay
			, "localhost"
			, "25001"
			, SOKET_TYPE_CLIENT_TCP
			, SOCKET_MODE_CLIENT_THREAD
			, 1
			, __mp
			, __mmapp
			, NULL
			, NULL
			, RecvCallback_Relay
		);
		
		// サーバー接続
		gnt_socket( &option_relay );
		
		// サーバーソケット生成オプション
		gnt_initialize_socket_option( 
			  &option
			, phostname
			, portnum
			, SOKET_TYPE_SERVER_TCP
			, SOCKET_MODE_THREAD
			, 32
			, __mp
			, __mmapp
			, AcceptCallback
			, SendCallback
			, RecvCallback
		);
		
		// ソケット追加情報
		option.queuelen			= 4096;
		option.recvbufsize		= 2048;
		option.queuebufsize		= 4096;
		option.inetflag			= inetflag;
		option.relay			= &option_relay;
		
		// サーバー接続
		gnt_socket( &option );
		
		for(;;)
		{
			(void) sleep(10);
		}
		
		// 接続情報の破棄
		gnt_free_socket( &option );
	}while( false );
	return exe_code;
}

/*
 * 接続完了コールバック
 */
void* AcceptCallback( void* args )
{
	printf(  "AcceptCallback\n" );
	return ( (void *) NULL );
}

/*
 * 送信コールバック
 */
void* SendCallback( void* args )
{
	printf(  "SendCallback\n" );
	return ( (void *) NULL );
}

/*
 * 受信コールバック
 */
void* RecvCallback( void* args )
{
	struct GNT_RECV_INFO *rinfo;
	STRUCT_SERVER_INFO *tinfo;
	STRUCT_SERVER_INFO *relay_tinfo;
	char *pbuf;
	do{
		printf(  "RecvCallback\n" );
		rinfo = (struct GNT_RECV_INFO *) args;
		pbuf  = (char*)gnt_upointer( __mp, rinfo->recvbuf_munit );
		tinfo = ( STRUCT_SERVER_INFO *)rinfo->tinfo;
		// relayに転送
		relay_tinfo = ( struct GNT_SERVER_THREAD_INFO *)gnt_upointer( tinfo->option->relay->__mp, tinfo->option->relay->connection_munit );
		gnt_send( tinfo->option->relay, &relay_tinfo->sockParam, pbuf, rinfo->recvlen, 0 );
		if( pbuf[rinfo->recvlen-1] == '\n' ){
			pbuf[rinfo->recvlen-1] = '\0';
			rinfo->recvlen -= 1;
		}
		printf( "client msg : %s\n" , pbuf );
	}while( false );
	return ( void * ) NULL;
}

/*
 * 受信コールバック( relay )
 */
void* RecvCallback_Relay( void* args )
{
	struct GNT_RECV_INFO *rinfo;
	char *pbuf;
	do{
		printf( "RecvCallback_Relay\n" );
		rinfo = (struct GNT_RECV_INFO *)args;
		pbuf = (char*)gnt_upointer( __mp, rinfo->recvbuf_munit );
		if( pbuf[0] != '\0' )
		{
			printf( "relay : %s\n", pbuf );
		}
	}while( false );
	return ( (void *) NULL );
}
