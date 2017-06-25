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

#include "gdt_memory_allocator.h"

/*
 *  新規メモリ確保
 * @param _ppool メモリプールのポインタ
 * @param  allocate_size(size_t) 確保するメモリのサイズ
 * @param  alignment_size(size_t) アラインメントサイズ
 * @param  fix_memory_unit(size_t) デフォルトのメモリユニット数
 * @param  free_memory_unit(size_t) デフォルトのメモリユニット数
 * @param  min_realloc(size_t) メモリ拡張最少バイト数
 * @return 確保したメモリのサイズ( 0なら失敗 )
 */
size_t gdt_initialize_memory( GDT_MEMORY_POOL** _ppool, size_t allocate_size, size_t max_allocate_size, size_t alignment_size, size_t fix_memory_unit, size_t free_memory_unit, size_t min_realloc )
{
	size_t memory_size		= 0;
	size_t byte_size		= 0;
	size_t max_byte_size	= 0;
	size_t memory_unit_one_size = 0;
	size_t memoryUnitSize = 0;
	GDT_MEMORY_UNIT* unit;
	int i;
	do{
		if( fix_memory_unit < FIX_MUNIT_SIZE ){
			fix_memory_unit = FIX_MUNIT_SIZE;
		}
		allocate_size = GDT_ALIGNUP( allocate_size, alignment_size );
		if( ( (*_ppool) = ( GDT_MEMORY_POOL * )malloc( sizeof( GDT_MEMORY_POOL ) ) ) == NULL ){
			printf( "_ppool allocate error\n" );
			break;
		}
		memset( (*_ppool), 0, sizeof( GDT_MEMORY_POOL ) );
		byte_size = sizeof( uint8_t ) * allocate_size;
		max_byte_size = sizeof( uint8_t ) * max_allocate_size;
		if( max_byte_size < byte_size ){
			max_byte_size = byte_size;
		}
		if( ( (*_ppool)->memory = ( void* )malloc( byte_size ) ) == NULL ){
			printf( "_ppool memory allocate error" );
			break;
		}
		(*_ppool)->top				= 0;
		(*_ppool)->end				= byte_size;
		(*_ppool)->bottom			= byte_size;
		(*_ppool)->size				= byte_size;
		(*_ppool)->max_size			= max_byte_size;
		(*_ppool)->alignment		= alignment_size;
		(*_ppool)->min_realloc		= GDT_ALIGNUP( min_realloc, alignment_size );
		(*_ppool)->fix_unit_size	= fix_memory_unit;
		(*_ppool)->unit_size		= ( free_memory_unit + fix_memory_unit );
		(*_ppool)->tail_munit		= -1;
		(*_ppool)->lock_munit		= -1;
		(*_ppool)->memory_buf_munit	= -1;
		(*_ppool)->alloc_type			= MEMORY_ALLOCATE_TYPE_MALLOC;
		(*_ppool)->endian			= gdt_endian();
		memory_unit_one_size = GDT_ALIGNUP( sizeof( GDT_MEMORY_UNIT ), alignment_size );
		memoryUnitSize = memory_unit_one_size * ( fix_memory_unit + free_memory_unit );
		(*_ppool)->bottom = (*_ppool)->end - memoryUnitSize;
		(*_ppool)->memory_unit_size_one = memory_unit_one_size;
		for( i = ((*_ppool)->unit_size-1); i >= 0; i-- )
		{
			unit = (GDT_MEMORY_UNIT*)( ( (uint8_t*)(*_ppool)->memory + (*_ppool)->end ) - ( memory_unit_one_size * ( (*_ppool)->unit_size - i ) ) );
			gdt_initialize_memory_unit( unit );
			unit->id = ( ((*_ppool)->unit_size-1) - i );
		}
		memory_size = (*_ppool)->size;
	}while( FALSE );
	return memory_size;
}

/*
 *  mmapシステム使用領域の確保
 * @param _ppool メモリプールのポインタ
 * @param  allocate_size(size_t) 確保するメモリのサイズ
 * @param  alignment_size(size_t) アラインメントサイズ
 * @param  fix_memory_unit(size_t) デフォルトのメモリユニット数
 * @param  free_memory_unit(size_t) デフォルトのメモリユニット数
 * @param  min_realloc(size_t) メモリ拡張最少バイト数
 * @return 確保したメモリのサイズ( 0なら失敗 )
 */
size_t gdt_initialize_mmapmemory( GDT_MEMORY_POOL** _ppool, size_t allocate_size, size_t max_allocate_size, size_t alignment_size, size_t fix_memory_unit, size_t free_memory_unit, size_t min_realloc )
{
	size_t memory_size			= 0;
#if defined(__LINUX__) || defined(__BSD_UNIX__)
	size_t byte_size			= 0;
	size_t max_byte_size		= 0;
	size_t memory_unit_one_size	= 0;
	size_t memoryUnitSize		= 0;
	GDT_MEMORY_UNIT* unit		= NULL;
	int i;
	do{
		if( fix_memory_unit < FIX_MUNIT_SIZE ){
			fix_memory_unit = FIX_MUNIT_SIZE;
		}
		allocate_size = GDT_ALIGNUP( allocate_size, alignment_size );
		if( ( (*_ppool) = ( GDT_MEMORY_POOL * )mmap( 0, sizeof( GDT_MEMORY_POOL ), PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0 ) ) == NULL ){
			printf( "_ppool allocate error\n" );
			break;
		}
		memset( (*_ppool), 0, sizeof( GDT_MEMORY_POOL ) );
		byte_size = sizeof( uint8_t ) * allocate_size;
		max_byte_size = sizeof( uint8_t ) * max_allocate_size;
		if( max_byte_size < byte_size ){
			max_byte_size = byte_size;
		}
		if( ( (*_ppool)->memory = ( void* )mmap( 0, byte_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0 ) ) == NULL ){
			printf( "_ppool memory allocate error" );
			break;
		}
		memset( (*_ppool)->memory, 0, byte_size );
		(*_ppool)->top				= 0;
		(*_ppool)->end				= byte_size;
		(*_ppool)->bottom			= byte_size;
		(*_ppool)->size				= byte_size;
		(*_ppool)->max_size			= max_byte_size;
		(*_ppool)->alignment		= alignment_size;
		(*_ppool)->min_realloc		= GDT_ALIGNUP( min_realloc, alignment_size );
		(*_ppool)->fix_unit_size	= fix_memory_unit;
		(*_ppool)->unit_size		= ( free_memory_unit + fix_memory_unit );
		(*_ppool)->tail_munit		= -1;
		(*_ppool)->lock_munit		= -1;
		(*_ppool)->memory_buf_munit	= -1;
		(*_ppool)->alloc_type			= MEMORY_ALLOCATE_TYPE_MMAP;
		(*_ppool)->endian			= gdt_endian();
		memory_unit_one_size = GDT_ALIGNUP( sizeof( GDT_MEMORY_UNIT ), alignment_size );
		memoryUnitSize = memory_unit_one_size * ( fix_memory_unit + free_memory_unit );
		(*_ppool)->bottom = (*_ppool)->end - memoryUnitSize;
		(*_ppool)->memory_unit_size_one = memory_unit_one_size;
		for( i = ((*_ppool)->unit_size-1); i >= 0; i-- )
		{
			unit = (GDT_MEMORY_UNIT*)( ( (*_ppool)->memory + (*_ppool)->end ) - ( memory_unit_one_size * ( (*_ppool)->unit_size - i ) ) );
			gdt_initialize_memory_unit( unit );
			unit->id = ( ((*_ppool)->unit_size-1) - i );
		}
		memory_size = (*_ppool)->size;
	}while( FALSE );
#endif // #if defined(__LINUX__) || defined(__BSD_UNIX__)
	return memory_size;
}

/*
 * create tiny memory pool
 */
int32_t gdt_create_mini_memory( GDT_MEMORY_POOL* _ppool, size_t allocate_size )
{
	int32_t tiny_pool_munit = -1;
	allocate_size = GDT_ALIGNUP( allocate_size, _ppool->alignment );
	if( 0 >= (tiny_pool_munit = gdt_create_munit( _ppool, sizeof(GDT_MEMORY_POOL), MEMORY_TYPE_DEFAULT )) ){
		return tiny_pool_munit;
	}
	GDT_MEMORY_POOL* tiny_pool = ( GDT_MEMORY_POOL * )GDT_POINTER( _ppool, tiny_pool_munit );
	memset( tiny_pool, 0, sizeof( GDT_MEMORY_POOL ) );
	size_t max_byte_size = sizeof( uint8_t ) * allocate_size;
	int32_t memory_buf_munit = gdt_create_munit( _ppool, max_byte_size, MEMORY_TYPE_DEFAULT );
	tiny_pool->memory = ( void* )GDT_POINTER( _ppool, memory_buf_munit );
	tiny_pool->top				= 0;
	tiny_pool->end				= max_byte_size;
	tiny_pool->bottom			= max_byte_size;
	tiny_pool->size				= max_byte_size;
	tiny_pool->max_size			= max_byte_size;
	tiny_pool->alignment		= _ppool->alignment;
	tiny_pool->min_realloc		= GDT_ALIGNUP( max_byte_size, _ppool->alignment );
	tiny_pool->fix_unit_size	= FIX_MUNIT_SIZE;
	tiny_pool->unit_size		= ( FIX_MUNIT_SIZE );
	tiny_pool->tail_munit		= -1;
	tiny_pool->lock_munit		= -1;
	tiny_pool->memory_buf_munit = memory_buf_munit;
	tiny_pool->alloc_type			= MEMORY_ALLOCATE_TYPE_MINI;
	tiny_pool->endian			= gdt_endian();
	uint32_t memory_unit_one_size = GDT_ALIGNUP( sizeof( GDT_MEMORY_UNIT ), _ppool->alignment );
	uint32_t memoryUnitSize = memory_unit_one_size * ( FIX_MUNIT_SIZE );
	tiny_pool->bottom = tiny_pool->end - memoryUnitSize;
	tiny_pool->memory_unit_size_one = memory_unit_one_size;
	int i;
	GDT_MEMORY_UNIT* unit		= NULL;
	for( i = (tiny_pool->unit_size-1); i >= 0; i-- )
	{
		unit = (GDT_MEMORY_UNIT*)( ( (uint8_t*)tiny_pool->memory + tiny_pool->end ) - ( memory_unit_one_size * ( tiny_pool->unit_size - i ) ) );
		gdt_initialize_memory_unit( unit );
		unit->id = ( (tiny_pool->unit_size-1) - i );
	}
	return tiny_pool_munit;
}

int32_t gdt_create_clone_mini_memory( GDT_MEMORY_POOL* _ppool, GDT_MEMORY_POOL* _mini_ppool )
{
	int32_t tiny_pool_munit = -1;
	if( 0 >= (tiny_pool_munit = gdt_create_munit( _ppool, sizeof(GDT_MEMORY_POOL), MEMORY_TYPE_DEFAULT )) ){
		return tiny_pool_munit;
	}
	GDT_MEMORY_POOL* tiny_pool = ( GDT_MEMORY_POOL * )GDT_POINTER( _ppool, tiny_pool_munit );
	memset( tiny_pool, 0, sizeof( GDT_MEMORY_POOL ) );
	size_t max_byte_size = _mini_ppool->size;
	int32_t memory_buf_munit = gdt_create_munit( _ppool, max_byte_size, MEMORY_TYPE_DEFAULT );
	tiny_pool->memory = ( void* )GDT_POINTER( _ppool, memory_buf_munit );
	memcpy((uint8_t*)tiny_pool->memory,(uint8_t*)_mini_ppool->memory,_mini_ppool->size);
	tiny_pool->top				= _mini_ppool->top;
	tiny_pool->end				= _mini_ppool->end;
	tiny_pool->bottom			= _mini_ppool->bottom;
	tiny_pool->size				= _mini_ppool->size;
	tiny_pool->max_size			= _mini_ppool->max_size;
	tiny_pool->alignment		= _mini_ppool->alignment;
	tiny_pool->min_realloc		= _mini_ppool->min_realloc;
	tiny_pool->fix_unit_size	= _mini_ppool->fix_unit_size;
	tiny_pool->unit_size		= _mini_ppool->unit_size;
	tiny_pool->tail_munit		= _mini_ppool->tail_munit;
	tiny_pool->lock_munit		= _mini_ppool->lock_munit;
	tiny_pool->memory_buf_munit	= memory_buf_munit;
	tiny_pool->alloc_type		= _mini_ppool->alloc_type;
	tiny_pool->endian			= _mini_ppool->endian;
	tiny_pool->memory_unit_size_one 	= _mini_ppool->memory_unit_size_one;
	//gdt_memory_info( tiny_pool );
	//gdt_memory_info( _mini_ppool );
	return tiny_pool_munit;
}

int32_t gdt_copy_mini_memory( GDT_MEMORY_POOL* _dest_ppool, GDT_MEMORY_POOL* _src_ppool )
{
	if( _dest_ppool->alloc_type != MEMORY_ALLOCATE_TYPE_MINI || _src_ppool->alloc_type != MEMORY_ALLOCATE_TYPE_MINI ){
		return GDT_SYSTEM_ERROR;
	}
	if( _dest_ppool->max_size != _src_ppool->max_size ){
		return GDT_SYSTEM_ERROR;
	}
	//memcpy((uint8_t*)_dest_ppool->memory,(uint8_t*)_src_ppool->memory,_src_ppool->size);
	memcpy((uint8_t*)_dest_ppool->memory,(uint8_t*)_src_ppool->memory,_src_ppool->top);
	memcpy(((uint8_t*)_dest_ppool->memory)+_src_ppool->bottom,((uint8_t*)_src_ppool->memory)+_src_ppool->bottom,_src_ppool->size - _src_ppool->bottom);
	_dest_ppool->top			= _src_ppool->top;
	_dest_ppool->end			= _src_ppool->end;
	_dest_ppool->bottom			= _src_ppool->bottom;
	_dest_ppool->size			= _src_ppool->size;
	_dest_ppool->max_size		= _src_ppool->max_size;
	_dest_ppool->alignment		= _src_ppool->alignment;
	_dest_ppool->min_realloc	= _src_ppool->min_realloc;
	_dest_ppool->fix_unit_size	= _src_ppool->fix_unit_size;
	_dest_ppool->unit_size		= _src_ppool->unit_size;
	_dest_ppool->tail_munit		= _src_ppool->tail_munit;
	_dest_ppool->lock_munit		= _src_ppool->lock_munit;
	_dest_ppool->alloc_type		= _src_ppool->alloc_type;
	_dest_ppool->endian			= _src_ppool->endian;
	_dest_ppool->memory_unit_size_one 	= _src_ppool->memory_unit_size_one;
	//gdt_memory_info( _dest_ppool );
	return GDT_SYSTEM_OK;
}

/*
 * clean memory pool
 */
void gdt_memory_clean( GDT_MEMORY_POOL* _ppool )
{
	GDT_MEMORY_UNIT* unit;
	int i;
	//memset( _ppool->memory, 0, _ppool->size );
	_ppool->top				= 0;
	_ppool->end				= _ppool->size;
	_ppool->bottom			= _ppool->size;
	_ppool->unit_size		= _ppool->fix_unit_size;
	_ppool->tail_munit		= -1;
	_ppool->lock_munit		= -1;
	_ppool->bottom = _ppool->end - GDT_ALIGNUP( sizeof( GDT_MEMORY_UNIT ), _ppool->alignment ) * ( _ppool->unit_size );
	_ppool->memory_unit_size_one = GDT_ALIGNUP( sizeof( GDT_MEMORY_UNIT ), _ppool->alignment );
	for( i = (_ppool->unit_size-1); i >= 0; i-- )
	{
		unit = (GDT_MEMORY_UNIT*)( ( (uint8_t*)_ppool->memory + _ppool->end ) - ( _ppool->memory_unit_size_one * ( _ppool->unit_size - i ) ) );
		gdt_initialize_memory_unit( unit );
		unit->id = ( (_ppool->unit_size-1) - i );
	}
}

/*
 * memory拡張
 * @param _ppool メモリプールのポインタ
 * @param  allocate_size(size_t) 確保するメモリのサイズ
 * @param  current_unit 新しくメモリ確保しようとしているメモリユニット
 * @return 確保したメモリのサイズ( 0なら失敗 )
 */
size_t gdt_realloc_memory( GDT_MEMORY_POOL* _ppool, size_t allocate_size, GDT_MEMORY_UNIT** current_unit )
{
	size_t memory_size	= 0;
	size_t byte_size	= 0;
	uint32_t current_unitId = ( current_unit != NULL && (*current_unit) != NULL ) ? (*current_unit)->id : 0;
	int i;
	void* newmemory;
	size_t memory_unit_one_size;
	size_t memoryUnitSize;
	GDT_MEMORY_UNIT* unit;
	GDT_MEMORY_UNIT* newUnit;
	size_t newBottomPosition;
	do{
		if( _ppool->size == _ppool->max_size ){
			printf( "_ppool memory size is full\n" );
			break;
		}
		allocate_size = GDT_ALIGNUP( allocate_size, _ppool->alignment );
		if( _ppool->size + allocate_size > _ppool->max_size ){
			allocate_size = _ppool->max_size - _ppool->size;
		}
		else{
			if( allocate_size < _ppool->min_realloc ){
#ifdef __GDT_DEBUG__
					printf( "[gdt_realloc_memory] min size check s : %zd -> s2 : %"PRIu32"\n", allocate_size, _ppool->min_realloc );
#endif
				allocate_size = _ppool->min_realloc;
			}
		}
		byte_size = sizeof( uint8_t ) * allocate_size;
		newmemory = ( void* )realloc( _ppool->memory, ( _ppool->size + byte_size ) );
		if( newmemory == NULL ){
			printf( "newmemory reallocate error" );
			break;
		}
		_ppool->memory = newmemory;
		_ppool->size			= ( _ppool->size + byte_size );
		_ppool->end			= _ppool->size;
		memory_unit_one_size = GDT_ALIGNUP( sizeof( GDT_MEMORY_UNIT ), _ppool->alignment );
		memoryUnitSize = memory_unit_one_size * _ppool->unit_size;
		newBottomPosition = ( _ppool->end ) - memoryUnitSize;
		for( i = (_ppool->unit_size-1); i >= 0; i-- )
		{
			unit = (GDT_MEMORY_UNIT*)( ( (uint8_t*)_ppool->memory + _ppool->bottom ) + ( memory_unit_one_size * i ) );
			newUnit = (GDT_MEMORY_UNIT*)( ( (uint8_t*)_ppool->memory + _ppool->end ) - ( memory_unit_one_size * ( _ppool->unit_size -i ) ) );
			if( current_unit != NULL && (*current_unit) != NULL )
			{
				if( current_unitId == unit->id ){
					*current_unit = newUnit;
				}
			}
			newUnit->p			= unit->p;
			newUnit->top		= unit->top;
			newUnit->size		= unit->size;
			newUnit->status		= unit->status;
			newUnit->id			= unit->id;
			newUnit->parent		= unit->parent;
			newUnit->child		= unit->child;
		}
		memset( ( (uint8_t*)_ppool->memory + _ppool->top ), 0, ( _ppool->bottom - _ppool->top ) );
		_ppool->bottom = newBottomPosition;
		memory_size = byte_size;
	}while( FALSE );
	return memory_size;
}

size_t gdt_mgetsize( GDT_MEMORY_POOL* _ppool, size_t size )
{
	return GDT_ALIGNUP( size, _ppool->alignment );
}

/*
 * メモリユニットの初期化
 * @param _ppool メモリプールのポインタ
 * @param unit			情報を保持するメモリユニット
 * @return 成功:0 , 失敗:0以外
 */
uint32_t gdt_initialize_memory_unit( GDT_MEMORY_UNIT * unit )
{
	unit->p			= 0;
	unit->top		= 0;
	unit->size		= 0;
	unit->status	= MEMORY_STATUS_FREE;
	unit->type		= MEMORY_TYPE_DEFAULT;
	unit->id		= unit->id;
	unit->parent	= -1;
	unit->child		= -1;
	return 0;
}

/*
 * メモリユニットの破棄
 * @param _ppool メモリプールのポインタ
 * @param unit		破棄するメモリユニット
 * @return 成功:0 , 失敗:0以外
 */
uint32_t gdt_free_memory_unit( GDT_MEMORY_POOL* _ppool, int32_t *munit_id )
{
	GDT_MEMORY_UNIT *unit;
	if( *munit_id < _ppool->fix_unit_size ){
		printf("fix memory unit can not free\n");
		return GDT_SYSTEM_ERROR;
	}
	unit = GDT_PUNIT( _ppool, *munit_id );
	if( unit == NULL ){
		printf( "[gdt_free_memory_unit] unit is NULL \n" );
		return GDT_SYSTEM_ERROR;
	}
	if( unit->status == MEMORY_STATUS_USE )
	{
		//memset( ((uint8_t*)_ppool->memory + unit->p), 0, unit->size );
		*((uint8_t*)_ppool->memory + unit->p) = '\0';
		unit->top		= unit->p;
		unit->status	= MEMORY_STATUS_GARBAGE;
		unit->type		= MEMORY_TYPE_DEFAULT;
	}
	*munit_id = -1;
	return GDT_SYSTEM_OK;
}

/*
 * メモリプールから必要なメモリを探す
 * @param _ppool メモリプールのポインタ
 * @param allocate_size	確保するメモリサイズ
 * @param unit			情報を保持するメモリユニット
 * @return 成功:0 , 失敗:0以外
 */
uint32_t gdt_malloc( GDT_MEMORY_POOL* _ppool, size_t allocate_size, GDT_MEMORY_UNIT** unit )
{
	size_t reallocSize = 0;
	size_t over_size = 0;
	GDT_MEMORY_UNIT *_parentunit;
//#ifdef __GDT_DEBUG__
//		printf( "[gdt_malloc:%d] allocate_size %zd Byte\n", _ppool->alloc_type, allocate_size );
//#endif
	allocate_size = GDT_ALIGNUP( allocate_size, _ppool->alignment );
	if( allocate_size > _ppool->max_size ){
		printf( "[gdt_malloc] allocate_size over max_size = %"PRIu32", allocate_size = %zd \n" , _ppool->max_size, allocate_size );
		return GDT_SYSTEM_ERROR;
	}
	if( allocate_size <= 0 )
	{
		printf( "[gdt_malloc] invalid allocate_size : %"PRIu32", %zd \n" , _ppool->max_size, allocate_size );
		return GDT_SYSTEM_ERROR;
	}
	if( _ppool->top + allocate_size >= _ppool->bottom )
	{
//#ifdef __GDT_DEBUG__
//		printf( "[gdt_malloc] gdt_realloc_memory : %p\n", (*unit) );
//#endif
		if( _ppool->alloc_type == MEMORY_ALLOCATE_TYPE_MALLOC ){
			reallocSize = gdt_realloc_memory( _ppool, allocate_size, unit );
		}
		if( _ppool->top + allocate_size >= _ppool->bottom )
		{
			over_size = ( _ppool->top + allocate_size ) - _ppool->bottom;
			printf( "[gdt_malloc] _ppool allocate size over %zd Byte\n", over_size );
			printf( "[gdt_malloc] reallocSize %zd Byte\n", reallocSize );
			return GDT_SYSTEM_ERROR;
		}
//#ifdef __GDT_DEBUG__
//		printf( "[gdt_malloc] new pointer : %p\n", (*unit) );
//#endif
	}
	(*unit)->p			= _ppool->top;
	(*unit)->top		= (*unit)->p;
	(*unit)->size		= allocate_size;
	(*unit)->status		= MEMORY_STATUS_USE;
	_ppool->top = _ppool->top + allocate_size;
	if( _ppool->tail_munit != -1 && _ppool->tail_munit >= _ppool->fix_unit_size )
	{
		_parentunit = GDT_PUNIT( _ppool, _ppool->tail_munit );
		if( _parentunit != NULL && _parentunit->status != MEMORY_STATUS_FREE )
		{
			(*unit)->parent = _parentunit->id;
			_parentunit->child = (*unit)->id;
		}
		else{
			printf("is free munit\n");
		}
	}
	_ppool->tail_munit = (*unit)->id;
	return GDT_SYSTEM_OK;
}

/*
 * メモリプールから必要なメモリを探す
 * @param _ppool メモリプールのポインタ
 * @param allocate_size	確保するメモリサイズ
 * @param unit			情報を保持するメモリユニット
 * @return 成功:0 , 失敗:0以外
 */
uint32_t gdt_safe_malloc( GDT_MEMORY_POOL* _ppool, size_t allocate_size, GDT_MEMORY_UNIT** unit )
{
	uint32_t error_code = 0;
	do{
		error_code = gdt_initialize_memory_unit( (*unit) );
		if( error_code != 0 )
		{
			printf(  "[gdt_safe_malloc]gdt_initialize_memory_unit error = %d\n", error_code );
			break;
		}
		error_code = gdt_malloc( _ppool, allocate_size, unit );
		if( error_code != 0 ){
			printf(  "[gdt_safe_malloc]gdt_malloc error = %d\n", error_code );
			break;
		}
	}while( false );
	return error_code;
}

/*
 * 新しい固定メモリユニットの割り当て
 * @param _ppool メモリプールのポインタ
 * @return メモリプールに配置されているメモリユニット配列のid値
 */
int32_t gdt_create_fixmunit( GDT_MEMORY_POOL* _ppool, int32_t id, size_t size )
{
	GDT_MEMORY_UNIT* _unit = NULL;
	GDT_MEMORY_UNIT* unit = NULL;
	uint32_t error_code = 0;
	do{
		_unit = gdt_get_fixmunit( _ppool, id );
		if( _unit == NULL ){
			printf( "[gdt_create_fixmunit]gdt_get_fixmunit error\n" );
			break;
		}
		if( _unit->status == MEMORY_STATUS_FREE )
		{
			error_code = gdt_safe_malloc( _ppool, size, &_unit );
			if( error_code != 0 ){
				printf( "[gdt_create_fixmunit]gdt_safe_malloc error = %d\n", error_code );
				break;
			}
		}
		unit = _unit;
	}while( false );
	return ( unit != NULL ) ? unit->id : -1;
}

/*
 * 固定メモリユニットの取得
 * 使い方としてはIDをあらかじめ決めてしまって、
 * そのIDでどこからでもアクセスしたい場合に使う
 * @param _ppool メモリプールのポインタ
 * @return メモリユニット
 */
GDT_MEMORY_UNIT* gdt_get_fixmunit( GDT_MEMORY_POOL* _ppool, int32_t id )
{
	GDT_MEMORY_UNIT* unit = NULL;
	size_t memory_unit_one_size = 0;
	do{
		if( _ppool == NULL ){
			printf(  "[gdt_get_fixmunit]_ppool is null\n" );
			break;
		}
		if( id < 0 || id >= _ppool->fix_unit_size ){
			printf(  "[gdt_get_fixmunit] out of range\n" );
			break;
		}
		memory_unit_one_size = GDT_ALIGNUP( sizeof( GDT_MEMORY_UNIT ), _ppool->alignment );
		unit = (GDT_MEMORY_UNIT*)( ( (uint8_t*)_ppool->memory + _ppool->end ) - ( memory_unit_one_size * ( id + 1 ) ) );
	}while( false );
	return unit;
}

/*
 * 新しいメモリユニットの割り当て
 * @param _ppool メモリプールのポインタ
 * @return メモリプールに配置されているメモリユニット配列のid値
 */
int32_t gdt_create_munit( GDT_MEMORY_POOL* _ppool, size_t size, uint8_t type )
{
	GDT_MEMORY_UNIT* unit = NULL;
	if( GDT_ALIGNUP( size, _ppool->alignment ) > _ppool->max_size ){
		printf( "[gdt_create_munit] allocate_size over max_size = %"PRIu32", allocate_size = %zd \n" , _ppool->max_size, size );
		return -1;
	}
	unit = gdt_find_freemunit( _ppool, size );
	if( unit == NULL ){
		printf( "[gdt_create_munit]gdt_find_freemunit error\n" );
		return -1;
	}
	if( unit->status == MEMORY_STATUS_FREE )
	{
		if( ( gdt_malloc( _ppool, size, &unit ) ) != 0 )
		{
			printf( "[gdt_create_munit]gdt_malloc error\n" );
			return -1;
		}
	}
	unit->type = type;
	return unit->id;
}

/*
 * 空メモリユニットを取得
 * @param _ppool メモリプールのポインタ
 * @return 空のメモリユニット
 */
GDT_MEMORY_UNIT* gdt_find_freemunit( GDT_MEMORY_POOL* _ppool, size_t size )
{
	GDT_MEMORY_UNIT* unit			= NULL;
	size_t memory_unit_one_size		= GDT_ALIGNUP( sizeof( GDT_MEMORY_UNIT ), _ppool->alignment );
	do{
		size = GDT_ALIGNUP( size, _ppool->alignment );
		if( ( _ppool->bottom - memory_unit_one_size ) <= _ppool->top )
		{
			size_t reallocSize = 0;
			if( _ppool->alloc_type == MEMORY_ALLOCATE_TYPE_MALLOC ){
				reallocSize = gdt_realloc_memory( _ppool, memory_unit_one_size, NULL );
			}
			if( ( _ppool->bottom - memory_unit_one_size ) <= _ppool->top )
			{
				int i;
				GDT_MEMORY_UNIT* freeunit		= NULL;
				GDT_MEMORY_UNIT* garbageunit	= NULL;
				GDT_MEMORY_UNIT* _unit		= NULL;
				for( i = _ppool->unit_size; i > _ppool->fix_unit_size; i-- ){
					_unit = (GDT_MEMORY_UNIT*)( ( (uint8_t*)_ppool->memory + _ppool->end ) - ( memory_unit_one_size * i ) );
					if( freeunit == NULL && _unit->status == MEMORY_STATUS_FREE ){
						freeunit = _unit;
						if( garbageunit != NULL ){
							break;
						}
					}
					else if( garbageunit == NULL && _unit->status == MEMORY_STATUS_GARBAGE && _unit->size >= size ){
						garbageunit = _unit;
						break;
					}
				}
				if( garbageunit != NULL){
					gdt_resize_garbage(_ppool,garbageunit,freeunit,size);
					unit = garbageunit;
					unit->status = MEMORY_STATUS_USE;
				}
				else if( freeunit != NULL ){
					unit = freeunit;
				}
				else{
					size_t over_size = ( _ppool->bottom - memory_unit_one_size ) - _ppool->top;
					printf( "[gdt_find_freemunit] GDT_MEMORY_UNIT allocate size over %zd Byte\n", over_size );
					printf( "[gdt_find_freemunit] reallocSize %zd Byte\n", reallocSize );
					break;
				}
			}
			else{
				_ppool->bottom = _ppool->bottom - memory_unit_one_size;
				unit = (GDT_MEMORY_UNIT*)( ( (uint8_t*)_ppool->memory + _ppool->bottom ) );
				gdt_initialize_memory_unit( unit );
				unit->id = _ppool->unit_size;
				++_ppool->unit_size;
			}
		}
		else{
			_ppool->bottom = _ppool->bottom - memory_unit_one_size;
			unit = (GDT_MEMORY_UNIT*)( ( (uint8_t*)_ppool->memory + _ppool->bottom ) );
			gdt_initialize_memory_unit( unit );
			unit->id = _ppool->unit_size;
			++_ppool->unit_size;
		}
	}while( false );
	return unit;
}

int gdt_resize_garbage(GDT_MEMORY_POOL* _ppool,GDT_MEMORY_UNIT* garbageunit,GDT_MEMORY_UNIT* freeunit, size_t size )
{
	GDT_MEMORY_UNIT* child_unit		= NULL;
	size_t memory_unit_one_size		= GDT_ALIGNUP( sizeof( GDT_MEMORY_UNIT ), _ppool->alignment );
	if( garbageunit->size - size >= memory_unit_one_size ){
		if( freeunit == NULL ){
			if( ( _ppool->bottom - memory_unit_one_size ) <= _ppool->top )
			{
				size_t reallocSize = 0;
				if( _ppool->alloc_type == MEMORY_ALLOCATE_TYPE_MALLOC ){
					reallocSize = gdt_realloc_memory( _ppool, memory_unit_one_size, NULL );
				}
				if( ( _ppool->bottom - memory_unit_one_size ) <= _ppool->top )
				{
					size_t over_size = ( _ppool->bottom - memory_unit_one_size ) - _ppool->top;
					printf( "[gdt_resize_garbage] GDT_MEMORY_UNIT allocate size over %zd Byte\n", over_size );
					printf( "[gdt_resize_garbage] reallocSize %zd Byte\n", reallocSize );
					return GDT_SYSTEM_ERROR;
				}
			}
			_ppool->bottom = _ppool->bottom - memory_unit_one_size;
			freeunit = (GDT_MEMORY_UNIT*)( ( (uint8_t*)_ppool->memory + _ppool->bottom ) );
			gdt_initialize_memory_unit( freeunit );
			freeunit->id = _ppool->unit_size;
			++_ppool->unit_size;
		}
		freeunit->p			= garbageunit->p+size;
		freeunit->top		= freeunit->p;
		freeunit->size		= garbageunit->size - size;
		freeunit->status	= MEMORY_STATUS_GARBAGE;
		freeunit->parent	= garbageunit->id;
		if( _ppool->tail_munit == garbageunit->id ){
			_ppool->tail_munit = freeunit->id;
		}
		garbageunit->size	= size;
		if( garbageunit->child >= 0 )
		{
			child_unit = (GDT_MEMORY_UNIT*)( ( (uint8_t*)_ppool->memory + _ppool->end ) - ( _ppool->memory_unit_size_one * ( garbageunit->child + 1 ) ) );
			if( child_unit != NULL && child_unit->status != MEMORY_STATUS_FREE )
			{
				freeunit->child		= child_unit->id;
				child_unit->parent	= freeunit->id;
			}
		}
		garbageunit->child	= freeunit->id;
	}
	//unit = garbageunit;
	//unit->status = MEMORY_STATUS_USE;
	return GDT_SYSTEM_OK;
}

/*
 * メモリユニットの取得
 * @param _ppool メモリプールのポインタ
 * @return メモリユニット構造体のアドレス
 */
GDT_MEMORY_UNIT* gdt_get_munit( GDT_MEMORY_POOL* _ppool, int32_t id )
{
	GDT_MEMORY_UNIT* unit = NULL;
	if( id < _ppool->fix_unit_size || id >= _ppool->unit_size ){
		printf(  "[gdt_get_munit]unit id out of range : %d\n", id );
		return unit;
	}
	unit = (GDT_MEMORY_UNIT*)( ( (uint8_t*)_ppool->memory + _ppool->end ) - ( _ppool->memory_unit_size_one * ( id + 1 ) ) );
	return unit;
}

/*
 * メモリユニットの管理するメモリの先頭アドレス取得
 * @param _ppool メモリプールのポインタ
 * @return メモリユニットの管理するメモリの先頭アドレス取得
 */
void* gdt_upointer( GDT_MEMORY_POOL* _ppool, int32_t id )
{
	void* pt = NULL;
	GDT_MEMORY_UNIT* unit = NULL;
	do{
		if( id < _ppool->fix_unit_size || id >= _ppool->unit_size ){
			printf(  "[gdt_upointer]unit id out of range[%d]\n", id );
			break;
		}
		unit = (GDT_MEMORY_UNIT*)( ( (uint8_t*)_ppool->memory + _ppool->end ) - ( _ppool->memory_unit_size_one * ( id + 1 ) ) );
		pt = ((uint8_t*)_ppool->memory + unit->p);
	}while( false );
	return pt;
}

void* gdt_fixupointer( GDT_MEMORY_POOL* _ppool, int32_t id )
{
	void* pt = NULL;
	GDT_MEMORY_UNIT* unit = NULL;
	do{
		if( id < 0 || id >= _ppool->fix_unit_size ){
			printf(  "[gdt_upointer]unit id out of range[%d]\n", id );
			break;
		}
		unit = (GDT_MEMORY_UNIT*)( ( (uint8_t*)_ppool->memory + _ppool->end ) - ( _ppool->memory_unit_size_one * ( id + 1 ) ) );
		pt = ((uint8_t*)_ppool->memory + unit->p);
	}while( false );
	return pt;
}

/*
 * メモリユニットの管理するメモリのオフセット番目のアドレス取得
 * @param _ppool メモリプールのポインタ
 * @return メモリユニットの管理するメモリのオフセット番目のポインタ
 */
void* gdt_offsetpointer( GDT_MEMORY_POOL* _ppool, int32_t id, size_t size, int32_t offset )
{
	void* pt = NULL;
	GDT_MEMORY_UNIT* unit = NULL;
	if( id < 0 || id >= _ppool->unit_size ){
		printf(  "[gdt_offsetpointer]unit id out of range[%d]\n", id );
		return pt;
	}
	unit = (GDT_MEMORY_UNIT*)( ( (uint8_t*)_ppool->memory + _ppool->end ) - ( _ppool->memory_unit_size_one * ( id + 1 ) ) );
	if( ( size * offset ) >= unit->size ){
		printf(  "[gdt_offsetpointer]unit memory size out of range\n" );
		return pt;
	}
	pt = ((uint8_t*)_ppool->memory + unit->p + ( size * offset ) );
	return pt;
}

/*
 * メモリユニットの管理するメモリのサイズ取得
 * @param _ppool メモリプールのポインタ
 * @return メモリユニットの管理するメモリのサイズ取得
 */
size_t gdt_usize( GDT_MEMORY_POOL* _ppool, int32_t id )
{
	GDT_MEMORY_UNIT* unit = NULL;
	if( id < 0 || id >= _ppool->unit_size ){
		printf(  "[gdt_usize]unit id out of range[%d]\n", id );
		return 0;
	}
	unit = (GDT_MEMORY_UNIT*)( ( (uint8_t*)_ppool->memory + _ppool->end ) - ( _ppool->memory_unit_size_one * ( id + 1 ) ) );
	return unit->size;
}

/*
 *  メモリ解放
 * @param _ppool 開放するメモリプールのポインタ
 * @return 開放したメモリのサイズ( 0なら未開放 )
 */
size_t gdt_free( GDT_MEMORY_POOL* _ppool )
{
	size_t memory_size = 0;
	if( _ppool != NULL )
	{
		memory_size = _ppool->size;
		if( _ppool->memory != NULL )
		{
			if( _ppool->alloc_type == MEMORY_ALLOCATE_TYPE_MALLOC ){
				free( _ppool->memory );
			}
			else{
#if defined(__LINUX__) || defined(__BSD_UNIX__)
				munmap( _ppool->memory, memory_size );
#endif
			}
			_ppool->memory = NULL;
		}
		if( _ppool->alloc_type == MEMORY_ALLOCATE_TYPE_MALLOC ){
			free( _ppool );
		}
		else{
#if defined(__LINUX__) || defined(__BSD_UNIX__)
			munmap(_ppool, sizeof( GDT_MEMORY_POOL ) );
#endif
		}
		_ppool = NULL;
	}
	return memory_size;
}

void gdt_set_buffer( GDT_MEMORY_POOL* _ppool, int32_t id, GDT_BYTE_BUFFER* pbuffer )
{
	if( id < _ppool->fix_unit_size || id >= _ppool->unit_size ){
		printf(  "[gdt_upointer]unit id out of range[%d]\n", id );
		return;
	}
	GDT_MEMORY_UNIT* unit = (GDT_MEMORY_UNIT*)( ( (uint8_t*)_ppool->memory + _ppool->end ) - ( _ppool->memory_unit_size_one * ( id + 1 ) ) );
	pbuffer->endian = _ppool->endian;
	pbuffer->buffer = ((uint8_t*)_ppool->memory + unit->p);
	pbuffer->pos = pbuffer->buffer;
	pbuffer->size = unit->size;
}

uint16_t gdt_pop_little_to_host_bit16( GDT_BYTE_BUFFER* pbuffer )
{
	uint16_t v = 0;
	if( pbuffer->endian == GDT_BIG_ENDIAN ){
		v = BYTE_SWAP_BIT16( *( (uint16_t*)pbuffer->pos ) );
	}
	else{
		v = *( (uint16_t*)pbuffer->pos );
	}
	pbuffer->pos+=2;
	return v;
}

uint16_t gdt_pop_big_to_host_bit16( GDT_BYTE_BUFFER* pbuffer )
{
	uint16_t v = 0;
	if( pbuffer->endian == GDT_LITTLE_ENDIAN ){
		v = BYTE_SWAP_BIT16( *( (uint16_t*)pbuffer->pos ) );
	}
	else{
		v = *( (uint16_t*)pbuffer->pos );
	}
	pbuffer->pos+=2;
	return v;
}

uint32_t gdt_pop_little_to_host_bit32( GDT_BYTE_BUFFER* pbuffer )
{
	uint32_t v = 0;
	if( pbuffer->endian == GDT_BIG_ENDIAN ){
		v = BYTE_SWAP_BIT32( *( (uint32_t*)pbuffer->pos ) );
	}
	else{
		v = *( (uint32_t*)pbuffer->pos );
	}
	pbuffer->pos+=4;
	return v;
}

uint32_t gdt_pop_big_to_host_bit32( GDT_BYTE_BUFFER* pbuffer )
{
	uint32_t v = 0;
	if( pbuffer->endian == GDT_LITTLE_ENDIAN ){
		v = BYTE_SWAP_BIT32( *( (uint32_t*)pbuffer->pos ) );
	}
	else{
		v = *( (uint32_t*)pbuffer->pos );
	}
	pbuffer->pos+=4;
	return v;
}

int32_t gdt_create_memory_info( GDT_MEMORY_POOL* _ppool, GDT_BYTE_BUFFER* pbuffer )
{
	MEMORY_PUSH_BIT32_L( _ppool, pbuffer->pos, _ppool->end );
	MEMORY_PUSH_BIT32_L( _ppool, pbuffer->pos, _ppool->top );
	MEMORY_PUSH_BIT32_L( _ppool, pbuffer->pos, _ppool->bottom );
	MEMORY_PUSH_BIT32_L( _ppool, pbuffer->pos, _ppool->size );
	MEMORY_PUSH_BIT32_L( _ppool, pbuffer->pos, _ppool->max_size );
	MEMORY_PUSH_BIT32_L( _ppool, pbuffer->pos, _ppool->alignment );
	MEMORY_PUSH_BIT32_L( _ppool, pbuffer->pos, _ppool->memory_unit_size_one );
	MEMORY_PUSH_BIT32_L( _ppool, pbuffer->pos, _ppool->min_realloc );
	MEMORY_PUSH_BIT32_L( _ppool, pbuffer->pos, _ppool->fix_unit_size );
	MEMORY_PUSH_BIT32_L( _ppool, pbuffer->pos, _ppool->unit_size );
	MEMORY_PUSH_BIT32_L( _ppool, pbuffer->pos, _ppool->tail_munit );
	MEMORY_PUSH_BIT32_L( _ppool, pbuffer->pos, _ppool->lock_munit );
	MEMORY_PUSH_BIT32_L( _ppool, pbuffer->pos, _ppool->alloc_type );
	MEMORY_PUSH_BIT32_L( _ppool, pbuffer->pos, _ppool->endian );
//	pbuffer->pos = pbuffer->buffer;
//	printf("pbuffer size : %dbyte\n",(int)pbuffer->size);
//	printf("memory_pool->end : %d ?? %d\n", _ppool->end, gdt_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->top : %d ?? %d\n", _ppool->top, gdt_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->bottom : %d ?? %d\n", _ppool->bottom, gdt_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->size : %d ?? %d\n", _ppool->size, gdt_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->max_size : %d ?? %d\n", _ppool->max_size, gdt_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->alignment : %d ?? %d\n", _ppool->alignment, gdt_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->memory_unit_size_one : %d ?? %d\n", _ppool->memory_unit_size_one, gdt_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->min_realloc : %d ?? %d\n", _ppool->min_realloc, gdt_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->fix_unit_size : %d ?? %d\n", _ppool->fix_unit_size, gdt_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->unit_size : %d ?? %d\n", _ppool->unit_size, gdt_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->tail_munit : %d ?? %d\n", _ppool->tail_munit, gdt_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->lock_munit : %d ?? %d\n", _ppool->lock_munit, gdt_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->alloc_type : %d ?? %d\n", _ppool->alloc_type, gdt_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->endian : %d ?? %d\n", _ppool->endian, gdt_pop_little_to_host_bit32( pbuffer ));
//	pbuffer->pos = pbuffer->buffer;
	return GDT_SYSTEM_OK;
}

void gdt_memory_info( GDT_MEMORY_POOL* _ppool )
{
	size_t freeSize = 0;
	if( _ppool == NULL ){
		printf(  "[gdt_memory_info]_ppool is null\n" );
		return;
	}
	printf(  "#############################################################\n");
	printf(  "# memory info\n");
	printf(  "#############################################################\n");
	freeSize = ( ( _ppool->bottom - _ppool->top ) );
	printf(  "total : %d Byte\n", _ppool->size );
	printf(  "use   : %zd Byte\n", _ppool->size - freeSize );
	printf(  "free  : %zd Byte\n", freeSize );
	printf(  "units : %d\n", _ppool->unit_size );
	printf(  "max size : %d Byte\n", _ppool->max_size );
	printf(  "memory top = %p\n", ( (uint8_t*)_ppool->memory ) );
	printf(  "memory end = %p\n", ( (uint8_t*)_ppool->memory + _ppool->size ) );
	printf(  "memory freetop = %p\n", ( (uint8_t*)_ppool->memory + _ppool->top ) );
	printf(  "memory freebottom = %p\n", ( (uint8_t*)_ppool->memory + _ppool->bottom ) );
	printf(  "#############################################################\n");
}

void gdt_memory_unit_info( GDT_MEMORY_POOL* _ppool )
{
	int i;
	uint32_t freeUnitCount = 0;
	uint32_t garbageUnitCount = 0;
	GDT_MEMORY_UNIT* unit;
	size_t memory_unit_one_size = 0;
	if( _ppool == NULL ){
		printf(  "[gdt_memory_unit_info]_ppool is null\n" );
		return;
	}
	printf(  "#############################################################\n");
	printf(  "# -- MEMORY_UNIT size %zd Byte --\n", (size_t)( _ppool->end - _ppool->bottom ) );
	printf(  "# total memory units = %"PRIu32"\n", _ppool->unit_size );
	printf(  "#############################################################\n");
	memory_unit_one_size = GDT_ALIGNUP( sizeof( GDT_MEMORY_UNIT ), _ppool->alignment );
	for( i = (_ppool->unit_size-1); i >= 0; i-- )
	{
		unit = (GDT_MEMORY_UNIT*)( ( (uint8_t*)_ppool->memory + _ppool->end ) - ( memory_unit_one_size * ( _ppool->unit_size - i ) ) );
		(void) fprintf(
				stdout
			, "[%d] addr:%p:%p, p:%p, pend:%p, memsize:%d Byte, status:%d, id:%d, parent:%d, child:%d, type:%d\n"
			, _ppool->unit_size - ( i + 1 )
			, unit
			, (uint8_t*)(unit) + memory_unit_one_size
			, (uint8_t*)_ppool->memory + unit->p
			, (uint8_t*)_ppool->memory + unit->p + unit->size
			, unit->size
			, unit->status
			, unit->id
			, unit->parent
			, unit->child
			, unit->type
		);
		if( unit->status == MEMORY_STATUS_FREE ){
			++freeUnitCount;
		}
		else if( unit->status == MEMORY_STATUS_GARBAGE ){
			++garbageUnitCount;
		}
	}
	printf(  "free memory units = %d\n", freeUnitCount );
	printf(  "garbage memory units = %d\n", garbageUnitCount );
	printf(  "#############################################################\n");
}

int gdt_endian()
{
	int v = 1;
	if( *(char*)&v ){
#ifdef __GDT_DEBUG__
		//printf( "GDT_LITTLE_ENDIAN\n" );
#endif
		return GDT_LITTLE_ENDIAN;
	}
	else{
#ifdef __GDT_DEBUG__
		//printf( "GDT_BIG_ENDIAN\n" );
#endif
		return GDT_BIG_ENDIAN;
	}
}
