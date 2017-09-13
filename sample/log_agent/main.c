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

GDT_MEMORY_POOL* main_memory_pool = NULL;

// callback
int on_connect(GDT_SERVER_CONNECTION_INFO* connection);
int32_t on_payload_recv(uint32_t payload_type, uint8_t* payload, size_t payload_len, GDT_RECV_INFO *gdt_recv_info);
int on_close(GDT_SERVER_CONNECTION_INFO* connection);

int on_connect(GDT_SERVER_CONNECTION_INFO* connection)
{
	return 0;
}

int32_t on_payload_recv(uint32_t payload_type, uint8_t* payload, size_t payload_len, GDT_RECV_INFO *gdt_recv_info)
{
	gdt_print_payload(payload_type,payload,payload_len,128);
	switch( payload_type )
	{
		case 0x01:
			break;
		default:
			// invalid packet
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
	int exe_code = EX_OK;
	gnt_set_argv( &argc, &argv, &envp );
	gdt_set_defaultsignal();
	//gdt_daemonize( 0 , 0 );
	gdt_srand_32();
	do{
		if( -1 == gdt_initialize_memory_f64(&main_memory_pool,SIZE_MBYTE*1)){
			printf( "gdt_initialize_memory_f64 error\n" );
			break;
		}
		char* hostname = NULL;
		char* portnum = "37012";
		GDT_MEMORY_POOL* memory_pool = NULL;
		GDT_SOCKET_OPTION *option = NULL;
		size_t maxconnection = 1;
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
		set_message_buffer(option,SIZE_KBYTE*32);
		set_on_connect_event( option, on_connect);
		set_on_payload_recv_event(option, on_payload_recv);
		set_on_close_event( option, on_close );
		gdt_socket(option);
		int32_t sync_timer = time(NULL);
		int32_t current_time = 0;
		int32_t base_path_munit = gdt_create_munit(main_memory_pool,MAXPATHLEN,MEMORY_TYPE_DEFAULT);
		int32_t current_path_munit = gdt_create_munit(main_memory_pool,MAXPATHLEN,MEMORY_TYPE_DEFAULT);
		int32_t next_path_munit = gdt_create_munit(main_memory_pool,MAXPATHLEN,MEMORY_TYPE_DEFAULT);

		if( argc <= 1 ){
			printf("argument : log_path\n");
			break;
		}
		char* base_path = (char*)GDT_POINTER(main_memory_pool,base_path_munit);
		gdt_strcopy(base_path,argv[1],gdt_usize(main_memory_pool,base_path_munit));
		size_t base_path_len = gdt_strlen(base_path);
		if( base_path_len == 0 ){
			printf("invalid log_path");
			break;
		}
		if( base_path[base_path_len-1] != '/' ){
			base_path[base_path_len] = '/';
			base_path[base_path_len+1] = '\0';
		}
		base_path_len = gdt_strlen(base_path);
		time_t nowtime = time(NULL);
		struct tm t;
		char date[20];
#ifdef __WINDOWS__
		localtime_s(&t, &nowtime);
#else
		localtime_r(&nowtime, &t);
#endif
		strftime( date, sizeof(date), "%Y%m%d", &t );
		size_t path_len = 0;
		char* current_path = (char*)GDT_POINTER(main_memory_pool,current_path_munit);
		path_len = gdt_strlink( current_path, 0, base_path, gdt_strlen(base_path), gdt_usize(main_memory_pool,current_path_munit) );
		path_len = gdt_strlink( current_path, path_len, "error_", 6, gdt_usize(main_memory_pool,current_path_munit) );
		path_len = gdt_strlink( current_path, path_len, date, gdt_strlen(date), gdt_usize(main_memory_pool,current_path_munit) );
		path_len = gdt_strlink( current_path, path_len, ".log", 4, gdt_usize(main_memory_pool,current_path_munit) );
		GDT_FILE_INFO finfo;
		finfo.size=0;
		size_t old_size = 0;
		while (1) {
			gdt_server_update(option);
			usleep(100000); // wait 1ms
			current_time = time(NULL);
			if( current_time - sync_timer > 1 ){
				sync_timer = current_time;
			}
			gdt_fget_info(current_path,&finfo);
			// printf("file size : %d\n",(int)finfo.size);
			if( old_size < finfo.size ){
				//printf("update!!\n");
				char buf[64];
				size_t read_size = gdt_fread_range(current_path,buf,old_size,sizeof(buf));
				//printf("read(%d->%d) : %s\n",(int)old_size,(int)(old_size+read_size),buf);
				//printf("%s",buf);
				gdt_send_broadcast(option,buf,gdt_strlen(buf),0x01);
				old_size += read_size;
			}
		}
		gdt_free_socket(option);
		gdt_free(option->memory_pool);
		gdt_free(main_memory_pool);
	}while( false );
	return exe_code;
}
