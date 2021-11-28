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

#ifdef __cplusplus
extern "C"{
#endif

#ifndef _QS_SCRIPT_H_
#define _QS_SCRIPT_H_

#include "qs_core.h"
#include "qs_hash.h"
#include "qs_array.h"
#include "qs_variable.h"
#include "qs_node.h"
#include "qs_json.h"
#include "qs_io.h"
#include "qs_string.h"
#include "qs_token_analyzer.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define QS_FUNCTION_TYPE_SYSTEM 1
#define QS_FUNCTION_TYPE_USER 2

typedef void* (*QS_SCRIPT_FUNCTION)( QS_MEMORY_POOL* _ppool, void* args );

typedef struct QS_SCRIPT
{
	int32_t self_munit;
	int32_t textbuf_munit;
	int32_t tokens_munit;
	int32_t rootnode_munit;
	int32_t v_hash_munit; // variable
	int32_t f_hash_munit; // function
	int32_t s_hash_munit; // stack
	int32_t return_munit;
	int32_t int_cache_munit;
	int32_t hash_alloc_size;
} QS_SCRIPT;

typedef struct QS_FUNCTION_INFO
{
	int8_t type;
	QS_SCRIPT_FUNCTION func;
	int32_t userfunction_munit;
} QS_FUNCTION_INFO;

typedef struct QS_FUNCTION_RETURN
{
	int32_t refid;
	int32_t id;
	int32_t munit;
	int32_t value;
} QS_FUNCTION_RETURN;

// token_analyzer ->  pase_code -> execute
int32_t qs_init_script( QS_MEMORY_POOL* _ppool, size_t valiablehash_size, size_t functionhash_size, int32_t init_token_size );
void qs_import_script( QS_MEMORY_POOL* _ppool, int32_t *p_unitid, char* filename );
void qs_input_script( QS_MEMORY_POOL* _ppool, int32_t *p_unitid, char* pstr );

// parse code
int qs_parse_code( QS_MEMORY_POOL* _ppool, QS_SCRIPT* pscript );
int qs_parse_code_core( QS_MEMORY_POOL* _ppool, QS_SCRIPT* pscript, QS_NODE* node, int i );
int qs_parse_symbol( QS_MEMORY_POOL* _ppool, QS_SCRIPT* pscript, QS_NODE* node, int index );
int qs_parse_function( QS_MEMORY_POOL* _ppool, QS_SCRIPT* pscript, QS_NODE* node, int index );
int qs_parse_function_args( QS_MEMORY_POOL* _ppool, QS_SCRIPT* pscript, QS_NODE* node, int index );
int qs_parse_array_index( QS_MEMORY_POOL* _ppool, QS_SCRIPT* pscript, QS_NODE* node, int index );
int qs_parse_if( QS_MEMORY_POOL* _ppool, QS_SCRIPT* pscript, QS_NODE* node, int index );
int qs_parse_while( QS_MEMORY_POOL* _ppool, QS_SCRIPT* pscript, QS_NODE* node, int index );
int qs_parse_block( QS_MEMORY_POOL* _ppool, QS_SCRIPT* pscript, QS_NODE* node, int index );
int qs_parse_array( QS_MEMORY_POOL* _ppool, QS_SCRIPT* pscript, QS_NODE* node, QS_TOKEN* pt, int index );
int qs_parse_rel( QS_MEMORY_POOL* _ppool, QS_SCRIPT* pscript, QS_NODE* node, QS_TOKEN* pt, int index );
int qs_parse_cmp( QS_MEMORY_POOL* _ppool, QS_SCRIPT* pscript, QS_NODE* node, QS_TOKEN* pt, int index );
int qs_parse_expr( QS_MEMORY_POOL* _ppool, QS_SCRIPT* pscript, QS_NODE* node, QS_TOKEN* pt, int index );
int qs_parse_addsub( QS_MEMORY_POOL* _ppool, QS_SCRIPT* pscript, QS_NODE* node, QS_TOKEN* pt, int index );
int qs_parse_muldiv( QS_MEMORY_POOL* _ppool, QS_SCRIPT* pscript, QS_NODE* node, QS_TOKEN* pt, int index );

// execute
void qs_exec( QS_MEMORY_POOL* _ppool, int32_t *p_unitid );
int32_t qs_exec_core( QS_MEMORY_POOL* _ppool, QS_SCRIPT *pscript, QS_NODE* node );
int32_t qs_exec_function( QS_MEMORY_POOL* _ppool, QS_SCRIPT *pscript, QS_NODE* node );
int32_t qs_exec_array_set( QS_MEMORY_POOL* _ppool, QS_SCRIPT *pscript, QS_NODE* node );

QS_FUNCTION_RETURN* qs_exec_get_array_offset(QS_MEMORY_POOL* _ppool, QS_SCRIPT *pscript, QS_NODE* right_node, int i);

int32_t qs_exec_array_create( QS_MEMORY_POOL* _ppool, QS_SCRIPT *pscript, QS_NODE* node );
int32_t qs_exec_array_get( QS_MEMORY_POOL* _ppool, QS_SCRIPT *pscript, QS_NODE* node );
int32_t qs_exec_expr( QS_MEMORY_POOL* _ppool, QS_SCRIPT *pscript, QS_NODE* node, int32_t result_cache_munit );
int32_t qs_exec_if( QS_MEMORY_POOL* _ppool, QS_SCRIPT *pscript, QS_NODE* node );
int32_t qs_exec_while( QS_MEMORY_POOL* _ppool, QS_SCRIPT *pscript, QS_NODE* node );

int32_t qs_create_return( QS_MEMORY_POOL* _ppool, QS_SCRIPT *pscript, int32_t data_munit, int32_t id );

// function
int qs_add_system_function( QS_MEMORY_POOL* _ppool, int32_t munit, char* functionname, QS_SCRIPT_FUNCTION func, int32_t id );
int qs_add_user_function( QS_MEMORY_POOL* _ppool, int32_t munit, char* functionname, int32_t function_munit, int32_t id );

// system function
void* qs_script_system_function_echo( QS_MEMORY_POOL* _ppool, void* args );
void* qs_script_system_function_count( QS_MEMORY_POOL* _ppool, void* args );
void* qs_script_system_function_file_exist( QS_MEMORY_POOL* _ppool, void* args );
void* qs_script_system_function_file_size( QS_MEMORY_POOL* _ppool, void* args );
void* qs_script_system_function_file_extension( QS_MEMORY_POOL* _ppool, void* args );
void* qs_script_system_function_file_get( QS_MEMORY_POOL* _ppool, void* args );
void* qs_script_system_function_file_put( QS_MEMORY_POOL* _ppool, void* args );
void* qs_script_system_function_file_add( QS_MEMORY_POOL* _ppool, void* args );
void* qs_script_system_function_json_encode( QS_MEMORY_POOL* _ppool, void* args );
void* qs_script_system_function_json_decode( QS_MEMORY_POOL* _ppool, void* args );
void* qs_script_system_function_gmtime( QS_MEMORY_POOL* _ppool, void* args );

int32_t qs_init_http_script( QS_MEMORY_POOL* _ppool, const char* script_file, const char* ini_json_file );
int32_t qs_add_http_request( QS_MEMORY_POOL* _ppool, int32_t script_munit, char* arg, int32_t header_munit, int32_t get_parameter_munit, int32_t post_parameter_munit );

#endif /*_QS_SCRIPT_H_*/

#ifdef __cplusplus
}
#endif
