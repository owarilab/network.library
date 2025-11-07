/*
 * Copyright (c) Katsuya Owari
 */

#include "qs_memory_allocator.h"
#ifdef __WINDOWS__
#pragma warning(disable : 4996)
#endif

// 64bit example -> 1024LLU * 1024LLU * 1024LLU * 4LLU
size_t qs_initialize_memory( QS_MEMORY_POOL** memory, size_t allocate_size, size_t max_allocate_size, size_t alignment_size, size_t fix_memory_unit, size_t free_memory_unit, size_t min_realloc )
{
	size_t memory_size		= 0;
	size_t byte_size		= 0;
	size_t max_byte_size	= 0;
	size_t memory_block_one_size = 0;
	size_t memoryUnitSize = 0;
	QS_MEMORY_UNIT* unit;
	int64_t i;
	do{
		allocate_size = QS_ALIGNUP( allocate_size, alignment_size );
		if( ( (*memory) = ( QS_MEMORY_POOL * )malloc( sizeof( QS_MEMORY_POOL ) ) ) == NULL ){
			printf( "memory struct allocate error\n" );
			break;
		}
		memset( (*memory), 0, sizeof( QS_MEMORY_POOL ) );
		byte_size = sizeof( uint8_t ) * allocate_size;
		max_byte_size = sizeof( uint8_t ) * max_allocate_size;
		if( max_byte_size < byte_size ){
			max_byte_size = byte_size;
		}
		if( ( (*memory)->memory = ( void* )malloc( byte_size ) ) == NULL ){
			printf( "memory allocate error\n" );
			break;
		}
		(*memory)->top				= 0;
		(*memory)->end				= byte_size;
		(*memory)->bottom			= byte_size;
		(*memory)->size				= byte_size;
		(*memory)->max_size			= max_byte_size;
		(*memory)->alignment		= alignment_size;
		(*memory)->min_realloc		= QS_ALIGNUP( min_realloc, alignment_size );
		(*memory)->fix_memory_block_size	= fix_memory_unit;
		(*memory)->block_size		= ( free_memory_unit + fix_memory_unit );
		(*memory)->tail_memory_block		= -1;
		(*memory)->lock_memory_block		= -1;
		(*memory)->memid_save_data	= -1;
		(*memory)->alloc_type			= MEMORY_ALLOCATE_TYPE_MALLOC;
		(*memory)->endian			= qs_endian();
		(*memory)->debug			= MEMORY_DEBUG;
		(*memory)->error_code       = 0;
		memory_block_one_size = QS_ALIGNUP( sizeof( QS_MEMORY_UNIT ), alignment_size );
		memoryUnitSize = memory_block_one_size * ( fix_memory_unit + free_memory_unit );
		(*memory)->bottom = (*memory)->end - memoryUnitSize;
		(*memory)->memory_block_size_one = memory_block_one_size;
		for( i = ((*memory)->block_size-1); i >= 0; i-- )
		{
			unit = (QS_MEMORY_UNIT*)( ( (uint8_t*)(*memory)->memory + (*memory)->end ) - ( memory_block_one_size * ( (*memory)->block_size - i ) ) );
			qs_initialize_memory_block( unit );
			unit->id = ( ((*memory)->block_size-1) - i );
		}
		memory_size = (*memory)->size;
	}while( FALSE );
	return memory_size;
}

size_t qs_initialize_memory_f64( QS_MEMORY_POOL** memory, size_t allocate_size )
{
	return qs_initialize_memory(memory, allocate_size, allocate_size, MEMORY_ALIGNMENT_SIZE_BIT_64, 0, 0, 0);
}

size_t qs_initialize_mmapmemory( QS_MEMORY_POOL** memory, size_t allocate_size, size_t max_allocate_size, size_t alignment_size, size_t fix_memory_unit, size_t free_memory_unit, size_t min_realloc )
{
	size_t memory_size			= 0;
#if defined(__LINUX__) || defined(__BSD_UNIX__)
	size_t byte_size			= 0;
	size_t max_byte_size		= 0;
	size_t memory_block_one_size	= 0;
	size_t memoryUnitSize		= 0;
	QS_MEMORY_UNIT* unit		= NULL;
	int i;
	do{
		if( fix_memory_unit < FIX_MUNIT_SIZE ){
			fix_memory_unit = FIX_MUNIT_SIZE;
		}
		allocate_size = QS_ALIGNUP( allocate_size, alignment_size );
		if( ( (*memory) = ( QS_MEMORY_POOL * )mmap( 0, sizeof( QS_MEMORY_POOL ), PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0 ) ) == NULL ){
			printf( "memory struct allocate error\n" );
			break;
		}
		memset( (*memory), 0, sizeof( QS_MEMORY_POOL ) );
		byte_size = sizeof( uint8_t ) * allocate_size;
		max_byte_size = sizeof( uint8_t ) * max_allocate_size;
		if( max_byte_size < byte_size ){
			max_byte_size = byte_size;
		}
		if( ( (*memory)->memory = ( void* )mmap( 0, byte_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0 ) ) == NULL ){
			printf( "memory allocate error" );
			break;
		}
		memset( (*memory)->memory, 0, byte_size );
		(*memory)->top				= 0;
		(*memory)->end				= byte_size;
		(*memory)->bottom			= byte_size;
		(*memory)->size				= byte_size;
		(*memory)->max_size			= max_byte_size;
		(*memory)->alignment		= alignment_size;
		(*memory)->min_realloc		= QS_ALIGNUP( min_realloc, alignment_size );
		(*memory)->fix_memory_block_size	= fix_memory_unit;
		(*memory)->block_size		= ( free_memory_unit + fix_memory_unit );
		(*memory)->tail_memory_block		= -1;
		(*memory)->lock_memory_block		= -1;
		(*memory)->memid_save_data	= -1;
		(*memory)->mmap_fd			= -1;
		(*memory)->alloc_type		= MEMORY_ALLOCATE_TYPE_MMAP;
		(*memory)->endian			= qs_endian();
		(*memory)->debug			= MEMORY_DEBUG;
		(*memory)->error_code	    = 0;
		memory_block_one_size = QS_ALIGNUP( sizeof( QS_MEMORY_UNIT ), alignment_size );
		memoryUnitSize = memory_block_one_size * ( fix_memory_unit + free_memory_unit );
		(*memory)->bottom = (*memory)->end - memoryUnitSize;
		(*memory)->memory_block_size_one = memory_block_one_size;
		for( i = ((*memory)->block_size-1); i >= 0; i-- )
		{
			unit = (QS_MEMORY_UNIT*)( ( (*memory)->memory + (*memory)->end ) - ( memory_block_one_size * ( (*memory)->block_size - i ) ) );
			qs_initialize_memory_block( unit );
			unit->id = ( ((*memory)->block_size-1) - i );
		}
		memory_size = (*memory)->size;
	}while( FALSE );
#endif // #if defined(__LINUX__) || defined(__BSD_UNIX__)
	return memory_size;
}

size_t qs_initialize_mmapmemory_f64( QS_MEMORY_POOL** memory, size_t allocate_size )
{
	return qs_initialize_memory(memory, allocate_size, allocate_size, MEMORY_ALIGNMENT_SIZE_BIT_64, 0, 0, 0);
}

size_t qs_initialize_mmapmemory_f( const char* file_name, QS_MEMORY_POOL** memory, size_t allocate_size )
{
	size_t memory_size			= 0;
#if defined(__LINUX__) || defined(__BSD_UNIX__)
	size_t byte_size			= 0;
	size_t max_byte_size		= 0;
	size_t memory_block_one_size	= 0;
	size_t memoryUnitSize		= 0;
	size_t free_memory_unit = 0;
	size_t fix_memory_unit = FIX_MUNIT_SIZE;
	size_t min_realloc = 0;
	allocate_size = QS_ALIGNUP( allocate_size, MEMORY_ALIGNMENT_SIZE_BIT_64 );
	if( ( (*memory) = ( QS_MEMORY_POOL * )mmap( 0, sizeof( QS_MEMORY_POOL ), PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0 ) ) == NULL ){
		return 0;
	}
	memset( (*memory), 0, sizeof( QS_MEMORY_POOL ) );
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
	if( ( (*memory)->memory = ( void* )mmap( 0, byte_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 ) ) == NULL ){
		return 0;
	}
	if(not_exist==1){
		memset( (*memory)->memory, 0, byte_size );
	}
	(*memory)->top				= 0;
	(*memory)->end				= byte_size;
	(*memory)->bottom			= byte_size;
	(*memory)->size				= byte_size;
	(*memory)->max_size			= max_byte_size;
	(*memory)->alignment		= MEMORY_ALIGNMENT_SIZE_BIT_64;
	(*memory)->min_realloc		= QS_ALIGNUP( min_realloc, MEMORY_ALIGNMENT_SIZE_BIT_64 );
	(*memory)->fix_memory_block_size	= fix_memory_unit;
	(*memory)->block_size		= ( free_memory_unit + fix_memory_unit );
	(*memory)->tail_memory_block		= -1;
	(*memory)->lock_memory_block		= -1;
	(*memory)->memid_save_data	= -1;
	(*memory)->mmap_fd			= fd;
	(*memory)->alloc_type		= MEMORY_ALLOCATE_TYPE_MMAP;
	(*memory)->endian			= qs_endian();
	(*memory)->debug			= MEMORY_DEBUG;
	(*memory)->error_code		= 0;
	memory_block_one_size = QS_ALIGNUP( sizeof( QS_MEMORY_UNIT ), MEMORY_ALIGNMENT_SIZE_BIT_64 );
	memoryUnitSize = memory_block_one_size * ( fix_memory_unit + free_memory_unit );
	(*memory)->bottom = (*memory)->end - memoryUnitSize;
	(*memory)->memory_block_size_one = memory_block_one_size;
	
	if(not_exist==1){
		int i;
		QS_MEMORY_UNIT* unit		= NULL;
		for( i = ((*memory)->block_size-1); i >= 0; i-- ){
			unit = (QS_MEMORY_UNIT*)( ( (*memory)->memory + (*memory)->end ) - ( memory_block_one_size * ( (*memory)->block_size - i ) ) );
			qs_initialize_memory_block( unit );
			unit->id = ( ((*memory)->block_size-1) - i );
		}
		// make memory header
		int32_t header_size = SAVE_MEMORY_HEADER_SIZE;
		(*memory)->top = header_size;
		QS_BYTE_BUFFER buffer;
		buffer.endian = (*memory)->endian;
		buffer.buffer = (uint8_t*)((*memory)->memory);
		buffer.pos = buffer.buffer;
		buffer.size = header_size;
		qs_create_memory_info((*memory),&buffer);
	} else{
		int32_t* pv = (*memory)->memory;
		(*memory)->end = *(pv++);
		(*memory)->top = *(pv++);
		(*memory)->bottom = *(pv++);
		(*memory)->size = *(pv++);
		(*memory)->max_size = *(pv++);
		(*memory)->alignment = *(pv++);
		(*memory)->memory_block_size_one = *(pv++);
		(*memory)->min_realloc = *(pv++);
		(*memory)->fix_memory_block_size = *(pv++);
		(*memory)->block_size = *(pv++);
		(*memory)->tail_memory_block = *(pv++);
		(*memory)->memid_save_data = *(pv++);
		(*memory)->alloc_type = *(pv++);
		(*memory)->endian = *(pv++);
	}
	
	memory_size = (*memory)->size;
#endif // #if defined(__LINUX__) || defined(__BSD_UNIX__)
#ifdef __WINDOWS__
	if (((*memory) = (QS_MEMORY_POOL *)malloc(sizeof(QS_MEMORY_POOL))) == NULL) {
		printf("memory allocate error\n");
		return 0;
	}
	memset((*memory), 0, sizeof(QS_MEMORY_POOL));

	size_t byte_size = 0;
	size_t max_byte_size = 0;
	size_t memory_block_one_size = 0;
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
	if (INVALID_HANDLE_VALUE == ((*memory)->h_file = CreateFile(file_name, access, 0, 0, open, 0, 0))) {
		printf("CreateFile error : %ld\n",GetLastError());
		return 0;
	}
	DWORD map_flag = PAGE_READWRITE;
	if (0 > ((*memory)->h_map = CreateFileMapping((*memory)->h_file, 0, map_flag, 0, 0, file_name))) {
		CloseHandle((*memory)->h_file);
		(*memory)->h_file = INVALID_HANDLE_VALUE;
		printf("CreateFileMapping error\n");
		return 0;
	}
	DWORD view_flag = FILE_MAP_WRITE;
	if (NULL == ((*memory)->memory = MapViewOfFile((*memory)->h_map, view_flag, 0, 0, 0))) {
		CloseHandle((*memory)->h_file);
		(*memory)->h_file = INVALID_HANDLE_VALUE;
		CloseHandle((*memory)->h_map);
		(*memory)->h_map = 0;
		printf("MapViewOfFile error\n");
		return 0;
	}

	if (not_exist == 1) {
		memset((*memory)->memory, 0, byte_size);
	}
	(*memory)->top = 0;
	(*memory)->end = byte_size;
	(*memory)->bottom = byte_size;
	(*memory)->size = byte_size;
	(*memory)->max_size = max_byte_size;
	(*memory)->alignment = MEMORY_ALIGNMENT_SIZE_BIT_64;
	(*memory)->min_realloc = QS_ALIGNUP(min_realloc, MEMORY_ALIGNMENT_SIZE_BIT_64);
	(*memory)->fix_memory_block_size = fix_memory_unit;
	(*memory)->block_size = (free_memory_unit + fix_memory_unit);
	(*memory)->tail_memory_block = -1;
	(*memory)->lock_memory_block = -1;
	(*memory)->memid_save_data = -1;
	(*memory)->mmap_fd = 0;
	(*memory)->alloc_type = MEMORY_ALLOCATE_TYPE_MMAP;
	(*memory)->endian = qs_endian();
	(*memory)->debug = MEMORY_DEBUG;
	memory_block_one_size = QS_ALIGNUP(sizeof(QS_MEMORY_UNIT), MEMORY_ALIGNMENT_SIZE_BIT_64);
	memoryUnitSize = memory_block_one_size * (fix_memory_unit + free_memory_unit);
	(*memory)->bottom = (*memory)->end - memoryUnitSize;
	(*memory)->memory_block_size_one = memory_block_one_size;

	if (not_exist == 1) {
		int i;
		QS_MEMORY_UNIT* unit = NULL;
		for (i = ((*memory)->block_size - 1); i >= 0; i--) {
			unit = (QS_MEMORY_UNIT*)(((uint8_t*)(*memory)->memory + (*memory)->end) - (memory_block_one_size * ((*memory)->block_size - i)));
			qs_initialize_memory_block(unit);
			unit->id = (((*memory)->block_size - 1) - i);
		}
		// make memory header
		int32_t header_size = 56;
		(*memory)->top = header_size;
		QS_BYTE_BUFFER buffer;
		buffer.endian = (*memory)->endian;
		buffer.buffer = (uint8_t*)((*memory)->memory);
		buffer.pos = buffer.buffer;
		buffer.size = header_size; // 4byte * 14
		qs_create_memory_info((*memory), &buffer);
	}
	else {
		int32_t* pv = (*memory)->memory;
		(*memory)->end = *(pv++);
		(*memory)->top = *(pv++);
		(*memory)->bottom = *(pv++);
		(*memory)->size = *(pv++);
		(*memory)->max_size = *(pv++);
		(*memory)->alignment = *(pv++);
		(*memory)->memory_block_size_one = *(pv++);
		(*memory)->min_realloc = *(pv++);
		(*memory)->fix_memory_block_size = *(pv++);
		(*memory)->block_size = *(pv++);
		(*memory)->tail_memory_block = *(pv++);
		(*memory)->memid_save_data = *(pv++);
		(*memory)->alloc_type = *(pv++);
		(*memory)->endian = *(pv++);
	}

	memory_size = (*memory)->size;
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

int32_t qs_create_mini_memory( QS_MEMORY_POOL* memory, size_t allocate_size )
{
	int32_t tiny_pool_munit = -1;
	allocate_size = QS_ALIGNUP( allocate_size, memory->alignment );
	if( -1 == (tiny_pool_munit = qs_create_memory_block( memory, sizeof(QS_MEMORY_POOL) )) ){
		return tiny_pool_munit;
	}
	QS_MEMORY_POOL* tiny_pool = ( QS_MEMORY_POOL * )QS_GET_POINTER( memory, tiny_pool_munit );
	memset( tiny_pool, 0, sizeof( QS_MEMORY_POOL ) );
	size_t max_byte_size = sizeof( uint8_t ) * allocate_size;
	int32_t memid_save_data = qs_create_memory_block( memory, max_byte_size );
	if(-1==memid_save_data){
		if(memory->debug){
			printf("qs_create_mini_memory error\n");
		}
		return -1;
	}
	tiny_pool->memory = ( void* )QS_GET_POINTER( memory, memid_save_data );
	tiny_pool->top				= 0;
	tiny_pool->end				= max_byte_size;
	tiny_pool->bottom			= max_byte_size;
	tiny_pool->size				= max_byte_size;
	tiny_pool->max_size			= max_byte_size;
	tiny_pool->alignment		= memory->alignment;
	tiny_pool->min_realloc		= QS_ALIGNUP( max_byte_size, memory->alignment );
	tiny_pool->fix_memory_block_size	= FIX_MUNIT_SIZE;
	tiny_pool->block_size		= ( FIX_MUNIT_SIZE );
	tiny_pool->tail_memory_block		= -1;
	tiny_pool->lock_memory_block		= -1;
	tiny_pool->memid_save_data = memid_save_data;
	tiny_pool->alloc_type		= MEMORY_ALLOCATE_TYPE_MINI;
	tiny_pool->endian			= qs_endian();
	tiny_pool->debug			= MEMORY_DEBUG;
	tiny_pool->error_code		= 0;
	uint32_t memory_block_one_size = QS_ALIGNUP( sizeof( QS_MEMORY_UNIT ), memory->alignment );
	uint32_t memoryUnitSize = memory_block_one_size * ( FIX_MUNIT_SIZE );
	tiny_pool->bottom = tiny_pool->end - memoryUnitSize;
	tiny_pool->memory_block_size_one = memory_block_one_size;
	int i;
	QS_MEMORY_UNIT* unit		= NULL;
	for( i = (tiny_pool->block_size-1); i >= 0; i-- )
	{
		unit = (QS_MEMORY_UNIT*)( ( (uint8_t*)tiny_pool->memory + tiny_pool->end ) - ( memory_block_one_size * ( tiny_pool->block_size - i ) ) );
		qs_initialize_memory_block( unit );
		unit->id = ( (tiny_pool->block_size-1) - i );
	}
	return tiny_pool_munit;
}

int32_t qs_create_clone_mini_memory( QS_MEMORY_POOL* memory, QS_MEMORY_POOL* _minimemory )
{
	int32_t tiny_pool_munit = -1;
	if( 0 >= (tiny_pool_munit = qs_create_memory_block( memory, sizeof(QS_MEMORY_POOL) )) ){
		return tiny_pool_munit;
	}
	QS_MEMORY_POOL* tiny_pool = ( QS_MEMORY_POOL * )QS_GET_POINTER( memory, tiny_pool_munit );
	memset( tiny_pool, 0, sizeof( QS_MEMORY_POOL ) );
	size_t max_byte_size = _minimemory->size;
	int32_t memid_save_data = qs_create_memory_block( memory, max_byte_size );
	tiny_pool->memory = ( void* )QS_GET_POINTER( memory, memid_save_data );
	memcpy((uint8_t*)tiny_pool->memory,(uint8_t*)_minimemory->memory,_minimemory->size);
	tiny_pool->top				= _minimemory->top;
	tiny_pool->end				= _minimemory->end;
	tiny_pool->bottom			= _minimemory->bottom;
	tiny_pool->size				= _minimemory->size;
	tiny_pool->max_size			= _minimemory->max_size;
	tiny_pool->alignment		= _minimemory->alignment;
	tiny_pool->min_realloc		= _minimemory->min_realloc;
	tiny_pool->fix_memory_block_size	= _minimemory->fix_memory_block_size;
	tiny_pool->block_size		= _minimemory->block_size;
	tiny_pool->tail_memory_block		= _minimemory->tail_memory_block;
	tiny_pool->lock_memory_block		= _minimemory->lock_memory_block;
	tiny_pool->memid_save_data	= memid_save_data;
	tiny_pool->alloc_type		= _minimemory->alloc_type;
	tiny_pool->endian			= _minimemory->endian;
	tiny_pool->debug			= _minimemory->debug;
	tiny_pool->error_code       = _minimemory->error_code;
	tiny_pool->memory_block_size_one 	= _minimemory->memory_block_size_one;
	return tiny_pool_munit;
}

int32_t qs_copy_mini_memory( QS_MEMORY_POOL* _destmemory, QS_MEMORY_POOL* _srcmemory )
{
	if( _destmemory->alloc_type != MEMORY_ALLOCATE_TYPE_MINI || _srcmemory->alloc_type != MEMORY_ALLOCATE_TYPE_MINI ){
		return QS_SYSTEM_ERROR;
	}
	if( _destmemory->max_size != _srcmemory->max_size ){
		return QS_SYSTEM_ERROR;
	}
	//memcpy((uint8_t*)_destmemory->memory,(uint8_t*)_srcmemory->memory,_srcmemory->size);
	memcpy((uint8_t*)_destmemory->memory,(uint8_t*)_srcmemory->memory,_srcmemory->top);
	memcpy(((uint8_t*)_destmemory->memory)+_srcmemory->bottom,((uint8_t*)_srcmemory->memory)+_srcmemory->bottom,_srcmemory->size - _srcmemory->bottom);
	_destmemory->top			= _srcmemory->top;
	_destmemory->end			= _srcmemory->end;
	_destmemory->bottom			= _srcmemory->bottom;
	_destmemory->size			= _srcmemory->size;
	_destmemory->max_size		= _srcmemory->max_size;
	_destmemory->alignment		= _srcmemory->alignment;
	_destmemory->min_realloc	= _srcmemory->min_realloc;
	_destmemory->fix_memory_block_size	= _srcmemory->fix_memory_block_size;
	_destmemory->block_size		= _srcmemory->block_size;
	_destmemory->tail_memory_block		= _srcmemory->tail_memory_block;
	_destmemory->lock_memory_block		= _srcmemory->lock_memory_block;
	_destmemory->alloc_type		= _srcmemory->alloc_type;
	_destmemory->endian			= _srcmemory->endian;
	_destmemory->debug			= _srcmemory->debug;
	_destmemory->error_code		= _srcmemory->error_code;
	_destmemory->memory_block_size_one 	= _srcmemory->memory_block_size_one;
	//qs_memory_info( _destmemory );
	return QS_SYSTEM_OK;
}

int32_t qs_resize_copy_mini_memory(QS_MEMORY_POOL* _destmemory, QS_MEMORY_POOL* _srcmemory)
{
	if (_destmemory->alloc_type != MEMORY_ALLOCATE_TYPE_MINI || _srcmemory->alloc_type != MEMORY_ALLOCATE_TYPE_MINI) {
		return QS_SYSTEM_ERROR;
	}
	if (_destmemory->max_size < _srcmemory->max_size) {
		return QS_SYSTEM_ERROR;
	}
	if (_destmemory->size < _srcmemory->size) {
		return QS_SYSTEM_ERROR;
	}
	//memcpy((uint8_t*)_destmemory->memory,(uint8_t*)_srcmemory->memory,_srcmemory->size);
	uint8_t* pbottom = ((uint8_t*)_destmemory->memory) + (_destmemory->size - ((int)(_srcmemory->size - _srcmemory->bottom)));
	memcpy((uint8_t*)_destmemory->memory, (uint8_t*)_srcmemory->memory, _srcmemory->top);
	memcpy(pbottom, ((uint8_t*)_srcmemory->memory) + _srcmemory->bottom, _srcmemory->size - _srcmemory->bottom);
	_destmemory->top = _srcmemory->top;
	//_destmemory->end = _srcmemory->end;
	_destmemory->bottom = (_destmemory->size - ((int)(_srcmemory->size - _srcmemory->bottom)));
	//_destmemory->size = _srcmemory->size;
	//_destmemory->max_size = _srcmemory->max_size;
	_destmemory->alignment = _srcmemory->alignment;
	_destmemory->min_realloc = _srcmemory->min_realloc;
	_destmemory->fix_memory_block_size = _srcmemory->fix_memory_block_size;
	_destmemory->block_size = _srcmemory->block_size;
	_destmemory->tail_memory_block = _srcmemory->tail_memory_block;
	_destmemory->lock_memory_block = _srcmemory->lock_memory_block;
	_destmemory->alloc_type = _srcmemory->alloc_type;
	_destmemory->endian = _srcmemory->endian;
	_destmemory->debug = _srcmemory->debug;
	_destmemory->error_code = _srcmemory->error_code;
	_destmemory->memory_block_size_one = _srcmemory->memory_block_size_one;
	//qs_memory_info( _destmemory );
	return QS_SYSTEM_OK;
}

void qs_memory_clean( QS_MEMORY_POOL* memory )
{
	qs_memory_clean_core( memory, 0 );
}

void qs_safe_memory_clean( QS_MEMORY_POOL* memory )
{
	qs_memory_clean_core( memory, 1 );
}

void qs_memory_clean_core( QS_MEMORY_POOL* memory, int safe_clean )
{
	QS_MEMORY_UNIT* unit;
	int i;
	if(safe_clean){
		memset( memory->memory, 0, memory->size );
	}
	memory->top				= 0;
	memory->end				= memory->size;
	memory->bottom			= memory->size;
	memory->block_size		= memory->fix_memory_block_size;
	memory->tail_memory_block		= -1;
	memory->lock_memory_block		= -1;
	memory->bottom = memory->end - QS_ALIGNUP( sizeof( QS_MEMORY_UNIT ), memory->alignment ) * ( memory->block_size );
	memory->memory_block_size_one = QS_ALIGNUP( sizeof( QS_MEMORY_UNIT ), memory->alignment );
	for( i = (memory->block_size-1); i >= 0; i-- ){
		unit = (QS_MEMORY_UNIT*)( ( (uint8_t*)memory->memory + memory->end ) - ( memory->memory_block_size_one * ( memory->block_size - i ) ) );
		qs_initialize_memory_block( unit );
		unit->id = ( (memory->block_size-1) - i );
	}
}

size_t qs_realloc_memory( QS_MEMORY_POOL* memory, size_t allocate_size, QS_MEMORY_UNIT** current_unit )
{
	size_t memory_size	= 0;
	size_t byte_size	= 0;
	uint32_t current_unitId = ( current_unit != NULL && (*current_unit) != NULL ) ? (*current_unit)->id : 0;
	int i;
	void* newmemory;
	size_t memory_block_one_size;
	size_t memoryUnitSize;
	QS_MEMORY_UNIT* unit;
	QS_MEMORY_UNIT* newUnit;
	size_t newBottomPosition;
	do{
		if( memory->size == memory->max_size ){
			if(memory->debug){
				printf( "memory memory size is full\n" );
			}
			memory->error_code = QS_MEMORY_ERROR_CODE_RESIZE;
			break;
		}
		allocate_size = QS_ALIGNUP( allocate_size, memory->alignment );
		if( memory->size + allocate_size > memory->max_size ){
			allocate_size = memory->max_size - memory->size;
		}
		else{
			if( allocate_size < memory->min_realloc ){
				if(memory->debug){
					printf( "[qs_realloc_memory] min size check s : %zd -> s2 : %"PRIu64"\n", allocate_size, memory->min_realloc );
				}
				allocate_size = memory->min_realloc;
			}
		}
		byte_size = sizeof( uint8_t ) * allocate_size;
		newmemory = ( void* )realloc( memory->memory, ( memory->size + byte_size ) );
		if( newmemory == NULL ){
			if(memory->debug){
				printf( "newmemory reallocate error" );
			}
			break;
		}
		memory->memory = newmemory;
		memory->size			= ( memory->size + byte_size );
		memory->end			= memory->size;
		memory_block_one_size = QS_ALIGNUP( sizeof( QS_MEMORY_UNIT ), memory->alignment );
		memoryUnitSize = memory_block_one_size * memory->block_size;
		newBottomPosition = ( memory->end ) - memoryUnitSize;
		for( i = (memory->block_size-1); i >= 0; i-- )
		{
			unit = (QS_MEMORY_UNIT*)( ( (uint8_t*)memory->memory + memory->bottom ) + ( memory_block_one_size * i ) );
			newUnit = (QS_MEMORY_UNIT*)( ( (uint8_t*)memory->memory + memory->end ) - ( memory_block_one_size * ( memory->block_size -i ) ) );
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
		memset( ( (uint8_t*)memory->memory + memory->top ), 0, ( memory->bottom - memory->top ) );
		memory->bottom = newBottomPosition;
		memory_size = byte_size;
	}while( FALSE );
	return memory_size;
}

size_t qs_mgetsize( QS_MEMORY_POOL* memory, size_t size )
{
	return QS_ALIGNUP( size, memory->alignment );
}

uint32_t qs_initialize_memory_block( QS_MEMORY_UNIT * unit )
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

uint32_t qs_free_memory_block( QS_MEMORY_POOL* memory, int32_t* munit_id )
{
	QS_MEMORY_UNIT *unit;
	if( *munit_id < memory->fix_memory_block_size ){
		if(memory->debug){
			printf("fix memory unit can not free\n");
		}
		return QS_SYSTEM_ERROR;
	}
	unit = QS_PUNIT( memory, *munit_id );
	if( unit == NULL ){
		if(memory->debug){
			printf( "[qs_free_memory_block] block is NULL \n" );
		}
		return QS_SYSTEM_ERROR;
	}
	if( unit->status == MEMORY_STATUS_USE ){
		//memset( ((uint8_t*)memory->memory + unit->p), 0, unit->size );
		*((uint8_t*)memory->memory + unit->p) = '\0';
		unit->top		= unit->p;
		unit->status	= MEMORY_STATUS_GARBAGE;
		unit->type		= MEMORY_TYPE_DEFAULT;
	}
	*munit_id = -1;
	return QS_SYSTEM_OK;
}

uint32_t qs_malloc( QS_MEMORY_POOL* memory, size_t allocate_size, QS_MEMORY_UNIT** unit )
{
	size_t reallocSize = 0;
	size_t over_size = 0;
	QS_MEMORY_UNIT *_parentunit;
//#ifdef __QS_DEBUG__
//		printf( "[qs_malloc:%d] allocate_size %zd Byte\n", memory->alloc_type, allocate_size );
//#endif
	allocate_size = QS_ALIGNUP( allocate_size, memory->alignment );
	if( allocate_size > memory->max_size ){
		if(memory->debug){
			printf( "[qs_malloc] allocate_size over max_size = %"PRIu64", allocate_size = %zd \n" , memory->max_size, allocate_size );
		}
		return QS_SYSTEM_ERROR;
	}
	if( allocate_size <= 0 )
	{
		if(memory->debug){
			printf( "[qs_malloc] invalid allocate_size : %"PRIu64", %zd \n" , memory->max_size, allocate_size );
		}
		return QS_SYSTEM_ERROR;
	}
	if( memory->top + allocate_size >= memory->bottom )
	{
//#ifdef __QS_DEBUG__
//		printf( "[qs_malloc] qs_realloc_memory : %p\n", (*unit) );
//#endif
		if( memory->alloc_type == MEMORY_ALLOCATE_TYPE_MALLOC ){
			reallocSize = qs_realloc_memory( memory, allocate_size, unit );
			if(memory->error_code == QS_MEMORY_ERROR_CODE_RESIZE){
				return QS_SYSTEM_ERROR;
			}
		}
		if( memory->top + allocate_size >= memory->bottom )
		{
			over_size = ( memory->top + allocate_size ) - memory->bottom;
			if(memory->debug){
				printf( "[qs_malloc] memory allocate size over %zd Byte\n", over_size );
				printf( "[qs_malloc] reallocSize %zd Byte\n", reallocSize );
			}
			return QS_SYSTEM_ERROR;
		}
//#ifdef __QS_DEBUG__
//		printf( "[qs_malloc] new pointer : %p\n", (*unit) );
//#endif
	}
	(*unit)->p			= memory->top;
	(*unit)->top		= (*unit)->p;
	(*unit)->size		= allocate_size;
	(*unit)->status		= MEMORY_STATUS_USE;
	memory->top = memory->top + allocate_size;
	if( memory->tail_memory_block != -1 && memory->tail_memory_block >= memory->fix_memory_block_size )
	{
		_parentunit = QS_PUNIT( memory, memory->tail_memory_block );
		if( _parentunit != NULL && _parentunit->status != MEMORY_STATUS_FREE )
		{
			(*unit)->parent = _parentunit->id;
			_parentunit->child = (*unit)->id;
		}
		else{
			printf("is free munit\n");
		}
	}
	memory->tail_memory_block = (*unit)->id;
	return QS_SYSTEM_OK;
}

uint32_t qs_safe_malloc( QS_MEMORY_POOL* memory, size_t allocate_size, QS_MEMORY_UNIT** unit )
{
	uint32_t error_code = 0;
	do{
		error_code = qs_initialize_memory_block( (*unit) );
		if( error_code != 0 )
		{
			printf(  "[qs_safe_malloc]qs_initialize_memory_block error = %d\n", error_code );
			break;
		}
		error_code = qs_malloc( memory, allocate_size, unit );
		if( error_code != 0 ){
			printf(  "[qs_safe_malloc]qs_malloc error = %d\n", error_code );
			break;
		}
	}while( false );
	return error_code;
}

int32_t qs_create_fix_memory_block( QS_MEMORY_POOL* memory, int32_t id, size_t size )
{
	QS_MEMORY_UNIT* _unit = NULL;
	QS_MEMORY_UNIT* unit = NULL;
	uint32_t error_code = 0;
	do{
		_unit = qs_get_fix_memory_block( memory, id );
		if( _unit == NULL ){
			printf( "[qs_create_fix_memory_block]qs_get_fix_memory_block error\n" );
			break;
		}
		if( _unit->status == MEMORY_STATUS_FREE )
		{
			error_code = qs_safe_malloc( memory, size, &_unit );
			if( error_code != 0 ){
				printf( "[qs_create_fix_memory_block]qs_safe_malloc error = %d\n", error_code );
				break;
			}
		}
		unit = _unit;
	}while( false );
	return ( unit != NULL ) ? unit->id : -1;
}

QS_MEMORY_UNIT* qs_get_fix_memory_block( QS_MEMORY_POOL* memory, int32_t id )
{
	QS_MEMORY_UNIT* unit = NULL;
	size_t memory_block_one_size = 0;
	do{
		if( memory == NULL ){
			printf(  "[qs_get_fix_memory_block]memory is null\n" );
			break;
		}
		if( id < 0 || id >= memory->fix_memory_block_size ){
			printf(  "[qs_get_fix_memory_block] out of range\n" );
			break;
		}
		memory_block_one_size = QS_ALIGNUP( sizeof( QS_MEMORY_UNIT ), memory->alignment );
		unit = (QS_MEMORY_UNIT*)( ( (uint8_t*)memory->memory + memory->end ) - ( memory_block_one_size * ( id + 1 ) ) );
	}while( false );
	return unit;
}

int32_t qs_create_memory_block( QS_MEMORY_POOL* memory, size_t size )
{
	QS_MEMORY_UNIT* unit = NULL;
	if( QS_ALIGNUP( size, memory->alignment ) > memory->max_size ){
		if(memory->debug){
			printf( "[qs_create_memory_block] allocate_size over max_size = %"PRIu64", allocate_size = %zd \n" , memory->max_size, size );
		}
		return -1;
	}
	unit = qs_find_free_memory_block( memory, size );
	if( unit == NULL ){
		if(memory->debug){
			printf( "[qs_create_memory_block]qs_find_freemunit error\n" );
		}
		return -1;
	}
	if( unit->status == MEMORY_STATUS_FREE )
	{
		if( ( qs_malloc( memory, size, &unit ) ) != 0 )
		{
			if(memory->debug){
				printf( "[qs_create_memory_block]qs_malloc error\n" );
			}
			return -1;
		}
	}
	unit->type = MEMORY_TYPE_DEFAULT;
	return unit->id;
}

QS_MEMORY_UNIT* qs_find_free_memory_block( QS_MEMORY_POOL* memory, size_t size )
{
	QS_MEMORY_UNIT* unit			= NULL;
	size_t memory_block_one_size		= QS_ALIGNUP( sizeof( QS_MEMORY_UNIT ), memory->alignment );
	do{
		size = QS_ALIGNUP( size, memory->alignment );
		if( ( memory->bottom - memory_block_one_size ) <= memory->top )
		{
			size_t reallocSize = 0;
			if( memory->alloc_type == MEMORY_ALLOCATE_TYPE_MALLOC ){
				reallocSize = qs_realloc_memory( memory, memory_block_one_size, NULL );
			}
			if( ( memory->bottom - memory_block_one_size ) <= memory->top )
			{
				int i;
				QS_MEMORY_UNIT* freeunit		= NULL;
				QS_MEMORY_UNIT* garbageunit	= NULL;
				QS_MEMORY_UNIT* _unit		= NULL;
				for( i = memory->block_size; i > memory->fix_memory_block_size; i-- ){
					_unit = (QS_MEMORY_UNIT*)( ( (uint8_t*)memory->memory + memory->end ) - ( memory_block_one_size * i ) );
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
					qs_resize_garbage(memory,garbageunit,freeunit,size);
					unit = garbageunit;
					unit->status = MEMORY_STATUS_USE;
				}
				else if( freeunit != NULL ){
					unit = freeunit;
				}
				else{
					size_t over_size = ( memory->bottom - memory_block_one_size ) - memory->top;
					if(memory->debug){
						printf( "[qs_find_freemunit] QS_MEMORY_UNIT allocate size over %zd Byte\n", over_size );
						printf( "[qs_find_freemunit] reallocSize %zd Byte\n", reallocSize );
					}
					break;
				}
			}
			else{
				memory->bottom = memory->bottom - memory_block_one_size;
				unit = (QS_MEMORY_UNIT*)( ( (uint8_t*)memory->memory + memory->bottom ) );
				qs_initialize_memory_block( unit );
				unit->id = memory->block_size;
				++memory->block_size;
			}
		}
		else{
			memory->bottom = memory->bottom - memory_block_one_size;
			unit = (QS_MEMORY_UNIT*)( ( (uint8_t*)memory->memory + memory->bottom ) );
			qs_initialize_memory_block( unit );
			unit->id = memory->block_size;
			++memory->block_size;
		}
	}while( false );
	return unit;
}

int qs_resize_garbage(QS_MEMORY_POOL* memory,QS_MEMORY_UNIT* garbageunit,QS_MEMORY_UNIT* freeunit, size_t size )
{
	QS_MEMORY_UNIT* child_unit		= NULL;
	size_t memory_block_one_size		= QS_ALIGNUP( sizeof( QS_MEMORY_UNIT ), memory->alignment );
	if( garbageunit->size - size >= memory_block_one_size ){
		if( freeunit == NULL ){
			if( ( memory->bottom - memory_block_one_size ) <= memory->top )
			{
				size_t reallocSize = 0;
				if( memory->alloc_type == MEMORY_ALLOCATE_TYPE_MALLOC ){
					reallocSize = qs_realloc_memory( memory, memory_block_one_size, NULL );
				}
				if( ( memory->bottom - memory_block_one_size ) <= memory->top )
				{
					size_t over_size = ( memory->bottom - memory_block_one_size ) - memory->top;
					if(memory->debug){
						printf( "[qs_resize_garbage] QS_MEMORY_UNIT allocate size over %zd Byte\n", over_size );
						printf( "[qs_resize_garbage] reallocSize %zd Byte\n", reallocSize );
					}
					return QS_SYSTEM_ERROR;
				}
			}
			memory->bottom = memory->bottom - memory_block_one_size;
			freeunit = (QS_MEMORY_UNIT*)( ( (uint8_t*)memory->memory + memory->bottom ) );
			qs_initialize_memory_block( freeunit );
			freeunit->id = memory->block_size;
			++memory->block_size;
		}
		freeunit->p			= garbageunit->p+size;
		freeunit->top		= freeunit->p;
		freeunit->size		= garbageunit->size - size;
		freeunit->status	= MEMORY_STATUS_GARBAGE;
		freeunit->parent	= garbageunit->id;
		if( memory->tail_memory_block == garbageunit->id ){
			memory->tail_memory_block = freeunit->id;
		}
		garbageunit->size	= size;
		if( garbageunit->child >= 0 )
		{
			child_unit = (QS_MEMORY_UNIT*)( ( (uint8_t*)memory->memory + memory->end ) - ( memory->memory_block_size_one * ( garbageunit->child + 1 ) ) );
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

QS_MEMORY_UNIT* qs_get_memory_block( QS_MEMORY_POOL* memory, int32_t id )
{
	QS_MEMORY_UNIT* unit = NULL;
	if( id < memory->fix_memory_block_size || id >= memory->block_size ){
		if(memory->debug){
			printf(  "[qs_get_memory_block]block id out of range : %d\n", id );
		}
		return unit;
	}
	unit = (QS_MEMORY_UNIT*)( ( (uint8_t*)memory->memory + memory->end ) - ( memory->memory_block_size_one * ( id + 1 ) ) );
	return unit;
}

void* qs_upointer( QS_MEMORY_POOL* memory, int32_t id )
{
	void* pt = NULL;
	QS_MEMORY_UNIT* unit = NULL;
	do{
		if( id < memory->fix_memory_block_size || id >= memory->block_size ){
			if(memory->debug){
				printf(  "[qs_upointer]unit id out of range[%d]\n", id );
			}
			break;
		}
		unit = (QS_MEMORY_UNIT*)( ( (uint8_t*)memory->memory + memory->end ) - ( memory->memory_block_size_one * ( id + 1 ) ) );
		pt = ((uint8_t*)memory->memory + unit->p);
	}while( false );
	return pt;
}

void* qs_fixupointer( QS_MEMORY_POOL* memory, int32_t id )
{
	void* pt = NULL;
	QS_MEMORY_UNIT* unit = NULL;
	do{
		if( id < 0 || id >= memory->fix_memory_block_size ){
			if(memory->debug){
				printf(  "[qs_upointer]unit id out of range[%d]\n", id );
			}
			break;
		}
		unit = (QS_MEMORY_UNIT*)( ( (uint8_t*)memory->memory + memory->end ) - ( memory->memory_block_size_one * ( id + 1 ) ) );
		pt = ((uint8_t*)memory->memory + unit->p);
	}while( false );
	return pt;
}

void* qs_offsetpointer( QS_MEMORY_POOL* memory, int32_t id, size_t size, int32_t offset )
{
	void* pt = NULL;
	QS_MEMORY_UNIT* unit = NULL;
	if( id < 0 || id >= memory->block_size ){
		printf(  "[qs_offsetpointer]unit id out of range[%d]\n", id );
		return pt;
	}
	unit = (QS_MEMORY_UNIT*)( ( (uint8_t*)memory->memory + memory->end ) - ( memory->memory_block_size_one * ( id + 1 ) ) );
	if( ( size * offset ) >= unit->size ){
		printf(  "[qs_offsetpointer]unit memory size out of range\n" );
		return pt;
	}
	pt = ((uint8_t*)memory->memory + unit->p + ( size * offset ) );
	return pt;
}

size_t qs_usize( QS_MEMORY_POOL* memory, int32_t id )
{
	QS_MEMORY_UNIT* unit = NULL;
	if( id < 0 || id >= memory->block_size ){
		printf(  "[qs_usize]unit id out of range[%d]\n", id );
		return 0;
	}
	unit = (QS_MEMORY_UNIT*)( ( (uint8_t*)memory->memory + memory->end ) - ( memory->memory_block_size_one * ( id + 1 ) ) );
	return unit->size;
}

size_t qs_free( QS_MEMORY_POOL* memory )
{
	size_t memory_size = 0;
	if( memory != NULL ){
		memory_size = memory->size;
		if( memory->memory != NULL ){
			if( memory->alloc_type == MEMORY_ALLOCATE_TYPE_MALLOC ){
				free( memory->memory );
			}
			else{
#if defined(__LINUX__) || defined(__BSD_UNIX__)
				if(memory->mmap_fd!=-1){
					close(memory->mmap_fd);
					memory->mmap_fd = -1;
				}
				munmap( memory->memory, memory_size );
#endif
#ifdef __WINDOWS__
				CloseHandle(memory->h_file);
				memory->h_file = INVALID_HANDLE_VALUE;
				CloseHandle(memory->h_map);
				memory->h_map = 0;
#endif
			}
			memory->memory = NULL;
		}
		if( memory->alloc_type == MEMORY_ALLOCATE_TYPE_MALLOC ){
			free( memory );
		}
		else{
#if defined(__LINUX__) || defined(__BSD_UNIX__)
			munmap(memory, sizeof( QS_MEMORY_POOL ) );
#endif
#ifdef __WINDOWS__
			free(memory);
#endif
		}
		memory = NULL;
	}
	return memory_size;
}

size_t qs_memory_available_size( QS_MEMORY_POOL* memory )
{
	size_t freeSize = ( ( memory->bottom - memory->top ) );
	return freeSize;
}

size_t qs_memory_use_size( QS_MEMORY_POOL* memory )
{
	size_t use_size = ( memory->size - ( ( memory->bottom - memory->top ) ) );
	return use_size;
}

void qs_set_buffer( QS_MEMORY_POOL* memory, int32_t id, QS_BYTE_BUFFER* pbuffer )
{
	if( id < memory->fix_memory_block_size || id >= memory->block_size ){
		if(memory->debug){
			printf(  "[qs_upointer]unit id out of range[%d]\n", id );
		}
		return;
	}
	QS_MEMORY_UNIT* unit = (QS_MEMORY_UNIT*)( ( (uint8_t*)memory->memory + memory->end ) - ( memory->memory_block_size_one * ( id + 1 ) ) );
	pbuffer->endian = memory->endian;
	pbuffer->buffer = ((uint8_t*)memory->memory + unit->p);
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

int32_t qs_create_memory_info( QS_MEMORY_POOL* memory, QS_BYTE_BUFFER* pbuffer )
{
	MEMORY_PUSH_BIT32_L( memory, pbuffer->pos, memory->end );
	MEMORY_PUSH_BIT32_L( memory, pbuffer->pos, memory->top );
	MEMORY_PUSH_BIT32_L( memory, pbuffer->pos, memory->bottom );
	MEMORY_PUSH_BIT32_L( memory, pbuffer->pos, memory->size );
	MEMORY_PUSH_BIT32_L( memory, pbuffer->pos, memory->max_size );
	MEMORY_PUSH_BIT32_L( memory, pbuffer->pos, memory->alignment );
	MEMORY_PUSH_BIT32_L( memory, pbuffer->pos, memory->memory_block_size_one );
	MEMORY_PUSH_BIT32_L( memory, pbuffer->pos, memory->min_realloc );
	MEMORY_PUSH_BIT32_L( memory, pbuffer->pos, memory->fix_memory_block_size );
	MEMORY_PUSH_BIT32_L( memory, pbuffer->pos, memory->block_size );
	MEMORY_PUSH_BIT32_L( memory, pbuffer->pos, memory->tail_memory_block );
	MEMORY_PUSH_BIT32_L( memory, pbuffer->pos, memory->memid_save_data );
	MEMORY_PUSH_BIT32_L( memory, pbuffer->pos, memory->alloc_type );
	MEMORY_PUSH_BIT32_L( memory, pbuffer->pos, memory->endian );
	
//	pbuffer->pos = pbuffer->buffer;
//	printf("pbuffer size : %dbyte\n",(int)pbuffer->size);
//	printf("memory_pool->end : %d ?? %d\n", memory->end, qs_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->top : %d ?? %d\n", memory->top, qs_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->bottom : %d ?? %d\n", memory->bottom, qs_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->size : %d ?? %d\n", memory->size, qs_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->max_size : %d ?? %d\n", memory->max_size, qs_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->alignment : %d ?? %d\n", memory->alignment, qs_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->memory_block_size_one : %d ?? %d\n", memory->memory_block_size_one, qs_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->min_realloc : %d ?? %d\n", memory->min_realloc, qs_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->fix_memory_block_size : %d ?? %d\n", memory->fix_memory_block_size, qs_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->block_size : %d ?? %d\n", memory->block_size, qs_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->tail_memory_block : %d ?? %d\n", memory->tail_memory_block, qs_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->lock_munit : %d ?? %d\n", memory->lock_munit, qs_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->alloc_type : %d ?? %d\n", memory->alloc_type, qs_pop_little_to_host_bit32( pbuffer ));
//	printf("memory_pool->endian : %d ?? %d\n", memory->endian, qs_pop_little_to_host_bit32( pbuffer ));
//	pbuffer->pos = pbuffer->buffer;
	return QS_SYSTEM_OK;
}

void qs_memory_info( QS_MEMORY_POOL* memory )
{
	size_t freeSize = 0;
	if( memory == NULL ){
		if(memory->debug){
			printf(  "[qs_memory_info]memory is null\n" );
		}
		return;
	}
	printf(  "#############################################################\n");
	printf(  "# memory info\n");
	printf(  "#############################################################\n");
	freeSize = ( ( memory->bottom - memory->top ) );
#ifdef __WINDOWS__
	printf(  "total : %llu Byte\n", memory->size );
	printf(  "use   : %llu Byte\n", memory->size - freeSize );
	printf(  "free  : %llu Byte\n", freeSize );
	printf(  "blocks : %llu\n", memory->block_size );
	printf(  "max size : %llu Byte\n", memory->max_size );
#else
	printf(  "total : %lu Byte\n", memory->size );
	printf(  "use   : %lu Byte\n", memory->size - freeSize );
	printf(  "free  : %lu Byte\n", freeSize );
	printf(  "blocks : %lu\n", memory->block_size );
	printf(  "max size : %lu Byte\n", memory->max_size );
#endif
	printf(  "memory top = %p\n", ( (uint8_t*)memory->memory ) );
	printf(  "memory end = %p\n", ( (uint8_t*)memory->memory + memory->size ) );
	printf(  "memory freetop = %p\n", ( (uint8_t*)memory->memory + memory->top ) );
	printf(  "memory freebottom = %p\n", ( (uint8_t*)memory->memory + memory->bottom ) );
	printf(  "#############################################################\n");
}

void qs_memory_size( QS_MEMORY_POOL* memory )
{
	size_t free_size = ( ( memory->bottom - memory->top ) );
	printf(  "use   : %zd bytes\n", memory->size - free_size );
}

void qs_memory_block_info( QS_MEMORY_POOL* memory )
{
	int i;
	uint32_t free_count = 0;
	uint32_t garbage_count = 0;
	QS_MEMORY_UNIT* unit;
	size_t memory_block_one_size = 0;
	if( memory == NULL ){
		if(memory->debug){
			printf(  "[qs_memory_block_info]memory is null\n" );
		}
		return;
	}
	printf(  "#############################################################\n");
	printf(  "# -- MEMORY_UNIT size %zd Byte --\n", (size_t)( memory->end - memory->bottom ) );
	printf(  "# total memory blocks = %"PRIu64"\n", memory->block_size );
	printf(  "#############################################################\n");
	memory_block_one_size = QS_ALIGNUP( sizeof( QS_MEMORY_UNIT ), memory->alignment );
	for( i = (memory->block_size-1); i >= 0; i-- )
	{
		unit = (QS_MEMORY_UNIT*)( ( (uint8_t*)memory->memory + memory->end ) - ( memory_block_one_size * ( memory->block_size - i ) ) );
#ifdef __WINDOWS__
		(void) fprintf(
				stdout
			, "[%lld] addr:%p:%p, p:%p, pend:%p, memsize:%lld Byte, status:%d, id:%d, parent:%d, child:%d, type:%d\n"
			, memory->block_size - ( i + 1 )
			, unit
			, (uint8_t*)(unit) + memory_block_one_size
			, (uint8_t*)memory->memory + unit->p
			, (uint8_t*)memory->memory + unit->p + unit->size
			, unit->size
			, unit->status
			, unit->id
			, unit->parent
			, unit->child
			, unit->type
		);
#else
		(void) fprintf(
				stdout
			, "[%ld] addr:%p:%p, p:%p, pend:%p, memsize:%ld Byte, status:%d, id:%d, parent:%d, child:%d, type:%d\n"
			, memory->block_size - ( i + 1 )
			, unit
			, (uint8_t*)(unit) + memory_block_one_size
			, (uint8_t*)memory->memory + unit->p
			, (uint8_t*)memory->memory + unit->p + unit->size
			, unit->size
			, unit->status
			, unit->id
			, unit->parent
			, unit->child
			, unit->type
		);
#endif
		if( unit->status == MEMORY_STATUS_FREE ){
			++free_count;
		}
		else if( unit->status == MEMORY_STATUS_GARBAGE ){
			++garbage_count;
		}
	}
	printf(  "free memory blocks = %d\n", free_count );
	printf(  "garbage memory blocks = %d\n", garbage_count );
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
