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

#ifndef _GNT_QUEUE_H_
#define _GNT_QUEUE_H_

#include "core.h"
#include "gnt_memory_allocator.h"

struct MSGQUEUE
{
	int32_t top;		// 先頭
	int32_t tail;		// 末尾
	size_t queuelen;	// キュー配列のサイズ
	int32_t queuemunit;	// キューを格納するメモリユニットの配列格納先
};

struct MSG_INFO
{
	int32_t id;			// メッセージ配信者識別ID
	int32_t msgmunit;	// メッセージ文字列を格納するメモリユニットの配列格納先
	size_t len;			// メッセージのサイズ
	uint8_t status;		// メッセージ状態( 削除フラグとか )
};

void createMsgQueue( GNT_MEMORY_POOL* _ppool, int32_t *q_munit, size_t qlen, size_t size );
int pushQueue( GNT_MEMORY_POOL* _ppool, int32_t q_munit, const char* pbuf, size_t size );

#endif /*_GNT_QUEUE_H_*/
