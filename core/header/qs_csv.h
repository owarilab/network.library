/*
 * Copyright (c) Katsuya Owari
 */

#ifdef __cplusplus
extern "C"{
#endif

#ifndef _QS_CSV_H_
#define _QS_CSV_H_

#include "qs_core.h"
#include "qs_string.h"
#include "qs_token_analyzer.h"
#include "qs_array.h"
#include "qs_io.h"

typedef struct QS_CSV
{
	int32_t memid_csv_array;
} QS_CSV;

int32_t qs_csv_file_load(QS_MEMORY_POOL* memory, const char * file_name);
int32_t qs_csv_parse(QS_MEMORY_POOL* memory, const char * src_csv);
int32_t qs_csv_get_line_length(QS_MEMORY_POOL* memory, int32_t memid_csv);
int32_t qs_csv_get_row_length(QS_MEMORY_POOL* memory, int32_t memid_csv, int32_t line_pos);
char* qs_csv_get_row(QS_MEMORY_POOL* memory, int32_t memid_csv, int32_t line_pos, int32_t row_pos);
char* qs_csv_build_csv(QS_MEMORY_POOL* memory, int32_t memid_csv, size_t buffer_size);
int32_t qs_csv_init(QS_MEMORY_POOL* memory);
int32_t qs_csv_add_row(QS_MEMORY_POOL* memory, int32_t memid_csv, int32_t line_pos, const char* row);
int32_t qs_csv_add_line(QS_MEMORY_POOL* memory, int32_t memid_csv, int32_t memid_array);

#endif /*_QS_CSV_H_*/

#ifdef __cplusplus
}
#endif
