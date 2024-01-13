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

#include "qs_hash.h"

int32_t qs_create_hash( QS_MEMORY_POOL* memory, size_t hash_size )
{
	int32_t memid_hash = -1;
	if( hash_size <= 0 ){
		return -1;
	}
	if( -1 == (memid_hash = qs_create_memory_block( memory, sizeof( struct QS_HASH ) )) ){
		return -1;
	}
	QS_HASH *hash = (struct QS_HASH *)QS_GET_POINTER( memory, memid_hash );
	hash->hash_size = hash_size;
	hash->hash_munit = qs_create_memory_block( memory, sizeof( struct QS_HASH ) * hash->hash_size );
	if( -1 == hash->hash_munit ){
		return -1;
	}
	QS_HASH *hash_child = (struct QS_HASH *)QS_GET_POINTER( memory, hash->hash_munit );
	uint32_t i;
	for( i = 0; i < hash->hash_size; i++ )
	{
		hash_child[i].hash_size	= 0;
		hash_child[i].hash_munit	= -1;
	}
	return memid_hash;
}

QS_HASH_ELEMENT* qs_add_hash( QS_MEMORY_POOL* memory, int32_t memid_hash, int32_t memid_name_string, int32_t memid_data, int32_t id )
{
	struct QS_HASH *hash;
	struct QS_HASH *hashchild;
	struct QS_HASH_ELEMENT *hashelement;
	uint32_t hashkey;
	uint32_t i;
	QS_HASH_ELEMENT* is_push = NULL;
	if( -1 == memid_hash ){
		return is_push;
	}
	char* hash_name = (char*)QS_GET_POINTER( memory, memid_name_string );
	hash = (struct QS_HASH *)QS_GET_POINTER( memory, memid_hash );
	hashkey = qs_ihash( (char*)QS_GET_POINTER( memory, memid_name_string ), hash->hash_size );
	hashchild = (struct QS_HASH *)QS_GET_POINTER( memory, hash->hash_munit );
	if( -1 == hashchild[hashkey].hash_munit )
	{
		if( -1 == (hashchild[hashkey].hash_munit = qs_create_memory_block( memory, sizeof( struct QS_HASH_ELEMENT ) * QS_HASH_ELEMENT_SIZE ))){
			return is_push;
		}
		hashchild[hashkey].hash_size = QS_HASH_ELEMENT_SIZE;
		hashelement = (struct QS_HASH_ELEMENT*)QS_GET_POINTER( memory, hashchild[hashkey].hash_munit );
		for( i = 0; i < hashchild[hashkey].hash_size; ++i )
		{
			hashelement[i].memid_hash_name = -1;
			hashelement[i].memid_hash_element_data = -1;
			hashelement[i].id = -1;
			hashelement[i].create_time = 0;
			hashelement[i].life_time = 0;
		}
	} else {
		hashelement = (struct QS_HASH_ELEMENT*)QS_GET_POINTER( memory, hashchild[hashkey].hash_munit );
	}
	for( i = 0; i < hashchild[hashkey].hash_size; ++i )
	{
		if( -1 != hashelement[i].memid_hash_name )
		{
			if( !strcmp( (char*)QS_GET_POINTER( memory, hashelement[i].memid_hash_name ), hash_name ) )
			{
				if( hashelement[i].memid_hash_element_data != memid_data ){
					qs_free_memory_block( memory, &hashelement[i].memid_hash_element_data );
				}
				hashelement[i].memid_hash_element_data = memid_data;
				hashelement[i].id = id;
				//hashelement[i].create_time = time(NULL);
				is_push = &hashelement[i];
				break;
			}
		}
		else{
			hashelement[i].memid_hash_name = memid_name_string;
			hashelement[i].memid_hash_element_data = memid_data;
			hashelement[i].id = id;
			//hashelement[i].create_time = time(NULL);
			is_push = &hashelement[i];
			break;
		}
	}
	if( is_push == NULL ){
		int32_t resize_munit = -1;
		size_t resize = hashchild[hashkey].hash_size * QS_HASH_ELEMENT_RESIZE_QUANTITY;
		//printf("resize hash : %d -> %d\n",(int)(hashchild[hashkey].hash_size),(int)(resize));
		if( -1 == (resize_munit = qs_create_memory_block( memory, sizeof( struct QS_HASH_ELEMENT ) * resize ))){
			return is_push;
		}
		hashelement = (struct QS_HASH_ELEMENT*)QS_GET_POINTER( memory, resize_munit );
		QS_HASH_ELEMENT *oldhashelement = (struct QS_HASH_ELEMENT*)QS_GET_POINTER( memory, hashchild[hashkey].hash_munit );
		for( i = 0; i < resize; ++i ){
			if( i < hashchild[hashkey].hash_size ){
				hashelement[i].memid_hash_name = oldhashelement[i].memid_hash_name;
				hashelement[i].memid_hash_element_data = oldhashelement[i].memid_hash_element_data;
				hashelement[i].id = oldhashelement[i].id;
				hashelement[i].create_time = oldhashelement[i].create_time;
				hashelement[i].life_time = oldhashelement[i].life_time;
			} else {
				hashelement[i].memid_hash_name = -1;
				hashelement[i].memid_hash_element_data = -1;
				hashelement[i].id = -1;
				hashelement[i].create_time = 0;
				hashelement[i].life_time = 0;
			}
		}
		
		hashelement[hashchild[hashkey].hash_size].memid_hash_name = memid_name_string;
		hashelement[hashchild[hashkey].hash_size].memid_hash_element_data = memid_data;
		hashelement[hashchild[hashkey].hash_size].id = id;
		//hashelement[hashchild[hashkey].hash_size].create_time = time(NULL);
		is_push = &hashelement[hashchild[hashkey].hash_size];
		
		hashchild[hashkey].hash_size = resize;
		hashchild[hashkey].hash_munit = resize_munit;
	}
	return is_push;
}

int32_t qs_make_hash_name( QS_MEMORY_POOL* memory, int32_t memid_hash,const char* name)
{
	int32_t memid_name_string = qs_get_hash_name( memory, memid_hash, name );
	if( memid_name_string == -1 ){
		if( -1 == (memid_name_string = qs_create_memory_block( memory, strlen( name )+1 ))){
			return -1;
		}
		char* pbuf = (char*)QS_GET_POINTER( memory, memid_name_string );
		qs_strcopy( pbuf, name, qs_usize( memory, memid_name_string ) );
	}
	return memid_name_string;
}

void qs_add_hash_hash(QS_MEMORY_POOL* memory, int32_t memid_hash, const char* name, int32_t memid_target_hash)
{
	int32_t memid_name = qs_make_hash_name(memory, memid_hash, name);
	if (memid_name == -1) {
		return;
	}
	int32_t hash_munit = qs_get_hash(memory, memid_hash, name);
	if (hash_munit != -1) {
		printf("hash exist\n");
	}
	QS_HASH_ELEMENT* is_push = qs_add_hash(memory, memid_hash, memid_name, memid_target_hash, ELEMENT_HASH);
	if (NULL == is_push) {
		printf("[qs_add_hash_hash] is_push is NULL\n");
	}
}

void qs_add_hash_value( QS_MEMORY_POOL* memory, int32_t memid_hash, const char* name, const char* value, int32_t id )
{
	char* pbuf;
	int32_t memid_data;
	int32_t memid_name = qs_make_hash_name( memory, memid_hash, name );
	if( memid_name == -1 ){
		return;
	}
	if( -1 == (memid_data = qs_create_memory_block( memory, strlen( value )+1 ))){
		return;
	}
	pbuf = (char*)QS_GET_POINTER( memory, memid_data );
	memcpy( pbuf, value, qs_usize( memory, memid_data ) );
	QS_HASH_ELEMENT* is_push = qs_add_hash( memory, memid_hash, memid_name, memid_data, id );
	if( NULL==is_push){
		printf("[qs_add_hash_value] is_push is NULL\n");
	}
}

void qs_add_hash_array(QS_MEMORY_POOL* memory, int32_t memid_hash, const char* name, int32_t memid_target_array)
{
	int32_t memid_name = qs_make_hash_name(memory, memid_hash, name);
	if (memid_name == -1) {
		return;
	}
	int32_t array_munit = qs_get_hash(memory, memid_hash, name);
	if (array_munit != -1) {
		printf("array exist\n");
	}
	QS_HASH_ELEMENT* is_push = qs_add_hash(memory, memid_hash, memid_name, memid_target_array, ELEMENT_ARRAY);
	if (NULL == is_push) {
		printf("[qs_add_hash_array] is_push is NULL\n");
	}
}

void qs_add_hash_array_string( QS_MEMORY_POOL* memory, int32_t memid_hash, const char* name, const char* value )
{
	int32_t memid_name = qs_make_hash_name( memory, memid_hash, name );
	if( memid_name == -1 ){
		return;
	}
	int32_t array_munit = qs_get_hash( memory, memid_hash, name );
	if( array_munit == -1 ){
		array_munit = qs_create_array( memory, QS_HASH_ELEMENT_ARRAY_SIZE );
		if( -1 == array_munit ){
			return;
		}
	}
	if( QS_SYSTEM_ERROR == qs_array_push_string( memory, &array_munit, value ) ){
		return;
	}
	QS_HASH_ELEMENT* is_push = qs_add_hash( memory, memid_hash, memid_name, array_munit, ELEMENT_ARRAY );
	if( NULL==is_push){
		printf("[qs_add_hash_array_string] is_push is NULL\n");
	}
}

void qs_add_hash_array_empty_string( QS_MEMORY_POOL* memory, int32_t memid_hash, const char* name, size_t size )
{
	int32_t memid_name = qs_make_hash_name( memory, memid_hash, name );
	if( memid_name == -1 ){
		return;
	}
	int32_t array_munit = qs_get_hash( memory, memid_hash, name );
	if( array_munit == -1 ){
		array_munit = qs_create_array( memory, QS_HASH_ELEMENT_ARRAY_SIZE );
		if( -1 == array_munit ){
			return;
		}
	}
	if( QS_SYSTEM_ERROR == qs_array_push_empty_string( memory, &array_munit, size ) ){
		return;
	}
	QS_HASH_ELEMENT* is_push = qs_add_hash( memory, memid_hash, memid_name, array_munit, ELEMENT_ARRAY );
	if( NULL==is_push){
		printf("[qs_add_hash_array_empty_string] is_push is NULL\n");
	}
}

QS_HASH_ELEMENT* qs_add_hash_binary( QS_MEMORY_POOL* memory, int32_t memid_hash, const char* name, uint8_t* binary, size_t size )
{
	uint8_t* bin;
	int32_t memid_name = qs_make_hash_name( memory, memid_hash, name );
	if( memid_name == -1 ){
		return NULL;
	}
	int32_t memid_data = qs_get_hash( memory, memid_hash, name );
	if( memid_data == -1 ){
		if( -1 == ( memid_data = qs_create_memory_block( memory, size ) ) ){
			return NULL;
		}
	}
	else{
		if( qs_usize(memory,memid_data) < size ){
			if( -1 == ( memid_data = qs_create_memory_block( memory, size ) ) ){
				return NULL;
			}
		}
	}
	bin = (uint8_t*)QS_GET_POINTER( memory, memid_data );
	memcpy( bin, binary, size );
	QS_MEMORY_UNIT* punit = qs_get_memory_block( memory, memid_data );
	punit->top = size;
	QS_HASH_ELEMENT* is_push = qs_add_hash( memory, memid_hash, memid_name, memid_data, ELEMENT_LITERAL_BIN );
	return is_push;
}

QS_HASH_ELEMENT* qs_add_hash_integer( QS_MEMORY_POOL* memory, int32_t memid_hash, const char* name, int32_t value )
{
	int32_t memid_name = qs_make_hash_name( memory, memid_hash, name );
	if( memid_name == -1 ){
		return NULL;
	}
	int32_t memid_data = qs_get_hash( memory, memid_hash, name );
	if( memid_data == -1 ){
		if( -1 == ( memid_data = qs_create_memory_block( memory, NUMERIC_BUFFER_SIZE ) ) ){
			return NULL;
		}
	}
	size_t data_size = qs_usize(memory,memid_data);
	qs_itoa( value, (char*)QS_GET_POINTER(memory,memid_data), data_size );
	int32_t* pv = QS_PINT32(memory,memid_data);
	*pv = value;
	QS_HASH_ELEMENT* is_push = qs_add_hash( memory, memid_hash, memid_name, memid_data, ELEMENT_LITERAL_NUM );
	if( NULL==is_push){
		printf("[qs_add_hash_integer] is_push is NULL\n");
	}
	return is_push;
}

void qs_add_hash_integer_kint( QS_MEMORY_POOL* memory, int32_t memid_hash, int32_t memid_name_string, int32_t value )
{
	int32_t memid_data;
	if( -1 == ( memid_data = qs_create_memory_block( memory, NUMERIC_BUFFER_SIZE ) ) ){
		return;
	}
	size_t data_size = qs_usize(memory,memid_data);
	qs_itoa( value, (char*)QS_GET_POINTER(memory,memid_data), data_size );
	int32_t* pv = QS_PINT32(memory,memid_data);
	*pv = value;
	QS_HASH_ELEMENT* is_push = qs_add_hash( memory, memid_hash, memid_name_string, memid_data, ELEMENT_LITERAL_NUM );
	if( NULL==is_push){
		printf("[qs_add_hash_integer_kint] is_push is NULL\n");
	}
}

QS_HASH_ELEMENT* qs_add_hash_string( QS_MEMORY_POOL* memory, int32_t memid_hash, const char* name, const char* value )
{
	char* pbuf;
	int32_t memid_name = qs_make_hash_name( memory, memid_hash, name );
	if( memid_name == -1 ){
		return NULL;
	}
	int32_t memid_data = qs_get_hash( memory, memid_hash, name );
	size_t data_size = 0;
	if( memid_data == -1 ){
		if( -1 == ( memid_data = qs_create_memory_block( memory, strlen( value )+1 ) ) ){
			return NULL;
		}
		data_size = qs_usize( memory, memid_data );
	}
	else{
		size_t size = strlen( value )+1;
		data_size = qs_usize( memory, memid_data );
		if( data_size < size ){
			if( -1 == ( memid_data = qs_create_memory_block( memory, strlen( value )+1 ) ) ){
				return NULL;
			}
		}
		data_size = qs_usize( memory, memid_data );
	}
	pbuf = (char*)QS_GET_POINTER( memory, memid_data );
	memcpy( pbuf, value, data_size );
	QS_HASH_ELEMENT* is_push = qs_add_hash( memory, memid_hash, memid_name, memid_data, ELEMENT_LITERAL_STR );
	if( NULL==is_push){
		printf("[qs_add_hash_string] is_push is NULL\n");
	}
	return is_push;
}

void qs_add_hash_emptystring( QS_MEMORY_POOL* memory, int32_t memid_hash, const char* name, size_t string_size )
{
	char* pbuf;
	int32_t memid_data;
	int32_t memid_name = qs_make_hash_name( memory, memid_hash, name );
	if( memid_name == -1 ){
		return;
	}
	if( -1 == ( memid_data = qs_create_memory_block( memory, string_size ) ) ){
		return;
	}
	pbuf = (char*)QS_GET_POINTER( memory, memid_data );
	//memset( pbuf, 0, qs_usize( memory, memid_data ) );
	pbuf[0] = '\0';
	QS_HASH_ELEMENT* is_push = qs_add_hash( memory, memid_hash, memid_name, memid_data, ELEMENT_LITERAL_STR );
	if( NULL==is_push){
		printf("[qs_add_hash_emptystring] is_push is NULL\n");
	}
}

void qs_add_hash_value_kstring( QS_MEMORY_POOL* memory, int32_t memid_hash, const char* name, int32_t memid_data, int32_t id )
{
	int32_t memid_name = qs_make_hash_name( memory, memid_hash, name );
	if( memid_name == -1 ){
		return;
	}
	QS_HASH_ELEMENT* is_push = qs_add_hash( memory, memid_hash, memid_name, memid_data, id );
	if( NULL==is_push){
		printf("[qs_add_hash_value_kstring] is_push is NULL\n");
	}
}

int32_t qs_move_hash( QS_MEMORY_POOL* memory, int32_t memid_hash, const char* hash_name_from, const char* hash_name_to )
{
	if( memid_hash == -1 ){
		return -1;
	}
	struct QS_HASH *hash = (struct QS_HASH *)QS_GET_POINTER( memory, memid_hash );
	struct QS_HASH *hashchild = (struct QS_HASH *)QS_GET_POINTER( memory, hash->hash_munit );
	uint32_t hashkey = qs_ihash( hash_name_from, hash->hash_size );
	int32_t retmunit = -1;
	if( -1 == hashchild[hashkey].hash_munit ){
		return retmunit;
	}
	if( qs_strlen(hash_name_from) != qs_strlen(hash_name_to) ){
		return retmunit;
	}
	QS_HASH_ELEMENT *hashelement = (struct QS_HASH_ELEMENT*)QS_GET_POINTER( memory, hashchild[hashkey].hash_munit );
	uint32_t i;
	for( i = 0; i < hashchild[hashkey].hash_size; i++ )
	{
		if( hashelement[i].memid_hash_name >= 0 )
		{
			char* tmpname = (char*)QS_GET_POINTER( memory, hashelement[i].memid_hash_name );
			if( !strcmp( tmpname, hash_name_from ) )
			{
				memcpy(tmpname,hash_name_to,qs_strlen(hash_name_to));
				QS_HASH_ELEMENT* elm = qs_add_hash( memory, memid_hash, hashelement[i].memid_hash_name, hashelement[i].memid_hash_element_data, hashelement[i].id );
				if( elm != &hashelement[i] ){
					hashelement[i].memid_hash_name = -1;
					hashelement[i].memid_hash_element_data = -1;
					hashelement[i].id = -1;
					hashelement[i].create_time = 0;
					hashelement[i].life_time = 0;
				}
				break;
			}
		}
	}
	return retmunit;
}

int32_t qs_remove_hash( QS_MEMORY_POOL* memory, int32_t memid_hash, const char* hash_name )
{
	struct QS_HASH *hash = (struct QS_HASH *)QS_GET_POINTER( memory, memid_hash );
	struct QS_HASH *hashchild = (struct QS_HASH *)QS_GET_POINTER( memory, hash->hash_munit );
	uint32_t hashkey = qs_ihash( hash_name, hash->hash_size );
	QS_HASH_ELEMENT *hashelement = (struct QS_HASH_ELEMENT*)QS_GET_POINTER( memory, hashchild[hashkey].hash_munit );
	uint32_t i;
	for( i = 0; i < hashchild[hashkey].hash_size; i++ )
	{
		if( hashelement[i].memid_hash_name >= 0 )
		{
			if( !strcmp( (char*)QS_GET_POINTER( memory, hashelement[i].memid_hash_name ), hash_name ) )
			{
				hashelement[i].id = -1;
				hashelement[i].memid_hash_name = -1;
				hashelement[i].memid_hash_element_data = -1;
				hashelement[i].create_time = 0;
				hashelement[i].life_time = 0;
				break;
			}
		}
	}
	return QS_SYSTEM_OK;
}

char* qs_get_hash_string( QS_MEMORY_POOL* memory, int32_t memid_hash, const char* hash_name )
{
	if( memid_hash == -1 ){
		return NULL;
	}
	struct QS_HASH *hash = (struct QS_HASH *)QS_GET_POINTER( memory, memid_hash );
	struct QS_HASH *hashchild = (struct QS_HASH *)QS_GET_POINTER( memory, hash->hash_munit );
	uint32_t hashkey = qs_ihash( hash_name, hash->hash_size );
	QS_HASH_ELEMENT* elm = qs_get_hash_core( memory, hash, hashchild, hash_name, hashkey );
	if(elm==NULL){
		return NULL;
	}
	if(elm->id != ELEMENT_LITERAL_STR){
		return NULL;
	}
	return (char*)QS_GET_POINTER(memory,elm->memid_hash_element_data);
}

int32_t* qs_get_hash_integer( QS_MEMORY_POOL* memory, int32_t memid_hash, const char* hash_name )
{
	if( memid_hash == -1 ){
		return NULL;
	}
	struct QS_HASH *hash = (struct QS_HASH *)QS_GET_POINTER( memory, memid_hash );
	struct QS_HASH *hashchild = (struct QS_HASH *)QS_GET_POINTER( memory, hash->hash_munit );
	uint32_t hashkey = qs_ihash( hash_name, hash->hash_size );
	QS_HASH_ELEMENT* elm = qs_get_hash_core( memory, hash, hashchild, hash_name, hashkey );
	if(elm==NULL){
		return NULL;
	}
	if(elm->id != ELEMENT_LITERAL_NUM){
		return NULL;
	}
	return QS_PINT32(memory,elm->memid_hash_element_data);
}

int32_t qs_get_hash( QS_MEMORY_POOL* memory, int32_t memid_hash, const char* hash_name )
{
	if( memid_hash == -1 ){
		return -1;
	}
	struct QS_HASH *hash = (struct QS_HASH *)QS_GET_POINTER( memory, memid_hash );
	struct QS_HASH *hashchild = (struct QS_HASH *)QS_GET_POINTER( memory, hash->hash_munit );
	uint32_t hashkey = qs_ihash( hash_name, hash->hash_size );
	QS_HASH_ELEMENT* elm = qs_get_hash_core( memory, hash, hashchild, hash_name, hashkey );
	if(elm==NULL){
		return -1;
	}
	return elm->memid_hash_element_data;
}

int32_t qs_get_hash_fix_ihash( QS_MEMORY_POOL* memory, int32_t memid_hash, const char* hash_name, uint32_t hashkey )
{
	if( memid_hash == -1 ){
		return -1;
	}
	struct QS_HASH *hash = (struct QS_HASH *)QS_GET_POINTER( memory, memid_hash );
	struct QS_HASH *hashchild = (struct QS_HASH *)QS_GET_POINTER( memory, hash->hash_munit );
	int32_t retmunit = -1;
	if( -1 == hashchild[hashkey].hash_munit )
	{
		//printf( "hash element is not found : %s\n", hash_name );
		return retmunit;
	}
	QS_HASH_ELEMENT *hashelement = (struct QS_HASH_ELEMENT*)QS_GET_POINTER( memory, hashchild[hashkey].hash_munit );
	uint32_t i;
	for( i = 0; i < hashchild[hashkey].hash_size; i++ )
	{
		if( hashelement[i].memid_hash_name >= 0 )
		{
			if( !strcmp( (char*)QS_GET_POINTER( memory, hashelement[i].memid_hash_name ), hash_name ) )
			{
				retmunit = hashelement[i].memid_hash_element_data;
				break;
			}
		}
	}
	return retmunit;
}

QS_HASH_ELEMENT* qs_get_hash_core( QS_MEMORY_POOL* memory, struct QS_HASH *hash, QS_HASH *hashchild, const char* hash_name, uint32_t hashkey )
{
	QS_HASH_ELEMENT* ret = NULL;
	if( -1 == hashchild[hashkey].hash_munit )
	{
		//printf( "hash element is not found : %s\n", hash_name );
		return NULL;
	}
	QS_HASH_ELEMENT *hashelement = (struct QS_HASH_ELEMENT*)QS_GET_POINTER( memory, hashchild[hashkey].hash_munit );
	uint32_t i;
	for( i = 0; i < hashchild[hashkey].hash_size; i++ )
	{
		if( hashelement[i].memid_hash_name >= 0 )
		{
			if( !strcmp( (char*)QS_GET_POINTER( memory, hashelement[i].memid_hash_name ), hash_name ) )
			{
				ret = &hashelement[i];
				break;
			}
		}
	}
	return ret;
}

void qs_clear_hash_string( QS_MEMORY_POOL* memory, int32_t memid_hash, const char* name )
{
	int32_t memid_data = qs_get_hash( memory, memid_hash, name );
	if( memid_data >= 0 ){
		//memset( (char*)QS_GET_POINTER(memory,memid_data), 0, QS_PUNIT_USIZE(memory,memid_data) );
		*((char*)QS_GET_POINTER(memory,memid_data)) = '\0';
	}
}

int32_t qs_replace_hash_string( QS_MEMORY_POOL* memory, int32_t memid_hash, const char* name, const char* value )
{
	int32_t memid_data = qs_get_hash( memory, memid_hash, name );
	if( memid_data >= 0 ){
		memcpy( (char*)QS_GET_POINTER(memory,memid_data), value, QS_PUNIT_USIZE(memory,memid_data) );
		*( ((char*)QS_GET_POINTER(memory,memid_data))+QS_PUNIT_USIZE(memory,memid_data)-1 ) = '\0';
	}
	return memid_data;
}

int32_t qs_get_hash_name( QS_MEMORY_POOL* memory, int32_t memid_hash, const char* hash_name )
{
	int32_t retmunit = -1;
	QS_HASH *hash;
	QS_HASH *hashchild;
	uint32_t hashkey;
	hash = (QS_HASH *)QS_GET_POINTER( memory, memid_hash );
	if( hash->hash_size == 0 ){
		return retmunit;
	}
	hashkey = qs_ihash( hash_name, hash->hash_size );
	hashchild = (QS_HASH *)QS_GET_POINTER( memory, hash->hash_munit );
	if( -1 == hashchild[hashkey].hash_munit ){
		return retmunit;
	}
	QS_HASH_ELEMENT *hashelement = (QS_HASH_ELEMENT*)QS_GET_POINTER( memory, hashchild[hashkey].hash_munit );
	uint32_t i;
	for( i = 0; i < hashchild[hashkey].hash_size; ++i ){
		if( hashelement[i].memid_hash_name != -1 ){
			if( !strcmp( (char*)QS_GET_POINTER( memory, hashelement[i].memid_hash_name ), hash_name ) ){
				retmunit = hashelement[i].memid_hash_name;
				break;
			}
		}
	}
	return retmunit;
}

int32_t qs_get_hash_id( QS_MEMORY_POOL* memory, int32_t memid_hash, const char* hash_name )
{
	int32_t retid = -1;
	QS_HASH *hash;
	QS_HASH *hashchild;
	uint32_t hashkey;
	hash = (QS_HASH *)QS_GET_POINTER( memory, memid_hash );
	hashkey = qs_ihash( hash_name, hash->hash_size );
	hashchild = (QS_HASH *)QS_GET_POINTER( memory, hash->hash_munit );
	if( -1 == hashchild[hashkey].hash_munit )
	{
		return retid;
	}
	QS_HASH_ELEMENT *hashelement = (QS_HASH_ELEMENT*)QS_GET_POINTER( memory, hashchild[hashkey].hash_munit );
	uint32_t i;
	for( i = 0; i < hashchild[hashkey].hash_size; i++ )
	{
		if( hashelement[i].memid_hash_name >= 0 )
		{
			if( !strcmp( (char*)QS_GET_POINTER( memory, hashelement[i].memid_hash_name ), hash_name ) )
			{
				retid = hashelement[i].id;
				break;
			}
		}
	}
	return retid;
}

QS_HASH_ELEMENT* qs_get_hash_element( QS_MEMORY_POOL* memory, int32_t memid_hash, const char* hash_name )
{
	QS_HASH_ELEMENT* ret = NULL;
	QS_HASH *hash;
	QS_HASH *hashchild;
	uint32_t hashkey;
	hash = (QS_HASH *)QS_GET_POINTER( memory, memid_hash );
	hashkey = qs_ihash( hash_name, hash->hash_size );
	hashchild = (QS_HASH *)QS_GET_POINTER( memory, hash->hash_munit );
	if( -1 == hashchild[hashkey].hash_munit )
	{
		return ret;
	}
	QS_HASH_ELEMENT *hashelement = (QS_HASH_ELEMENT*)QS_GET_POINTER( memory, hashchild[hashkey].hash_munit );
	uint32_t i;
	for( i = 0; i < hashchild[hashkey].hash_size; i++ )
	{
		if( hashelement[i].memid_hash_name >= 0 )
		{
			if( !strcmp( (char*)QS_GET_POINTER( memory, hashelement[i].memid_hash_name ), hash_name ) )
			{
				ret = &hashelement[i];
				break;
			}
		}
	}
	return ret;
}

int32_t qs_hash_length( QS_MEMORY_POOL* memory, int32_t memid_hash )
{
	int32_t length = 0;
	struct QS_HASH *hash;
	struct QS_HASH *hashchild;
	struct QS_HASH_ELEMENT *hashelement;
	uint32_t i,j;
	hash = (struct QS_HASH *)QS_GET_POINTER( memory, memid_hash );
	hashchild = (struct QS_HASH *)QS_GET_POINTER( memory, hash->hash_munit );
	for( j = 0; j < hash->hash_size; j++ )
	{
		if( hashchild[j].hash_munit >= 0 )
		{
			hashelement = (struct QS_HASH_ELEMENT*)QS_GET_POINTER( memory, hashchild[j].hash_munit );
			for( i = 0; i < hashchild[j].hash_size; i++ )
			{
				if( hashelement[i].memid_hash_name >= 0 )
				{
					++length;
				}
			}
		}
	}
	return length;
}


int32_t qs_init_hash_foreach( QS_MEMORY_POOL* memory, int32_t memid_hash, QS_HASH_FOREACH* hf )
{
	hf->p1 = 0;
	hf->p2 = 0;
	hf->root = (QS_HASH*)QS_GET_POINTER( memory, memid_hash );
	hf->child = (QS_HASH*)QS_GET_POINTER( memory, hf->root->hash_munit );
	hf->he = NULL;
	return QS_SYSTEM_OK;
}


QS_HASH_ELEMENT* qs_hash_foreach( QS_MEMORY_POOL* memory, QS_HASH_FOREACH* hf )
{
	QS_HASH_ELEMENT* ret = NULL;
	do{
		if( hf->he == NULL ){
			if( hf->p1 >= hf->root->hash_size ){
				hf->p1 = 0;
				break;
			}
			if( hf->child[hf->p1].hash_munit == -1 ){
				++hf->p1;
				continue;
			}
			hf->he = (QS_HASH_ELEMENT*)QS_GET_POINTER( memory, hf->child[hf->p1]. hash_munit );
		}
		if( hf->p2 >= hf->child[hf->p1].hash_size ){
			hf->he = NULL;
			hf->p2 = 0;
			++hf->p1;
			continue;
		}
		if( hf->he[hf->p2].memid_hash_name == -1 ){
			++hf->p2;
			continue;
		}
		ret = &hf->he[hf->p2];
		++hf->p2;
		break;
	}while( true );
	return ret;
}

int32_t qs_get_hash_keys( QS_MEMORY_POOL* memory, int32_t memid_hash, int is_sort_asc)
{
	int32_t ret = -1;
	int32_t memid_array = -1;
	int32_t memid_sorted_array = -1;
	struct QS_HASH *hash;
	struct QS_HASH *hashchild;
	struct QS_HASH_ELEMENT *hashelement;
	int32_t i,j;
	hash = (struct QS_HASH *)QS_GET_POINTER( memory, memid_hash );
	hashchild = (struct QS_HASH *)QS_GET_POINTER( memory, hash->hash_munit );
	for( j = 0; j < hash->hash_size; j++ )
	{
		if( hashchild[j].hash_munit >= 0 )
		{
			hashelement = (struct QS_HASH_ELEMENT*)QS_GET_POINTER( memory, hashchild[j].hash_munit );
			for( i = 0; i < hashchild[j].hash_size; i++ )
			{
				if( hashelement[i].memid_hash_name >= 0 )
				{
					const char* name = (char*)QS_GET_POINTER( memory, hashelement[i].memid_hash_name );
					if( QS_SYSTEM_ERROR == qs_array_push_string( memory, &memid_array, name ) ){
						return ret;
					}
				}
			}
		}
	}

	int32_t array_size = qs_array_length( memory, memid_array );
	int32_t sort_buffer = qs_create_memory_block( memory, sizeof( int32_t ) * array_size );
	if( sort_buffer == -1 ){
		return ret;
	}

	int32_t* sort_num_array = (int32_t*)QS_GET_POINTER( memory, sort_buffer );
	for( i = 0; i < array_size; ++i ){
		QS_ARRAY_ELEMENT* elm = qs_array_get( memory, memid_array, i );
		if( elm == NULL ){
			return ret;
		}
		int32_t memid_name = elm->memid_array_element_data;
		const char* name = (char*)QS_GET_POINTER( memory, memid_name );
		sort_num_array[i] = memid_name;
		for( j = i-1; j >= 0; j-- )
		{
			const char* name2 = (char*)QS_GET_POINTER( memory, sort_num_array[j] );
			int cmp = strcmp( name, name2 );
			if( is_sort_asc ? cmp < 0 : cmp > 0 ){
				int32_t memid_name2 = sort_num_array[j];
				sort_num_array[j+1] = memid_name2;
				sort_num_array[j] = memid_name;
			}
			else{
				break;
			}
		}
	}
	
	for( i = 0; i < array_size; ++i ){
		const char* name = (char*)QS_GET_POINTER( memory, sort_num_array[i] );
		if( QS_SYSTEM_ERROR == qs_array_push_string( memory, &memid_sorted_array, name ) ){
			return ret;
		}
	}

	ret = memid_sorted_array;
	return ret;
}