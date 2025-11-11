/*
 * Copyright (c) Katsuya Owari
 */

#include "qs_array.h"

int32_t qs_create_array( QS_MEMORY_POOL* _ppool, size_t allocsize, size_t buffer_size )
{
	QS_ARRAY* parray;
	QS_ARRAY_ELEMENT* elm;
	int32_t tmpmunit;
	int i;
	if( -1 == ( tmpmunit = qs_create_memory_block( _ppool, sizeof( QS_ARRAY ) ) ) ){
		return -1;
	}
	parray = (QS_ARRAY*)QS_GET_POINTER( _ppool, tmpmunit );
	parray->max_size = allocsize;
	if( buffer_size != 0 && buffer_size < NUMERIC_BUFFER_SIZE ){
		buffer_size = NUMERIC_BUFFER_SIZE;
	}
	parray->buffer_size = buffer_size;
	parray->len = 0;
	if( -1 == ( parray->munit = qs_create_memory_block( _ppool, sizeof( QS_ARRAY_ELEMENT ) * allocsize ) ) ){
		qs_free_memory_unit( _ppool, &tmpmunit );
		return -1;
	}
	elm = (QS_ARRAY_ELEMENT*)QS_GET_POINTER( _ppool, parray->munit );
	for( i = 0; i < allocsize; i++ ){
		(elm+i)->id = 0;
		(elm+i)->munit = -1;//qs_create_memory_block( _ppool, sizeof( char ) * parray->buffer_size );
		if( buffer_size > 0 ){
			if( -1 == ( (elm+i)->buf_munit = qs_create_memory_block( _ppool, sizeof( char ) * parray->buffer_size ) ) ){
				return -1;
			}
		}
	}
	return tmpmunit;
}

int32_t qs_create_array_buffer( QS_MEMORY_POOL* _ppool, size_t allocsize, size_t buffer_size )
{
	QS_ARRAY* parray;
	QS_ARRAY_ELEMENT* elm;
	int32_t tmpmunit;
	int i;
	if( -1 == ( tmpmunit = qs_create_memory_block( _ppool, sizeof( QS_ARRAY ) ) ) ){
		return -1;
	}
	parray = (QS_ARRAY*)QS_GET_POINTER( _ppool, tmpmunit );
	parray->max_size = allocsize;
	if( buffer_size != 0 && buffer_size < NUMERIC_BUFFER_SIZE ){
		buffer_size = NUMERIC_BUFFER_SIZE;
	}
	parray->buffer_size = buffer_size;
	parray->len = 0;
	if( -1 == ( parray->munit = qs_create_memory_block( _ppool, sizeof( QS_ARRAY_ELEMENT ) * allocsize ) ) ){
		qs_free_memory_unit( _ppool, &tmpmunit );
		return -1;
	}
	elm = (QS_ARRAY_ELEMENT*)QS_GET_POINTER( _ppool, parray->munit );
	for( i = 0; i < allocsize; i++ ){
		(elm+i)->id = ELEMENT_LITERAL_STR;
		if( -1 == ( (elm+i)->munit = qs_create_memory_block( _ppool, sizeof( char ) * parray->buffer_size ) ) ){
			return -1;
		}
		parray->len++;
		char* pbuf = (char*)QS_GET_POINTER(_ppool,(elm+i)->munit);
		*pbuf = '\0';
	}
	return tmpmunit;
}

int32_t qs_reset_array( QS_MEMORY_POOL* _ppool, int32_t munit )
{
	QS_ARRAY* parray;
	QS_ARRAY_ELEMENT* elm;
	int i;
	if( -1 == ( munit = qs_resize_array( _ppool, munit ) ) ){
		return -1;
	}
	parray = (QS_ARRAY*)QS_GET_POINTER( _ppool, munit );
	parray->max_size = parray->max_size;
	parray->len = 0;
	elm = (QS_ARRAY_ELEMENT*)QS_GET_POINTER( _ppool, parray->munit );
	for( i = 0; i < parray->len; i++ ){
		(elm+i)->id = 0;
		//(*(char*)QS_GET_POINTER(_ppool,(elm+i)->munit)) = '\0';
	}
	return 0;
}

int32_t qs_resize_array( QS_MEMORY_POOL* _ppool, int32_t munit )
{
	if( -1 == munit ){
		if( -1 == ( munit = qs_create_array( _ppool, QS_ARRAY_SIZE_DEFAULT, NUMERIC_BUFFER_SIZE ) ) ){
			printf("qs_resize_array parray->len >= parray->max_size\n");
			return munit;
		}
	}
	else{
		QS_ARRAY* parray = (QS_ARRAY*)QS_GET_POINTER( _ppool, munit );
		if( parray->len >= parray->max_size )
		{
			size_t i;
			size_t allocsize = parray->max_size;
			QS_ARRAY_ELEMENT* elm;
			int32_t tmpmunit = qs_create_memory_block( _ppool, sizeof( QS_ARRAY_ELEMENT ) * ( parray->max_size + allocsize ) );
			if( -1 == tmpmunit ){
				printf("qs_resize_array error. %d\n",(int)(parray->max_size + allocsize));
				return munit;
			}
			memcpy( 
				  ( QS_ARRAY_ELEMENT* )QS_GET_POINTER( _ppool, tmpmunit )
				, ( QS_ARRAY_ELEMENT* )QS_GET_POINTER( _ppool, parray->munit )
				, sizeof( QS_ARRAY_ELEMENT )*parray->max_size
			);
			qs_free_memory_unit( _ppool, &parray->munit );
			parray->munit = tmpmunit;
			elm = (QS_ARRAY_ELEMENT*)QS_GET_POINTER( _ppool, parray->munit );
			for( i = parray->max_size; i < parray->max_size + allocsize; i++ ){
				(elm+i)->id = 0;
				(elm+i)->munit = -1;
				if( parray->buffer_size > 0 ){
					if( -1 == ( (elm+i)->buf_munit = qs_create_memory_block( _ppool, sizeof( char ) * parray->buffer_size ) ) ){
						printf("qs_resize_array init error.\n");
						munit = -1;
						return munit;
					}
				}
				//(elm+1)->tmp_munit = -1;//qs_create_memory_block( _ppool, sizeof( char ) * parray->buffer_size );
			}
			parray->max_size += allocsize;
		}
	}
	return munit;
}

int32_t qs_next_push_munit( QS_MEMORY_POOL* _ppool, int32_t munit )
{
	QS_ARRAY* parray;
	QS_ARRAY_ELEMENT* elm;
	int32_t tmpmunit = -1;
	if( -1 == ( munit = qs_resize_array( _ppool, munit ) ) ){
		return tmpmunit;
	}
	parray = (QS_ARRAY*)QS_GET_POINTER( _ppool, munit );
	elm = (QS_ARRAY_ELEMENT*)QS_GET_POINTER( _ppool, parray->munit );
	if( parray->len >= parray->max_size ){
		printf("[debug] parray->len >= parray->max_size\n");
		return tmpmunit;
	}
	tmpmunit = elm[parray->len].munit;
	return tmpmunit;
}

int32_t qs_array_push( QS_MEMORY_POOL* _ppool, int32_t* pmunit, int id, int32_t munit )
{
	QS_ARRAY* parray;
	QS_ARRAY_ELEMENT* elm;
	int32_t is_push = -1;
	if( -1 == ( (*pmunit) = qs_resize_array( _ppool, (*pmunit) ) ) ){
		printf("qs_resize_array error \n");
		return is_push;
	}
	parray = (QS_ARRAY*)QS_GET_POINTER( _ppool, (*pmunit) );
	elm = (QS_ARRAY_ELEMENT*)QS_GET_POINTER( _ppool, parray->munit );
	if( parray->len >= parray->max_size ){
		printf("[debug] parray->len >= parray->max_size\n");
		return is_push;
	}
	elm[parray->len].id = id;
	elm[parray->len].munit = munit;
	parray->len++;
	is_push = 1;
	return is_push;
}

int32_t qs_array_push_integer( QS_MEMORY_POOL* _ppool, int32_t* pmunit, int32_t value )
{
	QS_ARRAY* parray;
	QS_ARRAY_ELEMENT* elm;
	(*pmunit) = qs_resize_array( _ppool, (*pmunit) );
	parray = (QS_ARRAY*)QS_GET_POINTER( _ppool, (*pmunit) );
	elm = (QS_ARRAY_ELEMENT*)QS_GET_POINTER( _ppool, parray->munit );
	if( parray->len >= parray->max_size ){
		return QS_SYSTEM_ERROR;
	}
	elm[parray->len].id = ELEMENT_LITERAL_NUM;
	elm[parray->len].munit = elm[parray->len].buf_munit;
	qs_itoa( value, (char*)QS_GET_POINTER(_ppool,elm[parray->len].munit), qs_usize(_ppool,elm[parray->len].munit) );
	(*(int32_t*)(QS_GET_POINTER(_ppool,elm[parray->len].munit)+qs_usize(_ppool,elm[parray->len].munit)-sizeof(int32_t))) = value;
	parray->len++;
	return QS_SYSTEM_OK;
}

int32_t qs_array_push_string( QS_MEMORY_POOL* _ppool, int32_t* pmunit, const char* value )
{
	QS_ARRAY* parray;
	QS_ARRAY_ELEMENT* elm;
	(*pmunit) = qs_resize_array( _ppool, (*pmunit) );
	parray = (QS_ARRAY*)QS_GET_POINTER( _ppool, (*pmunit) );
	elm = (QS_ARRAY_ELEMENT*)QS_GET_POINTER( _ppool, parray->munit );
	if( parray->len >= parray->max_size ){
		return QS_SYSTEM_ERROR;
	}
	elm[parray->len].id = ELEMENT_LITERAL_STR;
	if( elm[parray->len].munit == -1 ){
		if( -1 == ( elm[parray->len].munit = qs_create_memory_block( _ppool, strlen(value)+1 ) ) ){
			return QS_SYSTEM_ERROR;
		}
	}
	else{
		if( qs_usize(_ppool,elm[parray->len].munit) <= strlen(value)+1 ){
			if( -1 == ( elm[parray->len].munit = qs_create_memory_block( _ppool, strlen(value)+1 ) ) ){
				return QS_SYSTEM_ERROR;
			}
		}
	}
	memcpy( (char*)QS_GET_POINTER(_ppool,elm[parray->len].munit),value,strlen(value));
	*((char*)QS_GET_POINTER(_ppool,elm[parray->len].munit)+(strlen(value))) = '\0';
	parray->len++;
	return QS_SYSTEM_OK;
}

int32_t qs_array_push_empty_string( QS_MEMORY_POOL* _ppool, int32_t* pmunit, size_t size )
{
	QS_ARRAY* parray;
	QS_ARRAY_ELEMENT* elm;
	(*pmunit) = qs_resize_array( _ppool, (*pmunit) );
	parray = (QS_ARRAY*)QS_GET_POINTER( _ppool, (*pmunit) );
	elm = (QS_ARRAY_ELEMENT*)QS_GET_POINTER( _ppool, parray->munit );
	if( parray->len >= parray->max_size ){
		return QS_SYSTEM_ERROR;
	}
	elm[parray->len].id = ELEMENT_LITERAL_STR;
	if( elm[parray->len].munit == -1 )
	{
		if( -1 == ( elm[parray->len].munit = qs_create_memory_block( _ppool, size ) ) ){
			return QS_SYSTEM_ERROR;
		}
	}
	else{
		if( qs_usize(_ppool,elm[parray->len].munit) <= size ){
			if( -1 == ( elm[parray->len].munit = qs_create_memory_block( _ppool, size ) ) ){
				return QS_SYSTEM_ERROR;
			}
		}
	}
	*((char*)QS_GET_POINTER(_ppool,elm[parray->len].munit)) = '\0';
	parray->len++;
	return QS_SYSTEM_OK;
}

QS_ARRAY_ELEMENT* qs_array_pop( QS_MEMORY_POOL* _ppool, int32_t arraymunit )
{
	QS_ARRAY_ELEMENT* retelm = NULL;
	if( -1 == arraymunit ){
		return retelm;
	}
	QS_ARRAY* parray = (QS_ARRAY*)QS_GET_POINTER( _ppool, arraymunit );
	if( parray == NULL || parray->len <= 0 ){
		return retelm;
	}
	retelm = (QS_ARRAY_ELEMENT*)QS_GET_POINTER( _ppool, parray->munit )+( parray->len-1 );
	parray->len--;
	return retelm;
}

QS_ARRAY_ELEMENT* qs_array_get( QS_MEMORY_POOL* _ppool, int32_t arraymunit, int index )
{
	QS_ARRAY_ELEMENT* retelm = NULL;
	if( arraymunit == -1 ){
		return retelm;
	}
	QS_ARRAY* parray = (QS_ARRAY*)QS_GET_POINTER( _ppool, arraymunit );
	if( parray == NULL || parray->len <= index ){
		return retelm;
	}
	retelm = (QS_ARRAY_ELEMENT*)QS_GET_POINTER( _ppool, parray->munit )+( index );
	return retelm;
}

QS_ARRAY_ELEMENT* qs_array_foreach( QS_MEMORY_POOL* _ppool, int32_t array_munit, size_t* psize )
{
	QS_ARRAY_ELEMENT* retelm = NULL;
	if( array_munit == -1 ){
		return retelm;
	}
	QS_ARRAY* parray = (QS_ARRAY*)QS_GET_POINTER( _ppool, array_munit );
	if( parray == NULL || parray->len <= (*psize) ){
		return retelm;
	}
	retelm = (QS_ARRAY_ELEMENT*)QS_GET_POINTER( _ppool, parray->munit )+( *psize );
	*psize = *psize + 1;
	return retelm;
}

int32_t qs_array_length( QS_MEMORY_POOL* _ppool, int32_t arraymunit )
{
	if( arraymunit == -1 ){
		return -1;
	}
	QS_ARRAY* parray = (QS_ARRAY*)QS_GET_POINTER( _ppool, arraymunit );
	if( parray == NULL ){
		return -1;
	}
	return parray->len;
}
