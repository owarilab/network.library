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
