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

#include "gdt_core.h"
#include "gdt_system.h"
#include "gdt_socket.h"
#include "gdt_memory_allocator.h"
#include "gdt_io.h"
#include "gdt_string.h"
#include "gdt_hash.h"
#include "gdt_script.h"
#include "gdt_json.h"

void* on_connect( void* args );
void* on_sent( void* args );
void* on_recv( void* args );
void* on_recv_udp(void* args);
void* on_close( void* args );

void* on_connect( void* args )
{
	GDT_SERVER_CONNECTION_INFO *tinfo = (GDT_SERVER_CONNECTION_INFO *)args;
	printf( "@@@ new connection : host=%s, serv=%s\n",tinfo->hbuf, tinfo->sbuf );
	return ( (void *) NULL );
}

void* on_sent( void* args )
{
	return ( (void *) NULL );
}

void* on_recv( void* args )
{
	struct GDT_RECV_INFO *rinfo;
	GDT_SERVER_CONNECTION_INFO *tinfo;
	char *pbuf;
	do{
		rinfo = (struct GDT_RECV_INFO *) args;
		tinfo = rinfo->tinfo;
		pbuf = (char*)gdt_upointer( tinfo->option->memory_pool, rinfo->recvbuf_munit );
		pbuf[rinfo->recvlen] = '\0';
		printf( "@@@ recv tcp message(%d:%dbyte) : %s, host=%s, serv=%s\n", tinfo->sockparam.payload_type, (int)tinfo->sockparam.payloadlen, pbuf,tinfo->hbuf, tinfo->sbuf );
		gdt_send( tinfo->option, &tinfo->sockparam, pbuf, rinfo->recvlen, 0x10 );
		//gdt_send_broadcast( tinfo->option, pbuf, rinfo->recvlen, 0 );
	}while( false );
	return ( (void *) NULL );
}

void* on_recv_udp(void* args)
{
	struct GDT_RECV_INFO *rinfo;
	GDT_SERVER_CONNECTION_INFO * tinfo;
	char *pbuf;
	rinfo = (struct GDT_RECV_INFO *) args;
	tinfo = (GDT_SERVER_CONNECTION_INFO *)rinfo->tinfo;
	pbuf = (char*)gdt_upointer(tinfo->option->memory_pool, rinfo->recvbuf_munit);
	pbuf[rinfo->recvlen] = '\0';
	printf( "@@@ recv udp message : %s, host=%s, serv=%s\n", pbuf,tinfo->hbuf, tinfo->sbuf );
	return ((void *)NULL);
}

void* on_close( void* args )
{
	GDT_SERVER_CONNECTION_INFO *tinfo = (GDT_SERVER_CONNECTION_INFO *)args;
	printf( "@@@ close connection : host=%s, serv=%s\n",tinfo->hbuf, tinfo->sbuf );
	return ( (void *) NULL );
}

int main( int argc, char *argv[], char *envp[] )
{
	int exe_code = EX_OK;
	char* phostname;
	char hostname[256];
	char portnum[32];
	int inetflag = 0;
	int result;
	GDT_MEMORY_POOL* memory_pool = NULL;
	do{
		if( gdt_initialize_memory( &memory_pool, SIZE_MBYTE * 8, SIZE_MBYTE * 8, MEMORY_ALIGNMENT_SIZE_BIT_64, FIX_MUNIT_SIZE, 1	, SIZE_KBYTE * 16 ) <= 0 ){
			printf( "gdt_initialize_memory error\n" );
			break;
		}

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
		GDT_SOCKET_OPTION option_udp;
		gdt_initialize_socket_option( &option, phostname, portnum, SOKET_TYPE_SERVER_TCP, SOCKET_MODE_NONBLOCKING, PROTOCOL_SIMPLE, 32, memory_pool, NULL, on_connect, on_sent, on_recv, on_close );
		option.inetflag			= inetflag;
		gdt_initialize_socket_option(&option_udp, phostname, portnum, SOKET_TYPE_SERVER_UDP, SOCKET_MODE_NONBLOCKING, PROTOCOL_PLAIN, 1, memory_pool, NULL, NULL, NULL, on_recv_udp, NULL);
		gdt_socket(&option);
		gdt_socket(&option_udp);
		while (1) { 
			gdt_server_update(&option);
			gdt_server_update(&option_udp);
			usleep(50);
		}
		gdt_free_socket(&option);
		gdt_free_socket(&option_udp);
		gdt_free( memory_pool );
	}while( false );
	return exe_code;
}
