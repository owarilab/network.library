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
int on_connect(QS_SERVER_CONNECTION_INFO* connection);
int32_t on_recv(uint32_t payload_type, uint8_t* payload, size_t payload_len, QS_RECV_INFO *qs_recv_info );
int on_close(QS_SERVER_CONNECTION_INFO* connection);

int32_t on_recv_udp(uint32_t payload_type, uint8_t* payload, size_t payload_len, QS_RECV_INFO *qs_recv_info );

int main( int argc, char *argv[], char *envp[] )
{
	qs_set_defaultsignal();
	QS_SOCKET_OPTION* option = qs_create_tcp_server(NULL, "52001");
	set_on_connect_event(option,on_connect);
	set_on_payload_recv_event(option,on_recv);
	set_on_close_event(option,on_close);
	qs_socket(option);
	
	QS_SOCKET_OPTION* udp_option = qs_create_udp_server(NULL, "52001");
	set_on_payload_recv_event(udp_option,on_recv_udp);
	qs_socket(udp_option);

	int timer = time(NULL);
	int now_time = timer;
	while(1){
		qs_server_update(option);
		qs_server_update(udp_option);
		qs_sleep(100);
		now_time = time(NULL);
		if( now_time - timer > 5 ){
			timer = now_time;
			qs_send_broadcast( option,"timer",5,0x01);
		}
	}
	qs_free_socket(option);
	qs_free(option->memory_pool);
	qs_free_socket(udp_option);
	qs_free(udp_option->memory_pool);
	return 0;
}

int on_connect(QS_SERVER_CONNECTION_INFO* connection)
{
	printf("on_connect\n");
	return 0;
}

int32_t on_recv(uint32_t payload_type, uint8_t* payload, size_t payload_len, QS_RECV_INFO *qs_recv_info )
{
	printf("on_recv(payload_type:%d,payload_len:%d)%s\n",payload_type,(int)payload_len,(char*)payload);
	return 0;
}

int on_close(QS_SERVER_CONNECTION_INFO* connection)
{
	printf("on_close\n");
	return 0;
}

int32_t on_recv_udp(uint32_t payload_type, uint8_t* payload, size_t payload_len, QS_RECV_INFO *qs_recv_info )
{
	printf("on_recv_udp(payload_type:%d,payload_len:%d)%s\n",payload_type,(int)payload_len,(char*)payload);
	struct sockaddr_storage from;
	socklen_t fromlen;
	memcpy(&from,&qs_recv_info->tinfo->sockparam.from,qs_recv_info->tinfo->sockparam.fromlen);
	fromlen = qs_recv_info->tinfo->sockparam.fromlen;
	qs_send_message(0x01, (char*)&from,fromlen, qs_recv_info);
	struct sockaddr_in* pfrom = (struct sockaddr_in*)&from;
	printf("%s:%d --> server\n", inet_ntoa(pfrom->sin_addr), ntohs(pfrom->sin_port));
	return 0;
}
