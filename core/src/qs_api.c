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

#include "qs_api.h"

int api_qs_init(QS_SERVER_CONTEXT** ppcontext, int port)
{
	if( ( (*ppcontext) = ( QS_SERVER_CONTEXT * )malloc( sizeof( QS_SERVER_CONTEXT ) ) ) == NULL ){
		return -1;
	}
	QS_SERVER_CONTEXT* context = *ppcontext;
	context->system_data = NULL;
	context->on_connect = NULL;
	context->on_recv = NULL;
	context->on_close = NULL;
	context->current_time = time(NULL);
	context->update_time = 0;
	QS_MEMORY_POOL* main_memory_pool = NULL;
	int32_t maxconnection = 100;
	if( qs_initialize_memory( &main_memory_pool, SIZE_MBYTE * 24, SIZE_MBYTE * 24, MEMORY_ALIGNMENT_SIZE_BIT_64, FIX_MUNIT_SIZE, 1, SIZE_KBYTE * 16 ) <= 0 ){
		free(context); context=NULL;
		return -2;
	}
	context->memory = (void*)main_memory_pool;
	if( -1 == ( context->memid_temporary_memory = qs_create_mini_memory( main_memory_pool, SIZE_MBYTE * 8 ) ) ){
		free(context); context=NULL;
		return -3;
	}
	int32_t server_munit = qs_create_munit(main_memory_pool, sizeof(QS_SOCKET_OPTION), MEMORY_TYPE_DEFAULT);
	if (server_munit == -1) {
		free(context); context=NULL;
		return -4;
	}
	QS_SOCKET_OPTION* server = (QS_SOCKET_OPTION*)QS_GET_POINTER(main_memory_pool, server_munit);
	char portnum[32];
	memset( portnum, 0, sizeof( portnum ) );
	snprintf( portnum, sizeof( portnum ) -1, "%d", port );
	if (-1 == qs_initialize_socket_option(server, NULL, portnum, SOCKET_TYPE_SERVER_TCP, SOCKET_MODE_NONBLOCKING, PROTOCOL_PLAIN, maxconnection, main_memory_pool, NULL)) {
		free(context); context=NULL;
		return -5;
	}
	set_on_connect_event(server, api_qs_core_on_connect );
	set_on_packet_recv_event(server, api_qs_core_on_recv );
	set_on_close_event(server, api_qs_core_on_close );
	if (-1 == qs_socket(server)) {
		free(context); context=NULL;
		return -6;
	}
	context->memid_server = server_munit;
	int32_t memid_scheduler = qs_create_munit(main_memory_pool, sizeof(SYSTEM_UPDATE_SCHEDULER), MEMORY_TYPE_DEFAULT);
	if (memid_scheduler == -1) {
		free(context); context=NULL;
		return -7;
	}
	SYSTEM_UPDATE_SCHEDULER* scheduler = (SYSTEM_UPDATE_SCHEDULER*)QS_GET_POINTER(main_memory_pool, memid_scheduler);
	qs_initialize_scheduler_high_speed(scheduler);
	context->memid_scheduler = memid_scheduler;
	server->application_data = (void*)context;
	//qs_memory_info(main_memory_pool);
	return 0;
}
void api_qs_set_on_connect_event(QS_SERVER_CONTEXT* context, QS_EVENT_FUNCTION on_connect )
{
	context->on_connect = on_connect;
}
void api_qs_set_on_packet_recv_event(QS_SERVER_CONTEXT* context, QS_EVENT_FUNCTION on_recv )
{
	context->on_recv = on_recv;
}
void api_qs_set_on_close_event(QS_SERVER_CONTEXT* context, QS_EVENT_FUNCTION on_close )
{
	context->on_close = on_close;
}
void api_qs_update(QS_SERVER_CONTEXT* context)
{
	QS_MEMORY_POOL* main_memory_pool = (QS_MEMORY_POOL*)context->memory;
	QS_SOCKET_OPTION* server = (QS_SOCKET_OPTION*)QS_GET_POINTER(main_memory_pool, context->memid_server);
	context->current_time = time(NULL);
	if (context->current_time - context->update_time > 60) {
		if( QS_SYSTEM_ERROR == qs_log_rotate(&server->log_access_file_info, "./", "access", 0)){}
		if( QS_SYSTEM_ERROR == qs_log_rotate(&server->log_error_file_info, "./", "error", 0)){}
		context->update_time = context->current_time;
	}
	qs_server_update(server);
}

void api_qs_sleep(QS_SERVER_CONTEXT* context)
{
	QS_MEMORY_POOL* main_memory_pool = (QS_MEMORY_POOL*)context->memory;
	SYSTEM_UPDATE_SCHEDULER* scheduler = (SYSTEM_UPDATE_SCHEDULER*)QS_GET_POINTER(main_memory_pool, context->memid_scheduler);
	qs_sleep(scheduler->sleep_time);
	qs_update_scheduler(scheduler);
}

void api_qs_free(QS_SERVER_CONTEXT* context)
{
	QS_MEMORY_POOL* main_memory_pool = (QS_MEMORY_POOL*)context->memory;
	QS_SOCKET_OPTION* server = (QS_SOCKET_OPTION*)QS_GET_POINTER(main_memory_pool, context->memid_server);
	qs_free_socket( server );
	qs_free( server->memory_pool );
	free(context);
}

void api_qs_on_recv( void* params )
{
	QS_RECV_INFO *rinfo = (QS_RECV_INFO *)params;
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
			api_qs_exec_http(rinfo);
			break;
		}
		case QS_HTTP_SOCK_PHASE_HANDSHAKE_WEBSOCKET:
		{
			break;
		}
		case QS_HTTP_SOCK_PHASE_MSG_WEBSOCKET:
		{
			api_qs_exec_websocket(rinfo);
			break;
		}
	}
	QS_SERVER_CONTEXT* context = ((QS_SOCKET_OPTION*)rinfo->tinfo->qs_socket_option)->application_data;
	QS_MEMORY_POOL* main_memory_pool = (QS_MEMORY_POOL*)context->memory;
	SYSTEM_UPDATE_SCHEDULER* scheduler = (SYSTEM_UPDATE_SCHEDULER*)QS_GET_POINTER(main_memory_pool, context->memid_scheduler);
	scheduler->counter++;
}

void api_qs_exec_websocket(QS_RECV_INFO *rinfo)
{
	QS_SERVER_CONNECTION_INFO * tinfo = (QS_SERVER_CONNECTION_INFO *)rinfo->tinfo;
	QS_SOCKET_OPTION* option = (QS_SOCKET_OPTION*)tinfo->qs_socket_option;
	QS_SERVER_CONTEXT* context = (QS_SERVER_CONTEXT*)option->application_data;
	QS_MEMORY_POOL * temporary_memory = ( QS_MEMORY_POOL* )QS_GET_POINTER( option->memory_pool, context->memid_temporary_memory );
	qs_memory_clean( temporary_memory );
	QS_SOCKPARAM* psockparam = &tinfo->sockparam;
	ssize_t _tmpmsglen = qs_parse_websocket_binary( option, psockparam, (uint8_t*)qs_upointer( option->memory_pool, rinfo->recvbuf_munit ), rinfo->recvlen, tinfo->recvmsg_munit );
	if( _tmpmsglen < 0 ){
		qs_disconnect( psockparam );
		return;
	}
	if( psockparam->tmpmsglen == 0 ){
		if(rinfo->recvlen==0){ // ping packet
			return;
		}
		ssize_t ret = 0;
		if (-1 == (ret = qs_send_all(psockparam->acc, "test", 4, 0))) {}
	}
}

void api_qs_exec_http(QS_RECV_INFO *rinfo)
{
	QS_SERVER_CONNECTION_INFO * tinfo = (QS_SERVER_CONNECTION_INFO *)rinfo->tinfo;
	QS_SOCKET_OPTION* option = (QS_SOCKET_OPTION*)tinfo->qs_socket_option;
	QS_SERVER_CONTEXT* context = (QS_SERVER_CONTEXT*)option->application_data;
	QS_MEMORY_POOL * temporary_memory = ( QS_MEMORY_POOL* )QS_GET_POINTER( option->memory_pool, context->memid_temporary_memory );
	qs_memory_clean( temporary_memory );
	QS_HTTP_REQUEST_COMMON http_request;
	int32_t http_status_code = http_request_common(rinfo, &http_request, temporary_memory);
	// API
	if (http_status_code == 404) {
		if(context->on_recv!=NULL){
			context->system_data = (void*)(&http_request);
			http_status_code = context->on_recv((void*)rinfo);
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
	qs_http_access_log(&option->log_access_file_info, http_request.http_version,http_request.user_agent,tinfo->hbuf, http_request.method, http_request.request, http_status_code);
	qs_disconnect(&tinfo->sockparam);
}
int api_qs_core_on_connect(QS_SERVER_CONNECTION_INFO* connection)
{
	QS_SERVER_CONTEXT* context = ((QS_SOCKET_OPTION*)connection->qs_socket_option)->application_data;
	if(context->on_connect){
		context->on_connect((void*)connection);
	}
	return 0;
}
void* api_qs_core_on_recv( void* args )
{
	api_qs_on_recv( args );
	return ( (void *) NULL );
}
int api_qs_core_on_close(QS_SERVER_CONNECTION_INFO* connection)
{
	QS_SERVER_CONTEXT* context = ((QS_SOCKET_OPTION*)connection->qs_socket_option)->application_data;
	if(context->on_close){
		context->on_close((void*)connection);
	}
	return 0;
}

char* api_qs_get_http_method(void* params)
{
	QS_RECV_INFO *rinfo = (QS_RECV_INFO *)params;
	QS_SERVER_CONNECTION_INFO * tinfo = (QS_SERVER_CONNECTION_INFO *)rinfo->tinfo;
	QS_SOCKET_OPTION* option = (QS_SOCKET_OPTION*)tinfo->qs_socket_option;
	QS_SERVER_CONTEXT* context = (QS_SERVER_CONTEXT*)option->application_data;
	QS_HTTP_REQUEST_COMMON* http_request = (QS_HTTP_REQUEST_COMMON*)context->system_data;
	return http_request->method;
}
char* api_qs_get_http_path(void* params)
{
	QS_RECV_INFO *rinfo = (QS_RECV_INFO *)params;
	QS_SERVER_CONNECTION_INFO * tinfo = (QS_SERVER_CONNECTION_INFO *)rinfo->tinfo;
	QS_SOCKET_OPTION* option = (QS_SOCKET_OPTION*)tinfo->qs_socket_option;
	QS_SERVER_CONTEXT* context = (QS_SERVER_CONTEXT*)option->application_data;
	QS_HTTP_REQUEST_COMMON* http_request = (QS_HTTP_REQUEST_COMMON*)context->system_data;
	return http_request->request;
}
char* api_qs_get_http_get_parameter(void* params, char* name)
{
	QS_RECV_INFO *rinfo = (QS_RECV_INFO *)params;
	QS_SERVER_CONNECTION_INFO * tinfo = (QS_SERVER_CONNECTION_INFO *)rinfo->tinfo;
	QS_SOCKET_OPTION* option = (QS_SOCKET_OPTION*)tinfo->qs_socket_option;
	QS_SERVER_CONTEXT* context = (QS_SERVER_CONTEXT*)option->application_data;
	QS_HTTP_REQUEST_COMMON* http_request = (QS_HTTP_REQUEST_COMMON*)context->system_data;
	if (-1 == http_request->memid_get_parameter_hash) {
		return NULL;
	}
	if (-1 == qs_get_hash(http_request->temporary_memory, http_request->memid_get_parameter_hash, name)) {
		return NULL;
	}
	char* value = (char*)QS_GET_POINTER(http_request->temporary_memory, qs_get_hash(http_request->temporary_memory, http_request->memid_get_parameter_hash, name));
	return value;
}
char* api_qs_get_http_post_parameter(void* params, char* name)
{
	QS_RECV_INFO *rinfo = (QS_RECV_INFO *)params;
	QS_SERVER_CONNECTION_INFO * tinfo = (QS_SERVER_CONNECTION_INFO *)rinfo->tinfo;
	QS_SOCKET_OPTION* option = (QS_SOCKET_OPTION*)tinfo->qs_socket_option;
	QS_SERVER_CONTEXT* context = (QS_SERVER_CONTEXT*)option->application_data;
	QS_HTTP_REQUEST_COMMON* http_request = (QS_HTTP_REQUEST_COMMON*)context->system_data;
	if (-1 == http_request->memid_post_parameter_hash) {
		return NULL;
	}
	if (-1 == qs_get_hash(http_request->temporary_memory, http_request->memid_post_parameter_hash, name)) {
		return NULL;
	}
	char* value = (char*)QS_GET_POINTER(http_request->temporary_memory, qs_get_hash(http_request->temporary_memory, http_request->memid_post_parameter_hash, name));
	return value;
}
void api_qs_send_response(void* params, char* response)
{
	QS_RECV_INFO *rinfo = (QS_RECV_INFO *)params;
	QS_SERVER_CONNECTION_INFO * tinfo = (QS_SERVER_CONNECTION_INFO *)rinfo->tinfo;
	QS_SOCKET_OPTION* option = (QS_SOCKET_OPTION*)tinfo->qs_socket_option;
	qs_send( option, &tinfo->sockparam, response, qs_strlen( response ), 0 );
}
