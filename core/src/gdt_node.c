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

#include "gdt_node.h"

int32_t gdt_createrootnode( GDT_MEMORY_POOL* _ppool )
{
	int32_t rootnode_munit = -1;
	GDT_NODE *rootnode = NULL;
	if( -1 == ( rootnode_munit = gdt_create_munit( _ppool, sizeof( GDT_NODE ), MEMORY_TYPE_DEFAULT ) ) ){
		return rootnode_munit;
	}
	rootnode = (GDT_NODE*)GDT_POINTER( _ppool, rootnode_munit );
	rootnode->id                = ELEMENT_ROOT;
	rootnode->element_munit     = -1;
	rootnode->element_munit     = -1;
	rootnode->pos               = 0;
	rootnode->elmsize           = 0;
	rootnode->exec_tmp_munit    = -1;
	rootnode->exec_tmp_id       = -1;
	return rootnode_munit;
}

int32_t gdt_addnodeelement( GDT_MEMORY_POOL* _ppool, GDT_NODE* node )
{
	int32_t element_munit;
	GDT_NODE* childnode;
	if( -1 == ( element_munit = gdt_create_munit( _ppool, sizeof( GDT_NODE ), MEMORY_TYPE_DEFAULT ) ) ){
		return element_munit;
	}
	childnode = (GDT_NODE*)GDT_POINTER( _ppool, element_munit );
	childnode->id             = ELEMENT_CHILD_ARRAY;
	childnode->element_munit  = -1;
	childnode->pos            = 0;
	childnode->elmsize        = 0;
	childnode->exec_tmp_munit = -1;
	childnode->exec_tmp_id = -1;
	gdt_addelement( _ppool, node, ELEMENT_CHILD, element_munit );
	return element_munit;
}

int32_t gdt_addelement( GDT_MEMORY_POOL* _ppool, GDT_NODE* node, int id, int32_t data_munit )
{
	int allocsize = 8;
	int32_t tmpmunit;
	GDT_NODE* tmplist1, *tmplist2;
	GDT_NODE* workelemlist = NULL;
	int i;
	if( node->element_munit == -1 ){
		if( -1 == ( node->element_munit = gdt_create_munit( _ppool, sizeof( GDT_NODE ) * allocsize, MEMORY_TYPE_DEFAULT ) ) ){
			return -1;
		}
		node->elmsize = allocsize;
		workelemlist = ( GDT_NODE* )GDT_POINTER( _ppool, node->element_munit );
		for( i = 0; i < allocsize; i++ ){
			workelemlist[i].element_munit  = -1;
			workelemlist[i].exec_tmp_munit = -1;
			workelemlist[i].exec_tmp_id    = -1;
		}
	}
	else{
		if( node->pos >= node->elmsize ){
			tmplist1 = ( GDT_NODE* )GDT_POINTER( _ppool, node->element_munit );
			if( -1 == ( tmpmunit = gdt_create_munit( _ppool, sizeof( GDT_NODE ) * ( node->elmsize + allocsize ), MEMORY_TYPE_DEFAULT ) ) ){
				return -1;
			}
			tmplist2 = ( GDT_NODE* )GDT_POINTER( _ppool, tmpmunit );
			memcpy( tmplist2, tmplist1, sizeof( GDT_NODE ) * node->elmsize );
			workelemlist = ( GDT_NODE* )GDT_POINTER( _ppool, tmpmunit );
			for( i = node->elmsize; i < node->elmsize+allocsize; i++ ){
				workelemlist[i].element_munit  = -1;
				workelemlist[i].exec_tmp_munit = -1;
				workelemlist[i].exec_tmp_id    = -1;
			}
			gdt_free_memory_unit( _ppool, &node->element_munit );
			node->element_munit = tmpmunit;
			node->elmsize += allocsize;
		}
		else{
			workelemlist = ( GDT_NODE* )GDT_POINTER( _ppool, node->element_munit );
		}
	}
	workelemlist[node->pos].id = id;
	workelemlist[node->pos].element_munit = data_munit;
	node->pos++;
	return node->element_munit;
}

void gdt_elementdump( GDT_MEMORY_POOL* _ppool, GDT_NODE* node )
{
	printf("-----------------------------------------------------------------------\n");
	printf("element dump\n");
	printf("-----------------------------------------------------------------------\n");
	gdt_elementdumpchild( _ppool, node, 0 );
	printf("-----------------------------------------------------------------------\n");
}

void gdt_elementdumpchild( GDT_MEMORY_POOL* _ppool, GDT_NODE* node, int32_t index )
{
	int i,j;
	GDT_NODE* childnode;
	GDT_NODE* workelemlist;
	do{
		if( node->element_munit == -1 )
		{
			printf("[debug] gdt_elementdumpchild node->element_munit == -1[%d]\n",node->id);
			break;
		}
		workelemlist = ( GDT_NODE* )GDT_POINTER( _ppool, node->element_munit );
		for( j=0;j<index;j++ ){ printf("  "); }
		printf("element_munit : %d\n", node->element_munit );
		if( workelemlist == NULL )
		{
			printf("[debug] gdt_elementdumpchild workelemlist == NULL\n");
			break;
		}
		for( i = 0; i < node->pos; i++ )
		{
			if( workelemlist[i].id == ELEMENT_CHILD ){
				for( j=0;j<index;j++ ){ printf("  "); }
				printf("find child, element_munit : %d\n", workelemlist[i].element_munit);
				childnode = (GDT_NODE*)GDT_POINTER( _ppool, workelemlist[i].element_munit );
				gdt_elementdumpchild( _ppool, childnode, index+1 );
			}else{
				for( j=0;j<index;j++ ){ printf("  "); }
				if( workelemlist[i].id == ELEMENT_ARRAY){
					printf("element[%d][%d] : array object\n" , workelemlist[i].element_munit, workelemlist[i].id );
					//gdt_array_dump( _ppool, workelemlist[i].element_munit, index );
				}
				else if( workelemlist[i].id == ELEMENT_HASH ){
					printf("element[%d][%d] : hash object\n" , workelemlist[i].element_munit, workelemlist[i].id );
					//gdt_dump_hash( _ppool, workelemlist[i].element_munit, index );
				}
				else{
					printf("element[%d][%d] : %s\n" , workelemlist[i].element_munit, workelemlist[i].id, (char*)GDT_POINTER( _ppool, workelemlist[i].element_munit ) );
				}
			}
		}
	}while( false );
}

