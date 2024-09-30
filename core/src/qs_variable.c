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

#include "qs_variable.h"

size_t qs_set_cache_alloc_info(QS_CACHE_SERVER_DATA* data, size_t page_size, size_t json_memory_size)
{
	data->page_size = page_size;
	data->key_size = 128;
	data->hash_base = (data->page_size / (SIZE_KBYTE * 100));
	if (data->hash_base <= 0) {
		data->hash_base = 1;
	}
	data->cache_base = (data->page_size / (SIZE_KBYTE * 10));
	if (data->cache_base <= 0) {
		data->cache_base = 1;
	}
	data->hash_size = 8 * data->hash_base;
	data->cache_size = 8 * data->cache_base;
	data->chain_base = (data->cache_size / 5000);
	if (data->chain_base <= 0) {
		data->chain_base = 1;
	}
	data->chain_size = SIZE_MBYTE * data->chain_base;
	data->array_size = SIZE_MBYTE * data->chain_base * 5;
	data->alloc_size = data->chain_size + (data->page_size * 2) + SIZE_MBYTE;
	data->json_element_size = json_memory_size / (data->key_size * 8);
	return data->alloc_size;
}

size_t qs_cache_alloc(QS_CACHE_SERVER_DATA* data)
{
	size_t size;
	if( (size = qs_initialize_memory_f64( &data->memory_pool, data->alloc_size) ) <= 0 ){
		return -1;
	}
	if(-1==(data->cache_id = qs_create_cache(data->memory_pool, data->chain_size,data->cache_size, data->page_size,data->hash_size,data->key_size))){
		qs_free(data->memory_pool);
		return -1;
	}
	return size;
}

int32_t qs_create_cache_B1MB( QS_MEMORY_POOL* memory)
{
	size_t key_size = 64;
	size_t hash_size = 128;
	size_t cache_size = 700;
	size_t chain_allocate_size = cache_size*(key_size+20);
	size_t page_allocate_size = (qs_memory_available_size(memory) - (chain_allocate_size + SIZE_KBYTE * 8)) / 2;
	int32_t cache_id = qs_create_cache(memory,chain_allocate_size,cache_size,page_allocate_size,hash_size,key_size);
	if(-1==cache_id){
		printf("qs_create_cache error\n");
		return -1;
	}
	return cache_id;
}

int32_t qs_create_cache_B128MB( QS_MEMORY_POOL* memory)
{
	size_t key_size = 64;
	size_t hash_size = 1000000;
	size_t cache_size = 50000;
	size_t chain_allocate_size = cache_size*(key_size+20);
	size_t page_allocate_size = (qs_memory_available_size(memory) - (chain_allocate_size + SIZE_KBYTE * 8)) / 2;
	int32_t cache_id = qs_create_cache(memory,chain_allocate_size,cache_size,page_allocate_size,hash_size,key_size);
	if(-1==cache_id){
		printf("qs_create_cache error\n");
		return -1;
	}
	return cache_id;
}

int32_t qs_create_cache_B256MB( QS_MEMORY_POOL* memory)
{
	size_t key_size = 128;
	size_t hash_size = 1500000;
	size_t cache_size = 50000;
	size_t chain_allocate_size = cache_size*(key_size+20);
	size_t page_allocate_size = (qs_memory_available_size(memory) - (chain_allocate_size + SIZE_KBYTE * 8)) / 2;
	int32_t cache_id = qs_create_cache(memory,chain_allocate_size,cache_size,page_allocate_size,hash_size,key_size);
	if(-1==cache_id){
		printf("qs_create_cache error\n");
		return -1;
	}
	return cache_id;
}

int32_t qs_create_cache_B512MB( QS_MEMORY_POOL* memory)
{
	size_t key_size = 128;
	size_t hash_size = 1500000;
	size_t cache_size = 100000;
	size_t chain_allocate_size = cache_size*(key_size+20);
	size_t page_allocate_size = (qs_memory_available_size(memory) - (chain_allocate_size + SIZE_KBYTE * 8)) / 2;
	int32_t cache_id = qs_create_cache(memory,chain_allocate_size,cache_size,page_allocate_size,hash_size,key_size);
	if(-1==cache_id){
		printf("qs_create_cache error\n");
		return -1;
	}
	return cache_id;
}

int32_t qs_create_cache_B1GB( QS_MEMORY_POOL* memory)
{
	size_t key_size   = 256;
	size_t hash_size  = 5000000;
	size_t cache_size = 100000;
	size_t chain_allocate_size = cache_size*(key_size+20);
	size_t page_allocate_size = (qs_memory_available_size(memory) - (chain_allocate_size + SIZE_KBYTE * 8)) / 2;
	int32_t cache_id = qs_create_cache(memory,chain_allocate_size,cache_size,page_allocate_size,hash_size,key_size);
	if(-1==cache_id){
		printf("qs_create_cache error\n");
		return -1;
	}
	return cache_id;
}

int32_t qs_create_cache( QS_MEMORY_POOL* memory,size_t chain_allocate_size,size_t max_cache_size,size_t page_allocate_size,size_t page_hash_size,size_t max_key_size)
{
	int32_t cache_id = qs_create_memory_block(memory,sizeof(QS_CACHE));
	if(cache_id==-1){
		return -1;
	}
	QS_CACHE* cache = (QS_CACHE*)QS_GET_POINTER(memory,cache_id);
	cache->memory = memory;
	cache->page_allocate_size = page_allocate_size;
	cache->chain_allocate_size = chain_allocate_size;
	cache->max_cache_size = max_cache_size;
	cache->max_key_size = max_key_size;
	cache->hash_size = page_hash_size;
	cache->page = 0;
	cache->swap_count = 0;
	cache->is_swap = 1;
	cache->chain_memory = qs_create_mini_memory(cache->memory,cache->chain_allocate_size);
	if(-1==cache->chain_memory){
		return -1;
	}
	QS_MEMORY_POOL* chain_memory = (QS_MEMORY_POOL*)QS_GET_POINTER(cache->memory,cache->chain_memory);
	qs_memory_clean(chain_memory);
	cache->chain = qs_create_chain_array(chain_memory,cache->max_cache_size,sizeof(int8_t)*cache->max_key_size);
	if(-1==cache->chain){
		return -1;
	}
	cache->memory_page1 = qs_create_mini_memory(cache->memory,cache->page_allocate_size);
	if(-1==cache->memory_page1){
		return -1;
	}
	cache->memory_page2 = qs_create_mini_memory(cache->memory,cache->page_allocate_size);
	if(-1==cache->memory_page2){
		return -1;
	}
	QS_MEMORY_POOL* page1 = (QS_MEMORY_POOL*)QS_GET_POINTER(cache->memory,cache->memory_page1);
	QS_MEMORY_POOL* page2 = (QS_MEMORY_POOL*)QS_GET_POINTER(cache->memory,cache->memory_page2);
	qs_memory_clean(page1);
	qs_memory_clean(page2);
	cache->page1_hash = qs_create_hash(page1,cache->hash_size);
	if(-1==cache->page1_hash){
		return -1;
	}
	cache->page2_hash = qs_create_hash(page2,cache->hash_size);
	if(-1==cache->page2_hash){
		return -1;
	}
	return cache_id;
}

int32_t qs_create_storage_cache_B1MB( const char* store_name, QS_MEMORY_POOL** pp_memory )
{
	size_t key_size = 64;
	size_t hash_size = 1000;
	size_t cache_size = 1;
	size_t page_allocate_size = SIZE_KBYTE*980;
	int32_t cache_id = qs_create_storage_cache(store_name,pp_memory,cache_size,page_allocate_size,hash_size,key_size);
	if(-1==cache_id){
		printf("qs_create_cache error\n");
		return -1;
	}
	return cache_id;
}

int32_t qs_create_storage_cache( const char* store_name, QS_MEMORY_POOL** pp_memory,size_t max_cache_size,size_t page_allocate_size,size_t page_hash_size,size_t max_key_size)
{
	int is_swap = 0;
	size_t page_size = (is_swap==1) ? (page_allocate_size * 2) : page_allocate_size;
	size_t chain_allocate_size = (max_cache_size * (max_key_size + 8)) + SIZE_KBYTE;
	size_t allocate_size = chain_allocate_size + page_size + SIZE_KBYTE * 4;
	if(0>=qs_initialize_mmapmemory_f( store_name, pp_memory, allocate_size )){
		printf("qs_initialize_mmapmemory_f error\n");
		return -1;
	}
	QS_MEMORY_POOL* memory = *pp_memory;
	int32_t cache_id = -1;
	if(memory->memid_save_data == -1){
		cache_id = qs_create_memory_block(memory,sizeof(QS_CACHE));
		if(cache_id==-1){
			printf("create cache error\n");
			return -1;
		}
		QS_CACHE* cache = (QS_CACHE*)QS_GET_POINTER(memory,cache_id);
		cache->memory = memory;
		cache->page_allocate_size = page_allocate_size;
		cache->chain_allocate_size = chain_allocate_size;
		cache->max_cache_size = max_cache_size;
		cache->max_key_size = max_key_size;
		cache->hash_size = page_hash_size;
		cache->page = 0;
		cache->swap_count = 0;
		cache->is_swap = is_swap;
		cache->chain_memory = qs_create_mini_memory(cache->memory,cache->chain_allocate_size);
		if(-1==cache->chain_memory){
			printf("qs_create_mini_memory error\n");
			return -1;
		}
		QS_MEMORY_POOL* chain_memory = (QS_MEMORY_POOL*)QS_GET_POINTER(cache->memory,cache->chain_memory);
		qs_memory_clean(chain_memory);
		cache->chain = qs_create_chain_array(chain_memory,cache->max_cache_size,sizeof(int8_t)*cache->max_key_size);
		if(-1==cache->chain){
			printf("qs_create_chain_array error\n");
			return -1;
		}
		cache->memory_page1 = qs_create_mini_memory(cache->memory,cache->page_allocate_size);
		if(-1==cache->memory_page1){
			printf("qs_create_mini_memory page1 error\n");
			return -1;
		}
		QS_MEMORY_POOL* page1 = (QS_MEMORY_POOL*)QS_GET_POINTER(cache->memory,cache->memory_page1);
		qs_memory_clean(page1);
		cache->page1_hash = qs_create_hash(page1,cache->hash_size);
		if(-1==cache->page1_hash){
			printf("page1_hash error\n");
			return -1;
		}
		if(cache->is_swap==1){
			cache->memory_page2 = qs_create_mini_memory(cache->memory,cache->page_allocate_size);
			if(-1==cache->memory_page2){
				printf("qs_create_mini_memory page2 error\n");
				return -1;
			}
			QS_MEMORY_POOL* page2 = (QS_MEMORY_POOL*)QS_GET_POINTER(cache->memory,cache->memory_page2);
			qs_memory_clean(page2);
			cache->page2_hash = qs_create_hash(page2,cache->hash_size);
			if(-1==cache->page2_hash){
				printf("page2_hash error\n");
				return -1;
			}
		}
		memory->memid_save_data = cache_id;
		qs_sync_mmap_memory(memory);
	} else{
		cache_id = memory->memid_save_data;
		QS_CACHE* cache = (QS_CACHE*)QS_GET_POINTER(memory,cache_id);
		cache->memory = memory;
		QS_MEMORY_POOL* page1 = (QS_MEMORY_POOL*)QS_GET_POINTER(cache->memory,cache->memory_page1);
		page1->memory = ( void* )QS_GET_POINTER( memory, page1->memid_save_data );
		if(cache->is_swap==1){
			QS_MEMORY_POOL* page2 = (QS_MEMORY_POOL*)QS_GET_POINTER(cache->memory,cache->memory_page2);
			page2->memory = ( void* )QS_GET_POINTER( memory, page2->memid_save_data );
		}
	}
	return cache_id;
}

void qs_get_cache_page(QS_CACHE* cache,QS_CACHE_PAGE* page)
{
	if(cache->page==0){
		page->memory = (QS_MEMORY_POOL*)QS_GET_POINTER(cache->memory,cache->memory_page1);
		page->hash_id = cache->page1_hash;
	} else {
		page->memory = (QS_MEMORY_POOL*)QS_GET_POINTER(cache->memory,cache->memory_page2);
		page->hash_id = cache->page2_hash;
	}
}

void qs_swap_page(QS_CACHE* cache,QS_CACHE_PAGE* page)
{
	if(cache->is_swap==0){
		return;
	}
	QS_MEMORY_POOL* chain_memory = (QS_MEMORY_POOL*)QS_GET_POINTER(cache->memory,cache->chain_memory);
	QS_MEMORY_POOL* current_page;
	int32_t current_hash_id;
	QS_MEMORY_POOL* backup_page;
	int32_t backup_hash_id;
	if(cache->page==0){
		current_page = (QS_MEMORY_POOL*)QS_GET_POINTER(cache->memory,cache->memory_page1);
		current_hash_id = cache->page1_hash;
		backup_page = (QS_MEMORY_POOL*)QS_GET_POINTER(cache->memory,cache->memory_page2);
		qs_memory_clean(backup_page);
		cache->page2_hash = qs_create_hash(backup_page,cache->hash_size);
		backup_hash_id = cache->page2_hash;
	} else {
		backup_page = (QS_MEMORY_POOL*)QS_GET_POINTER(cache->memory,cache->memory_page1);
		qs_memory_clean(backup_page);
		cache->page1_hash = qs_create_hash(backup_page,cache->hash_size);
		backup_hash_id = cache->page1_hash;
		current_page = (QS_MEMORY_POOL*)QS_GET_POINTER(cache->memory,cache->memory_page2);
		current_hash_id = cache->page2_hash;
	}
	void* current=NULL;
	while( NULL != (current=qs_get_chain(chain_memory,cache->chain,current)) ){
		uint8_t* chain = (uint8_t*)current;
		char* key = (char*)(chain);
		QS_HASH_ELEMENT* elm = qs_get_hash_element(current_page,current_hash_id,key);
		if(elm!=NULL){
			if(elm->id==ELEMENT_LITERAL_NUM){
				int32_t value = QS_INT32(current_page, elm->memid_hash_element_data);
				if(NULL==qs_add_hash_integer( backup_page, backup_hash_id, key, value )){
					// error
				}
				//printf("ELEMENT_LITERAL_NUM k = %s, h = %d, id = %d, value = %d\n",key,elm->memid_hash_element_data,elm->id,value);
			} else if( elm->id==ELEMENT_LITERAL_STR ){
				char* value = (char*)QS_GET_POINTER(current_page, elm->memid_hash_element_data);
				if(NULL==qs_add_hash_string( backup_page, backup_hash_id, key, value )){
					// error
				}
			} else {
				printf("other hash k = %s, h = %d, id = %d\n",key,elm->memid_hash_element_data,elm->id);
			}
		}
		if(qs_memory_available_size(backup_page) < backup_page->size / 4){
			break;
		}
	}
	page->memory = backup_page;
	page->hash_id = backup_hash_id;
	cache->page = !cache->page;
	cache->swap_count++;
}

void qs_add_cache_key(QS_CACHE* cache,char* key)
{
	QS_MEMORY_POOL* chain_memory = (QS_MEMORY_POOL*)QS_GET_POINTER(cache->memory,cache->chain_memory);
	char* pstr = (char*)qs_add_chain(chain_memory,cache->chain);
	if(pstr!=NULL){
		memcpy(pstr,key,qs_strlen(key));
	} else {
		void* c = qs_get_chain(chain_memory,cache->chain,NULL);
		if(c!=NULL){
			qs_remove_chain(chain_memory,cache->chain,c);
			pstr = (char*)qs_add_chain(chain_memory,cache->chain);
			if(pstr!=NULL){
				memcpy(pstr,key,qs_strlen(key));
			}
		}
	}
}

int32_t qs_cache_int(QS_CACHE* cache,char* key,int32_t value, int32_t life_time)
{
	QS_CACHE_PAGE cache_page;
	qs_get_cache_page(cache,&cache_page);
	QS_HASH_ELEMENT* elm = NULL;
	if(NULL==(elm=qs_add_hash_integer( cache_page.memory, cache_page.hash_id, key, value ))){
		qs_swap_page(cache,&cache_page);
		if(NULL==(elm=qs_add_hash_integer( cache_page.memory, cache_page.hash_id, key, value ))){
			//printf("[qs_cache_int] qs_add_hash_integer error\n");
			return QS_SYSTEM_ERROR;
		}
	}
	elm->create_time = time(NULL);
	elm->life_time = life_time;
	if(cache->is_swap==1){
		qs_add_cache_key(cache,key);
	}
	return QS_SYSTEM_OK;
}

int32_t qs_cache_string(QS_CACHE* cache,char* key,char* value, int32_t life_time)
{
	QS_CACHE_PAGE cache_page;
	qs_get_cache_page(cache,&cache_page);
	QS_HASH_ELEMENT* elm = NULL;
	if(NULL==(elm=qs_add_hash_string( cache_page.memory, cache_page.hash_id, key, value ))){
		qs_swap_page(cache,&cache_page);
		if(NULL==(elm=qs_add_hash_string( cache_page.memory, cache_page.hash_id, key, value ))){
			//printf("[qs_cache_string] qs_add_hash_string error\n");
			return QS_SYSTEM_ERROR;
		}
	}
	elm->create_time = time(NULL);
	elm->life_time = life_time;
	if(cache->is_swap==1){
		qs_add_cache_key(cache,key);
	}
	return QS_SYSTEM_OK;
}

int32_t qs_cache_binary(QS_CACHE* cache,char* key,uint8_t* bin,size_t bin_size, int32_t life_time)
{
	QS_CACHE_PAGE cache_page;
	qs_get_cache_page(cache,&cache_page);
	QS_HASH_ELEMENT* elm = NULL;
	if(NULL==(elm=qs_add_hash_binary( cache_page.memory, cache_page.hash_id, key, bin, bin_size ))){
		qs_swap_page(cache,&cache_page);
		if(NULL==(elm=qs_add_hash_binary( cache_page.memory, cache_page.hash_id, key, bin, bin_size ))){
			return QS_SYSTEM_ERROR;
		}
	}
	elm->create_time = time(NULL);
	elm->life_time = life_time;
	if(cache->is_swap==1){
		qs_add_cache_key(cache,key);
	}
	return QS_SYSTEM_OK;
}

int32_t qs_remove_cache(QS_CACHE* cache,char* key)
{
	QS_CACHE_PAGE cache_page;
	qs_get_cache_page(cache,&cache_page);
	return qs_remove_hash(cache_page.memory,cache_page.hash_id,key);
}

int32_t qs_cache_length(QS_CACHE* cache)
{
	QS_CACHE_PAGE cache_page;
	qs_get_cache_page(cache,&cache_page);
	return qs_hash_length(cache_page.memory,cache_page.hash_id);
}

void qs_array_dump( QS_MEMORY_POOL* _ppool, int32_t munit, int index )
{
	QS_ARRAY* parray;
	QS_ARRAY_ELEMENT* elm;
	int i,k;
	if( -1 == munit ){
		return;
	}
	//if(1){for( k=0;k<index;k++ ){ printf("  ");}}
	parray = (QS_ARRAY*)QS_GET_POINTER( _ppool, munit );
	if( -1 == parray->memid){
		return;
	}
	elm = (QS_ARRAY_ELEMENT*)QS_GET_POINTER( _ppool, parray->memid );
	printf("[\n");
	for( i = 0; i < parray->len; i++ )
	{
		if( (elm+i)->id == ELEMENT_LITERAL_NUM ){
			if( i > 0 ){ printf(",\n"); }
			for( k=0;k<index+1;k++ ){ printf("  "); }
			printf("%s",(char*)QS_GET_POINTER(_ppool,(elm+i)->memid_array_element_data));
		}
		else if( (elm+i)->id == ELEMENT_LITERAL_NUM_64 ){
			if( i > 0 ){ printf(",\n"); }
			for( k=0;k<index+1;k++ ){ printf("  "); }
			printf("%s",(char*)QS_GET_POINTER(_ppool,(elm+i)->memid_array_element_data));
		}
		else if( (elm+i)->id == ELEMENT_LITERAL_NUM_U64 ){
			if( i > 0 ){ printf(",\n"); }
			for( k=0;k<index+1;k++ ){ printf("  "); }
			printf("%s",(char*)QS_GET_POINTER(_ppool,(elm+i)->memid_array_element_data));
		}
		else if( (elm+i)->id == ELEMENT_LITERAL_STR ){
			if( i > 0 ){ printf(",\n"); }
			for( k=0;k<index+1;k++ ){ printf("  "); }
			printf("\"%s\"",(char*)QS_GET_POINTER(_ppool,(elm+i)->memid_array_element_data));
		}
		else if( (elm+i)->id == ELEMENT_ARRAY ){
			if( i > 0 ){ printf(",\n"); }
			for( k=0;k<index+1;k++ ){ printf("  ");}
			qs_array_dump( _ppool, (elm+i)->memid_array_element_data, index+1 );
		}
		else if( (elm+i)->id==ELEMENT_HASH){
			if( i > 0 ){ printf(",\n"); }
			qs_hash_dump( _ppool, (elm+i)->memid_array_element_data, index+1 );
		}
	}
	printf("\n");
	for( k=0;k<index;k++ ){ printf("  "); }
	printf("]\n");
}

void qs_hash_dump( QS_MEMORY_POOL* _ppool, int32_t h_munit, int index )
{
	struct QS_HASH *hash;
	struct QS_HASH *hashchild;
	struct QS_HASH_ELEMENT *hashelement;
	uint32_t i,j,k;
	uint32_t cnt = 0;
	uint32_t hashlength = 0;
	hash = (struct QS_HASH *)QS_GET_POINTER( _ppool, h_munit );
	hashchild = (struct QS_HASH *)QS_GET_POINTER( _ppool, hash->hash_munit );
	if(index==1){ for( k=0;k<index;k++ ){ printf("  "); } }
	printf("{\n");
	for( j = 0; j < hash->hash_size; j++ ){
		if( hashchild[j].hash_munit >= 0 )
		{
			hashelement = (struct QS_HASH_ELEMENT*)QS_GET_POINTER( _ppool, hashchild[j].hash_munit );
			for( i = 0; i < hashchild[j].hash_size; i++ )
			{
				if( hashelement[i].memid_hash_name >= 0 )
				{
					if( hashelement[i].id==ELEMENT_HASH){
						if( cnt > 0 ){ printf(",\n"); }
						for( k=0;k<index+1;k++ ){ printf("  "); }
						printf( "\"%s\":"
								, (char*)QS_GET_POINTER( _ppool, hashelement[i].memid_hash_name )
							  );
						qs_hash_dump( _ppool, hashelement[i].memid_hash_element_data, index+1 );
						cnt++;
					}
					else if( hashelement[i].id==ELEMENT_ARRAY){
						if( cnt > 0 ){ printf(",\n"); }
						for( k=0;k<index+1;k++ ){ printf("  "); }
						printf( "\"%s\":"
								, (char*)QS_GET_POINTER( _ppool, hashelement[i].memid_hash_name )
							  );
						qs_array_dump( _ppool, hashelement[i].memid_hash_element_data, index+1 );
						cnt++;
					}
					else{
						if( cnt > 0 ){ printf(",\n"); }
						for( k=0;k<index+1;k++ ){ printf("  "); }
						if( hashelement[i].id == ELEMENT_LITERAL_STR ){
							printf( "\"%s\":\"%s\""
									, (char*)QS_GET_POINTER( _ppool, hashelement[i].memid_hash_name )
									, (char*)QS_GET_POINTER( _ppool, hashelement[i].memid_hash_element_data )
								  );
							cnt++;
						}
						else if( hashelement[i].id == ELEMENT_LITERAL_NUM ){
							printf( "\"%s\":%s"
									, (char*)QS_GET_POINTER( _ppool, hashelement[i].memid_hash_name )
									, (char*)QS_GET_POINTER( _ppool, hashelement[i].memid_hash_element_data )
								  );
							cnt++;
						}
						else if( hashelement[i].id == ELEMENT_LITERAL_NUM_64 ){
							printf( "\"%s\":%s"
									, (char*)QS_GET_POINTER( _ppool, hashelement[i].memid_hash_name )
									, (char*)QS_GET_POINTER( _ppool, hashelement[i].memid_hash_element_data )
								  );
							cnt++;
						}
						else if( hashelement[i].id == ELEMENT_LITERAL_NUM_U64 ){
							printf( "\"%s\":%s"
									, (char*)QS_GET_POINTER( _ppool, hashelement[i].memid_hash_name )
									, (char*)QS_GET_POINTER( _ppool, hashelement[i].memid_hash_element_data )
								  );
							cnt++;
						}
					}
				}
			}
			++hashlength;
		}
	}
	printf("\n");
	for( k=0;k<index;k++ ){ printf("  "); }
	printf("}\n");
}

int32_t qs_opendir( QS_MEMORY_POOL* _ppool, const char* path )
{
	char tmppath[MAXPATHLEN];
	char* pathstart = tmppath;
	memcpy( pathstart, path, strlen(path) );
	pathstart += strlen(path);
	*pathstart = '\0';
	struct stat st;
#ifdef __WINDOWS__
	WIN32_FIND_DATA fd;
	HANDLE handle;
	char tmpstr[MAXPATHLEN];
	memset(tmpstr, 0, sizeof(tmpstr));
	memcpy(tmpstr, path, strlen(path));
	*(tmpstr + strlen(path)) = '*';
	if (INVALID_HANDLE_VALUE == (handle = FindFirstFileEx(tmpstr, FindExInfoStandard, &fd, FindExSearchNameMatch, NULL, 0)))
	{
		printf("error : FindFirstFileEx %s\n",tmpstr);
		return -1;
	}
#else
	DIR *dir;
	struct dirent *dent;
	if (NULL == (dir = opendir(path)))
	{
		printf("error : opendir\n");
		return -1;
	}
	dent = readdir(dir);
#endif
	int32_t path_array_munit = qs_create_array( _ppool, 128 );
	do{
#ifdef __WINDOWS__
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (fd.cFileName[0] == '.') {
				if (fd.cFileName[1] == '\0' || fd.cFileName[1] == '.') {
					continue;
				}
			}
		}
		memcpy(pathstart, fd.cFileName, strlen(fd.cFileName));
		*(pathstart + strlen(fd.cFileName)) = '\0';
		if( stat( tmppath, &st ) < 0 )
#else
		if (dent->d_name[0] == '.') {
			if (dent->d_name[1] == '\0' || dent->d_name[1] == '.') {
				continue;
			}
		}
		memcpy(pathstart, dent->d_name, strlen(dent->d_name));
		*(pathstart + strlen(dent->d_name)) = '\0';
		if( lstat( tmppath, &st ) < 0 )
#endif
		{
#ifdef __WINDOWS__
			FindClose(handle);
#else
			closedir(dir);
#endif
			printf("error : lstat %s\n",tmppath);
			return -1;
		}
#ifdef __WINDOWS__
		// directory
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			*(pathstart + strlen(fd.cFileName)) = '/';
			*(pathstart + strlen(fd.cFileName) + 1) = '\0';
#else
		// directory
		if (S_ISDIR(st.st_mode)) {
			*(pathstart + strlen(dent->d_name)) = '/';
			*(pathstart + strlen(dent->d_name) + 1) = '\0';
#endif
			int32_t tmp_array = qs_opendir( _ppool, tmppath );
			if( -1 != tmp_array )
			{
				QS_ARRAY* parray;
				QS_ARRAY_ELEMENT* elm;
				int i;
				parray = (QS_ARRAY*)QS_GET_POINTER( _ppool, tmp_array );
				elm = (QS_ARRAY_ELEMENT*)QS_GET_POINTER( _ppool, parray->memid );
				for( i = 0; i < parray->len; i++ ){
					qs_array_push(_ppool,&path_array_munit,ELEMENT_LITERAL_STR,(elm+i)->memid_array_element_data);
				}
			}
		}
#ifdef __WINDOWS__
		else{
#else
		// file
		else if (S_ISREG(st.st_mode)) {
#endif
			qs_array_push_string(_ppool, &path_array_munit, tmppath);
		}
#ifdef __WINDOWS__
		} while (FindNextFile(handle, &fd));
		FindClose(handle);
#else
		} while ((dent = readdir(dir)) != NULL);
		closedir(dir);
#endif
	return path_array_munit;
}

