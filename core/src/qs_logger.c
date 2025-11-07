/*
 * Copyright (c) Katsuya Owari
 */

#include "qs_logger.h"

int32_t qs_make_log_rotate_file_path(char* dest, size_t dest_size,char* base_path,char* file_name_prefix, time_t offset)
{
	char* path = dest;
	size_t path_buffer_len = dest_size;
	time_t nowtime = time(NULL) - offset;
	struct tm t;
	char date[20];
	qs_localtime(&t, &nowtime);
	strftime( date, sizeof(date), "%Y%m%d", &t );
	size_t path_len = 0;
	path_len = qs_strlink( path, 0, base_path, qs_strlen(base_path), path_buffer_len );
	path_len = qs_strlink( path, path_len, file_name_prefix, qs_strlen(file_name_prefix), path_buffer_len );
	path_len = qs_strlink( path, path_len, "_", 1, path_buffer_len );
	path_len = qs_strlink( path, path_len, date, qs_strlen(date), path_buffer_len );
	path_len = qs_strlink( path, path_len, ".log", 4, path_buffer_len );
	path[path_len] = '\0';
	return QS_SYSTEM_OK;
}

int32_t qs_log_rotate(QS_FILE_INFO* log_file_info,char* base_path,char* file_name_prefix, time_t offset)
{
	char path_buffer[MAXPATHLEN];
	memset(path_buffer,0,sizeof(path_buffer));
	// base_path : "./" , file_name_prefix : "access" , offset : 0
	qs_make_log_rotate_file_path(path_buffer, sizeof(path_buffer), base_path, file_name_prefix, offset);
	if (strcmp(log_file_info->path, path_buffer)) {
		qs_fclose(log_file_info);
		if (QS_SYSTEM_ERROR == qs_fopen(path_buffer, "a", log_file_info)) {
			return QS_SYSTEM_ERROR;
		}
	}
	return QS_SYSTEM_OK;
}

int32_t qs_log_open(QS_FILE_INFO* log_file_info,const char* file_path)
{
	if (strcmp(log_file_info->path, file_path)) {
		qs_fclose(log_file_info);
		if (QS_SYSTEM_ERROR == qs_fopen((char*)file_path, "a", log_file_info)) {
			return QS_SYSTEM_ERROR;
		}
	}
	return QS_SYSTEM_OK;
}

int32_t qs_log_close(QS_FILE_INFO* log_file_info)
{
	return qs_fclose(log_file_info);
}

int32_t qs_log_output(QS_FILE_INFO* log_file_info,char* log)
{
	if(log_file_info->f!=NULL){
		qs_fwrite2(log_file_info,log,qs_strlen(log));
		fflush(log_file_info->f);
	}
	return QS_SYSTEM_OK;
}
