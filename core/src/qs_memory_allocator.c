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

#include "qs_memory_allocator.h"
#ifdef __WINDOWS__
#pragma warning(disable : 4996)
#endif

// 64bit example -> 1024LLU * 1024LLU * 1024LLU * 4LLU
size_t qs_initialize_memory( QS_MEMORY_POOL** _ppool, size_t allocate_size, size_t max_allocate_size, size_t alignment_size, size_t fix_memory_unit, size_t free_memory_unit, size_t min_realloc )
{
	size_t memory_size		= 0;
	size_t byte_size		= 0;
	size_t max_byte_size	= 0;
	size_t memory_unit_one_size = 0;
	size_t memoryUnitSize = 0;
	QS_MEMORY_UNIT* unit;
	int64_t i;
	do{
		allocate_size = QS_ALIGNUP( allocate_size, alignment_size );
		if( ( (*_ppool) = ( QS_MEMORY_POOL * )malloc( sizeof( QS_MEMORY_POOL ) ) ) == NULL ){
			printf( "_ppool allocate error\n" );
			break;
		}
		memset( (*_ppool), 0, sizeof( QS_MEMORY_POOL ) );
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
		(*_ppool)->min_realloc		= QS_ALIGNUP( min_realloc, alignment_size );
		(*_ppool)->fix_unit_size	= fix_memory_unit;
		(*_ppool)->unit_size		= ( free_memory_unit + fix_memory_unit );
		(*_ppool)->tail_munit		= -1;
		(*_ppool)->lock_munit		= -1;
		(*_ppool)->memory_buf_munit	= -1;
		(*_ppool)->alloc_type			= MEMORY_ALLOCATE_TYPE_MALLOC;
		(*_ppool)->endian			= qs_endian();
		(*_ppool)->debug			= MEMORY_DEBUG;
		memory_unit_one_size = QS_ALIGNUP( sizeof( QS_MEMORY_UNIT ), alignment_size );
		memoryUnitSize = memory_unit_one_size * ( fix_memory_unit + free_memory_unit );
		(*_ppool)->bottom = (*_ppool)->end - memoryUnitSize;
		(*_ppool)->memory_unit_size_one = memory_unit_one_size;
		for( i = ((*_ppool)->unit_size-1); i >= 0; i-- )
		{
			unit = (QS_MEMORY_UNIT*)( ( (uint8_t*)(*_ppool)->memory + (*_ppool)->end ) - ( memory_unit_one_size * ( (*_ppool)->unit_size - i ) ) );
			qs_initialize_memory_unit( unit );
			unit->id = ( ((*_ppool)->unit_size-1) - i );
		}
		memory_size = (*_ppool)->size;
	}while( FALSE );
	return memory_size;
}

size_t qs_initialize_memory_f64( QS_MEMORY_POOL** _ppool, size_t allocate_size )
{
	return qs_initialize_memory(_ppool, allocate_size, allocate_size, MEMORY_ALIGNMENT_SIZE_BIT_64, 0, 0, 0);
}

size_t qs_initialize_mmapmemory( QS_MEMORY_POOL** _ppool, size_t allocate_size, size_t max_allocate_size, size_t alignment_size, size_t fix_memory_unit, size_t free_memory_unit, size_t min_realloc )
{
	size_t memory_size			= 0;
#if defined(__LINUX__) || defined(__BSD_UNIX__)
	size_t byte_size			= 0;
	size_t max_byte_size		= 0;
	size_t memory_unit_one_size	= 0;
	size_t memoryUnitSize		= 0;
	QS_MEMORY_UNIT* unit		= NULL;
	int i;
	do{
		if( fix_memory_unit < FIX_MUNIT_SIZE ){
			fix_memory_unit = FIX_MUNIT_SIZE;
		}
		allocate_size = QS_ALIGNUP( allocate_size, alignment_size );
		if( ( (*_ppool) = ( QS_MEMORY_POOL * )mmap( 0, sizeof( QS_MEMORY_POOL ), PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0 ) ) == NULL ){
			printf( "_ppool allocate error\n" );
			break;
		}
		memset( (*_ppool), 0, sizeof( QS_MEMORY_POOL ) );
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
		(*_ppool)->min_realloc		= QS_ALIGNUP( min_realloc, alignment_size );
		(*_ppool)->fix_unit_size	= fix_memory_unit;
		(*_ppool)->unit_size		= ( free_memory_unit + fix_memory_unit );
		(*_ppool)->tail_munit		= -1;
		(*_ppool)->lock_munit		= -1;
		(*_ppool)->memory_buf_munit	= -1;
		(*_ppool)->mmap_fd			= -1;
		(*_ppool)->alloc_type			= MEMORY_ALLOCATE_TYPE_MMAP;
		(*_ppool)->endian			= qs_endian();
		(*_ppool)->debug			= MEMORY_DEBUG;
		memory_unit_one_size = QS_ALIGNUP( sizeof( QS_MEMORY_UNIT ), alignment_size );
		memoryUnitSize = memory_unit_one_size * ( fix_memory_unit + free_memory_unit );
		(*_ppool)->bottom = (*_ppool)->end - memoryUnitSize;
		(*_ppool)->memory_unit_size_one = memory_unit_one_size;
		for( i = ((*_ppool)->unit_size-1); i >= 0; i-- )
		{
			unit = (QS_MEMORY_UNIT*)( ( (*_ppool)->memory + (*_ppool)->end ) - ( memory_unit_one_size * ( (*_ppool)->unit_size - i ) ) );
			qs_initialize_memory_unit( unit );
			unit->id = ( ((*_ppool)->unit_size-1) - i );
		}
		memory_size = (*_ppool)->size;
	}while( FALSE );
#endif // #if defined(__LINUX__) || defined(__BSD_UNIX__)
	return memory_size;
}

size_t qs_initialize_mmapmemory_f64( QS_MEMORY_POOL** _ppool, size_t allocate_size )
{
	return qs_initialize_memory(_ppool, allocate_size, allocate_size, MEMORY_ALIGNMENT_SIZE_BIT_64, 0, 0, 0);
}

size_t qs_initialize_mmapmemory_f( const char* file_name, QS_MEMORY_POOL** _ppool, size_t allocate_size )
{
	size_t memory_size			= 0;
#if defined(__LINUX__) || defined(__BSD_UNIX__)
	size_t byte_size			= 0;
	size_t max_byte_size		= 0;
	size_t memory_unit_one_size	= 0;
	size_t memoryUnitSize		= 0;
	size_t free_memory_unit = 0;
	size_t fix_memory_unit = FIX_MUNIT_SIZE;
	size_t min_realloc = 0;
	allocate_size = QS_ALIGNUP( allocate_size, MEMORY_ALIGNMENT_SIZE_BIT_64 );
	if( ( (*_ppool) = ( QS_MEMORY_POOL * )mmap( 0, sizeof( QS_MEMORY_POOL ), PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0 ) ) == NULL ){
		return 0;
	}
	memset( (*_ppool), 0, sizeof( QS_MEMORY_POOL ) );
	byte_size = sizeof( uint8_t ) * allocate_size;
	max_byte_size = sizeof( uint8_t ) * allocate_size;
	if( max_byte_size < byte_size ){
		max_byte_size = byte_size;
	}
	int not_exist = 0;
	struct stat st;
	if (lstat(file_name, &st) < 0){
		not_exist = 1;
	}
	int fd = open(file_name, O_CREAT | O_RDWR, 0755);
	if( fd < 0 ){
		return 0;
	}
	
	if(not_exist==1){
		// make fill file
		int error = 0;
		char c = 0;
		if(0>(error = lseek(fd, byte_size, SEEK_SET))){
			close(fd);
			return 0;
		}
		if(-1==write(fd, &c, sizeof(char))){
			close(fd);
			return 0;
		}
		if(0>(error = lseek(fd, 0, SEEK_SET))){
			close(fd);
			return 0;
		}
	}
	if( ( (*_ppool)->memory = ( void* )mmap( 0, byte_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 ) ) == NULL ){
		return 0;
	}
	if(not_exist==1){
		memset( (*_ppool)->memory, 0, byte_size );
	}
	(*_ppool)->top				= 0;
	(*_ppool)->end				= byte_size;
	(*_ppool)->bottom			= byte_size;
	(*_ppool)->size				= byte_size;
	(*_ppool)->max_size			= max_byte_size;
	(*_ppool)->alignment		= MEMORY_ALIGNMENT_SIZE_BIT_64;
	(*_ppool)->min_realloc		= QS_ALIGNUP( min_realloc, MEMORY_ALIGNMENT_SIZE_BIT_64 );
	(*_ppool)->fix_unit_size	= fix_memory_unit;
	(*_ppool)->unit_size		= ( free_memory_unit + fix_memory_unit );
	(*_ppool)->tail_munit		= -1;
	(*_ppool)->lock_munit		= -1;
	(*_ppool)->memory_buf_munit	= -1;
	(*_ppool)->mmap_fd			= fd;
	(*_ppool)->alloc_type		= MEMORY_ALLOCATE_TYPE_MMAP;
	(*_ppool)->endian			= qs_endian();
	(*_ppool)->debug			= MEMORY_DEBUG;
	memory_unit_one_size = QS_ALIGNUP( sizeof( QS_MEMORY_UNIT ), MEMORY_ALIGNMENT_SIZE_BIT_64 );
	memoryUnitSize = memory_unit_one_size * ( fix_memory_unit + free_memory_unit );
	(*_ppool)->bottom = (*_ppool)->end - memoryUnitSize;
	(*_ppool)->memory_unit_size_one = memory_unit_one_size;
	
	if(not_exist==1){
		int i;
		QS_MEMORY_UNIT* unit		= NULL;
		for( i = ((*_ppool)->unit_size-1); i >= 0; i-- ){
			unit = (QS_MEMORY_UNIT*)( ( (*_ppool)->memory + (*_ppool)->end ) - ( memory_unit_one_size * ( (*_ppool)->unit_size - i ) ) );
			qs_initialize_memory_unit( unit );
			unit->id = ( ((*_ppool)->unit_size-1) - i );
		}
		// make memory header
		int32_t header_size = 56;
		(*_ppool)->top = header_size;
		QS_BYTE_BUFFER buffer;
		buffer.endian = (*_ppool)->endian;
		buffer.buffer = (uint8_t*)((*_ppool)->memory);
		buffer.pos = buffer.buffer;
		buffer.size = header_size; // 4byte * 14
		qs_create_memory_info((*_ppool),&buffer);
	} else{
		int32_t* pv = (*_ppool)->memory;
		(*_ppool)->end = *(pv++);
		(*_ppool)->top = *(pv++);
		(*_ppool)->bottom = *(pv++);
		(*_ppool)->size = *(pv++);
		(*_ppool)->max_size = *(pv++);
		(*_ppool)->alignment = *(pv++);
		(*_ppool)->memory_unit_size_one = *(pv++);
		(*_ppool)->min_realloc = *(pv++);
		(*_ppool)->fix_unit_size = *(pv++);
		(*_ppool)->unit_size = *(pv++);
		(*_ppool)->tail_munit = *(pv++);
		(*_ppool)->memory_buf_munit = *(pv++);
		(*_ppool)->alloc_type = *(pv++);
		(*_ppool)->endian = *(pv++);
	}
	
	memory_size = (*_ppool)->size;
#endif // #if defined(__LINUX__) || defined(__BSD_UNIX__)
#ifdef __WINDOWS__
	if (((*_ppool) = (QS_MEMORY_POOL *)malloc(sizeof(QS_MEMORY_POOL))) == NULL) {
		printf("_ppool allocate error\n");
		return 0;
	}
	memset((*_ppool), 0, sizeof(QS_MEMORY_POOL));

	size_t byte_size = 0;
	size_t max_byte_size = 0;
	size_t memory_unit_one_size = 0;
	size_t memoryUnitSize = 0;
	size_t free_memory_unit = 0;
	size_t fix_memory_unit = FIX_MUNIT_SIZE;
	size_t min_realloc = 0;
	allocate_size = QS_ALIGNUP(allocate_size, MEMORY_ALIGNMENT_SIZE_BIT_64);
	byte_size = sizeof(uint8_t) * allocate_size;
	max_byte_size = sizeof(uint8_t) * allocate_size;
	if (max_byte_size < byte_size) {
		max_byte_size = byte_size;
	}
	int not_exist = 0;
	struct stat st;
	if (stat(file_name, &st) < 0) {
		not_exist = 1;
	}

	int fd = open(file_name, O_CREAT | O_RDWR, 0755);
	if (fd < 0) {
		printf("open error\n");
		return 0;
	}

	if (not_exist == 1) {
		// make fill file
		int error = 0;
		char c = 0;
		if (0 > (error = _lseeki64(fd, byte_size, SEEK_SET))) {
			close(fd);
			printf("lseek error\n");
			return 0;
		}
		if (-1 == write(fd, &c, sizeof(char))) {
			close(fd);
			printf("write error\n");
			return 0;
		}
		if (0 > (error = lseek(fd, 0, SEEK_SET))) {
			close(fd);
			printf("lseek2 error\n");
			return 0;
		}
	}
	close(fd);

	DWORD access = GENERIC_READ | GENERIC_WRITE;
	DWORD open = OPEN_EXISTING;
	if (INVALID_HANDLE_VALUE == ((*_ppool)->h_file = CreateFile(file_name, access, 0, 0, open, 0, 0))) {
		printf("CreateFile error : %d\n",GetLastError());
		return 0;
	}
	DWORD map_flag = PAGE_READWRITE;
	if (0 > ((*_ppool)->h_map = CreateFileMapping((*_ppool)->h_file, 0, map_flag, 0, 0, file_name))) {
		CloseHandle((*_ppool)->h_file);
		(*_ppool)->h_file = INVALID_HANDLE_VALUE;
		printf("CreateFileMapping error\n");
		return 0;
	}
	DWORD view_flag = FILE_MAP_WRITE;
	if (NULL == ((*_ppool)->memory = MapViewOfFile((*_ppool)->h_map, view_flag, 0, 0, 0))) {
		CloseHandle((*_ppool)->h_file);
		(*_ppool)->h_file = INVALID_HANDLE_VALUE;
		CloseHandle((*_ppool)->h_map);
		(*_ppool)->h_map = 0;
		printf("MapViewOfFile error\n");
		return 0;
	}

	if (not_exist == 1) {
		memset((*_ppool)->memory, 0, byte_size);
	}
	(*_ppool)->top = 0;
	(*_ppool)->end = byte_size;
	(*_ppool)->bottom = byte_size;
	(*_ppool)->size = byte_size;
	(*_ppool)->max_size = max_byte_size;
	(*_ppool)->alignment = MEMORY_ALIGNMENT_SIZE_BIT_64;
	(*_ppool)->min_realloc = QS_ALIGNUP(min_realloc, MEMORY_ALIGNMENT_SIZE_BIT_64);
	(*_ppool)->fix_unit_size = fix_memory_unit;
	(*_ppool)->unit_size = (free_memory_unit + fix_memory_unit);
	(*_ppool)->tail_munit = -1;
	(*_ppool)->lock_munit = -1;
	(*_ppool)->memory_buf_munit = -1;
	(*_ppool)->mmap_fd = 0;
	(*_ppool)->alloc_type = MEMORY_ALLOCATE_TYPE_MMAP;
	(*_ppool)->endian = qs_endian();
	(*_ppool)->debug = MEMORY_DEBUG;
	memory_unit_one_size = QS_ALIGNUP(sizeof(QS_MEMORY_UNIT), MEMORY_ALIGNMENT_SIZE_BIT_64);
	memoryUnitSize = memory_unit_one_size * (fix_memory_unit + free_memory_unit);
	(*_ppool)->bottom = (*_ppool)->end - memoryUnitSize;
	(*_ppool)->memory_unit_size_one = memory_unit_one_size;

	if (not_exist == 1) {
		int i;
		QS_MEMORY_UNIT* unit = NULL;
		for (i = ((*_ppool)->unit_size - 1); i >= 0; i--) {
			unit = (QS_MEMORY_UNIT*)(((uint8_t*)(*_ppool)->memory + (*_ppool)->end) - (memory_unit_one_size * ((*_ppool)->unit_size - i)));
			qs_initialize_memory_unit(unit);
			unit->id = (((*_ppool)->unit_size - 1) - i);
		}
		// make memory header
		int32_t header_size = 56;
		(*_ppool)->top = header_size;
		QS_BYTE_BUFFER buffer;
		buffer.endian = (*_ppool)->endian;
		buffer.buffer = (uint8_t*)((*_ppool)->memory);
		buffer.pos = buffer.buffer;
		buffer.size = header_size; // 4byte * 14
		qs_create_memory_info((*_ppool), &buffer);
	}
	else {
		int32_t* pv = (*_ppool)->memory;
		(*_ppool)->end = *(pv++);
		(*_ppool)->top = *(pv++);
		(*_ppool)->bottom = *(pv++);
		(*_ppool)->size = *(pv++);
		(*_ppool)->max_size = *(pv++);
		(*_ppool)->alignment = *(pv++);
		(*_ppool)->memory_unit_size_one = *(pv++);
		(*_ppool)->min_realloc = *(pv++);
		(*_ppool)->fix_unit_size = *(pv++);
		(*_ppool)->unit_size = *(pv++);
		(*_ppool)->tail_munit = *(pv++);
		(*_ppool)->memory_buf_munit = *(pv++);
		(*_ppool)->alloc_type = *(pv++);
		(*_ppool)->endian = *(pv++);
	}

	memory_size = (*_ppool)->size;
#endif
	return memory_size;
}

int32_t qs_sync_mmap_memory(QS_MEMORY_POOL* memory_pool)
{
	// make memory header
	int32_t header_size = 56;
	QS_BYTE_BUFFER buffer;
	buffer.endian = memory_pool->endian;
	buffer.buffer = (uint8_t*)(memory_pool->memory);
	buffer.pos = buffer.buffer;
	buffer.size = header_size; // 4byte * 14
	qs_create_memory_info(memory_pool, &buffer);
#if defined(__LINUX__) || defined(__BSD_UNIX__)
		msync(memory_pool->memory,memory_pool->size,MS_SYNC);
#endif // #if defined(__LINUX__) || defined(__BSD_UNIX__)
	return QS_SYSTEM_OK;
}

int32_t qs_async_mmap_memory(QS_MEMORY_POOL* memory_pool)
{
	// make memory header
	int32_t header_size = 56;
	QS_BYTE_BUFFER buffer;
	buffer.endian = memory_pool->endian;
	buffer.buffer = (uint8_t*)(memory_pool->memory);
	buffer.pos = buffer.buffer;
	buffer.size = header_size; // 4byte * 14
	qs_create_memory_info(memory_pool, &buffer);
#if defined(__LINUX__) || defined(__BSD_UNIX__)
		msync(memory_pool->memory,memory_pool->size,MS_ASYNC);
#endif // #if defined(__LINUX__) || defined(__BSD_UNIX__)
	return QS_SYSTEM_OK;
}

int32_t qs_create_mini_memory( QS_MEMORY_POOL* _ppool, size_t allocate_size )
{
	int32_t tiny_pool_munit = -1;
	allocate_size = QS_ALIGNUP( allocate_size, _ppool->alignment );
	if( -1 == (tiny_pool_munit = qs_create_munit( _ppool, sizeof(QS_MEMORY_POOL), MEMORY_TYPE_DEFAULT )) ){
		return tiny_pool_munit;
	}
	QS_MEMORY_POOL* tiny_pool = ( QS_MEMORY_POOL * )QS_GET_POINTER( _ppool, tiny_pool_munit );
	memset( tiny_pool, 0, sizeof( QS_MEMORY_POOL ) );
	size_t max_byte_size = sizeof( uint8_t ) * allocate_size;
	int32_t memory_buf_munit = qs_create_munit( _ppool, max_byte_size, MEMORY_TYPE_DEFAULT );
	if(-1==memory_buf_munit){
		if(_ppool->debug){
			printf("qs_create_mini_memory error\n");
		}
		return -1;
	}
	tiny_pool->memory = ( void* )QS_GET_POINTER( _ppool, memory_buf_munit );
	tiny_pool->top				= 0;
	tiny_pool->end				= max_byte_size;
	tiny_pool->bottom			= max_byte_size;
	tiny_pool->size				= max_byte_size;
	tiny_pool->max_size			= max_byte_size;
	tiny_pool->alignment		= _ppool->alignment;
	tiny_pool->min_realloc		= QS_ALIGNUP( max_byte_size, _ppool->alignment );
	tiny_pool->fix_unit_size	= FIX_MUNIT_SIZE;
	tiny_pool->unit_size		= ( FIX_MUNIT_SIZE );
	tiny_pool->tail_munit		= -1;
	tiny_pool->lock_munit		= -1;
	tiny_pool->memory_buf_munit = memory_buf_munit;
	tiny_pool->alloc_type			= MEMORY_ALLOCATE_TYPE_MINI;
	tiny_pool->endian			= qs_endian();
	tiny_pool->debug			= MEMORY_DEBUG;
	uint32_t memory_unit_one_size = QS_ALIGNUP( sizeof( QS_MEMORY_UNIT ), _ppool->alignment );
	uint32_t memoryUnitSize = memory_unit_one_size * ( FIX_MUNIT_SIZE );
	tiny_pool->bottom = tiny_pool->end - memoryUnitSize;
	tiny_pool->memory_unit_size_one = memory_unit_one_size;
	int i;
	QS_MEMORY_UNIT* unit		= NULL;
	for( i = (tiny_pool->unit_size-1); i >= 0; i-- )
	{
		unit = (QS_MEMORY_UNIT*)( ( (uint8_t*)tiny_pool->memory + tiny_pool->end ) - ( memory_unit_one_size * ( tiny_pool->unit_size - i ) ) );
		qs_initialize_memory_unit( unit );
		unit->id = ( (tiny_pool->unit_size-1) - i );
	}
	return tiny_pool_munit;
}

int32_t qs_create_clone_mini_memory( QS_MEMORY_POOL* _ppool, QS_MEMORY_POOL* _mini_ppool )
{
	int32_t tiny_pool_munit = -1;
	if( 0 >= (tiny_pool_munit = qs_create_munit( _ppool, sizeof(QS_MEMORY_POOL), MEMORY_TYPE_DEFAULT )) ){
		return tiny_pool_munit;
	}
	QS_MEMORY_POOL* tiny_pool = ( QS_MEMORY_POOL * )QS_GET_POINTER( _ppool, tiny_pool_munit );
	memset( tiny_pool, 0, sizeof( QS_MEMORY_POOL ) );
	size_t max_byte_size = _mini_ppool->size;
	int32_t memory_buf_munit = qs_create_munit( _ppool, max_byte_size, MEMORY_TYPE_DEFAULT );
	tiny_pool->memory = ( void* )QS_GET_POINTER( _ppool, memory_buf_munit );
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
	tiny_pool->debug			= _mini_ppool->debug;
	tiny_pool->memory_unit_size_one 	= _mini_ppool->memory_unit_size_one;
	return tiny_pool_munit;
}

int32_t qs_copy_mini_memory( QS_MEMORY_POOL* _dest_ppool, QS_MEMORY_POOL* _src_ppool )
{
	if( _dest_ppool->alloc_type != MEMORY_ALLOCATE_TYPE_MINI || _src_ppool->alloc_type != MEMORY_ALLOCATE_TYPE_MINI ){
		return QS_SYSTEM_ERROR;
	}
	if( _dest_ppool->max_size != _src_ppool->max_size ){
		return QS_SYSTEM_ERROR;
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
	_dest_ppool->debug			= _src_ppool->debug;
	_dest_ppool->memory_unit_size_one 	= _src_ppool->memory_unit_size_one;
	//qs_memory_info( _dest_ppool );
	return QS_SYSTEM_OK;
}

void qs_memory_clean( QS_MEMORY_POOL* _ppool )
{
	QS_MEMORY_UNIT* unit;
	int i;
	//memset( _ppool->memory, 0, _ppool->size );
	_ppool->top				= 0;
	_ppool->end				= _ppool->size;
	_ppool->bottom			= _ppool->size;
	_ppool->unit_size		= _ppool->fix_unit_size;
	_ppool->tail_munit		= -1;
	_ppool->lock_munit		= -1;
	_ppool->bottom = _ppool->end - QS_ALIGNUP( sizeof( QS_MEMORY_UNIT ), _ppool->alignment ) * ( _ppool->unit_size );
	_ppool->memory_unit_size_one = QS_ALIGNUP( sizeof( QS_MEMORY_UNIT ), _ppool->alignment );
	for( i = (_ppool->unit_size-1); i >= 0; i-- ){
		unit = (QS_MEMORY_UNIT*)( ( (uint8_t*)_ppool->memory + _ppool->end ) - ( _ppool->memory_unit_size_one * ( _ppool->unit_size - i ) ) );
		qs_initialize_memory_unit( unit );
		unit->id = ( (_ppool->unit_size-1) - i );
	}
}

size_t qs_realloc_memory( QS_MEMORY_POOL* _ppool, size_t allocate_size, QS_MEMORY_UNIT** current_unit )
{
	size_t memory_size	= 0;
	size_t byte_size	= 0;
	uint32_t current_unitId = ( current_unit != NULL && (*current_unit) != NULL ) ? (*current_unit)->id : 0;
	int i;
	void* newmemory;
	size_t memory_unit_one_size;
	size_t memoryUnitSize;
	QS_MEMORY_UNIT* unit;
	QS_MEMORY_UNIT* newUnit;
	size_t newBottomPosition;
	do{
		if( _ppool->size == _ppool->max_size ){
			printf( "_ppool memory size is full\n" );
			break;
		}
		allocate_size = QS_ALIGNUP( allocate_size, _ppool->alignment );
		if( _ppool->size + allocate_size > _ppool->max_size ){
			allocate_size = _ppool->max_size - _ppool->size;
		}
		else{
			if( allocate_size < _ppool->min_realloc ){
#ifdef __QS_DEBUG__
					printf( "[qs_realloc_memory] min size check s : %zd -> s2 : %"PRIu64"\n", allocate_size, _ppool->min_realloc );
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
		memory_unit_one_size = QS_ALIGNUP( sizeof( QS_MEMORY_UNIT ), _ppool->alignment );
		memoryUnitSize = memory_unit_one_size * _ppool->unit_size;
		newBottomPosition = ( _ppool->end ) - memoryUnitSize;
		for( i = (_ppool->unit_size-1); i >= 0; i-- )
		{
			unit = (QS_MEMORY_UNIT*)( ( (uint8_t*)_ppool->memory + _ppool->bottom ) + ( memory_unit_one_size * i ) );
			newUnit = (QS_MEMORY_UNIT*)( ( (uint8_t*)_ppool->memory + _ppool->end ) - ( memory_unit_one_size * ( _ppool->unit_size -i ) ) );
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

size_t qs_mgetsize( QS_MEMORY_POOL* _ppool, size_t size )
{
	return QS_ALIGNUP( size, _ppool->alignment );
}

uint32_t qs_initialize_memory_unit( QS_MEMORY_UNIT * unit )
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

uint32_t qs_free_memory_unit( QS_MEMORY_POOL* _ppool, int32_t *munit_id )
{
	QS_MEMORY_UNIT *unit;
	if( *munit_id < _ppool->fix_unit_size ){
		printf("fix memory unit can not free\n");
		return QS_SYSTEM_ERROR;
	}
	unit = QS_PUNIT( _ppool, *munit_id );
	if( unit == NULL ){
		printf( "[qs_free_memory_unit] unit is NULL \n" );
		return QS_SYSTEM_ERROR;
	}
	if( unit->status == MEMORY_STATUS_USE ){
		//memset( ((uint8_t*)_ppool->memory + unit->p), 0, unit->size );
		*((uint8_t*)_ppool->memory + unit->p) = '\0';
		unit->top		= unit->p;
		unit->status	= MEMORY_STATUS_GARBAGE;
		unit->type		= MEMORY_TYPE_DEFAULT;
	}
	*munit_id = -1;
	return QS_SYSTEM_OK;
}

uint32_t qs_malloc( QS_MEMORY_POOL* _ppool, size_t allocate_size, QS_MEMORY_UNIT** unit )
{
	size_t reallocSize = 0;
	size_t over_size = 0;
	QS_MEMORY_UNIT *_parentunit;
//#ifdef __QS_DEBUG__
//		printf( "[qs_malloc:%d] allocate_size %zd Byte\n", _ppool->alloc_type, allocate_size );
//#endif
	allocate_size = QS_ALIGNUP( allocate_size, _ppool->alignment );
	if( allocate_size > _ppool->max_size ){
		printf( "[qs_malloc] allocate_size over max_size = %"PRIu64", allocate_size = %zd \n" , _ppool->max_size, allocate_size );
		return QS_SYSTEM_ERROR;
	}
	if( allocate_size <= 0 )
	{
		printf( "[qs_malloc] invalid allocate_size : %"PRIu64", %zd \n" , _ppool->max_size, allocate_size );
		return QS_SYSTEM_ERROR;
	}
	if( _ppool->top + allocate_size >= _ppool->bottom )
	{
//#ifdef __QS_DEBUG__
//		printf( "[qs_malloc] qs_realloc_memory : %p\n", (*unit) );
//#endif
		if( _ppool->alloc_type == MEMORY_ALLOCATE_TYPE_MALLOC ){
			reallocSize = qs_realloc_memory( _ppool, allocate_size, unit );
		}
		if( _ppool->top + allocate_size >= _ppool->bottom )
		{
			over_size = ( _ppool->top + allocate_size ) - _ppool->bottom;
			if(_ppool->debug){
				printf( "[qs_malloc] _ppool allocate size over %zd Byte\n", over_size );
				printf( "[qs_malloc] reallocSize %zd Byte\n", reallocSize );
			}
			return QS_SYSTEM_ERROR;
		}
//#ifdef __QS_DEBUG__
//		printf( "[qs_malloc] new pointer : %p\n", (*unit) );
//#endif
	}
	(*unit)->p			= _ppool->top;
	(*unit)->top		= (*unit)->p;
	(*unit)->size		= allocate_size;
	(*unit)->status		= MEMORY_STATUS_USE;
	_ppool->top = _ppool->top + allocate_size;
	if( _ppool->tail_munit != -1 && _ppool->tail_munit >= _ppool->fix_unit_size )
	{
		_parentunit = QS_PUNIT( _ppool, _ppool->tail_munit );
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
	return QS_SYSTEM_OK;
}

uint32_t qs_safe_malloc( QS_MEMORY_POOL* _ppool, size_t allocate_size, QS_MEMORY_UNIT** unit )
{
	uint32_t error_code = 0;
	do{
		error_code = qs_initialize_memory_unit( (*unit) );
		if( error_code != 0 )
		{
			printf(  "[qs_safe_malloc]qs_initialize_memory_unit error = %d\n", error_code );
			break;
		}
		error_code = qs_malloc( _ppool, allocate_size, unit );
		if( error_code != 0 ){
			printf(  "[qs_safe_malloc]qs_malloc error = %d\n", error_code );
			break;
		}
	}while( false );
	return error_code;
}

int32_t qs_create_fixmunit( QS_MEMORY_POOL* _ppool, int32_t id, size_t size )
{
	QS_MEMORY_UNIT* _unit = NULL;
	QS_MEMORY_UNIT* unit = NULL;
	uint32_t error_code = 0;
	do{
		_unit = qs_get_fixmunit( _ppool, id );
		if( _unit == NULL ){
			printf( "[qs_create_fixmunit]qs_get_fixmunit error\n" );
			break;
		}
		if( _unit->status == MEMORY_STATUS_FREE )
		{
			error_code = qs_safe_malloc( _ppool, size, &_unit );
			if( error_code != 0 ){
				printf( "[qs_create_fixmunit]qs_safe_malloc error = %d\n", error_code );
				break;
			}
		}
		unit = _unit;
	}while( false );
	return ( unit != NULL ) ? unit->id : -1;
}

QS_MEMORY_UNIT* qs_get_fixmunit( QS_MEMORY_POOL* _ppool, int32_t id )
{
	QS_MEMORY_UNIT* unit = NULL;
	size_t memory_unit_one_size = 0;
	do{
		if( _ppool == NULL ){
			printf(  "[qs_get_fixmunit]_ppool is null\n" );
			break;
		}
		if( id < 0 || id >= _ppool->fix_unit_size ){
			printf(  "[qs_get_fixmunit] out of range\n" );
			break;
		}
		memory_unit_one_size = QS_ALIGNUP( sizeof( QS_MEMORY_UNIT ), _ppool->alignment );
		unit = (QS_MEMORY_UNIT*)( ( (uint8_t*)_ppool->memory + _ppool->end ) - ( memory_unit_one_size * ( id + 1 ) ) );
	}while( false );
	return unit;
}

int32_t qs_create_munit( QS_MEMORY_POOL* _ppool, size_t size, uint8_t type )
{
	QS_MEMORY_UNIT* unit = NULL;
	if( QS_ALIGNUP( size, _ppool->alignment ) > _ppool->max_size ){
		printf( "[qs_create_munit] allocate_size over max_size = %"PRIu64", allocate_size = %zd \n" , _ppool->max_size, size );
		return -1;
	}
	unit = qs_find_freemunit( _ppool, size );
	if( unit == NULL ){
		if(_ppool->debug){
			printf( "[qs_create_munit]qs_find_freemunit error\n" );
		}
		return -1;
	}
	if( unit->status == MEMORY_STATUS_FREE )
	{
		if( ( qs_malloc( _ppool, size, &unit ) ) != 0 )
		{
			if(_ppool->debug){
				printf( "[qs_create_munit]qs_malloc error\n" );
			}
			return -1;
		}
	}
	unit->type = type;
	return unit->id;
}

int32_t qs_create_memory_block( QS_MEMORY_POOL* _ppool, size_t size )
{
	return qs_create_munit( _ppool, size, MEMORY_TYPE_DEFAULT );
}

QS_MEMORY_UNIT* qs_find_freemunit( QS_MEMORY_POOL* _ppool, size_t size )
{
	QS_MEMORY_UNIT* unit			= NULL;
	size_t memory_unit_one_size		= QS_ALIGNUP( sizeof( QS_MEMORY_UNIT ), _ppool->alignment );
	do{
		size = QS_ALIGNUP( size, _ppool->alignment );
		if( ( _ppool->bottom - memory_unit_one_size ) <= _ppool->top )
		{
			size_t reallocSize = 0;
			if( _ppool->alloc_type == MEMORY_ALLOCATE_TYPE_MALLOC ){
				reallocSize = qs_realloc_memory( _ppool, memory_unit_one_size, NULL );
			}
			if( ( _ppool->bottom - memory_unit_one_size ) <= _ppool->top )
			{
				int i;
				QS_MEMORY_UNIT* freeunit		= NULL;
				QS_MEMORY_UNIT* garbageunit	= NULL;
				QS_MEMORY_UNIT* _unit		= NULL;
				for( i = _ppool->unit_size; i > _ppool->fix_unit_size; i-- ){
					_unit = (QS_MEMORY_UNIT*)( ( (uint8_t*)_ppool->memory + _ppool->end ) - ( memory_unit_one_size * i ) );
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
					qs_resize_garbage(_ppool,garbageunit,freeunit,size);
					unit = garbageunit;
					unit->status = MEMORY_STATUS_USE;
				}
				else if( freeunit != NULL ){
					unit = freeunit;
				}
				else{
					size_t over_size = ( _ppool->bottom - memory_unit_one_size ) - _ppool->top;
					if(_ppool->debug){
						printf( "[qs_find_freemunit] QS_MEMORY_UNIT allocate size over %zd Byte\n", over_size );
						printf( "[qs_find_freemunit] reallocSize %zd Byte\n", reallocSize );
					}
					break;
				}
			}
			else{
				_ppool->bottom = _ppool->bottom - memory_unit_one_size;
				unit = (QS_MEMORY_UNIT*)( ( (uint8_t*)_ppool->memory + _ppool->bottom ) );
				qs_initialize_memory_unit( unit );
				unit->id = _ppool->unit_size;
				++_ppool->unit_size;
			}
		}
		else{
			_ppool->bottom = _ppool->bottom - memory_unit_one_size;
			unit = (QS_MEMORY_UNIT*)( ( (uint8_t*)_ppool->memory + _ppool->bottom ) );
			qs_initialize_memory_unit( unit );
			unit->id = _ppool->unit_size;
			++_ppool->unit_size;
		}
	}while( false );
	return unit;
}

int qs_resize_garbage(QS_MEMORY_POOL* _ppool,QS_MEMORY_UNIT* garbageunit,QS_MEMORY_UNIT* freeunit, size_t size )
{
	QS_MEMORY_UNIT* child_unit		= NULL;
	size_t memory_unit_one_size		= QS_ALIGNUP( sizeof( QS_MEMORY_UNIT ), _ppool->alignment );
	if( garbageunit->size - size >= memory_unit_one_size ){
		if( freeunit == NULL ){
			if( ( _ppool->bottom - memory_unit_one_size ) <= _ppool->top )
			{
				size_t reallocSize = 0;
				if( _ppool->alloc_type == MEMORY_ALLOCATE_TYPE_MALLOC ){
					reallocSize = qs_realloc_memory( _ppool, memory_unit_one_size, NULL );
				}
				if( ( _ppool->bottom - memory_unit_one_size ) <= _ppool->top )
				{
					size_t over_size = ( _ppool->bottom - memory_unit_one_size ) - _ppool->top;
					printf( "[qs_resize_garbage] QS_MEMORY_UNIT allocate size over %zd Byte\n", over_size );
					printf( "[qs_resize_garbage] reallocSize %zd Byte\n", reallocSize );
					return QS_SYSTEM_ERROR;
				}
			}
			_ppool->bottom = _ppool->bottom - memory_unit_one_size;
			freeunit = (QS_MEMORY_UNIT*)( ( (uint8_t*)_ppool->memory + _ppool->bottom ) );
			qs_initialize_memory_unit( freeunit );
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
			child_unit = (QS_MEMORY_UNIT*)( ( (uint8_t*)_ppool->memory + _ppool->end ) - ( _ppool->memory_unit_size_one * ( garbageunit->child + 1 ) ) );
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
	return QS_SYSTEM_OK;
}

QS_MEMORY_UNIT* qs_get_munit( QS_MEMORY_POOL* _ppool, int32_t id )
{
	QS_MEMORY_UNIT* unit = NULL;
	if( id < _ppool->fix_unit_size || id >= _ppool->unit_size ){
		printf(  "[qs_get_munit]unit id out of range : %d\n", id );
		return unit;
	}
	unit = (QS_MEMORY_UNIT*)( ( (uint8_t*)_ppool->memory + _ppool->end ) - ( _ppool->memory_unit_size_one * ( id + 1 ) ) );
	return unit;
}

void* qs_upointer( QS_MEMORY_POOL* _ppool, int32_t id )
{
	void* pt = NULL;
	QS_MEMORY_UNIT* unit = NULL;
	do{
		if( id < _ppool->fix_unit_size || id >= _ppool->unit_size ){
			printf(  "[qs_upointer]unit id out of range[%d]\n", id );
			break;
		}
		unit = (QS_MEMORY_UNIT*)( ( (uint8_t*)_ppool->memory + _ppool->end ) - ( _ppool->memory_unit_size_one * ( id + 1 ) ) );
		pt = ((uint8_t*)_ppool->memory + unit->p);
	}while( false );
	return pt;
}

void* qs_fixupointer( QS_MEMORY_POOL* _ppool, int32_t id )
{
	void* pt = NULL;
	QS_MEMORY_UNIT* unit = NULL;
	do{
		if( id < 0 || id >= _ppool->fix_unit_size ){
			printf(  "[qs_upointer]unit id out of range[%d]\n", id );
			break;
		}
		unit = (QS_MEMORY_UNIT*)( ( (uint8_t*)_ppool->memory + _ppool->end ) - ( _ppool->memory_unit_size_one * ( id + 1 ) ) );
		pt = ((uint8_t*)_ppool->memory + unit->p);
	}while( false );
	return pt;
}

void* qs_offsetpointer( QS_MEMORY_POOL* _ppool, int32_t id, size_t size, int32_t offset )
{
	void* pt = NULL;
	QS_MEMORY_UNIT* unit = NULL;
	if( id < 0 || id >= _ppool->unit_size ){
		printf(  "[qs_offsetpointer]unit id out of range[%d]\n", id );
		return pt;
	}
	unit = (QS_MEMORY_UNIT*)( ( (uint8_t*)_ppool->memory + _ppool->end ) - ( _ppool->memory_unit_size_one * ( id + 1 ) ) );
	if( ( size * offset ) >= unit->size ){
		printf(  "[qs_offsetpointer]unit memory size out of range\n" );
		return pt;
	}
	pt = ((uint8_t*)_ppool->memory + unit->p + ( size * offset ) );
	return pt;
}

size_t qs_usize( QS_MEMORY_POOL* _ppool, int32_t id )
{
	QS_MEMORY_UNIT* unit = NULL;
	if( id < 0 || id >= _ppool->unit_size ){
		printf(  "[qs_usize]unit id out of range[%d]\n", id );
		return 0;
	}
	unit = (QS_MEMORY_UNIT*)( ( (uint8_t*)_ppool->memory + _ppool->end ) - ( _ppool->memory_unit_size_one * ( id + 1 ) ) );
	return unit->size;
}

size_t qs_free( QS_MEMORY_POOL* _ppool )
{
	size_t memory_size = 0;
	if( _ppool != NULL ){
		memory_size = _ppool->size;
		if( _ppool->memory != NULL ){
			if( _ppool->alloc_type == MEMORY_ALLOCATE_TYPE_MALLOC ){
				free( _ppool->memory );
			}
			else{
#if defined(__LINUX__) || defined(__BSD_UNIX__)
				if(_ppool->mmap_fd!=-1){
					close(_ppool->mmap_fd);
					_ppool->mmap_fd = -1;
				}
				munmap( _ppool->memory, memory_size );
#endif
#ifdef __WINDOWS__
				CloseHandle(_ppool->h_file);
				_ppool->h_file = INVALID_HANDLE_VALUE;
				CloseHandle(_ppool->h_map);
				_ppool->h_map = 0;
#endif
			}
			_ppool->memory = NULL;
		}
		if( _ppool->alloc_type == MEMORY_ALLOCATE_TYPE_MALLOC ){
			free( _ppool );
		}
		else{
#if defined(__LINUX__) || defined(__BSD_UNIX__)
			munmap(_ppool, sizeof( QS_MEMORY_POOL ) );
#endif
#ifdef __WINDOWS__
			free(_ppool);
#endif
		}
		_ppool = NULL;
	}
	return memory_size;
}

void qs_set_buffer( QS_MEMORY_POOL* _ppool, int32_t id, QS_BYTE_BUFFER* pbuffer )
{
	if( id < _ppool->fix_unit_size || id >= _ppool->unit_size ){
		printf(  "[qs_upointer]unit id out of range[%d]\n", id );
		return;
	}
	QS_MEMORY_UNIT* unit = (QS_MEMORY_UNIT*)( ( (uint8_t*)_ppool->memory + _ppool->end ) - ( _ppool->memory_unit_size_one * ( id + 1 ) ) );
	pbuffer->endian = _ppool->endian;
	pbuffer->buffer = ((uint8_t*)_ppool->memory + unit->p);
	pbuffer->pos = pbuffer->buffer;
	pbuffer->size = unit->size;
}

uint16_t qs_pop_little_to_host_bit16( QS_BYTE_BUFFER* pbuffer )
{
	uint16_t v = 0;
	if( pbuffer->endian == QS_BIG_ENDIAN ){
		v = BYTE_SWAP_BIT16( *( (uint16_t*)pbuffer->pos ) );
	}
	else{
		v = *( (uint16_t*)pbuffer->pos );
	}
	pbuffer->pos+=2;
	return v;
}

uint16_t qs_pop_big_to_host_bit16( QS_BYTE_BUFFER* pbuffer )
{
	uint16_t v = 0;
	if( pbuffer->endian == QS_LITTLE_ENDIAN ){
		v = BYTE_SWAP_BIT16( *( (uint16_t*)pbuffer->pos ) );
	}
	else{
		v = *( (uint16_t*)pbuffer->pos );
	}
	pbuffer->pos+=2;
	return v;
}

uint32_t qs_pop_little_to_host_bit32( QS_BYTE_BUFFER* pbuffer )
{
	uint32_t v = 0;
	if( pbuffer->endian == QS_BIG_ENDIAN ){
		v = BYTE_SWAP_BIT32( *( (uint32_t*)pbuffer->pos ) );
	}
	else{
		v = *( (uint32_t*)pbuffer->pos );
	}
	pbuffer->pos+=4;
	return v;
}

uint32_t qs_pop_big_to_host_bit32( QS_BYTE_BUFFER* pbuffer )
{
	uint32_t v = 0;
	if( pbuffer->endian == QS_LITTLE_ENDIAN ){
		v = BYTE_SWAP_BIT32( *( (uint32_t*)pbuffer->pos ) );
	}
	else{
		v = *( (uint32_t*)pbuffer->pos );
	}
	pbuffer->pos+=4;
	return v;
}

uint64_t qs_pop_little_to_host_bit64( QS_BYTE_BUFFER* pbuffer )
{
	uint64_t v = 0;
	if( pbuffer->endian == QS_BIG_ENDIAN ){
		v = BYTE_SWAP_BIT64( *( (uint64_t*)pbuffer->pos ) );
	}
	else{
		v = *( (uint64_t*)pbuffer->pos );
	}
	pbuffer->pos+=8;
	return v;
}

uint64_t qs_pop_big_to_host_bit64( QS_BYTE_BUFFER* pbuffer )
{
	uint64_t v = 0;
	if( pbuffer->endian == QS_LITTLE_ENDIAN ){
		v = BYTE_SWAP_BIT64( *( (uint64_t*)pbuffer->pos ) );
	}
	else{
		v = *( (uint64_t*)pbuffer->pos );
	}
	pbuffer->pos+=8;
	return v;
}

int32_t qs_create_memory_info( QS_MEMORY_POOL* _ppool, QS_BYTE_BUFFER* pbuffer )
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
	MEMORY_PUSH_BIT32_L( _ppool, pbuffer->pos, _ppool->memory_buf_munit );
	MEMORY_PUSH_BIT32_L( _ppool, pbuffer->pos, _ppool->alloc_type );
	MEMORY_PUSH_BIT32_L( _ppool, pbuffer->pos, _ppool->endian );
	
//	pbuffer->pos = pbuffer->buffer;
//	printf("pbuffer size : %dbyte\n",(int)pbuffer->size);
//	printf("memory_pool->end : %d ?? %d\n", _ppool->end, qs_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->top : %d ?? %d\n", _ppool->top, qs_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->bottom : %d ?? %d\n", _ppool->bottom, qs_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->size : %d ?? %d\n", _ppool->size, qs_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->max_size : %d ?? %d\n", _ppool->max_size, qs_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->alignment : %d ?? %d\n", _ppool->alignment, qs_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->memory_unit_size_one : %d ?? %d\n", _ppool->memory_unit_size_one, qs_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->min_realloc : %d ?? %d\n", _ppool->min_realloc, qs_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->fix_unit_size : %d ?? %d\n", _ppool->fix_unit_size, qs_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->unit_size : %d ?? %d\n", _ppool->unit_size, qs_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->tail_munit : %d ?? %d\n", _ppool->tail_munit, qs_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->lock_munit : %d ?? %d\n", _ppool->lock_munit, qs_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->alloc_type : %d ?? %d\n", _ppool->alloc_type, qs_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->endian : %d ?? %d\n", _ppool->endian, qs_pop_little_to_host_bit32( pbuffer ));
//	pbuffer->pos = pbuffer->buffer;
	return QS_SYSTEM_OK;
}

void qs_memory_info( QS_MEMORY_POOL* _ppool )
{
	size_t freeSize = 0;
	if( _ppool == NULL ){
		printf(  "[qs_memory_info]_ppool is null\n" );
		return;
	}
	printf(  "#############################################################\n");
	printf(  "# memory info\n");
	printf(  "#############################################################\n");
	freeSize = ( ( _ppool->bottom - _ppool->top ) );
	printf(  "total : %lu Byte\n", _ppool->size );
	printf(  "use   : %lu Byte\n", _ppool->size - freeSize );
	printf(  "free  : %lu Byte\n", freeSize );
	printf(  "units : %lu\n", _ppool->unit_size );
	printf(  "max size : %lu Byte\n", _ppool->max_size );
	printf(  "memory top = %p\n", ( (uint8_t*)_ppool->memory ) );
	printf(  "memory end = %p\n", ( (uint8_t*)_ppool->memory + _ppool->size ) );
	printf(  "memory freetop = %p\n", ( (uint8_t*)_ppool->memory + _ppool->top ) );
	printf(  "memory freebottom = %p\n", ( (uint8_t*)_ppool->memory + _ppool->bottom ) );
	printf(  "#############################################################\n");
}

void qs_memory_size( QS_MEMORY_POOL* _ppool )
{
	size_t freeSize = ( ( _ppool->bottom - _ppool->top ) );
	printf(  "use   : %zd Byte\n", _ppool->size - freeSize );
}

void qs_memory_unit_info( QS_MEMORY_POOL* _ppool )
{
	int i;
	uint32_t freeUnitCount = 0;
	uint32_t garbageUnitCount = 0;
	QS_MEMORY_UNIT* unit;
	size_t memory_unit_one_size = 0;
	if( _ppool == NULL ){
		printf(  "[qs_memory_unit_info]_ppool is null\n" );
		return;
	}
	printf(  "#############################################################\n");
	printf(  "# -- MEMORY_UNIT size %zd Byte --\n", (size_t)( _ppool->end - _ppool->bottom ) );
	printf(  "# total memory units = %"PRIu64"\n", _ppool->unit_size );
	printf(  "#############################################################\n");
	memory_unit_one_size = QS_ALIGNUP( sizeof( QS_MEMORY_UNIT ), _ppool->alignment );
	for( i = (_ppool->unit_size-1); i >= 0; i-- )
	{
		unit = (QS_MEMORY_UNIT*)( ( (uint8_t*)_ppool->memory + _ppool->end ) - ( memory_unit_one_size * ( _ppool->unit_size - i ) ) );
		(void) fprintf(
				stdout
			, "[%ld] addr:%p:%p, p:%p, pend:%p, memsize:%ld Byte, status:%d, id:%d, parent:%d, child:%d, type:%d\n"
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

int qs_endian()
{
	int v = 1;
	if( *(char*)&v ){
		return QS_LITTLE_ENDIAN;
	}
	else{
		return QS_BIG_ENDIAN;
	}
}
