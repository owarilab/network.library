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

#include "gdt_core.h"
#include "gdt_system.h"
#include "gdt_socket.h"
#include "gdt_memory_allocator.h"
#include "gdt_io.h"
#include "gdt_string.h"
#include "gdt_random.h"
#include "gdt_hash.h"
#include "gdt_script.h"
#include "gdt_sha1.h"
#include "gdt_sha2.h"

GDT_MEMORY_POOL* __mp;
GDT_MEMORY_POOL* __mmapp;

//typedef struct GDT_SERVER_THREAD_INFO STRUCT_SERVER_INFO;
typedef struct GDT_SERVER_CHILD_INFO STRUCT_SERVER_INFO;

void* _connection_start_callback( void* args );
void* _send_finish_callback( void* args );
void* _recv_callback( void* args );
void* _close_callback( void* args );

/*
 * 接続完了コールバック
 */
void* _connection_start_callback( void* args )
{
#ifdef __GDT_DEBUG__
	printf(  "_connection_start_callback\n" );
#endif
	STRUCT_SERVER_INFO *tinfo = (STRUCT_SERVER_INFO *)args;
printf( "@@@ host:%s, serv:%s\n",tinfo->hbuf, tinfo->sbuf );
	return ( (void *) NULL );
}

/*
 * 送信コールバック
 */
void* _send_finish_callback( void* args )
{
#ifdef __GDT_DEBUG__
	printf(  "_send_finish_callback\n" );
#endif
	return ( (void *) NULL );
}

/*
 * 受信コールバック
 */
void* _recv_callback( void* args )
{
#ifdef __GDT_DEBUG__
	printf(  "_recv_callback\n" );
#endif
	struct GDT_RECV_INFO *rinfo;
	STRUCT_SERVER_INFO *tinfo;
	STRUCT_SERVER_INFO *tmptinfo;
	int i;
	char *pbuf;
	do{
		rinfo = (struct GDT_RECV_INFO *) args;
		tinfo = (STRUCT_SERVER_INFO *)rinfo->tinfo;
		pbuf = (char*)gdt_upointer( __mp, rinfo->recvbuf_munit );
		// send self user
		gdt_send( tinfo->option, &tinfo->sockParam, pbuf, rinfo->recvlen, 0 );
		// send to connecting users
		for( i = 0; i < tinfo->option->maxconnection; i++ )
		{
			if( tinfo->option->connection_munit >= 0 )
			{
				tmptinfo = gdt_offsetpointer( tinfo->option->__mp, tinfo->option->connection_munit, sizeof( STRUCT_SERVER_INFO ), i );
				if( tmptinfo->sockParam.acc > 0 && tmptinfo->sockParam.acc != tinfo->sockParam.acc )
				{
					gdt_send( tinfo->option, &tmptinfo->sockParam, pbuf, rinfo->recvlen, 0 );
				}
			}
		}
	}while( false );
	return ( (void *) NULL );
}

/*
 * 切断コールバック
 */
void* _close_callback( void* args )
{
#ifdef __GDT_DEBUG__
	printf(  "_close_callback\n" );
#endif
	return ( (void *) NULL );
}

/*
 * main関数
 * 
 * param - argc : コマンドライン引数の数
 *         argv : コマンドライン引数の配列
 *         envp : 環境変数の配列
 * server -p 1212 -v 4
 */
int main( int argc, char *argv[], char *envp[] )
{
	int exe_code = EX_OK;
	char* phostname;
	char hostname[256];
	char portnum[32];
	int inetflag = 0;
	int result;
	do{
		__mp = NULL;
		__mmapp = NULL;
		if( gdt_initialize_memory( 
			  &__mp
			, SIZE_MBYTE * 24						// 初期サイズ
			, SIZE_MBYTE * 24						// 最大メモリサイズ
			, MEMORY_ALIGNMENT_SIZE_BIT_64			// 64ビットアラインメント
			, FIX_MUNIT_SIZE						// 固定メモリユニット数
			, 1										// 初期メモリユニット数
			, SIZE_KBYTE * 16						// 最少メモリ確保数
		) <= 0 ){
			printf(  "gdt_initialize_memory error\n" );
			break;
		}

		if( gdt_initialize_mmapmemory( 
			  &__mmapp
			, SIZE_MBYTE * 24						// 初期サイズ
			, SIZE_MBYTE * 24						// 最大メモリサイズ
			, MEMORY_ALIGNMENT_SIZE_BIT_64			// 64ビットアラインメント
			, FIX_MUNIT_SIZE						// 固定メモリユニット数( 0から指定値まで決まった番号でメモリユニットを扱いたい場合指定 )
			, 16									// 初期メモリユニット数
			, SIZE_KBYTE * 16						// 最少メモリ確保数
		) <= 0 ){
			printf(  "gdt_initialize_mmapmemory error\n" );
			break;
		}
		
		(void) gdt_set_defaultsignal();

		//(void) daemonize( 0, 0 );

		phostname = NULL;
		memset( hostname, 0, sizeof( hostname ) );
		memset( portnum, 0, sizeof( portnum ) );
		snprintf( hostname, sizeof( hostname ) -1, "localhost" );
		snprintf( portnum, sizeof( portnum ) -1, "1024" );
		
		while( ( result = getopt( argc, argv, "v:h:p:" ) ) != -1 )
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
		
		GDT_SOCKET_OPTION option;
		gdt_initialize_socket_option( 
			  &option
			, phostname
			, portnum
			, SOKET_TYPE_SERVER_TCP
			, SOCKET_MODE_SELECT
			, PROTOCOL_PLAIN
			, 128
			, __mp
			, __mmapp
			, _connection_start_callback
			, _send_finish_callback
			, _recv_callback
			, _close_callback
		);
		option.queuelen			= 128;
		option.recvbufsize		= 1024;
		option.queuebufsize		= 1024;
		option.inetflag			= inetflag;

		gdt_socket( &option );

		for(;;)
		{
			(void) sleep(10);
		}

		gdt_free_socket( &option );

		if( gdt_free( __mp ) <= 0 ){
			printf(  "gdt_free error\n" );
		}
		if( gdt_free( __mmapp ) <= 0 ){
			printf(  "gdt_free error\n" );
		}
	}while( false );
	return exe_code;
}

