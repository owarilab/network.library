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

#ifndef _QS_PACKET_ROUTE_H_
#define _QS_PACKET_ROUTE_H_

#include "qs_core.h"
#include "qs_memory_allocator.h"
#include "qs_hash.h"
#include "qs_chain_array.h"
#include "qs_variable.h"
#include "qs_random.h"

#define QS_PACKET_ROUTE_ID_BUFFER_SIZE 33
#define QS_PACKET_ROUTE_ALLOC_SIZE_DEFAULT SIZE_MBYTE * 16
#define QS_PACKET_ROUTE_KEY_SIZE_DEFAULT 20
#define QS_PACKET_ROUTE_ROUTE_SIZE_DEFAULT 1000
#define QS_PACKET_ROUTE_DATA_SIZE_DEFAULT SIZE_KBYTE*2

typedef struct QS_PACKET_ROUTE_NODE
{
	int32_t status;
	int32_t route_name_id;
	int32_t connection_chain_array;
	int64_t owner_id;
	int64_t auto_id_counter;
	time_t create_time;
	time_t update_time;
	int32_t life_time;
	int32_t capacity;
	int32_t application_data_id;
	int32_t data_memory_id;
	size_t con_data_size;
} QS_PACKET_ROUTE_NODE;

typedef struct QS_PACKET_ROUTE_NODE_CONNECTION
{
	int32_t connection_index;
	int64_t id;
	int32_t data_memory_id;
} QS_PACKET_ROUTE_NODE_CONNECTION;

typedef struct QS_PACKET_ROUTE_OFFSET
{
	int32_t route_chain_offset;
	int32_t route_node_chain_offset;
	char id[QS_PACKET_ROUTE_ID_BUFFER_SIZE];
} QS_PACKET_ROUTE_OFFSET;

typedef struct QS_PACKET_ROUTE{
	QS_MEMORY_POOL* memory;
	int32_t cache_id;//node_index_hash_id;
	size_t cache_size;
	int32_t connection_node_offsets_id;
	int32_t offset_array_size;
	int32_t node_chain_array;
	size_t key_size;
	size_t data_size;
} QS_PACKET_ROUTE;

int32_t qs_init_packet_route(QS_MEMORY_POOL* memory,size_t max_connection,size_t max_route_chain,size_t key_size,size_t data_size);
int32_t qs_create_packet_route(QS_MEMORY_POOL* memory,int32_t packet_route_id,char* route_name, int32_t route_capacity, int32_t life_time,size_t con_data_size);
int32_t qs_get_packet_route(QS_MEMORY_POOL* memory,int32_t packet_route_id,char* route_name);
int32_t qs_clean_packet_route(QS_MEMORY_POOL* memory,int32_t packet_route_id);
int32_t qs_set_packet_route_owner(QS_MEMORY_POOL* memory,int32_t packet_route_id,char* route_name, int64_t owner_id);
int64_t qs_get_packet_route_owner(QS_MEMORY_POOL* memory,int32_t packet_route_id,char* route_name);
int32_t qs_update_packet_route_time(QS_MEMORY_POOL* memory,int32_t packet_route_id,char* route_name);
int32_t qs_add_packet_route_connection(QS_MEMORY_POOL* memory,int32_t packet_route_id,int32_t route_offset,int32_t connection_index);
void* qs_get_packet_route_connection_chain(QS_MEMORY_POOL* memory,int32_t packet_route_id,int32_t connection_index);
void* qs_foreach_packet_route_connection_chain(QS_MEMORY_POOL* memory, int32_t packet_route_id, int32_t connection_index, void* current);
void* qs_system_foreach_packet_route_connection_chain(QS_MEMORY_POOL* memory, int32_t packet_route_id, int32_t route_offset, void* current);
int32_t qs_get_packet_route_connection_offset(QS_MEMORY_POOL* memory,int32_t packet_route_id,int32_t connection_index);
char* qs_change_packet_route_connection_id(QS_MEMORY_POOL* memory,int32_t packet_route_id,int32_t connection_index);
char* qs_get_packet_route_connection_id(QS_MEMORY_POOL* memory,int32_t packet_route_id,int32_t connection_index);
int32_t qs_find_packet_route_connection_id(QS_MEMORY_POOL* memory,int32_t packet_route_id,char* connection_id);
QS_PACKET_ROUTE_NODE* qs_get_packet_route_connection_join_node(QS_MEMORY_POOL* memory,int32_t packet_route_id,int32_t connection_index);
int32_t qs_remove_packet_route_connection(QS_MEMORY_POOL* memory,int32_t packet_route_id,int32_t connection_index);
int32_t qs_change_packet_route_owner(QS_MEMORY_POOL* memory,int32_t packet_route_id,int32_t connection_index);
int32_t qs_remove_packet_route(QS_MEMORY_POOL* memory,int32_t packet_route_id,int32_t route_offset);
int32_t qs_remove_packet_route_core(QS_MEMORY_POOL* memory,int32_t packet_route_id,QS_PACKET_ROUTE_NODE* route_node);
int32_t qs_update_packet_route(QS_MEMORY_POOL* memory, int32_t packet_route_id);
int32_t qs_set_route_data(QS_MEMORY_POOL* memory, int32_t packet_route_id,int32_t route_offset, uint8_t* data, size_t data_size);
uint8_t* qs_get_route_data(QS_MEMORY_POOL* memory, int32_t packet_route_id,int32_t route_offset);
int32_t qs_get_route_info(QS_MEMORY_POOL* memory, int32_t packet_route_id, QS_MEMORY_POOL* dest_memory,int32_t route_offset);
int32_t qs_get_route_infos(QS_MEMORY_POOL* memory, int32_t packet_route_id, QS_MEMORY_POOL* dest_memory);
#endif /*_QS_PACKET_ROUTE_H_*/

#ifdef __cplusplus
}
#endif
