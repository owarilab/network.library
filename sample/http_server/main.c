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
#include "qs_logger.h"

// api sample
// curl -X POST -H "Content-Type: application/json" -d '{"id":"id_12345678", "password":"jsontest"}' http://localhost:8080/api/v1/login
// curl -X POST -d 'id=id_12345678&password=test' http://localhost:8080/api/v1/login

int32_t memid_temporary_memory = -1;
QS_FILE_INFO log_file_info;

int on_connect(QS_SERVER_CONNECTION_INFO* connection);
void* on_recv( void* args );
int on_close(QS_SERVER_CONNECTION_INFO* connection);

int main( int argc, char *argv[], char *envp[] )
{
	qs_finit(&log_file_info);
	qs_set_defaultsignal();
	QS_SOCKET_OPTION* option = qs_create_tcp_server_plane(NULL, "8080");
	memid_temporary_memory = qs_create_mini_memory( option->memory_pool, SIZE_KBYTE * 256 );
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
	switch( qs_http_protocol_filter(rinfo) )
	{
		case -1:
		{
			QS_SOCKPARAM* psockparam = &tinfo->sockparam;
			qs_disconnect( psockparam );
			return ( (void *) NULL );
		}
		case 0:
			return ( (void *) NULL );
		case 1:
			break;
	}

	QS_MEMORY_POOL * temporary_memory = ( QS_MEMORY_POOL* )QS_GET_POINTER( option->memory_pool, memid_temporary_memory );
	qs_memory_clean( temporary_memory );

	QS_HTTP_REQUEST_COMMON http_request;
	int32_t http_status_code = http_request_common(rinfo, &http_request, temporary_memory);

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
