/*
 * Copyright (c) Katsuya Owari
 */

#ifdef __cplusplus
extern "C"{
#endif

#ifndef _QS_LOGGER_H_
#define _QS_LOGGER_H_

#include "qs_system.h"
#include "qs_memory_allocator.h"
#include "qs_io.h"

int32_t qs_make_log_rotate_file_path(char* dest, size_t dest_size,char* base_path,char* file_name_prefix, time_t offset);
int32_t qs_log_rotate(QS_FILE_INFO* log_file_info,char* base_path,char* file_name_prefix, time_t offset);
int32_t qs_log_open(QS_FILE_INFO* log_file_info,const char* file_path);
int32_t qs_log_close(QS_FILE_INFO* log_file_info);
int32_t qs_log_output(QS_FILE_INFO* log_file_info,char* log);

#endif /*_QS_LOGGER_H_*/

#ifdef __cplusplus
}
#endif
