/*
 * Copyright (c) Katsuya Owari
 */

#include "qs_node.h"

int32_t qs_createrootnode( QS_MEMORY_POOL* _ppool )
{
	int32_t rootnode_munit = -1;
	QS_NODE *rootnode = NULL;
	if( -1 == ( rootnode_munit = qs_create_munit( _ppool, sizeof( QS_NODE ), MEMORY_TYPE_DEFAULT ) ) ){
		return rootnode_munit;
	}
	rootnode = (QS_NODE*)QS_GET_POINTER( _ppool, rootnode_munit );
	rootnode->id                = ELEMENT_ROOT;
	rootnode->element_munit     = -1;
	rootnode->element_munit     = -1;
	rootnode->pos               = 0;
	rootnode->elmsize           = 0;
	rootnode->exec_tmp_munit    = -1;
	rootnode->exec_tmp_id       = -1;
	return rootnode_munit;
}

int32_t qs_addnodeelement( QS_MEMORY_POOL* _ppool, QS_NODE* node )
{
	int32_t element_munit;
	QS_NODE* childnode;
	if( -1 == ( element_munit = qs_create_munit( _ppool, sizeof( QS_NODE ), MEMORY_TYPE_DEFAULT ) ) ){
		return element_munit;
	}
	childnode = (QS_NODE*)QS_GET_POINTER( _ppool, element_munit );
	childnode->id             = ELEMENT_CHILD_ARRAY;
	childnode->element_munit  = -1;
	childnode->pos            = 0;
	childnode->elmsize        = 0;
	childnode->exec_tmp_munit = -1;
	childnode->exec_tmp_id = -1;
	qs_addelement( _ppool, node, ELEMENT_CHILD, element_munit );
	return element_munit;
}

int32_t qs_addelement( QS_MEMORY_POOL* _ppool, QS_NODE* node, int id, int32_t data_munit )
{
	size_t allocsize = QS_NODE_SIZE;
	int32_t tmpmunit;
	QS_NODE* tmplist1, *tmplist2;
	QS_NODE* workelemlist = NULL;
	size_t i;
	if( node->element_munit == -1 ){
		if( -1 == ( node->element_munit = qs_create_munit( _ppool, sizeof( QS_NODE ) * allocsize, MEMORY_TYPE_DEFAULT ) ) ){
			return -1;
		}
		node->elmsize = allocsize;
		workelemlist = ( QS_NODE* )QS_GET_POINTER( _ppool, node->element_munit );
		for( i = 0; i < allocsize; i++ ){
			workelemlist[i].element_munit  = -1;
			workelemlist[i].exec_tmp_munit = -1;
			workelemlist[i].exec_tmp_id    = -1;
		}
	}
	else{
		if( node->pos >= node->elmsize ){
			size_t resize_size = node->elmsize * QS_NODE_RESIZE_QUANTITY;
			tmplist1 = ( QS_NODE* )QS_GET_POINTER( _ppool, node->element_munit );
			if( -1 == ( tmpmunit = qs_create_munit( _ppool, sizeof( QS_NODE ) * ( resize_size ), MEMORY_TYPE_DEFAULT ) ) ){
				return -1;
			}
			tmplist2 = ( QS_NODE* )QS_GET_POINTER( _ppool, tmpmunit );
			memcpy( tmplist2, tmplist1, sizeof( QS_NODE ) * node->elmsize );
			workelemlist = ( QS_NODE* )QS_GET_POINTER( _ppool, tmpmunit );
			for( i = node->elmsize; i < resize_size; i++ ){
				workelemlist[i].element_munit  = -1;
				workelemlist[i].exec_tmp_munit = -1;
				workelemlist[i].exec_tmp_id    = -1;
			}
			qs_free_memory_unit( _ppool, &node->element_munit );
			node->element_munit = tmpmunit;
			node->elmsize = resize_size;
		}
		else{
			workelemlist = ( QS_NODE* )QS_GET_POINTER( _ppool, node->element_munit );
		}
	}
	workelemlist[node->pos].id = id;
	workelemlist[node->pos].element_munit = data_munit;
	node->pos++;
	return node->element_munit;
}

void qs_elementdump( QS_MEMORY_POOL* _ppool, QS_NODE* node )
{
	printf("-----------------------------------------------------------------------\n");
	printf("element dump\n");
	printf("-----------------------------------------------------------------------\n");
	qs_elementdumpchild( _ppool, node, 0 );
	printf("-----------------------------------------------------------------------\n");
}

void qs_elementdumpchild( QS_MEMORY_POOL* _ppool, QS_NODE* node, int32_t index )
{
	int i,j;
	QS_NODE* childnode;
	QS_NODE* workelemlist;
	do{
		if( node->element_munit == -1 )
		{
			printf("[debug] qs_elementdumpchild node->element_munit == -1[%d]\n",node->id);
			break;
		}
		workelemlist = ( QS_NODE* )QS_GET_POINTER( _ppool, node->element_munit );
		for( j=0;j<index;j++ ){ printf("  "); }
		printf("element_munit : %d\n", node->element_munit );
		if( workelemlist == NULL )
		{
			printf("[debug] qs_elementdumpchild workelemlist == NULL\n");
			break;
		}
		for( i = 0; i < node->pos; i++ )
		{
			if( workelemlist[i].id == ELEMENT_CHILD ){
				for( j=0;j<index;j++ ){ printf("  "); }
				printf("find child, element_munit : %d\n", workelemlist[i].element_munit);
				childnode = (QS_NODE*)QS_GET_POINTER( _ppool, workelemlist[i].element_munit );
				qs_elementdumpchild( _ppool, childnode, index+1 );
			}else{
				for( j=0;j<index;j++ ){ printf("  "); }
				if( workelemlist[i].id == ELEMENT_ARRAY){
					printf("element[%d][%d] : array object\n" , workelemlist[i].element_munit, workelemlist[i].id );
				}
				else if( workelemlist[i].id == ELEMENT_HASH ){
					printf("element[%d][%d] : hash object\n" , workelemlist[i].element_munit, workelemlist[i].id );
				}
				else{
					printf("element[%d][%d] : %s\n" , workelemlist[i].element_munit, workelemlist[i].id, (char*)QS_GET_POINTER( _ppool, workelemlist[i].element_munit ) );
				}
			}
		}
	}while( false );
}

