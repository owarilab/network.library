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

#ifdef __cplusplus
extern "C"{
#endif

#ifndef _GDT_TOKEN_ANALYZER_H_
#define _GDT_TOKEN_ANALYZER_H_

#include "gdt_core.h"
#include "gdt_system.h"
#include "gdt_array.h"
#include "gdt_io.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define STRBUF_SIZE			4096	// max token size
#define ID_UNK				0		// 不明
#define ID_NUM				1		// 数値
#define ID_OP				2		// 演算子
#define ID_SIGN				3		// 記号
#define ID_STR				4		// 文字列
#define ID_SYMBOL			5		// 変数
#define ID_SYS_IF			6		// 予約文字( if )
#define ID_SYS_ELSE			7		// 予約文字( else )
#define ID_SYS_ELSEIF		8		// 予約文字( elseif )
#define ID_FLOAT			9		// 少数
#define ID_SYS_LOOP			10		// 予約文字( loop )
#define ID_SYS_WHILE		11		// 予約文字( while )

// system words 
#define SYS_LOOP    "loop"		// loop文
#define SYS_WHILE   "while"		// while文
#define SYS_IF      "if"		// if文
#define SYS_ELSEIF  "elseif"	// elseif文
#define SYS_ELSE    "else"		// else文

typedef struct GDT_TOKEN
{
	int32_t buf_munit;		// トークン文字列
	int type;				// トークンの種類
	size_t size;			// 文字サイズ
} GDT_TOKEN;

typedef struct GDT_TOKENS
{
	int32_t token_munit;	// トークン格納場所
	size_t size;			// token_munitに確保した配列サイズ
	uint32_t currentpos;	// 現在の配列番地
} GDT_TOKENS;

int gdt_token_analyzer( GDT_MEMORY_POOL* _ppool, int32_t tokens_munit, char* pstr );
int  gdt_addtoken( GDT_MEMORY_POOL* _ppool, int32_t tokens_munit, char* tokenbuf, int* tokensize, int type );
int  gdt_check_systemword( char* token );
void gdt_tokendump( GDT_MEMORY_POOL* _ppool, int32_t tokens_munit );

#endif /*_GDT_TOKEN_ANALYZER_H_*/

#ifdef __cplusplus
}
#endif
