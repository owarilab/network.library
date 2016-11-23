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
 * メイン処理実行
 * server -p 1212
 */
int main_proc()
{
	char* phostname;
	char hostname[256];
	char portnum[32];
	int inetflag = 0;
	int result;
	int exe_code = EX_OK;
	do{
		// 乱数初期化
		srand_32();
		srand_64();
		
		phostname = NULL;
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
		
		// サーバーソケット生成オプション
		GNT_SOCKET_OPTION option;
		gnt_initialize_socket_option( 
			  &option
			, phostname
			, portnum
			, SOKET_TYPE_SERVER_TCP
			, SOCKET_MODE_THREAD
			, PROTOCOL_PLAIN
			, 8
			, __mp
			, __mmapp
			, _connection_start_callback
			, _send_finish_callback
			, _recv_callback
		);
		option.queuelen			= 16;
		option.recvbufsize		= 2048;
		option.queuebufsize		= 4096;
		option.inetflag			= inetflag;
		// サーバー接続
		gnt_socket( &option );
		
		// メインループ
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
void* _connection_start_callback( void* args )
{
#ifdef __GNT_DEBUG__
	printf(  "_connection_start_callback\n" );
#endif
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
#ifdef __GNT_DEBUG__
	printf(  "_recv_callback\n" );
#endif
	struct GNT_RECV_INFO *rinfo;
	STRUCT_SERVER_INFO *tinfo;
	char *pbuf;
	do{
		rinfo = (struct GNT_RECV_INFO *) args;
		tinfo = (STRUCT_SERVER_INFO *)rinfo->tinfo;
		pbuf = (char*)gnt_upointer( __mp, rinfo->recvbuf_munit );
		if( pbuf[0] != '\0' && pbuf[0] != '\n' )
		{
			if( pbuf[rinfo->recvlen-1] == '\n' ){
				pbuf[rinfo->recvlen-1] = '\0';
				rinfo->recvlen = rinfo->recvlen - 1;
			}
			// パケットの送信
			gnt_send( tinfo->option, &tinfo->sockParam, pbuf, rinfo->recvlen, 0 );
			// キューに格納
			pushQueue( tinfo->option->__mmapp, tinfo->option->mq_munit, pbuf, rinfo->recvlen );
		}
	}while( false );
	return ( (void *) NULL );
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
		__mp = NULL;
		__mmapp = NULL;
		// メモリ確保
		if( gnt_initialize_memory( 
			  &__mp
			, SIZE_MBYTE * 8						// 初期サイズ
			, SIZE_MBYTE * 8						// 最大メモリサイズ
			, MEMORY_ALIGNMENT_SIZE_BIT_64			// 64ビットアラインメント
			, 16									// 固定メモリユニット数
			, 16									// 初期メモリユニット数
			, SIZE_KBYTE * 16						// 最少メモリ確保数
		) <= 0 ){
			printf(  "gnt_initialize_memory error\n" );
			break;
		}
		
		// 共有メモリ確保
		if( gnt_initialize_mmapmemory( 
			  &__mmapp
			, SIZE_MBYTE * 4						// 初期サイズ
			, SIZE_MBYTE * 4						// 最大メモリサイズ
			, MEMORY_ALIGNMENT_SIZE_BIT_64			// 64ビットアラインメント
			, 0										// 固定メモリユニット数( 0から指定値まで決まった番号でメモリユニットを扱いたい場合指定 )
			, 16									// 初期メモリユニット数
			, SIZE_KBYTE * 16						// 最少メモリ確保数
		) <= 0 ){
			printf(  "gnt_initialize_mmapmemory error\n" );
			break;
		}
		
		// コマンドライン引数をグローバル変数へセット
		(void) gnt_set_argv( &argc, &argv, &envp );
		
		// プロセス名の変更
		// setProcName( "gnt_server: parent process", 256 );
		
		// 実行環境取得
		(void) gnt_get_env();
		
		// signalの挙動を設定
		(void) gnt_set_defaultsignal();

		// デーモン化
		//(void) daemonize( 0, 0 );
		
		// 各アプリケーションのメイン処理実行
		exe_code = main_proc();
		
		// メモリ解放( これ以降は再度メモリ確保しないとポインタ参照がおかしくなる )
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

