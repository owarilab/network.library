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

#ifdef __cplusplus
extern "C"{
#endif

#ifndef _QS_TOKEN_ANALYZER_H_
#define _QS_TOKEN_ANALYZER_H_

#include "qs_core.h"
#include "qs_array.h"
#include "qs_io.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define QS_TOKEN_RESIZE_QUANTITY 4
#define QS_TOKEN_READ_BUFFER_SIZE_MIN SIZE_BYTE * 256
#define ID_UNK				0
#define ID_NUM				1
#define ID_OP				2
#define ID_SIGN				3
#define ID_STR				4
#define ID_SYMBOL			5
#define ID_SYS_IF			6
#define ID_SYS_ELSE			7
#define ID_SYS_ELSEIF		8
#define ID_FLOAT			9
#define ID_SYS_LOOP			10
#define ID_SYS_WHILE		11
#define ID_SYS_NEWLINE		12
#define ID_NUM_U64			107	


// system words 
#define SYS_LOOP "loop"
#define SYS_WHILE "while"
#define SYS_IF "if"
#define SYS_ELSEIF "elseif"
#define SYS_ELSE "else"

typedef struct QS_TOKEN
{
	int32_t buf_munit;
	uint8_t type;
	uint16_t size;
} QS_TOKEN;

typedef struct QS_TOKENS
{
	int32_t token_munit;
	size_t read_buffer_size;
	size_t size;
	uint32_t currentpos;
	uint32_t workpos;
	int32_t allocsize;
	uint8_t enable_newline;
} QS_TOKENS;

int32_t qs_inittoken( QS_MEMORY_POOL* _ppool, int32_t allocsize, size_t read_buffer_size );
int32_t qs_resize_token_buffer(QS_MEMORY_POOL* _ppool, int32_t tokens_munit, int32_t memid_currend_buffer, int tokensize);
int qs_token_analyzer( QS_MEMORY_POOL* _ppool, int32_t tokens_munit, char* pstr );
int  qs_addtoken( QS_MEMORY_POOL* _ppool, QS_TOKENS *ptokens, char* tokenbuf, int* tokensize, int type );
int  qs_check_systemword( char* token );
void qs_tokendump( QS_MEMORY_POOL* _ppool, int32_t tokens_munit );

#endif /*_QS_TOKEN_ANALYZER_H_*/

#ifdef __cplusplus
}
#endif
