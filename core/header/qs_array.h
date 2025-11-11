/*
 * Copyright (c) Katsuya Owari
 */

#ifdef __cplusplus
extern "C"{
#endif

#ifndef _QS_ARRAY_H_
#define _QS_ARRAY_H_

#include "qs_core.h"
#include "qs_type.h"
#include "qs_memory_allocator.h"
#include "qs_string.h"

#include <sys/stat.h>
#include <sys/types.h>
#if defined(__LINUX__) || defined(__BSD_UNIX__) || defined(__ANDROID__) || defined(__IOS__)
#include <dirent.h>
#include <unistd.h>
#include <utime.h>
#endif

#define QS_ARRAY_SIZE_DEFAULT 8

typedef struct QS_ARRAY
{
	int32_t munit;
	size_t len;
	size_t max_size;
	size_t buffer_size;
} QS_ARRAY;

typedef struct QS_ARRAY_ELEMENT
{
	int32_t munit;
	int32_t buf_munit;
	int32_t id;
} QS_ARRAY_ELEMENT;

int32_t qs_create_array( QS_MEMORY_POOL* _ppool, size_t allocsize, size_t buffer_size );
int32_t qs_create_array_buffer( QS_MEMORY_POOL* _ppool, size_t allocsize, size_t buffer_size );
int32_t qs_reset_array( QS_MEMORY_POOL* _ppool, int32_t munit );
int32_t qs_resize_array( QS_MEMORY_POOL* _ppool, int32_t munit );
int32_t qs_next_push_munit( QS_MEMORY_POOL* _ppool, int32_t munit );
int32_t qs_array_push( QS_MEMORY_POOL* _ppool, int32_t* pmunit, int id, int32_t munit );
int32_t qs_array_push_integer( QS_MEMORY_POOL* _ppool, int32_t* pmunit, int32_t value );
int32_t qs_array_push_string( QS_MEMORY_POOL* _ppool, int32_t* pmunit, const char* value );
int32_t qs_array_push_empty_string( QS_MEMORY_POOL* _ppool, int32_t* pmunit, size_t size );
QS_ARRAY_ELEMENT* qs_array_pop( QS_MEMORY_POOL* _ppool, int32_t arraymunit );
QS_ARRAY_ELEMENT* qs_array_get( QS_MEMORY_POOL* _ppool, int32_t arraymunit, int index );
QS_ARRAY_ELEMENT* qs_array_foreach( QS_MEMORY_POOL* _ppool, int32_t array_munit, size_t* psize );
int32_t qs_array_length( QS_MEMORY_POOL* _ppool, int32_t arraymunit );
#endif /*_QS_ARRAY_H_*/

#ifdef __cplusplus
}
#endif
