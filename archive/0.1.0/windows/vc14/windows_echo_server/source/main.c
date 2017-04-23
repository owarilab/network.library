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

#include<stdio.h>
#include <process.h>
#include<gdt_memory_allocator.h>
#include<gdt_socket.h>

void* on_connect(void* args);
void* on_sent(void* args);
void* on_recv(void* args);
void* on_close(void* args);

int main(int argc, char *argv[])
{
	GDT_SOCKET_OPTION option;
	GDT_MEMORY_POOL* memory_pool = NULL;
	do {
		if (gdt_initialize_memory(&memory_pool, SIZE_MBYTE * 16, SIZE_MBYTE * 16, MEMORY_ALIGNMENT_SIZE_BIT_64, FIX_MUNIT_SIZE, 1, SIZE_KBYTE * 16) <= 0) {
			printf("gdt_initialize_memory error\n");
			break;
		}
		gdt_initialize_socket_option(&option, NULL, "1024", SOKET_TYPE_SERVER_TCP, SOCKET_MODE_SELECT, PROTOCOL_PLAIN, 32, memory_pool, NULL, on_connect, on_sent, on_recv, on_close);
		gdt_socket(&option);
		gdt_free_socket(&option);
		gdt_free(memory_pool);
	} while (0);
	return 1;
}

void* on_connect(void* args)
{
	return ((void *)NULL);
}

void* on_sent(void* args)
{
	return ((void *)NULL);
}

void* on_recv(void* args)
{
	struct GDT_RECV_INFO *rinfo;
	GDT_SERVER_CONNECTION_INFO * tinfo;
	char *pbuf;
	int i;
	rinfo = (struct GDT_RECV_INFO *) args;
	tinfo = (GDT_SERVER_CONNECTION_INFO *)rinfo->tinfo;
	pbuf = (char*)gdt_upointer(tinfo->option->memory_pool, rinfo->recvbuf_munit);
	pbuf[rinfo->recvlen] = '\0';
	printf("%s\n", pbuf);
	gdt_send(tinfo->option, &tinfo->sockparam, pbuf, rinfo->recvlen, 0);
	return ((void *)NULL);
}

void* on_close(void* args)
{
	return ((void *)NULL);
}
