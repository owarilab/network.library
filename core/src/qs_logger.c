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
	}else{
		printf("%s",log);
	}
	return QS_SYSTEM_OK;
}

static const char* qs_log_level_name[] = { "DEBUG", "INFO", "WARN", "ERROR" };

int32_t qs_logger_init(QS_LOGGER* logger, int32_t level)
{
	qs_finit(&logger->file_info);
	logger->level = level;
	return QS_SYSTEM_OK;
}

void qs_logger_set_level(QS_LOGGER* logger, int32_t level)
{
	logger->level = level;
}

int32_t qs_log_vwrite(QS_LOGGER* logger, int32_t level, const char* file, int32_t line, const char* format, va_list ap)
{
	char log_buffer[SIZE_KBYTE*4];
	char date_buffer[32];
	const char* file_name = file;
	const char* p;
	time_t nowtime;
	struct tm t;
	size_t log_len;
	if( level < 0 || level >= QS_LOG_LEVEL_NONE || level < logger->level ){
		return QS_SYSTEM_OK;
	}
	nowtime = time(NULL);
	qs_localtime(&t, &nowtime);
	strftime( date_buffer, sizeof(date_buffer), "%Y-%m-%d %H:%M:%S", &t );
	snprintf( log_buffer, sizeof(log_buffer), "[%s] [%s] ", date_buffer, qs_log_level_name[level] );
	if( file != NULL ){
		for( p = file; *p != '\0'; ++p ){
			if( *p == '/' || *p == '\\' ){ file_name = p + 1; }
		}
		log_len = qs_strlen(log_buffer);
		snprintf( log_buffer + log_len, sizeof(log_buffer) - log_len, "[%s:%d] ", file_name, line );
	}
	log_len = qs_strlen(log_buffer);
	vsnprintf( log_buffer + log_len, sizeof(log_buffer) - log_len, format, ap );
	log_len = qs_strlen(log_buffer);
	if( log_len >= sizeof(log_buffer) - 1 ){
		log_len = sizeof(log_buffer) - 2;
	}
	log_buffer[log_len++] = '\n';
	log_buffer[log_len] = '\0';
	if( logger->file_info.f == NULL ){
		// qs_log_output は stdout 時に改行を足すため、ここでは直接出す
		fputs( log_buffer, stdout );
		return QS_SYSTEM_OK;
	}
	return qs_log_output(&logger->file_info, log_buffer);
}

int32_t qs_log_write(QS_LOGGER* logger, int32_t level, const char* file, int32_t line, const char* format, ...)
{
	int32_t result;
	va_list ap;
	va_start(ap, format);
	result = qs_log_vwrite(logger, level, file, line, format, ap);
	va_end(ap);
	return result;
}

int32_t qs_log_debug(QS_LOGGER* logger, const char* format, ...)
{
	int32_t result;
	va_list ap;
	va_start(ap, format);
	result = qs_log_vwrite(logger, QS_LOG_LEVEL_DEBUG, NULL, 0, format, ap);
	va_end(ap);
	return result;
}

int32_t qs_log_info(QS_LOGGER* logger, const char* format, ...)
{
	int32_t result;
	va_list ap;
	va_start(ap, format);
	result = qs_log_vwrite(logger, QS_LOG_LEVEL_INFO, NULL, 0, format, ap);
	va_end(ap);
	return result;
}

int32_t qs_log_warn(QS_LOGGER* logger, const char* format, ...)
{
	int32_t result;
	va_list ap;
	va_start(ap, format);
	result = qs_log_vwrite(logger, QS_LOG_LEVEL_WARN, NULL, 0, format, ap);
	va_end(ap);
	return result;
}

int32_t qs_log_error(QS_LOGGER* logger, const char* format, ...)
{
	int32_t result;
	va_list ap;
	va_start(ap, format);
	result = qs_log_vwrite(logger, QS_LOG_LEVEL_ERROR, NULL, 0, format, ap);
	va_end(ap);
	return result;
}
