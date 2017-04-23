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

#ifdef __cplusplus
extern "C"{
#endif

/*
 * memory manager
 * TODO : support pthread, GC
 */
#ifndef _GDT_MEMORY_ALLOCATOR_H_
#define _GDT_MEMORY_ALLOCATOR_H_

#include "gdt_core.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#if defined(__LINUX__) || defined(__BSD_UNIX__) || defined(__ANDROID__) || defined(__IOS__)
#include <sys/mman.h> 
#endif
#include <errno.h>

#define MEMORY_ALLOCATE_TYPE_MALLOC 0
#define MEMORY_ALLOCATE_TYPE_MMAP 1
#define MEMORY_ALLOCATE_TYPE_MINI 2


// byte order
#define GDT_LITTLE_ENDIAN 1
#define GDT_BIG_ENDIAN 2

#define MEMORY_STATUS_FREE 0
#define MEMORY_STATUS_USE  1
#define MEMORY_STATUS_GARBAGE 2
#define GDT_ALIGNUP( size, bytes ) ((((size_t)(size)+(bytes)-1)&(~((bytes)-1))))
#define GDT_POINTER( _ppool, munit_id ) (((uint8_t*)_ppool->memory+(*(uint32_t*)(((uint8_t*)_ppool->memory+_ppool->end)-(_ppool->memory_unit_size_one*(munit_id+1))))))
#define GDT_FIXPOINTER( _ppool, munit_id ) (((uint8_t*)_ppool->memory+(*(uint32_t*)(((uint8_t*)_ppool->memory+_ppool->end)-(_ppool->memory_unit_size_one*(munit_id+1))))))
#define GDT_PUNIT( _ppool, munit_id ) (GDT_MEMORY_UNIT*)(((uint8_t*)_ppool->memory+_ppool->end)-(_ppool->memory_unit_size_one*(munit_id+1)))
#define GDT_PUNIT_USIZE( _ppool, munit_id ) (*(uint32_t*)((((uint8_t*)_ppool->memory+_ppool->end)-(_ppool->memory_unit_size_one*(munit_id+1)))+sizeof(uint32_t)))

#define BYTE_SWAP_BIT16( v ) (v >> 8) | ( (v & 0xff) << 8 )
#define BYTE_SWAP_BIT32( v ) (v >> 24) | ( (v & 0x000000ff) << 24 ) | ( (v & 0x0000ff00) << 8 ) | ( (v & 0x00ff0000) >> 8 )

#define MEMORY_PUSH_BIT16_L( _ppool, mem, v ) \
	if( _ppool->endian==GDT_LITTLE_ENDIAN ){ *((uint16_t*)mem) = v; } \
	else{ *((uint16_t*)mem) = (v >> 8) | ( (v & 0xff) << 8 ); } \
	mem+=2;

#define MEMORY_PUSH_BIT32_L( _ppool, mem, v ) \
	if( _ppool->endian==GDT_LITTLE_ENDIAN ){ *((uint32_t*)mem) = v; } \
	else{ *((uint32_t*)mem) = (v >> 24) | ( (v & 0x000000ff) << 24 ) | ( (v & 0x0000ff00) << 8 ) | ( (v & 0x00ff0000) >> 8 ); } \
	mem+=4;

#define MEMORY_PUSH_CAST_BIT32_L( _ppool, mem, v ) \
	if( _ppool->endian==GDT_LITTLE_ENDIAN ){ *((uint32_t*)mem) = *((uint32_t*)(&v)); } \
	else{ *((uint32_t*)mem) = (*((uint32_t*)(&v)) >> 24) | ( (*((uint32_t*)(&v)) & 0x000000ff) << 24 ) | ( (*((uint32_t*)(&v)) & 0x0000ff00) << 8 ) | ( (*((uint32_t*)(&v)) & 0x00ff0000) >> 8 ); } \
	mem+=4;

#define MEMORY_PUSH_BIT16_B( _ppool, mem, v ) \
	if( _ppool->endian==GDT_BIG_ENDIAN ){ *((uint16_t*)mem) = v; } \
	else{ *((uint16_t*)mem) = (v >> 8) | ( (v & 0xff) << 8 ); } \
	mem+=2;

#define MEMORY_PUSH_BIT32_B( _ppool, mem, v ) \
	if( _ppool->endian==GDT_BIG_ENDIAN ){ *((uint32_t*)mem) = v; } \
	else{ *((uint32_t*)mem) = (v >> 24) | ( (v & 0x000000ff) << 24 ) | ( (v & 0x0000ff00) << 8 ) | ( (v & 0x00ff0000) >> 8 ); } \
	mem+=4;

#define MEMORY_PUSH_CAST_BIT32_B( _ppool, mem, v ) \
	if( _ppool->endian==GDT_BIG_ENDIAN ){ *((uint32_t*)mem) = *((uint32_t*)(&v)); } \
	else{ *((uint32_t*)mem) = (*((uint32_t*)(&v)) >> 24) | ( (*((uint32_t*)(&v)) & 0x000000ff) << 24 ) | ( (*((uint32_t*)(&v)) & 0x0000ff00) << 8 ) | ( (*((uint32_t*)(&v)) & 0x00ff0000) >> 8 ); } \
	mem+=4;

#define FIX_MUNIT_SIZE 4

typedef struct GDT_MEMORY_POOL{
	void *memory;
	uint32_t end;
	uint32_t top;
	uint32_t bottom;
	uint32_t size;
	uint32_t max_size;
	uint32_t alignment;	
	uint32_t memory_unit_size_one;
	uint32_t min_realloc;
	uint32_t fix_unit_size;
	uint32_t unit_size;
	int32_t	tail_munit;
	int32_t lock_munit;
	int alloc_type;
	uint8_t endian;
} GDT_MEMORY_POOL;

typedef struct GDT_MEMORY_UNIT{
	uint32_t p;	
	uint32_t size;
	uint32_t top;
	uint8_t	status;
	uint8_t	type;
	int32_t	id;
	int32_t	parent;
	int32_t	child;
} GDT_MEMORY_UNIT;

typedef struct GDT_MUNIT_QUEUE
{
	uint32_t size;
	int32_t top;
	int32_t tail;
	int32_t lost_count;
} GDT_MUNIT_QUEUE;

typedef struct GDT_BYTE_BUFFER
{
	uint8_t endian;
	uint8_t* buffer;
	uint8_t* pos;
	size_t size;
} GDT_BYTE_BUFFER;

// public
size_t gdt_initialize_memory( GDT_MEMORY_POOL** _ppool, size_t allocate_size, size_t max_allocate_size, size_t alignment_size, size_t fix_memory_unit, size_t free_memory_unit, size_t min_realloc );
size_t gdt_initialize_mmapmemory( GDT_MEMORY_POOL** _ppool, size_t allocate_size, size_t max_allocate_size, size_t alignment_size, size_t fix_memory_unit, size_t free_memory_unit, size_t min_realloc );
int32_t gdt_create_mini_memory( GDT_MEMORY_POOL* _ppool, size_t allocate_size );
int32_t gdt_create_clone_mini_memory( GDT_MEMORY_POOL* _ppool, GDT_MEMORY_POOL* _mini_ppool );
int32_t gdt_copy_mini_memory( GDT_MEMORY_POOL* _dest_ppool, GDT_MEMORY_POOL* _src_ppool );
void gdt_memory_clean( GDT_MEMORY_POOL* _ppool );
size_t gdt_mgetsize( GDT_MEMORY_POOL* _ppool, size_t size );
uint32_t gdt_free_memory_unit( GDT_MEMORY_POOL* _ppool, int32_t *munit_id );
int32_t gdt_create_fixmunit( GDT_MEMORY_POOL* _ppool, int32_t id, size_t size );
GDT_MEMORY_UNIT* gdt_get_fixmunit( GDT_MEMORY_POOL* _ppool, int32_t id );
int32_t gdt_create_munit( GDT_MEMORY_POOL* _ppool, size_t size, uint8_t type );
GDT_MEMORY_UNIT* gdt_get_munit( GDT_MEMORY_POOL* _ppool, int32_t id );
void* gdt_upointer( GDT_MEMORY_POOL* _ppool, int32_t id );
void* gdt_fixupointer( GDT_MEMORY_POOL* _ppool, int32_t id );
void* gdt_offsetpointer( GDT_MEMORY_POOL* _ppool, int32_t id, size_t size, int32_t offset );
size_t gdt_usize( GDT_MEMORY_POOL* _ppool, int32_t id );
size_t gdt_free( GDT_MEMORY_POOL* _ppool );
void gdt_set_buffer( GDT_MEMORY_POOL* _ppool, int32_t id, GDT_BYTE_BUFFER* pbuffer );
uint16_t gdt_pop_little_to_host_bit16( GDT_BYTE_BUFFER* pbuffer );
uint16_t gdt_pop_big_to_host_bit16( GDT_BYTE_BUFFER* pbuffer );
uint32_t gdt_pop_little_to_host_bit32( GDT_BYTE_BUFFER* pbuffer );
uint32_t gdt_pop_big_to_host_bit32( GDT_BYTE_BUFFER* pbuffer );
// private
GDT_MEMORY_UNIT* gdt_find_freemunit( GDT_MEMORY_POOL* _ppool, size_t size );
int gdt_resize_garbage(GDT_MEMORY_POOL* _ppool,GDT_MEMORY_UNIT* garbageunit,GDT_MEMORY_UNIT* freeunit, size_t size);
int gdt_endian();

uint32_t gdt_safe_malloc( GDT_MEMORY_POOL* _ppool, size_t allocate_size, GDT_MEMORY_UNIT** unit );
uint32_t gdt_malloc( GDT_MEMORY_POOL* _ppool, size_t allocate_size, GDT_MEMORY_UNIT** unit );
size_t gdt_realloc_memory( GDT_MEMORY_POOL* _ppool, size_t allocate_size, GDT_MEMORY_UNIT** current_unit );
uint32_t gdt_initialize_memory_unit( GDT_MEMORY_UNIT * unit );

// debug
void gdt_memory_info( GDT_MEMORY_POOL* _ppool );
void gdt_memory_unit_info( GDT_MEMORY_POOL* _ppool );

#endif /*_GDT_MEMORY_ALLOCATOR_H_*/

#ifdef __cplusplus
}
#endif
