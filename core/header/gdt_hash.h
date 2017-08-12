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

#ifndef _GDT_HASH_H_
#define _GDT_HASH_H_

#include "gdt_core.h"
#include "gdt_memory_allocator.h"
#include "gdt_string.h"
#include "gdt_node.h"
#include "gdt_array.h"
#if defined(__LINUX__) || defined(__BSD_UNIX__) || defined(__ANDROID__) || defined(__IOS__)
#include <pthread.h>
#endif
typedef struct GDT_HASH
{
	size_t hash_size;
	int32_t hash_munit;
} GDT_HASH;

typedef struct GDT_HASH_ELEMENT
{
	int32_t id;
	int32_t hashname_munit;
	int32_t elm_munit;
} GDT_HASH_ELEMENT;

int32_t gdt_create_hash( GDT_MEMORY_POOL* _ppool, size_t hlen );
int32_t gdt_add_hash( GDT_MEMORY_POOL* _ppool, int32_t h_munit, int32_t name_munit, int32_t data_munit, int32_t id );
int32_t gdt_make_hash_name( GDT_MEMORY_POOL* _ppool, int32_t h_munit,char* name);
void gdt_add_hash_array_string( GDT_MEMORY_POOL* _ppool, int32_t h_munit, char* name, char* value );
void gdt_add_hash_array_empty_string( GDT_MEMORY_POOL* _ppool, int32_t h_munit, char* name, size_t size );
void gdt_add_hash_binary( GDT_MEMORY_POOL* _ppool, int32_t h_munit, char* name, uint8_t* binary, size_t size );
void gdt_add_hash_value( GDT_MEMORY_POOL* _ppool, int32_t h_munit, char* name, char* value, int32_t id );
void gdt_add_hash_integer( GDT_MEMORY_POOL* _ppool, int32_t h_munit, char* name, int32_t value );
void gdt_add_hash_integer_kint( GDT_MEMORY_POOL* _ppool, int32_t h_munit, int32_t name_munit, int32_t value );
void gdt_add_hash_string( GDT_MEMORY_POOL* _ppool, int32_t h_munit, char* name, char* value );
void gdt_add_hash_value_kstring( GDT_MEMORY_POOL* _ppool, int32_t h_munit, char* name, int32_t data_munit, int32_t id );
void gdt_add_hash_emptystring( GDT_MEMORY_POOL* _ppool, int32_t h_munit, char* name, size_t string_size );
int32_t gdt_get_hash( GDT_MEMORY_POOL* _ppool, int32_t h_munit, char* hashname );
int32_t gdt_get_hash_fix_ihash( GDT_MEMORY_POOL* _ppool, int32_t h_munit, char* hashname, uint32_t hashkey );
int32_t gdt_get_hash_core( GDT_MEMORY_POOL* _ppool, struct GDT_HASH *hash, GDT_HASH *hashchild, char* hashname, uint32_t hashkey );
void gdt_clear_hash_string( GDT_MEMORY_POOL* _ppool, int32_t h_munit, char* name );
int32_t gdt_replace_hash_string( GDT_MEMORY_POOL* _ppool, int32_t h_munit, char* name, char* value );
int32_t gdt_get_hash_name( GDT_MEMORY_POOL* _ppool, int32_t h_munit, char* hashname );
int32_t gdt_get_hash_id( GDT_MEMORY_POOL* _ppool, int32_t h_munit, char* hashname );
void gdt_dump_hash( GDT_MEMORY_POOL* _ppool, int32_t h_munit, int index );
int32_t gdt_hash_length( GDT_MEMORY_POOL* _ppool, int32_t h_munit );

#endif /*_GDT_HASH_H_*/

#ifdef __cplusplus
}
#endif
