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

#ifndef _GDT_VARIABLE_H_
#define _GDT_VARIABLE_H_

#include "gdt_node.h"
#include "gdt_string.h"
#include "gdt_hash.h"
#include "gdt_array.h"
#include "gdt_chain_array.h"
#include "gdt_json.h"

typedef struct GDT_CACHE
{
	GDT_MEMORY_POOL* memory;
	size_t chain_allocate_size;
	size_t page_allocate_size;
	size_t hash_size;
	size_t max_cache_size;
	size_t max_key_size;
	int32_t page;
	int32_t chain_memory;
	int32_t chain;
	int32_t memory_page1;
	int32_t page1_hash;
	int32_t memory_page2;
	int32_t page2_hash;
	int32_t swap_count;
	int8_t is_swap;
} GDT_CACHE;

typedef struct GDT_CACHE_PAGE
{
	GDT_MEMORY_POOL* memory;
	int32_t hash_id;
} GDT_CACHE_PAGE;

int32_t gdt_create_cache( GDT_MEMORY_POOL* memory,size_t chain_allocate_size,size_t max_cache_size,size_t page_allocate_size,size_t page_hash_size,size_t max_key_size);
int32_t gdt_create_storage_cache( const char* store_name, GDT_MEMORY_POOL** pp_memory,size_t max_cache_size,size_t page_allocate_size,size_t page_hash_size,size_t max_key_size);
void gdt_get_cache_page(GDT_CACHE* cache,GDT_CACHE_PAGE* page);
void gdt_swap_page(GDT_CACHE* cache,GDT_CACHE_PAGE* page);
void gdt_add_cache_key(GDT_CACHE* cache,char* key);
int32_t gdt_cache_int(GDT_CACHE* cache,char* key,int32_t value,int32_t life_time);
int32_t gdt_cache_string(GDT_CACHE* cache,char* key,char* value, int32_t life_time);
int32_t gdt_cache_binary(GDT_CACHE* cache,char* key,uint8_t* bin,size_t bin_size, int32_t life_time);
int32_t gdt_remove_cache(GDT_CACHE* cache,char* key);
int32_t gdt_cache_length(GDT_CACHE* cache);


void gdt_array_dump( GDT_MEMORY_POOL* _ppool, int32_t munit, int index );
void gdt_hash_dump( GDT_MEMORY_POOL* _ppool, int32_t h_munit, int index );
int32_t gdt_opendir( GDT_MEMORY_POOL* _ppool, const char* path );

#endif /*_GDT_VARIABLE_H_*/

#ifdef __cplusplus
}
#endif
