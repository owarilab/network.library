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

#ifndef _QS_JSON_H_
#define _QS_JSON_H_

#include "qs_core.h"
#include "qs_hash.h"
#include "qs_array.h"
#include "qs_node.h"
#include "qs_token_analyzer.h"
#include "qs_string.h"
#include <stdio.h>
#include <string.h>

int32_t gdt_json_encode( QS_MEMORY_POOL* _ppool, QS_NODE* node, size_t buf_size );
int32_t gdt_json_encode_b( QS_MEMORY_POOL* _ppool, QS_NODE* node, int32_t buf_munit );
int32_t gdt_json_encode_parser_hash( QS_MEMORY_POOL* _ppool, int32_t buf_munit, int32_t h_munit );
int32_t gdt_json_encode_parser_array( QS_MEMORY_POOL* _ppool, int32_t buf_munit, int32_t a_munit );
int gdt_add_json_element( QS_MEMORY_POOL* _ppool, int32_t buf_munit, char* src, size_t src_size, uint8_t escape );
QS_NODE* gdt_get_json_root( QS_MEMORY_POOL* _ppool, int32_t json_root_munit );
int32_t gdt_get_json_root_hash(QS_MEMORY_POOL* _ppool, int32_t json_root_munit);
int32_t gdt_json_decode( QS_MEMORY_POOL* _ppool, const char* src );
int32_t gdt_json_decode_h( QS_MEMORY_POOL* _ppool, const char* src, int32_t hash_size, int32_t init_token_size );
int32_t gdt_json_decode_parser( QS_MEMORY_POOL* _ppool, QS_NODE* node, QS_TOKENS *ptokens, QS_TOKEN *token_list, int32_t hash_size );
int32_t gdt_json_decode_parser_hash( QS_MEMORY_POOL* _ppool, QS_NODE* node, QS_TOKENS *ptokens, QS_TOKEN *token_list, int32_t hash_size );
int32_t gdt_json_decode_parser_array( QS_MEMORY_POOL* _ppool, QS_NODE* node, QS_TOKENS *ptokens, QS_TOKEN *token_list );
int32_t gdt_make_json_root( QS_MEMORY_POOL* _ppool, int32_t data_munit, int id ); 

#endif /*_QS_JSON_H_*/

#ifdef __cplusplus
}
#endif
