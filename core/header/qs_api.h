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

#ifdef __cplusplus
extern "C"{
#endif

#ifndef _QS_API_H_
#define _QS_API_H_

#include "qs_socket.h"
#include "qs_protocol.h"
#include "qs_variable.h"

#define QS_EVENT_PARAMETER void*
typedef int (*QS_EVENT_FUNCTION)( QS_EVENT_PARAMETER params );

typedef struct QS_SERVER_CONTEXT
{
	void* memory;
	void* system_data;
	int32_t memid_temporary_memory;
	int32_t memid_scheduler;
	int32_t memid_server;
	time_t current_time;
	time_t update_time;
	QS_EVENT_FUNCTION on_connect;
	QS_EVENT_FUNCTION on_recv;
	QS_EVENT_FUNCTION on_close;
} QS_SERVER_CONTEXT;

int api_qs_init(QS_SERVER_CONTEXT** ppcontext);
void api_qs_set_on_connect_event(QS_SERVER_CONTEXT* context, QS_EVENT_FUNCTION on_connect );
void api_qs_set_on_packet_recv_event(QS_SERVER_CONTEXT* context, QS_EVENT_FUNCTION on_recv );
void api_qs_set_on_close_event(QS_SERVER_CONTEXT* context, QS_EVENT_FUNCTION on_close );
void api_qs_update(QS_SERVER_CONTEXT* context);
void api_qs_free(QS_SERVER_CONTEXT* context);
void api_qs_on_recv( void* params );
void api_qs_exec_http(QS_RECV_INFO *rinfo);
void api_qs_exec_websocket(QS_RECV_INFO *rinfo);
int api_qs_core_on_connect(QS_SERVER_CONNECTION_INFO* connection);
void* api_qs_core_on_recv( void* args );
int api_qs_core_on_close(QS_SERVER_CONNECTION_INFO* connection);

char* api_qs_get_http_method(QS_EVENT_PARAMETER params);
char* api_qs_get_http_path(QS_EVENT_PARAMETER params);
char* api_qs_get_http_get_parameter(QS_EVENT_PARAMETER params, char* name);
char* api_qs_get_http_post_parameter(QS_EVENT_PARAMETER params, char* name);
void api_qs_send_response(QS_EVENT_PARAMETER params, char* response);

#endif /*_QS_API_H_*/

#ifdef __cplusplus
}
#endif
