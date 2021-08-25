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

#ifndef _QS_HASH_H_
#define _QS_HASH_H_

#include "qs_core.h"
#include "qs_type.h"
#include "qs_array.h"
#include "qs_memory_allocator.h"
#include "qs_string.h"
#if defined(__LINUX__) || defined(__BSD_UNIX__) || defined(__ANDROID__) || defined(__IOS__)
#include <pthread.h>
#endif
typedef struct QS_HASH
{
	size_t hash_size;
	int32_t hash_munit;
} QS_HASH;

typedef struct QS_HASH_ELEMENT
{
	int32_t id;
	int32_t hashname_munit;
	int32_t elm_munit;
	time_t create_time;
	int32_t life_time;
} QS_HASH_ELEMENT;

typedef struct QS_HASH_FOREACH
{
	int32_t p1;
	int32_t p2;
	QS_HASH *root;
	QS_HASH *child;
	QS_HASH_ELEMENT* he;
} QS_HASH_FOREACH;

#define QS_HASH_ELEMENT_SIZE 2
#define QS_HASH_ELEMENT_RESIZE_QUANTITY 4
#define QS_HASH_ELEMENT_ARRAY_SIZE 8

int32_t qs_create_hash( QS_MEMORY_POOL* _ppool, size_t hlen );
QS_HASH_ELEMENT* qs_add_hash( QS_MEMORY_POOL* _ppool, int32_t h_munit, int32_t name_munit, int32_t data_munit, int32_t id );
int32_t qs_make_hash_name( QS_MEMORY_POOL* _ppool, int32_t h_munit,const char* name);
void qs_add_hash_hash(QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* name, int32_t hash_id);
void qs_add_hash_array(QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* name, int32_t array_id);
void qs_add_hash_array_string( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* name, const char* value );
void qs_add_hash_array_empty_string( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* name, size_t size );
QS_HASH_ELEMENT* qs_add_hash_binary( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* name, uint8_t* binary, size_t size );
void qs_add_hash_value( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* name, const char* value, int32_t id );
QS_HASH_ELEMENT* qs_add_hash_integer( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* name, int32_t value );
void qs_add_hash_integer_kint( QS_MEMORY_POOL* _ppool, int32_t h_munit, int32_t name_munit, int32_t value );
QS_HASH_ELEMENT* qs_add_hash_string( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* name, const char* value );
void qs_add_hash_value_kstring( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* name, int32_t data_munit, int32_t id );
void qs_add_hash_emptystring( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* name, size_t string_size );
int32_t qs_move_hash( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* hashname_from, const char* hashname_to );
int32_t qs_remove_hash( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* hashname );
char* qs_get_hash_string( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* hashname );
int32_t* qs_get_hash_integer( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* hashname );
int32_t qs_get_hash( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* hashname );
int32_t qs_get_hash_fix_ihash( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* hashname, uint32_t hashkey );
QS_HASH_ELEMENT* qs_get_hash_core( QS_MEMORY_POOL* _ppool, struct QS_HASH *hash, QS_HASH *hashchild, const char* hashname, uint32_t hashkey );
void qs_clear_hash_string( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* name );
int32_t qs_replace_hash_string( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* name, const char* value );
int32_t qs_get_hash_name( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* hashname );
int32_t qs_get_hash_id( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* hashname );
QS_HASH_ELEMENT* qs_get_hash_element( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* hashname );
int32_t qs_hash_length( QS_MEMORY_POOL* _ppool, int32_t h_munit );
int32_t qs_init_hash_foreach( QS_MEMORY_POOL* _ppool, int32_t h_munit, QS_HASH_FOREACH* hf );
QS_HASH_ELEMENT* qs_hash_foreach( QS_MEMORY_POOL* _ppool, QS_HASH_FOREACH* hf );

#endif /*_QS_HASH_H_*/

#ifdef __cplusplus
}
#endif
