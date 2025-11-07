/*
 * Copyright (c) Katsuya Owari
 */

#include "qs_packet_route.h"

int32_t qs_init_packet_route(QS_MEMORY_POOL* memory,size_t max_connection,size_t max_route_chain,size_t key_size,size_t data_size)
{
	qs_srand_32();
	int32_t packet_route_id = qs_create_memory_block(memory,sizeof(QS_PACKET_ROUTE));
	if(-1==packet_route_id){
		return -1;
	}
	QS_PACKET_ROUTE* packet_route = (QS_PACKET_ROUTE*)QS_GET_POINTER(memory,packet_route_id);
	packet_route->memory = memory;
	packet_route->key_size = key_size;
	packet_route->cache_size = max_route_chain;
	packet_route->data_size = data_size;
	size_t hash_size = max_route_chain;
	size_t cache_size = max_route_chain;
	size_t cache_chain_allocate_size = (( packet_route->key_size + (sizeof(int32_t)*10) ) * max_route_chain) + SIZE_KBYTE * 4;
	size_t cache_hash_allocate_size = ((packet_route->key_size + sizeof(QS_HASH_ELEMENT) + sizeof(QS_MEMORY_UNIT) + NUMERIC_BUFFER_SIZE ) * (hash_size*10));
	packet_route->cache_id = qs_create_cache(memory,cache_chain_allocate_size,cache_size,cache_hash_allocate_size,hash_size,packet_route->key_size);
	if(-1==packet_route->cache_id){
		return -1;
	}
	if(-1==(packet_route->connection_node_offsets_id=qs_create_memory_block(memory,sizeof(QS_PACKET_ROUTE_OFFSET)*max_connection))){
		return -1;
	}
	QS_PACKET_ROUTE_OFFSET* offsets = (QS_PACKET_ROUTE_OFFSET*)QS_GET_POINTER(memory,packet_route->connection_node_offsets_id);
	int i;
	for(i=0;i<max_connection;i++){
		offsets[i].route_chain_offset = -1;
		offsets[i].route_node_chain_offset = -1;
	}
	packet_route->offset_array_size = max_connection;
	if(-1==(packet_route->node_chain_array=qs_create_chain_array(memory,max_route_chain,QS_ALIGNUP( sizeof( QS_PACKET_ROUTE_NODE ), memory->alignment )))){
		return -1;
	}
	for(i=0;i<qs_get_chain_size(memory,packet_route->node_chain_array);i++){
		QS_PACKET_ROUTE_NODE* route_node = (QS_PACKET_ROUTE_NODE*)qs_get_chain_i(memory,packet_route->node_chain_array,i);
		if(data_size>0){
			route_node->data_memory_id = qs_create_mini_memory(memory,data_size + QS_FIX_MEMORY_SIZE);
			if(-1 == route_node->data_memory_id){
				return -1;
			}
		}else{
			route_node->data_memory_id = -1;
		}
	}
	return packet_route_id;
}

int32_t qs_create_packet_route(QS_MEMORY_POOL* memory,int32_t packet_route_id,char* route_name, int32_t route_capacity, int32_t life_time,size_t con_data_size)
{
	QS_PACKET_ROUTE* packet_route = (QS_PACKET_ROUTE*)QS_GET_POINTER(memory,packet_route_id);
	int32_t route_offset = qs_add_chain_i(memory,packet_route->node_chain_array);
	if(route_offset==-1){
		qs_clean_packet_route(memory,packet_route_id);
		route_offset = qs_add_chain_i(memory,packet_route->node_chain_array);
		if(route_offset==-1){
			return -1;
		}
	}
	QS_PACKET_ROUTE_NODE* route_node = (QS_PACKET_ROUTE_NODE*)qs_get_chain_i(memory,packet_route->node_chain_array,route_offset);
	if(route_node==NULL){
		printf("get route_node error\n");
		return -1;
	}
	
	if(route_node->status==0){
		int i;
		if(-1==(route_node->connection_chain_array = qs_create_chain_array(memory,route_capacity,sizeof(QS_PACKET_ROUTE_NODE_CONNECTION)))){
			return -1;
		}
		for(i=0;i<qs_get_chain_size(memory,route_node->connection_chain_array);i++){
			QS_PACKET_ROUTE_NODE_CONNECTION* con = (QS_PACKET_ROUTE_NODE_CONNECTION*)qs_get_chain_i(memory,route_node->connection_chain_array,i);
			con->data_memory_id = -1;
			if(con_data_size>0)
			{
				con->data_memory_id = qs_create_mini_memory(memory,con_data_size + QS_FIX_MEMORY_SIZE);
				if(-1 == con->data_memory_id){
					printf("con memory allocate error\n");
					return -1;
				}
				QS_MEMORY_POOL* con_data_memory = (QS_MEMORY_POOL*)QS_GET_POINTER(memory,con->data_memory_id);
				qs_safe_memory_clean(con_data_memory);
			}
		}
		route_node->route_name_id = qs_create_memory_block(packet_route->memory,packet_route->key_size);
		if(route_node->route_name_id==-1){
			return -1;
		}
		route_node->status=1;
	}else{
		int i;
		for(i=0;i<qs_get_chain_size(memory,route_node->connection_chain_array);i++){
			QS_PACKET_ROUTE_NODE_CONNECTION* con = (QS_PACKET_ROUTE_NODE_CONNECTION*)qs_get_chain_i(memory,route_node->connection_chain_array,i);
			if(-1 != con->data_memory_id){
				QS_MEMORY_POOL* con_data_memory = (QS_MEMORY_POOL*)QS_GET_POINTER(memory,con->data_memory_id);
				qs_safe_memory_clean(con_data_memory);
			}
		}
	}
	
	QS_CACHE* cache = (QS_CACHE*)QS_GET_POINTER(packet_route->memory,packet_route->cache_id);
	qs_cache_int(cache,route_name,route_offset,0);
	char* name = (char*)QS_GET_POINTER(packet_route->memory,route_node->route_name_id);
	qs_strcopy(name,route_name,QS_PUNIT_USIZE(packet_route->memory,route_node->route_name_id));
	route_node->create_time = time(NULL);
	route_node->update_time = route_node->create_time;
	route_node->life_time = life_time;
	route_node->owner_id = 0;
	route_node->auto_id_counter = 0;
	route_node->capacity = route_capacity;
	route_node->con_data_size = con_data_size;
	route_node->application_data_id = -1;
	if(-1!=route_node->data_memory_id)
	{
		QS_MEMORY_POOL* data_memory = (QS_MEMORY_POOL*)QS_GET_POINTER(packet_route->memory,route_node->data_memory_id);
		qs_memory_clean(data_memory);
		if(-1 == qs_create_fix_memory_block(data_memory, 0, packet_route->data_size)){
			return -1;
		}
		//qs_memory_info(data_memory);
		//printf("size : %d\n",(int)qs_usize(data_memory,0));
	}
	//printf("create route(%s) : %p, %d, %s\n", route_name,route_node,route_node->route_name_id,(char*)QS_GET_POINTER(packet_route->memory,route_node->route_name_id));
	return route_offset;
}

int32_t qs_clean_packet_route(QS_MEMORY_POOL* memory,int32_t packet_route_id)
{
	QS_PACKET_ROUTE* packet_route = (QS_PACKET_ROUTE*)QS_GET_POINTER(memory,packet_route_id);
	void* current = NULL;
	while (NULL != (current = qs_get_chain(memory, packet_route->node_chain_array, current))) {
		QS_PACKET_ROUTE_NODE* pnode = (QS_PACKET_ROUTE_NODE*)current;
		char* k = (char*)QS_GET_POINTER(memory, pnode->route_name_id);
		if(strcmp(k,"")){
			if (pnode->update_time<=(time(NULL)-pnode->life_time)) {
				//printf("remove status:%d,name:%s\n", pnode->status, (char*)QS_GET_POINTER(memory, pnode->route_name_id));
				if(-1 == qs_remove_packet_route_core(memory, packet_route_id,pnode)){
					printf("qs_remove_packet_route_core error\n");
				}
				break;
			}
		} else {
			printf("empty route name: %p\n",current);
			qs_show_chain_info(memory, packet_route->node_chain_array, current,0);
			qs_dump_chain_array(memory, packet_route->node_chain_array);
			qs_remove_chain(memory,packet_route->node_chain_array,(void*)current);
			exit(1);
			break;
		}
	}
	return QS_SYSTEM_OK;
}

int32_t qs_get_packet_route(QS_MEMORY_POOL* memory,int32_t packet_route_id,char* route_name)
{
	QS_PACKET_ROUTE* packet_route = (QS_PACKET_ROUTE*)QS_GET_POINTER(memory,packet_route_id);
	QS_CACHE *cache = (QS_CACHE*)QS_GET_POINTER(packet_route->memory,packet_route->cache_id);
	QS_CACHE_PAGE cache_page;
	qs_get_cache_page(cache,&cache_page);
	int32_t hash_id = qs_get_hash(cache_page.memory,cache_page.hash_id,route_name);
	if(hash_id==-1){
		return -1;
	}
	return QS_INT32(cache_page.memory,hash_id);
}

int32_t qs_set_packet_route_owner(QS_MEMORY_POOL* memory,int32_t packet_route_id,char* route_name, int64_t owner_id)
{
	int32_t route_offset = qs_get_packet_route(memory,packet_route_id,route_name);
	if(-1==route_offset){
		return -1;
	}
	QS_PACKET_ROUTE* packet_route = (QS_PACKET_ROUTE*)QS_GET_POINTER(memory,packet_route_id);
	QS_PACKET_ROUTE_NODE* route_node = (QS_PACKET_ROUTE_NODE*)qs_get_chain_i(memory,packet_route->node_chain_array,route_offset);
	route_node->owner_id = owner_id;
	return route_offset;
}

int64_t qs_get_packet_route_owner(QS_MEMORY_POOL* memory,int32_t packet_route_id,char* route_name)
{
	int32_t route_offset = qs_get_packet_route(memory,packet_route_id,route_name);
	if(-1==route_offset){
		return -1;
	}
	QS_PACKET_ROUTE* packet_route = (QS_PACKET_ROUTE*)QS_GET_POINTER(memory,packet_route_id);
	QS_PACKET_ROUTE_NODE* route_node = (QS_PACKET_ROUTE_NODE*)qs_get_chain_i(memory,packet_route->node_chain_array,route_offset);
	return route_node->owner_id;
}

int32_t qs_update_packet_route_time(QS_MEMORY_POOL* memory,int32_t packet_route_id,char* route_name)
{
	int32_t route_offset = qs_get_packet_route(memory,packet_route_id,route_name);
	if(-1==route_offset){
		return -1;
	}
	QS_PACKET_ROUTE* packet_route = (QS_PACKET_ROUTE*)QS_GET_POINTER(memory,packet_route_id);
	QS_PACKET_ROUTE_NODE* route_node = (QS_PACKET_ROUTE_NODE*)qs_get_chain_i(memory,packet_route->node_chain_array,route_offset);
	route_node->update_time = time(NULL);
	return route_offset;
}

int32_t qs_add_packet_route_connection(QS_MEMORY_POOL* memory,int32_t packet_route_id,int32_t route_offset,int32_t connection_index)
{
	QS_PACKET_ROUTE* packet_route = (QS_PACKET_ROUTE*)QS_GET_POINTER(memory,packet_route_id);
	QS_PACKET_ROUTE_NODE* route_node = (QS_PACKET_ROUTE_NODE*)qs_get_chain_i(memory,packet_route->node_chain_array,route_offset);
	int32_t connection_chain = qs_add_chain_i(memory,route_node->connection_chain_array);
	if(-1==connection_chain){
		return -1;
	}
	QS_PACKET_ROUTE_NODE_CONNECTION* con = (QS_PACKET_ROUTE_NODE_CONNECTION*)qs_get_chain_i(memory,route_node->connection_chain_array,connection_chain);
	if(-1 != con->data_memory_id){
		QS_MEMORY_POOL* con_data_memory = (QS_MEMORY_POOL*)QS_GET_POINTER(memory,con->data_memory_id);
		qs_safe_memory_clean(con_data_memory);
	}
	con->connection_index = connection_index;
	con->id = ++route_node->auto_id_counter;
	QS_PACKET_ROUTE_OFFSET* offsets = (QS_PACKET_ROUTE_OFFSET*)QS_GET_POINTER(memory,packet_route->connection_node_offsets_id);
	offsets[connection_index].route_chain_offset = route_offset;
	offsets[connection_index].route_node_chain_offset = connection_chain;
	return connection_chain;
}

void* qs_get_packet_route_connection_chain(QS_MEMORY_POOL* memory,int32_t packet_route_id,int32_t connection_index)
{
	QS_PACKET_ROUTE* packet_route = (QS_PACKET_ROUTE*)QS_GET_POINTER(memory,packet_route_id);
	QS_PACKET_ROUTE_OFFSET* offsets = (QS_PACKET_ROUTE_OFFSET*)QS_GET_POINTER(memory,packet_route->connection_node_offsets_id);
	int32_t route_offset = offsets[connection_index].route_chain_offset;
	int32_t connection_chain = offsets[connection_index].route_node_chain_offset;
	if(route_offset==-1||connection_chain==-1){
		return NULL;
	}
	QS_PACKET_ROUTE_NODE* route_node = (QS_PACKET_ROUTE_NODE*)qs_get_chain_i(memory,packet_route->node_chain_array,route_offset);
	return qs_get_chain_i(memory,route_node->connection_chain_array,connection_chain);
}

void* qs_foreach_packet_route_connection_chain(QS_MEMORY_POOL* memory, int32_t packet_route_id, int32_t connection_index, void* current)
{
	QS_PACKET_ROUTE* packet_route = (QS_PACKET_ROUTE*)QS_GET_POINTER(memory, packet_route_id);
	QS_PACKET_ROUTE_OFFSET* offsets = (QS_PACKET_ROUTE_OFFSET*)QS_GET_POINTER(memory, packet_route->connection_node_offsets_id);
	int32_t route_offset = offsets[connection_index].route_chain_offset;
	int32_t connection_chain = offsets[connection_index].route_node_chain_offset;
	if (route_offset == -1 || connection_chain == -1) {
		return NULL;
	}
	QS_PACKET_ROUTE_NODE* route_node = (QS_PACKET_ROUTE_NODE*)qs_get_chain_i(memory, packet_route->node_chain_array, route_offset);
	QS_PACKET_ROUTE_NODE_CONNECTION* con = (QS_PACKET_ROUTE_NODE_CONNECTION*)qs_get_chain_i(memory, route_node->connection_chain_array, connection_chain);
	if (con->connection_index != connection_index) {
		printf("invalid index %d, %d\n", con->connection_index, connection_index);
		return NULL;
	}
	return qs_get_chain(memory, route_node->connection_chain_array, current);
}

void* qs_system_foreach_packet_route_connection_chain(QS_MEMORY_POOL* memory, int32_t packet_route_id, int32_t route_offset, void* current)
{
	QS_PACKET_ROUTE* packet_route = (QS_PACKET_ROUTE*)QS_GET_POINTER(memory, packet_route_id);
	QS_PACKET_ROUTE_NODE* route_node = (QS_PACKET_ROUTE_NODE*)qs_get_chain_i(memory, packet_route->node_chain_array, route_offset);
	return qs_get_chain(memory, route_node->connection_chain_array, current);
}

int32_t qs_get_packet_route_connection_offset(QS_MEMORY_POOL* memory,int32_t packet_route_id,int32_t connection_index)
{
	QS_PACKET_ROUTE* packet_route = (QS_PACKET_ROUTE*)QS_GET_POINTER(memory,packet_route_id);
	QS_PACKET_ROUTE_OFFSET* offsets = (QS_PACKET_ROUTE_OFFSET*)QS_GET_POINTER(memory,packet_route->connection_node_offsets_id);
	int32_t route_offset = offsets[connection_index].route_chain_offset;
	return route_offset;
}

char* qs_change_packet_route_connection_id(QS_MEMORY_POOL* memory,int32_t packet_route_id,int32_t connection_index)
{
	QS_PACKET_ROUTE* packet_route = (QS_PACKET_ROUTE*)QS_GET_POINTER(memory,packet_route_id);
	QS_PACKET_ROUTE_OFFSET* offsets = (QS_PACKET_ROUTE_OFFSET*)QS_GET_POINTER(memory,packet_route->connection_node_offsets_id);
	qs_uniqid_r32(offsets[connection_index].id,sizeof(offsets[connection_index].id)-1);
	return offsets[connection_index].id;
}

char* qs_get_packet_route_connection_id(QS_MEMORY_POOL* memory,int32_t packet_route_id,int32_t connection_index)
{
	QS_PACKET_ROUTE* packet_route = (QS_PACKET_ROUTE*)QS_GET_POINTER(memory,packet_route_id);
	QS_PACKET_ROUTE_OFFSET* offsets = (QS_PACKET_ROUTE_OFFSET*)QS_GET_POINTER(memory,packet_route->connection_node_offsets_id);
	char* id = offsets[connection_index].id;
	return id;
}

int32_t qs_find_packet_route_connection_id(QS_MEMORY_POOL* memory,int32_t packet_route_id,char* connection_id)
{
	int32_t connection_index = -1;
	QS_PACKET_ROUTE* packet_route = (QS_PACKET_ROUTE*)QS_GET_POINTER(memory,packet_route_id);
	QS_PACKET_ROUTE_OFFSET* offsets = (QS_PACKET_ROUTE_OFFSET*)QS_GET_POINTER(memory,packet_route->connection_node_offsets_id);
	int i = 0;
	for(i=0;i<packet_route->offset_array_size;i++){
		if(!strcmp(offsets[i].id,connection_id)){
			connection_index = i;
			break;
		}
	}
	return connection_index;
}

QS_PACKET_ROUTE_NODE* qs_get_packet_route_connection_join_node(QS_MEMORY_POOL* memory,int32_t packet_route_id,int32_t connection_index)
{
	QS_PACKET_ROUTE* packet_route = (QS_PACKET_ROUTE*)QS_GET_POINTER(memory,packet_route_id);
	QS_PACKET_ROUTE_OFFSET* offsets = (QS_PACKET_ROUTE_OFFSET*)QS_GET_POINTER(memory,packet_route->connection_node_offsets_id);
	int32_t route_offset = offsets[connection_index].route_chain_offset;
	int32_t connection_chain = offsets[connection_index].route_node_chain_offset;
	if(route_offset==-1||connection_chain==-1){
		return NULL;
	}
	QS_PACKET_ROUTE_NODE* route_node = (QS_PACKET_ROUTE_NODE*)qs_get_chain_i(memory,packet_route->node_chain_array,route_offset);
	return route_node;
}

int32_t qs_change_packet_route_owner(QS_MEMORY_POOL* memory,int32_t packet_route_id,int32_t connection_index)
{
	QS_PACKET_ROUTE* packet_route = (QS_PACKET_ROUTE*)QS_GET_POINTER(memory,packet_route_id);
	QS_PACKET_ROUTE_OFFSET* offsets = (QS_PACKET_ROUTE_OFFSET*)QS_GET_POINTER(memory,packet_route->connection_node_offsets_id);
	int32_t route_offset = offsets[connection_index].route_chain_offset;
	int32_t connection_chain = offsets[connection_index].route_node_chain_offset;
	QS_PACKET_ROUTE_NODE* route_node = (QS_PACKET_ROUTE_NODE*)qs_get_chain_i(memory,packet_route->node_chain_array,route_offset);
	QS_PACKET_ROUTE_NODE_CONNECTION* con = (QS_PACKET_ROUTE_NODE_CONNECTION*)qs_get_chain_i(memory,route_node->connection_chain_array,connection_chain);
	if (con->connection_index != connection_index) {
		printf("invalid index %d, %d\n",con->connection_index,connection_index);
		return -1;
	}

	// change owner
	if(route_node->owner_id==connection_index){
		//printf("change owner : %ld\n",route_node->owner_id);
		route_node->owner_id=0;

		void* current = NULL;
		current=qs_get_chain(memory,route_node->connection_chain_array,current);
		while(NULL!=current){
			QS_PACKET_ROUTE_NODE_CONNECTION* con = (QS_PACKET_ROUTE_NODE_CONNECTION*)current;
			if(-1 != con->connection_index && con->connection_index != connection_index ){
				route_node->owner_id = con->connection_index;
				break;
			}
			current=qs_get_chain(memory,route_node->connection_chain_array,current);
		}
	}

	return 0;
}

int32_t qs_remove_packet_route_connection(QS_MEMORY_POOL* memory,int32_t packet_route_id,int32_t connection_index)
{
	QS_PACKET_ROUTE* packet_route = (QS_PACKET_ROUTE*)QS_GET_POINTER(memory,packet_route_id);
	QS_PACKET_ROUTE_OFFSET* offsets = (QS_PACKET_ROUTE_OFFSET*)QS_GET_POINTER(memory,packet_route->connection_node_offsets_id);
	int32_t route_offset = offsets[connection_index].route_chain_offset;
	int32_t connection_chain = offsets[connection_index].route_node_chain_offset;
	QS_PACKET_ROUTE_NODE* route_node = (QS_PACKET_ROUTE_NODE*)qs_get_chain_i(memory,packet_route->node_chain_array,route_offset);
	QS_PACKET_ROUTE_NODE_CONNECTION* con = (QS_PACKET_ROUTE_NODE_CONNECTION*)qs_get_chain_i(memory,route_node->connection_chain_array,connection_chain);
	if (con->connection_index != connection_index) {
		printf("invalid index %d, %d\n",con->connection_index,connection_index);
		return -1;
	}
	int32_t result = 0;
	qs_remove_chain(memory,route_node->connection_chain_array,(void*)con);
	con->connection_index = -1;
	con->id = ++route_node->auto_id_counter;
	offsets[connection_index].route_chain_offset = -1;
	offsets[connection_index].route_node_chain_offset = -1;
	
	int32_t length = 0;
	void* current = NULL;
	current=qs_get_chain(memory,route_node->connection_chain_array,current);
	while(NULL!=current){
		QS_PACKET_ROUTE_NODE_CONNECTION* con = (QS_PACKET_ROUTE_NODE_CONNECTION*)current;
		if(-1 != con->connection_index ){
			length++;
		}
		current=qs_get_chain(memory,route_node->connection_chain_array,current);
	}
	if(length==0){
		// if linetime < 0 do not delete
		if (route_node->life_time >= 0 && route_node->update_time <= (time(NULL) - route_node->life_time)) {
			if(-1 == qs_remove_packet_route(memory, packet_route_id, route_offset)){
				printf("[qs_remove_packet_route_connection] qs_remove_packet_route error\n");
			}
			result = 1;
		}
		else {
			route_node->owner_id = 0;
			route_node->auto_id_counter = 0;
		}
	}
	return result;
}

int32_t qs_remove_packet_route(QS_MEMORY_POOL* memory,int32_t packet_route_id,int32_t route_offset)
{
	QS_PACKET_ROUTE* packet_route = (QS_PACKET_ROUTE*)QS_GET_POINTER(memory,packet_route_id);
	QS_PACKET_ROUTE_NODE* route_node = (QS_PACKET_ROUTE_NODE*)qs_get_chain_i(memory,packet_route->node_chain_array,route_offset);
	if(route_node==NULL){
		printf("[qs_remove_packet_route] get chain error\n");
		return -1;
	}
	return qs_remove_packet_route_core(memory,packet_route_id,route_node);
}

int32_t qs_remove_packet_route_core(QS_MEMORY_POOL* memory,int32_t packet_route_id,QS_PACKET_ROUTE_NODE* route_node)
{
	QS_PACKET_ROUTE* packet_route = (QS_PACKET_ROUTE*)QS_GET_POINTER(memory,packet_route_id);
	QS_PACKET_ROUTE_OFFSET* offsets = (QS_PACKET_ROUTE_OFFSET*)QS_GET_POINTER(memory,packet_route->connection_node_offsets_id);
	void* current = NULL;
	current=qs_get_chain(memory,route_node->connection_chain_array,current);
	while(NULL!=current){
		QS_PACKET_ROUTE_NODE_CONNECTION* con = (QS_PACKET_ROUTE_NODE_CONNECTION*)current;
		if(-1 != con->connection_index ){
			offsets[con->connection_index].route_chain_offset = -1;
			offsets[con->connection_index].route_node_chain_offset = -1;
		}
		con->connection_index = -1;
		con->id = -1;
		void* tmp = current;
		current=qs_get_chain(memory,route_node->connection_chain_array,current);
		qs_remove_chain(memory,route_node->connection_chain_array,tmp);
	}
	if(route_node->route_name_id==-1){
		printf("empty route_name_id  : %p\n",route_node);
		return -1;
	}
	char* route_name = (char*)QS_GET_POINTER(memory,route_node->route_name_id);
	if(!strcmp(route_name,"")){
		printf("empty route_name : %p , %d, %s\n",route_node,route_node->route_name_id, route_name);
		return -1;
	}
	QS_CACHE* cache = (QS_CACHE*)QS_GET_POINTER(packet_route->memory,packet_route->cache_id);
	//qs_cache_int(cache,route_name,-1);
	qs_remove_cache(cache,route_name);
	memset(route_name,0,QS_PUNIT_USIZE(packet_route->memory,route_node->route_name_id));
	route_node->owner_id = 0;
	route_node->auto_id_counter = 0;
	qs_remove_chain(memory,packet_route->node_chain_array,(void*)route_node);
	return 0;
}

int32_t qs_update_packet_route(QS_MEMORY_POOL* memory, int32_t packet_route_id)
{
	time_t t = time(NULL);
	void* current = NULL;
	QS_PACKET_ROUTE* packet_route = (QS_PACKET_ROUTE*)QS_GET_POINTER(memory, packet_route_id);
	while(NULL != (current = qs_get_chain(memory, packet_route->node_chain_array, current)))
	{
		QS_PACKET_ROUTE_NODE* pnode = (QS_PACKET_ROUTE_NODE*)current;
		if (pnode->life_time < 0) {
			continue;
		}
		if (pnode->update_time <= ( t - pnode->life_time)) {
			int32_t length = 0;
			void* current_con = NULL;
			while (NULL != (current_con = qs_get_chain(memory, pnode->connection_chain_array, current_con)))
			{
				QS_PACKET_ROUTE_NODE_CONNECTION* con = (QS_PACKET_ROUTE_NODE_CONNECTION*)current_con;
				if (-1 != con->connection_index) {
					length++;
				}
			}

			if (length == 0) {
				if( -1 == qs_remove_packet_route_core(memory, packet_route_id, pnode)){
					printf("[qs_update_packet_route] qs_remove_packet_route_core() error\n");
				}
				return 1;
			}
		}
	}
	return 0;
}

int32_t qs_set_route_data(QS_MEMORY_POOL* memory, int32_t packet_route_id,int32_t route_offset, uint8_t* data, size_t data_size)
{
	QS_PACKET_ROUTE* packet_route = (QS_PACKET_ROUTE*)QS_GET_POINTER(memory, packet_route_id);
	QS_PACKET_ROUTE_NODE* route_node = (QS_PACKET_ROUTE_NODE*)qs_get_chain_i(packet_route->memory, packet_route->node_chain_array, route_offset);
	if(-1 == route_node->data_memory_id){
		return -1;
	}
	QS_MEMORY_POOL* data_memory = (QS_MEMORY_POOL*)QS_GET_POINTER(memory,route_node->data_memory_id);
	if(data_size>=qs_usize(data_memory,0)){
		//qs_memory_info(data_memory);
		//printf("%ld, %ld \n",data_size,qs_usize(data_memory,0));
		return -1;
	}
	uint8_t* dest_data = (uint8_t*)QS_FIXPOINTER(data_memory,0);
	memcpy(dest_data,data,data_size);
	dest_data[data_size] = '\0';
	return 0;
}
uint8_t* qs_get_route_data(QS_MEMORY_POOL* memory, int32_t packet_route_id,int32_t route_offset)
{
	QS_PACKET_ROUTE* packet_route = (QS_PACKET_ROUTE*)QS_GET_POINTER(memory, packet_route_id);
	QS_PACKET_ROUTE_NODE* route_node = (QS_PACKET_ROUTE_NODE*)qs_get_chain_i(packet_route->memory, packet_route->node_chain_array, route_offset);
	if(-1 == route_node->data_memory_id){
		return NULL;
	}
	QS_MEMORY_POOL* data_memory = (QS_MEMORY_POOL*)QS_GET_POINTER(memory,route_node->data_memory_id);
	return (uint8_t*)QS_FIXPOINTER(data_memory,0);
}

int32_t qs_get_route_info(QS_MEMORY_POOL* memory, int32_t packet_route_id, QS_MEMORY_POOL* dest_memory,int32_t route_offset)
{
	QS_PACKET_ROUTE* packet_route = (QS_PACKET_ROUTE*)QS_GET_POINTER(memory, packet_route_id);
	int32_t memid_temp_info_hash = qs_create_hash(dest_memory,8);
	if(-1==memid_temp_info_hash){
		return -1;
	}
	QS_PACKET_ROUTE_NODE* route_node = (QS_PACKET_ROUTE_NODE*)qs_get_chain_i(packet_route->memory, packet_route->node_chain_array, route_offset);
	int32_t connection_length = qs_get_chain_length(packet_route->memory, route_node->connection_chain_array);
	char* id = (char*)QS_GET_POINTER(packet_route->memory,route_node->route_name_id);
	if(-1 != route_node->data_memory_id){
		QS_MEMORY_POOL* data_memory = (QS_MEMORY_POOL*)QS_GET_POINTER(memory,route_node->data_memory_id);
		char* route_data = (char*)QS_FIXPOINTER(data_memory,0);
		qs_add_hash_string(dest_memory,memid_temp_info_hash,"data",route_data);
	}

	char empty_id[8];
	memset(empty_id,0,sizeof(empty_id));
	char* owner_id = empty_id;
	if(route_node->owner_id!=0)
	{
		QS_PACKET_ROUTE_OFFSET* offsets = (QS_PACKET_ROUTE_OFFSET*)QS_GET_POINTER(memory,packet_route->connection_node_offsets_id);
		owner_id = offsets[route_node->owner_id].id;
	}

	qs_add_hash_string(dest_memory,memid_temp_info_hash,"id",id);
	qs_add_hash_integer(dest_memory,memid_temp_info_hash,"max",route_node->capacity);
	qs_add_hash_integer(dest_memory,memid_temp_info_hash,"connection",connection_length);
	qs_add_hash_integer(dest_memory,memid_temp_info_hash,"created",route_node->create_time);
	qs_add_hash_integer(dest_memory,memid_temp_info_hash,"updated",route_node->update_time);
	qs_add_hash_string(dest_memory,memid_temp_info_hash,"owner_id",owner_id);
	//printf("%s : %d , capacity = %d, created = %ld\n",id,route_offset,route_node->capacity,route_node->create_time);
	return memid_temp_info_hash;
}

int32_t qs_get_route_infos(QS_MEMORY_POOL* memory, int32_t packet_route_id, QS_MEMORY_POOL* dest_memory)
{
	QS_PACKET_ROUTE* packet_route = (QS_PACKET_ROUTE*)QS_GET_POINTER(memory, packet_route_id);
	QS_CACHE* cache = (QS_CACHE*)QS_GET_POINTER(packet_route->memory,packet_route->cache_id);
	QS_CACHE_PAGE cache_page;
	qs_get_cache_page(cache,&cache_page);
	int32_t memid_info_hash = qs_create_hash(dest_memory,32);
	if(-1==memid_info_hash){
		return -1;
	}
	int32_t routes_array = qs_create_array(dest_memory,packet_route->cache_size);
	if(-1==routes_array){
		return -1;
	}
	QS_HASH_FOREACH hf;
	QS_HASH_ELEMENT* he;
	qs_init_hash_foreach( cache_page.memory, cache_page.hash_id, &hf );
	while( NULL != ( he = qs_hash_foreach( cache_page.memory, &hf ) ) ){
		//char* id = (char*)QS_GET_POINTER(cache_page.memory,he->memid_hash_name);
		int32_t route_offset = QS_INT32(cache_page.memory,he->memid_hash_element_data);
		int32_t memid_temp_info_hash = qs_get_route_info(memory,packet_route_id,dest_memory,route_offset);
		if(-1==memid_temp_info_hash){
			return -1;
		}
		qs_array_push(dest_memory,&routes_array,ELEMENT_HASH,memid_temp_info_hash);
	}
	qs_add_hash_array(dest_memory,memid_info_hash,"list",routes_array);
	return memid_info_hash;
}
