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

#ifndef _QS_VARIABLE_H_
#define _QS_VARIABLE_H_

#include "qs_node.h"
#include "qs_string.h"
#include "qs_hash.h"
#include "qs_array.h"
#include "qs_chain_array.h"
#include "qs_json.h"

typedef struct QS_CACHE_SERVER_DATA
{
	size_t page_size;
	size_t hash_base;
	size_t cache_base;
	size_t key_size;
	size_t hash_size;
	size_t cache_size;
	size_t chain_base;
	size_t chain_size;
	size_t json_size;
	size_t array_size;
	size_t alloc_size;
	QS_MEMORY_POOL* memory_pool;
	int32_t cache_id;
	int32_t json_element_size;
} QS_CACHE_SERVER_DATA;

typedef struct QS_CACHE
{
	QS_MEMORY_POOL* memory;
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
} QS_CACHE;

typedef struct QS_CACHE_PAGE
{
	QS_MEMORY_POOL* memory;
	int32_t hash_id;
} QS_CACHE_PAGE;

size_t qs_set_cache_alloc_info(QS_CACHE_SERVER_DATA* data, size_t page_size, size_t json_memory_size);
size_t qs_cache_alloc(QS_CACHE_SERVER_DATA* data);
int32_t qs_create_cache_B1MB( QS_MEMORY_POOL* memory);
int32_t qs_create_cache_B8MB( QS_MEMORY_POOL* memory);
int32_t qs_create_cache_B128MB( QS_MEMORY_POOL* memory);
int32_t qs_create_cache_B256MB( QS_MEMORY_POOL* memory);
int32_t qs_create_cache_B512MB( QS_MEMORY_POOL* memory);
int32_t qs_create_cache_B1GB( QS_MEMORY_POOL* memory);
int32_t qs_create_cache( QS_MEMORY_POOL* memory,size_t chain_allocate_size,size_t max_cache_size,size_t page_allocate_size,size_t page_hash_size,size_t max_key_size);
int32_t qs_create_storage_cache_B1MB( const char* store_name, QS_MEMORY_POOL** pp_memory );
int32_t qs_create_storage_cache_B8MB( const char* store_name, QS_MEMORY_POOL** pp_memory );
int32_t qs_create_storage_cache( const char* store_name, QS_MEMORY_POOL** pp_memory,size_t max_cache_size,size_t page_allocate_size,size_t page_hash_size,size_t max_key_size);
void qs_get_cache_page(QS_CACHE* cache,QS_CACHE_PAGE* page);
void qs_swap_page(QS_CACHE* cache,QS_CACHE_PAGE* page);
void qs_add_cache_key(QS_CACHE* cache,char* key);
int32_t qs_cache_int(QS_CACHE* cache,char* key,int32_t value,int32_t life_time);
int32_t qs_cache_string(QS_CACHE* cache,char* key,char* value, int32_t life_time);
int32_t qs_cache_binary(QS_CACHE* cache,char* key,uint8_t* bin,size_t bin_size, int32_t life_time);
int32_t qs_remove_cache(QS_CACHE* cache,char* key);
int32_t qs_cache_length(QS_CACHE* cache);

void qs_array_dump( QS_MEMORY_POOL* _ppool, int32_t munit, int index );
void qs_hash_dump( QS_MEMORY_POOL* _ppool, int32_t h_munit, int index );
int32_t qs_opendir( QS_MEMORY_POOL* _ppool, const char* path );

#endif /*_QS_VARIABLE_H_*/

#ifdef __cplusplus
}
#endif
