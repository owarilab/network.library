/*
 * Copyright (c) 2014-2016 Katsuya Owari
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
#include<gdt_core.h>
#include<gdt_memory_allocator.h>
#include<gdt_hash.h>
#include<gdt_script.h>
#include<gdt_base64.h>
#include<gdt_random.h>
#include<gdt_queue.h>
#include<gdt_socket.h>

GDT_MEMORY_POOL* __mp;
//typedef struct GDT_SERVER_THREAD_INFO GDT_SERVER_INFO;
typedef struct GDT_SERVER_CHILD_INFO GDT_SERVER_INFO;

void* _connection_start_callback(void* args);
void* _send_finish_callback(void* args);
void* _recv_callback(void* args);
void* _close_callback(void* args);

int main(int argc, char *argv[])
{
	GDT_SOCKET_OPTION option;
	do {
		__mp = NULL;
		if (gdt_initialize_memory(&__mp, SIZE_MBYTE * 16, SIZE_MBYTE * 16, MEMORY_ALIGNMENT_SIZE_BIT_64, FIX_MUNIT_SIZE, 1, SIZE_KBYTE * 16) <= 0) {
			printf("gdt_initialize_memory error\n");
			break;
		}

		gdt_initialize_socket_option(
			&option
			, NULL
			, "1024"
			, SOKET_TYPE_SERVER_TCP
			, SOCKET_MODE_SELECT
			, PROTOCOL_PLAIN
			, 4
			, __mp
			, NULL
			, _connection_start_callback
			, _send_finish_callback
			, _recv_callback
			, _close_callback
		);
		option.recvbufsize = 1024;
		option.queuebufsize = 1024;
		option.inetflag = 0;

		gdt_socket(&option);

		while (1) {
			Sleep(10);
		}

		gdt_free_socket(&option);
	} while (0);
	return 1;
}

/*
* 接続完了コールバック
*/
void* _connection_start_callback(void* args)
{
	GDT_SERVER_INFO * tinfo;
	tinfo = (GDT_SERVER_INFO *)args;
#ifdef __GDT_DEBUG__
	printf("_connection_start_callback\n");
#endif
	return ((void *)NULL);
}

/*
* 送信コールバック
*/
void* _send_finish_callback(void* args)
{
#ifdef __GDT_DEBUG__
	printf("_send_finish_callback\n");
#endif
	return ((void *)NULL);
}

/*
* 受信コールバック
*/
void* _recv_callback(void* args)
{
	struct GDT_RECV_INFO *rinfo;
	GDT_SERVER_INFO * tinfo;
	GDT_SERVER_INFO * tmptinfo;
	char *pbuf;
	int i;
#ifdef __GDT_DEBUG__
	printf("_recv_callback\n");
#endif
	rinfo = (struct GDT_RECV_INFO *) args;
	tinfo = (GDT_SERVER_INFO *)rinfo->tinfo;
	pbuf = (char*)gdt_upointer(__mp, rinfo->recvbuf_munit);
	pbuf[rinfo->recvlen] = '\0';
	printf("%s\n", pbuf);
	gdt_send(tinfo->option, &tinfo->sockParam, pbuf, rinfo->recvlen, 0);
	// send to connecting users
	for (i = 0; i < tinfo->option->maxconnection; i++)
	{
		if (tinfo->option->connection_munit >= 0)
		{
			tmptinfo = gdt_offsetpointer(tinfo->option->__mp, tinfo->option->connection_munit, sizeof(GDT_SERVER_INFO), i);
			if ((int)tmptinfo->sockParam.acc > 0 && tmptinfo->sockParam.acc != tinfo->sockParam.acc)
			{
				printf("sock id : %d\n", (int)tmptinfo->sockParam.acc);
				gdt_send(tinfo->option, &tmptinfo->sockParam, pbuf, rinfo->recvlen, 0);
			}
		}
	}
	return ((void *)NULL);
}

/*
* 切断コールバック
*/
void* _close_callback(void* args)
{
#ifdef __GDT_DEBUG__
	printf("_close_callback\n");
#endif
	return ((void *)NULL);
}
