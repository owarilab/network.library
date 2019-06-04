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
int on_connect(GDT_SERVER_CONNECTION_INFO* connection);
int32_t on_recv(uint32_t payload_type, uint8_t* payload, size_t payload_len, GDT_RECV_INFO *gdt_recv_info );
int on_close(GDT_SERVER_CONNECTION_INFO* connection);
int32_t on_udp_client_payload_recv(uint32_t payload_type, uint8_t* payload, size_t payload_len, GDT_RECV_INFO *gdt_recv_info);

int main( int argc, char *argv[], char *envp[] )
{
	GDT_SOCKET_OPTION* tcp_client = gdt_create_tcp_client("localhost", "52001");
	set_on_connect_event(tcp_client,on_connect);
	set_on_payload_recv_event(tcp_client,on_recv);
	set_on_close_event(tcp_client,on_close);
	gdt_socket(tcp_client);
	
	GDT_SOCKET_OPTION*  udp_client = gdt_create_udp_client("localhost", "52001");
	set_on_payload_recv_event(udp_client, on_udp_client_payload_recv);
	gdt_socket(udp_client);
	
	while(1){
		gdt_client_update(tcp_client);
		gdt_client_update(udp_client);
		usleep(100000);
		gdt_client_send_message(0x01,"test1",4,tcp_client);
		usleep(100000);
		gdt_client_send_message(0x02,"test2",4,udp_client);
	}
	gdt_free_socket(tcp_client);
	gdt_free(tcp_client->memory_pool);
	gdt_free_socket(udp_client);
	gdt_free(udp_client->memory_pool);
	return 0;
}

int on_connect(GDT_SERVER_CONNECTION_INFO* connection)
{
	printf("on_connect\n");
	return 0;
}

int32_t on_recv(uint32_t payload_type, uint8_t* payload, size_t payload_len, GDT_RECV_INFO *gdt_recv_info )
{
	printf("on_recv(payload_type:%d,payload_len:%d)%s\n",payload_type,(int)payload_len,(char*)payload);
	return 0;
}

int on_close(GDT_SERVER_CONNECTION_INFO* connection)
{
	printf("on_close\n");
	return 0;
}

int32_t on_udp_client_payload_recv(uint32_t payload_type, uint8_t* payload, size_t payload_len, GDT_RECV_INFO *gdt_recv_info)
{
	char *pbuf = (char*)payload;
	pbuf[payload_len] = '\0';
	printf("recv udp(payload_type:%d,payload_len:%d) %s\n", (int)payload_type, (int)payload_len, pbuf);
	return 0;
}
