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

#include "qs_csv.h"

int32_t qs_csv_file_load(QS_MEMORY_POOL* memory, const char * file_name)
{
	QS_FILE_INFO info;
	if(-1==qs_fget_info((char*)file_name,&info)){
		return -1;
	}
	
	int32_t memid_read_buffer = qs_create_memory_block(memory,info.size+1);
	if(-1==memid_read_buffer){
		return -1;
	}
	char* buffer = (char*)QS_GET_POINTER(memory,memid_read_buffer);
	if(0==qs_fread((char*)file_name,buffer,qs_usize(memory,memid_read_buffer))){
		return -1;
	}
	return qs_csv_parse(memory,buffer);
}
int32_t qs_csv_parse(QS_MEMORY_POOL* memory, const char * src_csv)
{
	int32_t memid_csv = qs_create_memory_block(memory,sizeof(QS_CSV));
	if(-1==memid_csv){
		return -1;
	}
	QS_CSV* csv = (QS_CSV*)QS_GET_POINTER(memory,memid_csv);
	csv->memid_csv_array = -1;
	int32_t memid_tokens = -1;
	if( -1 == ( memid_tokens = qs_inittoken( memory, 1000, QS_TOKEN_READ_BUFFER_SIZE_MIN ) ) ){
		return -1;
	}
	QS_TOKENS *tokens = (QS_TOKENS*)QS_GET_POINTER(memory,memid_tokens);
	tokens->enable_newline = 1;
	if(0!=qs_token_analyzer(memory,memid_tokens,(char*)src_csv)){
		return -1;
	}
	if( tokens->token_munit == -1 ){
		return -1;
	}
	QS_TOKEN *token_list = (QS_TOKEN*)QS_GET_POINTER(memory,tokens->token_munit);
	int32_t memid_current_array = -1;
	int i;
	int is_empty = 0;
	for( i = 0; i < tokens->currentpos; i++ )
	{
		if(token_list[i].type==ID_SYS_NEWLINE){
			if(-1==qs_array_push(memory,&csv->memid_csv_array,ELEMENT_ARRAY,memid_current_array)){
				return -1;
			}
			memid_current_array = -1;
			is_empty = 0;
			continue;
		}
		if(token_list[i].type==ID_SIGN){
			if(is_empty==1){
				if(-1==qs_array_push_string(memory,&memid_current_array,"")){
					return -1;
				}
			}
			if(!strcmp((char*)QS_GET_POINTER(memory,token_list[i].buf_munit),",")){
				is_empty = 1;
				continue;
			}
		}
		if(-1==qs_array_push_string(memory,&memid_current_array,(char*)QS_GET_POINTER(memory,token_list[i].buf_munit))){
			return -1;
		}
		is_empty = 0;
	}
	if(memid_current_array!=-1){
		if(-1==qs_array_push(memory,&csv->memid_csv_array,ELEMENT_ARRAY,memid_current_array)){
			return -1;
		}
		memid_current_array = -1;
	}
	return memid_csv;
}

int32_t qs_csv_get_line_length(QS_MEMORY_POOL* memory, int32_t memid_csv)
{
	if(-1==memid_csv){
		return -1;
	}
	QS_CSV* csv = (QS_CSV*)QS_GET_POINTER(memory,memid_csv);
	return qs_array_length(memory,csv->memid_csv_array);
}

int32_t qs_csv_get_row_length(QS_MEMORY_POOL* memory, int32_t memid_csv, int32_t line_pos)
{
	if(-1==memid_csv){
		return -1;
	}
	QS_CSV* csv = (QS_CSV*)QS_GET_POINTER(memory,memid_csv);
	QS_ARRAY_ELEMENT* elm = qs_array_get(memory,csv->memid_csv_array,line_pos);
	if(NULL==elm){
		return -1;
	}
	return qs_array_length(memory,elm->munit);
}
char* qs_csv_get_row(QS_MEMORY_POOL* memory, int32_t memid_csv, int32_t line_pos, int32_t row_pos)
{
	if(-1==memid_csv){
		return NULL;
	}
	QS_CSV* csv = (QS_CSV*)QS_GET_POINTER(memory,memid_csv);
	QS_ARRAY_ELEMENT* elm = qs_array_get(memory,csv->memid_csv_array,line_pos);
	if(NULL==elm){
		return NULL;
	}
	QS_ARRAY_ELEMENT* row_elm = qs_array_get(memory,elm->munit,row_pos);
	if(NULL==row_elm){
		return NULL;
	}
	return (char*)QS_GET_POINTER(memory,row_elm->munit);
}