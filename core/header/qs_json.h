/*
 * Copyright (c) Katsuya Owari
 */

#ifdef __cplusplus
extern "C"{
#endif

#ifndef _QS_JSON_H_
#define _QS_JSON_H_

#include "qs_core.h"
#include "qs_hash.h"
#include "qs_array.h"
#include "qs_node.h"
#include "qs_token_analyzer.h"
#include "qs_string.h"
#include <stdio.h>
#include <string.h>

int32_t qs_json_encode( QS_MEMORY_POOL* _ppool, QS_NODE* node, size_t buf_size );
int32_t qs_json_encode_b( QS_MEMORY_POOL* _ppool, QS_NODE* node, int32_t buf_munit );
int32_t qs_json_encode_hash( QS_MEMORY_POOL* _ppool, int32_t memid_hash, size_t buffer_size );
int32_t qs_json_encode_array( QS_MEMORY_POOL* _ppool, int32_t memid_hash, size_t buffer_size );
int32_t qs_json_encode_parser_hash( QS_MEMORY_POOL* _ppool, int32_t buf_munit, int32_t h_munit );
int32_t qs_json_encode_parser_array( QS_MEMORY_POOL* _ppool, int32_t buf_munit, int32_t a_munit );
int qs_add_json_element( QS_MEMORY_POOL* _ppool, int32_t buf_munit, char* src, size_t src_size, uint8_t escape );
QS_NODE* qs_get_json_root( QS_MEMORY_POOL* _ppool, int32_t json_root_munit );
int32_t qs_get_json_root_hash(QS_MEMORY_POOL* _ppool, int32_t json_root_munit);
int32_t qs_json_decode( QS_MEMORY_POOL* _ppool, const char* src );
int32_t qs_json_decode_h( QS_MEMORY_POOL* _ppool, const char* src, int32_t hash_size, int32_t init_token_size );
int32_t qs_json_decode_parser( QS_MEMORY_POOL* _ppool, QS_NODE* node, QS_TOKENS *ptokens, QS_TOKEN *token_list, int32_t hash_size );
int32_t qs_json_decode_parser_hash( QS_MEMORY_POOL* _ppool, QS_NODE* node, QS_TOKENS *ptokens, QS_TOKEN *token_list, int32_t hash_size, int index );
int32_t qs_json_decode_parser_array( QS_MEMORY_POOL* _ppool, QS_NODE* node, QS_TOKENS *ptokens, QS_TOKEN *token_list, int index );
int32_t qs_make_json_root( QS_MEMORY_POOL* _ppool, int32_t data_munit, int id ); 

#endif /*_QS_JSON_H_*/

#ifdef __cplusplus
}
#endif
