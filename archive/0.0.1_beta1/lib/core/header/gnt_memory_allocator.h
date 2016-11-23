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

/*
 * my memory manager
 * TODO : support pthread, GC
 */
#ifndef _GNT_MEMORY_ALLOCATOR_H_
#define _GNT_MEMORY_ALLOCATOR_H_

#include "core.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef LINUX_OS
#include <sys/mman.h> 
#endif
#include <errno.h>

#define MEMORY_STATUS_FREE 0
#define MEMORY_STATUS_USE  1
#define MEMORY_STATUS_GARBAGE 2
#define ALIGNUP( nAddress, nBytes ) ( ( ( (size_t)(nAddress) + (nBytes) -1 ) & ( ~( (nBytes) -1 ) ) ) )

typedef struct MEMORY_POOL GNT_MEMORY_POOL;
typedef struct MEMORY_UNIT GNT_MEMORY_UNIT;

struct MEMORY_POOL{
	void *memory;			// メモリを確保する領域
	size_t end;				// メモリプールの終端位置
	size_t top;				// 上位空きメモリ領域
	size_t bottom;			// 下位空きメモリ領域
	size_t size;			// 確保したメモリサイズ
	size_t maxsize;			// 拡張できる最大メモリサイズ
	size_t alignment;		// アラインメントサイズ
	size_t min_realloc;		// メモリ拡張最少バイト数
	size_t fix_unit_size;	// 固定メモリユニット数
	size_t unit_size;		// メモリユニットの使用数
	int32_t	tailmunit;		// 末尾のメモリを参照しているメモリユニット
	int mtype;				// メモリの種類
};

struct MEMORY_UNIT{
	size_t	p;				// 使用するメモリの開始位置
	size_t	top;			// 空きメモリ開始位置
	size_t	size;			// 使用するメモリサイズ
	uint8_t	status;			// メモリユニットの状態
	uint8_t	type;			// メモリユニットの種類( void*が何か判別用 )
	int32_t	id;				// メモリユニットのid値
	int32_t	parent;			// 親メモリユニットのid値
	int32_t	child;			// 子メモリユニットのid値
};

// public
size_t gnt_initialize_memory( GNT_MEMORY_POOL** _ppool, size_t allocate_size, size_t max_allocate_size, size_t alignment_size, size_t fix_memory_unit, size_t free_memory_unit, size_t min_realloc );
size_t gnt_initialize_mmapmemory( GNT_MEMORY_POOL** _ppool, size_t allocate_size, size_t max_allocate_size, size_t alignment_size, size_t fix_memory_unit, size_t free_memory_unit, size_t min_realloc );
size_t gnt_mgetsize( GNT_MEMORY_POOL* _ppool, size_t size );
uint32_t gnt_free_memory_unit( GNT_MEMORY_POOL* _ppool, GNT_MEMORY_UNIT * unit );
int32_t gnt_createFixMemoryUnit( GNT_MEMORY_POOL* _ppool, int32_t id, size_t size );
GNT_MEMORY_UNIT* gnt_getFixMemoryUnit( GNT_MEMORY_POOL* _ppool, int32_t id );
int32_t gnt_createMemoryUnit( GNT_MEMORY_POOL* _ppool, size_t size, uint8_t type );
int32_t gnt_coppyMemoryUnit( GNT_MEMORY_POOL* _ppool, int32_t id, size_t size );
GNT_MEMORY_UNIT* gnt_get_memoryunit( GNT_MEMORY_POOL* _ppool, int32_t id );
void* gnt_upointer( GNT_MEMORY_POOL* _ppool, int32_t id );
void* gnt_offsetpointer( GNT_MEMORY_POOL* _ppool, int32_t id, size_t size, int32_t offset );
size_t gnt_usize( GNT_MEMORY_POOL* _ppool, int32_t id );
size_t gnt_free( GNT_MEMORY_POOL* _ppool );

// debug
void gnt_memory_info( GNT_MEMORY_POOL* _ppool );
void gnt_memory_unit_info( GNT_MEMORY_POOL* _ppool );
uint32_t gnt_get_memory_unit_size( GNT_MEMORY_POOL* _ppool );

// private
GNT_MEMORY_UNIT* gnt_findFreeMemoryUnit( GNT_MEMORY_POOL* _ppool, size_t size );
uint32_t gnt_safe_malloc( GNT_MEMORY_POOL* _ppool, size_t allocate_size, GNT_MEMORY_UNIT** unit );
uint32_t gnt_malloc( GNT_MEMORY_POOL* _ppool, size_t allocate_size, GNT_MEMORY_UNIT** unit );
size_t gnt_realloc_memory( GNT_MEMORY_POOL* _ppool, size_t allocate_size, GNT_MEMORY_UNIT** currentUnit );
uint32_t gnt_initialize_memory_unit( GNT_MEMORY_POOL* _ppool, GNT_MEMORY_UNIT * unit );



#endif /*_GNT_MEMORY_ALLOCATOR_H_*/
