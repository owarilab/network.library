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
GNT_MEMORY_POOL* __mp;
// mmapメモリプール
GNT_MEMORY_POOL* __mmapp;

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
	
	char* phostname;
	char hostname[256];
	char portnum[32];
	int inetflag = 0;
	int result;
	GNT_SOCKET_OPTION option;
	
	do{
		__mp = NULL;
		__mmapp = NULL;
		// メモリ確保
		if( gnt_initialize_memory( 
			  &__mp
			, SIZE_MBYTE * 1						// 初期サイズ
			, SIZE_MBYTE * 1						// 最大メモリサイズ
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
		
		// コマンドライン引数の確認( getoptを使用する場合は -> #include <unistd.h> )
		// 第３引数 : オプション( オプションにパラメータがある場合は後ろに「:」を付ける )
		int opt;
		while( ( opt = getopt( argc, argv, "h" ) ) != -1 )
		{
			switch( opt )
			{
				case 'h':
					fprintf( stdout, "Usage : %s \n -h : help\n", argv[0] );
					exit( 0 );
					break;
				case '?':
					fprintf( stdout, "Usage : %s \n -h : help\n", argv[0] );
					exit( 0 );
					break;
			}
		}
		
		// コマンドライン引数をグローバル変数へセット
		(void) set_global_argv( &argc, &argv, &envp );
		
		// 実行環境取得
		(void) getEnv();
		
		// コマンドライン引数の出力
		// (void) print_argv();
		
		// シグナルの設定( defaultで用意しているものを使用 )
		(void) setSignalOption();
		
		// デーモン化
		// (void) daemonize( 0, 0 );
		
		// プロセス名の変更
		setProcName( "gnt_relay: parent process", 256 );
		
		// 乱数初期化
		srand_32();
		srand_64();
		
		// 初期値設定
		phostname = NULL;
		memset( hostname, 0, sizeof( hostname ) );
		memset( portnum, 0, sizeof( portnum ) );
		snprintf( hostname, sizeof( hostname ) -1, "localhost" );
		snprintf( portnum, sizeof( portnum ) -1, "25001" );
		
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
		option.queuelen			= 128;
		option.recvbufsize		= 4096;
		option.queuebufsize		= 4096;
		option.inetflag			= inetflag;
		
		// サーバー接続
		gnt_socket( &option );
		
		for(;;){
			sleep( 10 );
		}
		
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
	STRUCT_SERVER_INFO *tinfo;
	STRUCT_SERVER_INFO *tmptinfo;
	struct GNT_RECV_INFO *rinfo;
	char *pbuf;
	int i;
	do{
		printf( "RecvCallback\n" );
		rinfo = (struct GNT_RECV_INFO *)args;
		tinfo = (STRUCT_SERVER_INFO *)rinfo->tinfo;
		pbuf = (char*)gnt_upointer( __mp, rinfo->recvbuf_munit );
		printf( "relay msg : %s\n" , pbuf );
		if( pbuf[0] != '\0' )
		{
			for( i = 0; i < tinfo->option->maxconnection; i++ )
			{
				if( tinfo->option->connection_munit >= 0 )
				{
					tmptinfo = gnt_offsetpointer( tinfo->option->__mp, tinfo->option->connection_munit, sizeof( STRUCT_SERVER_INFO ), i );
					if( tmptinfo->sockParam.acc >= 0 && tmptinfo->sockParam.acc != tinfo->sockParam.acc )
					{
						gnt_send( tinfo->option, &tmptinfo->sockParam, pbuf, rinfo->recvlen, 0 );
					}
				}
			}
		}
	}while( false );
	return ( (void *) NULL );
}
