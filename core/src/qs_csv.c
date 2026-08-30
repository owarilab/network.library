/*
 * Copyright (c) Katsuya Owari
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
	int32_t memid_csv = qs_csv_init(memory);
	if(-1==memid_csv){
		return -1;
	}
	QS_CSV* csv = (QS_CSV*)QS_GET_POINTER(memory,memid_csv);
	if(NULL==src_csv){
		return -1;
	}
	int32_t memid_field = qs_create_memory_block(memory, qs_strlen(src_csv)+1);
	if(-1==memid_field){
		return -1;
	}
	char* field = (char*)QS_GET_POINTER(memory,memid_field);
	int32_t memid_current_array = -1;
	size_t field_len = 0;
	int in_quotes = 0;
	int field_started = 0;
	const char* p = src_csv;
	for(;;p++){
		char current = *p;
		if(in_quotes){
			if(current=='"'){
				if(*(p+1)=='"'){
					field[field_len++] = '"';
					p++;
				}else{
					in_quotes = 0;
				}
			}else if(current=='\0'){
				return -1;
			}else{
				field[field_len++] = current;
			}
			field_started = 1;
			continue;
		}
		if(current=='"' && !field_started && field_len==0){
			in_quotes = 1;
			field_started = 1;
			continue;
		}
		if(current==',' || current=='\n' || current=='\0'){
			if(current=='\0' && memid_current_array==-1 && field_len==0 && !field_started){
				break;
			}
			field[field_len] = '\0';
			if(-1==qs_array_push_string(memory,&memid_current_array,field)){
				return -1;
			}
			field_len = 0;
			field_started = 0;
			if(current==','){
				continue;
			}
			if(-1==qs_array_push(memory,&csv->memid_csv_array,ELEMENT_ARRAY,memid_current_array)){
				return -1;
			}
			memid_current_array = -1;
			if(current=='\0'){
				break;
			}
			continue;
		}
		if(current=='\r' && *(p+1)=='\n'){
			continue;
		}
		field[field_len++] = current;
		field_started = 1;
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
	return qs_array_length(memory,elm->memid_array_element_data);
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
	QS_ARRAY_ELEMENT* row_elm = qs_array_get(memory,elm->memid_array_element_data,row_pos);
	if(NULL==row_elm){
		return NULL;
	}
	return (char*)QS_GET_POINTER(memory,row_elm->memid_array_element_data);
}

int32_t qs_csv_build_csv_memid(QS_MEMORY_POOL* memory, int32_t memid_csv, size_t buffer_size)
{
	if(-1==memid_csv){
		return -1;
	}
	QS_CSV* csv = (QS_CSV*)QS_GET_POINTER(memory,memid_csv);
	int32_t memid_csv_string = -1;
	if(-1==(memid_csv_string = qs_create_memory_block(memory,sizeof(char) * buffer_size))){
		return -1;
	}
	char* csv_string = (char*)QS_GET_POINTER(memory,memid_csv_string);
	size_t link_len = 0;
	int32_t i;
	int32_t line_count = qs_array_length(memory,csv->memid_csv_array);
	for(i=0;i<line_count;i++){
		QS_ARRAY_ELEMENT* elm = qs_array_get(memory,csv->memid_csv_array,i);
		if(NULL==elm){
			return -1;
		}
		int32_t j;
		int32_t row_count = qs_array_length(memory,elm->memid_array_element_data);
		for(j=0;j<row_count;j++){
			QS_ARRAY_ELEMENT* row_elm = qs_array_get(memory,elm->memid_array_element_data,j);
			if(NULL==row_elm){
				return -1;
			}
			if(row_elm->memid_array_element_data < 0){
				return -1;
			}
			char* str = (char*)QS_GET_POINTER(memory,row_elm->memid_array_element_data);
			if(str == NULL){
				return -1;
			}

			// check numeric (empty string is not numeric)
			size_t str_len = qs_strlen(str);
			int is_numeric = (str_len > 0) ? 1 : 0;
			if(is_numeric){
				size_t k;
				for(k=0;k<str_len;k++){
					if(k == 0 && str[k] == '-') continue;
					if(!isdigit((unsigned char)str[k]) && str[k]!='.'){
						is_numeric = 0;
						break;
					}
				}
			}

			if(!is_numeric){
				if(link_len+2>=buffer_size){
					return -1;
				}
				link_len = qs_strlink( csv_string, link_len, "\"", 1, buffer_size );
				size_t k;
				for(k=0;k<str_len;k++){
					if(str[k]=='"'){
						if(link_len+2>=buffer_size){
							return -1;
						}
						link_len = qs_strlink( csv_string, link_len, "\"\"", 2, buffer_size );
					}else{
						if(link_len+1>=buffer_size){
							return -1;
						}
						link_len = qs_strlink( csv_string, link_len, str+k, 1, buffer_size );
					}
				}
				if(link_len+2>=buffer_size){
					return -1;
				}
				link_len = qs_strlink( csv_string, link_len, "\"", 1, buffer_size );
			}else{
				if(link_len+str_len>=buffer_size){
					return -1;
				}
				link_len = qs_strlink( csv_string, link_len, str, str_len, buffer_size );
			}
			if(j!=row_count-1){
				if(link_len+1>=buffer_size){
					return -1;
				}
				link_len = qs_strlink( csv_string, link_len, ",", 1, buffer_size );
			}
		}
		if(i!=line_count-1){
			if(link_len+1>=buffer_size){
				return -1;
			}
			link_len = qs_strlink( csv_string, link_len, "\n", 1, buffer_size );
		}
	}
	return memid_csv_string;
}

char* qs_csv_build_csv(QS_MEMORY_POOL* memory, int32_t memid_csv, size_t buffer_size)
{
	int32_t memid_csv_string = qs_csv_build_csv_memid(memory, memid_csv, buffer_size);
	if(memid_csv_string == -1){
		return NULL;
	}
	return (char*)QS_GET_POINTER(memory,memid_csv_string);
}

int32_t qs_csv_init(QS_MEMORY_POOL* memory)
{
	int32_t memid_csv;
	if(-1==(memid_csv = qs_create_memory_block(memory,sizeof(QS_CSV)))){
		return -1;
	}
	QS_CSV* csv = (QS_CSV*)QS_GET_POINTER(memory,memid_csv);
	csv->memid_csv_array = -1;
	return memid_csv;
}

int32_t qs_csv_add_row(QS_MEMORY_POOL* memory, int32_t memid_csv, int32_t line_pos, const char* row)
{
	if(-1==memid_csv){
		return -1;
	}
	QS_CSV* csv = (QS_CSV*)QS_GET_POINTER(memory,memid_csv);
	int32_t current_length = qs_array_length(memory, csv->memid_csv_array);
	if(line_pos >= current_length){
		int32_t i;
		for(i = current_length; i < line_pos; i++){
			int32_t empty_memid_array = -1;
			if(-1 == qs_array_push_string(memory, &empty_memid_array, "")){
				return -1;
			}
			if(-1 == qs_array_push(memory, &csv->memid_csv_array, ELEMENT_ARRAY, empty_memid_array)){
				return -1;
			}
		}
		int32_t memid_array = -1;
		if(-1==qs_array_push_string(memory,&memid_array,row)){
			return -1;
		}
		if(-1==qs_array_push(memory,&csv->memid_csv_array,ELEMENT_ARRAY,memid_array)){
			return -1;
		}
	}else{
		QS_ARRAY_ELEMENT* elm = qs_array_get(memory,csv->memid_csv_array,line_pos);
		if(NULL==elm){
			return -1;
		}
		if(-1==qs_array_push_string(memory,&elm->memid_array_element_data,row)){
			return -1;
		}
	}
	return QS_SYSTEM_OK;
}

int32_t qs_csv_add_line(QS_MEMORY_POOL* memory, int32_t memid_csv, int32_t memid_array)
{
	if(-1==memid_csv){
		return -1;
	}
	QS_CSV* csv = (QS_CSV*)QS_GET_POINTER(memory,memid_csv);
	if(-1==qs_array_push(memory,&csv->memid_csv_array,ELEMENT_ARRAY,memid_array)){
		return -1;
	}
	return 0;
}
