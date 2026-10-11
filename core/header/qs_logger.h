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
#include <stdarg.h>

#ifdef __GNUC__
#define QS_LOG_PRINTF_CHECK(fmt_index, arg_index) __attribute__((format(printf, fmt_index, arg_index)))
#else
#define QS_LOG_PRINTF_CHECK(fmt_index, arg_index)
#endif

typedef enum QS_LOG_LEVEL
{
	QS_LOG_LEVEL_DEBUG = 0,
	QS_LOG_LEVEL_INFO,
	QS_LOG_LEVEL_WARN,
	QS_LOG_LEVEL_ERROR,
	QS_LOG_LEVEL_NONE,			// ロガーの閾値に指定すると全出力を抑制
} QS_LOG_LEVEL;

typedef struct QS_LOGGER
{
	QS_FILE_INFO file_info;		// 出力先（未オープンなら stdout）
	int32_t level;				// この値以上のレベルのみ出力
} QS_LOGGER;

int32_t qs_make_log_rotate_file_path(char* dest, size_t dest_size,char* base_path,char* file_name_prefix, time_t offset);
int32_t qs_log_rotate(QS_FILE_INFO* log_file_info,char* base_path,char* file_name_prefix, time_t offset);
int32_t qs_log_open(QS_FILE_INFO* log_file_info,const char* file_path);
int32_t qs_log_close(QS_FILE_INFO* log_file_info);
int32_t qs_log_output(QS_FILE_INFO* log_file_info,char* log);

int32_t qs_logger_init(QS_LOGGER* logger, int32_t level);
void qs_logger_set_level(QS_LOGGER* logger, int32_t level);

// 共通関数。file が NULL のときは位置情報を出さない
int32_t qs_log_vwrite(QS_LOGGER* logger, int32_t level, const char* file, int32_t line, const char* format, va_list ap) QS_LOG_PRINTF_CHECK(5, 0);
int32_t qs_log_write(QS_LOGGER* logger, int32_t level, const char* file, int32_t line, const char* format, ...) QS_LOG_PRINTF_CHECK(5, 6);

// 関数版（位置情報なし）
int32_t qs_log_debug(QS_LOGGER* logger, const char* format, ...) QS_LOG_PRINTF_CHECK(2, 3);
int32_t qs_log_info(QS_LOGGER* logger, const char* format, ...) QS_LOG_PRINTF_CHECK(2, 3);
int32_t qs_log_warn(QS_LOGGER* logger, const char* format, ...) QS_LOG_PRINTF_CHECK(2, 3);
int32_t qs_log_error(QS_LOGGER* logger, const char* format, ...) QS_LOG_PRINTF_CHECK(2, 3);

// マクロ版（__FILE__ / __LINE__ を付与）
#define QS_LOG_DEBUG(logger, ...) qs_log_write(logger, QS_LOG_LEVEL_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define QS_LOG_INFO(logger, ...)  qs_log_write(logger, QS_LOG_LEVEL_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define QS_LOG_WARN(logger, ...)  qs_log_write(logger, QS_LOG_LEVEL_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define QS_LOG_ERROR(logger, ...) qs_log_write(logger, QS_LOG_LEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__)

#endif /*_QS_LOGGER_H_*/

#ifdef __cplusplus
}
#endif
