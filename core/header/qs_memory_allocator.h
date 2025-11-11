/*
 * Copyright (c) Katsuya Owari
 */

#ifdef __cplusplus
extern "C"{
#endif

/*
 * memory manager
 * TODO : support pthread, GC
 */
#ifndef _QS_MEMORY_ALLOCATOR_H_
#define _QS_MEMORY_ALLOCATOR_H_

#include "qs_core.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#if defined(__LINUX__) || defined(__BSD_UNIX__) || defined(__ANDROID__) || defined(__IOS__)
#include <sys/mman.h> 
#endif
#include <errno.h>

#ifdef __WINDOWS__
#include <io.h>
#endif

#define MEMORY_DEBUG 0

#define MEMORY_ALIGNMENT_SIZE_BIT_64 ( SIZE_BYTE * 8 )
#define MEMORY_ALIGNMENT_SIZE_BIT_32 ( SIZE_BYTE * 4 )
// memory types of qs_memory_allocator
#define MEMORY_TYPE_DEFAULT 0

#define MEMORY_ALLOCATE_TYPE_MALLOC 0
#define MEMORY_ALLOCATE_TYPE_MMAP 1
#define MEMORY_ALLOCATE_TYPE_MINI 2

// byte order
#define QS_LITTLE_ENDIAN 1
#define QS_BIG_ENDIAN 2

#define MEMORY_STATUS_FREE 0
#define MEMORY_STATUS_USE  1
#define MEMORY_STATUS_GARBAGE 2
#define QS_ALIGNUP( size, bytes ) ((((size_t)(size)+(bytes)-1)&(~((bytes)-1))))
#define QS_GET_POINTER( _ppool, munit_id ) (((uint8_t*)_ppool->memory+(*(uint64_t*)(((uint8_t*)_ppool->memory+_ppool->end)-(_ppool->memory_unit_size_one*(munit_id+1))))))
#define QS_FIXPOINTER( _ppool, munit_id ) (((uint8_t*)_ppool->memory+(*(uint64_t*)(((uint8_t*)_ppool->memory+_ppool->end)-(_ppool->memory_unit_size_one*(munit_id+1))))))
#define QS_PUNIT( _ppool, munit_id ) (QS_MEMORY_UNIT*)(((uint8_t*)_ppool->memory+_ppool->end)-(_ppool->memory_unit_size_one*(munit_id+1)))
#define QS_PUNIT_USIZE( _ppool, munit_id ) (*(uint64_t*)((((uint8_t*)_ppool->memory+_ppool->end)-(_ppool->memory_unit_size_one*(munit_id+1)))+sizeof(uint64_t)))
#define QS_PINT32( _ppool, munit_id ) (int32_t*)(QS_GET_POINTER(_ppool,munit_id)+QS_PUNIT_USIZE(_ppool,munit_id)-sizeof(int32_t))
#define QS_INT32( _ppool, munit_id ) (*(int32_t*)(QS_GET_POINTER(_ppool,munit_id)+QS_PUNIT_USIZE(_ppool,munit_id)-sizeof(int32_t)))

#define BYTE_SWAP_BIT16( v ) (v >> 8) | ( (v & 0xff) << 8 )
#define BYTE_SWAP_BIT32( v ) (v >> 24) | ( (v & 0x000000ff) << 24 ) | ( (v & 0x0000ff00) << 8 ) | ( (v & 0x00ff0000) >> 8 )
#define BYTE_SWAP_BIT64( v ) (v & 0xff00000000000000) >> 56 | (v & 0x00ff000000000000) >> 40 | (v & 0x0000ff0000000000) >> 24 | (v & 0x000000ff00000000) >> 8 | (v & 0x00000000ff000000) << 8 | (v & 0x0000000000ff0000) << 24 | (v & 0x000000000000ff00) << 40 | (v & 0x00000000000000ff) << 56

#define MEMORY_PUSH_BIT16_L( _ppool, mem, v ) \
	if( _ppool->endian==QS_LITTLE_ENDIAN ){ *((uint16_t*)mem) = v; } \
	else{ *((uint16_t*)mem) = (v >> 8) | ( (v & 0xff) << 8 ); } \
	mem+=2;

#define MEMORY_PUSH_BIT32_L( _ppool, mem, v ) \
	if( _ppool->endian==QS_LITTLE_ENDIAN ){ *((uint32_t*)mem) = v; } \
	else{ *((uint32_t*)mem) = (v >> 24) | ( (v & 0x000000ff) << 24 ) | ( (v & 0x0000ff00) << 8 ) | ( (v & 0x00ff0000) >> 8 ); } \
	mem+=4;

#define MEMORY_PUSH_CAST_BIT32_L( _ppool, mem, v ) \
	if( _ppool->endian==QS_LITTLE_ENDIAN ){ *((uint32_t*)mem) = *((uint32_t*)(&v)); } \
	else{ *((uint32_t*)mem) = (*((uint32_t*)(&v)) >> 24) | ( (*((uint32_t*)(&v)) & 0x000000ff) << 24 ) | ( (*((uint32_t*)(&v)) & 0x0000ff00) << 8 ) | ( (*((uint32_t*)(&v)) & 0x00ff0000) >> 8 ); } \
	mem+=4;

#define MEMORY_PUSH_BIT16_B( _ppool, mem, v ) \
	if( _ppool->endian==QS_BIG_ENDIAN ){ *((uint16_t*)mem) = v; } \
	else{ *((uint16_t*)mem) = (v >> 8) | ( (v & 0xff) << 8 ); } \
	mem+=2;
#define MEMORY_PUSH_BIT16_B2( endian, mem, v ) \
	if( endian==QS_BIG_ENDIAN ){ *((uint16_t*)mem) = v; } \
	else{ *((uint16_t*)mem) = (v >> 8) | ( (v & 0xff) << 8 ); } \
	mem+=2;

#define MEMORY_PUSH_BIT32_B( _ppool, mem, v ) \
	if( _ppool->endian==QS_BIG_ENDIAN ){ *((uint32_t*)mem) = v; } \
	else{ *((uint32_t*)mem) = (v >> 24) | ( (v & 0x000000ff) << 24 ) | ( (v & 0x0000ff00) << 8 ) | ( (v & 0x00ff0000) >> 8 ); } \
	mem+=4;

#define MEMORY_PUSH_BIT32_B2( endian, mem, v ) \
	if( endian==QS_BIG_ENDIAN ){ *((uint32_t*)mem) = v; } \
	else{ *((uint32_t*)mem) = (v >> 24) | ( (v & 0x000000ff) << 24 ) | ( (v & 0x0000ff00) << 8 ) | ( (v & 0x00ff0000) >> 8 ); } \
	mem+=4;

#define MEMORY_PUSH_CAST_BIT32_B( _ppool, mem, v ) \
	if( _ppool->endian==QS_BIG_ENDIAN ){ *((uint32_t*)mem) = *(v); } \
	else{ *((uint32_t*)mem) = (*(v) >> 24) | ( (*(v) & 0x000000ff) << 24 ) | ( (*(v) & 0x0000ff00) << 8 ) | ( (*(v) & 0x00ff0000) >> 8 ); } \
	mem+=4;

#define MEMORY_PUSH_BIT64_B( _ppool, mem, v ) \
	if( _ppool->endian==QS_BIG_ENDIAN ){ *((uint32_t*)mem) = v; } \
	else{ *((uint64_t*)mem) = (v & 0xff00000000000000) >> 56 | (v & 0x00ff000000000000) >> 40 | (v & 0x0000ff0000000000) >> 24 | (v & 0x000000ff00000000) >> 8 | (v & 0x00000000ff000000) << 8 | (v & 0x0000000000ff0000) << 24 | (v & 0x000000000000ff00) << 40 | (v & 0x00000000000000ff) << 56; } \
	mem+=8;

#define MEMORY_PUSH_BIT64_B2( endian, mem, v ) \
	if( endian==QS_BIG_ENDIAN ){ *((uint64_t*)mem) = v; } \
	else{ *((uint64_t*)mem) = (v & 0xff00000000000000) >> 56 | (v & 0x00ff000000000000) >> 40 | (v & 0x0000ff0000000000) >> 24 | (v & 0x000000ff00000000) >> 8 | (v & 0x00000000ff000000) << 8 | (v & 0x0000000000ff0000) << 24 | (v & 0x000000000000ff00) << 40 | (v & 0x00000000000000ff) << 56; } \
	mem+=8;

#define FIX_MUNIT_SIZE 4

# define QS_MEMORY_ERROR_CODE_RESIZE -1000

typedef struct QS_MEMORY_POOL{
	void *memory;
	uint64_t end;
	uint64_t top;
	uint64_t bottom;
	uint64_t size;
	uint64_t max_size;
	uint64_t alignment;	
	uint64_t memory_unit_size_one;
	uint64_t min_realloc;
	uint64_t fix_unit_size;
	uint64_t unit_size;
	int32_t	tail_munit;
	int32_t lock_munit;
	int32_t memory_buf_munit;
	int alloc_type;
	uint8_t endian;
	uint8_t debug;
	int32_t error_code;
	int mmap_fd;
#ifdef __WINDOWS__
	HANDLE h_file;
	HANDLE h_map;
#endif
} QS_MEMORY_POOL;

typedef struct QS_MEMORY_UNIT{
	uint64_t p;	
	uint64_t size;
	uint64_t top;
	uint8_t	status;
	uint8_t	type;
	int32_t	id;
	int32_t	parent;
	int32_t	child;
} QS_MEMORY_UNIT;

typedef struct QS_BYTE_BUFFER
{
	uint8_t endian;
	uint8_t* buffer;
	uint8_t* pos;
	size_t size;
} QS_BYTE_BUFFER;

// public
size_t qs_initialize_memory( QS_MEMORY_POOL** _ppool, size_t allocate_size, size_t max_allocate_size, size_t alignment_size, size_t fix_memory_unit, size_t free_memory_unit, size_t min_realloc );
size_t qs_initialize_memory_f64( QS_MEMORY_POOL** _ppool, size_t allocate_size );
size_t qs_initialize_mmapmemory( QS_MEMORY_POOL** _ppool, size_t allocate_size, size_t max_allocate_size, size_t alignment_size, size_t fix_memory_unit, size_t free_memory_unit, size_t min_realloc );
size_t qs_initialize_mmapmemory_f64( QS_MEMORY_POOL** _ppool, size_t allocate_size );
size_t qs_initialize_mmapmemory_f( const char* file_name, QS_MEMORY_POOL** _ppool, size_t allocate_size );
int32_t qs_sync_mmap_memory(QS_MEMORY_POOL* memory_pool);
int32_t qs_async_mmap_memory(QS_MEMORY_POOL* memory_pool);
int32_t qs_create_mini_memory( QS_MEMORY_POOL* _ppool, size_t allocate_size );
int32_t qs_create_clone_mini_memory( QS_MEMORY_POOL* _ppool, QS_MEMORY_POOL* _mini_ppool );
int32_t qs_copy_mini_memory( QS_MEMORY_POOL* _dest_ppool, QS_MEMORY_POOL* _src_ppool );
int32_t qs_resize_copy_mini_memory(QS_MEMORY_POOL* _dest_ppool, QS_MEMORY_POOL* _src_ppool);
void qs_memory_clean( QS_MEMORY_POOL* _ppool );
size_t qs_mgetsize( QS_MEMORY_POOL* _ppool, size_t size );
uint32_t qs_free_memory_unit( QS_MEMORY_POOL* _ppool, int32_t *munit_id );
int32_t qs_create_fixmunit( QS_MEMORY_POOL* _ppool, int32_t id, size_t size );
QS_MEMORY_UNIT* qs_get_fixmunit( QS_MEMORY_POOL* _ppool, int32_t id );
int32_t qs_create_munit( QS_MEMORY_POOL* _ppool, size_t size, uint8_t type );
int32_t qs_create_memory_block( QS_MEMORY_POOL* _ppool, size_t size );
QS_MEMORY_UNIT* qs_get_munit( QS_MEMORY_POOL* _ppool, int32_t id );
void* qs_upointer( QS_MEMORY_POOL* _ppool, int32_t id );
void* qs_fixupointer( QS_MEMORY_POOL* _ppool, int32_t id );
void* qs_offsetpointer( QS_MEMORY_POOL* _ppool, int32_t id, size_t size, int32_t offset );
size_t qs_usize( QS_MEMORY_POOL* _ppool, int32_t id );
size_t qs_free( QS_MEMORY_POOL* _ppool );
size_t qs_memory_available_size( QS_MEMORY_POOL* _ppool );
void qs_set_buffer( QS_MEMORY_POOL* _ppool, int32_t id, QS_BYTE_BUFFER* pbuffer );
uint16_t qs_pop_little_to_host_bit16( QS_BYTE_BUFFER* pbuffer );
uint16_t qs_pop_big_to_host_bit16( QS_BYTE_BUFFER* pbuffer );
uint32_t qs_pop_little_to_host_bit32( QS_BYTE_BUFFER* pbuffer );
uint32_t qs_pop_big_to_host_bit32( QS_BYTE_BUFFER* pbuffer );
uint64_t qs_pop_little_to_host_bit64( QS_BYTE_BUFFER* pbuffer );
uint64_t qs_pop_big_to_host_bit64( QS_BYTE_BUFFER* pbuffer );
int32_t qs_create_memory_info( QS_MEMORY_POOL* _ppool, QS_BYTE_BUFFER* pbuffer );
// private
QS_MEMORY_UNIT* qs_find_freemunit( QS_MEMORY_POOL* _ppool, size_t size );
int qs_resize_garbage(QS_MEMORY_POOL* _ppool,QS_MEMORY_UNIT* garbageunit,QS_MEMORY_UNIT* freeunit, size_t size);
int qs_endian();

uint32_t qs_safe_malloc( QS_MEMORY_POOL* _ppool, size_t allocate_size, QS_MEMORY_UNIT** unit );
uint32_t qs_malloc( QS_MEMORY_POOL* _ppool, size_t allocate_size, QS_MEMORY_UNIT** unit );
size_t qs_realloc_memory( QS_MEMORY_POOL* _ppool, size_t allocate_size, QS_MEMORY_UNIT** current_unit );
uint32_t qs_initialize_memory_unit( QS_MEMORY_UNIT * unit );

// debug
void qs_memory_info( QS_MEMORY_POOL* _ppool );
void qs_memory_size( QS_MEMORY_POOL* _ppool );
void qs_memory_unit_info( QS_MEMORY_POOL* _ppool );

#endif /*_QS_MEMORY_ALLOCATOR_H_*/

#ifdef __cplusplus
}
#endif
