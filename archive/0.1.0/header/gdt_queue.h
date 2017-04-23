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

#ifndef _GDT_QUEUE_H_
#define _GDT_QUEUE_H_

#include "gdt_core.h"
#include "gdt_memory_allocator.h"
#if defined(__LINUX__) || defined(__BSD_UNIX__) || defined(__ANDROID__) || defined(__IOS__)
#include <pthread.h>
#endif
#ifdef __WINDOWS__
#endif

typedef struct GDT_MSGQUEUE
{
	int32_t top;
	int32_t tail;
	size_t queuelen;
	int32_t queuemunit;
	int32_t mqlock_munit;
} GDT_MSGQUEUE;

typedef struct GDT_MSG_INFO
{
	int32_t id;
	int32_t msgmunit;
	size_t len;
	uint8_t status;
} GDT_MSG_INFO;

void gdt_create_message_queue( GDT_MEMORY_POOL* _ppool, int32_t *q_munit, size_t qlen, size_t size );
int gdt_push_queue( GDT_MEMORY_POOL* _ppool, int32_t q_munit, const char* pbuf, size_t size );
int gdt_safe_push_queue( GDT_MEMORY_POOL* _ppool, int32_t q_munit, const char* pbuf, size_t size );
int32_t gdt_dequeue( GDT_MEMORY_POOL* _ppool, int32_t q_munit );

#endif /*_GDT_QUEUE_H_*/

#ifdef __cplusplus
}
#endif
