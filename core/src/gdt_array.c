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

#include "gdt_array.h"

int32_t gdt_create_array( GDT_MEMORY_POOL* _ppool, size_t allocsize, size_t buffer_size )
{
	GDT_ARRAY* parray;
	GDT_ARRAY_ELEMENT* elm;
	int32_t tmpmunit;
	int i;
	do{
		if( 0 >= ( tmpmunit = gdt_create_munit( _ppool, sizeof( GDT_ARRAY ), MEMORY_TYPE_DEFAULT ) ) ){
			break;
		}
		parray = (GDT_ARRAY*)GDT_POINTER( _ppool, tmpmunit );
		parray->max_size = allocsize;
		if( buffer_size != 0 && buffer_size < NUMERIC_BUFFER_SIZE ){
			buffer_size = NUMERIC_BUFFER_SIZE;
		}
		parray->buffer_size = buffer_size;
		parray->len = 0;
		if( 0 >= ( parray->munit = gdt_create_munit( _ppool, sizeof( GDT_ARRAY_ELEMENT ) * allocsize, MEMORY_TYPE_DEFAULT ) ) ){
			gdt_free_memory_unit( _ppool, &tmpmunit );
			tmpmunit = -1;
			break;
		}
		elm = (GDT_ARRAY_ELEMENT*)GDT_POINTER( _ppool, parray->munit );
		for( i = 0; i < allocsize; i++ )
		{
			(elm+i)->id = 0;
			(elm+i)->munit = -1;//gdt_create_munit( _ppool, sizeof( char ) * parray->buffer_size, MEMORY_TYPE_DEFAULT );
			if( buffer_size > 0 ){
				if( 0>= ( (elm+i)->buf_munit = gdt_create_munit( _ppool, sizeof( char ) * parray->buffer_size, MEMORY_TYPE_DEFAULT ) ) ){
					break;
				}
			}
		}
	}while( false );
	return tmpmunit;
}

void gdt_reset_array( GDT_MEMORY_POOL* _ppool, int32_t munit )
{
	GDT_ARRAY* parray;
	GDT_ARRAY_ELEMENT* elm;
	int i;
	do{
		if( 0 >= ( munit = gdt_resize_array( _ppool, munit ) ) ){
			break;
		}
		parray = (GDT_ARRAY*)GDT_POINTER( _ppool, munit );
		parray->max_size = parray->max_size;
		parray->len = 0;
		elm = (GDT_ARRAY_ELEMENT*)GDT_POINTER( _ppool, parray->munit );
		for( i = 0; i < parray->len; i++ )
		{
			(elm+i)->id = 0;
			//(*(char*)GDT_POINTER(_ppool,(elm+i)->munit)) = '\0';
		}
	}while( false );
}

int32_t gdt_resize_array( GDT_MEMORY_POOL* _ppool, int32_t munit )
{
	if( munit <= 0 )
	{
		if( 0 >= ( munit = gdt_create_array( _ppool, 8, NUMERIC_BUFFER_SIZE ) ) ){
			printf("gdt_resize_array parray->len >= parray->max_size\n");
			return munit;
		}
	}
	else{
		GDT_ARRAY* parray = (GDT_ARRAY*)GDT_POINTER( _ppool, munit );
		if( parray->len >= parray->max_size )
		{
			int i;
			size_t allocsize = 8;
			GDT_ARRAY_ELEMENT* elm;
			int32_t tmpmunit = gdt_create_munit( _ppool, sizeof( GDT_ARRAY_ELEMENT ) * ( parray->max_size + allocsize ), MEMORY_TYPE_DEFAULT );
			if( tmpmunit <= 0 ){
				printf("gdt_resize_array error.\n");
				return munit;
			}
			memcpy( 
					( GDT_ARRAY_ELEMENT* )GDT_POINTER( _ppool, tmpmunit )
				, ( GDT_ARRAY_ELEMENT* )GDT_POINTER( _ppool, parray->munit )
				, sizeof( GDT_ARRAY_ELEMENT )*parray->max_size
			);
			gdt_free_memory_unit( _ppool, &parray->munit );
			parray->munit = tmpmunit;
			elm = (GDT_ARRAY_ELEMENT*)GDT_POINTER( _ppool, parray->munit );
			for( i = parray->max_size; i < parray->max_size + allocsize; i++ ){
				(elm+i)->id = 0;
				(elm+i)->munit = -1;
				if( 0 >= ( (elm+i)->buf_munit = gdt_create_munit( _ppool, sizeof( char ) * parray->buffer_size, MEMORY_TYPE_DEFAULT ) ) ){
					printf("gdt_resize_array init error.\n");
					munit = -1;
					return munit;
				}
				//(elm+1)->tmp_munit = -1;//gdt_create_munit( _ppool, sizeof( char ) * parray->buffer_size, MEMORY_TYPE_DEFAULT );
			}
			parray->max_size += allocsize;
		}
	}
	return munit;
}

int32_t gdt_next_push_munit( GDT_MEMORY_POOL* _ppool, int32_t munit )
{
	GDT_ARRAY* parray;
	GDT_ARRAY_ELEMENT* elm;
	int32_t tmpmunit = -1;
	if( -1 == ( munit = gdt_resize_array( _ppool, munit ) ) ){
		return tmpmunit;
	}
	parray = (GDT_ARRAY*)GDT_POINTER( _ppool, munit );
	elm = (GDT_ARRAY_ELEMENT*)GDT_POINTER( _ppool, parray->munit );
	if( parray->len >= parray->max_size ){
		printf("[debug] parray->len >= parray->max_size\n");
		return tmpmunit;
	}
	tmpmunit = elm[parray->len].munit;
	return tmpmunit;
}

int32_t gdt_array_push( GDT_MEMORY_POOL* _ppool, int32_t* pmunit, int id, int32_t munit )
{
	GDT_ARRAY* parray;
	GDT_ARRAY_ELEMENT* elm;
	int32_t freemunit = -1;
	if( -1 == ( (*pmunit) = gdt_resize_array( _ppool, (*pmunit) ) ) ){
		printf("gdt_resize_array error \n");
		return freemunit;
	}
	parray = (GDT_ARRAY*)GDT_POINTER( _ppool, (*pmunit) );
	elm = (GDT_ARRAY_ELEMENT*)GDT_POINTER( _ppool, parray->munit );
	if( parray->len >= parray->max_size ){
		printf("[debug] parray->len >= parray->max_size\n");
		return freemunit;
	}
	elm[parray->len].id = id;
	if( munit != elm[parray->len].munit && elm[parray->len].munit > 0 )
	{
		freemunit = elm[parray->len].munit;
	}
	elm[parray->len].munit= munit;
	parray->len++;
	return freemunit;
}

int32_t gdt_array_push_integer( GDT_MEMORY_POOL* _ppool, int32_t* pmunit, int32_t value )
{
	GDT_ARRAY* parray;
	GDT_ARRAY_ELEMENT* elm;
	(*pmunit) = gdt_resize_array( _ppool, (*pmunit) );
	parray = (GDT_ARRAY*)GDT_POINTER( _ppool, (*pmunit) );
	elm = (GDT_ARRAY_ELEMENT*)GDT_POINTER( _ppool, parray->munit );
	if( parray->len >= parray->max_size ){
		return GDT_SYSTEM_ERROR;
	}
	elm[parray->len].id = ELEMENT_LITERAL_NUM;
	elm[parray->len].munit = elm[parray->len].buf_munit;
	gdt_itoa( value, (char*)GDT_POINTER(_ppool,elm[parray->len].munit), gdt_usize(_ppool,elm[parray->len].munit) );
	(*(int32_t*)(GDT_POINTER(_ppool,elm[parray->len].munit)+gdt_usize(_ppool,elm[parray->len].munit)-sizeof(int32_t))) = value;
	parray->len++;
	return GDT_SYSTEM_OK;
}

GDT_ARRAY_ELEMENT* gdt_array_pop( GDT_MEMORY_POOL* _ppool, int32_t arraymunit )
{
	GDT_ARRAY_ELEMENT* retelm = NULL;
	if( arraymunit <= 0 ){
		return retelm;
	}
	GDT_ARRAY* parray = (GDT_ARRAY*)GDT_POINTER( _ppool, arraymunit );
	if( parray == NULL || parray->len <= 0 ){
		return retelm;
	}
	retelm = (GDT_ARRAY_ELEMENT*)GDT_POINTER( _ppool, parray->munit )+( parray->len-1 );
	parray->len--;
	return retelm;
}

int32_t gdt_array_length( GDT_MEMORY_POOL* _ppool, int32_t arraymunit )
{
	if( arraymunit <= 0 ){
		return -1;
	}
	GDT_ARRAY* parray = (GDT_ARRAY*)GDT_POINTER( _ppool, arraymunit );
	if( parray == NULL ){
		return -1;
	}
	return parray->len;
}

void gdt_array_dump( GDT_MEMORY_POOL* _ppool, int32_t munit, int index )
{
	GDT_ARRAY* parray;
	GDT_ARRAY_ELEMENT* elm;
	int i,k;
	do{
		for( k=0;k<index;k++ ){ printf("  "); }
		printf("[\n");
		if( munit > 0 ){
			parray = (GDT_ARRAY*)GDT_POINTER( _ppool, munit );
			elm = (GDT_ARRAY_ELEMENT*)GDT_POINTER( _ppool, parray->munit );
			for( i = 0; i < parray->len; i++ )
			{
				if( (elm+i)->id == ELEMENT_LITERAL_NUM ){
					for( k=0;k<index+1;k++ ){ printf("  "); }
					if( i > 0 ){ printf(","); }
					printf("%s\n",(char*)GDT_POINTER(_ppool,(elm+i)->munit));
				}
				if( (elm+i)->id == ELEMENT_LITERAL_STR ){
					for( k=0;k<index+1;k++ ){ printf("  "); }
					if( i > 0 ){ printf(","); }
					printf("\"%s\"\n",(char*)GDT_POINTER(_ppool,(elm+i)->munit));
				}
				else if( (elm+i)->id == ELEMENT_ARRAY ){
					if( i > 0 ){ printf(","); }
					gdt_array_dump( _ppool, (elm+i)->munit, index+1 );
				}
				else if( (elm+i)->id==ELEMENT_HASH){
					if( i > 0 ){ printf(","); }
					gdt_dump_hash( _ppool, (elm+i)->munit, index+1 );
				}
			}
		}
		for( k=0;k<index;k++ ){ printf("  "); }
		printf("]\n");
	}while( false );
}
