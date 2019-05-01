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

#include "gdt_hash.h"

int32_t gdt_create_hash( GDT_MEMORY_POOL* _ppool, size_t hlen )
{
	int32_t h_munit = -1;
	if( hlen <= 0 ){
		return -1;
	}
	if( -1 == (h_munit = gdt_create_munit( _ppool, sizeof( struct GDT_HASH ), MEMORY_TYPE_DEFAULT )) ){
		return -1;
	}
	GDT_HASH *hash = (struct GDT_HASH *)GDT_POINTER( _ppool, h_munit );
	hash->hash_size = hlen;
	hash->hash_munit = gdt_create_munit( _ppool, sizeof( struct GDT_HASH ) * hlen, MEMORY_TYPE_DEFAULT );
	if( -1 == hash->hash_munit ){
		return -1;
	}
	GDT_HASH *hashchild = (struct GDT_HASH *)GDT_POINTER( _ppool, hash->hash_munit );
	uint32_t i;
	for( i = 0; i < hlen; i++ )
	{
		hashchild[i].hash_size	= 0;
		hashchild[i].hash_munit	= -1;
	}
	return h_munit;
}

GDT_HASH_ELEMENT* gdt_add_hash( GDT_MEMORY_POOL* _ppool, int32_t h_munit, int32_t name_munit, int32_t data_munit, int32_t id )
{
	struct GDT_HASH *hash;
	struct GDT_HASH *hashchild;
	struct GDT_HASH_ELEMENT *hashelement;
	uint32_t hashkey;
	char* pbuf;
	uint32_t i;
	GDT_HASH_ELEMENT* is_push = NULL;
	if( -1 == h_munit ){
		return is_push;
	}
	hash = (struct GDT_HASH *)GDT_POINTER( _ppool, h_munit );
	hashkey = gdt_ihash( (char*)GDT_POINTER( _ppool, name_munit ), hash->hash_size );
	pbuf = (char*)GDT_POINTER( _ppool, name_munit );
	hashchild = (struct GDT_HASH *)GDT_POINTER( _ppool, hash->hash_munit );
	if( -1 == hashchild[hashkey].hash_munit )
	{
		if( -1 == (hashchild[hashkey].hash_munit = gdt_create_munit( _ppool, sizeof( struct GDT_HASH_ELEMENT ) * GDT_HASH_ELEMENT_SIZE, MEMORY_TYPE_DEFAULT ))){
			return is_push;
		}
		hashchild[hashkey].hash_size = GDT_HASH_ELEMENT_SIZE;
		hashelement = (struct GDT_HASH_ELEMENT*)GDT_POINTER( _ppool, hashchild[hashkey].hash_munit );
		for( i = 0; i < hashchild[hashkey].hash_size; i++ )
		{
			hashelement[i].hashname_munit = -1;
			hashelement[i].elm_munit = -1;
			hashelement[i].id = -1;
			hashelement[i].create_time = 0;
			hashelement[i].life_time = 0;
		}
	}
	hashelement = (struct GDT_HASH_ELEMENT*)GDT_POINTER( _ppool, hashchild[hashkey].hash_munit );
	for( i = 0; i < hashchild[hashkey].hash_size; i++ )
	{
		if( hashelement[i].hashname_munit >= 0 )
		{
			if( !strcmp( (char*)GDT_POINTER( _ppool, hashelement[i].hashname_munit ), pbuf ) )
			{
				if( hashelement[i].elm_munit != data_munit ){
					gdt_free_memory_unit( _ppool, &hashelement[i].elm_munit );
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
		size_t resize = hashchild[hashkey].hash_size * 2;
		//printf("resize hash : %d -> %d\n",(int)(hashchild[hashkey].hash_size),(int)(resize));
		if( -1 == (resize_munit = gdt_create_munit( _ppool, sizeof( struct GDT_HASH_ELEMENT ) * resize, MEMORY_TYPE_DEFAULT ))){
			return is_push;
		}
		hashelement = (struct GDT_HASH_ELEMENT*)GDT_POINTER( _ppool, resize_munit );
		GDT_HASH_ELEMENT *oldhashelement = (struct GDT_HASH_ELEMENT*)GDT_POINTER( _ppool, hashchild[hashkey].hash_munit );
		for( i = 0; i < resize; i++ ){
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

int32_t gdt_make_hash_name( GDT_MEMORY_POOL* _ppool, int32_t h_munit,const char* name)
{
	int32_t name_munit = gdt_get_hash_name( _ppool, h_munit, name );
	if( name_munit == -1 ){
		if( -1 == (name_munit = gdt_create_munit( _ppool, strlen( name )+1, MEMORY_TYPE_DEFAULT ))){
			return -1;
		}
		char* pbuf = (char*)GDT_POINTER( _ppool, name_munit );
		gdt_strcopy( pbuf, name, gdt_usize( _ppool, name_munit ) );
	}
	return name_munit;
}

void gdt_add_hash_value( GDT_MEMORY_POOL* _ppool, int32_t h_munit, const char* name, const char* value, int32_t id )
{
	char* pbuf;
	int32_t datamunit;
	int32_t namemunit = gdt_make_hash_name( _ppool, h_munit, name );
	if( namemunit == -1 ){
		return;
	}
	if( -1 == (datamunit = gdt_create_munit( _ppool, strlen( value )+1, MEMORY_TYPE_DEFAULT ))){
		return;
	}
	pbuf = (char*)GDT_POINTER( _ppool, datamunit );
	memcpy( pbuf, value, gdt_usize( _ppool, datamunit ) );
	GDT_HASH_ELEMENT* is_push = gdt_add_hash( _ppool, h_munit, namemunit, datamunit, id );
	if( NULL==is_push){
		printf("is_push is NULL\n");
	}
}

void gdt_add_hash_array_string( GDT_MEMORY_POOL* _ppool, int32_t h_munit, const char* name, const char* value )
{
	int32_t namemunit = gdt_make_hash_name( _ppool, h_munit, name );
	if( namemunit == -1 ){
		return;
	}
	int32_t array_munit = gdt_get_hash( _ppool, h_munit, name );
	if( array_munit == -1 ){
		array_munit = gdt_create_array( _ppool, 8, 0 );
		if( -1 == array_munit ){
			return;
		}
	}
	if( GDT_SYSTEM_ERROR == gdt_array_push_string( _ppool, &array_munit, value ) ){
		return;
	}
	GDT_HASH_ELEMENT* is_push = gdt_add_hash( _ppool, h_munit, namemunit, array_munit, ELEMENT_ARRAY );
	if( NULL==is_push){
		printf("is_push is NULL\n");
	}
}

void gdt_add_hash_array_empty_string( GDT_MEMORY_POOL* _ppool, int32_t h_munit, const char* name, size_t size )
{
	int32_t namemunit = gdt_make_hash_name( _ppool, h_munit, name );
	if( namemunit == -1 ){
		return;
	}
	int32_t array_munit = gdt_get_hash( _ppool, h_munit, name );
	if( array_munit == -1 ){
		array_munit = gdt_create_array( _ppool, 8, 0 );
		if( -1 == array_munit ){
			return;
		}
	}
	if( GDT_SYSTEM_ERROR == gdt_array_push_empty_string( _ppool, &array_munit, size ) ){
		return;
	}
	GDT_HASH_ELEMENT* is_push = gdt_add_hash( _ppool, h_munit, namemunit, array_munit, ELEMENT_ARRAY );
	if( NULL==is_push){
		printf("is_push is NULL\n");
	}
}

void gdt_add_hash_binary( GDT_MEMORY_POOL* _ppool, int32_t h_munit, const char* name, uint8_t* binary, size_t size )
{
	uint8_t* bin;
	int32_t namemunit = gdt_make_hash_name( _ppool, h_munit, name );
	if( namemunit == -1 ){
		return;
	}
	int32_t datamunit = gdt_get_hash( _ppool, h_munit, name );
	if( datamunit == -1 ){
		if( -1 == ( datamunit = gdt_create_munit( _ppool, size, MEMORY_TYPE_DEFAULT ) ) ){
			return;
		}
	}
	else{
		if( gdt_usize(_ppool,datamunit) < size ){
			if( -1 == ( datamunit = gdt_create_munit( _ppool, size, MEMORY_TYPE_DEFAULT ) ) ){
				return;
			}
		}
	}
	bin = (uint8_t*)GDT_POINTER( _ppool, datamunit );
	memcpy( bin, binary, size );
	GDT_HASH_ELEMENT* is_push = gdt_add_hash( _ppool, h_munit, namemunit, datamunit, ELEMENT_LITERAL_BIN );
	if( NULL==is_push){
		printf("is_push is NULL\n");
	}
}

GDT_HASH_ELEMENT* gdt_add_hash_integer( GDT_MEMORY_POOL* _ppool, int32_t h_munit, const char* name, int32_t value )
{
	int32_t namemunit = gdt_make_hash_name( _ppool, h_munit, name );
	if( namemunit == -1 ){
		return NULL;
	}
	int32_t datamunit = gdt_get_hash( _ppool, h_munit, name );
	if( datamunit == -1 ){
		if( -1 == ( datamunit = gdt_create_munit( _ppool, NUMERIC_BUFFER_SIZE, MEMORY_TYPE_DEFAULT ) ) ){
			return NULL;
		}
	}
	gdt_itoa( value, (char*)GDT_POINTER(_ppool,datamunit), gdt_usize(_ppool,datamunit) );
	(*(int32_t*)(GDT_POINTER(_ppool,datamunit)+gdt_usize(_ppool,datamunit)-sizeof(int32_t))) = value;
	GDT_HASH_ELEMENT* is_push = gdt_add_hash( _ppool, h_munit, namemunit, datamunit, ELEMENT_LITERAL_NUM );
	//if( NULL==is_push){ printf("is_push is NULL\n"); }
	return is_push;
}

void gdt_add_hash_integer_kint( GDT_MEMORY_POOL* _ppool, int32_t h_munit, int32_t name_munit, int32_t value )
{
	int32_t datamunit;
	if( -1 == ( datamunit = gdt_create_munit( _ppool, NUMERIC_BUFFER_SIZE, MEMORY_TYPE_DEFAULT ) ) ){
		return;
	}
	gdt_itoa( value, (char*)GDT_POINTER(_ppool,datamunit), gdt_usize(_ppool,datamunit) );
	(*(int32_t*)(GDT_POINTER(_ppool,datamunit)+gdt_usize(_ppool,datamunit)-sizeof(int32_t))) = value;
	GDT_HASH_ELEMENT* is_push = gdt_add_hash( _ppool, h_munit, name_munit, datamunit, ELEMENT_LITERAL_NUM );
	if( NULL==is_push){
		printf("is_push is NULL\n");
	}
}

GDT_HASH_ELEMENT* gdt_add_hash_string( GDT_MEMORY_POOL* _ppool, int32_t h_munit, const char* name, const char* value )
{
	char* pbuf;
	int32_t namemunit = gdt_make_hash_name( _ppool, h_munit, name );
	if( namemunit == -1 ){
		return NULL;
	}
	int32_t datamunit = gdt_get_hash( _ppool, h_munit, name );
	if( datamunit == -1 ){
		if( -1 == ( datamunit = gdt_create_munit( _ppool, strlen( value )+1, MEMORY_TYPE_DEFAULT ) ) ){
			return NULL;
		}
	}
	else{
		size_t size = strlen( value )+1;
		if( gdt_usize(_ppool,datamunit) < size ){
			if( -1 == ( datamunit = gdt_create_munit( _ppool, strlen( value )+1, MEMORY_TYPE_DEFAULT ) ) ){
				return NULL;
			}
		}
	}
	pbuf = (char*)GDT_POINTER( _ppool, datamunit );
	memcpy( pbuf, value, gdt_usize( _ppool, datamunit ) );
	GDT_HASH_ELEMENT* is_push = gdt_add_hash( _ppool, h_munit, namemunit, datamunit, ELEMENT_LITERAL_STR );
	//if( NULL==is_push){printf("is_push is NULL\n");}
	return is_push;
}

void gdt_add_hash_emptystring( GDT_MEMORY_POOL* _ppool, int32_t h_munit, const char* name, size_t string_size )
{
	char* pbuf;
	int32_t datamunit;
	int32_t namemunit = gdt_make_hash_name( _ppool, h_munit, name );
	if( namemunit == -1 ){
		return;
	}
	if( -1 == ( datamunit = gdt_create_munit( _ppool, string_size, MEMORY_TYPE_DEFAULT ) ) ){
		return;
	}
	pbuf = (char*)GDT_POINTER( _ppool, datamunit );
	//memset( pbuf, 0, gdt_usize( _ppool, datamunit ) );
	pbuf[0] = '\0';
	GDT_HASH_ELEMENT* is_push = gdt_add_hash( _ppool, h_munit, namemunit, datamunit, ELEMENT_LITERAL_STR );
	if( NULL==is_push){
		printf("is_push is NULL\n");
	}
}

void gdt_add_hash_value_kstring( GDT_MEMORY_POOL* _ppool, int32_t h_munit, const char* name, int32_t data_munit, int32_t id )
{
	int32_t namemunit = gdt_make_hash_name( _ppool, h_munit, name );
	if( namemunit == -1 ){
		return;
	}
	GDT_HASH_ELEMENT* is_push = gdt_add_hash( _ppool, h_munit, namemunit, data_munit, id );
	if( NULL==is_push){
		printf("is_push is NULL\n");
	}
}

int32_t gdt_move_hash( GDT_MEMORY_POOL* _ppool, int32_t h_munit, const char* hashname_from, const char* hashname_to )
{
	if( h_munit == -1 ){
		return -1;
	}
	struct GDT_HASH *hash = (struct GDT_HASH *)GDT_POINTER( _ppool, h_munit );
	struct GDT_HASH *hashchild = (struct GDT_HASH *)GDT_POINTER( _ppool, hash->hash_munit );
	uint32_t hashkey = gdt_ihash( hashname_from, hash->hash_size );
	int32_t retmunit = -1;
	if( -1 == hashchild[hashkey].hash_munit ){
		return retmunit;
	}
	if( gdt_strlen(hashname_from) != gdt_strlen(hashname_to) ){
		return retmunit;
	}
	GDT_HASH_ELEMENT *hashelement = (struct GDT_HASH_ELEMENT*)GDT_POINTER( _ppool, hashchild[hashkey].hash_munit );
	uint32_t i;
	for( i = 0; i < hashchild[hashkey].hash_size; i++ )
	{
		if( hashelement[i].hashname_munit >= 0 )
		{
			char* tmpname = (char*)GDT_POINTER( _ppool, hashelement[i].hashname_munit );
			if( !strcmp( tmpname, hashname_from ) )
			{
				memcpy(tmpname,hashname_to,gdt_strlen(hashname_to));
				GDT_HASH_ELEMENT* elm = gdt_add_hash( _ppool, h_munit, hashelement[i].hashname_munit, hashelement[i].elm_munit, hashelement[i].id );
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

int32_t gdt_remove_hash( GDT_MEMORY_POOL* _ppool, int32_t h_munit, const char* hashname )
{
	struct GDT_HASH *hash = (struct GDT_HASH *)GDT_POINTER( _ppool, h_munit );
	struct GDT_HASH *hashchild = (struct GDT_HASH *)GDT_POINTER( _ppool, hash->hash_munit );
	uint32_t hashkey = gdt_ihash( hashname, hash->hash_size );
	GDT_HASH_ELEMENT *hashelement = (struct GDT_HASH_ELEMENT*)GDT_POINTER( _ppool, hashchild[hashkey].hash_munit );
	uint32_t i;
	for( i = 0; i < hashchild[hashkey].hash_size; i++ )
	{
		if( hashelement[i].hashname_munit >= 0 )
		{
			if( !strcmp( (char*)GDT_POINTER( _ppool, hashelement[i].hashname_munit ), hashname ) )
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
	return GDT_SYSTEM_OK;
}

int32_t gdt_get_hash( GDT_MEMORY_POOL* _ppool, int32_t h_munit, const char* hashname )
{
	if( h_munit == -1 ){
		return -1;
	}
	struct GDT_HASH *hash = (struct GDT_HASH *)GDT_POINTER( _ppool, h_munit );
	struct GDT_HASH *hashchild = (struct GDT_HASH *)GDT_POINTER( _ppool, hash->hash_munit );
	uint32_t hashkey = gdt_ihash( hashname, hash->hash_size );
	GDT_HASH_ELEMENT* elm = gdt_get_hash_core( _ppool, hash, hashchild, hashname, hashkey );
	if(elm==NULL){
		return -1;
	}
	return elm->elm_munit;
}

int32_t gdt_get_hash_fix_ihash( GDT_MEMORY_POOL* _ppool, int32_t h_munit, const char* hashname, uint32_t hashkey )
{
	if( h_munit == -1 ){
		return -1;
	}
	struct GDT_HASH *hash = (struct GDT_HASH *)GDT_POINTER( _ppool, h_munit );
	struct GDT_HASH *hashchild = (struct GDT_HASH *)GDT_POINTER( _ppool, hash->hash_munit );
	int32_t retmunit = -1;
	if( -1 == hashchild[hashkey].hash_munit )
	{
		//printf( "hash element is not found : %s\n", hashname );
		return retmunit;
	}
	GDT_HASH_ELEMENT *hashelement = (struct GDT_HASH_ELEMENT*)GDT_POINTER( _ppool, hashchild[hashkey].hash_munit );
	uint32_t i;
	for( i = 0; i < hashchild[hashkey].hash_size; i++ )
	{
		if( hashelement[i].hashname_munit >= 0 )
		{
			if( !strcmp( (char*)GDT_POINTER( _ppool, hashelement[i].hashname_munit ), hashname ) )
			{
				retmunit = hashelement[i].elm_munit;
				break;
			}
		}
	}
	return retmunit;
}

GDT_HASH_ELEMENT* gdt_get_hash_core( GDT_MEMORY_POOL* _ppool, struct GDT_HASH *hash, GDT_HASH *hashchild, const char* hashname, uint32_t hashkey )
{
	GDT_HASH_ELEMENT* ret = NULL;
	if( -1 == hashchild[hashkey].hash_munit )
	{
		//printf( "hash element is not found : %s\n", hashname );
		return NULL;
	}
	GDT_HASH_ELEMENT *hashelement = (struct GDT_HASH_ELEMENT*)GDT_POINTER( _ppool, hashchild[hashkey].hash_munit );
	uint32_t i;
	for( i = 0; i < hashchild[hashkey].hash_size; i++ )
	{
		if( hashelement[i].hashname_munit >= 0 )
		{
			if( !strcmp( (char*)GDT_POINTER( _ppool, hashelement[i].hashname_munit ), hashname ) )
			{
				ret = &hashelement[i];
				break;
			}
		}
	}
	return ret;
}

void gdt_clear_hash_string( GDT_MEMORY_POOL* _ppool, int32_t h_munit, const char* name )
{
	int32_t data_munit = gdt_get_hash( _ppool, h_munit, name );
	if( data_munit >= 0 ){
		//memset( (char*)GDT_POINTER(_ppool,data_munit), 0, GDT_PUNIT_USIZE(_ppool,data_munit) );
		*((char*)GDT_POINTER(_ppool,data_munit)) = '\0';
	}
}

int32_t gdt_replace_hash_string( GDT_MEMORY_POOL* _ppool, int32_t h_munit, const char* name, const char* value )
{
	int32_t data_munit = gdt_get_hash( _ppool, h_munit, name );
	if( data_munit >= 0 ){
		memcpy( (char*)GDT_POINTER(_ppool,data_munit), value, GDT_PUNIT_USIZE(_ppool,data_munit) );
		*( ((char*)GDT_POINTER(_ppool,data_munit))+GDT_PUNIT_USIZE(_ppool,data_munit)-1 ) = '\0';
	}
	return data_munit;
}

int32_t gdt_get_hash_name( GDT_MEMORY_POOL* _ppool, int32_t h_munit, const char* hashname )
{
	int32_t retmunit = -1;
	struct GDT_HASH *hash;
	struct GDT_HASH *hashchild;
	uint32_t hashkey;
	hash = (struct GDT_HASH *)GDT_POINTER( _ppool, h_munit );
	if( hash->hash_size == 0 ){
		return retmunit;
	}
	hashkey = gdt_ihash( hashname, hash->hash_size );
	hashchild = (struct GDT_HASH *)GDT_POINTER( _ppool, hash->hash_munit );
	if( -1 == hashchild[hashkey].hash_munit ){
		return retmunit;
	}
	GDT_HASH_ELEMENT *hashelement = (struct GDT_HASH_ELEMENT*)GDT_POINTER( _ppool, hashchild[hashkey].hash_munit );
	uint32_t i;
	for( i = 0; i < hashchild[hashkey].hash_size; i++ ){
		if( hashelement[i].hashname_munit >= 0 ){
			if( !strcmp( (char*)GDT_POINTER( _ppool, hashelement[i].hashname_munit ), hashname ) ){
				retmunit = hashelement[i].hashname_munit;
				break;
			}
		}
	}
	return retmunit;
}

int32_t gdt_get_hash_id( GDT_MEMORY_POOL* _ppool, int32_t h_munit, const char* hashname )
{
	int32_t retid = -1;
	GDT_HASH *hash;
	GDT_HASH *hashchild;
	uint32_t hashkey;
	hash = (GDT_HASH *)GDT_POINTER( _ppool, h_munit );
	hashkey = gdt_ihash( hashname, hash->hash_size );
	hashchild = (GDT_HASH *)GDT_POINTER( _ppool, hash->hash_munit );
	if( -1 == hashchild[hashkey].hash_munit )
	{
		return retid;
	}
	GDT_HASH_ELEMENT *hashelement = (GDT_HASH_ELEMENT*)GDT_POINTER( _ppool, hashchild[hashkey].hash_munit );
	uint32_t i;
	for( i = 0; i < hashchild[hashkey].hash_size; i++ )
	{
		if( hashelement[i].hashname_munit >= 0 )
		{
			if( !strcmp( (char*)GDT_POINTER( _ppool, hashelement[i].hashname_munit ), hashname ) )
			{
				retid = hashelement[i].id;
				break;
			}
		}
	}
	return retid;
}

GDT_HASH_ELEMENT* gdt_get_hash_element( GDT_MEMORY_POOL* _ppool, int32_t h_munit, const char* hashname )
{
	GDT_HASH_ELEMENT* ret = NULL;
	GDT_HASH *hash;
	GDT_HASH *hashchild;
	uint32_t hashkey;
	hash = (GDT_HASH *)GDT_POINTER( _ppool, h_munit );
	hashkey = gdt_ihash( hashname, hash->hash_size );
	hashchild = (GDT_HASH *)GDT_POINTER( _ppool, hash->hash_munit );
	if( -1 == hashchild[hashkey].hash_munit )
	{
		return ret;
	}
	GDT_HASH_ELEMENT *hashelement = (GDT_HASH_ELEMENT*)GDT_POINTER( _ppool, hashchild[hashkey].hash_munit );
	uint32_t i;
	for( i = 0; i < hashchild[hashkey].hash_size; i++ )
	{
		if( hashelement[i].hashname_munit >= 0 )
		{
			if( !strcmp( (char*)GDT_POINTER( _ppool, hashelement[i].hashname_munit ), hashname ) )
			{
				ret = &hashelement[i];
				break;
			}
		}
	}
	return ret;
}

int32_t gdt_hash_length( GDT_MEMORY_POOL* _ppool, int32_t h_munit )
{
	int32_t length = 0;
	struct GDT_HASH *hash;
	struct GDT_HASH *hashchild;
	struct GDT_HASH_ELEMENT *hashelement;
	uint32_t i,j;
	hash = (struct GDT_HASH *)GDT_POINTER( _ppool, h_munit );
	hashchild = (struct GDT_HASH *)GDT_POINTER( _ppool, hash->hash_munit );
	for( j = 0; j < hash->hash_size; j++ )
	{
		if( hashchild[j].hash_munit >= 0 )
		{
			hashelement = (struct GDT_HASH_ELEMENT*)GDT_POINTER( _ppool, hashchild[j].hash_munit );
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


int32_t gdt_init_hash_foreach( GDT_MEMORY_POOL* _ppool, int32_t h_munit, GDT_HASH_FOREACH* hf )
{
	hf->p1 = 0;
	hf->p2 = 0;
	hf->root = (GDT_HASH*)GDT_POINTER( _ppool, h_munit );
	hf->child = (GDT_HASH*)GDT_POINTER( _ppool, hf->root->hash_munit );
	hf->he = NULL;
	return GDT_SYSTEM_OK;
}


GDT_HASH_ELEMENT* gdt_hash_foreach( GDT_MEMORY_POOL* _ppool, GDT_HASH_FOREACH* hf )
{
	GDT_HASH_ELEMENT* ret = NULL;
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
			hf->he = (GDT_HASH_ELEMENT*)GDT_POINTER( _ppool, hf->child[hf->p1]. hash_munit );
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

