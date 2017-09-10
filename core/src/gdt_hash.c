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
	if( 0 <= (h_munit = gdt_create_munit( _ppool, sizeof( struct GDT_HASH ), MEMORY_TYPE_DEFAULT )) )
	{
		GDT_HASH *hash = (struct GDT_HASH *)GDT_POINTER( _ppool, h_munit );
		hash->hash_size = hlen;
		hash->hash_munit = gdt_create_munit( _ppool, sizeof( struct GDT_HASH ) * hlen, MEMORY_TYPE_DEFAULT );
		if( 0 >= hash->hash_munit ){
			return -1;
		}
		GDT_HASH *hashchild = (struct GDT_HASH *)GDT_POINTER( _ppool, hash->hash_munit );
		uint32_t i;
		for( i = 0; i < hlen; i++ )
		{
			hashchild[i].hash_size	= 0;
			hashchild[i].hash_munit	= -1;
		}
	}
	return h_munit;
}

int32_t gdt_add_hash( GDT_MEMORY_POOL* _ppool, int32_t h_munit, int32_t name_munit, int32_t data_munit, int32_t id )
{
	struct GDT_HASH *hash;
	struct GDT_HASH *hashchild;
	struct GDT_HASH_ELEMENT *hashelement;
	uint32_t hashkey;
	char* pbuf;
	uint32_t i;
	int32_t is_push = 0;
	if( h_munit < 0 ){
		return GDT_SYSTEM_ERROR;
	}
	hash = (struct GDT_HASH *)GDT_POINTER( _ppool, h_munit );
	hashkey = gdt_ihash( (char*)GDT_POINTER( _ppool, name_munit ), hash->hash_size );
	pbuf = (char*)GDT_POINTER( _ppool, name_munit );
	hashchild = (struct GDT_HASH *)GDT_POINTER( _ppool, hash->hash_munit );
	if( hashchild[hashkey].hash_munit < 0 )
	{
		if( 0 >= (hashchild[hashkey].hash_munit = gdt_create_munit( _ppool, sizeof( struct GDT_HASH_ELEMENT ) * 16, MEMORY_TYPE_DEFAULT ))){
			return GDT_SYSTEM_ERROR;
		}
		hashchild[hashkey].hash_size = 16;
		hashelement = (struct GDT_HASH_ELEMENT*)GDT_POINTER( _ppool, hashchild[hashkey].hash_munit );
		for( i = 0; i < hashchild[hashkey].hash_size; i++ )
		{
			hashelement[i].hashname_munit = -1;
			hashelement[i].elm_munit = -1;
			hashelement[i].id = -1;
		}
	}
	hashelement = (struct GDT_HASH_ELEMENT*)GDT_POINTER( _ppool, hashchild[hashkey].hash_munit );
	for( i = 0; i < hashchild[hashkey].hash_size; i++ )
	{
		if( hashelement[i].hashname_munit > 0 )
		{
			if( !strcmp( (char*)GDT_POINTER( _ppool, hashelement[i].hashname_munit ), pbuf ) )
			{
				if( hashelement[i].elm_munit != data_munit ){
					gdt_free_memory_unit( _ppool, &hashelement[i].elm_munit );
				}
				hashelement[i].elm_munit = data_munit;
				hashelement[i].id = id;
				is_push = 1;
				break;
			}
		}
		else{
			hashelement[i].hashname_munit = name_munit;
			hashelement[i].elm_munit = data_munit;
			hashelement[i].id = id;
			is_push = 1;
			break;
		}
	}
	if( is_push == 0 ){
		printf("hash size over\n");
		return  GDT_SYSTEM_ERROR;
	}
	return  GDT_SYSTEM_OK;
}

int32_t gdt_make_hash_name( GDT_MEMORY_POOL* _ppool, int32_t h_munit,const char* name)
{
	int32_t name_munit = gdt_get_hash_name( _ppool, h_munit, name );
	if( name_munit == -1 ){
		if( -1 == (name_munit = gdt_create_munit( _ppool, strlen( name )+1, MEMORY_TYPE_DEFAULT ))){
			printf("memory error\n");
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
	if( 0 > (datamunit = gdt_create_munit( _ppool, strlen( value )+1, MEMORY_TYPE_DEFAULT ))){
		printf("memory error\n");
		return;
	}
	pbuf = (char*)GDT_POINTER( _ppool, datamunit );
	memcpy( pbuf, value, gdt_usize( _ppool, datamunit ) );
	gdt_add_hash( _ppool, h_munit, namemunit, datamunit, id );
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
	gdt_add_hash( _ppool, h_munit, namemunit, array_munit, ELEMENT_ARRAY );
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
	gdt_add_hash( _ppool, h_munit, namemunit, array_munit, ELEMENT_ARRAY );
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
		if( -1 == (	datamunit = gdt_create_munit( _ppool, size, MEMORY_TYPE_DEFAULT ) ) ){
			return;
		}
	}
	else{
		if( gdt_usize(_ppool,datamunit) < size ){
			if( -1 == (	datamunit = gdt_create_munit( _ppool, size, MEMORY_TYPE_DEFAULT ) ) ){
				return;
			}
		}
	}
	bin = (uint8_t*)GDT_POINTER( _ppool, datamunit );
	memcpy( bin, binary, size );
	gdt_add_hash( _ppool, h_munit, namemunit, datamunit, ELEMENT_LITERAL_BIN );
}

void gdt_add_hash_integer( GDT_MEMORY_POOL* _ppool, int32_t h_munit, const char* name, int32_t value )
{
	int32_t namemunit = gdt_make_hash_name( _ppool, h_munit, name );
	if( namemunit == -1 ){
		return;
	}
	int32_t datamunit = gdt_get_hash( _ppool, h_munit, name );
	if( datamunit == -1 ){
		if( -1 == (	datamunit = gdt_create_munit( _ppool, NUMERIC_BUFFER_SIZE, MEMORY_TYPE_DEFAULT ) ) ){
			return;
		}
	}
	gdt_itoa( value, (char*)GDT_POINTER(_ppool,datamunit), gdt_usize(_ppool,datamunit) );
	(*(int32_t*)(GDT_POINTER(_ppool,datamunit)+gdt_usize(_ppool,datamunit)-sizeof(int32_t))) = value;
	gdt_add_hash( _ppool, h_munit, namemunit, datamunit, ELEMENT_LITERAL_NUM );
}

void gdt_add_hash_integer_kint( GDT_MEMORY_POOL* _ppool, int32_t h_munit, int32_t name_munit, int32_t value )
{
	int32_t datamunit;
	if( -1 == (	datamunit = gdt_create_munit( _ppool, NUMERIC_BUFFER_SIZE, MEMORY_TYPE_DEFAULT ) ) ){
		return;
	}
	gdt_itoa( value, (char*)GDT_POINTER(_ppool,datamunit), gdt_usize(_ppool,datamunit) );
	(*(int32_t*)(GDT_POINTER(_ppool,datamunit)+gdt_usize(_ppool,datamunit)-sizeof(int32_t))) = value;
	gdt_add_hash( _ppool, h_munit, name_munit, datamunit, ELEMENT_LITERAL_NUM );
}

void gdt_add_hash_string( GDT_MEMORY_POOL* _ppool, int32_t h_munit, const char* name, const char* value )
{
	char* pbuf;
	int32_t namemunit = gdt_make_hash_name( _ppool, h_munit, name );
	if( namemunit == -1 ){
		return;
	}
	int32_t datamunit = gdt_get_hash( _ppool, h_munit, name );
	if( datamunit == -1 ){
		if( -1 == (	datamunit = gdt_create_munit( _ppool, strlen( value )+1, MEMORY_TYPE_DEFAULT ) ) ){
			return;
		}
	}
	else{
		size_t size = strlen( value )+1;
		if( gdt_usize(_ppool,datamunit) < size ){
			if( -1 == (	datamunit = gdt_create_munit( _ppool, strlen( value )+1, MEMORY_TYPE_DEFAULT ) ) ){
				return;
			}
		}
	}
	pbuf = (char*)GDT_POINTER( _ppool, datamunit );
	memcpy( pbuf, value, gdt_usize( _ppool, datamunit ) );
	gdt_add_hash( _ppool, h_munit, namemunit, datamunit, ELEMENT_LITERAL_STR );
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
		printf("memory error\n");
		return;
	}
	pbuf = (char*)GDT_POINTER( _ppool, datamunit );
	//memset( pbuf, 0, gdt_usize( _ppool, datamunit ) );
	pbuf[0] = '\0';
	gdt_add_hash( _ppool, h_munit, namemunit, datamunit, ELEMENT_LITERAL_STR );
}

void gdt_add_hash_value_kstring( GDT_MEMORY_POOL* _ppool, int32_t h_munit, const char* name, int32_t data_munit, int32_t id )
{
	int32_t namemunit = gdt_make_hash_name( _ppool, h_munit, name );
	if( namemunit == -1 ){
		return;
	}
	gdt_add_hash( _ppool, h_munit, namemunit, data_munit, id );
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
	if( hashchild[hashkey].hash_munit <= 0 ){
		return retmunit;
	}
	if( gdt_strlen(hashname_from) != gdt_strlen(hashname_to) ){
		return retmunit;
	}
	GDT_HASH_ELEMENT *hashelement = (struct GDT_HASH_ELEMENT*)GDT_POINTER( _ppool, hashchild[hashkey].hash_munit );
	uint32_t i;
	for( i = 0; i < hashchild[hashkey].hash_size; i++ )
	{
		if( hashelement[i].hashname_munit > 0 )
		{
			char* tmpname = (char*)GDT_POINTER( _ppool, hashelement[i].hashname_munit );
			if( !strcmp( tmpname, hashname_from ) )
			{
				memcpy(tmpname,hashname_to,gdt_strlen(hashname_to));
				retmunit = gdt_add_hash( _ppool, h_munit, hashelement[i].hashname_munit, hashelement[i].elm_munit, hashelement[i].id );
				hashelement[i].hashname_munit = -1;
				hashelement[i].elm_munit = -1;
				hashelement[i].id = -1;
				break;
			}
		}
	}
	return retmunit;
}

int32_t gdt_get_hash( GDT_MEMORY_POOL* _ppool, int32_t h_munit, const char* hashname )
{
	if( h_munit == -1 ){
		return -1;
	}
	struct GDT_HASH *hash = (struct GDT_HASH *)GDT_POINTER( _ppool, h_munit );
	struct GDT_HASH *hashchild = (struct GDT_HASH *)GDT_POINTER( _ppool, hash->hash_munit );
	uint32_t hashkey = gdt_ihash( hashname, hash->hash_size );
	int32_t retmunit = gdt_get_hash_core( _ppool, hash, hashchild, hashname, hashkey );
	return retmunit;
}

int32_t gdt_get_hash_fix_ihash( GDT_MEMORY_POOL* _ppool, int32_t h_munit, const char* hashname, uint32_t hashkey )
{
	if( h_munit == -1 ){
		return -1;
	}
	struct GDT_HASH *hash = (struct GDT_HASH *)GDT_POINTER( _ppool, h_munit );
	struct GDT_HASH *hashchild = (struct GDT_HASH *)GDT_POINTER( _ppool, hash->hash_munit );
	//int32_t retmunit = gdt_get_hash_core( _ppool, hash, hashchild, hashname, hashkey );
	int32_t retmunit = -1;
	if( hashchild[hashkey].hash_munit <= 0 )
	{
		//printf( "hash element is not found : %s\n", hashname );
		return retmunit;
	}
	GDT_HASH_ELEMENT *hashelement = (struct GDT_HASH_ELEMENT*)GDT_POINTER( _ppool, hashchild[hashkey].hash_munit );
	uint32_t i;
	for( i = 0; i < hashchild[hashkey].hash_size; i++ )
	{
		if( hashelement[i].hashname_munit > 0 )
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

int32_t gdt_get_hash_core( GDT_MEMORY_POOL* _ppool, struct GDT_HASH *hash, GDT_HASH *hashchild, const char* hashname, uint32_t hashkey )
{
	int32_t retmunit = -1;
	if( hashchild[hashkey].hash_munit <= 0 )
	{
		//printf( "hash element is not found : %s\n", hashname );
		return retmunit;
	}
	GDT_HASH_ELEMENT *hashelement = (struct GDT_HASH_ELEMENT*)GDT_POINTER( _ppool, hashchild[hashkey].hash_munit );
	uint32_t i;
	for( i = 0; i < hashchild[hashkey].hash_size; i++ )
	{
		if( hashelement[i].hashname_munit > 0 )
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

void gdt_clear_hash_string( GDT_MEMORY_POOL* _ppool, int32_t h_munit, const char* name )
{
	int32_t data_munit = gdt_get_hash( _ppool, h_munit, name );
	if( data_munit > 0 ){
		//memset( (char*)GDT_POINTER(_ppool,data_munit), 0, GDT_PUNIT_USIZE(_ppool,data_munit) );
		*((char*)GDT_POINTER(_ppool,data_munit)) = '\0';
	}
}

int32_t gdt_replace_hash_string( GDT_MEMORY_POOL* _ppool, int32_t h_munit, const char* name, const char* value )
{
	int32_t data_munit = gdt_get_hash( _ppool, h_munit, name );
	if( data_munit > 0 ){
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
	if( hashchild[hashkey].hash_munit < 0 )
	{
		return retmunit;
	}
	GDT_HASH_ELEMENT *hashelement = (struct GDT_HASH_ELEMENT*)GDT_POINTER( _ppool, hashchild[hashkey].hash_munit );
	uint32_t i;
	for( i = 0; i < hashchild[hashkey].hash_size; i++ )
	{
		if( hashelement[i].hashname_munit > 0 )
		{
			if( !strcmp( (char*)GDT_POINTER( _ppool, hashelement[i].hashname_munit ), hashname ) )
			{
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
	if( hashchild[hashkey].hash_munit < 0 )
	{
		return retid;
	}
	GDT_HASH_ELEMENT *hashelement = (GDT_HASH_ELEMENT*)GDT_POINTER( _ppool, hashchild[hashkey].hash_munit );
	uint32_t i;
	for( i = 0; i < hashchild[hashkey].hash_size; i++ )
	{
		if( hashelement[i].hashname_munit > 0 )
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

int32_t gdt_hash_length( GDT_MEMORY_POOL* _ppool, int32_t h_munit )
{
	int32_t length = 0;
	struct GDT_HASH *hash;
	struct GDT_HASH *hashchild;
	struct GDT_HASH_ELEMENT *hashelement;
	uint32_t i,j;
	do{
		hash = (struct GDT_HASH *)GDT_POINTER( _ppool, h_munit );
		hashchild = (struct GDT_HASH *)GDT_POINTER( _ppool, hash->hash_munit );
		for( j = 0; j < hash->hash_size; j++ )
		{
			if( hashchild[j].hash_munit > 0 )
			{
				hashelement = (struct GDT_HASH_ELEMENT*)GDT_POINTER( _ppool, hashchild[j].hash_munit );
				for( i = 0; i < hashchild[j].hash_size; i++ )
				{
					if( hashelement[i].hashname_munit > 0 )
					{
						++length;
					}
				}
			}
		}
	}while( false );
	return length;
}

void gdt_dump_hash( GDT_MEMORY_POOL* _ppool, int32_t h_munit, int index )
{
	struct GDT_HASH *hash;
	struct GDT_HASH *hashchild;
	struct GDT_HASH_ELEMENT *hashelement;
	uint32_t i,j,k;
	uint32_t cnt = 0;
	do{
		hash = (struct GDT_HASH *)GDT_POINTER( _ppool, h_munit );
		hashchild = (struct GDT_HASH *)GDT_POINTER( _ppool, hash->hash_munit );
		for( k=0;k<index;k++ ){ printf("  "); }
		printf("{\n");
		for( j = 0; j < hash->hash_size; j++ )
		{
			if( hashchild[j].hash_munit > 0 )
			{
				hashelement = (struct GDT_HASH_ELEMENT*)GDT_POINTER( _ppool, hashchild[j].hash_munit );
				for( i = 0; i < hashchild[j].hash_size; i++ )
				{
					if( hashelement[i].hashname_munit > 0 )
					{
						if( hashelement[i].id==ELEMENT_HASH){
							for( k=0;k<index+1;k++ ){ printf("  "); }
							if( cnt > 0 ){ printf(","); }
							printf( "\"%s\":\n"
								, (char*)GDT_POINTER( _ppool, hashelement[i].hashname_munit )
							);
							gdt_dump_hash( _ppool, hashelement[i].elm_munit, index+1 );
						}
						if( hashelement[i].id==ELEMENT_ARRAY){
							for( k=0;k<index+1;k++ ){ printf("  "); }
							if( cnt > 0 ){ printf(","); }
							printf( "\"%s\":\n"
								, (char*)GDT_POINTER( _ppool, hashelement[i].hashname_munit )
							);
							gdt_array_dump( _ppool, hashelement[i].elm_munit, index+1 );
						}
						else{
							for( k=0;k<index+1;k++ ){ printf("  "); }
							if( cnt > 0 ){ printf(","); }
							if( hashelement[i].id == ELEMENT_LITERAL_STR ){
								printf( "\"%s\":\"%s\"\n"
									, (char*)GDT_POINTER( _ppool, hashelement[i].hashname_munit )
									, (char*)GDT_POINTER( _ppool, hashelement[i].elm_munit )
								);
								cnt++;
							}
							else if( hashelement[i].id == ELEMENT_LITERAL_NUM ){
								printf( "\"%s\":%s\n"
									, (char*)GDT_POINTER( _ppool, hashelement[i].hashname_munit )
									, (char*)GDT_POINTER( _ppool, hashelement[i].elm_munit )
								);
								cnt++;
							}
						}
					}
				}
			}
		}
		for( k=0;k<index;k++ ){ printf("  "); }
		printf("}\n");
	}while( false );
}
