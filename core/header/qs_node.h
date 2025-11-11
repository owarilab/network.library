/*
 * Copyright (c) Katsuya Owari
 */

#ifdef __cplusplus
extern "C"{
#endif

#ifndef _QS_NODE_H_
#define _QS_NODE_H_

#include "qs_core.h"
#include "qs_type.h"
#include "qs_memory_allocator.h"
#include "qs_hash.h"
#include "qs_array.h"

#define QS_NODE_SIZE 16
#define QS_NODE_RESIZE_QUANTITY 4

typedef struct QS_NODE
{
	int id;
	int32_t element_munit; // QS_NODE( array )
	int pos;
	size_t elmsize;
	int32_t exec_tmp_munit;
	int32_t exec_tmp_id;
} QS_NODE;

// node tree
int32_t qs_createrootnode( QS_MEMORY_POOL* _ppool );
int32_t qs_addnodeelement( QS_MEMORY_POOL* _ppool, QS_NODE* node );
int32_t qs_addhashnodeelement( QS_MEMORY_POOL* _ppool, QS_NODE* node );
int32_t qs_addelement( QS_MEMORY_POOL* _ppool, QS_NODE* node, int id, int32_t data_munit );
int32_t qs_addhashelement( QS_MEMORY_POOL* _ppool, QS_NODE* node, int id, int32_t data_munit );
void qs_elementdump( QS_MEMORY_POOL* _ppool, QS_NODE* node );
void qs_elementdumpchild( QS_MEMORY_POOL* _ppool, QS_NODE* node, int32_t index );

#endif /*_QS_NODE_H_*/

#ifdef __cplusplus
}
#endif
