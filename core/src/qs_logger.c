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

int32_t qs_make_log_file_path(char* dest, size_t dest_size,char* base_path,char* file_name_prefix, time_t offset)
{
	char* path = dest;
	size_t path_buffer_len = dest_size;
	time_t nowtime = time(NULL) - offset;
	struct tm t;
	char date[20];
	gdt_localtime(&t, &nowtime);
	strftime( date, sizeof(date), "%Y%m%d", &t );
	size_t path_len = 0;
	path_len = gdt_strlink( path, 0, base_path, gdt_strlen(base_path), path_buffer_len );
	path_len = gdt_strlink( path, path_len, file_name_prefix, gdt_strlen(file_name_prefix), path_buffer_len );
	path_len = gdt_strlink( path, path_len, "_", 1, path_buffer_len );
	path_len = gdt_strlink( path, path_len, date, gdt_strlen(date), path_buffer_len );
	path_len = gdt_strlink( path, path_len, ".log", 4, path_buffer_len );
	path[path_len] = '\0';
	return QS_SYSTEM_OK;
}

int32_t qs_log_rotate(QS_FILE_INFO* log_file_info,char* base_path,char* file_name_prefix, time_t offset)
{
	char path_buffer[MAXPATHLEN];
	memset(path_buffer,0,sizeof(path_buffer));
	// base_path : "./" , file_name_prefix : "access" , offset : 0
	qs_make_log_file_path(path_buffer, sizeof(path_buffer), base_path, file_name_prefix, offset);
	if (strcmp(log_file_info->path, path_buffer)) {
		gdt_fclose(log_file_info);
		if (QS_SYSTEM_ERROR == gdt_fopen(path_buffer, "a", log_file_info)) {
			return QS_SYSTEM_ERROR;
		}
	}
	return QS_SYSTEM_OK;
}

int32_t qs_http_access_log(QS_FILE_INFO* log_file_info,char* http_version, char* user_agent,char* client_ip,char* method,char* request,int32_t http_status_code)
{
	char log_buffer[SIZE_KBYTE*4];
	size_t log_buffer_size = sizeof(log_buffer);
	char status_code_buffer[20];
	gdt_itoa( http_status_code, status_code_buffer, sizeof(status_code_buffer) );
	char date_buffer[128];
	struct tm t;
	time_t nowtime = time(NULL);
	gdt_localtime(&t, &nowtime);
	strftime( date_buffer, sizeof(date_buffer), "%Y-%m-%d %H:%M:%S", &t);
	size_t log_len = 0;
	log_len = gdt_strlink( log_buffer, log_len, client_ip, gdt_strlen(client_ip), log_buffer_size );
	log_len = gdt_strlink( log_buffer, log_len, " ", 1, log_buffer_size );
	log_len = gdt_strlink( log_buffer, log_len, method, gdt_strlen(method), log_buffer_size );
	log_len = gdt_strlink( log_buffer, log_len, " ", 1, log_buffer_size );
	log_len = gdt_strlink( log_buffer, log_len, http_version, gdt_strlen(http_version), log_buffer_size );
	log_len = gdt_strlink( log_buffer, log_len, " ", 1, log_buffer_size );
	log_len = gdt_strlink( log_buffer, log_len, request, gdt_strlen(request), log_buffer_size );
	log_len = gdt_strlink( log_buffer, log_len, " ", 1, log_buffer_size );
	log_len = gdt_strlink( log_buffer, log_len, status_code_buffer, gdt_strlen(status_code_buffer), log_buffer_size );
	log_len = gdt_strlink( log_buffer, log_len, " [", 2, log_buffer_size );
	log_len = gdt_strlink( log_buffer, log_len, date_buffer, gdt_strlen(date_buffer), log_buffer_size );
	log_len = gdt_strlink( log_buffer, log_len, "] ", 2, log_buffer_size );
	log_len = gdt_strlink( log_buffer, log_len, user_agent, gdt_strlen(user_agent), log_buffer_size );
	log_buffer[log_len++] = '\n';
	log_buffer[log_len] = '\0';
	if(log_file_info->f!=NULL){
		gdt_fwrite2(log_file_info,log_buffer,gdt_strlen(log_buffer));
		fflush(log_file_info->f);
	}
	return QS_SYSTEM_OK;
}
