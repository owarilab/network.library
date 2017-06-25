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

#ifndef _GDT_NODE_H_
#define _GDT_NODE_H_

#include "gdt_core.h"
#include "gdt_memory_allocator.h"
#include "gdt_hash.h"
#include "gdt_array.h"

// base elements
#define ELEMENT_NONE				0
#define ELEMENT_ROOT				1
#define ELEMENT_CHILD				2
#define ELEMENT_CHILD_ARRAY			3
#define ELEMENT_CHILD_HASH			4

// additional elements
#define ELEMENT_LITERAL_NUM			5
#define ELEMENT_LITERAL_FLOAT		6
#define ELEMENT_LITERAL_STR			7
#define ELEMENT_CMP					8
#define ELEMENT_FUNCTION			9
#define ELEMENT_VALIABLE			10
#define ELEMENT_IF					11
#define ELEMENT_ELSEIF				12
#define ELEMENT_ELSE				13
#define ELEMENT_ASSIGNMENT			14
#define ELEMENT_OP					15
#define ELEMENT_LOOP				16
#define ELEMENT_WHILE				17
#define ELEMENT_OP_LITERAL_MINUS	18
#define ELEMENT_RETURN				19
#define ELEMENT_ARRAY				20
#define ELEMENT_ARRAY_REFERENCE		21
#define ELEMENT_HASH				22
#define ELEMENT_HASH_OP				23
#define ELEMENT_LITERAL_BIN			24
#define ELEMENT_QUEUE				25

typedef struct GDT_NODE
{
	int id;
	int32_t element_munit; // GDT_NODE( array )
	int pos;
	size_t elmsize;
	int32_t exec_tmp_munit;
	int32_t exec_tmp_id;
} GDT_NODE;

// node tree
int32_t gdt_createrootnode( GDT_MEMORY_POOL* _ppool );
int32_t gdt_addnodeelement( GDT_MEMORY_POOL* _ppool, GDT_NODE* node );
int32_t gdt_addhashnodeelement( GDT_MEMORY_POOL* _ppool, GDT_NODE* node );
int32_t gdt_addelement( GDT_MEMORY_POOL* _ppool, GDT_NODE* node, int id, int32_t data_munit );
int32_t gdt_addhashelement( GDT_MEMORY_POOL* _ppool, GDT_NODE* node, int id, int32_t data_munit );
void gdt_elementdump( GDT_MEMORY_POOL* _ppool, GDT_NODE* node );
void gdt_elementdumpchild( GDT_MEMORY_POOL* _ppool, GDT_NODE* node, int32_t index );

#endif /*_GDT_NODE_H_*/

#ifdef __cplusplus
}
#endif
