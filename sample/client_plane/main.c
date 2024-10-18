/*
 * Copyright (c) 2014-2024 Katsuya Owari
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

int on_connect(QS_SERVER_CONNECTION_INFO* connection);
int32_t on_recv(uint32_t payload_type, uint8_t* payload, size_t payload_len, QS_RECV_INFO *qs_recv_info );
void* on_pkt_recv( void* args );
int on_close(QS_SERVER_CONNECTION_INFO* connection);
int32_t on_udp_client_payload_recv(uint32_t payload_type, uint8_t* payload, size_t payload_len, QS_RECV_INFO *qs_recv_info);

int main( int argc, char *argv[], char *envp[] )
{
	QS_SOCKET_OPTION* tcp_client = qs_create_tcp_client_plane("localhost", "52001");
	set_on_connect_event(tcp_client,on_connect);
	set_on_packet_recv_event(tcp_client, on_pkt_recv );
	set_on_close_event(tcp_client,on_close);
	qs_socket(tcp_client);
	//qs_wait_client_socket(tcp_client);
	
	while(1){
		qs_client_update(tcp_client);
		qs_sleep(10);
	}
	qs_free_socket(tcp_client);
	qs_free(tcp_client->memory_pool);
	return 0;
}

int on_connect(QS_SERVER_CONNECTION_INFO* connection)
{
	printf("on_connect\n");
	return 0;
}

void* on_pkt_recv( void* args )
{
	QS_RECV_INFO *rinfo = (QS_RECV_INFO*)args;
	QS_SERVER_CONNECTION_INFO *connection = (QS_SERVER_CONNECTION_INFO*)rinfo->tinfo;
	QS_SOCKET_OPTION* tcp_client = (QS_SOCKET_OPTION*)connection->qs_socket_option;
	printf("on_pkt_recv\n");

	char* msg = (char*)QS_GET_POINTER(tcp_client->memory_pool,rinfo->recvbuf_munit);
	msg+=6; // skip header bytes
	int len = rinfo->recvlen;
	printf("msg len:%d , msg:%s\n",len,msg);

	return ( (void *) NULL );
}

int on_close(QS_SERVER_CONNECTION_INFO* connection)
{
	printf("on_close\n");
	return 0;
}
