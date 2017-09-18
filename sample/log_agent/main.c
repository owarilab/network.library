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
int32_t g_reciver_munit;

#define READ_SIZE SIZE_KBYTE*32
#define AGENT_BUF_SIZE SIZE_KBYTE*514
#define RECIVER_STATUS_DEFAULT 0
#define RECIVER_STATUS_RECIVE 1
#define RECIVER_STATUS_WAIT 2

#define GDT_LOG_AGENT_TARGET_STATUS_DEFAULT 0
#define GDT_LOG_AGENT_TARGET_STATUS_RUNNING 1

#define MAX_LOG_AGENT_TARGETS 8

#define LOG_PATH_TYPE_ERROR 0x01
#define LOG_PATH_TYPE_INFO  0x02
#define LOG_PATH_TYPE_DEBUG 0x04

typedef struct GDT_LOG_AGENT_TARGET
{
	GDT_MEMORY_POOL* memory_pool;
	int32_t path_munit;
	int32_t agent_buf_munit;
	int32_t agent_buf_pos;
	size_t current_pos;
	int64_t filetime;
	int status;
} GDT_LOG_AGENT_TARGET;

typedef struct GDT_LOG_RECIVER
{
	GDT_MEMORY_POOL* memory_pool;
	int32_t log_targets_munit;
	int log_targets_length;
	int status;
} GDT_LOG_RECIVER;

int32_t make_log_reciver(GDT_MEMORY_POOL* memory_pool,size_t length);
int32_t init_log_reciver(GDT_LOG_RECIVER* reciver);
int32_t init_log_agent_target(GDT_LOG_AGENT_TARGET* target);
int32_t make_date_log_path(GDT_MEMORY_POOL* memory_pool,int32_t dest_munit,char* base_path,int type);
int32_t make_log_path(GDT_MEMORY_POOL* memory_pool,int32_t dest_munit,char* path);


int32_t add_log_agent_target(uint32_t payload_type, uint8_t* payload, size_t payload_len, GDT_RECV_INFO *gdt_recv_info);
int32_t add_log_agent_target(uint32_t payload_type, uint8_t* payload, size_t payload_len, GDT_RECV_INFO *gdt_recv_info)
{
	GDT_LOG_RECIVER* recivers = (GDT_LOG_RECIVER*)GDT_POINTER(main_memory_pool,g_reciver_munit);
	GDT_LOG_RECIVER* reciver = (recivers+gdt_recv_info->tinfo->index);
	GDT_LOG_AGENT_TARGET* targets = (GDT_LOG_AGENT_TARGET*)GDT_POINTER(reciver->memory_pool,reciver->log_targets_munit);
	GDT_LOG_AGENT_TARGET* target = NULL;
	char* pbuf = (char*)payload;
	if( !strcmp(pbuf,"date_log_error") || !strcmp(pbuf,"date_log")){
		target = targets+reciver->log_targets_length++;
		target->status = GDT_LOG_AGENT_TARGET_STATUS_RUNNING;
		target->current_pos = 0;
		target->filetime = 0;
		make_date_log_path(target->memory_pool,target->path_munit,"./",LOG_PATH_TYPE_ERROR);
	}
	if( !strcmp(pbuf,"date_log_info") || !strcmp(pbuf,"date_log")){
		target = targets+reciver->log_targets_length++;
		target->status = GDT_LOG_AGENT_TARGET_STATUS_RUNNING;
		target->current_pos = 0;
		target->filetime = 0;
		make_date_log_path(target->memory_pool,target->path_munit,"./",LOG_PATH_TYPE_INFO);
	}
	if( !strcmp(pbuf,"date_log_debug") || !strcmp(pbuf,"date_log")){
		target = targets+reciver->log_targets_length++;
		target->status = GDT_LOG_AGENT_TARGET_STATUS_RUNNING;
		target->current_pos = 0;
		target->filetime = 0;
		make_date_log_path(target->memory_pool,target->path_munit,"./",LOG_PATH_TYPE_DEBUG);
	}
	if( !strcmp(pbuf,"apache_error_log") ){
		target = targets+reciver->log_targets_length++;
		target->status = GDT_LOG_AGENT_TARGET_STATUS_RUNNING;
		target->current_pos = 0;
		target->filetime = 0;
		make_log_path(target->memory_pool,target->path_munit,"/var/log/httpd/error.log");
	}
	if( !strcmp(pbuf,"apache_access_log") ){
		target = targets+reciver->log_targets_length++;
		target->status = GDT_LOG_AGENT_TARGET_STATUS_RUNNING;
		target->current_pos = 0;
		target->filetime = 0;
		make_log_path(target->memory_pool,target->path_munit,"/var/log/httpd/access.log");
	}
	if( !strcmp(pbuf,"nginx_error_log") ){
		target = targets+reciver->log_targets_length++;
		target->status = GDT_LOG_AGENT_TARGET_STATUS_RUNNING;
		target->current_pos = 0;
		target->filetime = 0;
		make_log_path(target->memory_pool,target->path_munit,"/var/log/nginx/error.log");
	}
	if( !strcmp(pbuf,"nginx_access_log") ){
		target = targets+reciver->log_targets_length++;
		target->status = GDT_LOG_AGENT_TARGET_STATUS_RUNNING;
		target->current_pos = 0;
		target->filetime = 0;
		make_log_path(target->memory_pool,target->path_munit,"/var/log/nginx/access.log");
	}
	reciver->status = RECIVER_STATUS_RECIVE;
	return GDT_SYSTEM_OK;
}

// callback
int on_connect(GDT_SERVER_CONNECTION_INFO* connection);
int32_t on_payload_recv(uint32_t payload_type, uint8_t* payload, size_t payload_len, GDT_RECV_INFO *gdt_recv_info);
int on_close(GDT_SERVER_CONNECTION_INFO* connection);

int32_t make_log_reciver(GDT_MEMORY_POOL* memory_pool,size_t length)
{
	int32_t reciver_munit = gdt_create_munit(memory_pool,sizeof(GDT_LOG_RECIVER)*length,MEMORY_TYPE_DEFAULT);
	GDT_LOG_RECIVER* recivers = (GDT_LOG_RECIVER*)GDT_POINTER(memory_pool,reciver_munit);
	GDT_LOG_AGENT_TARGET* targets = NULL;
	int i,j;
	for( i = 0; i < length; i++ ){
		(recivers+i)->memory_pool = memory_pool;
		(recivers+i)->status = RECIVER_STATUS_DEFAULT;
		(recivers+i)->log_targets_munit = gdt_create_munit(memory_pool,sizeof(GDT_LOG_AGENT_TARGET)*MAX_LOG_AGENT_TARGETS,MEMORY_TYPE_DEFAULT);
		(recivers+i)->log_targets_length = 0;
		targets = (GDT_LOG_AGENT_TARGET*)GDT_POINTER(memory_pool,(recivers+i)->log_targets_munit);
		for(j=0;j<MAX_LOG_AGENT_TARGETS;j++){
			(targets+j)->memory_pool = memory_pool;
			(targets+j)->path_munit = gdt_create_munit(main_memory_pool,MAXPATHLEN,MEMORY_TYPE_DEFAULT);
			(targets+j)->agent_buf_munit = gdt_create_munit(main_memory_pool,AGENT_BUF_SIZE,MEMORY_TYPE_DEFAULT);
			(targets+j)->agent_buf_pos = 0;
			(targets+j)->current_pos = 0;
			(targets+j)->filetime = 0;
			(targets+j)->status = GDT_LOG_AGENT_TARGET_STATUS_DEFAULT;
		}
	}
	return reciver_munit;
}

int32_t init_log_reciver(GDT_LOG_RECIVER* reciver)
{
	GDT_LOG_AGENT_TARGET* targets = NULL;
	int j;
	reciver->status = RECIVER_STATUS_DEFAULT;
	reciver->log_targets_length = 0;
	targets = (GDT_LOG_AGENT_TARGET*)GDT_POINTER(reciver->memory_pool,reciver->log_targets_munit);
	for(j=0;j<MAX_LOG_AGENT_TARGETS;j++){
		init_log_agent_target((targets+j));
	}
	return GDT_SYSTEM_OK;
}

int32_t init_log_agent_target(GDT_LOG_AGENT_TARGET* target)
{
	char* path = (char*)GDT_POINTER(target->memory_pool,target->path_munit);
	memset(path,0,gdt_usize(target->memory_pool,target->path_munit));
	char* buf = (char*)GDT_POINTER(target->memory_pool,target->agent_buf_munit);
	memset(buf,0,gdt_usize(target->memory_pool,target->agent_buf_munit));
	target->agent_buf_pos = 0;
	target->current_pos = 0;
	target->filetime = 0;
	target->status = GDT_LOG_AGENT_TARGET_STATUS_DEFAULT;
	return GDT_SYSTEM_OK;
}

int32_t make_date_log_path(GDT_MEMORY_POOL* memory_pool,int32_t dest_munit,char* base_path,int type)
{
	if( dest_munit == -1 ){
		return -1;
	}
	char* path = (char*)GDT_POINTER(memory_pool,dest_munit);
	size_t path_buffer_len = gdt_usize(memory_pool,dest_munit);
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
	path_len = gdt_strlink( path, 0, base_path, gdt_strlen(base_path), path_buffer_len );
	if( type == LOG_PATH_TYPE_ERROR ){
		path_len = gdt_strlink( path, path_len, "error", 5, path_buffer_len );
	}
	if( type == LOG_PATH_TYPE_INFO ){
		path_len = gdt_strlink( path, path_len, "info", 4, path_buffer_len );
	}
	if( type == LOG_PATH_TYPE_DEBUG ){
		path_len = gdt_strlink( path, path_len, "debug", 5, path_buffer_len );
	}
	path_len = gdt_strlink( path, path_len, "_", 1, path_buffer_len );
	path_len = gdt_strlink( path, path_len, date, gdt_strlen(date), path_buffer_len );
	path_len = gdt_strlink( path, path_len, ".log", 4, path_buffer_len );
	return dest_munit;
}

int32_t make_log_path(GDT_MEMORY_POOL* memory_pool,int32_t dest_munit,char* path)
{
	if( dest_munit == -1 ){
		return -1;
	}
	char* dest = (char*)GDT_POINTER(memory_pool,dest_munit);
	size_t path_buffer_len = gdt_usize(memory_pool,dest_munit);
	size_t path_len = 0;
	path_len = gdt_strlink( dest, 0, path, gdt_strlen(path), path_buffer_len );
	return dest_munit;
}

int on_connect(GDT_SERVER_CONNECTION_INFO* connection)
{
	return 0;
}

int32_t on_payload_recv(uint32_t payload_type, uint8_t* payload, size_t payload_len, GDT_RECV_INFO *gdt_recv_info)
{
	//gdt_print_payload(payload_type,payload,payload_len,128);
	GDT_LOG_RECIVER* recivers = NULL;
	GDT_LOG_RECIVER* reciver = NULL;
	switch( payload_type )
	{
		case 0x01:
			add_log_agent_target(payload_type, payload, payload_len, gdt_recv_info);
			break;
		case 0x02:
			recivers = (GDT_LOG_RECIVER*)GDT_POINTER(main_memory_pool,g_reciver_munit);
			reciver = (recivers+gdt_recv_info->tinfo->index);
			reciver->status = RECIVER_STATUS_RECIVE;
			break;
		default:
			// invalid packet
			break;
	}
	return 0;
}

int on_close(GDT_SERVER_CONNECTION_INFO* connection)
{
	GDT_LOG_RECIVER* recivers = (GDT_LOG_RECIVER*)GDT_POINTER(main_memory_pool,g_reciver_munit);
	GDT_LOG_RECIVER* reciver = (recivers+connection->index);
	init_log_reciver(reciver);
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
		if( -1 == gdt_initialize_memory_f64(&main_memory_pool,SIZE_MBYTE*32)){
			printf( "gdt_initialize_memory_f64 error\n" );
			break;
		}
		char* hostname = NULL;
		char* portnum = "37012";
		GDT_MEMORY_POOL* memory_pool = NULL;
		GDT_SOCKET_OPTION *option = NULL;
		size_t maxconnection = 4;
		if (gdt_initialize_memory_f64(&memory_pool, SIZE_MBYTE*8) <= 0) {
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
		set_message_buffer(option,SIZE_KBYTE*512);
		set_on_connect_event( option, on_connect);
		set_on_payload_recv_event(option, on_payload_recv);
		set_on_close_event( option, on_close );
		gdt_socket(option);

		g_reciver_munit = make_log_reciver(main_memory_pool,maxconnection);
		if( g_reciver_munit == -1 ){
			break;
		}
		GDT_LOG_RECIVER* recivers = (GDT_LOG_RECIVER*)GDT_POINTER(main_memory_pool,g_reciver_munit);
		GDT_LOG_AGENT_TARGET* targets = NULL;
		int i,j;
		char* current_path = NULL;
		char* pbuf = NULL;
		GDT_FILE_INFO finfo;
		int sleeptime = 20000;
		int execcount = 0;
		int execcount_total = 0;
		uint32_t sleepcnt = 0;

		int32_t sync_timer = time(NULL);
		int32_t current_time = 0;
		while (1) {
			gdt_server_update(option);
			current_time = time(NULL);
			if( current_time - sync_timer > 3 ){
				sync_timer = current_time;
				if( execcount_total+execcount == 0 ){
					sleeptime = 300000;
					sleepcnt++;
				}
				execcount_total=0;
			}
			execcount_total+=execcount;
			if(execcount>0){
				sleeptime = 20000;
				sleepcnt = 0;
			}
			else if( execcount_total > 0 ){
				sleeptime = 100000;
			}
			usleep(sleeptime);
			execcount = 0;
			for( i = 0; i < maxconnection; i++ ){
				if( (recivers+i)->status == RECIVER_STATUS_RECIVE )
				{
					targets = (GDT_LOG_AGENT_TARGET*)GDT_POINTER((recivers+i)->memory_pool,(recivers+i)->log_targets_munit);
					for(j=0;j<MAX_LOG_AGENT_TARGETS;j++){
						if( (targets+j)->status == GDT_LOG_AGENT_TARGET_STATUS_RUNNING ){
							current_path = (char*)GDT_POINTER((targets+j)->memory_pool,(targets+j)->path_munit);
							finfo.size=0;
							gdt_fget_info(current_path,&finfo);
							if( (targets+j)->current_pos+(targets+j)->agent_buf_pos < finfo.size 
								|| ( finfo.size > 0 && finfo.update_usec > (targets+j)->filetime ) 
							){
								if( (targets+j)->current_pos+(targets+j)->agent_buf_pos >= finfo.size )
								{
									if( finfo.update_usec > (targets+j)->filetime && (targets+j)->filetime > 0 ){
										(targets+j)->current_pos = 0;
										(targets+j)->agent_buf_pos = 0;
										printf("file time update %lld (%s)\n",finfo.update_usec,current_path);
									}
								}
								(targets+j)->filetime = finfo.update_usec;
								pbuf = (char*)GDT_POINTER((targets+j)->memory_pool,(targets+j)->agent_buf_munit);
								pbuf+=(targets+j)->agent_buf_pos;
								size_t read_size = gdt_fread_bin_range(current_path,pbuf,(targets+j)->current_pos+(targets+j)->agent_buf_pos,READ_SIZE);
								int32_t line_check = (int32_t)(read_size-1);
								while(pbuf[line_check] != '\n'){
									line_check--;
									if( line_check < 0 ){
										break;
									}
								}
								if( line_check >= 0 
									|| (targets+j)->agent_buf_pos >= AGENT_BUF_SIZE-READ_SIZE-1 
									|| (targets+j)->current_pos + (targets+j)->agent_buf_pos + read_size >= finfo.size
								)
								{
									if( (targets+j)->current_pos + (targets+j)->agent_buf_pos + read_size >= finfo.size )
									{
										(targets+j)->agent_buf_pos+=read_size;
										pbuf[read_size] = '\0';
									}
									else if( line_check >= 0 ){
										(targets+j)->agent_buf_pos+=line_check+1;
										pbuf[line_check+1] = '\0';
									}
									else{
										(targets+j)->agent_buf_pos+=read_size;
										pbuf[read_size] = '\0';
									}
									GDT_SERVER_CONNECTION_INFO *tmptinfo = gdt_offsetpointer( option->memory_pool, option->connection_munit, sizeof( GDT_SERVER_CONNECTION_INFO ), i );
									if( tmptinfo->sockparam.acc > 0 )
									{
										ssize_t ret = 0;
										pbuf = (char*)GDT_POINTER((targets+j)->memory_pool,(targets+j)->agent_buf_munit);
										if( -1 == ( ret = gdt_send( option, &tmptinfo->sockparam, pbuf, gdt_strlen(pbuf), 0x01 ) ) ){
											if( option->close_callback != NULL ){
												option->close_callback( tmptinfo );
											}
											gdt_free_sockparam( option, &tmptinfo->sockparam );
										}
										else{
											(recivers+i)->status = RECIVER_STATUS_WAIT;
										}
										//printf("send(%d) : %d/%d\n",(int)ret,(int)((targets+j)->current_pos+(targets+j)->agent_buf_pos),(int)finfo.size);
										if( (int)((targets+j)->current_pos+(targets+j)->agent_buf_pos) == (int)finfo.size ){
											printf("send(%d) : %d/%d\n",(int)sleeptime,(int)((targets+j)->current_pos+(targets+j)->agent_buf_pos),(int)finfo.size);
										}
									}
									(targets+j)->current_pos += (targets+j)->agent_buf_pos;
									(targets+j)->agent_buf_pos = 0;
								}
								else{
									(targets+j)->agent_buf_pos+=read_size;
									//printf("buffer(%d) : %d/%d\n",(int)read_size,(int)((targets+j)->current_pos+(targets+j)->agent_buf_pos),(int)finfo.size);
								}
								execcount++;
								usleep(sleeptime);
							}
						}
					}
				}
			}
		}
		gdt_free_socket(option);
		gdt_free(option->memory_pool);
		gdt_free(main_memory_pool);
	}while( false );
	return exe_code;
}
