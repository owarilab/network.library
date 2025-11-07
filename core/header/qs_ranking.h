/*
 * Copyright (c) Katsuya Owari
 */

#ifdef __cplusplus
extern "C"{
#endif

#ifndef _QS_RANKING_H_
#define _QS_RANKING_H_

#include "qs_core.h"
#include "qs_system.h"
#include "qs_memory_allocator.h"
#include "qs_hash.h"
#include "qs_array.h"
#include "qs_json.h"

#define RANKING_USER_HASH_SIZE 4

typedef struct QS_RANKING{
	int32_t self_id;
	int32_t ranking_user_munit;
	int32_t key_size;
	int32_t ranking_index_memory_id;
	int32_t ranking_index_munit;
	int32_t tail_ranking;
	int32_t sort_buffer_munit;
	int32_t low_value;
	int32_t refresh_size;
	int32_t need_entry;
	size_t array_size;
	int32_t get_max;
} QS_RANKING;

typedef struct QS_RANKING_USER{
	char* id;
	uint32_t value;
	uint32_t ranking;
} QS_RANKING_USER;

typedef struct QS_RANKING_SORT{
	int32_t value;
	int32_t ranking;
	int32_t munit;
} QS_RANKING_SORT;

int32_t qs_create_ranking( QS_MEMORY_POOL* _ppool, size_t size, int32_t key_size, int32_t get_max, int32_t refresh_size);
int32_t qs_entry_ranking( QS_MEMORY_POOL* _ppool, int32_t ranking_munit, char* id);
int32_t qs_set_ranking_value( QS_MEMORY_POOL* _ppool, int32_t ranking_munit, char* id, uint32_t value );
int32_t qs_add_ranking_value( QS_MEMORY_POOL* _ppool, int32_t ranking_munit, char* id, uint32_t value );
int32_t qs_ranking_sort_all( QS_MEMORY_POOL* _ppool, int32_t ranking_munit );
int32_t qs_get_ranking( QS_MEMORY_POOL* _ppool, int32_t ranking_munit, int32_t offset, int32_t length, QS_MEMORY_POOL* dest_memory );
void qs_push_integer( QS_MEMORY_POOL* _ppool, int32_t munit,int32_t value );

#endif /*_QS_RANKING_H_*/

#ifdef __cplusplus
}
#endif
