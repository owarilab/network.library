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
