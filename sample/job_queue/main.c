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

#include "gdt_system.h"
#include "gdt_socket.h"
#include "gdt_random.h"
#include "gdt_queue.h"

GDT_MEMORY_POOL* main_memory_pool = NULL;
int32_t job_queue_munit = -1;
int32_t job_status = 0;

// callback
int on_connect(GDT_SERVER_CONNECTION_INFO* connection);
int32_t on_payload_recv(uint32_t payload_type, uint8_t* payload, size_t payload_len, GDT_RECV_INFO *gdt_recv_info);
int32_t on_payload_recv_client(uint32_t payload_type, uint8_t* payload, size_t payload_len, GDT_RECV_INFO *gdt_recv_info);
int on_close(GDT_SERVER_CONNECTION_INFO* connection);

int on_connect(GDT_SERVER_CONNECTION_INFO* connection)
{
	return 0;
}

int32_t on_payload_recv(uint32_t payload_type, uint8_t* payload, size_t payload_len, GDT_RECV_INFO *gdt_recv_info)
{
	switch( payload_type )
	{
		case 0x01: // enqueue
			if( -1 == gdt_enqueue( main_memory_pool, job_queue_munit, (const char*)payload, payload_len ) ){
				gdt_send_message(0x01,"1",1,gdt_recv_info);
			}
			else{
				printf( "enqueue success.\n" );
				gdt_send_message(0x01,"0",1,gdt_recv_info);
			}
			break;
		default:
			break;
	}
	return 0;
}

int32_t on_payload_recv_client(uint32_t payload_type, uint8_t* payload, size_t payload_len, GDT_RECV_INFO *gdt_recv_info)
{
	switch( payload_type )
	{
		case 0x01:
			job_status = 0;
			break;
		default:
			break;
	}
	return 0;
}

int on_close(GDT_SERVER_CONNECTION_INFO* connection)
{
	return 0;
}

int main( int argc, char *argv[], char *envp[] )
{
	int exe_code = 0;
	gnt_set_argv( &argc, &argv, &envp );
	gdt_set_defaultsignal();
	//gdt_daemonize( 0 , 0 );
	gdt_srand_32();
	do{
		if( -1 == gdt_initialize_memory_f64(&main_memory_pool,SIZE_MBYTE*8)){
			printf( "gdt_initialize_memory_f64 error\n" );
			break;
		}
		gdt_create_message_queue( main_memory_pool, &job_queue_munit, 10000, SIZE_BYTE*128 );

		// set queue server
		char* hostname = NULL;
		char* portnum = "39001";
		GDT_MEMORY_POOL* memory_pool = NULL;
		GDT_SOCKET_OPTION *option = NULL;
		size_t maxconnection = 4;
		if (gdt_initialize_memory_f64(&memory_pool, SIZE_MBYTE*1) <= 0) {
			printf("gdt_initialize_memory error\n");
			break;
		}
		int32_t option_munit = gdt_create_munit( memory_pool, sizeof( GDT_SOCKET_OPTION ), MEMORY_TYPE_DEFAULT );
		if( option_munit == -1 ){
			break;
		}
		option = (GDT_SOCKET_OPTION*)GDT_POINTER(memory_pool,option_munit);
		if (0 != gdt_initialize_socket_option(option, hostname, portnum, SOKET_TYPE_SERVER_TCP, SOCKET_MODE_NONBLOCKING, PROTOCOL_SIMPLE, maxconnection, memory_pool, NULL)) {
			break;
		}
		set_message_buffer(option,SIZE_KBYTE*16);
		set_on_connect_event( option, on_connect);
		set_on_payload_recv_event(option, on_payload_recv);
		set_on_close_event( option, on_close );
		gdt_socket(option);

		// set worker connection
		GDT_SOCKET_OPTION* tcp_client = gdt_create_tcp_client("localhost", "38001");
		set_on_connect_event( tcp_client, on_connect );
		set_on_payload_recv_event(tcp_client, on_payload_recv_client);
		set_on_close_event( tcp_client, on_close );
		gdt_socket(tcp_client);

		while (1) {
			gdt_server_update(option);
			gdt_client_update(tcp_client);
			usleep(1000);
			if( job_status == 0 && gdt_get_queue_length(main_memory_pool,job_queue_munit) > 0 ){
				int32_t message_munit = gdt_dequeue(main_memory_pool,job_queue_munit);
				char* job = (char*)GDT_POINTER(main_memory_pool,message_munit);
				printf( "dequeue : %s\n", job );
				gdt_client_send_message(0x01, job, gdt_strlen(job), tcp_client);
				job_status = 1;
			}
		}
		gdt_free_socket(tcp_client);
		gdt_free(tcp_client->memory_pool);
		gdt_free_socket(option);
		gdt_free(option->memory_pool);
		gdt_free(main_memory_pool);
	}while( false );
	return exe_code;
}
