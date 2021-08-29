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

int32_t qs_create_hash( QS_MEMORY_POOL* _ppool, size_t hlen )
{
	int32_t h_munit = -1;
	if( hlen <= 0 ){
		return -1;
	}
	if( -1 == (h_munit = qs_create_memory_block( _ppool, sizeof( struct QS_HASH ) )) ){
		return -1;
	}
	QS_HASH *hash = (struct QS_HASH *)QS_GET_POINTER( _ppool, h_munit );
	hash->hash_size = hlen;
	hash->hash_munit = qs_create_memory_block( _ppool, sizeof( struct QS_HASH ) * hlen );
	if( -1 == hash->hash_munit ){
		return -1;
	}
	QS_HASH *hashchild = (struct QS_HASH *)QS_GET_POINTER( _ppool, hash->hash_munit );
	uint32_t i;
	for( i = 0; i < hlen; i++ )
	{
		hashchild[i].hash_size	= 0;
		hashchild[i].hash_munit	= -1;
	}
	return h_munit;
}

QS_HASH_ELEMENT* qs_add_hash( QS_MEMORY_POOL* _ppool, int32_t h_munit, int32_t name_munit, int32_t data_munit, int32_t id )
{
	struct QS_HASH *hash;
	struct QS_HASH *hashchild;
	struct QS_HASH_ELEMENT *hashelement;
	uint32_t hashkey;
	uint32_t i;
	QS_HASH_ELEMENT* is_push = NULL;
	if( -1 == h_munit ){
		return is_push;
	}
	char* hash_name = (char*)QS_GET_POINTER( _ppool, name_munit );
	hash = (struct QS_HASH *)QS_GET_POINTER( _ppool, h_munit );
	hashkey = qs_ihash( (char*)QS_GET_POINTER( _ppool, name_munit ), hash->hash_size );
	hashchild = (struct QS_HASH *)QS_GET_POINTER( _ppool, hash->hash_munit );
	if( -1 == hashchild[hashkey].hash_munit )
	{
		if( -1 == (hashchild[hashkey].hash_munit = qs_create_memory_block( _ppool, sizeof( struct QS_HASH_ELEMENT ) * QS_HASH_ELEMENT_SIZE ))){
			return is_push;
		}
		hashchild[hashkey].hash_size = QS_HASH_ELEMENT_SIZE;
		hashelement = (struct QS_HASH_ELEMENT*)QS_GET_POINTER( _ppool, hashchild[hashkey].hash_munit );
		for( i = 0; i < hashchild[hashkey].hash_size; ++i )
		{
			hashelement[i].hashname_munit = -1;
			hashelement[i].elm_munit = -1;
			hashelement[i].id = -1;
			hashelement[i].create_time = 0;
			hashelement[i].life_time = 0;
		}
	} else {
		hashelement = (struct QS_HASH_ELEMENT*)QS_GET_POINTER( _ppool, hashchild[hashkey].hash_munit );
	}
	for( i = 0; i < hashchild[hashkey].hash_size; ++i )
	{
		if( -1 != hashelement[i].hashname_munit )
		{
			if( !strcmp( (char*)QS_GET_POINTER( _ppool, hashelement[i].hashname_munit ), hash_name ) )
			{
				if( hashelement[i].elm_munit != data_munit ){
					qs_free_memory_unit( _ppool, &hashelement[i].elm_munit );
				}
				hashelement[i].elm_munit = data_munit;
				hashelement[i].id = id;
				//hashelement[i].create_time = time(NULL);
				is_push = &hashelement[i];
				break;
			}
		}
		else{
			hashelement[i].hashname_munit = name_munit;
			hashelement[i].elm_munit = data_munit;
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
		if( -1 == (resize_munit = qs_create_memory_block( _ppool, sizeof( struct QS_HASH_ELEMENT ) * resize ))){
			return is_push;
		}
		hashelement = (struct QS_HASH_ELEMENT*)QS_GET_POINTER( _ppool, resize_munit );
		QS_HASH_ELEMENT *oldhashelement = (struct QS_HASH_ELEMENT*)QS_GET_POINTER( _ppool, hashchild[hashkey].hash_munit );
		for( i = 0; i < resize; ++i ){
			if( i < hashchild[hashkey].hash_size ){
				hashelement[i].hashname_munit = oldhashelement[i].hashname_munit;
				hashelement[i].elm_munit = oldhashelement[i].elm_munit;
				hashelement[i].id = oldhashelement[i].id;
				hashelement[i].create_time = oldhashelement[i].create_time;
				hashelement[i].life_time = oldhashelement[i].life_time;
			} else {
				hashelement[i].hashname_munit = -1;
				hashelement[i].elm_munit = -1;
				hashelement[i].id = -1;
				hashelement[i].create_time = 0;
				hashelement[i].life_time = 0;
			}
		}
		
		hashelement[hashchild[hashkey].hash_size].hashname_munit = name_munit;
		hashelement[hashchild[hashkey].hash_size].elm_munit = data_munit;
		hashelement[hashchild[hashkey].hash_size].id = id;
		//hashelement[hashchild[hashkey].hash_size].create_time = time(NULL);
		is_push = &hashelement[hashchild[hashkey].hash_size];
		
		hashchild[hashkey].hash_size = resize;
		hashchild[hashkey].hash_munit = resize_munit;
	}
	return is_push;
}

int32_t qs_make_hash_name( QS_MEMORY_POOL* _ppool, int32_t h_munit,const char* name)
{
	int32_t name_munit = qs_get_hash_name( _ppool, h_munit, name );
	if( name_munit == -1 ){
		if( -1 == (name_munit = qs_create_memory_block( _ppool, strlen( name )+1 ))){
			return -1;
		}
		char* pbuf = (char*)QS_GET_POINTER( _ppool, name_munit );
		qs_strcopy( pbuf, name, qs_usize( _ppool, name_munit ) );
	}
	return name_munit;
}

void qs_add_hash_hash(QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* name, int32_t hash_id)
{
	int32_t namemunit = qs_make_hash_name(_ppool, h_munit, name);
	if (namemunit == -1) {
		return;
	}
	int32_t hash_munit = qs_get_hash(_ppool, h_munit, name);
	if (hash_munit != -1) {
		printf("hash exist\n");
	}
	QS_HASH_ELEMENT* is_push = qs_add_hash(_ppool, h_munit, namemunit, hash_id, ELEMENT_HASH);
	if (NULL == is_push) {
		printf("is_push is NULL\n");
	}
}

void qs_add_hash_value( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* name, const char* value, int32_t id )
{
	char* pbuf;
	int32_t datamunit;
	int32_t namemunit = qs_make_hash_name( _ppool, h_munit, name );
	if( namemunit == -1 ){
		return;
	}
	if( -1 == (datamunit = qs_create_memory_block( _ppool, strlen( value )+1 ))){
		return;
	}
	pbuf = (char*)QS_GET_POINTER( _ppool, datamunit );
	memcpy( pbuf, value, qs_usize( _ppool, datamunit ) );
	QS_HASH_ELEMENT* is_push = qs_add_hash( _ppool, h_munit, namemunit, datamunit, id );
	if( NULL==is_push){
		printf("is_push is NULL\n");
	}
}

void qs_add_hash_array(QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* name, int32_t array_id)
{
	int32_t namemunit = qs_make_hash_name(_ppool, h_munit, name);
	if (namemunit == -1) {
		return;
	}
	int32_t array_munit = qs_get_hash(_ppool, h_munit, name);
	if (array_munit != -1) {
		printf("array exist\n");
	}
	QS_HASH_ELEMENT* is_push = qs_add_hash(_ppool, h_munit, namemunit, array_id, ELEMENT_ARRAY);
	if (NULL == is_push) {
		printf("is_push is NULL\n");
	}
}

void qs_add_hash_array_string( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* name, const char* value )
{
	int32_t namemunit = qs_make_hash_name( _ppool, h_munit, name );
	if( namemunit == -1 ){
		return;
	}
	int32_t array_munit = qs_get_hash( _ppool, h_munit, name );
	if( array_munit == -1 ){
		array_munit = qs_create_array( _ppool, QS_HASH_ELEMENT_ARRAY_SIZE, 0 );
		if( -1 == array_munit ){
			return;
		}
	}
	if( QS_SYSTEM_ERROR == qs_array_push_string( _ppool, &array_munit, value ) ){
		return;
	}
	QS_HASH_ELEMENT* is_push = qs_add_hash( _ppool, h_munit, namemunit, array_munit, ELEMENT_ARRAY );
	if( NULL==is_push){
		printf("is_push is NULL\n");
	}
}

void qs_add_hash_array_empty_string( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* name, size_t size )
{
	int32_t namemunit = qs_make_hash_name( _ppool, h_munit, name );
	if( namemunit == -1 ){
		return;
	}
	int32_t array_munit = qs_get_hash( _ppool, h_munit, name );
	if( array_munit == -1 ){
		array_munit = qs_create_array( _ppool, QS_HASH_ELEMENT_ARRAY_SIZE, 0 );
		if( -1 == array_munit ){
			return;
		}
	}
	if( QS_SYSTEM_ERROR == qs_array_push_empty_string( _ppool, &array_munit, size ) ){
		return;
	}
	QS_HASH_ELEMENT* is_push = qs_add_hash( _ppool, h_munit, namemunit, array_munit, ELEMENT_ARRAY );
	if( NULL==is_push){
		printf("is_push is NULL\n");
	}
}

QS_HASH_ELEMENT* qs_add_hash_binary( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* name, uint8_t* binary, size_t size )
{
	uint8_t* bin;
	int32_t namemunit = qs_make_hash_name( _ppool, h_munit, name );
	if( namemunit == -1 ){
		return NULL;
	}
	int32_t datamunit = qs_get_hash( _ppool, h_munit, name );
	if( datamunit == -1 ){
		if( -1 == ( datamunit = qs_create_memory_block( _ppool, size ) ) ){
			return NULL;
		}
	}
	else{
		if( qs_usize(_ppool,datamunit) < size ){
			if( -1 == ( datamunit = qs_create_memory_block( _ppool, size ) ) ){
				return NULL;
			}
		}
	}
	bin = (uint8_t*)QS_GET_POINTER( _ppool, datamunit );
	memcpy( bin, binary, size );
	QS_MEMORY_UNIT* punit = qs_get_munit( _ppool, datamunit );
	punit->top = size;
	QS_HASH_ELEMENT* is_push = qs_add_hash( _ppool, h_munit, namemunit, datamunit, ELEMENT_LITERAL_BIN );
	return is_push;
}

QS_HASH_ELEMENT* qs_add_hash_integer( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* name, int32_t value )
{
	int32_t namemunit = qs_make_hash_name( _ppool, h_munit, name );
	if( namemunit == -1 ){
		return NULL;
	}
	int32_t datamunit = qs_get_hash( _ppool, h_munit, name );
	if( datamunit == -1 ){
		if( -1 == ( datamunit = qs_create_memory_block( _ppool, NUMERIC_BUFFER_SIZE ) ) ){
			return NULL;
		}
	}
	size_t data_size = qs_usize(_ppool,datamunit);
	qs_itoa( value, (char*)QS_GET_POINTER(_ppool,datamunit), data_size );
	(*(int32_t*)(QS_GET_POINTER(_ppool,datamunit)+data_size-sizeof(int32_t))) = value;
	QS_HASH_ELEMENT* is_push = qs_add_hash( _ppool, h_munit, namemunit, datamunit, ELEMENT_LITERAL_NUM );
	//if( NULL==is_push){ printf("is_push is NULL\n"); }
	return is_push;
}

void qs_add_hash_integer_kint( QS_MEMORY_POOL* _ppool, int32_t h_munit, int32_t name_munit, int32_t value )
{
	int32_t datamunit;
	if( -1 == ( datamunit = qs_create_memory_block( _ppool, NUMERIC_BUFFER_SIZE ) ) ){
		return;
	}
	size_t data_size = qs_usize(_ppool,datamunit);
	qs_itoa( value, (char*)QS_GET_POINTER(_ppool,datamunit), data_size );
	(*(int32_t*)(QS_GET_POINTER(_ppool,datamunit)+data_size-sizeof(int32_t))) = value;
	QS_HASH_ELEMENT* is_push = qs_add_hash( _ppool, h_munit, name_munit, datamunit, ELEMENT_LITERAL_NUM );
	if( NULL==is_push){
		printf("is_push is NULL\n");
	}
}

QS_HASH_ELEMENT* qs_add_hash_string( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* name, const char* value )
{
	char* pbuf;
	int32_t namemunit = qs_make_hash_name( _ppool, h_munit, name );
	if( namemunit == -1 ){
		return NULL;
	}
	int32_t datamunit = qs_get_hash( _ppool, h_munit, name );
	size_t data_size = 0;
	if( datamunit == -1 ){
		if( -1 == ( datamunit = qs_create_memory_block( _ppool, strlen( value )+1 ) ) ){
			return NULL;
		}
		data_size = qs_usize( _ppool, datamunit );
	}
	else{
		size_t size = strlen( value )+1;
		data_size = qs_usize( _ppool, datamunit );
		if( data_size < size ){
			if( -1 == ( datamunit = qs_create_memory_block( _ppool, strlen( value )+1 ) ) ){
				return NULL;
			}
		}
		data_size = qs_usize( _ppool, datamunit );
	}
	pbuf = (char*)QS_GET_POINTER( _ppool, datamunit );
	memcpy( pbuf, value, data_size );
	QS_HASH_ELEMENT* is_push = qs_add_hash( _ppool, h_munit, namemunit, datamunit, ELEMENT_LITERAL_STR );
	//if( NULL==is_push){printf("is_push is NULL\n");}
	return is_push;
}

void qs_add_hash_emptystring( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* name, size_t string_size )
{
	char* pbuf;
	int32_t datamunit;
	int32_t namemunit = qs_make_hash_name( _ppool, h_munit, name );
	if( namemunit == -1 ){
		return;
	}
	if( -1 == ( datamunit = qs_create_memory_block( _ppool, string_size ) ) ){
		return;
	}
	pbuf = (char*)QS_GET_POINTER( _ppool, datamunit );
	//memset( pbuf, 0, qs_usize( _ppool, datamunit ) );
	pbuf[0] = '\0';
	QS_HASH_ELEMENT* is_push = qs_add_hash( _ppool, h_munit, namemunit, datamunit, ELEMENT_LITERAL_STR );
	if( NULL==is_push){
		printf("is_push is NULL\n");
	}
}

void qs_add_hash_value_kstring( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* name, int32_t data_munit, int32_t id )
{
	int32_t namemunit = qs_make_hash_name( _ppool, h_munit, name );
	if( namemunit == -1 ){
		return;
	}
	QS_HASH_ELEMENT* is_push = qs_add_hash( _ppool, h_munit, namemunit, data_munit, id );
	if( NULL==is_push){
		printf("is_push is NULL\n");
	}
}

int32_t qs_move_hash( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* hashname_from, const char* hashname_to )
{
	if( h_munit == -1 ){
		return -1;
	}
	struct QS_HASH *hash = (struct QS_HASH *)QS_GET_POINTER( _ppool, h_munit );
	struct QS_HASH *hashchild = (struct QS_HASH *)QS_GET_POINTER( _ppool, hash->hash_munit );
	uint32_t hashkey = qs_ihash( hashname_from, hash->hash_size );
	int32_t retmunit = -1;
	if( -1 == hashchild[hashkey].hash_munit ){
		return retmunit;
	}
	if( qs_strlen(hashname_from) != qs_strlen(hashname_to) ){
		return retmunit;
	}
	QS_HASH_ELEMENT *hashelement = (struct QS_HASH_ELEMENT*)QS_GET_POINTER( _ppool, hashchild[hashkey].hash_munit );
	uint32_t i;
	for( i = 0; i < hashchild[hashkey].hash_size; i++ )
	{
		if( hashelement[i].hashname_munit >= 0 )
		{
			char* tmpname = (char*)QS_GET_POINTER( _ppool, hashelement[i].hashname_munit );
			if( !strcmp( tmpname, hashname_from ) )
			{
				memcpy(tmpname,hashname_to,qs_strlen(hashname_to));
				QS_HASH_ELEMENT* elm = qs_add_hash( _ppool, h_munit, hashelement[i].hashname_munit, hashelement[i].elm_munit, hashelement[i].id );
				if( elm != &hashelement[i] ){
					hashelement[i].hashname_munit = -1;
					hashelement[i].elm_munit = -1;
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

int32_t qs_remove_hash( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* hashname )
{
	struct QS_HASH *hash = (struct QS_HASH *)QS_GET_POINTER( _ppool, h_munit );
	struct QS_HASH *hashchild = (struct QS_HASH *)QS_GET_POINTER( _ppool, hash->hash_munit );
	uint32_t hashkey = qs_ihash( hashname, hash->hash_size );
	QS_HASH_ELEMENT *hashelement = (struct QS_HASH_ELEMENT*)QS_GET_POINTER( _ppool, hashchild[hashkey].hash_munit );
	uint32_t i;
	for( i = 0; i < hashchild[hashkey].hash_size; i++ )
	{
		if( hashelement[i].hashname_munit >= 0 )
		{
			if( !strcmp( (char*)QS_GET_POINTER( _ppool, hashelement[i].hashname_munit ), hashname ) )
			{
				hashelement[i].id = -1;
				hashelement[i].hashname_munit = -1;
				hashelement[i].elm_munit = -1;
				hashelement[i].create_time = 0;
				hashelement[i].life_time = 0;
				break;
			}
		}
	}
	return QS_SYSTEM_OK;
}

char* qs_get_hash_string( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* hashname )
{
	if( h_munit == -1 ){
		return NULL;
	}
	struct QS_HASH *hash = (struct QS_HASH *)QS_GET_POINTER( _ppool, h_munit );
	struct QS_HASH *hashchild = (struct QS_HASH *)QS_GET_POINTER( _ppool, hash->hash_munit );
	uint32_t hashkey = qs_ihash( hashname, hash->hash_size );
	QS_HASH_ELEMENT* elm = qs_get_hash_core( _ppool, hash, hashchild, hashname, hashkey );
	if(elm==NULL){
		return NULL;
	}
	if(elm->id != ELEMENT_LITERAL_STR){
		return NULL;
	}
	return (char*)QS_GET_POINTER(_ppool,elm->elm_munit);
}

int32_t* qs_get_hash_integer( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* hashname )
{
	if( h_munit == -1 ){
		return NULL;
	}
	struct QS_HASH *hash = (struct QS_HASH *)QS_GET_POINTER( _ppool, h_munit );
	struct QS_HASH *hashchild = (struct QS_HASH *)QS_GET_POINTER( _ppool, hash->hash_munit );
	uint32_t hashkey = qs_ihash( hashname, hash->hash_size );
	QS_HASH_ELEMENT* elm = qs_get_hash_core( _ppool, hash, hashchild, hashname, hashkey );
	if(elm==NULL){
		return NULL;
	}
	if(elm->id != ELEMENT_LITERAL_NUM){
		return NULL;
	}
	return QS_PINT32(_ppool,elm->elm_munit);
}

int32_t qs_get_hash( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* hashname )
{
	if( h_munit == -1 ){
		return -1;
	}
	struct QS_HASH *hash = (struct QS_HASH *)QS_GET_POINTER( _ppool, h_munit );
	struct QS_HASH *hashchild = (struct QS_HASH *)QS_GET_POINTER( _ppool, hash->hash_munit );
	uint32_t hashkey = qs_ihash( hashname, hash->hash_size );
	QS_HASH_ELEMENT* elm = qs_get_hash_core( _ppool, hash, hashchild, hashname, hashkey );
	if(elm==NULL){
		return -1;
	}
	return elm->elm_munit;
}

int32_t qs_get_hash_fix_ihash( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* hashname, uint32_t hashkey )
{
	if( h_munit == -1 ){
		return -1;
	}
	struct QS_HASH *hash = (struct QS_HASH *)QS_GET_POINTER( _ppool, h_munit );
	struct QS_HASH *hashchild = (struct QS_HASH *)QS_GET_POINTER( _ppool, hash->hash_munit );
	int32_t retmunit = -1;
	if( -1 == hashchild[hashkey].hash_munit )
	{
		//printf( "hash element is not found : %s\n", hashname );
		return retmunit;
	}
	QS_HASH_ELEMENT *hashelement = (struct QS_HASH_ELEMENT*)QS_GET_POINTER( _ppool, hashchild[hashkey].hash_munit );
	uint32_t i;
	for( i = 0; i < hashchild[hashkey].hash_size; i++ )
	{
		if( hashelement[i].hashname_munit >= 0 )
		{
			if( !strcmp( (char*)QS_GET_POINTER( _ppool, hashelement[i].hashname_munit ), hashname ) )
			{
				retmunit = hashelement[i].elm_munit;
				break;
			}
		}
	}
	return retmunit;
}

QS_HASH_ELEMENT* qs_get_hash_core( QS_MEMORY_POOL* _ppool, struct QS_HASH *hash, QS_HASH *hashchild, const char* hashname, uint32_t hashkey )
{
	QS_HASH_ELEMENT* ret = NULL;
	if( -1 == hashchild[hashkey].hash_munit )
	{
		//printf( "hash element is not found : %s\n", hashname );
		return NULL;
	}
	QS_HASH_ELEMENT *hashelement = (struct QS_HASH_ELEMENT*)QS_GET_POINTER( _ppool, hashchild[hashkey].hash_munit );
	uint32_t i;
	for( i = 0; i < hashchild[hashkey].hash_size; i++ )
	{
		if( hashelement[i].hashname_munit >= 0 )
		{
			if( !strcmp( (char*)QS_GET_POINTER( _ppool, hashelement[i].hashname_munit ), hashname ) )
			{
				ret = &hashelement[i];
				break;
			}
		}
	}
	return ret;
}

void qs_clear_hash_string( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* name )
{
	int32_t data_munit = qs_get_hash( _ppool, h_munit, name );
	if( data_munit >= 0 ){
		//memset( (char*)QS_GET_POINTER(_ppool,data_munit), 0, QS_PUNIT_USIZE(_ppool,data_munit) );
		*((char*)QS_GET_POINTER(_ppool,data_munit)) = '\0';
	}
}

int32_t qs_replace_hash_string( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* name, const char* value )
{
	int32_t data_munit = qs_get_hash( _ppool, h_munit, name );
	if( data_munit >= 0 ){
		memcpy( (char*)QS_GET_POINTER(_ppool,data_munit), value, QS_PUNIT_USIZE(_ppool,data_munit) );
		*( ((char*)QS_GET_POINTER(_ppool,data_munit))+QS_PUNIT_USIZE(_ppool,data_munit)-1 ) = '\0';
	}
	return data_munit;
}

int32_t qs_get_hash_name( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* hashname )
{
	int32_t retmunit = -1;
	QS_HASH *hash;
	QS_HASH *hashchild;
	uint32_t hashkey;
	hash = (QS_HASH *)QS_GET_POINTER( _ppool, h_munit );
	if( hash->hash_size == 0 ){
		return retmunit;
	}
	hashkey = qs_ihash( hashname, hash->hash_size );
	hashchild = (QS_HASH *)QS_GET_POINTER( _ppool, hash->hash_munit );
	if( -1 == hashchild[hashkey].hash_munit ){
		return retmunit;
	}
	QS_HASH_ELEMENT *hashelement = (QS_HASH_ELEMENT*)QS_GET_POINTER( _ppool, hashchild[hashkey].hash_munit );
	uint32_t i;
	for( i = 0; i < hashchild[hashkey].hash_size; ++i ){
		if( hashelement[i].hashname_munit != -1 ){
			if( !strcmp( (char*)QS_GET_POINTER( _ppool, hashelement[i].hashname_munit ), hashname ) ){
				retmunit = hashelement[i].hashname_munit;
				break;
			}
		}
	}
	return retmunit;
}

int32_t qs_get_hash_id( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* hashname )
{
	int32_t retid = -1;
	QS_HASH *hash;
	QS_HASH *hashchild;
	uint32_t hashkey;
	hash = (QS_HASH *)QS_GET_POINTER( _ppool, h_munit );
	hashkey = qs_ihash( hashname, hash->hash_size );
	hashchild = (QS_HASH *)QS_GET_POINTER( _ppool, hash->hash_munit );
	if( -1 == hashchild[hashkey].hash_munit )
	{
		return retid;
	}
	QS_HASH_ELEMENT *hashelement = (QS_HASH_ELEMENT*)QS_GET_POINTER( _ppool, hashchild[hashkey].hash_munit );
	uint32_t i;
	for( i = 0; i < hashchild[hashkey].hash_size; i++ )
	{
		if( hashelement[i].hashname_munit >= 0 )
		{
			if( !strcmp( (char*)QS_GET_POINTER( _ppool, hashelement[i].hashname_munit ), hashname ) )
			{
				retid = hashelement[i].id;
				break;
			}
		}
	}
	return retid;
}

QS_HASH_ELEMENT* qs_get_hash_element( QS_MEMORY_POOL* _ppool, int32_t h_munit, const char* hashname )
{
	QS_HASH_ELEMENT* ret = NULL;
	QS_HASH *hash;
	QS_HASH *hashchild;
	uint32_t hashkey;
	hash = (QS_HASH *)QS_GET_POINTER( _ppool, h_munit );
	hashkey = qs_ihash( hashname, hash->hash_size );
	hashchild = (QS_HASH *)QS_GET_POINTER( _ppool, hash->hash_munit );
	if( -1 == hashchild[hashkey].hash_munit )
	{
		return ret;
	}
	QS_HASH_ELEMENT *hashelement = (QS_HASH_ELEMENT*)QS_GET_POINTER( _ppool, hashchild[hashkey].hash_munit );
	uint32_t i;
	for( i = 0; i < hashchild[hashkey].hash_size; i++ )
	{
		if( hashelement[i].hashname_munit >= 0 )
		{
			if( !strcmp( (char*)QS_GET_POINTER( _ppool, hashelement[i].hashname_munit ), hashname ) )
			{
				ret = &hashelement[i];
				break;
			}
		}
	}
	return ret;
}

int32_t qs_hash_length( QS_MEMORY_POOL* _ppool, int32_t h_munit )
{
	int32_t length = 0;
	struct QS_HASH *hash;
	struct QS_HASH *hashchild;
	struct QS_HASH_ELEMENT *hashelement;
	uint32_t i,j;
	hash = (struct QS_HASH *)QS_GET_POINTER( _ppool, h_munit );
	hashchild = (struct QS_HASH *)QS_GET_POINTER( _ppool, hash->hash_munit );
	for( j = 0; j < hash->hash_size; j++ )
	{
		if( hashchild[j].hash_munit >= 0 )
		{
			hashelement = (struct QS_HASH_ELEMENT*)QS_GET_POINTER( _ppool, hashchild[j].hash_munit );
			for( i = 0; i < hashchild[j].hash_size; i++ )
			{
				if( hashelement[i].hashname_munit >= 0 )
				{
					++length;
				}
			}
		}
	}
	return length;
}


int32_t qs_init_hash_foreach( QS_MEMORY_POOL* _ppool, int32_t h_munit, QS_HASH_FOREACH* hf )
{
	hf->p1 = 0;
	hf->p2 = 0;
	hf->root = (QS_HASH*)QS_GET_POINTER( _ppool, h_munit );
	hf->child = (QS_HASH*)QS_GET_POINTER( _ppool, hf->root->hash_munit );
	hf->he = NULL;
	return QS_SYSTEM_OK;
}


QS_HASH_ELEMENT* qs_hash_foreach( QS_MEMORY_POOL* _ppool, QS_HASH_FOREACH* hf )
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
			hf->he = (QS_HASH_ELEMENT*)QS_GET_POINTER( _ppool, hf->child[hf->p1]. hash_munit );
		}
		if( hf->p2 >= hf->child[hf->p1].hash_size ){
			hf->he = NULL;
			hf->p2 = 0;
			++hf->p1;
			continue;
		}
		if( hf->he[hf->p2].hashname_munit == -1 ){
			++hf->p2;
			continue;
		}
		ret = &hf->he[hf->p2];
		++hf->p2;
		break;
	}while( true );
	return ret;
}
