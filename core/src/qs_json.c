/*
 * Copyright (c) Katsuya Owari
 */

#include "qs_json.h"

int32_t qs_json_encode( QS_MEMORY_POOL* _ppool, QS_NODE* node, size_t buf_size )
{
	int32_t buf_munit = -1;
	int i;
	QS_NODE* childnode;
	QS_NODE* workelemlist;
	do{
		if( -1 == ( buf_munit = qs_create_memory_block( _ppool, sizeof( char ) * buf_size ) ) ){
			break;
		}
		if( node->element_munit == -1 )
		{
			//printf("[debug] qs_elementdumpchild node->element_munit == -1[%d]\n",node->id);
			break;
		}
		workelemlist = ( QS_NODE* )QS_GET_POINTER( _ppool, node->element_munit );
		if( workelemlist == NULL )
		{
			//printf("[debug] qs_elementdumpchild workelemlist == NULL\n");
			break;
		}
		for( i = 0; i < node->pos; i++ )
		{
			if( workelemlist[i].id == ELEMENT_CHILD ){
				childnode = (QS_NODE*)QS_GET_POINTER( _ppool, workelemlist[i].element_munit );
				qs_json_encode( _ppool, childnode, buf_size );
			}else{
				if( workelemlist[i].id == ELEMENT_ARRAY){
					qs_json_encode_parser_array( _ppool, buf_munit, workelemlist[i].element_munit );
				}
				else if( workelemlist[i].id == ELEMENT_HASH ){
					qs_json_encode_parser_hash( _ppool, buf_munit, workelemlist[i].element_munit );
				}
				else{
					//printf("invalid element[%d][%d] : %s\n" , workelemlist[i].element_munit, workelemlist[i].id, (char*)QS_GET_POINTER( _ppool, workelemlist[i].element_munit ) );
				}
			}
		}
		qs_add_json_element(_ppool, buf_munit, "\0", 1, 0);
	}while( false );
	return buf_munit;
}

int32_t qs_json_encode_b( QS_MEMORY_POOL* _ppool, QS_NODE* node, int32_t buf_munit )
{
	int i;
	QS_NODE* childnode;
	QS_NODE* workelemlist;
	do{
		if( buf_munit == -1 )
		{
			//printf("[debug] null buffer\n");
			break;
		}
		if( node->element_munit == -1 )
		{
			//printf("[debug] qs_elementdumpchild node->element_munit == -1[%d]\n",node->id);
			break;
		}
		workelemlist = ( QS_NODE* )QS_GET_POINTER( _ppool, node->element_munit );
		if( workelemlist == NULL )
		{
			//printf("[debug] qs_elementdumpchild workelemlist == NULL\n");
			break;
		}
		for( i = 0; i < node->pos; i++ )
		{
			if( workelemlist[i].id == ELEMENT_CHILD ){
				childnode = (QS_NODE*)QS_GET_POINTER( _ppool, workelemlist[i].element_munit );
				qs_json_encode_b( _ppool, childnode, buf_munit );
			}else{
				if( workelemlist[i].id == ELEMENT_ARRAY){
					qs_json_encode_parser_array( _ppool, buf_munit, workelemlist[i].element_munit );
				}
				else if( workelemlist[i].id == ELEMENT_HASH ){
					qs_json_encode_parser_hash( _ppool, buf_munit, workelemlist[i].element_munit );
				}
				else{
					//printf("invalid element[%d][%d] : %s\n" , workelemlist[i].element_munit, workelemlist[i].id, (char*)QS_GET_POINTER( _ppool, workelemlist[i].element_munit ) );
				}
			}
		}
		qs_add_json_element(_ppool, buf_munit, "\0", 1, 0);
	}while( false );
	return buf_munit;
}

int32_t qs_json_encode_hash( QS_MEMORY_POOL* _ppool, int32_t memid_hash, size_t buffer_size )
{
	int32_t memid_node = qs_make_json_root(_ppool,memid_hash,ELEMENT_HASH);
	if(-1==memid_node){
		return -1;
	}
	return qs_json_encode( _ppool, (QS_NODE*)QS_GET_POINTER(_ppool, memid_node), buffer_size );
}

int32_t qs_json_encode_array( QS_MEMORY_POOL* _ppool, int32_t memid_hash, size_t buffer_size )
{
	int32_t memid_node = qs_make_json_root(_ppool,memid_hash,ELEMENT_ARRAY);
	if(-1==memid_node){
		return -1;
	}
	return qs_json_encode( _ppool, (QS_NODE*)QS_GET_POINTER(_ppool, memid_node), buffer_size );
}

int32_t qs_json_encode_parser_hash( QS_MEMORY_POOL* _ppool, int32_t buf_munit, int32_t h_munit )
{
	struct QS_HASH *hash;
	struct QS_HASH *hashchild;
	struct QS_HASH_ELEMENT *hashelement;
	uint32_t i,j;
	uint32_t cnt = 0;
	do{
		hash = (struct QS_HASH *)QS_GET_POINTER( _ppool, h_munit );
		hashchild = (struct QS_HASH *)QS_GET_POINTER( _ppool, hash->hash_munit );
		qs_add_json_element( _ppool, buf_munit, "{", 1, 0 );
		for( j = 0; j < hash->hash_size; j++ )
		{
			if( hashchild[j].hash_munit > 0 )
			{
				hashelement = (struct QS_HASH_ELEMENT*)QS_GET_POINTER( _ppool, hashchild[j].hash_munit );
				for( i = 0; i < hashchild[j].hash_size; i++ )
				{
					if( hashelement[i].hashname_munit > 0 )
					{
						if( hashelement[i].id==ELEMENT_HASH){
							if( cnt > 0 ){ 
								qs_add_json_element( _ppool, buf_munit, ",", 1, 0);
							}
							qs_add_json_element( _ppool, buf_munit, "\"", 1, 0);
							qs_add_json_element( _ppool, buf_munit, (char*)QS_GET_POINTER( _ppool, hashelement[i].hashname_munit ), qs_strlen((char*)QS_GET_POINTER( _ppool, hashelement[i].hashname_munit )), 1 );
							qs_add_json_element( _ppool, buf_munit, "\":", 2, 0);
							qs_json_encode_parser_hash( _ppool, buf_munit, hashelement[i].elm_munit );
							cnt++;
						}
						else if( hashelement[i].id==ELEMENT_ARRAY){
							if( cnt > 0 ){ 
								qs_add_json_element( _ppool, buf_munit, ",", 1, 0);
							}
							qs_add_json_element( _ppool, buf_munit, "\"", 1, 0);
							qs_add_json_element( _ppool, buf_munit, (char*)QS_GET_POINTER( _ppool, hashelement[i].hashname_munit ), qs_strlen((char*)QS_GET_POINTER( _ppool, hashelement[i].hashname_munit )), 1 );
							qs_add_json_element( _ppool, buf_munit, "\":", 2, 0);
							qs_json_encode_parser_array( _ppool, buf_munit, hashelement[i].elm_munit );
							cnt++;
						}
						else{
							if( cnt > 0 ){ 
								qs_add_json_element( _ppool, buf_munit, ",", 1, 0);
							}
							if( hashelement[i].id == ELEMENT_LITERAL_STR ){
								qs_add_json_element( _ppool, buf_munit, "\"", 1, 0);
								qs_add_json_element( _ppool, buf_munit, (char*)QS_GET_POINTER( _ppool, hashelement[i].hashname_munit ), qs_strlen((char*)QS_GET_POINTER( _ppool, hashelement[i].hashname_munit )), 1 );
								qs_add_json_element( _ppool, buf_munit, "\":", 2, 0);
								qs_add_json_element( _ppool, buf_munit, "\"", 1, 0);
								qs_add_json_element( _ppool, buf_munit, (char*)QS_GET_POINTER( _ppool, hashelement[i].elm_munit ), qs_strlen((char*)QS_GET_POINTER( _ppool, hashelement[i].elm_munit )), 1 );
								qs_add_json_element( _ppool, buf_munit, "\"", 1, 0);
								cnt++;
							}
							else if( hashelement[i].id == ELEMENT_LITERAL_NUM ){
								qs_add_json_element( _ppool, buf_munit, "\"", 1, 0);
								qs_add_json_element( _ppool, buf_munit, (char*)QS_GET_POINTER( _ppool, hashelement[i].hashname_munit ), qs_strlen((char*)QS_GET_POINTER( _ppool, hashelement[i].hashname_munit )), 1 );
								qs_add_json_element( _ppool, buf_munit, "\":", 2, 0);
								qs_add_json_element( _ppool, buf_munit, (char*)QS_GET_POINTER( _ppool, hashelement[i].elm_munit ), qs_strlen((char*)QS_GET_POINTER( _ppool, hashelement[i].elm_munit )), 1 );
								cnt++;
							}
							else{
								qs_add_json_element( _ppool, buf_munit, "\"", 1, 0);
								qs_add_json_element( _ppool, buf_munit, (char*)QS_GET_POINTER( _ppool, hashelement[i].hashname_munit ), qs_strlen((char*)QS_GET_POINTER( _ppool, hashelement[i].hashname_munit )), 1 );
								qs_add_json_element( _ppool, buf_munit, "\":", 2, 0);
								qs_add_json_element( _ppool, buf_munit, "\"", 1, 0);
								qs_add_json_element( _ppool, buf_munit, "NULL", 4, 0 );
								qs_add_json_element( _ppool, buf_munit, "\"", 1, 0);
							}
						}
					}
				}
			}
		}
		qs_add_json_element( _ppool, buf_munit, "}", 1, 0 );
	}while( false );
	return 0;
}

int32_t qs_json_encode_parser_array( QS_MEMORY_POOL* _ppool, int32_t buf_munit, int32_t a_munit )
{
	QS_ARRAY* parray;
	QS_ARRAY_ELEMENT* elm;
	int i;
	do{
		qs_add_json_element( _ppool, buf_munit, "[", 1, 0 );
		if( a_munit > 1 ){
			parray = (QS_ARRAY*)QS_GET_POINTER( _ppool, a_munit );
			elm = (QS_ARRAY_ELEMENT*)QS_GET_POINTER( _ppool, parray->munit );
			for( i = 0; i < parray->len; i++ )
			{
				if( (elm+i)->id == ELEMENT_LITERAL_NUM ){
					if( i > 0 ){ 
						qs_add_json_element( _ppool, buf_munit, ",", 1, 0);
					}
					qs_add_json_element( _ppool, buf_munit, (char*)QS_GET_POINTER( _ppool, (elm+i)->munit ), qs_strlen((char*)QS_GET_POINTER( _ppool, (elm+i)->munit)), 1 );
				}
				if( (elm+i)->id == ELEMENT_LITERAL_STR ){
					if( i > 0 ){ 
						qs_add_json_element( _ppool, buf_munit, ",", 1, 0);
					}
					qs_add_json_element( _ppool, buf_munit, "\"", 1, 0);
					qs_add_json_element( _ppool, buf_munit, (char*)QS_GET_POINTER( _ppool, (elm+i)->munit ), qs_strlen((char*)QS_GET_POINTER( _ppool, (elm+i)->munit)), 1 );
					qs_add_json_element( _ppool, buf_munit, "\"", 1, 0);
				}
				else if( (elm+i)->id == ELEMENT_ARRAY ){
					if( i > 0 ){ 
						qs_add_json_element( _ppool, buf_munit, ",", 1, 0);
					}
					qs_json_encode_parser_array( _ppool, buf_munit, (elm+i)->munit);
				}
				else if( (elm+i)->id==ELEMENT_HASH){
					if( i > 0 ){ 
						qs_add_json_element( _ppool, buf_munit, ",", 1, 0);
					}
					qs_json_encode_parser_hash( _ppool, buf_munit, (elm+i)->munit);
				}
			}
		}
		qs_add_json_element( _ppool, buf_munit, "]", 1, 0);
	}while( false );
	return 0;
}

int qs_add_json_element( QS_MEMORY_POOL* _ppool, int32_t buf_munit, char* src, size_t src_size, uint8_t escape)
{
	do{
		if( buf_munit <= 0 ){
			//printf("null buf_munit\n");
			break;
		}
		if( src_size == -1 ){
			//printf("src_size == -1\n");
			return 0;
		}
		if( src_size == 0 ){
			return 0;
		}
		QS_MEMORY_UNIT* punit = qs_get_munit( _ppool, buf_munit );
		uint32_t buf_size = punit->size - (punit->top - punit->p)-1;
		if( src_size > buf_size ){
			//printf("buffer out of range %d: %d\n", (int)( src_size + ( punit->top - punit->p ) ), (int)( punit->size ) );
			break;
		}
		char* pbuf = (char*)QS_GET_POINTER( _ppool, buf_munit ) + ( punit->top - punit->p );
		if (escape != 0)
		{
			int i;
			uint32_t addlen = 0;
			for (i = 0; i < src_size; i++) {
				switch (*(src + i))
				{
				case '"':
					*(pbuf++) = '\\';
					*(pbuf++) = '"';
					addlen += 2;
					break;
				case '\'':
					*(pbuf++) = '\\';
					*(pbuf++) = '\'';
					addlen += 2;
					break;
				case '\a':
					*(pbuf++) = '\\';
					*(pbuf++) = 'a';
					addlen += 2;
					break;
				case '\b':
					*(pbuf++) = '\\';
					*(pbuf++) = 'b';
					addlen += 2;
					break;
				case '\n':
					*(pbuf++) = '\\';
					*(pbuf++) = 'n';
					addlen += 2;
					break;
				case '\r':
					*(pbuf++) = '\\';
					*(pbuf++) = 'r';
					addlen += 2;
					break;
				case '\f':
					*(pbuf++) = '\\';
					*(pbuf++) = 'f';
					addlen += 2;
					break;
				case '\t':
					*(pbuf++) = '\\';
					*(pbuf++) = 't';
					addlen += 2;
					break;
				case '\v':
					*(pbuf++) = '\\';
					*(pbuf++) = 'v';
					addlen += 2;
					break;
				case '\\':
					*(pbuf++) = '\\';
					*(pbuf++) = '\\';
					addlen += 2;
					break;
				default:
					*(pbuf++) = *(src + i);
					addlen++;
					break;
				}
				if (addlen >= buf_size) {
					//printf("buffer out of range\n");
					break;
				}
			}
			punit->top += addlen;
		}
		else {
			memcpy( pbuf, src, src_size );
			punit->top += src_size;
		}
	}while( false );
	return 0;
}

QS_NODE* qs_get_json_root( QS_MEMORY_POOL* _ppool, int32_t json_root_munit )
{
	if( -1 == json_root_munit ){
		return NULL;
	}
	QS_NODE* rootnode = (QS_NODE*)QS_GET_POINTER(_ppool,json_root_munit);
	if(rootnode->pos==0){
		return NULL;
	}
	QS_NODE* workelemlist = ( QS_NODE* )QS_GET_POINTER( _ppool, rootnode->element_munit );
	return &workelemlist[0];
}

int32_t qs_get_json_root_hash(QS_MEMORY_POOL* _ppool, int32_t json_root_munit)
{
	if (-1 == json_root_munit) {
		return -1;
	}
	QS_NODE* rootnode = (QS_NODE*)QS_GET_POINTER(_ppool, json_root_munit);
	if (rootnode->pos == 0) {
		return -1;
	}
	QS_NODE* workelemlist = (QS_NODE*)QS_GET_POINTER(_ppool, rootnode->element_munit);
	if (workelemlist[0].id != ELEMENT_HASH) {
		return -1;
	}
	return workelemlist[0].element_munit;
}

int32_t qs_json_decode( QS_MEMORY_POOL* _ppool, const char* src )
{
	int32_t rootnode_munit = -1;
	int32_t tokens_munit = -1;
	if( -1 == ( tokens_munit = qs_inittoken( _ppool, 10000, QS_TOKEN_READ_BUFFER_SIZE_MIN ) ) ){
		return -1;
	}
	qs_token_analyzer( _ppool, tokens_munit, (char*)src );
	QS_TOKENS *ptokens = (QS_TOKENS*)QS_GET_POINTER(_ppool,tokens_munit);
	if( ptokens->token_munit == -1 ){
		return -1;
	}
	QS_TOKEN *token_list = (QS_TOKEN*)QS_GET_POINTER(_ppool,ptokens->token_munit);
	rootnode_munit = qs_createrootnode( _ppool );
	QS_NODE *rootnode = (QS_NODE*)QS_GET_POINTER( _ppool, rootnode_munit );
	if(0!=qs_json_decode_parser( _ppool, rootnode, ptokens, token_list, 128 )){
		return -1;
	}
	//qs_tokendump(_ppool,tokens_munit);
	return rootnode_munit;
}

int32_t qs_json_decode_h( QS_MEMORY_POOL* _ppool, const char* src, int32_t hash_size, int32_t init_token_size )
{
	int32_t rootnode_munit = -1;
	int32_t tokens_munit = -1;
	if( -1 == ( tokens_munit = qs_inittoken( _ppool, init_token_size, QS_TOKEN_READ_BUFFER_SIZE_MIN ) ) ){
		return -1;
	}
	qs_token_analyzer( _ppool, tokens_munit, (char*)src );
	QS_TOKENS *ptokens = (QS_TOKENS*)QS_GET_POINTER(_ppool,tokens_munit);
	QS_TOKEN *token_list = (QS_TOKEN*)QS_GET_POINTER(_ppool,ptokens->token_munit);
	rootnode_munit = qs_createrootnode( _ppool );
	QS_NODE *rootnode = (QS_NODE*)QS_GET_POINTER( _ppool, rootnode_munit );
	//qs_tokendump(_ppool,tokens_munit);
	if(0!=qs_json_decode_parser( _ppool, rootnode, ptokens, token_list, hash_size )){
		return -1;
	}
	return rootnode_munit;
}

int32_t qs_json_decode_parser( QS_MEMORY_POOL* _ppool, QS_NODE* node, QS_TOKENS *ptokens, QS_TOKEN *token_list, int32_t hash_size )
{
	int32_t error = 0;
	if( ptokens->workpos < ptokens->currentpos )
	{
		do{
			if( token_list[ptokens->workpos].type == ID_SIGN ){
				if( *((char*)QS_GET_POINTER(_ppool,token_list[ptokens->workpos].buf_munit)) == '{' ){
					node->id = ELEMENT_HASH;
					ptokens->workpos++;
					int32_t parser_child_munit = -1;
					if( -1 == ( parser_child_munit = qs_json_decode_parser_hash( _ppool, node, ptokens, token_list, hash_size, 0 ) ) ){
						error = 1;
						break;
					}
					qs_addelement( _ppool, node, ELEMENT_HASH, parser_child_munit );
				}
				else if( *((char*)QS_GET_POINTER(_ppool,token_list[ptokens->workpos].buf_munit)) == '[' ){
					node->id = ELEMENT_ARRAY;
					ptokens->workpos++;
					int32_t parser_child_munit = -1;
					if( -1 == ( parser_child_munit = qs_json_decode_parser_array( _ppool, node, ptokens, token_list, 0 ) ) ){
						error = 1;
						break;
					}
					qs_addelement( _ppool, node, ELEMENT_ARRAY, parser_child_munit );
				}
				else{
					error = 1;
					break;
				}
			}else{
				error = 1;
				break;
			}
		}while(false);
	}
	return error;
}

int32_t qs_json_decode_parser_hash( QS_MEMORY_POOL* _ppool, QS_NODE* node, QS_TOKENS *ptokens, QS_TOKEN *token_list, int32_t hash_size, int index )
{
	int32_t working_munit = -1;
	int finish = 0;
	while( ptokens->workpos < ptokens->currentpos )
	{
		if( token_list[ptokens->workpos].type == ID_STR ) {
			if( -1 == working_munit ){
				if( -1 == ( working_munit = qs_create_hash( _ppool, hash_size ) ) ){
					//printf( "create_hash is failed\n" );
					break;
				}
			}
			if( ptokens->workpos+2 >= ptokens->currentpos ){
				//printf( "invalid json format1\n" );
				working_munit = -1;
				break;
			}
			if( token_list[ptokens->workpos+1].type != ID_SIGN || *((char*)QS_GET_POINTER(_ppool,token_list[ptokens->workpos+1].buf_munit)) != ':' ){
				//printf( "invalid json format2, %d, %s\n", ptokens->workpos, (char*)QS_GET_POINTER(_ppool,token_list[ptokens->workpos+1].buf_munit) );
				working_munit = -1;
				break;
			}
			if( token_list[ptokens->workpos+2].type == ID_STR ){
				qs_add_hash( _ppool, working_munit, token_list[ptokens->workpos].buf_munit, token_list[ptokens->workpos+2].buf_munit, ELEMENT_LITERAL_STR );
				ptokens->workpos+=2;
			}
			else if( token_list[ptokens->workpos+2].type == ID_NUM ){
				qs_add_hash_integer_kint(_ppool,working_munit,token_list[ptokens->workpos].buf_munit,atoi((char*)QS_GET_POINTER(_ppool,token_list[ptokens->workpos+2].buf_munit)));
				//qs_add_hash( _ppool, working_munit, token_list[ptokens->workpos].buf_munit, token_list[ptokens->workpos+2].buf_munit, ELEMENT_LITERAL_STR );
				ptokens->workpos+=2;
			}
			else if( token_list[ptokens->workpos+2].type == ID_SIGN ){
				if( *((char*)QS_GET_POINTER(_ppool,token_list[ptokens->workpos+2].buf_munit)) == '{' ){
					int32_t parser_child_munit = -1;
					int32_t hash_name_munit = token_list[ptokens->workpos].buf_munit;
					ptokens->workpos+=3;
					if( -1 == ( parser_child_munit = qs_json_decode_parser_hash( _ppool, node, ptokens, token_list, 8, index+1 ) ) ){
						working_munit = -1;
						break;
					}
					qs_add_hash( _ppool, working_munit, hash_name_munit, parser_child_munit, ELEMENT_HASH );
					if( ptokens->workpos+1 < ptokens->currentpos ){
						if( *((char*)QS_GET_POINTER(_ppool,token_list[ptokens->workpos+1].buf_munit)) != '}' && *((char*)QS_GET_POINTER(_ppool,token_list[ptokens->workpos+1].buf_munit)) != ',' ){
							//printf( "invalid json hash : %c\n", *((char*)QS_GET_POINTER(_ppool,token_list[ptokens->workpos+1].buf_munit)) );
							working_munit = -1;
							break;
						}
					}
				}
				else if( *((char*)QS_GET_POINTER(_ppool,token_list[ptokens->workpos+2].buf_munit)) == '[' ){
					int32_t parser_child_munit = -1;
					int32_t hash_name_munit = token_list[ptokens->workpos].buf_munit;
					ptokens->workpos+=3;
					if( -1 == ( parser_child_munit = qs_json_decode_parser_array( _ppool, node, ptokens, token_list, index+1 ) ) ){
						working_munit = -1;
						break;
					}
					qs_add_hash( _ppool, working_munit, hash_name_munit, parser_child_munit, ELEMENT_ARRAY );
					if( ptokens->workpos+1 < ptokens->currentpos ){
						if( *((char*)QS_GET_POINTER(_ppool,token_list[ptokens->workpos+1].buf_munit)) != '}' && *((char*)QS_GET_POINTER(_ppool,token_list[ptokens->workpos+1].buf_munit)) != ',' ){
							//printf( "invalid json hash : %c\n", *((char*)QS_GET_POINTER(_ppool,token_list[ptokens->workpos+1].buf_munit)) );
							working_munit = -1;
							break;
						}
					}
				}
			}
		}
		else if( token_list[ptokens->workpos].type == ID_SIGN ){
			if( *((char*)QS_GET_POINTER(_ppool,token_list[ptokens->workpos].buf_munit)) == '}' ){
				finish = 1;
				break;
			}
			else if( *((char*)QS_GET_POINTER(_ppool,token_list[ptokens->workpos].buf_munit)) == ',' ){
			}
		}
		else{
			//printf("invalid token type:%d , %s\n",token_list[ptokens->workpos].type, (char*)QS_GET_POINTER(_ppool,token_list[ptokens->workpos].buf_munit));
			working_munit = -1;
			break;
		}
		ptokens->workpos++;
	}
	if(finish==0){
		working_munit = -1;
	}
	return working_munit;
}

int32_t qs_json_decode_parser_array( QS_MEMORY_POOL* _ppool, QS_NODE* node, QS_TOKENS *ptokens, QS_TOKEN *token_list, int index )
{
	int32_t working_munit = -1;
	int finish = 0;
	while( ptokens->workpos < ptokens->currentpos )
	{
		if( token_list[ptokens->workpos].type == ID_STR ) {
			qs_array_push( _ppool, &working_munit, ELEMENT_LITERAL_STR, token_list[ptokens->workpos].buf_munit );
		}
		else if( token_list[ptokens->workpos].type == ID_NUM ) {
			qs_array_push( _ppool, &working_munit, ELEMENT_LITERAL_NUM, token_list[ptokens->workpos].buf_munit );
		}
		else if( token_list[ptokens->workpos].type == ID_FLOAT ) {
			qs_array_push( _ppool, &working_munit, ELEMENT_LITERAL_FLOAT, token_list[ptokens->workpos].buf_munit );
		}
		else if( token_list[ptokens->workpos].type == ID_SIGN ){
			if( *((char*)QS_GET_POINTER(_ppool,token_list[ptokens->workpos].buf_munit)) == ']' ){
				finish = 1;
				if(working_munit==-1){
					working_munit = qs_create_array( _ppool, 8, NUMERIC_BUFFER_SIZE );
				}
				break;
			}
			else if( *((char*)QS_GET_POINTER(_ppool,token_list[ptokens->workpos].buf_munit)) == ',' ){
			}
			else if( *((char*)QS_GET_POINTER(_ppool,token_list[ptokens->workpos].buf_munit)) == '{' ){
				int32_t parser_child_munit = -1;
				ptokens->workpos++;
				if( -1 == ( parser_child_munit = qs_json_decode_parser_hash( _ppool, node, ptokens, token_list, 4, index+1 ) ) ){
					working_munit = -1;
					break;
				}
				qs_array_push( _ppool, &working_munit, ELEMENT_HASH, parser_child_munit );
				if( ptokens->workpos+1 < ptokens->currentpos ){
					if( *((char*)QS_GET_POINTER(_ppool,token_list[ptokens->workpos+1].buf_munit)) != ']' && *((char*)QS_GET_POINTER(_ppool,token_list[ptokens->workpos+1].buf_munit)) != ',' ){
						//printf( "json array error : %c\n", *((char*)QS_GET_POINTER(_ppool,token_list[ptokens->workpos+1].buf_munit)) );
						working_munit = -1;
						break;
					}
				}
			}
			else if( *((char*)QS_GET_POINTER(_ppool,token_list[ptokens->workpos].buf_munit)) == '[' ){
				int32_t parser_child_munit = -1;
				ptokens->workpos++;
				if( -1 == ( parser_child_munit = qs_json_decode_parser_array( _ppool, node, ptokens, token_list, index+1 ) ) ){
					working_munit = -1;
					break;
				}
				qs_array_push( _ppool, &working_munit, ELEMENT_ARRAY, parser_child_munit );
				if( ptokens->workpos+1 < ptokens->currentpos ){
					if( *((char*)QS_GET_POINTER(_ppool,token_list[ptokens->workpos+1].buf_munit)) != ']' && *((char*)QS_GET_POINTER(_ppool,token_list[ptokens->workpos+1].buf_munit)) != ',' ){
						//printf( "json array error : %c\n", *((char*)QS_GET_POINTER(_ppool,token_list[ptokens->workpos+1].buf_munit)) );
						working_munit = -1;
						break;
					}
				}
			}
			else{
				working_munit = -1;
				//printf("invalid token type:%d\n",token_list[ptokens->workpos].type);
				break;
			}
		}
		else{
			working_munit = -1;
			//printf("invalid token type:%d\n",token_list[ptokens->workpos].type);
			break;
		}
		ptokens->workpos++;
	}
	if(finish==0){
		working_munit = -1;
	}
	return working_munit;
}

int32_t qs_make_json_root( QS_MEMORY_POOL* _ppool, int32_t data_munit, int id )
{
	int32_t rootnode_munit = -1;
	do{
		rootnode_munit = qs_createrootnode( _ppool );
		QS_NODE *rootnode = (QS_NODE*)QS_GET_POINTER( _ppool, rootnode_munit );
		rootnode->id = id;
		qs_addelement( _ppool, rootnode, id, data_munit );
	}while( false );
	return rootnode_munit;
}
