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
#endif /*_QS_CHAIN_ARRAY_H_*/

#ifdef __cplusplus
}
#endif
