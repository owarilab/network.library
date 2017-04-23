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


#include "gdt_socket.h"
#include "gdt_memory_allocator.h"
#include "gdt_script.h"

const char *internal_server_error = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n500 Internal Server Error\r\n";
const char *request_uri_too_long_error = "HTTP/1.1 414 Request-URI Too Long\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n414 Request-URI Too Long\r\n";
int32_t tiny_mpool_munit = -1;
int32_t tiny_buffer_mpool_munit = -1;
int32_t script_munit = -1;

void* on_connect( void* args );
void* on_sent( void* args );
void* on_recv( void* args );
void* on_close( void* args );

void* on_connect( void* args )
{
	//GDT_SERVER_CONNECTION_INFO *tinfo = (GDT_SERVER_CONNECTION_INFO *)args;
	//printf( "@@@ new connection : host=%s, serv=%s\n",tinfo->hbuf, tinfo->sbuf );
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
		//printf( "@@@ recv message : host=%s, serv=%s\n",tinfo->hbuf, tinfo->sbuf );
		GDT_MEMORY_POOL * tiny_mp = ( GDT_MEMORY_POOL* )GDT_POINTER( tinfo->option->memory_pool, tiny_mpool_munit );
		gdt_memory_clean( tiny_mp );
		int32_t htmlhash = -1;
		do{
			if( tiny_buffer_mpool_munit == -1 )
			{
				if( 0 >= ( script_munit = gdt_init_http_script( tiny_mp, "./scripts/request.gs",  "./scripts/ini.json" ) ) ){
					printf("error 3\n");
					break;
				}
				tiny_buffer_mpool_munit = gdt_create_clone_mini_memory( tinfo->option->memory_pool, tiny_mp );
			}
			else{
				GDT_MEMORY_POOL * buf_tiny_mp = ( GDT_MEMORY_POOL* )GDT_POINTER( tinfo->option->memory_pool, tiny_buffer_mpool_munit );
				if( GDT_SYSTEM_OK != gdt_copy_mini_memory( tiny_mp, buf_tiny_mp ) ){
					printf("error 1\n");
					break;
				}
			}
			int32_t request_header_munit = -1;
			int32_t get_parameter_munit = -1;
			int32_t post_parameter_munit = -1;
			if( 0 >= ( request_header_munit = gdt_create_hash( tiny_mp, 16 ) ) ){
				break;
			}
			int i;
			const char http_header_strings[][3][256] = HTTP_HEADER_STRINGS;
			for( i = 0; i < sizeof(http_header_strings) / sizeof(http_header_strings[0]); i++ )
			{
				gdt_add_hash_value( 
					tiny_mp
					, request_header_munit
					, (char*)(http_header_strings[i][1])
					, (char*)GDT_POINTER(tinfo->option->memory_pool,gdt_get_hash( tinfo->option->memory_pool, tinfo->sockparam.http_header_munit, (char*)(http_header_strings[i][1]) ))
					,ELEMENT_LITERAL_STR
				);
			}
			if( strcmp( (char*)GDT_POINTER(tinfo->option->memory_pool,gdt_get_hash( tinfo->option->memory_pool, tinfo->sockparam.http_header_munit, "GET_PARAMS" )), "" ) != 0 )
			{
				if( 0 >= ( get_parameter_munit = gdt_create_hash( tiny_mp, 32 ) ) ){
					break;
				}
				char* pparam = (char*)GDT_POINTER(tinfo->option->memory_pool,gdt_get_hash( tinfo->option->memory_pool, tinfo->sockparam.http_header_munit, "GET_PARAMS" ));
				char param_name[2048];
				char param_value[2048];
				char param_urldecode[2048];
				for(;*pparam != '\0';)
				{
					pparam = gdt_readline( param_name, sizeof(param_name), pparam, '=' );
					pparam = gdt_readline( param_value, sizeof(param_value), pparam, '&' );
					gdt_urldecode( param_urldecode, sizeof( param_urldecode ), param_value );
					gdt_add_hash_value( tiny_mp, get_parameter_munit, param_name, param_urldecode,ELEMENT_LITERAL_STR );
				}
			}
			if( 
				!strcmp( "POST", (char*)GDT_POINTER(tinfo->option->memory_pool,gdt_get_hash( tinfo->option->memory_pool, tinfo->sockparam.http_header_munit, "HTTP_METHOD" )) )
				|| !strcmp( "PUT", (char*)GDT_POINTER(tinfo->option->memory_pool,gdt_get_hash( tinfo->option->memory_pool, tinfo->sockparam.http_header_munit, "HTTP_METHOD" )) )
			)
			{
				char* pparam = pbuf;
				char param_name[2048];
				char param_value[2048];
				char param_urldecode[2048];
				if( 0 >= ( post_parameter_munit = gdt_create_hash( tiny_mp, 32 ) ) ){
					break;
				}
				for(;*pparam != '\0';)
				{
					pparam = gdt_readline( param_name, sizeof(param_name), pparam, '=' );
					pparam = gdt_readline( param_value, sizeof(param_value), pparam, '&' );
					gdt_urldecode( param_urldecode, sizeof( param_urldecode ), param_value );
					gdt_add_hash_value( tiny_mp, post_parameter_munit, param_name, param_urldecode,ELEMENT_LITERAL_STR );
				}
			}
			if( GDT_SYSTEM_OK != gdt_add_http_request( tiny_mp, script_munit, pbuf, request_header_munit, get_parameter_munit, post_parameter_munit ) )
			{
				printf("error 4\n");
				break;
			}
			gdt_exec( tiny_mp, &script_munit );
			GDT_SCRIPT *pscript = NULL;
			pscript = (GDT_SCRIPT *)GDT_POINTER( tiny_mp, script_munit );
			htmlhash = gdt_get_hash( tiny_mp, pscript->v_hash_munit, "html" );
		}while( false );
		if( htmlhash > 0 ){
			gdt_send( tinfo->option, &tinfo->sockparam, (char*)GDT_POINTER(tiny_mp,htmlhash), gdt_usize( tiny_mp, htmlhash ), 0 );
		}
		else{
			gdt_send( tinfo->option, &tinfo->sockparam, (char*)internal_server_error, gdt_strlen( (char*)internal_server_error ), 0 );
		}
		gdt_disconnect( &tinfo->sockparam );
	}while( false );
	return ( (void *) NULL );
}

void* on_close( void* args )
{
	//GDT_SERVER_CONNECTION_INFO *tinfo = (GDT_SERVER_CONNECTION_INFO *)args;
	//printf( "@@@ close connection : host=%s, serv=%s\n",tinfo->hbuf, tinfo->sbuf );
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
		if( gdt_initialize_memory( &memory_pool, SIZE_MBYTE * 16, SIZE_MBYTE * 16, MEMORY_ALIGNMENT_SIZE_BIT_64, FIX_MUNIT_SIZE, 1	, SIZE_KBYTE * 16 ) <= 0 ){
			printf( "gdt_initialize_memory error\n" );
			break;
		}

		if( 0 >= ( tiny_mpool_munit = gdt_create_mini_memory( memory_pool, SIZE_KBYTE * 512 ) ) ){
			printf( "gdt_create_mini_memory error\n" );
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
		gdt_initialize_socket_option( &option, phostname, portnum, SOKET_TYPE_SERVER_TCP, SOCKET_MODE_SELECT, PROTOCOL_PLAIN, 128, memory_pool, NULL, on_connect, on_sent, on_recv, on_close );
		option.inetflag			= inetflag;
		gdt_socket( &option );
		// for(;;){ (void) sleep(10); }
		gdt_free_socket( &option );
		gdt_free( memory_pool );
	}while( false );
	return exe_code;
}
