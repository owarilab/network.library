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
void* on_connect(void* arg);
int32_t on_recv(uint32_t payload_type, uint8_t* payload, size_t payload_len, GDT_RECV_INFO *gdt_recv_info );
void* on_close(void* arg);

int main( int argc, char *argv[], char *envp[] )
{
	GDT_SOCKET_OPTION* option = gdt_create_tcp_client("localhost", "5001");
	set_on_connect_event(option,on_connect);
	set_on_payload_recv_event(option,on_recv);
	set_on_close_event(option,on_close);
	gdt_socket(option);
	while(1){
		usleep(1000);
		gdt_client_send_message(0x01,"test",4,option);
		usleep(1000);
		gdt_client_send_message(0x02,"test",4,option);
		usleep(1000);
		gdt_client_send_message(0x03,"test",4,option);
	}
	gdt_free_socket(option);
	gdt_free(option->memory_pool);
	return 0;
}

void* on_connect(void* arg)
{
	return (void*)NULL;
}

int32_t on_recv(uint32_t payload_type, uint8_t* payload, size_t payload_len, GDT_RECV_INFO *gdt_recv_info )
{
	return 0;
}

void* on_close(void* arg)
{
	return (void*)NULL;
}

