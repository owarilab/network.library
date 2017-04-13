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

#include "gdt_socket.h"
#include "gdt_memory_allocator.h"

void* on_connect( void* args );
int on_send( void* connection, GDT_SOCKET_ID id, char *buf, size_t bufsize, int flag );
void* on_sent( void* args );
void* on_recv( void* args );
void* on_close( void* args );

/*
 * main関数
 * 
 * param - argc : コマンドライン引数の数
 *         argv : コマンドライン引数の配列
 *         envp : 環境変数の配列
 * ipv6ではnicの指定を追加する : ex) fe80::20c:29ff:fe16:a0ad%eth0
 * ./client -v4 -hlocalhost -p1024
 * ./client -v6 -hlocalhost -p1024
 */
int main( int argc, char *argv[], char *envp[] )
{
	int exe_code = EX_OK;
	int result;
	char hostname[256];
	char portnum[32];
	int inetflag = 0;
	GDT_SOCKET_OPTION option;
	GDT_MEMORY_POOL* __mp = NULL;
	do{
		if( gdt_initialize_memory( &__mp, SIZE_MBYTE * 1, SIZE_MBYTE * 1, MEMORY_ALIGNMENT_SIZE_BIT_64, 16, 16, SIZE_KBYTE * 16) <= 0 )
		{
			printf(  "gdt_initialize_memory error\n" );
			break;
		}
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

		//gdt_initialize_socket_option( &option, hostname, portnum, SOKET_TYPE_CLIENT_TCP, SOCKET_MODE_SIMPLE_TERM, PROTOCOL_PLAIN, 1, __mp, NULL, on_connect, on_sent, on_recv, on_close );
		gdt_initialize_socket_option( &option, hostname, portnum, SOKET_TYPE_CLIENT_TCP, SOCKET_MODE_SIMPLE_TERM, PROTOCOL_SIMPLE, 1, __mp, NULL, on_connect, on_sent, on_recv, on_close );
		option.inetflag = inetflag;
		set_user_send_event(&option, on_send);
		gdt_socket( &option );
		
		size_t free_size = gdt_free( __mp );
		if( free_size <= 0 ){
			printf(  "gdt_free error\n" );
			break;
		}
	}while( false );
	return exe_code;
}

void* on_connect( void* args )
{
	struct GDT_SERVER_CONNECTION_INFO * tinfo;
	tinfo = (struct GDT_SERVER_CONNECTION_INFO *) args;
	gdt_send( tinfo->option, &tinfo->sockparam, "ok", 2, 0xff );
	return ( (void *) NULL );
}

int on_send( void* connection, GDT_SOCKET_ID id, char *buf, size_t bufsize, int flag )
{
	struct GDT_SERVER_CONNECTION_INFO * tinfo;
	tinfo = (struct GDT_SERVER_CONNECTION_INFO *) connection;
	return gdt_send( tinfo->option, &tinfo->sockparam, buf, bufsize, 0x01 );
}

void* on_sent( void* args )
{
	return ( (void *) NULL );
}

void* on_recv( void* args )
{
	struct GDT_RECV_INFO *rinfo;
	struct GDT_SERVER_CONNECTION_INFO * tinfo;
	char *pbuf;
	rinfo = (struct GDT_RECV_INFO *) args;
	tinfo = (struct GDT_SERVER_CONNECTION_INFO *)rinfo->tinfo;
	pbuf = (char*)gdt_upointer( tinfo->option->__mp, rinfo->recvbuf_munit );
	pbuf[rinfo->recvlen] = '\0';
	//int i;
	//for( i = 0; i < rinfo->recvlen; i++ ){ printf( "%02X ", pbuf[i] ); }
	//printf( "\n" );
	printf( "@@@ recv tcp message(%d:%dbyte) : %s\n", tinfo->sockparam.payload_type, (int)tinfo->sockparam.payloadlen, pbuf );
	return ( (void *) NULL );
}

void* on_close( void* args )
{
	return ( (void *) NULL );
}

