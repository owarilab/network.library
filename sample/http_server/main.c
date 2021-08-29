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

#include "qs_socket.h"
#include "qs_protocol.h"
#include "qs_variable.h"
#include "qs_random.h"
#include "qs_logger.h"

// get api sample
// curl -X POST -H "Content-Type: application/json" -d '{"k":"id_12345678", "v":"kvs_value1"}' http://localhost:8080/api/v1/set
// curl -X POST -d 'k=id_12345678&v=kvs_value1' http://localhost:8080/api/v1/set
// set api sample
// curl -X POST -H "Content-Type: application/json" -d '{"k":"id_12345678"}' http://localhost:8080/api/v1/get
// curl -X POST -d 'k=id_12345678' http://localhost:8080/api/v1/get

// set api sample
// curl -X POST -H "Content-Type: application/json" -d '[]' http://localhost:8080/api/v1/dump
// curl -X POST -d '' http://localhost:8080/api/v1/dump

// test data
/*
curl -X POST -H "Content-Type: application/json" -d '{"k":"id_1", "v":"kvs_value1"}' http://localhost:8080/api/v1/set
curl -X POST -H "Content-Type: application/json" -d '{"k":"id_2", "v":"kvs_value11"}' http://localhost:8080/api/v1/set
curl -X POST -H "Content-Type: application/json" -d '{"k":"id_3", "v":"kvs_value111"}' http://localhost:8080/api/v1/set
curl -X POST -H "Content-Type: application/json" -d '{"k":"id_4", "v":"kvs_value1111"}' http://localhost:8080/api/v1/set
curl -X POST -H "Content-Type: application/json" -d '{"k":"id_5", "v":"kvs_value11111"}' http://localhost:8080/api/v1/set
curl -X POST -H "Content-Type: application/json" -d '{"k":"id_6", "v":"kvs_value111111"}' http://localhost:8080/api/v1/set
curl -X POST -H "Content-Type: application/json" -d '{"k":"id_7", "v":"kvs_value1111111"}' http://localhost:8080/api/v1/set
curl -X POST -H "Content-Type: application/json" -d '{"k":"id_8", "v":"kvs_value11111111"}' http://localhost:8080/api/v1/set
curl -X POST -H "Content-Type: application/json" -d '{"k":"id_9", "v":"kvs_value111111111"}' http://localhost:8080/api/v1/set
*/

QS_MEMORY_POOL* kvs_storage_memory_pool = NULL;
int32_t memid_kvs_storage_id = -1;
int32_t memid_temporary_memory = -1;
int32_t memid_kvs_memory = -1;
int32_t memid_kvs_id = -1;
QS_FILE_INFO log_file_info;

int on_connect(QS_SERVER_CONNECTION_INFO* connection);
void* on_recv( void* args );
int on_close(QS_SERVER_CONNECTION_INFO* connection);

int main( int argc, char *argv[], char *envp[] )
{
	qs_finit(&log_file_info);
#ifndef __WINDOWS__
	qs_set_defaultsignal();
#else
	SetConsoleOutputCP(CP_UTF8);
#endif
	qs_srand_32();
	QS_SOCKET_OPTION* option = qs_create_tcp_server_plane(NULL, "8080");
	memid_temporary_memory = qs_create_mini_memory( option->memory_pool, SIZE_MBYTE * 8 );
	memid_kvs_memory = qs_create_mini_memory(option->memory_pool, SIZE_MBYTE * 1);

	QS_MEMORY_POOL* cache_memory = (QS_MEMORY_POOL*)QS_GET_POINTER(option->memory_pool,memid_kvs_memory);
	memid_kvs_id = qs_create_cache_B1MB(cache_memory);
	if(-1 == memid_kvs_id) {
		printf("create cache memory error\n");
		return -1;
	}

	memid_kvs_storage_id = qs_create_storage_cache_B1MB("./kvs_store_b1mb",&kvs_storage_memory_pool);
	if(-1 == memid_kvs_storage_id) {
		printf("create storage memory error\n");
		return -1;
	}

	set_on_connect_event(option,on_connect);
	set_on_packet_recv_event(option,on_recv);
	set_on_close_event(option,on_close);
	qs_socket(option);
	time_t current_time = time(NULL);
	time_t update_time = 0;
	while(1){
		current_time = time(NULL);
		if (current_time - update_time > 60) {
			if( QS_SYSTEM_ERROR == qs_log_rotate(&log_file_info, "./", "access", 0)){
				printf("qs_log_rotate error\n");
			}
			update_time = current_time;
		}
		qs_server_update(option);
		qs_sleep(100);
	}
	qs_free_socket(option);
	qs_free(option->memory_pool);
	return 0;
}

int on_connect(QS_SERVER_CONNECTION_INFO* connection)
{
	return 0;
}

void* on_recv( void* args )
{
	QS_RECV_INFO *rinfo = (QS_RECV_INFO *)args;
	QS_SERVER_CONNECTION_INFO * tinfo = (QS_SERVER_CONNECTION_INFO *)rinfo->tinfo;
	QS_SOCKET_OPTION* option = (QS_SOCKET_OPTION*)tinfo->qs_socket_option;
	QS_MEMORY_POOL * temporary_memory = ( QS_MEMORY_POOL* )QS_GET_POINTER( option->memory_pool, memid_temporary_memory );
	qs_memory_clean( temporary_memory );

	switch( qs_http_protocol_filter_with_websocket(rinfo) )
	{
		case -1:
		{
			QS_SOCKPARAM* psockparam = &tinfo->sockparam;
			qs_disconnect( psockparam );
			return ( (void *) NULL );
		}
		case 0:
		{
			return ( (void *) NULL );
		}
		case 1:
		{
			break;
		}
		case QS_HTTP_SOCK_PHASE_HANDSHAKE_WEBSOCKET:
		{
			return ( (void *) NULL );
		}
		case QS_HTTP_SOCK_PHASE_MSG_WEBSOCKET:
		{
			QS_SOCKPARAM* psockparam = &tinfo->sockparam;
			ssize_t _tmpmsglen = qs_parse_websocket_binary( option, psockparam, (uint8_t*)qs_upointer( option->memory_pool, rinfo->recvbuf_munit ), rinfo->recvlen, tinfo->recvmsg_munit );
			if( _tmpmsglen < 0 ){
				qs_disconnect( psockparam );
				return ( (void *) NULL );
			}
			if( psockparam->tmpmsglen == 0 ){
				char* msgpbuf = (char*)qs_upointer( option->memory_pool, tinfo->recvmsg_munit );
				rinfo->recvlen = _tmpmsglen;
				msgpbuf[rinfo->recvlen] = '\0';
				if(rinfo->recvlen==0){ // ping packet
					return ( (void *) NULL );
				}
				rinfo->recvbuf_munit = tinfo->recvmsg_munit;
				int32_t message_buffer_munit = qs_create_munit(temporary_memory,SIZE_KBYTE*8,MEMORY_TYPE_DEFAULT);
				void* buffer = QS_GET_POINTER(temporary_memory,message_buffer_munit);
				size_t buffer_size = qs_usize(temporary_memory,message_buffer_munit);
				int i;
				ssize_t sendlen = qs_make_websocket_msg(option,buffer,buffer_size,false,msgpbuf,qs_strlen(msgpbuf));
				ssize_t ret = 0;
				QS_SERVER_CONNECTION_INFO *tmptinfo;
				if( option->connection_munit == -1 ){
					return ( (void *) NULL );
				}
				for( i = 0; i < option->maxconnection; i++ ){
					tmptinfo = qs_offsetpointer( option->memory_pool, option->connection_munit, sizeof( QS_SERVER_CONNECTION_INFO ), i );
					if( tmptinfo->sockparam.acc != -1 && tmptinfo->sockparam.phase==QS_HTTP_SOCK_PHASE_MSG_WEBSOCKET){
						if( -1 == ( ret = qs_send_all( tmptinfo->sockparam.acc, buffer, sendlen, 0 ) ) ){
							if( option->close_callback != NULL ){
								option->close_callback( tmptinfo );
							}
							qs_free_sockparam( option, &tmptinfo->sockparam );
						}
					}
				}
			}
			return ( (void *) NULL );
		}
	}

	QS_HTTP_REQUEST_COMMON http_request;
	int32_t http_status_code = http_request_common(rinfo, &http_request, temporary_memory);

	// API
	if (http_status_code == 404) {
		if (!strcmp(http_request.method, "POST")) {
			// in memory kvs
			//QS_MEMORY_POOL* cache_memory = (QS_MEMORY_POOL*)QS_GET_POINTER(option->memory_pool,memid_kvs_memory);
			//QS_CACHE* cache = (QS_CACHE*)QS_GET_POINTER(cache_memory,memid_kvs_id);

			// storage kvs
			QS_CACHE* cache = (QS_CACHE*)QS_GET_POINTER(kvs_storage_memory_pool,memid_kvs_storage_id);

			int32_t memid_response = qs_create_hash(temporary_memory,32);
			qs_add_hash_string(temporary_memory, memid_response, "method", http_request.method);
			qs_add_hash_string(temporary_memory, memid_response, "request", http_request.request);
			qs_add_hash_string(temporary_memory, memid_response, "user_agent", http_request.user_agent);

			int32_t memid_response_data = qs_create_hash(temporary_memory, 32);

			if (!strcmp(http_request.request, "/api/v1/set")) {
				do {
					if (-1 == http_request.memid_post_parameter_hash) {
						break;
					}
					if (-1 == qs_get_hash(http_request.temporary_memory, http_request.memid_post_parameter_hash, "k")) {
						break;
					}
					if (-1 == qs_get_hash(http_request.temporary_memory, http_request.memid_post_parameter_hash, "v")) {
						break;
					}
					char* key = (char*)QS_GET_POINTER(http_request.temporary_memory, qs_get_hash(http_request.temporary_memory, http_request.memid_post_parameter_hash, "k"));
					char* value = (char*)QS_GET_POINTER(http_request.temporary_memory, qs_get_hash(http_request.temporary_memory, http_request.memid_post_parameter_hash, "v"));

					size_t key_size = strlen(key);
					if( key_size >= cache->max_key_size && key_size <= 0){
						printf("invalid key size : %d\n",(int)key_size);
					} else{
						qs_cache_string(cache,key,value,0);
					}

					qs_add_hash_string(temporary_memory, memid_response_data, "key", key);
					qs_add_hash_string(temporary_memory, memid_response_data, "value", value);

					//printf("/api/v1/set api %s, %s\n", key, value);
					qs_add_hash_hash(temporary_memory, memid_response, "data", memid_response_data);
					//response
					http_status_code = http_json_response_common(tinfo, option, temporary_memory, memid_response, SIZE_KBYTE * 4);
				} while (false);
			}
			if (!strcmp(http_request.request, "/api/v1/get")) {
				do {
					if (-1 == http_request.memid_post_parameter_hash) {
						break;
					}
					if (-1 == qs_get_hash(http_request.temporary_memory, http_request.memid_post_parameter_hash, "k")) {
						break;
					}
					char* key = (char*)QS_GET_POINTER(http_request.temporary_memory, qs_get_hash(http_request.temporary_memory, http_request.memid_post_parameter_hash, "k"));
					char* value = NULL;
					QS_CACHE_PAGE cache_page;
					qs_get_cache_page(cache,&cache_page);
					int32_t hash_id = qs_get_hash(cache_page.memory,cache_page.hash_id,key);
					//qs_hash_dump(cache_page.memory,cache_page.hash_id,0);
					if(-1 != hash_id){
						value = (char*)QS_GET_POINTER(cache_page.memory, hash_id);
					}
					qs_add_hash_string(temporary_memory, memid_response_data, "key", key);
					if(NULL!=value){
						qs_add_hash_string(temporary_memory, memid_response_data, "value", value);
					}
					//printf("/api/v1/get api %s, %s\n",key,value);
					qs_add_hash_hash(temporary_memory, memid_response, "data", memid_response_data);
					//response
					http_status_code = http_json_response_common(tinfo, option, temporary_memory, memid_response, SIZE_KBYTE * 4);
				} while (false);
			}
			if (!strcmp(http_request.request, "/api/v1/delete")) {
				do {
					if (-1 == http_request.memid_post_parameter_hash) {
						break;
					}
					if (-1 == qs_get_hash(http_request.temporary_memory, http_request.memid_post_parameter_hash, "k")) {
						break;
					}
					char* key = (char*)QS_GET_POINTER(http_request.temporary_memory, qs_get_hash(http_request.temporary_memory, http_request.memid_post_parameter_hash, "k"));
					QS_CACHE_PAGE cache_page;
					qs_get_cache_page(cache,&cache_page);
					qs_remove_hash(cache_page.memory,cache_page.hash_id,key);
					qs_add_hash_string(temporary_memory, memid_response_data, "key", key);
					//printf("/api/v1/delete api %s, %s\n",key,value);
					qs_add_hash_hash(temporary_memory, memid_response, "data", memid_response_data);
					//response
					http_status_code = http_json_response_common(tinfo, option, temporary_memory, memid_response, SIZE_KBYTE * 4);
				} while (false);
			}
			if (!strcmp(http_request.request, "/api/v1/dump")) {
				do {
					QS_CACHE_PAGE cache_page;
					qs_get_cache_page(cache, &cache_page);
					int32_t memid_temporary_json_memory = qs_create_mini_memory(temporary_memory, SIZE_MBYTE * 6);
					QS_MEMORY_POOL * temporary_json_memory = (QS_MEMORY_POOL*)QS_GET_POINTER(temporary_memory, memid_temporary_json_memory);
					qs_memory_clean(temporary_json_memory);
					qs_resize_copy_mini_memory(temporary_json_memory, cache_page.memory);
					//printf("/api/v1/dump api);
					http_status_code = http_json_response_common(tinfo, option, temporary_json_memory, cache_page.hash_id, SIZE_MBYTE * 3);
				} while (false);
			}
		}
	}

	if( http_status_code != 200 ){
		if( http_status_code == 304 ){
			qs_send( option, &tinfo->sockparam, HTTP_NOT_MODIFIED, qs_strlen( HTTP_NOT_MODIFIED ), 0 );
		}
		else if( http_status_code == 404 ){
			qs_send( option, &tinfo->sockparam, HTTP_NOT_FOUND, qs_strlen( HTTP_NOT_FOUND ), 0 );
		}
		else{
			qs_send( option, &tinfo->sockparam, HTTP_INTERNAL_SERVER_ERROR, qs_strlen( HTTP_INTERNAL_SERVER_ERROR ), 0 );
		}
	}

	// log
	qs_http_access_log(&log_file_info, http_request.http_version,http_request.user_agent,tinfo->hbuf, http_request.method, http_request.request, http_status_code);

	qs_disconnect(&tinfo->sockparam);
	return ((void *)NULL);
}

int on_close(QS_SERVER_CONNECTION_INFO* connection)
{
	return 0;
}
