// get api sample
// curl -X POST -H "Content-Type: application/json" -d '{"k":"id_12345678", "v":"kvs_value1"}' http://localhost:8080/api/v1/set
// curl -X POST -d 'k=id_12345678&v=kvs_value1' http://localhost:8080/api/v1/set

// set api sample
// curl -X POST -H "Content-Type: application/json" -d '{"k":"id_12345678"}' http://localhost:8080/api/v1/get
// curl -X POST -d 'k=id_12345678' http://localhost:8080/api/v1/get

// dump api sample
// curl -X POST -H "Content-Type: application/json" -d '[]' http://localhost:8080/api/v1/dump
// curl -X POST -d '' http://localhost:8080/api/v1/dump

// delete api sample
// curl -X POST -H "Content-Type: application/json" -d '{"k":"id_12345678"}' http://localhost:8080/api/v1/delete
// curl -X POST -d 'k=id_12345678' http://localhost:8080/api/v1/delete

// room/create api sample
// curl -X POST -H "Content-Type: application/json" -d '{"name":"test_room"}' http://localhost:8080/api/v1/room/create
// curl -X POST -d 'name=test_room' http://localhost:8080/api/v1/room/create

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

#include "qs_socket.h"
#include "qs_protocol.h"
#include "qs_variable.h"
#include "qs_random.h"
#include "qs_packet_route.h"

QS_MEMORY_POOL* memory_pool = NULL;
int32_t memid_temporary_memory = -1;
QS_MEMORY_POOL* kvs_storage_memory_pool = NULL;
int32_t memid_kvs_storage_id = -1;
int32_t memid_kvs_memory = -1;
int32_t memid_kvs_id = -1;

int32_t memid_route_memory = -1;
int32_t memid_packet_route = -1;

SYSTEM_UPDATE_SCHEDULER scheduler;

int on_connect(QS_SERVER_CONNECTION_INFO* connection);
void* on_recv( void* args );
int on_close(QS_SERVER_CONNECTION_INFO* connection);
void exec_http(QS_RECV_INFO *rinfo);
void exec_websocket(QS_RECV_INFO *rinfo);

int on_connect(QS_SERVER_CONNECTION_INFO* connection)
{
	return 0;
}

void* on_recv( void* args )
{
	QS_RECV_INFO *rinfo = (QS_RECV_INFO *)args;
	switch( qs_http_protocol_filter_with_websocket(rinfo) )
	{
		case -1:
		{
			QS_SERVER_CONNECTION_INFO * tinfo = (QS_SERVER_CONNECTION_INFO *)rinfo->tinfo;
			QS_SOCKPARAM* psockparam = &tinfo->sockparam;
			qs_disconnect( psockparam );
			break;
		}
		case QS_HTTP_SOCK_PHASE_RECV_CONTINUE:
		{
			break;
		}
		case QS_HTTP_SOCK_PHASE_PARSE_HTTP_HEADER:
		{
			break;
		}
		case QS_HTTP_SOCK_PHASE_MSG_HTTP:
		{
			exec_http(rinfo);
			break;
		}
		case QS_HTTP_SOCK_PHASE_HANDSHAKE_WEBSOCKET:
		{
			// make unique id
			QS_SERVER_CONNECTION_INFO * connection = (QS_SERVER_CONNECTION_INFO *)rinfo->tinfo;
			QS_MEMORY_POOL* route_memory = (QS_MEMORY_POOL*)QS_GET_POINTER(memory_pool,memid_route_memory);
			qs_change_packet_route_connection_id(route_memory,memid_packet_route,connection->index);
			break;
		}
		case QS_HTTP_SOCK_PHASE_MSG_WEBSOCKET:
		{
			exec_websocket(rinfo);
			break;
		}
	}
	scheduler.counter++;
	return ( (void *) NULL );
}

int on_close(QS_SERVER_CONNECTION_INFO* connection)
{
	QS_MEMORY_POOL* route_memory = (QS_MEMORY_POOL*)QS_GET_POINTER(memory_pool,memid_route_memory);
	void* data = qs_get_packet_route_connection_chain(route_memory, memid_packet_route, connection->index);
	if (NULL != data) {
		qs_remove_packet_route_connection(route_memory, memid_packet_route, connection->index);
	}
	return 0;
}

void exec_websocket(QS_RECV_INFO *rinfo)
{
	QS_SERVER_CONNECTION_INFO * tinfo = (QS_SERVER_CONNECTION_INFO *)rinfo->tinfo;
	QS_SOCKET_OPTION* option = (QS_SOCKET_OPTION*)tinfo->qs_socket_option;
	QS_MEMORY_POOL * temporary_memory = ( QS_MEMORY_POOL* )QS_GET_POINTER( option->memory_pool, memid_temporary_memory );
	qs_memory_clean( temporary_memory );

	QS_SOCKPARAM* psockparam = &tinfo->sockparam;
	ssize_t _tmpmsglen = qs_parse_websocket_binary( option, psockparam, (uint8_t*)qs_upointer( option->memory_pool, rinfo->recvbuf_munit ), rinfo->recvlen, tinfo->recvmsg_munit );
	if( _tmpmsglen < 0 ){
		qs_disconnect( psockparam );
		return;
	}
	if( psockparam->tmpmsglen == 0 ){
		char* msgpbuf = (char*)qs_upointer( option->memory_pool, tinfo->recvmsg_munit );
		rinfo->recvlen = _tmpmsglen;
		msgpbuf[rinfo->recvlen] = '\0';
		if(rinfo->recvlen==0){ // ping packet
			return;
		}
		rinfo->recvbuf_munit = tinfo->recvmsg_munit;

		QS_MEMORY_POOL* route_memory = (QS_MEMORY_POOL*)QS_GET_POINTER(memory_pool,memid_route_memory);
		int32_t memid_temp_data = qs_create_hash(temporary_memory, 32);
		if (-1 == memid_temp_data) {
			return;
		}
		qs_add_hash_string(temporary_memory,memid_temp_data,"id",qs_change_packet_route_connection_id(route_memory,memid_packet_route,tinfo->index));
		qs_add_hash_string(temporary_memory,memid_temp_data,"message",msgpbuf);
		int32_t memid_temp_data_json = qs_json_encode_hash(temporary_memory, memid_temp_data, SIZE_KBYTE * 4);
		if (-1 == memid_temp_data_json) {
			printf("qs_json_encode_hash error\n");
			return;
		}
		char* json = (char*)QS_GET_POINTER(temporary_memory, memid_temp_data_json);
		size_t json_len = qs_strlen(json);

		int32_t message_buffer_munit = qs_create_munit(temporary_memory,SIZE_KBYTE*8,MEMORY_TYPE_DEFAULT);
		void* buffer = QS_GET_POINTER(temporary_memory,message_buffer_munit);
		size_t buffer_size = qs_usize(temporary_memory,message_buffer_munit);
		int i;
		ssize_t sendlen = qs_make_websocket_msg(option,buffer,buffer_size,false,json,json_len);
		ssize_t ret = 0;
		QS_SERVER_CONNECTION_INFO *tmptinfo;
		if( option->connection_munit == -1 ){
			return;
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
}

void exec_http(QS_RECV_INFO *rinfo)
{
	QS_SERVER_CONNECTION_INFO * tinfo = (QS_SERVER_CONNECTION_INFO *)rinfo->tinfo;
	QS_SOCKET_OPTION* option = (QS_SOCKET_OPTION*)tinfo->qs_socket_option;
	QS_MEMORY_POOL * temporary_memory = ( QS_MEMORY_POOL* )QS_GET_POINTER( option->memory_pool, memid_temporary_memory );
	qs_memory_clean( temporary_memory );

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
					//printf("/api/v1/dump api\n");
					http_status_code = http_json_response_common(tinfo, option, temporary_json_memory, cache_page.hash_id, SIZE_MBYTE * 3);
				} while (false);
			}
			if (!strcmp(http_request.request, "/api/v1/room/create")) {
				do {
					if (-1 == http_request.memid_post_parameter_hash) {
						break;
					}
					if (-1 == qs_get_hash(http_request.temporary_memory, http_request.memid_post_parameter_hash, "name")) {
						break;
					}
					char* name = (char*)QS_GET_POINTER(http_request.temporary_memory, qs_get_hash(http_request.temporary_memory, http_request.memid_post_parameter_hash, "name"));
					if(strlen(name)>32){
						break;
					}
					char id[QS_PACKET_ROUTE_KEY_SIZE_DEFAULT+1];
					int32_t route_capacity = 10;
					int32_t life_time = 60 * 5; // 5 minutes
					qs_uniqid_r32(id,sizeof(id)-1);
					QS_MEMORY_POOL* route_memory = (QS_MEMORY_POOL*)QS_GET_POINTER(memory_pool,memid_route_memory);
					int32_t route_offset = qs_create_packet_route(route_memory, memid_packet_route, id, route_capacity, life_time);
					if(-1 == route_offset){
						break;
					}
					int32_t memid_route_data = qs_create_hash(temporary_memory, 32);
					if (-1 == memid_route_data) {
						break;
					}
					qs_add_hash_string(temporary_memory,memid_route_data,"name",name);
					int32_t memid_route_data_json = qs_json_encode_hash(temporary_memory, memid_route_data, SIZE_KBYTE * 4);
					if (-1 == memid_route_data_json) {
						printf("qs_json_encode_hash error\n");
						break;
					}
					char* json = (char*)QS_GET_POINTER(temporary_memory, memid_route_data_json);
					size_t json_len = qs_strlen(json);
					if( -1 == qs_set_route_data(route_memory, memid_packet_route, route_offset, (uint8_t*)json, json_len) ){
						printf("qs_set_route_data error : %ld\n",json_len);
						break;
					}
					int32_t memid_temp_info_hash = qs_get_route_info(route_memory, memid_packet_route,temporary_memory,route_offset);
					if(-1 == memid_temp_info_hash){
						break;
					}
					qs_add_hash_hash(temporary_memory, memid_response_data, "room", memid_temp_info_hash);
					qs_add_hash_hash(temporary_memory, memid_response, "data", memid_response_data);
					http_status_code = http_json_response_common(tinfo, option, temporary_memory, memid_response, SIZE_KBYTE * 4);
				} while (false);
			}
			if (!strcmp(http_request.request, "/api/v1/room/list")) {
				do {
					QS_MEMORY_POOL* route_memory = (QS_MEMORY_POOL*)QS_GET_POINTER(memory_pool,memid_route_memory);
					int32_t routes_hash = qs_get_route_infos(route_memory,memid_packet_route,temporary_memory);
					if(-1 == routes_hash){
						break;
					}
					qs_add_hash_hash(temporary_memory, memid_response_data, "rooms", routes_hash);
					qs_add_hash_hash(temporary_memory, memid_response, "data", memid_response_data);
					http_status_code = http_json_response_common(tinfo, option, temporary_memory, memid_response, SIZE_KBYTE * 128);
				} while (false);
			}
			if (!strcmp(http_request.request, "/api/v1/room/join")) {
				do {
					if (-1 == http_request.memid_post_parameter_hash) {
						break;
					}
					if (-1 == qs_get_hash(http_request.temporary_memory, http_request.memid_post_parameter_hash, "room_id")) {
						break;
					}
					if (-1 == qs_get_hash(http_request.temporary_memory, http_request.memid_post_parameter_hash, "connection_id")) {
						break;
					}
					char* room_id = (char*)QS_GET_POINTER(http_request.temporary_memory, qs_get_hash(http_request.temporary_memory, http_request.memid_post_parameter_hash, "room_id"));
					char* connection_id = (char*)QS_GET_POINTER(http_request.temporary_memory, qs_get_hash(http_request.temporary_memory, http_request.memid_post_parameter_hash, "connection_id"));
					QS_MEMORY_POOL* route_memory = (QS_MEMORY_POOL*)QS_GET_POINTER(memory_pool,memid_route_memory);
					int32_t connection_index = qs_find_packet_route_connection_id(route_memory,memid_packet_route,connection_id);
					if( -1 == connection_index){
						printf("qs_find_packet_route_connection_id error\n");
						break;
					}
					void* data = qs_get_packet_route_connection_chain(route_memory, memid_packet_route,connection_index);
					if(data!=NULL){
						printf("qs_get_packet_route_connection_chain error\n");
						break;
					}
					int32_t route_offset = qs_get_packet_route(route_memory, memid_packet_route, room_id);
					if (-1 == route_offset) {
						printf("qs_get_packet_route error\n");
						break;
					}
					int32_t connection_offset = qs_add_packet_route_connection(route_memory, memid_packet_route, route_offset,connection_index);
					if(-1==connection_offset){
						printf("qs_add_packet_route_connection error\n");
						break;
					}
					int32_t memid_temp_info_hash = qs_get_route_info(route_memory, memid_packet_route,temporary_memory,route_offset);
					if(-1 == memid_temp_info_hash){
						printf("qs_get_route_info error\n");
						break;
					}
					qs_add_hash_hash(temporary_memory, memid_response_data, "room", memid_temp_info_hash);
					qs_add_hash_hash(temporary_memory, memid_response, "data", memid_response_data);
					http_status_code = http_json_response_common(tinfo, option, temporary_memory, memid_response, SIZE_KBYTE * 4);
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
	qs_http_access_log(&option->log_access_file_info, http_request.http_version,http_request.user_agent,tinfo->hbuf, http_request.method, http_request.request, http_status_code);

	qs_disconnect(&tinfo->sockparam);
}

int main( int argc, char *argv[], char *envp[] )
{
#ifndef __WINDOWS__
	int exe_code = EX_OK;
#else
	int exe_code = 0;
#endif
	char* phostname;
	char hostname[256];
	char portnum[32];
	int inetflag = 0;
	gnt_set_argv( &argc, &argv, &envp );
#ifndef __WINDOWS__
	//qs_set_defaultsignal();
	//qs_daemonize(1, 0);
#else
	SetConsoleOutputCP(CP_UTF8);
#endif
	int32_t maxconnection = 1000;

	if( qs_initialize_memory( &memory_pool, SIZE_MBYTE * 128, SIZE_MBYTE * 128, MEMORY_ALIGNMENT_SIZE_BIT_64, FIX_MUNIT_SIZE, 1, SIZE_KBYTE * 16 ) <= 0 ){
		printf( "qs_initialize_memory error\n" );
		return -1;
	}

	if( 0 >= ( memid_temporary_memory = qs_create_mini_memory( memory_pool, SIZE_MBYTE * 8 ) ) ){
		printf( "qs_create_mini_memory error\n" );
		return -1;
	}

	qs_srand_32();

	// kvs
	{
		memid_kvs_memory = qs_create_mini_memory(memory_pool, SIZE_MBYTE * 1);

		QS_MEMORY_POOL* cache_memory = (QS_MEMORY_POOL*)QS_GET_POINTER(memory_pool,memid_kvs_memory);
		memid_kvs_id = qs_create_cache_B1MB(cache_memory);
		if(-1 == memid_kvs_id) {
			printf("create cache memory error\n");
			return -1;
		}

		memid_kvs_storage_id = qs_create_storage_cache_B1MB("./kvs_data_b1mb",&kvs_storage_memory_pool);
		if(-1 == memid_kvs_storage_id) {
			printf("create storage memory error\n");
			return -1;
		}
	}

	// packet route
	QS_MEMORY_POOL* route_memory = NULL;
	{	
		if( 0 >= ( memid_route_memory = qs_create_mini_memory( memory_pool, SIZE_MBYTE * 16 ) ) ){
			printf( "qs_create_mini_memory error\n" );
			return -1;
		}
		route_memory = (QS_MEMORY_POOL*)QS_GET_POINTER(memory_pool,memid_route_memory);
		memid_packet_route = qs_init_packet_route(route_memory, maxconnection, QS_PACKET_ROUTE_ROUTE_SIZE_DEFAULT, QS_PACKET_ROUTE_KEY_SIZE_DEFAULT, SIZE_KBYTE*2);
		if(-1==memid_packet_route){
			printf( "qs_init_packet_route error\n" );
			return -1;
		}
	}

	phostname = NULL;
	memset( hostname, 0, sizeof( hostname ) );
	memset( portnum, 0, sizeof( portnum ) );
	snprintf( hostname, sizeof( hostname ) -1, "localhost" );
	snprintf( portnum, sizeof( portnum ) -1, "8080" );
	QS_SOCKET_OPTION option;
	qs_initialize_socket_option( &option, phostname, portnum, SOCKET_TYPE_SERVER_TCP, SOCKET_MODE_NONBLOCKING, PROTOCOL_PLAIN, maxconnection, memory_pool, NULL );
	option.inetflag = inetflag;
	set_on_connect_event( &option, on_connect );
	set_on_packet_recv_event( &option, on_recv );
	set_on_close_event( &option, on_close );
	qs_set_connection_timeout(&option,5,0); // 5sec
	qs_socket( &option );
	qs_initialize_scheduler(&scheduler);
	
	scheduler.update_max = 10;
	scheduler.update_high = 100;
	scheduler.update_middle = 200;
	scheduler.update_low = 500;
	scheduler.update_idle = 10000;
	scheduler.counter_high = 100;
	scheduler.counter_middle = 50;
	scheduler.counter_low = 20;

	time_t current_time = time(NULL);
	time_t update_time = 0;
	for(;;){
		current_time = time(NULL);
		if (current_time - update_time > 60) {
			if( QS_SYSTEM_ERROR == qs_log_rotate(&option.log_access_file_info, "./", "access", 0)){
				printf("qs_log_rotate error\n");
			}
			if( QS_SYSTEM_ERROR == qs_log_rotate(&option.log_error_file_info, "./", "error", 0)){
				printf("qs_log_rotate error\n");
			}
			update_time = current_time;
		}
		qs_sleep(scheduler.sleep_time);
		qs_update_scheduler(&scheduler);
		qs_server_update(&option);
		qs_update_packet_route(route_memory, memid_packet_route);
	}
	qs_free_socket( &option );
	qs_free( option.memory_pool );
	return exe_code;
}
