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

#include "gdt_json.h"

int32_t gdt_json_encode( GDT_MEMORY_POOL* _ppool, GDT_NODE* node, size_t buf_size )
{
	int32_t buf_munit = -1;
	int i;
	GDT_NODE* childnode;
	GDT_NODE* workelemlist;
	do{
		if( 0 >= ( buf_munit = gdt_create_munit( _ppool, sizeof( char ) * buf_size, MEMORY_TYPE_DEFAULT ) ) ){
			break;
		}
		if( node->element_munit == -1 )
		{
			printf("[debug] gdt_elementdumpchild node->element_munit == -1[%d]\n",node->id);
			break;
		}
		workelemlist = ( GDT_NODE* )GDT_POINTER( _ppool, node->element_munit );
		if( workelemlist == NULL )
		{
			printf("[debug] gdt_elementdumpchild workelemlist == NULL\n");
			break;
		}
		for( i = 0; i < node->pos; i++ )
		{
			if( workelemlist[i].id == ELEMENT_CHILD ){
				childnode = (GDT_NODE*)GDT_POINTER( _ppool, workelemlist[i].element_munit );
				gdt_json_encode( _ppool, childnode, buf_size );
			}else{
				if( workelemlist[i].id == ELEMENT_ARRAY){
					gdt_json_encode_parser_array( _ppool, buf_munit, workelemlist[i].element_munit );
				}
				else if( workelemlist[i].id == ELEMENT_HASH ){
					gdt_json_encode_parser_hash( _ppool, buf_munit, workelemlist[i].element_munit );
				}
				else{
					printf("invalid element[%d][%d] : %s\n" , workelemlist[i].element_munit, workelemlist[i].id, (char*)GDT_POINTER( _ppool, workelemlist[i].element_munit ) );
				}
			}
		}
		gdt_add_json_element(_ppool, buf_munit, "\0", 1, 0);
	}while( false );
	return buf_munit;
}

int32_t gdt_json_encode_b( GDT_MEMORY_POOL* _ppool, GDT_NODE* node, int32_t buf_munit )
{
	int i;
	GDT_NODE* childnode;
	GDT_NODE* workelemlist;
	do{
		if( buf_munit == -1 )
		{
			printf("[debug] null buffer\n");
			break;
		}
		if( node->element_munit == -1 )
		{
			printf("[debug] gdt_elementdumpchild node->element_munit == -1[%d]\n",node->id);
			break;
		}
		workelemlist = ( GDT_NODE* )GDT_POINTER( _ppool, node->element_munit );
		if( workelemlist == NULL )
		{
			printf("[debug] gdt_elementdumpchild workelemlist == NULL\n");
			break;
		}
		for( i = 0; i < node->pos; i++ )
		{
			if( workelemlist[i].id == ELEMENT_CHILD ){
				childnode = (GDT_NODE*)GDT_POINTER( _ppool, workelemlist[i].element_munit );
				gdt_json_encode_b( _ppool, childnode, buf_munit );
			}else{
				if( workelemlist[i].id == ELEMENT_ARRAY){
					gdt_json_encode_parser_array( _ppool, buf_munit, workelemlist[i].element_munit );
				}
				else if( workelemlist[i].id == ELEMENT_HASH ){
					gdt_json_encode_parser_hash( _ppool, buf_munit, workelemlist[i].element_munit );
				}
				else{
					printf("invalid element[%d][%d] : %s\n" , workelemlist[i].element_munit, workelemlist[i].id, (char*)GDT_POINTER( _ppool, workelemlist[i].element_munit ) );
				}
			}
		}
		gdt_add_json_element(_ppool, buf_munit, "\0", 1, 0);
	}while( false );
	return buf_munit;
}

int32_t gdt_json_encode_parser_hash( GDT_MEMORY_POOL* _ppool, int32_t buf_munit, int32_t h_munit )
{
	struct GDT_HASH *hash;
	struct GDT_HASH *hashchild;
	struct GDT_HASH_ELEMENT *hashelement;
	uint32_t i,j;
	uint32_t cnt = 0;
	do{
		hash = (struct GDT_HASH *)GDT_POINTER( _ppool, h_munit );
		hashchild = (struct GDT_HASH *)GDT_POINTER( _ppool, hash->hash_munit );
		gdt_add_json_element( _ppool, buf_munit, "{", 1, 0 );
		for( j = 0; j < hash->hash_size; j++ )
		{
			if( hashchild[j].hash_munit > 0 )
			{
				hashelement = (struct GDT_HASH_ELEMENT*)GDT_POINTER( _ppool, hashchild[j].hash_munit );
				for( i = 0; i < hashchild[j].hash_size; i++ )
				{
					if( hashelement[i].hashname_munit > 0 )
					{
						if( hashelement[i].id==ELEMENT_HASH){
							if( cnt > 0 ){ 
								gdt_add_json_element( _ppool, buf_munit, ",", 1, 0);
							}
							gdt_add_json_element( _ppool, buf_munit, "\"", 1, 0);
							gdt_add_json_element( _ppool, buf_munit, (char*)GDT_POINTER( _ppool, hashelement[i].hashname_munit ), gdt_strlen((char*)GDT_POINTER( _ppool, hashelement[i].hashname_munit )), 1 );
							gdt_add_json_element( _ppool, buf_munit, "\":", 2, 0);
							gdt_json_encode_parser_hash( _ppool, buf_munit, hashelement[i].elm_munit );
							cnt++;
						}
						else if( hashelement[i].id==ELEMENT_ARRAY){
							if( cnt > 0 ){ 
								gdt_add_json_element( _ppool, buf_munit, ",", 1, 0);
							}
							gdt_add_json_element( _ppool, buf_munit, "\"", 1, 0);
							gdt_add_json_element( _ppool, buf_munit, (char*)GDT_POINTER( _ppool, hashelement[i].hashname_munit ), gdt_strlen((char*)GDT_POINTER( _ppool, hashelement[i].hashname_munit )), 1 );
							gdt_add_json_element( _ppool, buf_munit, "\":", 2, 0);
							gdt_json_encode_parser_array( _ppool, buf_munit, hashelement[i].elm_munit );
							cnt++;
						}
						else{
							if( cnt > 0 ){ 
								gdt_add_json_element( _ppool, buf_munit, ",", 1, 0);
							}
							if( hashelement[i].id == ELEMENT_LITERAL_STR ){
								gdt_add_json_element( _ppool, buf_munit, "\"", 1, 0);
								gdt_add_json_element( _ppool, buf_munit, (char*)GDT_POINTER( _ppool, hashelement[i].hashname_munit ), gdt_strlen((char*)GDT_POINTER( _ppool, hashelement[i].hashname_munit )), 1 );
								gdt_add_json_element( _ppool, buf_munit, "\":", 2, 0);
								gdt_add_json_element( _ppool, buf_munit, "\"", 1, 0);
								gdt_add_json_element( _ppool, buf_munit, (char*)GDT_POINTER( _ppool, hashelement[i].elm_munit ), gdt_strlen((char*)GDT_POINTER( _ppool, hashelement[i].elm_munit )), 1 );
								gdt_add_json_element( _ppool, buf_munit, "\"", 1, 0);
								cnt++;
							}
							else if( hashelement[i].id == ELEMENT_LITERAL_NUM ){
								gdt_add_json_element( _ppool, buf_munit, "\"", 1, 0);
								gdt_add_json_element( _ppool, buf_munit, (char*)GDT_POINTER( _ppool, hashelement[i].hashname_munit ), gdt_strlen((char*)GDT_POINTER( _ppool, hashelement[i].hashname_munit )), 1 );
								gdt_add_json_element( _ppool, buf_munit, "\":", 2, 0);
								gdt_add_json_element( _ppool, buf_munit, (char*)GDT_POINTER( _ppool, hashelement[i].elm_munit ), gdt_strlen((char*)GDT_POINTER( _ppool, hashelement[i].elm_munit )), 1 );
								cnt++;
							}
							else{
								gdt_add_json_element( _ppool, buf_munit, "\"", 1, 0);
								gdt_add_json_element( _ppool, buf_munit, (char*)GDT_POINTER( _ppool, hashelement[i].hashname_munit ), gdt_strlen((char*)GDT_POINTER( _ppool, hashelement[i].hashname_munit )), 1 );
								gdt_add_json_element( _ppool, buf_munit, "\":", 2, 0);
								gdt_add_json_element( _ppool, buf_munit, "\"", 1, 0);
								gdt_add_json_element( _ppool, buf_munit, "NULL", 4, 0 );
								gdt_add_json_element( _ppool, buf_munit, "\"", 1, 0);
							}
						}
					}
				}
			}
		}
		gdt_add_json_element( _ppool, buf_munit, "}", 1, 0 );
	}while( false );
	return 0;
}

int32_t gdt_json_encode_parser_array( GDT_MEMORY_POOL* _ppool, int32_t buf_munit, int32_t a_munit )
{
	GDT_ARRAY* parray;
	GDT_ARRAY_ELEMENT* elm;
	int i;
	do{
		gdt_add_json_element( _ppool, buf_munit, "[", 1, 0 );
		if( a_munit > 1 ){
			parray = (GDT_ARRAY*)GDT_POINTER( _ppool, a_munit );
			elm = (GDT_ARRAY_ELEMENT*)GDT_POINTER( _ppool, parray->munit );
			for( i = 0; i < parray->len; i++ )
			{
				if( (elm+i)->id == ELEMENT_LITERAL_NUM ){
					if( i > 0 ){ 
						gdt_add_json_element( _ppool, buf_munit, ",", 1, 0);
					}
					gdt_add_json_element( _ppool, buf_munit, (char*)GDT_POINTER( _ppool, (elm+i)->munit ), gdt_strlen((char*)GDT_POINTER( _ppool, (elm+i)->munit)), 1 );
				}
				if( (elm+i)->id == ELEMENT_LITERAL_STR ){
					if( i > 0 ){ 
						gdt_add_json_element( _ppool, buf_munit, ",", 1, 0);
					}
					gdt_add_json_element( _ppool, buf_munit, "\"", 1, 0);
					gdt_add_json_element( _ppool, buf_munit, (char*)GDT_POINTER( _ppool, (elm+i)->munit ), gdt_strlen((char*)GDT_POINTER( _ppool, (elm+i)->munit)), 1 );
					gdt_add_json_element( _ppool, buf_munit, "\"", 1, 0);
				}
				else if( (elm+i)->id == ELEMENT_ARRAY ){
					if( i > 0 ){ 
						gdt_add_json_element( _ppool, buf_munit, ",", 1, 0);
					}
					gdt_json_encode_parser_array( _ppool, buf_munit, (elm+i)->munit);
				}
				else if( (elm+i)->id==ELEMENT_HASH){
					if( i > 0 ){ 
						gdt_add_json_element( _ppool, buf_munit, ",", 1, 0);
					}
					gdt_json_encode_parser_hash( _ppool, buf_munit, (elm+i)->munit);
				}
			}
		}
		gdt_add_json_element( _ppool, buf_munit, "]", 1, 0);
	}while( false );
	return 0;
}

int gdt_add_json_element( GDT_MEMORY_POOL* _ppool, int32_t buf_munit, char* src, size_t src_size, uint8_t escape)
{
	do{
		if( buf_munit <= 0 ){
			printf("null buf_munit\n");
			break;
		}
		if( src_size == -1 ){
			printf("src_size == -1\n");
			return 0;
		}
		if( src_size == 0 ){
			return 0;
		}
		GDT_MEMORY_UNIT* punit = gdt_get_munit( _ppool, buf_munit );
		uint32_t buf_size = punit->size - (punit->top - punit->p)-1;
		if( src_size > buf_size ){
			//printf("buffer out of range %d: %d\n", src_size, (int)( punit->size ) );
			printf("buffer out of range %d: %d\n", (int)( src_size + ( punit->top - punit->p ) ), (int)( punit->size ) );
			break;
		}
		char* pbuf = (char*)GDT_POINTER( _ppool, buf_munit ) + ( punit->top - punit->p );
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
					printf("buffer out of range\n");
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

GDT_NODE* gdt_get_json_root( GDT_MEMORY_POOL* _ppool, int32_t json_root_munit )
{
	if( -1 == json_root_munit ){
		return NULL;
	}
	GDT_NODE* rootnode = (GDT_NODE*)GDT_POINTER(_ppool,json_root_munit);
	if(rootnode->pos==0){
		return NULL;
	}
	GDT_NODE* workelemlist = ( GDT_NODE* )GDT_POINTER( _ppool, rootnode->element_munit );
	return &workelemlist[0];
}

int32_t gdt_json_decode( GDT_MEMORY_POOL* _ppool, const char* src )
{
	int32_t rootnode_munit = -1;
	int32_t tokens_munit = -1;
	if( -1 == ( tokens_munit = gdt_inittoken( _ppool, 10000 ) ) ){
		return -1;
	}
	gdt_token_analyzer( _ppool, tokens_munit, (char*)src );
	GDT_TOKENS *ptokens = (GDT_TOKENS*)GDT_POINTER(_ppool,tokens_munit);
	if( ptokens->token_munit == -1 ){
		return -1;
	}
	GDT_TOKEN *token_list = (GDT_TOKEN*)GDT_POINTER(_ppool,ptokens->token_munit);
	rootnode_munit = gdt_createrootnode( _ppool );
	GDT_NODE *rootnode = (GDT_NODE*)GDT_POINTER( _ppool, rootnode_munit );
	//gdt_tokendump(_ppool,tokens_munit);
	if(0!=gdt_json_decode_parser( _ppool, rootnode, ptokens, token_list, 128 )){
		return -1;
	}
	return rootnode_munit;
}

int32_t gdt_json_decode_h( GDT_MEMORY_POOL* _ppool, const char* src, int32_t hash_size, int32_t init_token_size )
{
	int32_t rootnode_munit = -1;
	int32_t tokens_munit = -1;
	if( -1 == ( tokens_munit = gdt_inittoken( _ppool, init_token_size ) ) ){
		return -1;
	}
	gdt_token_analyzer( _ppool, tokens_munit, (char*)src );
	GDT_TOKENS *ptokens = (GDT_TOKENS*)GDT_POINTER(_ppool,tokens_munit);
	GDT_TOKEN *token_list = (GDT_TOKEN*)GDT_POINTER(_ppool,ptokens->token_munit);
	rootnode_munit = gdt_createrootnode( _ppool );
	GDT_NODE *rootnode = (GDT_NODE*)GDT_POINTER( _ppool, rootnode_munit );
	//gdt_tokendump(_ppool,tokens_munit);
	if(0!=gdt_json_decode_parser( _ppool, rootnode, ptokens, token_list, hash_size )){
		return -1;
	}
	return rootnode_munit;
}

int32_t gdt_json_decode_parser( GDT_MEMORY_POOL* _ppool, GDT_NODE* node, GDT_TOKENS *ptokens, GDT_TOKEN *token_list, int32_t hash_size )
{
	int32_t error = 0;
	while( ptokens->workpos < ptokens->currentpos )
	{
		if( token_list[ptokens->workpos].type == ID_SIGN ){
			if( *((char*)GDT_POINTER(_ppool,token_list[ptokens->workpos].buf_munit)) == '{' ){
				node->id = ELEMENT_HASH;
				ptokens->workpos++;
				int32_t parser_child_munit = -1;
				if( -1 == ( parser_child_munit = gdt_json_decode_parser_hash( _ppool, node, ptokens, token_list, hash_size ) ) ){
					error = 1;
					break;
				}
				gdt_addelement( _ppool, node, ELEMENT_HASH, parser_child_munit );
			}
			else if( *((char*)GDT_POINTER(_ppool,token_list[ptokens->workpos].buf_munit)) == '[' ){
				node->id = ELEMENT_ARRAY;
				ptokens->workpos++;
				int32_t parser_child_munit = -1;
				if( -1 == ( parser_child_munit = gdt_json_decode_parser_array( _ppool, node, ptokens, token_list ) ) ){
					error = 1;
					break;
				}
				gdt_addelement( _ppool, node, ELEMENT_ARRAY, parser_child_munit );
			}
			else if( 
				*((char*)GDT_POINTER(_ppool,token_list[ptokens->workpos].buf_munit)) == '}' 
				|| *((char*)GDT_POINTER(_ppool,token_list[ptokens->workpos].buf_munit)) == ']' 
			){
				ptokens->workpos++;
				break;
			}
		}
		else{
			break;
		}
		ptokens->workpos++;
	}
	return error;
}

int32_t gdt_json_decode_parser_hash( GDT_MEMORY_POOL* _ppool, GDT_NODE* node, GDT_TOKENS *ptokens, GDT_TOKEN *token_list, int32_t hash_size )
{
	int32_t working_munit = -1;
	while( ptokens->workpos < ptokens->currentpos )
	{
		if( token_list[ptokens->workpos].type == ID_STR ) {
			if( -1 == working_munit ){
				if( -1 == ( working_munit = gdt_create_hash( _ppool, hash_size ) ) ){
					printf( "create_hash is failed\n" );
					break;
				}
			}
			if( ptokens->workpos+2 >= ptokens->currentpos ){
				printf( "invalid json format1\n" );
				working_munit = -1;
				break;
			}
			if( token_list[ptokens->workpos+1].type != ID_SIGN || *((char*)GDT_POINTER(_ppool,token_list[ptokens->workpos+1].buf_munit)) != ':' ){
				printf( "invalid json format2, %d, %s\n", ptokens->workpos, (char*)GDT_POINTER(_ppool,token_list[ptokens->workpos+1].buf_munit) );
				working_munit = -1;
				break;
			}
			if( token_list[ptokens->workpos+2].type == ID_STR ){
				gdt_add_hash( _ppool, working_munit, token_list[ptokens->workpos].buf_munit, token_list[ptokens->workpos+2].buf_munit, ELEMENT_LITERAL_STR );
				ptokens->workpos+=2;
			}
			else if( token_list[ptokens->workpos+2].type == ID_NUM ){
				gdt_add_hash_integer_kint(_ppool,working_munit,token_list[ptokens->workpos].buf_munit,atoi((char*)GDT_POINTER(_ppool,token_list[ptokens->workpos+2].buf_munit)));
				//gdt_add_hash( _ppool, working_munit, token_list[ptokens->workpos].buf_munit, token_list[ptokens->workpos+2].buf_munit, ELEMENT_LITERAL_STR );
				ptokens->workpos+=2;
			}
			else if( token_list[ptokens->workpos+2].type == ID_SIGN ){
				if( *((char*)GDT_POINTER(_ppool,token_list[ptokens->workpos+2].buf_munit)) == '{' ){
					int32_t parser_child_munit = -1;
					int32_t hash_name_munit = token_list[ptokens->workpos].buf_munit;
					ptokens->workpos+=3;
					if( -1 == ( parser_child_munit = gdt_json_decode_parser_hash( _ppool, node, ptokens, token_list, 8 ) ) ){
						working_munit = -1;
						break;
					}
					gdt_add_hash( _ppool, working_munit, hash_name_munit, parser_child_munit, ELEMENT_HASH );
					if( ptokens->workpos+1 < ptokens->currentpos ){
						if( *((char*)GDT_POINTER(_ppool,token_list[ptokens->workpos+1].buf_munit)) != '}' && *((char*)GDT_POINTER(_ppool,token_list[ptokens->workpos+1].buf_munit)) != ',' ){
							printf( "invalid json hash : %c\n", *((char*)GDT_POINTER(_ppool,token_list[ptokens->workpos+1].buf_munit)) );
							working_munit = -1;
							break;
						}
					}
				}
				else if( *((char*)GDT_POINTER(_ppool,token_list[ptokens->workpos+2].buf_munit)) == '[' ){
					int32_t parser_child_munit = -1;
					int32_t hash_name_munit = token_list[ptokens->workpos].buf_munit;
					ptokens->workpos+=3;
					if( -1 == ( parser_child_munit = gdt_json_decode_parser_array( _ppool, node, ptokens, token_list ) ) ){
						working_munit = -1;
						break;
					}
					gdt_add_hash( _ppool, working_munit, hash_name_munit, parser_child_munit, ELEMENT_ARRAY );
					if( ptokens->workpos+1 < ptokens->currentpos ){
						if( *((char*)GDT_POINTER(_ppool,token_list[ptokens->workpos+1].buf_munit)) != '}' && *((char*)GDT_POINTER(_ppool,token_list[ptokens->workpos+1].buf_munit)) != ',' ){
							printf( "invalid json hash : %c\n", *((char*)GDT_POINTER(_ppool,token_list[ptokens->workpos+1].buf_munit)) );
							working_munit = -1;
							break;
						}
					}
				}
			}
		}
		else if( token_list[ptokens->workpos].type == ID_SIGN ){
			if( *((char*)GDT_POINTER(_ppool,token_list[ptokens->workpos].buf_munit)) == '}' ){
				break;
			}
			else if( *((char*)GDT_POINTER(_ppool,token_list[ptokens->workpos].buf_munit)) == ',' ){
			}
		}
		else{
			printf("invalid token type:%d , %s\n",token_list[ptokens->workpos].type, (char*)GDT_POINTER(_ppool,token_list[ptokens->workpos].buf_munit));
		}
		ptokens->workpos++;
	}
	return working_munit;
}

int32_t gdt_json_decode_parser_array( GDT_MEMORY_POOL* _ppool, GDT_NODE* node, GDT_TOKENS *ptokens, GDT_TOKEN *token_list )
{
	int32_t working_munit = -1;
	while( ptokens->workpos < ptokens->currentpos )
	{
		if( token_list[ptokens->workpos].type == ID_STR ) {
			gdt_array_push( _ppool, &working_munit, ELEMENT_LITERAL_STR, token_list[ptokens->workpos].buf_munit );
		}
		else if( token_list[ptokens->workpos].type == ID_NUM ) {
			gdt_array_push( _ppool, &working_munit, ELEMENT_LITERAL_NUM, token_list[ptokens->workpos].buf_munit );
		}
		else if( token_list[ptokens->workpos].type == ID_FLOAT ) {
			gdt_array_push( _ppool, &working_munit, ELEMENT_LITERAL_FLOAT, token_list[ptokens->workpos].buf_munit );
		}
		else if( token_list[ptokens->workpos].type == ID_SIGN ){
			if( *((char*)GDT_POINTER(_ppool,token_list[ptokens->workpos].buf_munit)) == ']' ){
				if(working_munit==-1){
					working_munit = gdt_create_array( _ppool, 8, NUMERIC_BUFFER_SIZE );
				}
				break;
			}
			else if( *((char*)GDT_POINTER(_ppool,token_list[ptokens->workpos].buf_munit)) == ',' ){
			}
			else if( *((char*)GDT_POINTER(_ppool,token_list[ptokens->workpos].buf_munit)) == '{' ){
				int32_t parser_child_munit = -1;
				ptokens->workpos++;
				if( -1 == ( parser_child_munit = gdt_json_decode_parser_hash( _ppool, node, ptokens, token_list, 4 ) ) ){
					working_munit = -1;
					break;
				}
				gdt_array_push( _ppool, &working_munit, ELEMENT_HASH, parser_child_munit );
				if( ptokens->workpos+1 < ptokens->currentpos ){
					if( *((char*)GDT_POINTER(_ppool,token_list[ptokens->workpos+1].buf_munit)) != ']' && *((char*)GDT_POINTER(_ppool,token_list[ptokens->workpos+1].buf_munit)) != ',' ){
						printf( "json array error : %c\n", *((char*)GDT_POINTER(_ppool,token_list[ptokens->workpos+1].buf_munit)) );
						working_munit = -1;
						break;
					}
				}
			}
			else if( *((char*)GDT_POINTER(_ppool,token_list[ptokens->workpos].buf_munit)) == '[' ){
				int32_t parser_child_munit = -1;
				ptokens->workpos++;
				if( -1 == ( parser_child_munit = gdt_json_decode_parser_array( _ppool, node, ptokens, token_list ) ) ){
					working_munit = -1;
					break;
				}
				gdt_array_push( _ppool, &working_munit, ELEMENT_ARRAY, parser_child_munit );
				if( ptokens->workpos+1 < ptokens->currentpos ){
					if( *((char*)GDT_POINTER(_ppool,token_list[ptokens->workpos+1].buf_munit)) != ']' && *((char*)GDT_POINTER(_ppool,token_list[ptokens->workpos+1].buf_munit)) != ',' ){
						printf( "json array error : %c\n", *((char*)GDT_POINTER(_ppool,token_list[ptokens->workpos+1].buf_munit)) );
						working_munit = -1;
						break;
					}
				}
			}
			else{
				working_munit = -1;
				printf("invalid token type:%d\n",token_list[ptokens->workpos].type);
				break;
			}
		}
		else{
			working_munit = -1;
			printf("invalid token type:%d\n",token_list[ptokens->workpos].type);
			break;
		}
		ptokens->workpos++;
	}
	return working_munit;
}

int32_t gdt_make_json_root( GDT_MEMORY_POOL* _ppool, int32_t data_munit, int id )
{
	int32_t rootnode_munit = -1;
	do{
		rootnode_munit = gdt_createrootnode( _ppool );
		GDT_NODE *rootnode = (GDT_NODE*)GDT_POINTER( _ppool, rootnode_munit );
		rootnode->id = id;
		gdt_addelement( _ppool, rootnode, id, data_munit );
	}while( false );
	return rootnode_munit;
}
