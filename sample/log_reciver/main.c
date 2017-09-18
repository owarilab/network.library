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
#include "gdt_json.h"
#include "gdt_memory_allocator.h"

GDT_MEMORY_POOL* main_memory_pool = NULL;
int32_t g_reciver_munit = -1;
int32_t g_reciver_length = 0;

uint32_t g_recv_line_count = 0;
uint32_t g_recv_line_count_total = 0;

typedef struct GDT_RECIVER
{
	char host[256];
	char port[32];
	char target[256];
	GDT_SOCKET_OPTION* tcp_client;
} GDT_RECIVER;

int on_connect(GDT_SERVER_CONNECTION_INFO* connection);
int32_t on_payload_recv(uint32_t payload_type, uint8_t* payload, size_t payload_len, GDT_RECV_INFO *gdt_recv_info);
int on_close(GDT_SERVER_CONNECTION_INFO* connection);

int main( int argc, char *argv[], char *envp[] )
{
	int exe_code = EX_OK;
	int result;
	char config_json_file_name[256];
	gnt_set_argv( &argc, &argv, &envp );
	do{
		if( -1 == gdt_initialize_memory_f64(&main_memory_pool,SIZE_MBYTE*8)){
			printf( "gdt_initialize_memory_f64 error\n" );
			break;
		}
		memset( config_json_file_name, 0, sizeof( config_json_file_name ) );
		int optioncount = 0;
		while( ( result = getopt( argc, argv, "c:" ) ) != -1 )
		{
			switch(result)
			{
			case 'c':
				fprintf( stdout,"config : %s\n", optarg );
				snprintf( config_json_file_name, sizeof( config_json_file_name ) -1, "%s", optarg );
				break;
			case ':':
				fprintf( stdout,"%c needs value\n", result );
				break;
			case '?':
				fprintf(stdout,"unknown\n");
				break;
			}
			optioncount++;
		}
		if( strcmp(config_json_file_name,"") )
		{
			GDT_FILE_INFO info;
			if( 0 == gdt_fget_info( config_json_file_name, &info ) )
			{
				int32_t json_string_munit = gdt_create_munit( main_memory_pool, sizeof( char )*info.size+1, MEMORY_TYPE_DEFAULT );
				if( 0 != gdt_fread( config_json_file_name, (char*)GDT_POINTER(main_memory_pool,json_string_munit), GDT_PUNIT_USIZE(main_memory_pool,json_string_munit) ) ){
					int32_t root_munit = gdt_json_decode( main_memory_pool, (char*)GDT_POINTER(main_memory_pool,json_string_munit) );
					GDT_NODE* rootnode = (GDT_NODE*)GDT_POINTER(main_memory_pool,root_munit);
					GDT_NODE* workelemlist = ( GDT_NODE* )GDT_POINTER( main_memory_pool, rootnode->element_munit );
					if( workelemlist[0].id == ELEMENT_ARRAY )
					{
						int i;
						GDT_ARRAY* parray;
						GDT_ARRAY_ELEMENT* elm;
						parray = (GDT_ARRAY*)GDT_POINTER( main_memory_pool, workelemlist[0].element_munit );
						elm = (GDT_ARRAY_ELEMENT*)GDT_POINTER( main_memory_pool, parray->munit );
						g_reciver_munit = gdt_create_munit(main_memory_pool,sizeof(GDT_RECIVER)*parray->len,MEMORY_TYPE_DEFAULT);
						GDT_RECIVER* recivers = (GDT_RECIVER*)GDT_POINTER(main_memory_pool,g_reciver_munit);
						g_reciver_length = parray->len;
						for( i = 0; i < parray->len; i++ )
						{
							if( (elm+i)->id == ELEMENT_HASH ){
								int32_t host_munit = gdt_get_hash(main_memory_pool,(elm+i)->munit,"host");
								int32_t port_munit = gdt_get_hash(main_memory_pool,(elm+i)->munit,"port");
								int32_t target_munit = gdt_get_hash(main_memory_pool,(elm+i)->munit,"target");
								if( host_munit != -1 && port_munit != -1 && target_munit != -1 ){
									snprintf( (recivers+i)->host, sizeof( (recivers+i)->host ) -1, "%s", (char*)GDT_POINTER(main_memory_pool,host_munit) );
									snprintf( (recivers+i)->port, sizeof( (recivers+i)->port ) -1, "%s", (char*)GDT_POINTER(main_memory_pool,port_munit) );
									snprintf( (recivers+i)->target, sizeof( (recivers+i)->target ) -1, "%s", (char*)GDT_POINTER(main_memory_pool,target_munit) );
									(recivers+i)->tcp_client = gdt_create_tcp_client((recivers+i)->host, (recivers+i)->port);
									set_message_buffer((recivers+i)->tcp_client,SIZE_MBYTE*4);
									set_on_connect_event( (recivers+i)->tcp_client, on_connect );
									set_on_payload_recv_event((recivers+i)->tcp_client, on_payload_recv);
									set_on_close_event( (recivers+i)->tcp_client, on_close );
									gdt_socket((recivers+i)->tcp_client);
									gdt_set_client_id((recivers+i)->tcp_client,i);
									gdt_client_send_message(0x01, (recivers+i)->target, strlen((recivers+i)->target), (recivers+i)->tcp_client);
								}
							}
						}
					}
				}
			}
		}
		if( g_reciver_munit == -1 )
		{
			printf("empty reciver\n");
			break;
		}
		GDT_RECIVER* recivers = (GDT_RECIVER*)GDT_POINTER(main_memory_pool,g_reciver_munit);
		int32_t sync_timer = time(NULL);
		int32_t current_time = 0;
		int sleeptime = 20000;
		int i;
		while (1) {
			for( i = 0; i < g_reciver_length; i++ ){
				gdt_client_update((recivers+i)->tcp_client);
				usleep(sleeptime);
			}
			current_time = time(NULL);
			if( g_recv_line_count > 0 ){
				sleeptime = 10000;
			}
			if( current_time - sync_timer > 3 ){
				if( g_recv_line_count == 0 ){
					sleeptime = 100000;
				}
				sync_timer = current_time;
				g_recv_line_count_total += g_recv_line_count;
				g_recv_line_count = 0;
			}
		}
		for( i = 0; i < g_reciver_length; i++ ){
			gdt_free_socket((recivers+i)->tcp_client);
			gdt_free((recivers+i)->tcp_client->memory_pool);
		}
	}while( false );
	return exe_code;
}

int on_connect(GDT_SERVER_CONNECTION_INFO* connection)
{
	return 0;
}

int32_t on_payload_recv(uint32_t payload_type, uint8_t* payload, size_t payload_len, GDT_RECV_INFO *gdt_recv_info)
{
	char *pbuf = (char*)payload;
	pbuf[payload_len] = '\0';
	//gdt_print_payload(payload_type,payload,payload_len,128);
	int i = 0;
	for( i = 0; i < payload_len; i++ ){
		if(pbuf[i] == '\n'){
			g_recv_line_count++;
		}
	}
	switch( payload_type )
	{
		case 0x01:
			//printf("%s",pbuf);
			//gdt_fwrite_bin_a("./out",pbuf,gdt_strlen(pbuf));
			printf("recv line(%d) : %d\n", g_recv_line_count, g_recv_line_count_total+g_recv_line_count);
			gdt_client_send_message(0x02, "1", 1, (GDT_SOCKET_OPTION*)gdt_recv_info->tinfo->gdt_socket_option);
			break;
		default:
			// invalid packet
			break;
	}
	return 0;
	//printf("recv(payload_type:%d,payload_len:%d) : %s\n", (int)payload_type, (int)payload_len, pbuf);
	return 0;
}

int on_close(GDT_SERVER_CONNECTION_INFO* connection)
{
	return 0;
}

