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

#include "qs_ranking.h"


int32_t qs_create_ranking( QS_MEMORY_POOL* _ppool, size_t size, int32_t key_size, int32_t get_max, int32_t refresh_size )
{
	int32_t ranking_munit = qs_create_munit( _ppool, sizeof( QS_RANKING ), MEMORY_TYPE_DEFAULT );
	if( ranking_munit == -1 ){
		return -1;
	}
	QS_RANKING* ranking = (QS_RANKING*)QS_GET_POINTER(_ppool,ranking_munit);
	ranking->self_id = ranking_munit;
	ranking->array_size = size;
	ranking->key_size = key_size + 1;
	ranking->get_max = get_max;
	size_t index_memory_size = ( ( ranking->key_size + SIZE_BYTE * 256 ) * size );
	if( -1 == ( ranking->ranking_index_memory_id = qs_create_mini_memory(_ppool, index_memory_size ) ) ){
		return -1;
	}
	QS_MEMORY_POOL* index_memory = (QS_MEMORY_POOL*)QS_GET_POINTER(_ppool,ranking->ranking_index_memory_id);
	ranking->ranking_index_munit = qs_create_hash( index_memory, size );
	if( ranking->ranking_index_munit == -1 ){
		return -1;
	}
	//size_t temp_memory_size = ( ( ranking->key_size + SIZE_KBYTE ) * ranking->get_max );
	ranking->ranking_user_munit = qs_create_array( _ppool, size, 0 );
	if( ranking->ranking_user_munit == -1 ){
		return -1;
	}
	ranking->tail_ranking = 0;
	QS_ARRAY* parray = (QS_ARRAY*)QS_GET_POINTER(_ppool,ranking->ranking_user_munit );
	QS_ARRAY_ELEMENT* elm = (QS_ARRAY_ELEMENT*)QS_GET_POINTER( _ppool, parray->munit );
	int i;
	int32_t user_munit = -1;
	for( i = 0; i < parray->max_size; i++ )
	{
		if( -1 == ( user_munit = qs_create_hash( _ppool, RANKING_USER_HASH_SIZE ) ) ){
			return -1;
		}
		qs_add_hash_emptystring( _ppool, user_munit, "id", ranking->key_size );
		qs_add_hash_integer( _ppool, user_munit, "value", 0 );
		qs_add_hash_integer( _ppool, user_munit, "ranking", ranking->tail_ranking );
		(elm+i)->id = ELEMENT_HASH;
		(elm+i)->munit = user_munit;
		parray->len++;
	}
	if (-1 == (ranking->sort_buffer_munit = qs_create_munit(_ppool, sizeof(QS_RANKING_SORT)*parray->max_size, MEMORY_TYPE_DEFAULT))) {
		return -1;
	}
	ranking->low_value = 0;
	ranking->refresh_size = refresh_size;
	ranking->need_entry = 1;
	return ranking_munit;
}

void qs_push_integer( QS_MEMORY_POOL* _ppool, int32_t munit,int32_t value )
{
	qs_itoa( value, (char*)QS_GET_POINTER(_ppool,munit), qs_usize(_ppool,munit) );
	int32_t* pv = QS_PINT32(_ppool,munit);
	*pv = value;
}

int32_t qs_entry_ranking( QS_MEMORY_POOL* _ppool, int32_t ranking_munit, char* id)
{
	QS_RANKING* ranking = (QS_RANKING*)QS_GET_POINTER(_ppool,ranking_munit);
	QS_MEMORY_POOL* index_memory = (QS_MEMORY_POOL*)QS_GET_POINTER(_ppool,ranking->ranking_index_memory_id);
	int32_t index_munit;
	int32_t index = -1;
	int i;
	QS_ARRAY* parray = (QS_ARRAY*)QS_GET_POINTER(_ppool,ranking->ranking_user_munit );
	QS_ARRAY_ELEMENT* elm = (QS_ARRAY_ELEMENT*)QS_GET_POINTER( _ppool, parray->munit );
	if( -1 == ( index_munit = qs_get_hash(index_memory,ranking->ranking_index_munit,id) ) ){
		char* pbuf;
		if(ranking->tail_ranking >= parray->max_size ){
			qs_ranking_sort_all( _ppool, ranking_munit );
			ranking->tail_ranking = ranking->tail_ranking - ranking->refresh_size;
			for( i = ranking->tail_ranking; i < parray->max_size; i++ ){
				pbuf = (char*)QS_GET_POINTER(_ppool,qs_get_hash(_ppool,(elm+i)->munit,"id"));
				qs_remove_hash(index_memory,ranking->ranking_index_munit,pbuf);
				memset( pbuf, 0, ranking->key_size );
				qs_push_integer( _ppool, qs_get_hash(_ppool,(elm+i)->munit,"value"),0 );
				qs_push_integer( _ppool, qs_get_hash(_ppool,(elm+i)->munit,"ranking"),ranking->tail_ranking );
			}
		}
		for( i = ranking->tail_ranking; i < parray->max_size; i++ )
		{
			pbuf = (char*)QS_GET_POINTER(_ppool,qs_get_hash(_ppool,(elm+i)->munit,"id"));
			if( !strcmp( "", pbuf ) ){
				ranking->tail_ranking++;
				memcpy( pbuf, id, ranking->key_size );
				pbuf[ranking->key_size-1] = '\0';
				qs_add_hash_integer( index_memory, ranking->ranking_index_munit, id, i );
				qs_push_integer( _ppool, qs_get_hash(_ppool,(elm+i)->munit,"value"),0 );
				qs_push_integer( _ppool, qs_get_hash(_ppool,(elm+i)->munit,"ranking"),ranking->tail_ranking );
				index = i;
				break;
			}
			if( i == parray->max_size-1 && index == -1 ){
				return -1;
			}
		}
	}
	return index;
}

int32_t qs_set_ranking_value( QS_MEMORY_POOL* _ppool, int32_t ranking_munit, char* id, uint32_t value )
{
	QS_RANKING* ranking = (QS_RANKING*)QS_GET_POINTER(_ppool,ranking_munit);
	QS_MEMORY_POOL* index_memory = (QS_MEMORY_POOL*)QS_GET_POINTER(_ppool,ranking->ranking_index_memory_id);
	int32_t index_munit;
	int32_t index = -1;
	int i;
	QS_ARRAY* parray = (QS_ARRAY*)QS_GET_POINTER(_ppool,ranking->ranking_user_munit );
	QS_ARRAY_ELEMENT* elm = (QS_ARRAY_ELEMENT*)QS_GET_POINTER( _ppool, parray->munit );
	if( -1 == ( index_munit = qs_get_hash(index_memory,ranking->ranking_index_munit,id) ) ){
		if(ranking->need_entry==1){
			//printf("not entry : %s\n",id);
			return -1;
		}
		char* pbuf;
		for( i = ranking->tail_ranking; i < parray->max_size; i++ )
		{
			pbuf = (char*)QS_GET_POINTER(_ppool,qs_get_hash(_ppool,(elm+i)->munit,"id"));
			if( !strcmp( "", pbuf ) ){
				ranking->tail_ranking++;
				memcpy( pbuf, id, ranking->key_size );
				pbuf[ranking->key_size-1] = '\0';
				qs_add_hash_integer( index_memory, ranking->ranking_index_munit, id, i );
				qs_push_integer( _ppool, qs_get_hash(_ppool,(elm+i)->munit,"value"),value );
				qs_push_integer( _ppool, qs_get_hash(_ppool,(elm+i)->munit,"ranking"),ranking->tail_ranking );
				if(ranking->low_value < value){
					ranking->low_value = value;
				}
				index = i;
				break;
			}
			if( i == parray->max_size-1 && index == -1 ){
				return QS_SYSTEM_ERROR;
			}
		}
	}
	else{
		index = QS_INT32(index_memory,index_munit);
		qs_push_integer( _ppool, qs_get_hash(_ppool,(elm+index)->munit,"value"),value );
		if(ranking->low_value < value){
			ranking->low_value = value;
		}
	}
	return QS_SYSTEM_OK;
}

int32_t qs_add_ranking_value( QS_MEMORY_POOL* _ppool, int32_t ranking_munit, char* id, uint32_t value )
{
	QS_RANKING* ranking = (QS_RANKING*)QS_GET_POINTER(_ppool,ranking_munit);
	QS_MEMORY_POOL* index_memory = (QS_MEMORY_POOL*)QS_GET_POINTER(_ppool,ranking->ranking_index_memory_id);
	int32_t index_munit;
	int32_t index = -1;
	int i;
	QS_ARRAY* parray = (QS_ARRAY*)QS_GET_POINTER(_ppool,ranking->ranking_user_munit );
	QS_ARRAY_ELEMENT* elm = (QS_ARRAY_ELEMENT*)QS_GET_POINTER( _ppool, parray->munit );
	if( -1 == ( index_munit = qs_get_hash(index_memory,ranking->ranking_index_munit,id) ) ){
		if(ranking->need_entry==1){
			return -1;
		}
		char* pbuf;
		for( i = ranking->tail_ranking; i < parray->max_size; i++ )
		{
			pbuf = (char*)QS_GET_POINTER(_ppool,qs_get_hash(_ppool,(elm+i)->munit,"id"));
			if( !strcmp( "", pbuf ) ){
				ranking->tail_ranking++;
				memcpy( pbuf, id, ranking->key_size );
				pbuf[ranking->key_size-1] = '\0';
				qs_add_hash_integer( index_memory, ranking->ranking_index_munit, id, i );
				qs_push_integer( _ppool, qs_get_hash(_ppool,(elm+i)->munit,"value"),value );
				qs_push_integer( _ppool, qs_get_hash(_ppool,(elm+i)->munit,"ranking"),ranking->tail_ranking );
				if(ranking->low_value < value){
					ranking->low_value = value;
				}
				index = i;
				break;
			}
			if( i == parray->max_size-1 && index == -1 ){
				return QS_SYSTEM_ERROR;
			}
		}
	}
	else{
		index = QS_INT32(index_memory,index_munit);
		int32_t h1 = qs_get_hash(_ppool,(elm+index)->munit,"value");
		int32_t v1 = QS_INT32(_ppool,h1);
		int32_t v = v1 + value;
		qs_push_integer( _ppool, qs_get_hash(_ppool,(elm+index)->munit,"value"), v );
		if(ranking->low_value < v){
			ranking->low_value = v;
		}
	}
	return QS_SYSTEM_OK;
}

int32_t qs_ranking_sort_all( QS_MEMORY_POOL* _ppool, int32_t ranking_munit )
{
	int i,j;
	QS_RANKING* ranking = (QS_RANKING*)QS_GET_POINTER(_ppool,ranking_munit);
	QS_MEMORY_POOL* index_memory = (QS_MEMORY_POOL*)QS_GET_POINTER(_ppool,ranking->ranking_index_memory_id);
	QS_ARRAY* parray = (QS_ARRAY*)QS_GET_POINTER(_ppool,ranking->ranking_user_munit );
	QS_ARRAY_ELEMENT* elm = (QS_ARRAY_ELEMENT*)QS_GET_POINTER( _ppool, parray->munit );
	int32_t tmph;
	size_t hash_size = RANKING_USER_HASH_SIZE;
	uint32_t hashkey_value = qs_ihash( "value", hash_size );
	uint32_t hashkey_ranking = qs_ihash( "ranking", hash_size );
	uint32_t hashkey_id = qs_ihash( "id", hash_size );
	QS_RANKING_SORT* sort_buffer = (QS_RANKING_SORT*)QS_GET_POINTER(_ppool,ranking->sort_buffer_munit);
	for( i = 0; i < parray->max_size; i++ )
	{
		int32_t target_hash_munit = qs_get_hash_fix_ihash(_ppool,(elm+i)->munit,"value",hashkey_value);
		int32_t target_rank_value = QS_INT32(_ppool,target_hash_munit);
		(sort_buffer+i)->munit = (elm+i)->munit;
		(sort_buffer+i)->value = target_rank_value;
		(sort_buffer+i)->ranking = 0;
	}
	int32_t tmp_munit;
	int32_t tmp_value;
	for( i = 0; i < parray->max_size; i++ )
	{
		for( j = parray->max_size-1; j > i; j-- ){
			if( (sort_buffer+(j-1))->value < (sort_buffer+j)->value ){
				tmp_munit = (sort_buffer+j)->munit;
				tmp_value = (sort_buffer+j)->value;
				(sort_buffer+j)->munit = (sort_buffer+(j-1))->munit;
				(sort_buffer+j)->value = (sort_buffer+(j-1))->value;
				(sort_buffer+(j-1))->munit = tmp_munit;
				(sort_buffer+(j-1))->value = tmp_value;
				tmph = (elm+j)->munit;
				(elm+j)->munit = (elm+(j-1))->munit;
				(elm+(j-1))->munit = tmph;
			}
		}
	}
	int32_t old_ranking_value = -1;
	int32_t ranking_offset = 0;
	qs_memory_clean(index_memory);
	ranking->ranking_index_munit = qs_create_hash( index_memory, parray->max_size );
	for( i = 0; i < parray->max_size; i++ )
	{
		if( old_ranking_value == -1 || (sort_buffer+i)->value < old_ranking_value ){
			ranking_offset=i+1;
		}
		old_ranking_value = (sort_buffer+i)->value;
		int32_t ranking_hash_munit = qs_get_hash_fix_ihash(_ppool,(elm+i)->munit,"ranking",hashkey_ranking);
		qs_push_integer( _ppool, ranking_hash_munit, ranking_offset );
		int32_t target_id_hash_munit = qs_get_hash_fix_ihash(_ppool,(elm+i)->munit,"id",hashkey_id);
		qs_add_hash_integer( index_memory, ranking->ranking_index_munit, (char*)QS_GET_POINTER(_ppool,target_id_hash_munit), i );
	}
	return QS_SYSTEM_OK;
}

int32_t qs_get_ranking( QS_MEMORY_POOL* _ppool, int32_t ranking_munit, int32_t offset, int32_t length, QS_MEMORY_POOL* dest_memory  )
{
	QS_RANKING* ranking = (QS_RANKING*)QS_GET_POINTER(_ppool,ranking_munit);
	if(length>ranking->get_max){
		length = ranking->get_max;
	}
	int32_t ranking_temp_munit = qs_create_array( dest_memory, length, 0 );
	QS_ARRAY* parray = (QS_ARRAY*)QS_GET_POINTER(_ppool,ranking->ranking_user_munit );
	QS_ARRAY_ELEMENT* elm = (QS_ARRAY_ELEMENT*)QS_GET_POINTER( _ppool, parray->munit );
	QS_ARRAY* temp_parray = (QS_ARRAY*)QS_GET_POINTER(dest_memory,ranking_temp_munit );
	QS_ARRAY_ELEMENT* temp_elm = (QS_ARRAY_ELEMENT*)QS_GET_POINTER( dest_memory, temp_parray->munit );
	int i;
	int temp_offset = 0;
	int32_t user_munit = -1;
	for( i = offset; i < parray->max_size; i++ )
	{
		char* pbuf = (char*)QS_GET_POINTER(_ppool,qs_get_hash(_ppool,(elm+i)->munit,"id"));
		if( strcmp( "", pbuf ) ){
			int32_t value = QS_INT32(_ppool,qs_get_hash(_ppool,(elm+i)->munit,"value"));
			int32_t rank = QS_INT32(_ppool,qs_get_hash(_ppool,(elm+i)->munit,"ranking"));
			if( -1 == ( user_munit = qs_create_hash( dest_memory, RANKING_USER_HASH_SIZE ) ) ){
				return -1;
			}
			qs_add_hash_string( dest_memory, user_munit, "id", pbuf );
			qs_add_hash_integer( dest_memory, user_munit, "value", value );
			qs_add_hash_integer( dest_memory, user_munit, "ranking", rank );
			(temp_elm+temp_offset)->id = ELEMENT_HASH;
			(temp_elm+temp_offset)->munit = user_munit;
			temp_parray->len++;
			temp_offset++;
			if(temp_offset>=length){
				break;
			}
		}
	}
	size_t temp_array_size = ( ( ranking->key_size + SIZE_BYTE * 64 ) * ranking->get_max );
	int32_t json_buffer_id = qs_create_munit(dest_memory,sizeof(uint8_t)*temp_array_size,MEMORY_TYPE_DEFAULT);
	if(-1==json_buffer_id){
		return -1;
	}
	int32_t root_munit = qs_make_json_root( dest_memory, ranking_temp_munit, ELEMENT_ARRAY );
	if(-1==root_munit){
		return -1;
	}
	int32_t json_munit = qs_json_encode_b( dest_memory, (QS_NODE*)QS_GET_POINTER( dest_memory, root_munit ), json_buffer_id);
	return json_munit;
}
