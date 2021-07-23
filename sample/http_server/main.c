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
#include "gdt_protocol.h"
#include "gdt_variable.h"

int on_connect(GDT_SERVER_CONNECTION_INFO* connection);
void* on_recv( void* args );
int on_close(GDT_SERVER_CONNECTION_INFO* connection);

int main( int argc, char *argv[], char *envp[] )
{
	gdt_set_defaultsignal();
	GDT_SOCKET_OPTION* option = gdt_create_tcp_server_plane(NULL, "8080");
	set_on_connect_event(option,on_connect);
	set_on_packet_recv_event(option,on_recv);
	set_on_close_event(option,on_close);
	gdt_socket(option);
	while(1){
		gdt_server_update(option);
		usleep(100);
	}
	gdt_free_socket(option);
	gdt_free(option->memory_pool);
	return 0;
}

int on_connect(GDT_SERVER_CONNECTION_INFO* connection)
{
	return 0;
}

void* on_recv( void* args )
{
	GDT_RECV_INFO *rinfo = (GDT_RECV_INFO *)args;
	GDT_SERVER_CONNECTION_INFO * tinfo = (GDT_SERVER_CONNECTION_INFO *)rinfo->tinfo;
	GDT_SOCKET_OPTION* option = (GDT_SOCKET_OPTION*)tinfo->gdt_socket_option;
	GDT_SOCKPARAM* psockparam = &tinfo->sockparam;
	switch( gdt_http_protocol_filter(rinfo) )
	{
		case -1:
			gdt_disconnect( psockparam );
			return ( (void *) NULL );
		case 0:
			return ( (void *) NULL );
		case 1:
			break;
	}
	gdt_send(option, &tinfo->sockparam, HTTP_OK, gdt_strlen(HTTP_OK), 0);
	gdt_hash_dump(option->memory_pool, tinfo->sockparam.http_header_munit,0);

	char *method = (char*)GDT_POINTER(option->memory_pool,gdt_get_hash( option->memory_pool, tinfo->sockparam.http_header_munit, "HTTP_METHOD" ));
	char *request = (char*)GDT_POINTER(option->memory_pool,gdt_get_hash( option->memory_pool, tinfo->sockparam.http_header_munit, "REQUEST" ));
	printf("method : %s , request : %s\n",method,request);

	gdt_disconnect(&tinfo->sockparam);
	return ((void *)NULL);
}

int on_close(GDT_SERVER_CONNECTION_INFO* connection)
{
	return 0;
}
