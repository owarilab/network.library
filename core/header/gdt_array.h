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

#ifndef _GDT_ARRAY_H_
#define _GDT_ARRAY_H_

#include "gdt_core.h"
#include "gdt_type.h"
#include "gdt_memory_allocator.h"
#include "gdt_string.h"

#include <sys/stat.h>
#include <sys/types.h>
#if defined(__LINUX__) || defined(__BSD_UNIX__) || defined(__ANDROID__) || defined(__IOS__)
#include <dirent.h>
#include <unistd.h>
#include <utime.h>
#endif

#define GDT_ARRAY_SIZE_DEFAULT 8

typedef struct GDT_ARRAY
{
	int32_t munit;
	size_t len;
	size_t max_size;
	size_t buffer_size;
} GDT_ARRAY;

typedef struct GDT_ARRAY_ELEMENT
{
	int32_t munit;
	int32_t buf_munit;
	int32_t id;
} GDT_ARRAY_ELEMENT;

int32_t gdt_create_array( GDT_MEMORY_POOL* _ppool, size_t allocsize, size_t buffer_size );
int32_t gdt_create_array_buffer( GDT_MEMORY_POOL* _ppool, size_t allocsize, size_t buffer_size );
void gdt_reset_array( GDT_MEMORY_POOL* _ppool, int32_t munit );
int32_t gdt_resize_array( GDT_MEMORY_POOL* _ppool, int32_t munit );
int32_t gdt_next_push_munit( GDT_MEMORY_POOL* _ppool, int32_t munit );
int32_t gdt_array_push( GDT_MEMORY_POOL* _ppool, int32_t* pmunit, int id, int32_t munit );
int32_t gdt_array_push_integer( GDT_MEMORY_POOL* _ppool, int32_t* pmunit, int32_t value );
int32_t gdt_array_push_string( GDT_MEMORY_POOL* _ppool, int32_t* pmunit, const char* value );
int32_t gdt_array_push_empty_string( GDT_MEMORY_POOL* _ppool, int32_t* pmunit, size_t size );
GDT_ARRAY_ELEMENT* gdt_array_pop( GDT_MEMORY_POOL* _ppool, int32_t arraymunit );
GDT_ARRAY_ELEMENT* gdt_array_get( GDT_MEMORY_POOL* _ppool, int32_t arraymunit, int index );
GDT_ARRAY_ELEMENT* gdt_array_foreach( GDT_MEMORY_POOL* _ppool, int32_t array_munit, size_t* psize );
int32_t gdt_array_length( GDT_MEMORY_POOL* _ppool, int32_t arraymunit );
#endif /*_GDT_ARRAY_H_*/

#ifdef __cplusplus
}
#endif
