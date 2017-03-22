/*
 * Copyright (c) 2014-2016 Katsuya Owari
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

#define MEMORY_STATUS_FREE 0
#define MEMORY_STATUS_USE  1
#define MEMORY_STATUS_GARBAGE 2
#define GDT_ALIGNUP( size, bytes ) ((((size_t)(size)+(bytes)-1)&(~((bytes)-1))))
#define GDT_POINTER( _ppool, munit_id ) (((uint8_t*)_ppool->memory+(*(uint32_t*)(((uint8_t*)_ppool->memory+_ppool->end)-(_ppool->munitmemsize*(munit_id+1))))))
#define GDT_FIXPOINTER( _ppool, munit_id ) (((uint8_t*)_ppool->memory+(*(uint32_t*)(((uint8_t*)_ppool->memory+_ppool->end)-(_ppool->munitmemsize*(munit_id+1))))))
#define GDT_PUNIT( _ppool, munit_id ) (GDT_MEMORY_UNIT*)(((uint8_t*)_ppool->memory+_ppool->end)-(_ppool->munitmemsize*(munit_id+1)))
#define GDT_PUNIT_USIZE( _ppool, munit_id ) (*(uint32_t*)((((uint8_t*)_ppool->memory+_ppool->end)-(_ppool->munitmemsize*(munit_id+1)))+sizeof(uint32_t)))

#define FIX_MUNIT_SIZE 4
#define FIX_MUNIT_FREE_MUNIT_QUEUE 0
#define FIX_MUNIT_FREE_MUNIT_QUEUE_ARRAY 1
#define FIX_MUNIT_GARBAGE_MUQ 2
#define FIX_MUNIT_GARBAGE_MUQ_ARRAY 3
#define FIX_MUNIT_MUQ_SIZE 1024

typedef struct GDT_MEMORY_POOL{
	void *memory;
	uint32_t end;
	uint32_t top;
	uint32_t bottom;
	uint32_t size;
	uint32_t maxsize;
	uint32_t alignment;	
	uint32_t munitmemsize;
	uint32_t min_realloc;
	uint32_t fix_unit_size;
	uint32_t unit_size;
	int32_t	tailmunit;
	int32_t lockmunit;
	int mtype;
} GDT_MEMORY_POOL;

typedef struct GDT_MEMORY_BLOCK
{
	int32_t munit;
	uint32_t end;
	uint32_t top;
	uint32_t bottom;
	uint32_t size;
	uint32_t maxsize;
	uint32_t alignment;	
	uint32_t munitmemsize;
	uint32_t min_realloc;
	uint32_t fix_unit_size;
	uint32_t unit_size;
	int32_t	tailmunit;
	int32_t lockmunit;
	int8_t mtype;
} GDT_MEMORY_BLOCK;

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

// public
size_t gdt_initialize_memory( GDT_MEMORY_POOL** _ppool, size_t allocate_size, size_t max_allocate_size, size_t alignment_size, size_t fix_memory_unit, size_t free_memory_unit, size_t min_realloc );
size_t gdt_initialize_mmapmemory( GDT_MEMORY_POOL** _ppool, size_t allocate_size, size_t max_allocate_size, size_t alignment_size, size_t fix_memory_unit, size_t free_memory_unit, size_t min_realloc );
int32_t gdt_create_mini_memory( GDT_MEMORY_POOL* _ppool, size_t allocate_size );
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

// debug
void gdt_memory_info( GDT_MEMORY_POOL* _ppool );
void gdt_memory_unit_info( GDT_MEMORY_POOL* _ppool );

// private
GDT_MEMORY_UNIT* gdt_find_freemunit( GDT_MEMORY_POOL* _ppool, size_t size );

uint32_t gdt_safe_malloc( GDT_MEMORY_POOL* _ppool, size_t allocate_size, GDT_MEMORY_UNIT** unit );
uint32_t gdt_malloc( GDT_MEMORY_POOL* _ppool, size_t allocate_size, GDT_MEMORY_UNIT** unit );
size_t gdt_realloc_memory( GDT_MEMORY_POOL* _ppool, size_t allocate_size, GDT_MEMORY_UNIT** currentUnit );
uint32_t gdt_initialize_memory_unit( GDT_MEMORY_UNIT * unit );

#endif /*_GDT_MEMORY_ALLOCATOR_H_*/

#ifdef __cplusplus
}
#endif
