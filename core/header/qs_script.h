/*
 * Copyright (c) Katsuya Owari
 */

#ifdef __cplusplus
extern "C"{
#endif

#ifndef _QS_SCRIPT_H_
#define _QS_SCRIPT_H_

#include "qs_core.h"
#include "qs_random.h"
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
void qs_import_script( QS_MEMORY_POOL* _ppool, int32_t* p_unitid, char* filename );
void qs_input_script( QS_MEMORY_POOL* _ppool, int32_t* p_unitid, char* pstr );

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
void qs_exec( QS_MEMORY_POOL* _ppool, int32_t* p_unitid );
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

void qs_set_return_string( QS_MEMORY_POOL* _ppool, int32_t memid_return, int32_t memid_value_string, const char* result);
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
void* qs_script_system_function_rand( QS_MEMORY_POOL* _ppool, void* args );

#endif /*_QS_SCRIPT_H_*/

#ifdef __cplusplus
}
#endif
