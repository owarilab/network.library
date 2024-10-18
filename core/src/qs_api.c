/*
 * Copyright (c) 2014-2024 Katsuya Owari
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

#include "qs_api.h"

//---------------------------------------------------
#include "qs_array.h"
#include "qs_hash.h"
#include "qs_socket.h"
#include "qs_protocol.h"
#include "qs_variable.h"
#include "qs_script.h"
#include "qs_packet_route.h"
#include "qs_random.h"
#include "qs_csv.h"

void api_qs_on_recv( void* params );
void api_qs_exec_http(QS_RECV_INFO *rinfo);
void api_qs_exec_websocket(QS_RECV_INFO *rinfo);
int api_qs_core_on_connect(QS_SERVER_CONNECTION_INFO* connection);
int32_t api_qs_core_on_recv(uint8_t* payload, size_t payload_len, QS_RECV_INFO *qs_recv_info);
int api_qs_core_on_close(QS_SERVER_CONNECTION_INFO* connection);
int api_qs_send_ws_message_common(QS_RECV_INFO *qs_recv_info,const char* message,int is_plane);
//---------------------------------------------------

int api_qs_init()
{
	qs_srand_32();
	qs_srand_64();
	return 0;
}

uint32_t api_qs_rand()
{
	return qs_rand_32();
}

int api_qs_memory_alloc(QS_MEMORY_CONTEXT* context, size_t alloc_size)
{
	context->memory = NULL;
	QS_MEMORY_POOL* memory;
	if( qs_initialize_memory( &memory, alloc_size, alloc_size, MEMORY_ALIGNMENT_SIZE_BIT_64, FIX_MUNIT_SIZE, 1, SIZE_KBYTE * 16 ) <= 0 ){
		return -1;
	}
	context->memory = memory;
	return 0;
}
int api_qs_memory_clean(QS_MEMORY_CONTEXT* context)
{
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)context->memory;
	qs_memory_clean(memory);
	return 0;
}
void api_qs_memory_info(QS_MEMORY_CONTEXT* context)
{
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)context->memory;
	qs_memory_info(memory);
}
size_t api_qs_memory_available_size(QS_MEMORY_CONTEXT* context)
{
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)context->memory;
	return qs_memory_available_size(memory);
}
int api_qs_memory_free(QS_MEMORY_CONTEXT* context)
{
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)context->memory;
	qs_free(memory);
	return 0;
}
int api_qs_array_create(QS_MEMORY_CONTEXT* context, QS_JSON_ELEMENT_ARRAY* array)
{
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)context->memory;
	array->memid_array = qs_create_array(memory,QS_ARRAY_SIZE_DEFAULT);
	if(array->memid_array==-1){
		return -1;
	}
	array->memory = (void*)memory;
	return 0;
}
int api_qs_array_push_integer(QS_JSON_ELEMENT_ARRAY* array,int32_t value)
{
	if(array->memid_array==-1){
		return -1;
	}
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)array->memory;
	return qs_array_push_integer(memory,&array->memid_array,value);
}
int api_qs_array_push_big_integer(QS_JSON_ELEMENT_ARRAY* array,int64_t value)
{
	if(array->memid_array==-1){
		return -1;
	}
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)array->memory;
	return qs_array_push_big_integer(memory,&array->memid_array,value);
}
int api_qs_array_push_unsigned_big_integer(QS_JSON_ELEMENT_ARRAY* array,uint64_t value)
{
	if(array->memid_array==-1){
		return -1;
	}
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)array->memory;
	return qs_array_push_unsigned_big_integer(memory,&array->memid_array,value);
}
int api_qs_array_push_string(QS_JSON_ELEMENT_ARRAY* array,const char* value)
{
	if(array->memid_array==-1){
		return -1;
	}
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)array->memory;
	return qs_array_push_string(memory,&array->memid_array,value);
}
int api_qs_array_push_object(QS_JSON_ELEMENT_ARRAY* array, QS_JSON_ELEMENT_OBJECT* object)
{
	if(array->memid_array==-1){
		return -1;
	}
	if(array->memory!=object->memory){
		return -1;
	}
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)array->memory;
	int32_t memid_push = qs_array_push(memory,&array->memid_array,ELEMENT_HASH,object->memid_object);
	return (memid_push!=-1) ? 0 : -1;
}
int api_qs_array_push_array(QS_JSON_ELEMENT_ARRAY* array, QS_JSON_ELEMENT_ARRAY* push_array)
{
	if(array->memid_array==-1){
		return -1;
	}
	if(array->memory!=push_array->memory){
		return -1;
	}
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)array->memory;
	int32_t memid_push = qs_array_push(memory,&array->memid_array,ELEMENT_ARRAY,push_array->memid_array);
	return (memid_push!=-1) ? 0 : -1;
}
int api_qs_object_create(QS_MEMORY_CONTEXT* context, QS_JSON_ELEMENT_OBJECT* object)
{
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)context->memory;
	object->memid_object = qs_create_hash(memory, 32);
	if (-1 == object->memid_object) {
		return -1;
	}
	object->memory = (void*)memory;
	return 0;
}
int api_qs_object_push_integer(QS_JSON_ELEMENT_OBJECT* object,const char* name,int32_t value)
{
	if(object->memid_object==-1){
		return -1;
	}
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)object->memory;
	QS_HASH_ELEMENT* elm = qs_add_hash_integer(memory,object->memid_object,name,value);
	if(NULL==elm){
		return -1;
	}
	return 0;
}
int api_qs_object_push_big_integer(QS_JSON_ELEMENT_OBJECT* object,const char* name,int64_t value)
{
	if(object->memid_object==-1){
		return -1;
	}
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)object->memory;
	QS_HASH_ELEMENT* elm = qs_add_hash_big_integer(memory,object->memid_object,name,value);
	if(NULL==elm){
		return -1;
	}
	return 0;
}
int api_qs_object_push_unsigned_big_integer(QS_JSON_ELEMENT_OBJECT* object,const char* name,uint64_t value)
{
	if(object->memid_object==-1){
		return -1;
	}
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)object->memory;
	QS_HASH_ELEMENT* elm = qs_add_hash_unsigned_big_integer(memory,object->memid_object,name,value);
	if(NULL==elm){
		return -1;
	}
	return 0;
}
int api_qs_object_push_string(QS_JSON_ELEMENT_OBJECT* object,const char* name,const char* value)
{
	if(object->memid_object==-1){
		return -1;
	}
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)object->memory;
	QS_HASH_ELEMENT* elm = qs_add_hash_string(memory,object->memid_object,name,value);
	if(NULL==elm){
		return -1;
	}
	return 0;
}
int api_qs_object_push_array(QS_JSON_ELEMENT_OBJECT* object,const char* name,QS_JSON_ELEMENT_ARRAY* array)
{
	if(object->memid_object==-1){
		return -1;
	}
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)object->memory;
	qs_add_hash_array(memory,object->memid_object,name,array->memid_array);
	return 0;
}
int api_qs_object_push_object(QS_JSON_ELEMENT_OBJECT* object,const char* name,QS_JSON_ELEMENT_OBJECT* push_object)
{
	if(object->memid_object==-1){
		return -1;
	}
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)object->memory;
	qs_add_hash_hash(memory,object->memid_object,name,push_object->memid_object);
	return 0;
}
char* api_qs_json_encode_object(QS_JSON_ELEMENT_OBJECT* object,size_t buffer_size)
{
	if(object->memid_object==-1){
		return 0;
	}
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)object->memory;
	int32_t memid_json = qs_json_encode_hash(memory, object->memid_object, buffer_size);
	if (-1 == memid_json) {
		return 0;
	}
	char* json = (char*)QS_GET_POINTER(memory, memid_json);
	return json;
}
char* api_qs_json_encode_array(QS_JSON_ELEMENT_ARRAY* array,size_t buffer_size)
{
	if(array->memid_array==-1){
		return 0;
	}
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)array->memory;
	int32_t memid_json = qs_json_encode_array(memory, array->memid_array, buffer_size);
	if (-1 == memid_json) {
		return 0;
	}
	char* json = (char*)QS_GET_POINTER(memory, memid_json);
	return json;
}

int api_qs_json_decode_object(QS_MEMORY_CONTEXT* context, QS_JSON_ELEMENT_OBJECT* object, const char* json)
{
	object->memid_object = -1;
	object->memory = NULL;
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)context->memory;
	QS_NODE* hashroot = qs_get_json_root(memory, qs_json_decode_h(memory, json, 8, 128));
	if (NULL == hashroot || hashroot->id != ELEMENT_HASH) {
		return -1;
	}
	object->memid_object = hashroot->element_munit;
	object->memory = (void*)memory;
	return 0;
}
int api_qs_object_exist(QS_JSON_ELEMENT_OBJECT* object,const char* name)
{
	if(object->memid_object==-1){
		return 0;
	}
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)object->memory;
	return qs_get_hash(memory,object->memid_object,name)!=-1;
}
int32_t* api_qs_object_get_integer(QS_JSON_ELEMENT_OBJECT* object,const char* name)
{
	if(object->memid_object==-1){
		return NULL;
	}
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)object->memory;
	return qs_get_hash_integer(memory,object->memid_object,name);
}
int64_t* api_qs_object_get_big_integer(QS_JSON_ELEMENT_OBJECT* object,const char* name)
{
	if(object->memid_object==-1){
		return NULL;
	}
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)object->memory;
	return qs_get_hash_big_integer(memory,object->memid_object,name);
}
uint64_t* api_qs_object_get_unsigned_big_integer(QS_JSON_ELEMENT_OBJECT* object,const char* name)
{
	if(object->memid_object==-1){
		return NULL;
	}
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)object->memory;
	return qs_get_hash_unsigned_big_integer(memory,object->memid_object,name);
}
int32_t api_qs_object_get_integer_val(QS_JSON_ELEMENT_OBJECT* object,const char* name)
{
	if(object->memid_object==-1){
		return 0;
	}
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)object->memory;
	int32_t* val = qs_get_hash_integer(memory,object->memid_object,name);
	if(NULL==val){
		return 0;
	}
	return *val;
}
int64_t api_qs_object_get_big_integer_val(QS_JSON_ELEMENT_OBJECT* object,const char* name)
{
	if(object->memid_object==-1){
		return 0;
	}
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)object->memory;

	int64_t return_val = 0;
	int32_t id = qs_get_hash_id(memory,object->memid_object,name);
	if(id == ELEMENT_LITERAL_NUM_64){
		int64_t* val = qs_get_hash_big_integer(memory,object->memid_object,name);
		if(NULL==val){
			return 0;
		}
		return_val = *val;
	}
	else if(id == ELEMENT_LITERAL_NUM_U64){
		uint64_t* val = qs_get_hash_unsigned_big_integer(memory,object->memid_object,name);
		if(NULL==val){
			return 0;
		}
		return_val = *val;
	}
	else if(id == ELEMENT_LITERAL_NUM){
		int32_t* val = qs_get_hash_integer(memory,object->memid_object,name);
		if(NULL==val){
			return 0;
		}
		return_val = *val;
	}
	return return_val;
}
uint64_t api_qs_object_get_unsigned_big_integer_val(QS_JSON_ELEMENT_OBJECT* object,const char* name)
{
	if(object->memid_object==-1){
		return 0;
	}
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)object->memory;
	uint64_t return_val = 0;
	int32_t id = qs_get_hash_id(memory,object->memid_object,name);
	if(id == ELEMENT_LITERAL_NUM_64){
		int64_t* val = qs_get_hash_big_integer(memory,object->memid_object,name);
		if(NULL==val){
			return 0;
		}
		return_val = *val;
	}
	else if(id == ELEMENT_LITERAL_NUM_U64){
		uint64_t* val = qs_get_hash_unsigned_big_integer(memory,object->memid_object,name);
		if(NULL==val){
			return 0;
		}
		return_val = *val;
	}
	else if(id == ELEMENT_LITERAL_NUM){
		int32_t* val = qs_get_hash_integer(memory,object->memid_object,name);
		if(NULL==val){
			return 0;
		}
		return_val = *val;
	}
	return return_val;
}
char* api_qs_object_get_string(QS_JSON_ELEMENT_OBJECT* object,const char* name)
{
	if(object->memid_object==-1){
		return NULL;
	}
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)object->memory;
	return qs_get_hash_string(memory,object->memid_object,name);
}
int api_qs_object_get_array(QS_JSON_ELEMENT_OBJECT* object,const char* name,QS_JSON_ELEMENT_ARRAY* dst_array)
{
	dst_array->memid_array = -1;
	dst_array->memory = NULL;
	if(object->memid_object==-1){
		return -1;
	}
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)object->memory;
	int32_t hash_id = qs_get_hash_id( memory, object->memid_object, name );
	if( hash_id != ELEMENT_ARRAY){
		return -1;
	}
	dst_array->memory = (void*)memory;
	dst_array->memid_array = qs_get_hash( memory, object->memid_object, name );
	return 0;
}
int api_qs_object_get_object(QS_JSON_ELEMENT_OBJECT* object,const char* name,QS_JSON_ELEMENT_OBJECT* dst_object)
{
	dst_object->memid_object = -1;
	dst_object->memory = NULL;
	if(object->memid_object==-1){
		return -1;
	}
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)object->memory;
	int32_t hash_id = qs_get_hash_id( memory, object->memid_object, name );
	if( hash_id != ELEMENT_HASH){
		return -1;
	}
	dst_object->memory = (void*)memory;
	dst_object->memid_object = qs_get_hash( memory, object->memid_object, name );
	return 0;
}
int api_qs_object_get_keys(QS_JSON_ELEMENT_OBJECT* object,QS_JSON_ELEMENT_ARRAY* dst_array)
{
	dst_array->memid_array = -1;
	dst_array->memory = NULL;
	if(object->memid_object==-1){
		return -1;
	}
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)object->memory;
	int is_sort_asc = 1;
	int32_t memid_array = qs_get_hash_keys(memory,memory,object->memid_object,is_sort_asc);
	if(-1==memid_array){
		return -1;
	}
	dst_array->memory = (void*)memory;
	dst_array->memid_array = memid_array;
	return 0;
}
int32_t api_qs_array_get_length(QS_JSON_ELEMENT_ARRAY* array)
{
	if(array->memid_array==-1){
		return -1;
	}
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)array->memory;
	return qs_array_length(memory,array->memid_array);
}
int32_t* api_qs_array_get_integer(QS_JSON_ELEMENT_ARRAY* array,int32_t offset)
{
	if(array->memid_array==-1){
		return NULL;
	}
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)array->memory;
	QS_ARRAY_ELEMENT* elm = qs_array_get(memory,array->memid_array,offset);
	if(elm==NULL){
		return NULL;
	}
	if(elm->id != ELEMENT_LITERAL_NUM){
		return NULL;
	}
	return QS_PINT32(memory,elm->memid_array_element_data);
}
int64_t* api_qs_array_get_big_integer(QS_JSON_ELEMENT_ARRAY* object,int32_t offset)
{
	if(object->memid_array==-1){
		return NULL;
	}
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)object->memory;
	QS_ARRAY_ELEMENT* elm = qs_array_get(memory,object->memid_array,offset);
	if(elm==NULL){
		return NULL;
	}
	if(elm->id != ELEMENT_LITERAL_NUM_64){
		return NULL;
	}
	return QS_PINT64(memory,elm->memid_array_element_data);
}
uint64_t* api_qs_array_get_unsigned_big_integer(QS_JSON_ELEMENT_ARRAY* object,int32_t offset)
{
	if(object->memid_array==-1){
		return NULL;
	}
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)object->memory;
	QS_ARRAY_ELEMENT* elm = qs_array_get(memory,object->memid_array,offset);
	if(elm==NULL){
		return NULL;
	}
	if(elm->id != ELEMENT_LITERAL_NUM_U64){
		return NULL;
	}
	return QS_PUINT64(memory,elm->memid_array_element_data);
}
char* api_qs_array_get_string(QS_JSON_ELEMENT_ARRAY* array,int32_t offset)
{
	if(array->memid_array==-1){
		return NULL;
	}
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)array->memory;
	QS_ARRAY_ELEMENT* elm = qs_array_get(memory,array->memid_array,offset);
	if(elm==NULL){
		return NULL;
	}
	if(elm->id != ELEMENT_LITERAL_STR){
		return NULL;
	}
	return (char*)QS_GET_POINTER(memory,elm->memid_array_element_data);
}
int api_qs_array_get_array(QS_JSON_ELEMENT_ARRAY* array,int32_t offset,QS_JSON_ELEMENT_ARRAY* dst_array)
{
	dst_array->memid_array = -1;
	dst_array->memory = NULL;
	if(array->memid_array==-1){
		return -1;
	}
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)array->memory;
	QS_ARRAY_ELEMENT* elm = qs_array_get(memory,array->memid_array,offset);
	if(elm==NULL){
		return -1;
	}
	if(elm->id != ELEMENT_ARRAY){
		return -1;
	}
	dst_array->memory = (void*)memory;
	dst_array->memid_array = elm->memid_array_element_data;
	return 0;
}
int api_qs_array_get_object(QS_JSON_ELEMENT_ARRAY* array,int32_t offset,QS_JSON_ELEMENT_OBJECT* dst_object)
{
	dst_object->memid_object = -1;
	dst_object->memory = NULL;
	if(array->memid_array==-1){
		return -1;
	}
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)array->memory;
	QS_ARRAY_ELEMENT* elm = qs_array_get(memory,array->memid_array,offset);
	if(elm==NULL){
		return -1;
	}
	if(elm->id != ELEMENT_HASH){
		return -1;
	}
	dst_object->memory = (void*)memory;
	dst_object->memid_object = elm->memid_array_element_data;
	return 0;
}



int api_qs_csv_read_file(QS_MEMORY_CONTEXT* context, QS_CSV_CONTEXT* csv, const char* csv_file_path)
{
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)context->memory;
	csv->memid_csv = qs_csv_file_load(memory,csv_file_path);
	if(-1==csv->memid_csv){
		return -1;
	}
	csv->memory = context->memory;
	return 0; 
}
int api_qs_csv_parse(QS_MEMORY_CONTEXT* context, QS_CSV_CONTEXT* csv, const char * src_csv)
{
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)context->memory;
	csv->memid_csv = qs_csv_parse(memory,src_csv);
	if(-1==csv->memid_csv){
		return -1;
	}
	csv->memory = context->memory;
	return 0; 
}
int32_t api_qs_csv_get_line_length(QS_CSV_CONTEXT* csv)
{
	if(-1==csv->memid_csv || csv->memory==NULL){
		return -1;
	}
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)csv->memory;
	return qs_csv_get_line_length(memory,csv->memid_csv);
}
int32_t api_qs_csv_get_row_length(QS_CSV_CONTEXT* csv, int32_t line_pos)
{
	if(-1==csv->memid_csv || csv->memory==NULL){
		return -1;
	}
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)csv->memory;
	return qs_csv_get_row_length(memory,csv->memid_csv,line_pos);
}
char* api_qs_csv_get_row(QS_CSV_CONTEXT* csv, int32_t line_pos, int32_t row_pos)
{
	if(-1==csv->memid_csv || csv->memory==NULL){
		return NULL;
	}
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)csv->memory;
	return qs_csv_get_row(memory,csv->memid_csv,line_pos,row_pos);
}
int api_qs_server_init(QS_SERVER_CONTEXT** ppcontext, int port, int32_t max_connection)
{
	if( ( (*ppcontext) = ( QS_SERVER_CONTEXT * )malloc( sizeof( QS_SERVER_CONTEXT ) ) ) == NULL ){
		return -1;
	}
	if(max_connection<=0){
		return -1;
	}
	QS_SERVER_CONTEXT* context = *ppcontext;
	context->system_data = NULL;
	context->on_connect = NULL;
	context->on_http_event = NULL;
	context->on_ws_event = NULL;
	context->on_close = NULL;
	context->current_time = time(NULL);
	context->update_time = 0;
	context->router_memory = NULL;
	context->memid_router = -1;
	context->kvs_memory = NULL;
	context->memid_kvs = -1;
	QS_MEMORY_POOL* main_memory_pool = NULL;
	//int32_t max_connection = 100;
	//if( qs_initialize_memory( &main_memory_pool, SIZE_MBYTE * 24, SIZE_MBYTE * 24, MEMORY_ALIGNMENT_SIZE_BIT_64, FIX_MUNIT_SIZE, 1, SIZE_KBYTE * 16 ) <= 0 ){
	//	free(context); context=NULL;
	//	return -2;
	//}
	size_t alloc_size = (((SIZE_KBYTE * 240LLU) * (size_t)(max_connection)) + (SIZE_MBYTE * 8LLU) + (SIZE_KBYTE * 32LLU));
	//printf("server alloc size : %zu\n",alloc_size);
	if( qs_initialize_memory( &main_memory_pool, alloc_size, alloc_size, MEMORY_ALIGNMENT_SIZE_BIT_64, FIX_MUNIT_SIZE, 1, SIZE_KBYTE * 16 ) <= 0 ){
		free(context); context=NULL;
		return -2;
	}
	context->memory = (void*)main_memory_pool;
	if( -1 == ( context->memid_temporary_memory = qs_create_mini_memory( main_memory_pool, SIZE_MBYTE * 8 ) ) ){
		free(context); context=NULL;
		return -3;
	}
	int32_t server_munit = qs_create_memory_block(main_memory_pool, sizeof(QS_SOCKET_OPTION));
	if (server_munit == -1) {
		free(context); context=NULL;
		return -4;
	}
	QS_SOCKET_OPTION* server = (QS_SOCKET_OPTION*)QS_GET_POINTER(main_memory_pool, server_munit);
	char portnum[32];
	memset( portnum, 0, sizeof( portnum ) );
	snprintf( portnum, sizeof( portnum ) -1, "%d", port );
	if (-1 == qs_initialize_socket_option(server, NULL, portnum, SOCKET_TYPE_SERVER_TCP, SOCKET_MODE_NONBLOCKING, PROTOCOL_PLAIN, max_connection, main_memory_pool, NULL)) {
		free(context); context=NULL;
		return -5;
	}
	qs_set_recv_buffer(server, SIZE_KBYTE*32);
	qs_set_send_buffer(server, SIZE_KBYTE*128);
	qs_set_message_buffer(server, SIZE_KBYTE*32);
	set_on_connect_event(server, api_qs_core_on_connect );
	set_on_plain_recv_event(server, api_qs_core_on_recv );
	set_on_close_event(server, api_qs_core_on_close );
	if (-1 == qs_socket(server)) {
		free(context); context=NULL;
		return -6;
	}
	context->memid_server = server_munit;
	int32_t memid_scheduler = qs_create_memory_block(main_memory_pool, sizeof(SYSTEM_UPDATE_SCHEDULER));
	if (memid_scheduler == -1) {
		free(context); context=NULL;
		return -7;
	}
	SYSTEM_UPDATE_SCHEDULER* scheduler = (SYSTEM_UPDATE_SCHEDULER*)QS_GET_POINTER(main_memory_pool, memid_scheduler);
	qs_initialize_scheduler_high_speed(scheduler);
	context->memid_scheduler = memid_scheduler;
	server->application_data = (void*)context;
	//qs_memory_info(main_memory_pool);
	return 0;
}
int api_qs_server_create_router(QS_SERVER_CONTEXT* context)
{
	QS_MEMORY_POOL* router_memory_pool = NULL;
	size_t route_save_data_size = QS_PACKET_ROUTE_DATA_SIZE_DEFAULT;
	size_t max_route_chain = QS_PACKET_ROUTE_ROUTE_SIZE_DEFAULT;
	size_t connection_data_size = (QS_PACKET_ROUTE_CAPACITY_DEFAULT * ( (SIZE_BYTE * 48) + QS_PACKET_ROUTE_CONNECTION_DATA_SIZE_DEFAULT + QS_PACKET_ROUTE_CONNECTION_DATA_SIZE_DEFAULT));
	size_t route_manage_size = max_route_chain * (connection_data_size + route_save_data_size + SIZE_KBYTE * 3);
	size_t alloc_size = QS_PACKET_ROUTE_ALLOC_SIZE_DEFAULT + route_manage_size;
	if( qs_initialize_memory( &router_memory_pool, alloc_size, alloc_size, MEMORY_ALIGNMENT_SIZE_BIT_64, FIX_MUNIT_SIZE, 1, SIZE_KBYTE * 16 ) <= 0 ){
		return -1;
	}
	context->router_memory = (void*)router_memory_pool;
	QS_MEMORY_POOL* server_memory_pool = (QS_MEMORY_POOL*)context->memory;
	QS_SOCKET_OPTION* server = (QS_SOCKET_OPTION*)QS_GET_POINTER(server_memory_pool, context->memid_server);
	context->memid_router = qs_init_packet_route(router_memory_pool, server->maxconnection, max_route_chain, QS_PACKET_ROUTE_KEY_SIZE_DEFAULT, route_save_data_size);
	if(-1==context->memid_router){
		return -1;
	}
	return 0;
}
void api_qs_router_memory_info(QS_SERVER_CONTEXT* context)
{
	if(NULL==context){
		return;
	}
	if(NULL==context->router_memory){
		return;
	}
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)context->router_memory;
	qs_memory_info(memory);
}
int api_qs_server_create_kvs(QS_SERVER_CONTEXT* context, int kvs_memory_type)
{
	QS_MEMORY_POOL* kvs_memory_pool = NULL;
	size_t kvs_alloc_size = SIZE_MBYTE * 1;
	switch(kvs_memory_type)
	{
		case QS_KVS_MEMORY_TYPE_B128MB:
			kvs_alloc_size = SIZE_MBYTE * 128;
			break;
		case QS_KVS_MEMORY_TYPE_B256MB:
			kvs_alloc_size = SIZE_MBYTE * 256;
			break;
		case QS_KVS_MEMORY_TYPE_B512MB:
			kvs_alloc_size = SIZE_MBYTE * 512;
			break;
		case QS_KVS_MEMORY_TYPE_B1024MB:
			kvs_alloc_size = SIZE_MBYTE * 1024;
			break;
		default:
			kvs_alloc_size = SIZE_MBYTE * 1;
			break;
	}
	if( qs_initialize_memory( &kvs_memory_pool, kvs_alloc_size + SIZE_KBYTE * 8, kvs_alloc_size + SIZE_KBYTE * 8, MEMORY_ALIGNMENT_SIZE_BIT_64, FIX_MUNIT_SIZE, 1, SIZE_KBYTE * 16 ) <= 0 ){
		return -1;
	}
	context->kvs_memory = (void*)kvs_memory_pool;
	context->memid_kvs_memory = qs_create_mini_memory(kvs_memory_pool, kvs_alloc_size);
	if(-1==context->memid_kvs_memory){
		return -1;
	}
	QS_MEMORY_POOL* cache_memory = (QS_MEMORY_POOL*)QS_GET_POINTER(kvs_memory_pool,context->memid_kvs_memory);
	context->memid_kvs = -1;
	switch(kvs_memory_type)
	{
		case QS_KVS_MEMORY_TYPE_B128MB:
			context->memid_kvs = qs_create_cache_B128MB(cache_memory);
			break;
		case QS_KVS_MEMORY_TYPE_B256MB:
			context->memid_kvs = qs_create_cache_B256MB(cache_memory);
			break;
		case QS_KVS_MEMORY_TYPE_B512MB:
			context->memid_kvs = qs_create_cache_B512MB(cache_memory);
			break;
		case QS_KVS_MEMORY_TYPE_B1024MB:
			context->memid_kvs = qs_create_cache_B1GB(cache_memory);
			break;
		default:
			context->memid_kvs = qs_create_cache_B1MB(cache_memory);
			break;
	}
	if(-1==context->memid_kvs){
		return -1;
	}
	return 0;
}
int api_qs_server_get_kvs(QS_SERVER_CONTEXT* context,QS_KVS_CONTEXT* kvs_context)
{
	if(NULL == context->kvs_memory){
		return -1;
	}
	QS_MEMORY_POOL* kvs_memory_pool = (QS_MEMORY_POOL*)context->kvs_memory;
	kvs_context->memory = kvs_memory_pool;
	kvs_context->memid_kvs_memory = context->memid_kvs_memory;
	kvs_context->memid_kvs = context->memid_kvs;
	kvs_context->is_persistence = false;
	memset(kvs_context->persistence_file_path,0,sizeof(kvs_context->persistence_file_path));
	return 0;
}
void api_qs_set_on_connect_event(QS_SERVER_CONTEXT* context, QS_EVENT_FUNCTION on_connect )
{
	context->on_connect = on_connect;
}
void api_qs_set_on_http_event(QS_SERVER_CONTEXT* context, QS_EVENT_FUNCTION on_http_event )
{
	context->on_http_event = on_http_event;
}
void api_qs_set_on_websocket_event(QS_SERVER_CONTEXT* context, QS_EVENT_FUNCTION on_ws_event )
{
	context->on_ws_event = on_ws_event;
}
void api_qs_set_on_close_event(QS_SERVER_CONTEXT* context, QS_EVENT_FUNCTION on_close )
{
	context->on_close = on_close;
}
int api_qs_server_create_logger_access(QS_SERVER_CONTEXT* context,const char* log_file_path)
{
	QS_MEMORY_POOL* main_memory_pool = (QS_MEMORY_POOL*)context->memory;
	QS_SOCKET_OPTION* server = (QS_SOCKET_OPTION*)QS_GET_POINTER(main_memory_pool, context->memid_server);
	if( QS_SYSTEM_ERROR == qs_log_open(&server->log_access_file_info, log_file_path)){
		return -1;
	}
	return 0;
}
int api_qs_server_create_logger_debug(QS_SERVER_CONTEXT* context,const char* log_file_path)
{
	QS_MEMORY_POOL* main_memory_pool = (QS_MEMORY_POOL*)context->memory;
	QS_SOCKET_OPTION* server = (QS_SOCKET_OPTION*)QS_GET_POINTER(main_memory_pool, context->memid_server);
	if( QS_SYSTEM_ERROR == qs_log_open(&server->log_debug_file_info, log_file_path)){
		return -1;
	}
	return 0;
}
int api_qs_server_create_logger_error(QS_SERVER_CONTEXT* context,const char* log_file_path)
{
	QS_MEMORY_POOL* main_memory_pool = (QS_MEMORY_POOL*)context->memory;
	QS_SOCKET_OPTION* server = (QS_SOCKET_OPTION*)QS_GET_POINTER(main_memory_pool, context->memid_server);
	if( QS_SYSTEM_ERROR == qs_log_open(&server->log_error_file_info, log_file_path)){
		return -1;
	}
	return 0;
}
void api_qs_update(QS_SERVER_CONTEXT* context)
{
	QS_MEMORY_POOL* main_memory_pool = (QS_MEMORY_POOL*)context->memory;
	QS_SOCKET_OPTION* server = (QS_SOCKET_OPTION*)QS_GET_POINTER(main_memory_pool, context->memid_server);
	context->current_time = time(NULL);
	if (context->current_time - context->update_time > 60) {
		//if( QS_SYSTEM_ERROR == qs_log_rotate(&server->log_access_file_info, "./", "access", 0)){}
		//if( QS_SYSTEM_ERROR == qs_log_rotate(&server->log_error_file_info, "./", "error", 0)){}
		if(-1 != context->memid_router){
			QS_MEMORY_POOL* router_memory = (QS_MEMORY_POOL*)context->router_memory;
			qs_update_packet_route(router_memory, context->memid_router);
		}
		context->update_time = context->current_time;
	}
	qs_server_update(server);
}

void api_qs_sleep(QS_SERVER_CONTEXT* context)
{
	QS_MEMORY_POOL* main_memory_pool = (QS_MEMORY_POOL*)context->memory;
	SYSTEM_UPDATE_SCHEDULER* scheduler = (SYSTEM_UPDATE_SCHEDULER*)QS_GET_POINTER(main_memory_pool, context->memid_scheduler);
	qs_sleep(scheduler->sleep_time);
	qs_update_scheduler(scheduler);
}

void api_qs_free(QS_SERVER_CONTEXT* context)
{
	QS_MEMORY_POOL* main_memory_pool = (QS_MEMORY_POOL*)context->memory;
	QS_SOCKET_OPTION* server = (QS_SOCKET_OPTION*)QS_GET_POINTER(main_memory_pool, context->memid_server);
	qs_free_socket( server );
	qs_free( server->memory_pool );
	if(context->router_memory!=NULL){
		QS_MEMORY_POOL* router_memory = context->router_memory;
		qs_free( router_memory );
	}
	if(context->kvs_memory!=NULL){
		QS_MEMORY_POOL* kvs_memory = context->kvs_memory;
		qs_free( kvs_memory );
	}
	free(context);
}

void api_qs_on_recv( void* params )
{
	QS_RECV_INFO *rinfo = (QS_RECV_INFO *)params;
	switch( qs_http_protocol_filter_with_websocket(rinfo) )
	{
		case -1:
		{
			QS_SERVER_CONNECTION_INFO * tinfo = (QS_SERVER_CONNECTION_INFO *)rinfo->tinfo;
			QS_SOCKPARAM* psockparam = &tinfo->sockparam;
			qs_disconnect( psockparam );
			break;
		}
		case QS_HTTP_SOCK_PHASE_RECV_CONTINUE:
		{
			break;
		}
		case QS_HTTP_SOCK_PHASE_PARSE_HTTP_HEADER:
		{
			break;
		}
		case QS_HTTP_SOCK_PHASE_MSG_HTTP:
		{
			api_qs_exec_http(rinfo);
			break;
		}
		case QS_HTTP_SOCK_PHASE_HANDSHAKE_WEBSOCKET:
		{
			// make unique id
			QS_SERVER_CONNECTION_INFO * connection = (QS_SERVER_CONNECTION_INFO *)rinfo->tinfo;
			QS_SERVER_CONTEXT* context = ((QS_SOCKET_OPTION*)connection->qs_socket_option)->application_data;
			if(-1 != context->memid_router)
			{
				QS_MEMORY_POOL* router_memory = (QS_MEMORY_POOL*)context->router_memory;
				qs_change_packet_route_connection_id(router_memory,context->memid_router,connection->index);
			}
			break;
		}
		case QS_HTTP_SOCK_PHASE_MSG_WEBSOCKET:
		{
			api_qs_exec_websocket(rinfo);
			break;
		}
	}
	QS_SERVER_CONTEXT* context = ((QS_SOCKET_OPTION*)rinfo->tinfo->qs_socket_option)->application_data;
	QS_MEMORY_POOL* main_memory_pool = (QS_MEMORY_POOL*)context->memory;
	SYSTEM_UPDATE_SCHEDULER* scheduler = (SYSTEM_UPDATE_SCHEDULER*)QS_GET_POINTER(main_memory_pool, context->memid_scheduler);
	scheduler->counter++;
}

void api_qs_exec_websocket(QS_RECV_INFO *rinfo)
{
	QS_SERVER_CONNECTION_INFO * tinfo = (QS_SERVER_CONNECTION_INFO *)rinfo->tinfo;
	QS_SOCKET_OPTION* option = (QS_SOCKET_OPTION*)tinfo->qs_socket_option;
	QS_SERVER_CONTEXT* context = (QS_SERVER_CONTEXT*)option->application_data;
	QS_MEMORY_POOL * temporary_memory = ( QS_MEMORY_POOL* )QS_GET_POINTER( option->memory_pool, context->memid_temporary_memory );
	qs_memory_clean( temporary_memory );
	QS_SOCKPARAM* psockparam = &tinfo->sockparam;
	ssize_t _tmpmsglen = qs_parse_websocket_binary( option, psockparam, (uint8_t*)qs_upointer( option->memory_pool, rinfo->recvbuf_munit ), rinfo->recvlen, tinfo->recvmsg_munit );
	if( _tmpmsglen < 0 ){
		qs_disconnect( psockparam );
		return;
	}
	if( psockparam->tmpmsglen == 0 ){
		if(_tmpmsglen==0){ // ping packet
			return;
		}
		char* msgpbuf = (char*)qs_upointer(option->memory_pool, tinfo->recvmsg_munit);
		msgpbuf[_tmpmsglen] = '\0';

		if(context->on_ws_event!=NULL){
			context->system_data = (void*)(msgpbuf);
			context->on_ws_event((void*)rinfo);
		}
		else
		{
			api_qs_send_ws_message_common(rinfo,msgpbuf,false);
		}
	}
}

void api_qs_exec_http(QS_RECV_INFO *rinfo)
{
	QS_SERVER_CONNECTION_INFO * tinfo = (QS_SERVER_CONNECTION_INFO *)rinfo->tinfo;
	QS_SOCKET_OPTION* option = (QS_SOCKET_OPTION*)tinfo->qs_socket_option;
	QS_SERVER_CONTEXT* context = (QS_SERVER_CONTEXT*)option->application_data;
	QS_MEMORY_POOL * temporary_memory = ( QS_MEMORY_POOL* )QS_GET_POINTER( option->memory_pool, context->memid_temporary_memory );
	qs_memory_clean( temporary_memory );
	QS_HTTP_REQUEST_COMMON http_request;
	int32_t http_status_code = http_request_common(rinfo, &http_request, temporary_memory);
	context->system_data = (void*)(&http_request);

	// API
	{
		QS_EVENT_PARAMETER params = (void*)rinfo;
		QS_MEMORY_CONTEXT memory;
		QS_JSON_ELEMENT_OBJECT object;
		memory.memory = (void*)temporary_memory;

		// curl -X POST http://localhost:8080/api/v1/mem/set -d 'k=memk1&v=memvalue1'
		if(!strcmp(api_qs_get_http_method(params),"POST")){
			if(!strcmp(api_qs_get_http_path(params),"/api/v1/mem/set")){
				char* key = api_qs_get_http_post_parameter(params,"k");
				char* value = api_qs_get_http_post_parameter(params,"v");
				if(key!=0&&value!=0){
					QS_KVS_CONTEXT kvs;
					if(-1!=api_qs_server_get_kvs(context,&kvs)){
						int result = api_qs_kvs_set(&kvs,key,value,0);
						api_qs_object_create(&memory,&object);
						api_qs_object_push_string(&object,"k",key);
						api_qs_object_push_string(&object,"v",value);
						api_qs_object_push_integer(&object,"result",result);
						http_status_code = api_qs_http_response_json(params,&object,1024 * 1024);
					}
				}
			}
		}

		// curl -X POST http://localhost:8080/api/v1/mem/get -d 'k=memk1'
		if(!strcmp(api_qs_get_http_method(params),"POST")){
			if(!strcmp(api_qs_get_http_path(params),"/api/v1/mem/get")){
				char* key = api_qs_get_http_post_parameter(params,"k");
				if(key!=0){
					QS_KVS_CONTEXT kvs;
					if(-1!=api_qs_server_get_kvs(context,&kvs)){
						char* value = api_qs_kvs_get(&kvs,key);
						api_qs_object_create(&memory,&object);
						api_qs_object_push_string(&object,"k",key);
						if(value==0){
							api_qs_object_push_string(&object,"v","");
						}else{
							api_qs_object_push_string(&object,"v",value);
						}
						http_status_code = api_qs_http_response_json(params,&object,1024 * 1024);
					}
				}
			}
		}

		// curl -X POST http://localhost:8080/api/v1/mem/delete -d 'k=memk1'
		if(!strcmp(api_qs_get_http_method(params),"POST")){
			if(!strcmp(api_qs_get_http_path(params),"/api/v1/mem/delete")){
				char* key = api_qs_get_http_post_parameter(params,"k");
				if(key!=0){
					QS_KVS_CONTEXT kvs;
					if(-1!=api_qs_server_get_kvs(context,&kvs)){
						api_qs_kvs_delete(&kvs,key);
						api_qs_object_create(&memory,&object);
						api_qs_object_push_string(&object,"k",key);
						http_status_code = api_qs_http_response_json(params,&object,1024*64);
					}
				}
			}
		}

		// curl -X POST http://localhost:8080/api/v1/mem/keys
		if(!strcmp(api_qs_get_http_method(params),"POST")){
			if(!strcmp(api_qs_get_http_path(params),"/api/v1/mem/keys")){
				QS_KVS_CONTEXT kvs;
				QS_JSON_ELEMENT_ARRAY array;
				if(-1!=api_qs_server_get_kvs(context,&kvs)){
					api_qs_object_create(&memory,&object);
					api_qs_array_create(&memory,&array);
					int32_t key_length = api_qs_kvs_keys(&array,&kvs);
					api_qs_object_push_integer(&object,"len",key_length);
					api_qs_object_push_array(&object,"keys",&array);
					http_status_code = api_qs_http_response_json(params,&object,1024*64);
				}
			}
		}

		// curl -X POST http://localhost:8080/api/v1/room/create -d 'name=testroom1'
		if(!strcmp(api_qs_get_http_method(params),"POST")){
			if(!strcmp(api_qs_get_http_path(params),"/api/v1/room/create")){
				char* room_name = api_qs_get_http_post_parameter(params,"name");
				if(room_name!=0){
					if(0==api_qs_room_create(context,room_name,&memory,&object)){
						http_status_code = api_qs_http_response_json(params,&object,1024*8);
					}
				}
			}
		}

		// curl -X POST http://localhost:8080/api/v1/room/list
		// curl -X GET http://localhost:8080/api/v1/room/list
		if(!strcmp(api_qs_get_http_method(params),"POST")||!strcmp(api_qs_get_http_method(params),"GET")){
			if(!strcmp(api_qs_get_http_path(params),"/api/v1/room/list")){
				if(0==api_qs_room_list(context,&memory,&object)){
					http_status_code = api_qs_http_response_json(params,&object,1024*8);
				}
			}
		}

		// curl -X POST http://localhost:8080/api/v1/room/join -d 'room_id=WTeiylPnFIDeyPOnTnBU&connection_id=klV7RRk1vgpshZfdzzPjcC4PM7sDBCd8'
		if(!strcmp(api_qs_get_http_method(params),"POST")){
			if(!strcmp(api_qs_get_http_path(params),"/api/v1/room/join")){
				char* room_id = api_qs_get_http_post_parameter(params,"room_id");
				char* connection_id = api_qs_get_http_post_parameter(params,"connection_id");
				if(room_id!=0&&connection_id!=0){
					if(0==api_qs_room_join(context,room_id,connection_id,&memory,&object)){
						http_status_code = api_qs_http_response_json(params,&object,1024*8);
					} else{
						http_status_code = 404;
					}
				}
			}
		}
	}
	if (http_status_code == 404) {
		if(context->on_http_event!=NULL){
			http_status_code = context->on_http_event((void*)rinfo);
		}
	}
	if( http_status_code != 200 ){
		if( http_status_code == 304 ){
			qs_send( option, &tinfo->sockparam, HTTP_NOT_MODIFIED, qs_strlen( HTTP_NOT_MODIFIED ), 0 );
		}
		else if( http_status_code == 404 ){
			qs_send( option, &tinfo->sockparam, HTTP_NOT_FOUND, qs_strlen( HTTP_NOT_FOUND ), 0 );
		}
		else{
			qs_send( option, &tinfo->sockparam, HTTP_INTERNAL_SERVER_ERROR, qs_strlen( HTTP_INTERNAL_SERVER_ERROR ), 0 );
		}
	}
	qs_http_access_log(&option->log_access_file_info, &http_request, http_status_code);
	qs_disconnect(&tinfo->sockparam);
}
int api_qs_core_on_connect(QS_SERVER_CONNECTION_INFO* connection)
{
	QS_SERVER_CONTEXT* context = ((QS_SOCKET_OPTION*)connection->qs_socket_option)->application_data;
	if(context->on_connect){
		context->on_connect((void*)connection);
	}
	return 0;
}
int32_t api_qs_core_on_recv(uint8_t* payload, size_t payload_len, QS_RECV_INFO *qs_recv_info)
{
	api_qs_on_recv( (void*)qs_recv_info );
	return 0;
}
int api_qs_core_on_close(QS_SERVER_CONNECTION_INFO* connection)
{
	QS_SERVER_CONTEXT* context = ((QS_SOCKET_OPTION*)connection->qs_socket_option)->application_data;
	if(context->on_close){
		context->on_close((void*)connection);
	}
	if(-1 != context->memid_router)
	{
		QS_MEMORY_POOL* router_memory = (QS_MEMORY_POOL*)context->router_memory;
		void* data = qs_get_packet_route_connection_chain(router_memory, context->memid_router, connection->index);
		if (NULL != data) {
			do{
				QS_SOCKET_OPTION* option = (QS_SOCKET_OPTION*)connection->qs_socket_option;
				QS_MEMORY_POOL * temporary_memory = ( QS_MEMORY_POOL* )QS_GET_POINTER( option->memory_pool, context->memid_temporary_memory );
				qs_memory_clean( temporary_memory );
				int32_t message_buffer_munit = qs_create_memory_block(temporary_memory, SIZE_KBYTE * 32);
				void* buffer = QS_GET_POINTER(temporary_memory, message_buffer_munit);
				size_t buffer_size = qs_usize(temporary_memory, message_buffer_munit);
				char* connection_id = qs_get_packet_route_connection_id(router_memory, context->memid_router, connection->index);
				int32_t route_offset = qs_get_packet_route_connection_offset(router_memory, context->memid_router, connection->index);

				// change owner
				qs_change_packet_route_owner(router_memory, context->memid_router, connection->index);
				qs_remove_packet_route_connection(router_memory, context->memid_router, connection->index);

				char* json = NULL;
				{
					int32_t memid_temp_info_hash = qs_get_route_info(router_memory,context->memid_router,temporary_memory,route_offset);
					if(-1==memid_temp_info_hash){
						return -1;
					}

					int32_t memid_response_body = qs_json_encode_hash(temporary_memory, memid_temp_info_hash, SIZE_KBYTE * 8);
					if (-1 == memid_response_body) {
						return -1;
					}
					json = (char*)QS_GET_POINTER(temporary_memory, memid_response_body);	
				}

				ssize_t sendlen = qs_make_ws_message_simple(temporary_memory, connection_id,"leave",json,buffer,buffer_size);
				void* current = NULL;
				ssize_t ret = 0;
				QS_SERVER_CONNECTION_INFO *tmptinfo;
				while (NULL != (current = qs_system_foreach_packet_route_connection_chain(router_memory, context->memid_router, route_offset, current))) {
					QS_PACKET_ROUTE_NODE_CONNECTION* con = (QS_PACKET_ROUTE_NODE_CONNECTION*)current;
					if(con->connection_index==connection->index){
						continue;
					}
					tmptinfo = qs_offsetpointer(option->memory_pool, option->connection_munit, sizeof(QS_SERVER_CONNECTION_INFO), con->connection_index);
					if (tmptinfo->sockparam.acc != -1 && tmptinfo->sockparam.phase == QS_HTTP_SOCK_PHASE_MSG_WEBSOCKET) {
						if (-1 == (ret = qs_send_all(tmptinfo->sockparam.acc, buffer, sendlen, 0))) {
							qs_close_socket_common(option, tmptinfo, 0);
						}
					}
				}
			}while(false);
		}
	}
	return 0;
}
char* api_qs_get_ws_message(QS_EVENT_PARAMETER params)
{
	QS_RECV_INFO *rinfo = (QS_RECV_INFO *)params;
	QS_SERVER_CONNECTION_INFO * tinfo = (QS_SERVER_CONNECTION_INFO *)rinfo->tinfo;
	QS_SOCKET_OPTION* option = (QS_SOCKET_OPTION*)tinfo->qs_socket_option;
	QS_SERVER_CONTEXT* context = (QS_SERVER_CONTEXT*)option->application_data;
	char* message = (char*)context->system_data;
	return message;
}
int api_qs_send_ws_message(QS_EVENT_PARAMETER params,const char* message)
{
	QS_RECV_INFO *qs_recv_info = (QS_RECV_INFO *)params;
	return api_qs_send_ws_message_common(qs_recv_info,message,false);
}
int api_qs_send_ws_message_plane(QS_EVENT_PARAMETER params,const char* message)
{
	QS_RECV_INFO *qs_recv_info = (QS_RECV_INFO *)params;
	return api_qs_send_ws_message_common(qs_recv_info,message,true);
}
int api_qs_send_ws_message_common(QS_RECV_INFO *qs_recv_info,const char* message,int is_plane)
{
	QS_SERVER_CONNECTION_INFO * tinfo = (QS_SERVER_CONNECTION_INFO *)qs_recv_info->tinfo;
	QS_SOCKET_OPTION* option = (QS_SOCKET_OPTION*)tinfo->qs_socket_option;
	QS_SERVER_CONTEXT* context = (QS_SERVER_CONTEXT*)option->application_data;
	QS_MEMORY_POOL * temporary_memory = ( QS_MEMORY_POOL* )QS_GET_POINTER( option->memory_pool, context->memid_temporary_memory );
	QS_SOCKPARAM* psockparam = &tinfo->sockparam;
	if(-1!=context->memid_router)
	{
		int32_t message_buffer_munit = qs_create_memory_block(temporary_memory,qs_strlen(message) + SIZE_KBYTE*8);
		if(-1==message_buffer_munit){
			return -1;
		}
		void* buffer = QS_GET_POINTER(temporary_memory,message_buffer_munit);
		size_t buffer_size = qs_usize(temporary_memory,message_buffer_munit);
		QS_MEMORY_POOL* router_memory = (QS_MEMORY_POOL*)context->router_memory;
		char* connection_id = qs_get_packet_route_connection_id(router_memory, context->memid_router, tinfo->index);
		ssize_t sendlen = 0;
		if(is_plane){
			sendlen = qs_make_websocket_msg((void*)buffer, buffer_size, false, message, qs_strlen(message));
		} else{
			sendlen = qs_make_ws_message_simple(temporary_memory, connection_id,"message",(char*)message,buffer,buffer_size);
		}
		if(sendlen==0){
			return -1;
		}
		ssize_t ret = 0;
		void* data = qs_get_packet_route_connection_chain(router_memory, context->memid_router, tinfo->index);
		if (NULL != data) {
			void* current = NULL;
			QS_SERVER_CONNECTION_INFO *tmptinfo;
			while (NULL != (current = qs_foreach_packet_route_connection_chain(router_memory, context->memid_router, tinfo->index, current))) {
				QS_PACKET_ROUTE_NODE_CONNECTION* con = (QS_PACKET_ROUTE_NODE_CONNECTION*)current;
				tmptinfo = qs_offsetpointer(option->memory_pool, option->connection_munit, sizeof(QS_SERVER_CONNECTION_INFO), con->connection_index);
				if (tmptinfo->sockparam.acc != -1 && tmptinfo->sockparam.phase == QS_HTTP_SOCK_PHASE_MSG_WEBSOCKET) {
					if (-1 == (ret = qs_send_all(tmptinfo->sockparam.acc, buffer, sendlen, 0))) {
						qs_close_socket_common(option, tmptinfo, 0);
					}
				}
			}
		}
		else {
			// send echo
			if (-1 == (ret = qs_send_all(psockparam->acc, buffer, sendlen, 0))) {

			}
		}
	} else {
		// simple broadcast
		int i;
		ssize_t ret = 0;
		QS_SERVER_CONNECTION_INFO *tmptinfo;
		if( option->connection_munit != -1 ){
			int32_t message_buffer_munit = qs_create_memory_block(temporary_memory,qs_strlen(message) + SIZE_KBYTE*8);
			if(-1==message_buffer_munit){
				return -1;
			}
			void* buffer = QS_GET_POINTER(temporary_memory,message_buffer_munit);
			size_t buffer_size = qs_usize(temporary_memory,message_buffer_munit);
			ssize_t sendlen = 0;
			if(is_plane){
				sendlen = qs_make_websocket_msg((void*)buffer, buffer_size, false, message, qs_strlen(message));
			} else{
				sendlen = qs_make_websocket_msg(buffer,buffer_size,false,(char*)message,qs_strlen(message));
			}
			if(sendlen==0){
				return -1;
			}
			for( i = 0; i < option->maxconnection; i++ ){
				tmptinfo = qs_offsetpointer( option->memory_pool, option->connection_munit, sizeof( QS_SERVER_CONNECTION_INFO ), i );
				if( tmptinfo->sockparam.acc != -1 && tmptinfo->sockparam.phase == QS_HTTP_SOCK_PHASE_MSG_WEBSOCKET ){
					if( -1 == ( ret = qs_send_all( tmptinfo->sockparam.acc, buffer, sendlen, 0 ) ) ){
						qs_close_socket_common(option, tmptinfo, 0);
					}
				}
			}
		}
	}
	return 0;
}
char* api_qs_get_http_method(void* params)
{
	QS_RECV_INFO *rinfo = (QS_RECV_INFO *)params;
	QS_SERVER_CONNECTION_INFO * tinfo = (QS_SERVER_CONNECTION_INFO *)rinfo->tinfo;
	QS_SOCKET_OPTION* option = (QS_SOCKET_OPTION*)tinfo->qs_socket_option;
	QS_SERVER_CONTEXT* context = (QS_SERVER_CONTEXT*)option->application_data;
	QS_HTTP_REQUEST_COMMON* http_request = (QS_HTTP_REQUEST_COMMON*)context->system_data;
	return http_request->method;
}
char* api_qs_get_http_path(void* params)
{
	QS_RECV_INFO *rinfo = (QS_RECV_INFO *)params;
	QS_SERVER_CONNECTION_INFO * tinfo = (QS_SERVER_CONNECTION_INFO *)rinfo->tinfo;
	QS_SOCKET_OPTION* option = (QS_SOCKET_OPTION*)tinfo->qs_socket_option;
	QS_SERVER_CONTEXT* context = (QS_SERVER_CONTEXT*)option->application_data;
	QS_HTTP_REQUEST_COMMON* http_request = (QS_HTTP_REQUEST_COMMON*)context->system_data;
	return http_request->request;
}
char* api_qs_get_http_get_parameter(void* params, const char* name)
{
	QS_RECV_INFO *rinfo = (QS_RECV_INFO *)params;
	QS_SERVER_CONNECTION_INFO * tinfo = (QS_SERVER_CONNECTION_INFO *)rinfo->tinfo;
	QS_SOCKET_OPTION* option = (QS_SOCKET_OPTION*)tinfo->qs_socket_option;
	QS_SERVER_CONTEXT* context = (QS_SERVER_CONTEXT*)option->application_data;
	QS_HTTP_REQUEST_COMMON* http_request = (QS_HTTP_REQUEST_COMMON*)context->system_data;
	//qs_hash_dump(http_request->temporary_memory,http_request->memid_get_parameter_hash,0);
	if (-1 == http_request->memid_get_parameter_hash) {
		return NULL;
	}
	if (-1 == qs_get_hash(http_request->temporary_memory, http_request->memid_get_parameter_hash, name)) {
		return NULL;
	}
	char* value = (char*)QS_GET_POINTER(http_request->temporary_memory, qs_get_hash(http_request->temporary_memory, http_request->memid_get_parameter_hash, name));
	return value;
}
char* api_qs_get_http_post_parameter(void* params, const char* name)
{
	QS_RECV_INFO *rinfo = (QS_RECV_INFO *)params;
	QS_SERVER_CONNECTION_INFO * tinfo = (QS_SERVER_CONNECTION_INFO *)rinfo->tinfo;
	QS_SOCKET_OPTION* option = (QS_SOCKET_OPTION*)tinfo->qs_socket_option;
	QS_SERVER_CONTEXT* context = (QS_SERVER_CONTEXT*)option->application_data;
	QS_HTTP_REQUEST_COMMON* http_request = (QS_HTTP_REQUEST_COMMON*)context->system_data;
	if (-1 == http_request->memid_post_parameter_hash) {
		return NULL;
	}
	if (-1 == qs_get_hash(http_request->temporary_memory, http_request->memid_post_parameter_hash, name)) {
		return NULL;
	}
	char* value = (char*)QS_GET_POINTER(http_request->temporary_memory, qs_get_hash(http_request->temporary_memory, http_request->memid_post_parameter_hash, name));
	return value;
}

char* api_qs_get_http_post_body(QS_EVENT_PARAMETER params)
{
	QS_RECV_INFO *rinfo = (QS_RECV_INFO *)params;
	QS_SERVER_CONNECTION_INFO * tinfo = (QS_SERVER_CONNECTION_INFO *)rinfo->tinfo;
	QS_SOCKET_OPTION* option = (QS_SOCKET_OPTION*)tinfo->qs_socket_option;
	char *body = (char*)qs_upointer(option->memory_pool, rinfo->recvbuf_munit);
	return body;
}

void api_qs_get_http_post_json_object(QS_EVENT_PARAMETER params, QS_JSON_ELEMENT_OBJECT* object)
{
	QS_RECV_INFO *rinfo = (QS_RECV_INFO *)params;
	QS_SERVER_CONNECTION_INFO * tinfo = (QS_SERVER_CONNECTION_INFO *)rinfo->tinfo;
	QS_SOCKET_OPTION* option = (QS_SOCKET_OPTION*)tinfo->qs_socket_option;
	QS_SERVER_CONTEXT* context = (QS_SERVER_CONTEXT*)option->application_data;
	QS_HTTP_REQUEST_COMMON* http_request = (QS_HTTP_REQUEST_COMMON*)context->system_data;
	object->memid_object = http_request->memid_post_parameter_hash;
	object->memory = (void*)option->memory_pool;
}

void api_qs_send_response(void* params, const char* response)
{
	QS_RECV_INFO *rinfo = (QS_RECV_INFO *)params;
	QS_SERVER_CONNECTION_INFO * tinfo = (QS_SERVER_CONNECTION_INFO *)rinfo->tinfo;
	QS_SOCKET_OPTION* option = (QS_SOCKET_OPTION*)tinfo->qs_socket_option;
	qs_send( option, &tinfo->sockparam, (char*)response, qs_strlen( response ), 0 );
}
QS_SERVER_CONTEXT* api_qs_get_server_context(QS_EVENT_PARAMETER params)
{
	QS_RECV_INFO *rinfo = (QS_RECV_INFO *)params;
	QS_SERVER_CONNECTION_INFO * tinfo = (QS_SERVER_CONNECTION_INFO *)rinfo->tinfo;
	QS_SOCKET_OPTION* option = (QS_SOCKET_OPTION*)tinfo->qs_socket_option;
	QS_SERVER_CONTEXT* context = (QS_SERVER_CONTEXT*)option->application_data;
	return context;
}
int api_qs_script_read_file(QS_MEMORY_CONTEXT* memory_context, QS_SERVER_SCRIPT_CONTEXT* script_context,const char* file_path)
{
	QS_MEMORY_POOL * script_memory = (QS_MEMORY_POOL *)memory_context->memory;
	if( -1 == ( script_context->memid_script = qs_init_script( script_memory, 256, 64, 256 ) ) ){
		return -1;
	}
	qs_add_system_function( script_memory, script_context->memid_script, "echo", qs_script_system_function_echo, 0 );
	qs_add_system_function( script_memory, script_context->memid_script, "count", qs_script_system_function_count, 0 );
	qs_add_system_function( script_memory, script_context->memid_script, "file_exist", qs_script_system_function_file_exist, 0 );
	qs_add_system_function( script_memory, script_context->memid_script, "file_size", qs_script_system_function_file_size, 0 );
	qs_add_system_function( script_memory, script_context->memid_script, "file_extension", qs_script_system_function_file_extension, 0 );
	qs_add_system_function( script_memory, script_context->memid_script, "file_get", qs_script_system_function_file_get, 0 );
	qs_add_system_function( script_memory, script_context->memid_script, "file_put", qs_script_system_function_file_put, 0 );
	qs_add_system_function( script_memory, script_context->memid_script, "file_add", qs_script_system_function_file_add, 0 );
	qs_add_system_function( script_memory, script_context->memid_script, "json_encode", qs_script_system_function_json_encode, 0 );
	qs_add_system_function( script_memory, script_context->memid_script, "json_decode", qs_script_system_function_json_decode, 0 );
	qs_add_system_function( script_memory, script_context->memid_script, "gmtime", qs_script_system_function_gmtime, 0 );
	qs_add_system_function( script_memory, script_context->memid_script, "rand", qs_script_system_function_rand, 0 );
	qs_import_script( script_memory, &script_context->memid_script, (char*)file_path );
	script_context->memory = (void*)script_memory;
	return 0;
}
int api_qs_script_set_argv_object(QS_SERVER_SCRIPT_CONTEXT* script_context,const char* name, QS_JSON_ELEMENT_OBJECT* object)
{
	QS_MEMORY_POOL * script_memory = ( QS_MEMORY_POOL* )(script_context->memory);
	QS_SCRIPT *script = (QS_SCRIPT *)QS_GET_POINTER( script_memory, script_context->memid_script );
	qs_add_hash_value_kstring( script_memory, script->v_hash_munit, name, object->memid_object, ELEMENT_HASH );
	return 0;
}
int api_qs_script_set_argv_string(QS_SERVER_SCRIPT_CONTEXT* script_context,const char* name, const char* value)
{
	QS_MEMORY_POOL * script_memory = ( QS_MEMORY_POOL* )(script_context->memory);
	QS_SCRIPT *script = (QS_SCRIPT *)QS_GET_POINTER( script_memory, script_context->memid_script );
	return (NULL == qs_add_hash_string( script_memory, script->v_hash_munit, name, value )) ? -1 : 0;
}
int api_qs_script_set_argv_integer(QS_SERVER_SCRIPT_CONTEXT* script_context,const char* name, int32_t value)
{
	QS_MEMORY_POOL * script_memory = ( QS_MEMORY_POOL* )(script_context->memory);
	QS_SCRIPT *script = (QS_SCRIPT *)QS_GET_POINTER( script_memory, script_context->memid_script );
	return (NULL == qs_add_hash_integer( script_memory, script->v_hash_munit, name, value )) ? -1 : 0;
}
int api_qs_script_run(QS_SERVER_SCRIPT_CONTEXT* script_context)
{
	QS_MEMORY_POOL * script_memory = ( QS_MEMORY_POOL* )(script_context->memory);
	qs_exec( script_memory, &script_context->memid_script );
	return 0;
}
char* api_qs_script_get_parameter(QS_SERVER_SCRIPT_CONTEXT* script_context, const char* name)
{
	QS_MEMORY_POOL * script_memory = ( QS_MEMORY_POOL* )(script_context->memory);
	QS_SCRIPT *pscript = (QS_SCRIPT *)QS_GET_POINTER(script_memory, script_context->memid_script);
	int32_t memid_parameter = qs_get_hash(script_memory, pscript->v_hash_munit, name);
	if(memid_parameter==-1){
		return 0;
	}
	return (char*)QS_GET_POINTER(script_memory,memid_parameter);
}
int api_qs_kvs_create_b1mb(QS_MEMORY_CONTEXT* memory_context, QS_KVS_CONTEXT* kvs_context)
{
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)memory_context->memory;
	kvs_context->memid_kvs_memory = qs_create_mini_memory(memory, SIZE_MBYTE * 1);
	QS_MEMORY_POOL* cache_memory = (QS_MEMORY_POOL*)QS_GET_POINTER(memory,kvs_context->memid_kvs_memory);
	kvs_context->memid_kvs = qs_create_cache_B1MB(cache_memory);
	if(-1 == kvs_context->memid_kvs) {
		return -1;
	}
	kvs_context->memory = (void*)memory;
	memset(kvs_context->persistence_file_path,0,sizeof(kvs_context->persistence_file_path));
	kvs_context->is_persistence = false;
	return 0;
}
int api_qs_kvs_create_b8mb(QS_MEMORY_CONTEXT* memory_context, QS_KVS_CONTEXT* kvs_context)
{
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)memory_context->memory;
	kvs_context->memid_kvs_memory = qs_create_mini_memory(memory, SIZE_MBYTE * 8);
	QS_MEMORY_POOL* cache_memory = (QS_MEMORY_POOL*)QS_GET_POINTER(memory,kvs_context->memid_kvs_memory);
	kvs_context->memid_kvs = qs_create_cache_B8MB(cache_memory);
	if(-1 == kvs_context->memid_kvs) {
		return -1;
	}
	kvs_context->memory = (void*)memory;
	memset(kvs_context->persistence_file_path,0,sizeof(kvs_context->persistence_file_path));
	kvs_context->is_persistence = false;
	return 0;
}
int api_qs_kvs_create_b1mb_persistence(QS_KVS_CONTEXT* kvs_context, const char* file_path)
{
	QS_MEMORY_POOL* memory = NULL;
	kvs_context->memid_kvs = qs_create_storage_cache_B1MB(file_path,&memory);
	if(-1 == kvs_context->memid_kvs) {
		return -1;
	}
	kvs_context->memory = (void*)memory;
	memset(kvs_context->persistence_file_path,0,sizeof(kvs_context->persistence_file_path));
	snprintf(kvs_context->persistence_file_path,sizeof(kvs_context->persistence_file_path),"%s",file_path);
	kvs_context->is_persistence = true;
	kvs_context->memid_kvs_memory = -1;
	return 0;
}
int api_qs_kvs_create_b8mb_persistence(QS_KVS_CONTEXT* kvs_context, const char* file_path)
{
	QS_MEMORY_POOL* memory = NULL;
	kvs_context->memid_kvs = qs_create_storage_cache_B8MB(file_path,&memory);
	if(-1 == kvs_context->memid_kvs) {
		return -1;
	}
	kvs_context->memory = (void*)memory;
	memset(kvs_context->persistence_file_path,0,sizeof(kvs_context->persistence_file_path));
	snprintf(kvs_context->persistence_file_path,sizeof(kvs_context->persistence_file_path),"%s",file_path);
	kvs_context->is_persistence = true;
	kvs_context->memid_kvs_memory = -1;
	return 0;
}
int api_qs_kvs_set(QS_KVS_CONTEXT* kvs_context,const char* key, const char* value, int32_t life_time)
{
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)kvs_context->memory;
	QS_MEMORY_POOL* cache_memory = NULL;
	if(kvs_context->is_persistence){
		cache_memory = memory;
	}else{
		cache_memory = (QS_MEMORY_POOL*)QS_GET_POINTER(memory,kvs_context->memid_kvs_memory);
	}
	QS_CACHE* cache = (QS_CACHE*)QS_GET_POINTER(cache_memory,kvs_context->memid_kvs);
	size_t key_size = strlen(key);
	if( key_size >= cache->max_key_size && key_size <= 0){
		//printf("invalid key size : %d\n",(int)key_size);
		return -1;
	}
	return qs_cache_string(cache,(char*)key,(char*)value,life_time);
}
char* api_qs_kvs_get(QS_KVS_CONTEXT* kvs_context,const char* key)
{
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)kvs_context->memory;
	QS_MEMORY_POOL* cache_memory = NULL;
	if(kvs_context->is_persistence){
		cache_memory = memory;
	}else{
		cache_memory = (QS_MEMORY_POOL*)QS_GET_POINTER(memory,kvs_context->memid_kvs_memory);
	}
	QS_CACHE* cache = (QS_CACHE*)QS_GET_POINTER(cache_memory,kvs_context->memid_kvs);
	char* value = NULL;
	QS_CACHE_PAGE cache_page;
	qs_get_cache_page(cache,&cache_page);
	int32_t memid_hash_value = qs_get_hash(cache_page.memory,cache_page.hash_id,key);
	if(-1 != memid_hash_value){
		value = (char*)QS_GET_POINTER(cache_page.memory, memid_hash_value);
	}
	return value;
}
size_t api_qs_kvs_get_buffer_size(QS_KVS_CONTEXT* kvs_context,const char* key)
{
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)kvs_context->memory;
	QS_MEMORY_POOL* cache_memory = NULL;
	if(kvs_context->is_persistence){
		cache_memory = memory;
	}else{
		cache_memory = (QS_MEMORY_POOL*)QS_GET_POINTER(memory,kvs_context->memid_kvs_memory);
	}
	QS_CACHE* cache = (QS_CACHE*)QS_GET_POINTER(cache_memory,kvs_context->memid_kvs);
	size_t buffer_size = 0;
	QS_CACHE_PAGE cache_page;
	qs_get_cache_page(cache,&cache_page);
	int32_t memid_hash_value = qs_get_hash(cache_page.memory,cache_page.hash_id,key);
	if(-1 != memid_hash_value){
		buffer_size = qs_usize(cache_page.memory, memid_hash_value);
	}
	return buffer_size;
}
int api_qs_kvs_delete(QS_KVS_CONTEXT* kvs_context,const char* key)
{
	QS_MEMORY_POOL* memory = (QS_MEMORY_POOL*)kvs_context->memory;
	QS_MEMORY_POOL* cache_memory = NULL;
	if(kvs_context->is_persistence){
		cache_memory = memory;
	}else{
		cache_memory = (QS_MEMORY_POOL*)QS_GET_POINTER(memory,kvs_context->memid_kvs_memory);
	}
	QS_CACHE* cache = (QS_CACHE*)QS_GET_POINTER(cache_memory,kvs_context->memid_kvs);
	QS_CACHE_PAGE cache_page;
	qs_get_cache_page(cache,&cache_page);
	return qs_remove_hash(cache_page.memory,cache_page.hash_id,key);
}
int32_t api_qs_kvs_keys(QS_JSON_ELEMENT_ARRAY* array, QS_KVS_CONTEXT* kvs_context)
{
	int32_t key_length = 0;
	QS_MEMORY_POOL* cache_main_memory = (QS_MEMORY_POOL*)kvs_context->memory;
	QS_MEMORY_POOL* cache_memory = NULL;
	if(kvs_context->is_persistence){
		cache_memory = cache_main_memory;
	}else{
		cache_memory = (QS_MEMORY_POOL*)QS_GET_POINTER(cache_main_memory,kvs_context->memid_kvs_memory);
	}
	QS_CACHE* cache = (QS_CACHE*)QS_GET_POINTER(cache_memory,kvs_context->memid_kvs);
	QS_CACHE_PAGE cache_page;
	qs_get_cache_page(cache,&cache_page);
	QS_HASH_FOREACH hf;
	QS_HASH_ELEMENT* he;
	qs_init_hash_foreach( cache_page.memory, cache_page.hash_id, &hf );
	while( NULL != ( he = qs_hash_foreach( cache_page.memory, &hf ) ) ){
		api_qs_array_push_string(array,(char*)QS_GET_POINTER(cache_page.memory,he->memid_hash_name));
		//printf(
		//	"%s=%s\n",
		//	(char*)QS_GET_POINTER(cache_page.memory,he->memid_hash_name),
		//	(char*)QS_GET_POINTER(cache_page.memory,he->memid_hash_element_data)
		//);
		key_length++;
	}
	return key_length;
}

int32_t api_qs_kvs_sorted_keys(QS_JSON_ELEMENT_ARRAY* array, QS_KVS_CONTEXT* kvs_context, int32_t is_sort_asc)
{
	int32_t key_length = 0;
	QS_MEMORY_POOL* sort_buffer_memory = (QS_MEMORY_POOL*)array->memory;
	QS_MEMORY_POOL* cache_main_memory = (QS_MEMORY_POOL*)kvs_context->memory;
	QS_MEMORY_POOL* cache_memory = NULL;
	if(kvs_context->is_persistence){
		cache_memory = cache_main_memory;
	}else{
		cache_memory = (QS_MEMORY_POOL*)QS_GET_POINTER(cache_main_memory,kvs_context->memid_kvs_memory);
	}
	QS_CACHE* cache = (QS_CACHE*)QS_GET_POINTER(cache_memory,kvs_context->memid_kvs);
	QS_CACHE_PAGE cache_page;
	qs_get_cache_page(cache,&cache_page);

	int32_t memid_sorted_keys = qs_get_hash_keys(sort_buffer_memory,cache_page.memory,cache_page.hash_id,is_sort_asc);
	if(-1 == memid_sorted_keys){
		return -1;
	}

	key_length = qs_array_length(sort_buffer_memory,memid_sorted_keys);
	int32_t i;
	QS_ARRAY_ELEMENT* ae;
	for(i=0;i<key_length;i++){
		ae = qs_array_get(sort_buffer_memory,memid_sorted_keys,i);
		if(NULL == ae){
			continue;
		}
		int32_t memid_key = ae->memid_array_element_data;
		char* key = (char*)QS_GET_POINTER(sort_buffer_memory,memid_key);
		api_qs_array_push_string(array,key);
	}
	return key_length;
}

int api_qs_persistence_kvs_memory_free(QS_KVS_CONTEXT* kvs_context)
{
	if(kvs_context->is_persistence){
		QS_MEMORY_POOL* cache_main_memory = (QS_MEMORY_POOL*)kvs_context->memory;
		qs_free(cache_main_memory);
	}
	return 0;
}

int api_qs_room_create(QS_SERVER_CONTEXT* context, const char* name, QS_MEMORY_CONTEXT* dest_memory, QS_JSON_ELEMENT_OBJECT* dest_object)
{
	int result = -1;
	do {
		dest_object->memory = NULL;
		dest_object->memid_object = -1;
		QS_MEMORY_POOL * dest_temporary_memory = ( QS_MEMORY_POOL* )dest_memory->memory;
		QS_MEMORY_POOL* server_memory_pool = (QS_MEMORY_POOL*)context->memory;
		QS_MEMORY_POOL * temporary_memory = ( QS_MEMORY_POOL* )QS_GET_POINTER( server_memory_pool, context->memid_temporary_memory );
		QS_MEMORY_POOL* router_memory = (QS_MEMORY_POOL*)context->router_memory;
		if(strlen(name)>32){
			result = -2;
			break;
		}
		char id[QS_PACKET_ROUTE_KEY_SIZE_DEFAULT+1];
		int32_t route_capacity = QS_PACKET_ROUTE_CAPACITY_DEFAULT;
		int32_t life_time = QS_PACKET_ROUTE_LIFE_TIME_SEC_DEFAULT;
		size_t con_data_size = QS_PACKET_ROUTE_CONNECTION_DATA_SIZE_DEFAULT;
		qs_uniqid_r32(id,sizeof(id)-1);
		int32_t route_offset = qs_create_packet_route(router_memory, context->memid_router, id, route_capacity, life_time, con_data_size);
		if(-1 == route_offset){
			break;
		}
		int32_t memid_route_data = qs_create_hash(temporary_memory, 32);
		if (-1 == memid_route_data) {
			break;
		}
		qs_add_hash_string(temporary_memory,memid_route_data,"name",name);
		int32_t memid_route_data_json = qs_json_encode_hash(temporary_memory, memid_route_data, SIZE_KBYTE * 4);
		if (-1 == memid_route_data_json) {
			//printf("qs_json_encode_hash error\n");
			break;
		}
		char* json = (char*)QS_GET_POINTER(temporary_memory, memid_route_data_json);
		size_t json_len = qs_strlen(json);
		if( -1 == qs_set_route_data(router_memory, context->memid_router, route_offset, (uint8_t*)json, json_len) ){
			//printf("qs_set_route_data error : %ld\n",json_len);
			break;
		}
		dest_object->memid_object = qs_get_route_info(router_memory, context->memid_router,dest_temporary_memory,route_offset);
		if(-1 == dest_object->memid_object){
			break;
		}
		dest_object->memory = (void*)dest_temporary_memory;
		//qs_hash_dump(dest_temporary_memory,dest_object->memory,0);
		result = 0;
	} while (false);
	return result;
}
int api_qs_room_list(QS_SERVER_CONTEXT* context, QS_MEMORY_CONTEXT* dest_memory, QS_JSON_ELEMENT_OBJECT* dest_object)
{
	int result = -1;
	do {
		dest_object->memory = NULL;
		dest_object->memid_object = -1;
		QS_MEMORY_POOL * temporary_memory = ( QS_MEMORY_POOL* )dest_memory->memory;
		QS_MEMORY_POOL* router_memory = (QS_MEMORY_POOL*)context->router_memory;
		dest_object->memid_object = qs_get_route_infos(router_memory,context->memid_router,temporary_memory);
		if(-1 == dest_object->memid_object){
			break;
		}
		dest_object->memory = (void*)temporary_memory;
		//qs_hash_dump(temporary_memory,dest_object->memid_object,0);
		result = 0;
	} while (false);
	return result;
}

int api_qs_room_join(QS_SERVER_CONTEXT* context, const char* room_id, const char* connection_id, QS_MEMORY_CONTEXT* dest_memory, QS_JSON_ELEMENT_OBJECT* dest_object)
{
	int result = -1;
	do {
		dest_object->memory = NULL;
		dest_object->memid_object = -1;
		QS_MEMORY_POOL * dest_temporary_memory = ( QS_MEMORY_POOL* )dest_memory->memory;
		QS_MEMORY_POOL* server_memory_pool = (QS_MEMORY_POOL*)context->memory;
		QS_SOCKET_OPTION* server = (QS_SOCKET_OPTION*)QS_GET_POINTER(server_memory_pool, context->memid_server);
		QS_MEMORY_POOL * temporary_memory = ( QS_MEMORY_POOL* )QS_GET_POINTER( server_memory_pool, context->memid_temporary_memory );
		QS_MEMORY_POOL* router_memory = (QS_MEMORY_POOL*)context->router_memory;
		int32_t connection_index = qs_find_packet_route_connection_id(router_memory,context->memid_router,(char*)connection_id);
		if( -1 == connection_index){
			//printf("qs_find_packet_route_connection_id error\n");
			break;
		}
		void* data = qs_get_packet_route_connection_chain(router_memory, context->memid_router,connection_index);
		if(data!=NULL){
			//printf("qs_get_packet_route_connection_chain error\n");
			break;
		}
		int32_t route_offset = qs_get_packet_route(router_memory, context->memid_router, (char*)room_id);
		if (-1 == route_offset) {
			//printf("qs_get_packet_route error\n");
			break;
		}
		int32_t connection_offset = qs_add_packet_route_connection(router_memory, context->memid_router, route_offset,connection_index);
		if(-1==connection_offset){
			//printf("qs_add_packet_route_connection error\n");
			break;
		}
		// set owner
		if (0==qs_get_packet_route_owner(router_memory, context->memid_router, (char*)room_id)) {
			if (-1==qs_set_packet_route_owner(router_memory, context->memid_router, (char*)room_id,connection_index)) {
				//printf("qs_set_packet_route_owner error\n");
				break;
			}
		}
		dest_object->memid_object = qs_get_route_info(router_memory, context->memid_router,dest_temporary_memory,route_offset);
		if(-1 == dest_object->memid_object){
			//printf("qs_get_route_info error\n");
			break;
		}
		dest_object->memory = dest_temporary_memory;
		// join message
		{
			int32_t message_buffer_munit = qs_create_memory_block(temporary_memory, SIZE_KBYTE * 8);
			void* buffer = QS_GET_POINTER(temporary_memory, message_buffer_munit);
			size_t buffer_size = qs_usize(temporary_memory, message_buffer_munit);
			char* connection_id = qs_get_packet_route_connection_id(router_memory, context->memid_router, connection_index);

			char* json = NULL;
			{
				int32_t memid_temp_info_hash = qs_get_route_info(router_memory,context->memid_router,temporary_memory,route_offset);
				if(-1==memid_temp_info_hash){
					break;
				}

				int32_t memid_response_body = qs_json_encode_hash(temporary_memory, memid_temp_info_hash, SIZE_KBYTE * 8);
				if (-1 == memid_response_body) {
					break;
				}
				json = (char*)QS_GET_POINTER(temporary_memory, memid_response_body);	
			}

			ssize_t sendlen = qs_make_ws_message_simple(temporary_memory, connection_id,"join",json,buffer,buffer_size);
			void* current = NULL;
			ssize_t ret = 0;
			QS_SERVER_CONNECTION_INFO *tmptinfo;
			while (NULL != (current = qs_foreach_packet_route_connection_chain(router_memory, context->memid_router, connection_index, current))) {
				QS_PACKET_ROUTE_NODE_CONNECTION* con = (QS_PACKET_ROUTE_NODE_CONNECTION*)current;
				tmptinfo = qs_offsetpointer(server->memory_pool, server->connection_munit, sizeof(QS_SERVER_CONNECTION_INFO), con->connection_index);
				if (tmptinfo->sockparam.acc != -1 && tmptinfo->sockparam.phase == QS_HTTP_SOCK_PHASE_MSG_WEBSOCKET) {
					if (-1 == (ret = qs_send_all(tmptinfo->sockparam.acc, buffer, sendlen, 0))) {
						qs_close_socket_common(server, tmptinfo, 0);
					}
				}
			}
		}
		result = 0;
	} while (false);
	return result;
}
int api_qs_http_response_json(QS_EVENT_PARAMETER params, QS_JSON_ELEMENT_OBJECT* object, size_t buffer_size)
{
	QS_MEMORY_POOL * dest_temporary_memory = ( QS_MEMORY_POOL* )object->memory;
	QS_RECV_INFO *rinfo = (QS_RECV_INFO *)params;
	QS_SERVER_CONNECTION_INFO * tinfo = (QS_SERVER_CONNECTION_INFO *)rinfo->tinfo;
	QS_SOCKET_OPTION* option = (QS_SOCKET_OPTION*)tinfo->qs_socket_option;
	//QS_SERVER_CONTEXT* context = (QS_SERVER_CONTEXT*)option->application_data;
	return http_json_response_common(tinfo,option,dest_temporary_memory,object->memid_object,buffer_size);
}

int api_qs_http_response_raw_json(QS_EVENT_PARAMETER params, QS_MEMORY_CONTEXT* temporary_memory,char* json)
{
	QS_MEMORY_POOL * dest_temporary_memory = ( QS_MEMORY_POOL* )temporary_memory->memory;
	QS_RECV_INFO *rinfo = (QS_RECV_INFO *)params;
	QS_SERVER_CONNECTION_INFO * tinfo = (QS_SERVER_CONNECTION_INFO *)rinfo->tinfo;
	QS_SOCKET_OPTION* option = (QS_SOCKET_OPTION*)tinfo->qs_socket_option;
	return http_json_response_send(tinfo,option,dest_temporary_memory,json);
}

char* api_qs_uniqid(QS_MEMORY_CONTEXT* memory_context, int32_t length)
{
	QS_MEMORY_POOL * memory = ( QS_MEMORY_POOL* )memory_context->memory;
	int32_t memid_string = qs_create_memory_block(memory, length + 1);
	if(-1==memid_string){
		return NULL;
	}
	char* string = (char*)QS_GET_POINTER(memory, memid_string);
	qs_uniqid_r32(string,length);
	return string;
}
