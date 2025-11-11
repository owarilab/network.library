/*
 * Copyright (c) Katsuya Owari
 */

#ifdef __cplusplus
extern "C"{
#endif

#ifndef _QS_CHAIN_ARRAY_H_
#define _QS_CHAIN_ARRAY_H_

#include "qs_core.h"
#include "qs_memory_allocator.h"

int32_t qs_create_chain_array(QS_MEMORY_POOL* memory,size_t chain_size, size_t data_size);
int32_t qs_resize_chain_array(QS_MEMORY_POOL* memory,int32_t chain_id);
void* qs_add_chain(QS_MEMORY_POOL* memory,int32_t chain_id);
int32_t qs_add_chain_i(QS_MEMORY_POOL* memory,int32_t chain_id);
void* qs_get_chain(QS_MEMORY_POOL* memory,int32_t chain_id,void* current);
void* qs_get_chain_i(QS_MEMORY_POOL* memory,int32_t chain_id,int32_t offset);
int32_t qs_remove_chain(QS_MEMORY_POOL* memory,int32_t chain_id,void* chain);
void qs_dump_chain_array(QS_MEMORY_POOL* memory,int32_t chain_id);
int qs_show_chain_info(QS_MEMORY_POOL* memory,int32_t chain_id,void* chain, int debug);
int32_t qs_get_chain_start(QS_MEMORY_POOL* memory,int32_t chain_id);
int32_t qs_get_chain_length(QS_MEMORY_POOL* memory,int32_t chain_id);
int32_t qs_get_chain_size(QS_MEMORY_POOL* memory,int32_t chain_id);
#endif /*_QS_CHAIN_ARRAY_H_*/

#ifdef __cplusplus
}
#endif
