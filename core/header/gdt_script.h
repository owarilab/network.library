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

#ifndef _GDT_SCRIPT_H_
#define _GDT_SCRIPT_H_

#include "gdt_core.h"
#include "gdt_hash.h"
#include "gdt_array.h"
#include "gdt_node.h"
#include "gdt_json.h"
#include "gdt_io.h"
#include "gdt_string.h"
#include "gdt_token_analyzer.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define FUNCTION_TYPE_SYSTEM 1
#define FUNCTION_TYPE_USER 2

typedef void* (*GDT_SCRIPT_FUNCTION)( GDT_MEMORY_POOL* _ppool, void* args );

typedef struct GDT_SCRIPT
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
} GDT_SCRIPT;

typedef struct GDT_FUNCTION_INFO
{
	int8_t type;
	GDT_SCRIPT_FUNCTION func;
	int32_t userfunction_munit;
} GDT_FUNCTION_INFO;

typedef struct GDT_FUNCTION_RETURN
{
	int32_t refid;
	int32_t id;
	int32_t munit;
	int32_t value;
} GDT_FUNCTION_RETURN;

// token_analyzer ->  pase_code -> execute
int32_t gdt_init_script( GDT_MEMORY_POOL* _ppool, size_t valiablehash_size, size_t functionhash_size );
void gdt_import_script( GDT_MEMORY_POOL* _ppool, int32_t *p_unitid, char* filename );
void gdt_input_script( GDT_MEMORY_POOL* _ppool, int32_t *p_unitid, char* pstr );

// parse code
int gdt_parse_code( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT* pscript );
int gdt_parse_code_core( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT* pscript, GDT_NODE* node, int i );
int gdt_parse_symbol( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT* pscript, GDT_NODE* node, int index );
int gdt_parse_function( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT* pscript, GDT_NODE* node, int index );
int gdt_parse_function_args( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT* pscript, GDT_NODE* node, int index );
int gdt_parse_array_index( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT* pscript, GDT_NODE* node, int index );
int gdt_parse_if( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT* pscript, GDT_NODE* node, int index );
int gdt_parse_while( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT* pscript, GDT_NODE* node, int index );
int gdt_parse_block( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT* pscript, GDT_NODE* node, int index );
int gdt_parse_array( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT* pscript, GDT_NODE* node, GDT_TOKEN* pt, int index );
int gdt_parse_rel( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT* pscript, GDT_NODE* node, GDT_TOKEN* pt, int index );
int gdt_parse_cmp( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT* pscript, GDT_NODE* node, GDT_TOKEN* pt, int index );
int gdt_parse_expr( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT* pscript, GDT_NODE* node, GDT_TOKEN* pt, int index );
int gdt_parse_addsub( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT* pscript, GDT_NODE* node, GDT_TOKEN* pt, int index );
int gdt_parse_muldiv( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT* pscript, GDT_NODE* node, GDT_TOKEN* pt, int index );

// execute
void gdt_exec( GDT_MEMORY_POOL* _ppool, int32_t *p_unitid );
int32_t gdt_exec_core( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT *pscript, GDT_NODE* node );
int32_t gdt_exec_function( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT *pscript, GDT_NODE* node );
int32_t gdt_exec_array_create( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT *pscript, GDT_NODE* node );
int32_t gdt_exec_array_get( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT *pscript, GDT_NODE* node );
int32_t gdt_exec_expr( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT *pscript, GDT_NODE* node, int32_t result_cache_munit );
int32_t gdt_exec_if( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT *pscript, GDT_NODE* node );
int32_t gdt_exec_while( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT *pscript, GDT_NODE* node );

int32_t gdt_create_return( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT *pscript, int32_t data_munit, int32_t id );

// function
int gdt_add_system_function( GDT_MEMORY_POOL* _ppool, int32_t munit, char* functionname, GDT_SCRIPT_FUNCTION func, int32_t id );
int gdt_add_user_function( GDT_MEMORY_POOL* _ppool, int32_t munit, char* functionname, int32_t function_munit, int32_t id );

// system function
void* gdt_script_system_function_echo( GDT_MEMORY_POOL* _ppool, void* args );
void* gdt_script_system_function_count( GDT_MEMORY_POOL* _ppool, void* args );
void* gdt_script_system_function_file_exist( GDT_MEMORY_POOL* _ppool, void* args );
void* gdt_script_system_function_file_size( GDT_MEMORY_POOL* _ppool, void* args );
void* gdt_script_system_function_file_extension( GDT_MEMORY_POOL* _ppool, void* args );
void* gdt_script_system_function_file_get( GDT_MEMORY_POOL* _ppool, void* args );
void* gdt_script_system_function_file_put( GDT_MEMORY_POOL* _ppool, void* args );
void* gdt_script_system_function_file_add( GDT_MEMORY_POOL* _ppool, void* args );
void* gdt_script_system_function_json_encode( GDT_MEMORY_POOL* _ppool, void* args );
void* gdt_script_system_function_json_decode( GDT_MEMORY_POOL* _ppool, void* args );
void* gdt_script_system_function_gmtime( GDT_MEMORY_POOL* _ppool, void* args );

int32_t gdt_init_http_script( GDT_MEMORY_POOL* _ppool, const char* script_file, const char* ini_json_file );
int32_t gdt_add_http_request( GDT_MEMORY_POOL* _ppool, int32_t script_munit, char* arg, int32_t header_munit, int32_t get_parameter_munit, int32_t post_parameter_munit );
int32_t gdt_script_run( GDT_MEMORY_POOL* _ppool, const char* script_file, const char* ini_json_file, char* arg, int32_t header_munit, int32_t get_parameter_munit, int32_t post_parameter_munit );

#endif /*_GDT_SCRIPT_H_*/

#ifdef __cplusplus
}
#endif
