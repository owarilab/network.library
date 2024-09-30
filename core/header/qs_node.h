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
