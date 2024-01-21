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

#include "qs_script.h"

int32_t qs_init_script( QS_MEMORY_POOL* _ppool, size_t valiablehash_size, size_t functionhash_size, int32_t init_token_size )
{
	QS_SCRIPT *pscript;
	int32_t munit = -1;
	if( -1 == (munit = qs_create_memory_block( _ppool, sizeof( QS_SCRIPT ) )) ){
#ifdef __QS_DEBUG__
		printf("qs_init_script error.\n");
#endif
		return -1;
	}
	pscript = (QS_SCRIPT *)QS_GET_POINTER( _ppool, munit );
	pscript->self_munit = munit;
	pscript->textbuf_munit = -1;
	pscript->hash_alloc_size = 64;
	if( -1 == ( pscript->tokens_munit = qs_inittoken( _ppool, init_token_size, QS_TOKEN_READ_BUFFER_SIZE_MIN ) ) ){
		return -1;
	}
	if( -1 == ( pscript->rootnode_munit = qs_createrootnode( _ppool ) ) ){
		return -1;
	}
	if( -1 == ( pscript->v_hash_munit = qs_create_hash( _ppool, valiablehash_size ) ) ){
		return -1;
	}
	if( -1 == ( pscript->f_hash_munit = qs_create_hash( _ppool, functionhash_size ) ) ){
		return -1;
	}
	if( -1 == ( pscript->s_hash_munit = qs_create_array( _ppool, 128 ) ) ){
		return -1;
	}
	if( -1 == ( pscript->return_munit = qs_create_memory_block( _ppool, sizeof( QS_FUNCTION_RETURN ) ) ) ){
		return -1;
	}
	if( -1 == ( pscript->int_cache_munit = qs_create_memory_block( _ppool, sizeof(int32_t) * 32 ) ) ){
		return -1;
	}
	return munit;
}

void qs_import_script( QS_MEMORY_POOL* _ppool, int32_t* p_unitid, char* filename )
{
	FILE* file;
	int fsize = 0;
	char *pstr;
	QS_SCRIPT *pscript;
	char* p;
#ifdef __WINDOWS__
	char c;
#else
	int8_t c;
#endif

#ifdef __QS_DEBUG__
#ifdef __WINDOWS__
//	LARGE_INTEGER start_pc, end_pc, freq_pc;
//	double sec_pc;
//	QueryPerformanceFrequency( &freq_pc );
//	QueryPerformanceCounter( &start_pc );
#else
//	struct timeval start_timeval, end_timeval;
//	double sec_timeofday;
//	gettimeofday( &start_timeval, NULL );
#endif
#endif
	pscript = (QS_SCRIPT *)QS_GET_POINTER( _ppool, ( *p_unitid ) );
	//qs_lstateout( _ppool, filename );
#ifdef __WINDOWS__
	if(0 != fopen_s(&file, filename, "r"))
#else
	if (!(file = fopen(filename, "r")))
#endif
	{
		if( filename )
		{
			printf("fopen error : %s\n", filename );
		}
		else
		{
			printf("filename is null\n");
		}
		return;
	}
	// fgetpos
	fseek( file, 0L, SEEK_END );
	fsize = ftell( file );
	if( fseek( file, 0L, SEEK_SET ) != 0 )
	{
		printf("fseek error\n");
		return;
	}
#ifdef __QS_DEBUG__
//	printf("filse size of %s : %dbytes\n", filename, fsize );
#endif
	pscript->textbuf_munit = qs_create_memory_block( _ppool, sizeof( char ) * ( fsize+1 ) );
	pstr = (char*)QS_GET_POINTER( _ppool, pscript->textbuf_munit );
	if( pstr == NULL )
	{
		printf("allocate error : token buf\n");
		return;
	}
	p = pstr;
	while( EOF != (c=fgetc(file)) ){ *(p++) = c; }
	*p = '\0';
	fclose(file);
#ifdef __QS_DEBUG__
#ifdef __WINDOWS__
//	QueryPerformanceCounter( &end_pc );
//	sec_pc = (end_pc.QuadPart - start_pc.QuadPart) / (double)freq_pc.QuadPart;
//	printf(  "## file read time = %lf[s]\n", sec_pc );
#else
//	gettimeofday( &end_timeval, NULL );
//	sec_timeofday = (end_timeval.tv_sec - start_timeval.tv_sec)
//			+ (end_timeval.tv_usec - start_timeval.tv_usec) / 1000000.0;
//	printf(  "## file read time = %lf[s]\n", sec_timeofday );
#endif
#endif

#ifdef __QS_DEBUG__
#ifdef __WINDOWS__
//	QueryPerformanceFrequency( &freq_pc );
//	QueryPerformanceCounter( &start_pc );
#else
//	gettimeofday( &start_timeval, NULL );
#endif
#endif
	qs_token_analyzer( _ppool, pscript->tokens_munit, pstr );
#ifdef __QS_DEBUG__
#ifdef __WINDOWS__
//	QueryPerformanceCounter( &end_pc );
//	sec_pc = (end_pc.QuadPart - start_pc.QuadPart) / (double)freq_pc.QuadPart;
//	//printf(  "## token analyse time = %lf[s]\n", sec_pc );
#else
//	gettimeofday( &end_timeval, NULL );
//	sec_timeofday = (end_timeval.tv_sec - start_timeval.tv_sec)
//			+ (end_timeval.tv_usec - start_timeval.tv_usec) / 1000000.0;
//	//printf(  "## token analyse time = %lf[s]\n", sec_timeofday );
#endif
#endif
	//qs_tokendump( _ppool, pscript->tokens_munit );
#ifdef __QS_DEBUG__
#ifdef __WINDOWS__
//	QueryPerformanceFrequency( &freq_pc );
//	QueryPerformanceCounter( &start_pc );
#else
//	gettimeofday( &start_timeval, NULL );
#endif
#endif
	qs_parse_code( _ppool, pscript );
#ifdef __QS_DEBUG__
#ifdef __WINDOWS__
//	QueryPerformanceCounter( &end_pc );
//	sec_pc = (end_pc.QuadPart - start_pc.QuadPart) / (double)freq_pc.QuadPart;
//	printf(  "## parse script time = %lf[s]\n", sec_pc );
#else
//	gettimeofday( &end_timeval, NULL );
//	sec_timeofday = (end_timeval.tv_sec - start_timeval.tv_sec)
//			+ (end_timeval.tv_usec - start_timeval.tv_usec) / 1000000.0;
//	printf(  "## parse script time = %lf[s]\n", sec_timeofday );
#endif
#endif
}

void qs_input_script( QS_MEMORY_POOL* _ppool, int32_t* p_unitid, char* pstr )
{
	QS_SCRIPT *pscript;
#ifdef __QS_DEBUG__
#ifdef __WINDOWS__
//	LARGE_INTEGER start_pc, end_pc, freq_pc;
//	double sec_pc;
//	QueryPerformanceFrequency( &freq_pc );
//	QueryPerformanceCounter( &start_pc );
#else
//	struct timeval start_timeval, end_timeval;
//	double sec_timeofday;
//	gettimeofday( &start_timeval, NULL );
#endif
#endif
	pscript = (QS_SCRIPT *)QS_GET_POINTER( _ppool, ( *p_unitid ) );
	qs_token_analyzer( _ppool, pscript->tokens_munit, pstr );
#ifdef __QS_DEBUG__
#ifdef __WINDOWS__
//	QueryPerformanceCounter( &end_pc );
//	sec_pc = (end_pc.QuadPart - start_pc.QuadPart) / (double)freq_pc.QuadPart;
//	printf(  "## token analyse time = %lf[s]\n", sec_pc );
#else
//	gettimeofday( &end_timeval, NULL );
//	sec_timeofday = (end_timeval.tv_sec - start_timeval.tv_sec)
//			+ (end_timeval.tv_usec - start_timeval.tv_usec) / 1000000.0;
//	printf(  "## token analyse time = %lf[s]\n", sec_timeofday );
#endif
#endif
	// qs_tokendump( _ppool, pscript );
#ifdef __QS_DEBUG__
#ifdef __WINDOWS__
//	QueryPerformanceFrequency( &freq_pc );
//	QueryPerformanceCounter( &start_pc );
#else
//	gettimeofday( &start_timeval, NULL );
#endif
#endif
	qs_parse_code( _ppool, pscript );
#ifdef __QS_DEBUG__
#ifdef __WINDOWS__
//	QueryPerformanceCounter( &end_pc );
//	sec_pc = (end_pc.QuadPart - start_pc.QuadPart) / (double)freq_pc.QuadPart;
//	printf(  "## parse script time = %lf[s]\n", sec_pc );
#else
//	gettimeofday( &end_timeval, NULL );
//	sec_timeofday = (end_timeval.tv_sec - start_timeval.tv_sec)
//			+ (end_timeval.tv_usec - start_timeval.tv_usec) / 1000000.0;
//	printf(  "## parse script time = %lf[s]\n", sec_timeofday );
#endif
#endif
}

int qs_parse_code( QS_MEMORY_POOL* _ppool, QS_SCRIPT* pscript )
{
	int error = 0;
	int i = 0;
	int tmpi = 0;
	QS_NODE *rootnode;
	QS_TOKENS *ptokens = (QS_TOKENS*)QS_GET_POINTER( _ppool, pscript->tokens_munit );
	rootnode = (QS_NODE*)QS_GET_POINTER( _ppool, pscript->rootnode_munit );
	do
	{
		tmpi = i;
		i = qs_parse_code_core( _ppool, pscript, rootnode, i );
		if( i < 0 ){
			error = 1;
			break;
		}
		if( i == tmpi ){
			printf( "invalid error\n" );
			break;
		}
	}while( i < ptokens->currentpos );
	return error;
}

int qs_parse_code_core( QS_MEMORY_POOL* _ppool, QS_SCRIPT* pscript, QS_NODE* node, int i )
{
	QS_TOKENS *ptokens = (QS_TOKENS*)QS_GET_POINTER( _ppool, pscript->tokens_munit );
	QS_TOKEN *token_list;
	char *pbuf;
	int error = 0;
	token_list = ( QS_TOKEN* )QS_GET_POINTER( _ppool, ptokens->token_munit );
	pbuf = (char*)QS_GET_POINTER( _ppool, token_list[i].buf_munit );
	switch( token_list[i].type )
	{
		case ID_SYMBOL:
			i = qs_parse_symbol( _ppool, pscript, node, i );
			break;
		case ID_SYS_IF:
			i = qs_parse_if( _ppool, pscript, node, i );
			break;
		case ID_SYS_WHILE:
			i = qs_parse_while( _ppool, pscript, node, i );
			break;
		case ID_SIGN:
			if( !strcmp( ";", pbuf ) ){
				i++;
			}
			else if( strcmp( "{", pbuf ) ){
				error = 1;
			}
			else{
				i = qs_parse_block( _ppool, pscript, node, i );
			}
			break;
		case ID_SYS_ELSE:
		case ID_SYS_ELSEIF:
		case ID_UNK:
		case ID_NUM:
		case ID_STR:
		case ID_OP:
			printf("qs_parse_code_core error : %d\n",token_list[i].type);
			error=1;
			break;
	}
	if( error != 0 )
	{
		printf("syntax error : %s\n" , pbuf );
		i = -1;
	}
	return i;
}

/*
 * { ... }
 */
int qs_parse_block( QS_MEMORY_POOL* _ppool, QS_SCRIPT* pscript, QS_NODE* node, int index )
{
	QS_TOKENS *ptokens = (QS_TOKENS*)QS_GET_POINTER( _ppool, pscript->tokens_munit );
	QS_TOKEN *token_list;
	QS_NODE *childnode;
	int32_t childmunit;
	char* pbuf;
	token_list = ( QS_TOKEN* )QS_GET_POINTER( _ppool, ptokens->token_munit );
	childmunit = qs_addnodeelement( _ppool, node );
	childnode = (QS_NODE*)QS_GET_POINTER( _ppool, childmunit );
	index++;
	while( index < ptokens->currentpos )
	{
		if( token_list[index].type == ID_SIGN )
		{
			pbuf = (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit );
			if( !strcmp( pbuf, "}" ) )
			{
				index++;
				break;
			}
		}
		index = qs_parse_code_core( _ppool, pscript, childnode, index );
		if( index < 0 ){
			printf( "parse error\n" );
			break;
		}
		if( token_list[index].type == ID_SIGN )
		{
			pbuf = (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit );
			if( !strcmp( pbuf, "}" ) )
			{
				index++;
				break;
			}
		}
	}
	return index;
}

int qs_parse_array( QS_MEMORY_POOL* _ppool, QS_SCRIPT* pscript, QS_NODE* node, QS_TOKEN* pt, int index )
{
	QS_TOKENS *ptokens = (QS_TOKENS*)QS_GET_POINTER( _ppool, pscript->tokens_munit );
	QS_TOKEN *token_list;
	QS_NODE *childnode;
	QS_NODE *stmtnode;
	int32_t childmunit;
	int32_t tmpmunit;
	int32_t stmtmunit;
	int tmpindex;
	char* pbuf;
	if( index >= ptokens->currentpos ){
		printf("invalid index2\n");
		return index;
	}
	token_list = ( QS_TOKEN* )QS_GET_POINTER( _ppool, ptokens->token_munit );
	pbuf = (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit );
	do{
		if( !strcmp( "[", pbuf ) )
		{
			childmunit = qs_addnodeelement( _ppool, node );
			childnode = (QS_NODE*)QS_GET_POINTER( _ppool, childmunit );
			if( 0 >= (tmpmunit = qs_create_memory_block( _ppool, 8 ))){
				printf("alloc error\n");
				return index;
			}
			pbuf = (char*)QS_GET_POINTER( _ppool, tmpmunit );
			qs_strcopy( pbuf, "array", QS_PUNIT_USIZE( _ppool, tmpmunit ) );
			qs_addelement( _ppool, childnode, ELEMENT_ARRAY, tmpmunit );
			index++;
			if( index >= ptokens->currentpos ){
				printf("invalid index parse_array\n");
				return index;
			}
			if( !strcmp( (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit ), "]" ) )
			{
				//printf("empty array\n");
				index++;
				return index;
			}
			else{
				stmtmunit = qs_addnodeelement( _ppool, childnode );
				stmtnode = (QS_NODE*)QS_GET_POINTER( _ppool, stmtmunit );
				while( index < ptokens->currentpos )
				{
					tmpindex = index;
					if( token_list[index].type == ID_SIGN )
					{
						pbuf = (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit );
						if( !strcmp( pbuf, "," ) ){
							childnode = (QS_NODE*)QS_GET_POINTER( _ppool, childmunit );
							stmtmunit = qs_addnodeelement( _ppool, childnode );
							stmtnode = (QS_NODE*)QS_GET_POINTER( _ppool, stmtmunit );
							index++;
						}
						else if( !strcmp( pbuf, "]" ) ){
							index++;
							break;
						}
					}
					stmtnode = (QS_NODE*)QS_GET_POINTER( _ppool, stmtmunit );
					if( token_list[index].type == ID_SIGN ){
						pbuf = (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit );
						if( !strcmp( (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit ), "[" ) )
						{
							index = qs_parse_array( _ppool, pscript, stmtnode, token_list, index );
							pbuf = (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit );
							if( !strcmp( pbuf, "]" ) ){
								index++;
								break;
							}
						}
						else if( !strcmp( ":", pbuf ) ){
							int32_t stmtchildmunit = qs_addnodeelement( _ppool, stmtnode );
							QS_NODE* stmtchildnode = (QS_NODE*)QS_GET_POINTER( _ppool, stmtchildmunit );
							qs_addelement( _ppool, stmtnode, ELEMENT_HASH_OP, token_list[index].buf_munit );
							index++;
							if( !strcmp( (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit ), "[" ) )
							{
								index = qs_parse_array( _ppool, pscript, stmtchildnode, token_list, index );
								pbuf = (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit );
								if( !strcmp( pbuf, "]" ) ){
									index++;
									break;
								}
							}
							else{
								index = qs_parse_rel( _ppool, pscript, stmtchildnode, token_list, index );
								pbuf = (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit );
								if( !strcmp( pbuf, "]" ) ){
									index++;
									break;
								}
							}
						}
					}
					else{
						int32_t stmtchildmunit = qs_addnodeelement( _ppool, stmtnode );
						QS_NODE* stmtchildnode = (QS_NODE*)QS_GET_POINTER( _ppool, stmtchildmunit );
						index = qs_parse_rel( _ppool, pscript, stmtchildnode, token_list, index );
						if( token_list[index].type == ID_OP )
						{
							pbuf = (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit );
							if( !strcmp( ">=",pbuf ) 
								|| !strcmp( "<=",pbuf )
								|| !strcmp( ">",pbuf )
								|| !strcmp( "<",pbuf )
								|| !strcmp( "||",pbuf )
								|| !strcmp( "&&",pbuf )
							)
							{
								stmtchildnode = (QS_NODE*)QS_GET_POINTER( _ppool, stmtchildmunit );
								qs_addelement( _ppool, stmtchildnode, ELEMENT_CMP, token_list[index].buf_munit );
								index++;
							}
						}
					}
					if( index == tmpindex ){
						printf( "invalid index1\n" );
						break;
					}
				}
			}
		}
	}while( false );
	return index;
}

int qs_parse_rel( QS_MEMORY_POOL* _ppool, QS_SCRIPT* pscript, QS_NODE* node, QS_TOKEN* pt, int index )
{
	int32_t tmpmunit;
	char* pbuf;
	QS_TOKENS *ptokens = (QS_TOKENS*)QS_GET_POINTER( _ppool, pscript->tokens_munit );
	index = qs_parse_cmp( _ppool, pscript, node, pt, index );
	if( index >= ptokens->currentpos ){
		return index;
	}
	pbuf = (char*)QS_GET_POINTER( _ppool, pt[index].buf_munit );
	if( !strcmp( "||", pbuf ) || !strcmp( "&&", pbuf ) )
	{
		tmpmunit = pt[index].buf_munit;
		index++;
		index = qs_parse_cmp( _ppool, pscript, node, pt, index );
		qs_addelement( _ppool, node, ELEMENT_OP, tmpmunit );
	}
	return index;
}

int qs_parse_cmp( QS_MEMORY_POOL* _ppool, QS_SCRIPT* pscript, QS_NODE* node, QS_TOKEN* pt, int index )
{
	int32_t tmpmunit;
	char* pbuf;
	QS_TOKENS *ptokens = (QS_TOKENS*)QS_GET_POINTER( _ppool, pscript->tokens_munit );
	index = qs_parse_addsub( _ppool, pscript, node, pt, index );
	if( index >= ptokens->currentpos ){
		return index;
	}
	pbuf = (char*)QS_GET_POINTER( _ppool, pt[index].buf_munit );
	if( !strcmp( ">=",pbuf ) 
		|| !strcmp( "<=",pbuf )
		|| !strcmp( ">",pbuf )
		|| !strcmp( "<",pbuf )
		|| !strcmp( "==",pbuf )
	)
	{
		tmpmunit = pt[index].buf_munit;
		index++;
		index = qs_parse_addsub( _ppool, pscript, node, pt, index );
		qs_addelement( _ppool, node, ELEMENT_OP, tmpmunit );
	}
	return index;
}

int qs_parse_addsub( QS_MEMORY_POOL* _ppool, QS_SCRIPT* pscript, QS_NODE* node, QS_TOKEN* pt, int index )
{
	int32_t tmpmunit;
	char* pbuf;
	QS_TOKENS *ptokens = (QS_TOKENS*)QS_GET_POINTER( _ppool, pscript->tokens_munit );
	index = qs_parse_muldiv( _ppool, pscript, node, pt, index );
	if( index >= ptokens->currentpos ){
		return index;
	}
	pbuf = (char*)QS_GET_POINTER( _ppool, pt[index].buf_munit );
	if( !strcmp( "+", pbuf ) )
	{
		tmpmunit = pt[index].buf_munit;
		index++;
		index = qs_parse_muldiv( _ppool, pscript, node, pt, index );
		qs_addelement( _ppool, node, ELEMENT_OP, tmpmunit );
	}
	else if( !strcmp( "-", pbuf ) )
	{
		tmpmunit = pt[index].buf_munit;
		index++;
		index = qs_parse_muldiv( _ppool, pscript, node, pt, index );
		qs_addelement( _ppool, node, ELEMENT_OP, tmpmunit );
	}
	return index;
}

int qs_parse_muldiv( QS_MEMORY_POOL* _ppool, QS_SCRIPT* pscript, QS_NODE* node, QS_TOKEN* pt, int index )
{
	int32_t tmpmunit;
	char* pbuf;
	QS_TOKENS *ptokens = (QS_TOKENS*)QS_GET_POINTER( _ppool, pscript->tokens_munit );
	index = qs_parse_expr( _ppool, pscript, node, pt, index );
	if( index >= ptokens->currentpos ){
		return index;
	}
	pbuf = (char*)QS_GET_POINTER( _ppool, pt[index].buf_munit );
	if( !strcmp( "*", pbuf ) )
	{
		tmpmunit = pt[index].buf_munit;
		index++;
		index = qs_parse_expr( _ppool, pscript, node, pt, index );
		qs_addelement( _ppool, node, ELEMENT_OP, tmpmunit );
	}
	else if( !strcmp( "/", pbuf ) )
	{
		tmpmunit = pt[index].buf_munit;
		index++;
		index = qs_parse_expr( _ppool, pscript, node, pt, index );
		qs_addelement( _ppool, node, ELEMENT_OP, tmpmunit );
	}
	else if( !strcmp( "%", pbuf ) )
	{
		tmpmunit = pt[index].buf_munit;
		index++;
		index = qs_parse_expr( _ppool, pscript, node, pt, index );
		qs_addelement( _ppool, node, ELEMENT_OP, tmpmunit );
	}
	return index;
}

int qs_parse_expr( QS_MEMORY_POOL* _ppool, QS_SCRIPT* pscript, QS_NODE* node, QS_TOKEN* pt, int index )
{
	if( !strcmp( "(", (char*)QS_GET_POINTER( _ppool, pt[index].buf_munit ) ) )
	{
		index++;
		index = qs_parse_rel( _ppool, pscript, node, pt, index );
		if( !strcmp( ")", (char*)QS_GET_POINTER( _ppool, pt[index].buf_munit ) ) )
		{
			index++;
		}
		else{
			printf( "? : %d \n", index );
		}
	}
	else{
		if( pt[index].type == ID_NUM )
		{
			qs_addelement( _ppool, node, ELEMENT_LITERAL_NUM, pt[index].buf_munit );
			index++;
		}
		else if( pt[index].type == ID_STR )
		{
			qs_addelement( _ppool, node, ELEMENT_LITERAL_STR, pt[index].buf_munit );
			index++;
		}
		else if( pt[index].type == ID_FLOAT )
		{
			qs_addelement( _ppool, node, ELEMENT_LITERAL_FLOAT, pt[index].buf_munit );
			index++;
		}
		else if( pt[index].type == ID_SYMBOL )
		{
			index = qs_parse_symbol( _ppool, pscript, node, index );
		}
		else{
			if( !strcmp((char*)QS_GET_POINTER( _ppool, pt[index].buf_munit ),"-") ){
				int32_t tmpmunit = pt[index].buf_munit;
				index++;
				index = qs_parse_expr( _ppool, pscript, node, pt, index );
				qs_addelement( _ppool, node, ELEMENT_OP_LITERAL_MINUS, tmpmunit );
			}
			else{
				//printf("other expr ?? : %s\n",(char*)QS_GET_POINTER( _ppool, pt[index].buf_munit ));
			}
		}
	}
	return index;
}

int qs_parse_symbol( QS_MEMORY_POOL* _ppool, QS_SCRIPT* pscript, QS_NODE* node, int index )
{
	QS_TOKENS *ptokens = (QS_TOKENS*)QS_GET_POINTER( _ppool, pscript->tokens_munit );
	QS_TOKEN *token_list;
	QS_NODE *childnode;
	QS_NODE *stmtnode;
	int32_t childmunit;
	int32_t stmtmunit;
	int32_t tmpmunit;
	char* pbuf;
	int tmpindex;
	if( index >= ptokens->currentpos ){
		printf("invalid index3\n");
		return index;
	}
	token_list = ( QS_TOKEN* )QS_GET_POINTER( _ppool, ptokens->token_munit );
	tmpmunit = token_list[index].buf_munit;
	pbuf = (char*)QS_GET_POINTER( _ppool, tmpmunit );
	if( !strcmp( pbuf, "def" ) ){
		index++;
		if( index >= ptokens->currentpos ){
			printf("invalid index4\n");
			return index;
		}
		index = qs_parse_function( _ppool, pscript, node, index );
		return index;
	}
	index++;
	if( index >= ptokens->currentpos ){
		printf("invalid index5\n");
		return index;
	}
	pbuf = (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit );
	// increment, decrement
	if( !strcmp( "++", pbuf ) || !strcmp( "--", pbuf) ){
		childmunit = qs_addnodeelement( _ppool, node );
		childnode = (QS_NODE*)QS_GET_POINTER( _ppool, childmunit );
		qs_addelement( _ppool, childnode, ELEMENT_VALIABLE, tmpmunit );
		stmtmunit = qs_addnodeelement( _ppool, childnode );
		stmtnode = (QS_NODE*)QS_GET_POINTER( _ppool, stmtmunit );
		int32_t memid_op = token_list[index].buf_munit;
		index++;
		if(!strcmp( pbuf, "++" )){
			qs_addelement( _ppool, stmtnode, ELEMENT_INCREMENT_AFTER, memid_op );
		}
		else if(!strcmp( pbuf, "--" )){
			qs_addelement( _ppool, stmtnode, ELEMENT_DECREMENT_AFTER, memid_op );
		}
		// variable
		qs_addelement( _ppool, stmtnode, ELEMENT_VALIABLE, tmpmunit );
		// =
		int32_t memid_op2 = qs_create_memory_block( _ppool, 8 );
		char* pbuf2 = (char*)QS_GET_POINTER( _ppool, memid_op2 );
		qs_strcopy( pbuf2, "=", QS_PUNIT_USIZE( _ppool, memid_op2 ) );
		qs_addelement( _ppool, childnode, ELEMENT_ASSIGNMENT, memid_op2 );
	}
	else if( !strcmp( "=", pbuf )
		|| !strcmp( "+=", pbuf )
		|| !strcmp( "-=", pbuf )
		|| !strcmp( "*=", pbuf )
		|| !strcmp( "/=", pbuf )
		|| !strcmp( "%=", pbuf )
		|| !strcmp( "^=", pbuf )
		|| !strcmp( "~=", pbuf )
		|| !strcmp( "|=", pbuf )
		|| !strcmp( "&=", pbuf )
	)
	{
		childmunit = qs_addnodeelement( _ppool, node );
		childnode = (QS_NODE*)QS_GET_POINTER( _ppool, childmunit );
		qs_addelement( _ppool, childnode, ELEMENT_VALIABLE, tmpmunit );
		stmtmunit = qs_addnodeelement( _ppool, childnode );
		stmtnode = (QS_NODE*)QS_GET_POINTER( _ppool, stmtmunit );
		tmpmunit = token_list[index].buf_munit;
		index++;

		if( token_list[index].type == ID_SIGN && !strcmp( "[", (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit ) ) ){
			index = qs_parse_array( _ppool, pscript, stmtnode, token_list, index );
		}
		else{
			while( index < ptokens->currentpos )
			{
				tmpindex = index;
				stmtnode = (QS_NODE*)QS_GET_POINTER( _ppool, stmtmunit );
				index = qs_parse_rel( _ppool, pscript, stmtnode, token_list, index );
				if( token_list[index].type == ID_SIGN )
				{
					pbuf = (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit );
					if( !strcmp( pbuf, ")" ) )
					{
						index++;
					}
				}
				if( token_list[index].type == ID_SIGN )
				{
					pbuf = (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit );
					if( !strcmp( ";",pbuf ) )
					{
						index++;
						break;
					}
				}
				else if( token_list[index].type == ID_OP )
				{
					pbuf = (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit );
					if( !strcmp( ">=",pbuf ) 
						|| !strcmp( "<=",pbuf )
						|| !strcmp( ">",pbuf )
						|| !strcmp( "<",pbuf )
						|| !strcmp( "||",pbuf )
						|| !strcmp( "&&",pbuf )
					)
					{
						qs_addelement( _ppool, stmtnode, ELEMENT_CMP, token_list[index].buf_munit );
						index++;
					}
					// 無名関数
					if( !strcmp( "=>",pbuf ) )
					{
						qs_addelement( _ppool, stmtnode, ELEMENT_CMP, token_list[index].buf_munit );
						index++;
						index = qs_parse_block( _ppool, pscript, stmtnode, index );
					}
				}
				if( index == tmpindex ){
					printf( "invalid index6\n" );
					break;
				}
			}
		}
		childnode = (QS_NODE*)QS_GET_POINTER( _ppool, childmunit );
		qs_addelement( _ppool, childnode, ELEMENT_ASSIGNMENT, tmpmunit );
	}
	else if( !strcmp( "(", pbuf ) )
	{
		index--;
		index = qs_parse_function_args( _ppool, pscript, node, index );
	}
	else if( !strcmp( "[", pbuf ) )
	{
		childmunit = qs_addnodeelement( _ppool, node );
		childnode = (QS_NODE*)QS_GET_POINTER( _ppool, childmunit );
		qs_addelement( _ppool, childnode, ELEMENT_ARRAY_REFERENCE, tmpmunit );
		index--;
		index = qs_parse_array_index( _ppool, pscript, childnode, index );
//printf("qs_parse_array_index : %s\n", (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit ));
		pbuf = (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit );
		if( !strcmp( "=", pbuf )
			|| !strcmp( "+=", pbuf )
			|| !strcmp( "-=", pbuf )
			|| !strcmp( "*=", pbuf )
			|| !strcmp( "/=", pbuf )
			|| !strcmp( "%=", pbuf )
			|| !strcmp( "^=", pbuf )
			|| !strcmp( "~=", pbuf )
			|| !strcmp( "|=", pbuf )
			|| !strcmp( "&=", pbuf )
		)
		{
			stmtmunit = qs_addnodeelement( _ppool, childnode );
			stmtnode = (QS_NODE*)QS_GET_POINTER( _ppool, stmtmunit );
			tmpmunit = token_list[index].buf_munit;
			index++;
			if( token_list[index].type == ID_SIGN && !strcmp( "[", (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit ) ) ){
				index = qs_parse_array( _ppool, pscript, stmtnode, token_list, index );
			}
			else{
				while( index < ptokens->currentpos )
				{
					tmpindex = index;
					stmtnode = (QS_NODE*)QS_GET_POINTER( _ppool, stmtmunit );
					index = qs_parse_rel( _ppool, pscript, stmtnode, token_list, index );
					if( token_list[index].type == ID_SIGN )
					{
						pbuf = (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit );
						if( !strcmp( pbuf, ")" ) )
						{
							index++;
						}
					}
					if( token_list[index].type == ID_SIGN )
					{
						pbuf = (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit );
						if( !strcmp( ";",pbuf ) )
						{
							index++;
							break;
						}
					}
					else if( token_list[index].type == ID_OP )
					{
						pbuf = (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit );
						if( !strcmp( ">=",pbuf ) 
							|| !strcmp( "<=",pbuf )
							|| !strcmp( ">",pbuf )
							|| !strcmp( "<",pbuf )
							|| !strcmp( "||",pbuf )
							|| !strcmp( "&&",pbuf )
						)
						{
							qs_addelement( _ppool, stmtnode, ELEMENT_CMP, token_list[index].buf_munit );
							index++;
						}
						// 無名関数
						if( !strcmp( "=>",pbuf ) )
						{
							qs_addelement( _ppool, stmtnode, ELEMENT_CMP, token_list[index].buf_munit );
							index++;
							index = qs_parse_block( _ppool, pscript, stmtnode, index );
						}
					}
					if( index == tmpindex ){
						printf( "invalid index6\n" );
						break;
					}
				}
			}
			childnode = (QS_NODE*)QS_GET_POINTER( _ppool, childmunit );
			qs_addelement( _ppool, childnode, ELEMENT_ASSIGNMENT, tmpmunit );
		}
	}
	else if( !strcmp( "return", (char*)QS_GET_POINTER( _ppool, tmpmunit ) ) ){
		childmunit = qs_addnodeelement( _ppool, node );
		childnode = (QS_NODE*)QS_GET_POINTER( _ppool, childmunit );
		qs_addelement( _ppool, childnode, ELEMENT_RETURN, tmpmunit );
		stmtmunit = qs_addnodeelement( _ppool, childnode );
		stmtnode = (QS_NODE*)QS_GET_POINTER( _ppool, stmtmunit );
		index = qs_parse_rel( _ppool, pscript, stmtnode, token_list, index );
	}
	else{
		qs_addelement( _ppool, node, ELEMENT_VALIABLE, tmpmunit );
	}
	return index;
}

int qs_parse_function( QS_MEMORY_POOL* _ppool, QS_SCRIPT* pscript, QS_NODE* node, int index )
{
	int32_t rootnode_munit = -1;
	QS_NODE *functionrootnode;
	QS_TOKENS *ptokens = (QS_TOKENS*)QS_GET_POINTER( _ppool, pscript->tokens_munit );
	QS_TOKEN *token_list;
	char* pfunctionname;
	if( index >= ptokens->currentpos ){
		printf("invalid index7\n");
		return index;
	}
	token_list = ( QS_TOKEN* )QS_GET_POINTER( _ppool, ptokens->token_munit );
	pfunctionname = (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit );
	do{
		rootnode_munit = qs_createrootnode( _ppool );
		if( rootnode_munit <= 0 ){
			break;
		}
		functionrootnode = (QS_NODE*)QS_GET_POINTER( _ppool, rootnode_munit );
		index = qs_parse_function_args( _ppool, pscript, functionrootnode, index );
		index = qs_parse_block( _ppool, pscript, functionrootnode, index );
		qs_add_user_function( _ppool, pscript->self_munit, pfunctionname, rootnode_munit, 0 );
	}while( false );
	return index;
}

int qs_parse_function_args( QS_MEMORY_POOL* _ppool, QS_SCRIPT* pscript, QS_NODE* node, int index )
{
	QS_TOKENS *ptokens = (QS_TOKENS*)QS_GET_POINTER( _ppool, pscript->tokens_munit );
	QS_TOKEN *token_list;
	QS_NODE *childnode;
	QS_NODE *stmtnode;
	int32_t childmunit;
	int32_t stmtmunit;
	int32_t tmpmunit;
	char* pbuf;
	int tmpindex;
	if( index >= ptokens->currentpos ){
		printf("invalid index8\n");
		return index;
	}
	token_list = ( QS_TOKEN* )QS_GET_POINTER( _ppool, ptokens->token_munit );
	tmpmunit = token_list[index].buf_munit;
	index++;
	if( index >= ptokens->currentpos ){
		printf("invalid index9\n");
		return index;
	}
	pbuf = (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit );
	do{
		childmunit = qs_addnodeelement( _ppool, node );
		childnode = (QS_NODE*)QS_GET_POINTER( _ppool, childmunit );
		qs_addelement( _ppool, childnode, ELEMENT_FUNCTION, tmpmunit );
		if( !strcmp( "(", pbuf ) )
		{
			index++;
			if( token_list[index].type == ID_SIGN && !strcmp( (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit ), ")" ) )
			{
				index++;
			}
			else{
				stmtmunit = qs_addnodeelement( _ppool, childnode );
				stmtnode = (QS_NODE*)QS_GET_POINTER( _ppool, stmtmunit );
				while( index < ptokens->currentpos )
				{
					tmpindex = index;
					if( token_list[index].type == ID_SIGN )
					{
						pbuf = (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit );
						if( !strcmp( pbuf, "," ) ){
							childnode = (QS_NODE*)QS_GET_POINTER( _ppool, childmunit );
							stmtmunit = qs_addnodeelement( _ppool, childnode );
							stmtnode = (QS_NODE*)QS_GET_POINTER( _ppool, stmtmunit );
							index++;
						}
						else if( !strcmp( pbuf, ")" ) ){
							index++;
							break;
						}
					}
					stmtnode = (QS_NODE*)QS_GET_POINTER( _ppool, stmtmunit );
					index = qs_parse_rel( _ppool, pscript, stmtnode, token_list, index );
					if( token_list[index].type == ID_OP )
					{
						pbuf = (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit );
						if( !strcmp( ">=",pbuf ) 
							|| !strcmp( "<=",pbuf )
							|| !strcmp( ">",pbuf )
							|| !strcmp( "<",pbuf )
							|| !strcmp( "||",pbuf )
							|| !strcmp( "&&",pbuf )
						)
						{
							stmtnode = (QS_NODE*)QS_GET_POINTER( _ppool, stmtmunit );
							qs_addelement( _ppool, stmtnode, ELEMENT_CMP, token_list[index].buf_munit );
							index++;
						}
					}
					if( token_list[index].type == ID_SIGN ){
						pbuf = (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit );
						if( !strcmp( ";",pbuf ) ){
							//printf( ";;;\n" );
							index++;
							break;
						}
					}
					if( index == tmpindex ){
						printf( "invalid index : %s\n", (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit ) );
						break;
					}
				}
			}
		}
	}while( false );
	return index;
}

int qs_parse_array_index( QS_MEMORY_POOL* _ppool, QS_SCRIPT* pscript, QS_NODE* node, int index )
{
	QS_TOKENS *ptokens = (QS_TOKENS*)QS_GET_POINTER( _ppool, pscript->tokens_munit );
	QS_TOKEN *token_list;
	QS_NODE *childnode;
	QS_NODE *stmtnode;
	int32_t childmunit;
	int32_t stmtmunit;
	int32_t tmpmunit;
	char* pbuf;
	int tmpindex;
	if( index >= ptokens->currentpos ){
		printf("invalid index11\n");
		return index;
	}
	token_list = ( QS_TOKEN* )QS_GET_POINTER( _ppool, ptokens->token_munit );
	tmpmunit = token_list[index].buf_munit;
	index++;
	if( index >= ptokens->currentpos ){
		printf("invalid index12\n");
		return index;
	}
	pbuf = (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit );
	do{
		childmunit = qs_addnodeelement( _ppool, node );
		childnode = (QS_NODE*)QS_GET_POINTER( _ppool, childmunit );
		qs_addelement( _ppool, childnode, ELEMENT_ARRAY_REFERENCE, tmpmunit );
		if( !strcmp( "[", pbuf ) )
		{
			stmtmunit = qs_addnodeelement( _ppool, childnode );
			stmtnode = (QS_NODE*)QS_GET_POINTER( _ppool, stmtmunit );
			index++;
			while( index < ptokens->currentpos )
			{
				tmpindex = index;
				if( token_list[index].type == ID_SIGN )
				{
					pbuf = (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit );
					if( !strcmp( pbuf, "]" ) ){
						index++;
						break;
					}
				}
				else if( token_list[index].type == ID_OP ){
					//printf("break : %s\n", (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit ));
					//break;
				}
				stmtnode = (QS_NODE*)QS_GET_POINTER( _ppool, stmtmunit );
				index = qs_parse_rel( _ppool, pscript, stmtnode, token_list, index );
//printf("qs_parse_rel : %s\n", (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit ));
				if( token_list[index].type == ID_OP )
				{
					pbuf = (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit );
					if( !strcmp( ">=",pbuf ) 
						|| !strcmp( "<=",pbuf )
						|| !strcmp( ">",pbuf )
						|| !strcmp( "<",pbuf )
						|| !strcmp( "||",pbuf )
						|| !strcmp( "&&",pbuf )
					)
					{
						stmtnode = (QS_NODE*)QS_GET_POINTER( _ppool, stmtmunit );
						qs_addelement( _ppool, stmtnode, ELEMENT_CMP, token_list[index].buf_munit );
						index++;
					}
				}
				if( token_list[index].type == ID_SIGN )
				{
					pbuf = (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit );
					if( !strcmp( pbuf, "]" ) ){
						index++;
						if( index >= ptokens->currentpos ){
							printf( "invalid index14\n" );
							break;
						}
						pbuf = (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit );
						if( !strcmp( pbuf, "[" ) ){
							childnode = (QS_NODE*)QS_GET_POINTER( _ppool, childmunit );
							stmtmunit = qs_addnodeelement( _ppool, childnode );
							stmtnode = (QS_NODE*)QS_GET_POINTER( _ppool, stmtmunit );
							index++;
						}
						else{
							//printf("else : %s\n", (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit ));
							break;
						}
					}
					else{
						//printf("break : %s\n", (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit ));
						index++;
						break;
					}
				}
				if( index == tmpindex ){
					printf( "invalid index16\n" );
					break;
				}
				if( token_list[index].type == ID_SIGN )
				{
					pbuf = (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit );
					if( !strcmp( pbuf, ";" ) ){
						break;
					}
				}
			}
		}
	}while( false );
	return index;
}

int qs_parse_if( QS_MEMORY_POOL* _ppool, QS_SCRIPT* pscript, QS_NODE* node, int index )
{
	char* pbuf;
	int tmpindex;
	int32_t tmpmunit;
	QS_TOKENS *ptokens = (QS_TOKENS*)QS_GET_POINTER( _ppool, pscript->tokens_munit );
	QS_TOKEN *token_list;
	QS_NODE *childnode;
	int32_t childmunit;
	QS_NODE *ifstmtnode;
	int32_t ifstmtmunit;
	QS_NODE *ifexprnode;
	int32_t ifexprmunit;
	token_list = ( QS_TOKEN* )QS_GET_POINTER( _ppool, ptokens->token_munit );
	childmunit = qs_addnodeelement( _ppool, node );
	childnode = (QS_NODE*)QS_GET_POINTER( _ppool, childmunit );
	qs_addelement( _ppool, childnode, ELEMENT_IF, token_list[index].buf_munit );
	index++;
	do{
		if( token_list[index].type != ID_SIGN )
		{
			break;
		}
		if( !strcmp( "(",(char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit ) ) )
		{
			index++;
			ifstmtmunit = qs_addnodeelement( _ppool, childnode );
			ifstmtnode = (QS_NODE*)QS_GET_POINTER( _ppool, ifstmtmunit );
			ifexprmunit = qs_addnodeelement( _ppool, ifstmtnode );
			ifexprnode = (QS_NODE*)QS_GET_POINTER( _ppool, ifexprmunit );
			while( index < ptokens->currentpos )
			{
				tmpindex = index;
				index = qs_parse_rel( _ppool, pscript, ifexprnode, token_list, index );
				if( token_list[index].type == ID_SIGN )
				{
					if( !strcmp( ")",(char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit ) ) ){
						index++;
						break;
					}
				}
				else if( token_list[index].type == ID_OP )
				{
					pbuf = (char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit );
					if( !strcmp ( "==", pbuf )
						|| !strcmp( ">=",pbuf ) 
						|| !strcmp( "<=",pbuf )
						|| !strcmp( ">",pbuf )
						|| !strcmp( "<",pbuf )
						|| !strcmp( "||",pbuf )
						|| !strcmp( "&&",pbuf )
					)
					{
						ifstmtnode = (QS_NODE*)QS_GET_POINTER( _ppool, ifstmtmunit );
						qs_addelement( _ppool, ifstmtnode, ELEMENT_CMP, token_list[index].buf_munit );
						index++;
						ifexprmunit = qs_addnodeelement( _ppool, ifstmtnode );
						ifexprnode = (QS_NODE*)QS_GET_POINTER( _ppool, ifexprmunit );
					}
				}
				if( index == tmpindex ){
					printf( "qs_parse_if invalid index\n" );
					break;
				}
			}
			if( index < ptokens->currentpos )
			{
				childnode = (QS_NODE*)QS_GET_POINTER( _ppool, childmunit );
				index = qs_parse_code_core( _ppool, pscript, childnode, index );
				if( index < 0 ){
					break;
				}
			}
		}
		else if( !strcmp( "{",(char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit ) ) )
		{
			childnode = (QS_NODE*)QS_GET_POINTER( _ppool, childmunit );
			if( index < ptokens->currentpos )
			{
				childnode = (QS_NODE*)QS_GET_POINTER( _ppool, childmunit );
				index = qs_parse_code_core( _ppool, pscript, childnode, index );
				if( index < 0 ){
					break;
				}
			}
		}
		if( token_list[index].type != ID_SYS_ELSE && token_list[index].type != ID_SYS_ELSEIF )
		{
			break;
		}
		else{
			tmpmunit = token_list[index].buf_munit;
			index++;
			if( token_list[index].type == ID_SYS_IF && token_list[index-1].type == ID_SYS_ELSE )
			{
				if( 0 >= ( tmpmunit = qs_create_memory_block( _ppool, 8 ) ) ){
					index = -1;
					break;
				}
				pbuf = (char*)QS_GET_POINTER( _ppool, tmpmunit );
				qs_strcopy( pbuf, "elseif", QS_PUNIT_USIZE( _ppool, tmpmunit ) );
				childnode = (QS_NODE*)QS_GET_POINTER( _ppool, childmunit );
				qs_addelement( _ppool, childnode, ELEMENT_ELSEIF, tmpmunit );
				index++;
			}
			else{
				if( token_list[index-1].type == ID_SYS_ELSE ){
					childnode = (QS_NODE*)QS_GET_POINTER( _ppool, childmunit );
					qs_addelement( _ppool, childnode, ELEMENT_ELSE, tmpmunit );
				}else{
					childnode = (QS_NODE*)QS_GET_POINTER( _ppool, childmunit );
					qs_addelement( _ppool, childnode, ELEMENT_ELSEIF, tmpmunit );
				}
			}
		}
	}while( index < ptokens->currentpos );
	return index;
}

int qs_parse_while( QS_MEMORY_POOL* _ppool, QS_SCRIPT* pscript, QS_NODE* node, int index )
{
	int tmpindex;
	QS_TOKENS *ptokens = (QS_TOKENS*)QS_GET_POINTER( _ppool, pscript->tokens_munit );
	QS_TOKEN *token_list;
	QS_NODE *childnode;
	int32_t childmunit;
	QS_NODE *whilestmtnode;
	int32_t whilestmtmunit;
	QS_NODE *whileexprnode;
	int32_t whileexprmunit;
	token_list = ( QS_TOKEN* )QS_GET_POINTER( _ppool, ptokens->token_munit );
	childmunit = qs_addnodeelement( _ppool, node );
	childnode = (QS_NODE*)QS_GET_POINTER( _ppool, childmunit );
	qs_addelement( _ppool, childnode, ELEMENT_WHILE, token_list[index].buf_munit );
	index++;
	do{
		if( token_list[index].type != ID_SIGN )
		{
			break;
		}
		if( !strcmp( "(",(char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit ) ) )
		{
			index++;
			childnode = (QS_NODE*)QS_GET_POINTER( _ppool, childmunit );
			whilestmtmunit = qs_addnodeelement( _ppool, childnode );
			whilestmtnode = (QS_NODE*)QS_GET_POINTER( _ppool, whilestmtmunit );
			whileexprmunit = qs_addnodeelement( _ppool, whilestmtnode );
			whileexprnode = (QS_NODE*)QS_GET_POINTER( _ppool, whileexprmunit );
			while( index < ptokens->currentpos )
			{
				tmpindex = index;
				index = qs_parse_rel( _ppool, pscript, whileexprnode, token_list, index );
				if( token_list[index].type == ID_SIGN )
				{
					if( !strcmp( ")",(char*)QS_GET_POINTER( _ppool, token_list[index].buf_munit ) ) ){
						index++;
						break;
					}
				}
				if( index == tmpindex ){
					printf( "invalid index18\n" );
					break;
				}
			}
			if( index < ptokens->currentpos )
			{
				childnode = (QS_NODE*)QS_GET_POINTER( _ppool, childmunit );
				index = qs_parse_code_core( _ppool, pscript, childnode, index );
				if( index < 0 ){
					break;
				}
			}
		}
	}while( index < ptokens->currentpos );
	return index;
}

void qs_exec( QS_MEMORY_POOL* _ppool, int32_t* p_unitid )
{
	QS_SCRIPT *pscript;
	QS_NODE *rootnode;
	QS_NODE *childnode;
	QS_NODE *node;
	int i;
	QS_NODE* workelemlist;
#ifdef __QS_DEBUG__
#ifdef __WINDOWS__
//	LARGE_INTEGER start_pc, end_pc, freq_pc;
//	double sec_pc;
//	QueryPerformanceFrequency( &freq_pc );
//	QueryPerformanceCounter( &start_pc );
#else
//	struct timeval start_timeval, end_timeval;
//	double sec_timeofday;
//	gettimeofday( &start_timeval, NULL );
#endif
#endif
	pscript = (QS_SCRIPT *)QS_GET_POINTER( _ppool, ( *p_unitid ) );
	rootnode = (QS_NODE*)QS_GET_POINTER( _ppool, pscript->rootnode_munit );
	node = rootnode;
	//qs_elementdump( _ppool, node );
	do{
		workelemlist = ( QS_NODE* )QS_GET_POINTER( _ppool, node->element_munit );
		if( workelemlist == NULL )
		{
			printf("workelemlist is null\n");
			break;
		}
		for( i = 0; i < node->pos; i++ )
		{
			childnode = (QS_NODE*)QS_GET_POINTER( _ppool, workelemlist[i].element_munit );
			qs_exec_core( _ppool, pscript, childnode );
		}
	}while( false );
#ifdef __QS_DEBUG__
#ifdef __WINDOWS__
//	QueryPerformanceCounter( &end_pc );
//	sec_pc = (end_pc.QuadPart - start_pc.QuadPart) / (double)freq_pc.QuadPart;
//	printf(  "## qs_exec time = %lf[s]\n", sec_pc );
#else
//	gettimeofday( &end_timeval, NULL );
//	sec_timeofday = (end_timeval.tv_sec - start_timeval.tv_sec)
//			+ (end_timeval.tv_usec - start_timeval.tv_usec) / 1000000.0;
//	printf(  "## qs_exec time = %lf[s]\n", sec_timeofday );
#endif
#endif
}

int32_t qs_exec_core( QS_MEMORY_POOL* _ppool, QS_SCRIPT *pscript, QS_NODE* node )
{
	int32_t returnmunit = -1;
	int32_t exec_expr_munit;
	QS_NODE* workelemlist;
	workelemlist = ( QS_NODE* )QS_GET_POINTER( _ppool, node->element_munit );
	switch( workelemlist[0].id )
	{
		case ELEMENT_VALIABLE:
		{
			QS_NODE* vnode = (QS_NODE*)QS_GET_POINTER( _ppool, workelemlist[1].element_munit );
			exec_expr_munit = qs_exec_expr( _ppool, pscript, vnode, pscript->int_cache_munit );
			if( exec_expr_munit > 0 )
			{
				QS_FUNCTION_RETURN* pfret = (QS_FUNCTION_RETURN*)QS_GET_POINTER( _ppool, exec_expr_munit );
				if( -1 != pfret->munit )
				{
					char* value = (char*)QS_GET_POINTER( _ppool, pfret->munit );
					size_t vsize = QS_PUNIT_USIZE(_ppool,pfret->munit);
					char* opname = (char*)QS_GET_POINTER( _ppool, workelemlist[2].element_munit );
					if( !strcmp( opname, "=" ) ){
						int32_t myhash = qs_get_hash( _ppool, pscript->v_hash_munit, (char*)QS_GET_POINTER( _ppool, workelemlist[0].element_munit ) );
						if( myhash > 0 && QS_PUNIT_USIZE(_ppool,myhash) >= vsize ){
							memcpy( (char*)(QS_GET_POINTER(_ppool,myhash)), value,vsize );
						}
						else{
							if( 0 >= ( myhash = qs_create_memory_block( _ppool, vsize ) ) ){
								break;
							}
							memcpy( (char*)(QS_GET_POINTER(_ppool,myhash)), value,vsize );
							qs_add_hash( _ppool, pscript->v_hash_munit, workelemlist[0].element_munit, myhash, pfret->id );
						}
					}
					else{
						printf("unsupported opname : %s\n", opname);
					}
				}
				else{
					printf("null value\n");
				}
			}
			else{
				printf("null result : %s \n", (char*)QS_GET_POINTER( _ppool, workelemlist[0].element_munit ));
			}
		}
		break;
		case ELEMENT_FUNCTION:
		{
			int32_t returnmunit = qs_exec_function( _ppool, pscript, node );
			if( returnmunit > 0 ){
				// TODO : check return
			}
		}
		break;
		case ELEMENT_IF:
		{
			int32_t returnmunit = qs_exec_if( _ppool, pscript, node );
			if( returnmunit > 0 ){
				// TODO : check return
			}
		}
		break;
		case ELEMENT_WHILE:
		{
			int32_t returnmunit = qs_exec_while( _ppool, pscript, node );
			if( returnmunit > 0 ){
				// TODO : check return
			}
		}
		break;
		case ELEMENT_CHILD:
		{
			int i;
			QS_NODE* childnode = NULL;
			for( i = 0; i < node->pos; i++ )
			{
				if( workelemlist[i].element_munit == -1 ){
					printf( "workelemlist[%d].element_munit == -1\n" , i );
				}
				else{
					childnode = (QS_NODE*)QS_GET_POINTER( _ppool, workelemlist[i].element_munit );
					if( childnode != NULL ){
						returnmunit = qs_exec_core( _ppool, pscript, childnode );
						if( returnmunit > 0 ){
							break;
						}
					}
					else{
						printf("child node is null\n");
					}
				}
			}
		}
		break;
		case ELEMENT_RETURN:
		{
			QS_NODE* vnode = (QS_NODE*)QS_GET_POINTER( _ppool, workelemlist[1].element_munit );
			returnmunit = qs_exec_expr( _ppool, pscript, vnode, 0 );
		}
		break;
		case ELEMENT_ARRAY_REFERENCE:
		{
			returnmunit = qs_exec_array_set( _ppool, pscript, node );
		}
		break;
		default:
			printf("unsupported element : %d\n", workelemlist[0].id );
		break;
	}
	return returnmunit;
}

int32_t qs_exec_function( QS_MEMORY_POOL* _ppool, QS_SCRIPT *pscript, QS_NODE* node )
{
	int32_t resultmunit = -1;
	int32_t funcmunit = -1;
	int32_t argsmunit = -1;
	int32_t exec_expr_munit;
	QS_NODE* vnode;
	QS_FUNCTION_RETURN* pret;
	QS_FUNCTION_INFO* pfuncinfo;
	QS_ARRAY* parray = NULL;
	int i;
	QS_NODE* workelemlist = ( QS_NODE* )QS_GET_POINTER( _ppool, node->element_munit );
	do{
		funcmunit = qs_get_hash( _ppool, pscript->f_hash_munit, (char*)QS_GET_POINTER( _ppool, workelemlist[0].element_munit ) );
		if( funcmunit <=0 ){
			printf("call undefined function : %s\n", (char*)QS_GET_POINTER( _ppool, workelemlist[0].element_munit ) );
			break;
		}
		if( node->pos > 1 )
		{
			for( i = 1; i < node->pos; i++ )
			{
				vnode = (QS_NODE*)QS_GET_POINTER( _ppool, workelemlist[i].element_munit );
				if( 0 >= vnode->element_munit ){
					continue;
				}
				QS_NODE* vworkelemlist = ( QS_NODE* )QS_GET_POINTER( _ppool, vnode->element_munit );
				if( vworkelemlist[0].id == ELEMENT_CHILD )
				{
					QS_NODE* cnode = (QS_NODE*)QS_GET_POINTER( _ppool, vworkelemlist[0].element_munit );
					QS_NODE* cworkelemlist = ( QS_NODE* )QS_GET_POINTER( _ppool, cnode->element_munit );
					if( cworkelemlist[0].id == ELEMENT_FUNCTION )
					{
						exec_expr_munit = qs_exec_expr( _ppool, pscript, vnode, 0 );
						if( exec_expr_munit > 0 ){
							QS_FUNCTION_RETURN* pfret = (QS_FUNCTION_RETURN*)QS_GET_POINTER( _ppool, exec_expr_munit );
							qs_array_push( _ppool, &argsmunit, pfret->id, pfret->munit );
						}
//						int32_t returnmunit = qs_exec_function( _ppool, pscript, cnode );
//						if( returnmunit > 0 ){
//							pret = (QS_FUNCTION_RETURN*)QS_GET_POINTER( _ppool, returnmunit );
//							if( pret != NULL )
//							{
//								qs_array_push( _ppool, &argsmunit, pret->id, pret->munit );
//							}
//						}
					}
					else if( cworkelemlist[0].id == ELEMENT_ARRAY_REFERENCE ){
						exec_expr_munit = qs_exec_expr( _ppool, pscript, vnode, 0 );
						if( exec_expr_munit > 0 ){
							QS_FUNCTION_RETURN* pfret = (QS_FUNCTION_RETURN*)QS_GET_POINTER( _ppool, exec_expr_munit );
							qs_array_push( _ppool, &argsmunit, pfret->id, pfret->munit );
						}
//						int32_t returnmunit = qs_exec_array_get( _ppool, pscript, cnode );
//						if( returnmunit > 0 ){
//							pret = (QS_FUNCTION_RETURN*)QS_GET_POINTER( _ppool, returnmunit );
//							if( pret != NULL )
//							{
//								qs_array_push( _ppool, &argsmunit, pret->id, pret->munit );
//							}
//						}
					}
				}
				else{
					exec_expr_munit = qs_exec_expr( _ppool, pscript, vnode, 0 );
					if( exec_expr_munit > 0 ){
						QS_FUNCTION_RETURN* pfret = (QS_FUNCTION_RETURN*)QS_GET_POINTER( _ppool, exec_expr_munit );
						qs_array_push( _ppool, &argsmunit, pfret->id, pfret->munit );
					}
				}
			}
		}
		pfuncinfo = (QS_FUNCTION_INFO*)QS_GET_POINTER( _ppool, funcmunit );
		if( pfuncinfo->type == QS_FUNCTION_TYPE_SYSTEM ){
			parray = (QS_ARRAY*)QS_GET_POINTER( _ppool, argsmunit );
			pret = (QS_FUNCTION_RETURN*)pfuncinfo->func( _ppool, parray );
			if( pret != NULL )
			{
				resultmunit = pret->refid;
			}
		}
		else if( pfuncinfo->type == QS_FUNCTION_TYPE_USER ){
			QS_NODE* function_node = NULL;
			QS_NODE* function_header_node = NULL;
			QS_NODE* function_body_node = NULL;
			QS_NODE* function_workelemlist = NULL;
			QS_NODE* function_headeremlist = NULL;
			QS_ARRAY_ELEMENT* elm = NULL;
			if( 0 >= pfuncinfo->userfunction_munit ){
				printf("null user function error\n");
				break;
			}
			function_node = (QS_NODE*)QS_GET_POINTER( _ppool, pfuncinfo->userfunction_munit );
			function_workelemlist = (QS_NODE*)QS_GET_POINTER( _ppool, function_node->element_munit );
			function_header_node = (QS_NODE*)QS_GET_POINTER( _ppool, function_workelemlist[0].element_munit );
			function_headeremlist = (QS_NODE*)QS_GET_POINTER( _ppool, function_header_node->element_munit );
			if( argsmunit > 0 )
			{
				parray = (QS_ARRAY*)QS_GET_POINTER( _ppool, argsmunit );
				if( parray != NULL ){
					elm = (QS_ARRAY_ELEMENT*)QS_GET_POINTER( _ppool, parray->memid );
				}
			}
			if( parray != NULL && elm != NULL )
			{
				if( function_header_node->pos-1 > parray->len || function_header_node->pos-1 < parray->len ){
					printf("warnning : argument count => need %d args : request %zd args\n" , function_header_node->pos-1, parray->len );
				}
				if( function_header_node->pos > 1 )
				{
					for( i = 1; i < function_header_node->pos; i++ )
					{
						if( parray->len > i-1 ){
							QS_NODE* argnode = (QS_NODE*)QS_GET_POINTER( _ppool, function_headeremlist[i].element_munit );
							QS_NODE* argelmlist = (QS_NODE*)QS_GET_POINTER( _ppool, argnode->element_munit );
							qs_add_hash_value( 
								_ppool
								, pscript->v_hash_munit
								, (char*)QS_GET_POINTER( _ppool, argelmlist[0].element_munit )
								, (char*)QS_GET_POINTER( _ppool, elm[i-1].memid_array_element_data )
								, elm[i-1].id
							);
						}
					}
				}
			}
			else{
				if( function_header_node->pos > 1 ){
					printf("warnning : argument count => need %d args : request 0 args\n" , function_header_node->pos-1 );
				}
			}
			function_body_node = (QS_NODE*)QS_GET_POINTER( _ppool, function_workelemlist[1].element_munit );
			resultmunit = qs_exec_core( _ppool, pscript, function_body_node );
		}
	}while( false );
	return resultmunit;
}

int32_t qs_exec_array_set( QS_MEMORY_POOL* _ppool, QS_SCRIPT *pscript, QS_NODE* node )
{
	if( node->pos <= 1 ){
		return -1;
	}
	QS_NODE* rootlist = ( QS_NODE* )QS_GET_POINTER( _ppool, node->element_munit );
	QS_NODE* right_node = (QS_NODE*)QS_GET_POINTER( _ppool, rootlist[1].element_munit );
	QS_NODE* right_list = ( QS_NODE* )QS_GET_POINTER( _ppool, right_node->element_munit );
	QS_NODE tmp_node;
	tmp_node.id = 0;
	tmp_node.element_munit = -1;
	char* hashname = (char*)QS_GET_POINTER( _ppool, right_list[0].element_munit );
	int32_t hash_id = qs_get_hash_id( _ppool, pscript->v_hash_munit, hashname );
	if( hash_id == ELEMENT_ARRAY|| hash_id == ELEMENT_HASH ){
		tmp_node.id = hash_id;
		tmp_node.element_munit = qs_get_hash( _ppool, pscript->v_hash_munit, hashname );
	}
	int i;
	for( i = 1; i < right_node->pos; i++ ){
		QS_FUNCTION_RETURN* pret = qs_exec_get_array_offset(_ppool,pscript,right_node,i);
		if(pret==NULL){
			printf("null offset\n");
			return -1;
		}
		if( tmp_node.id == ELEMENT_ARRAY ){
			int array_index = -1;
			QS_ARRAY *parray = (QS_ARRAY*)QS_GET_POINTER( _ppool, tmp_node.element_munit );
			if( parray == NULL || parray->len <= 0 ){
				printf("clean array\n");
				break;
			}
			array_index = atoi( (char*)QS_GET_POINTER( _ppool, pret->munit ) );
			if( array_index >= 0 && array_index < parray->len ){
				if( parray == NULL ){
					printf("invalid array access\n");
				}
				else{
					QS_ARRAY_ELEMENT* retelm = (QS_ARRAY_ELEMENT*)QS_GET_POINTER( _ppool, parray->memid )+( array_index );
					tmp_node.id =  retelm->id;
					tmp_node.element_munit = retelm->memid_array_element_data;
					if(i==right_node->pos-1){
						QS_NODE* left_node = (QS_NODE*)QS_GET_POINTER( _ppool, rootlist[2].element_munit );
						int32_t exec_expr_munit = qs_exec_expr( _ppool, pscript, left_node, pscript->int_cache_munit );
						if( exec_expr_munit > 0 ){
							QS_FUNCTION_RETURN* pfret = (QS_FUNCTION_RETURN*)QS_GET_POINTER( _ppool, exec_expr_munit );
							if( -1 != pfret->munit ){
								if(pfret->id==ELEMENT_LITERAL_NUM&&retelm->id==ELEMENT_LITERAL_NUM){
									memcpy(
										(uint8_t*)QS_GET_POINTER(_ppool,retelm->memid_array_element_data),
										(uint8_t*)QS_GET_POINTER(_ppool,pfret->munit),
										QS_PUNIT_USIZE(_ppool,retelm->memid_array_element_data)
									);
								}
								else{
									printf("unsupported\n");
								}
							}
						}
					}
				}
			}
			else{
				printf("out of array range\n");
			}
		}
		else if( tmp_node.id == ELEMENT_HASH )
		{
			tmp_node.id = qs_get_hash_id( _ppool, tmp_node.element_munit, (char*)QS_GET_POINTER(_ppool,pret->munit) );
			tmp_node.element_munit = qs_get_hash( _ppool, tmp_node.element_munit, (char*)QS_GET_POINTER(_ppool,pret->munit) );
			printf("unsupported\n");
		}
	}
	return -1;
}

QS_FUNCTION_RETURN* qs_exec_get_array_offset(QS_MEMORY_POOL* _ppool, QS_SCRIPT *pscript, QS_NODE* right_node, int i)
{
	QS_NODE* right_list = ( QS_NODE* )QS_GET_POINTER( _ppool, right_node->element_munit );
	QS_NODE* vnode = (QS_NODE*)QS_GET_POINTER( _ppool, right_list[i].element_munit );
	QS_NODE* vworkelemlist = ( QS_NODE* )QS_GET_POINTER( _ppool, vnode->element_munit );
	if( vworkelemlist[0].id == ELEMENT_CHILD ){
		QS_NODE* cnode = (QS_NODE*)QS_GET_POINTER( _ppool, vworkelemlist[0].element_munit );
		QS_NODE* cworkelemlist = ( QS_NODE* )QS_GET_POINTER( _ppool, cnode->element_munit );
		if( cworkelemlist[0].id == ELEMENT_FUNCTION ){
			int32_t returnmunit = qs_exec_function( _ppool, pscript, cnode );
			if( returnmunit > 0 ){
				return (QS_FUNCTION_RETURN*)QS_GET_POINTER( _ppool, returnmunit );
			}
		}
		else if( cworkelemlist[0].id == ELEMENT_ARRAY ){
			int32_t returnmunit = qs_exec_array_create( _ppool, pscript, cnode );
			if( returnmunit > 0 ){
				return (QS_FUNCTION_RETURN*)QS_GET_POINTER( _ppool, returnmunit );
			}
		}
	}
	else{
		if( vworkelemlist[0].id == ELEMENT_FUNCTION ){
			int32_t returnmunit = qs_exec_function( _ppool, pscript, vnode );
			if( returnmunit > 0 ){
				return (QS_FUNCTION_RETURN*)QS_GET_POINTER( _ppool, returnmunit );
			}
		}
		else if( vworkelemlist[0].id == ELEMENT_ARRAY ){
			int32_t returnmunit = qs_exec_array_create( _ppool, pscript, vnode );
			if( returnmunit > 0 ){
				return (QS_FUNCTION_RETURN*)QS_GET_POINTER( _ppool, returnmunit );
			}
		}
		else{
			int32_t exec_expr_munit = qs_exec_expr( _ppool, pscript, vnode, 0 );
			if( exec_expr_munit > 0 ){
				return (QS_FUNCTION_RETURN*)QS_GET_POINTER( _ppool, exec_expr_munit );
			}
		}
	}
	return NULL;
}

int32_t qs_exec_array_create( QS_MEMORY_POOL* _ppool, QS_SCRIPT *pscript, QS_NODE* node )
{
	int32_t resultmunit = -1;
	int32_t argsmunit = -1;
	int32_t hash_munit = -1;
	QS_NODE* vnode;
	QS_NODE* workelemlist;
	QS_NODE* vworkelemlist;
	QS_NODE* cworkelemlist;
	QS_NODE* cnode;
	QS_FUNCTION_RETURN* pret;
	workelemlist = ( QS_NODE* )QS_GET_POINTER( _ppool, node->element_munit );
	if( node->pos > 1 )
	{
		int i;
		for( i = 1; i < node->pos; i++ )
		{
			vnode = (QS_NODE*)QS_GET_POINTER( _ppool, workelemlist[i].element_munit );
			if( 0 >= vnode->element_munit ){
				continue;
			}
			vworkelemlist = ( QS_NODE* )QS_GET_POINTER( _ppool, vnode->element_munit );
			if( vworkelemlist[0].id == ELEMENT_CHILD )
			{
				cnode = (QS_NODE*)QS_GET_POINTER( _ppool, vworkelemlist[0].element_munit );
				cworkelemlist = ( QS_NODE* )QS_GET_POINTER( _ppool, cnode->element_munit );
				if( cworkelemlist[0].id == ELEMENT_FUNCTION )
				{
					int32_t returnmunit = qs_exec_function( _ppool, pscript, cnode );
					if( returnmunit > 0 ){
						pret = (QS_FUNCTION_RETURN*)QS_GET_POINTER( _ppool, returnmunit );
						if( pret != NULL )
						{
							qs_array_push( _ppool, &argsmunit, pret->id, pret->munit );
						}
					}
				}
				else if( cworkelemlist[0].id == ELEMENT_ARRAY ){
					int32_t returnmunit = qs_exec_array_create( _ppool, pscript, cnode );
					if( returnmunit > 0 ){
						pret = (QS_FUNCTION_RETURN*)QS_GET_POINTER( _ppool, returnmunit );
						if( pret != NULL )
						{
							qs_array_push( _ppool, &argsmunit, pret->id, pret->munit );
						}
					}
				}
				else{
					int32_t exec_expr_munit = qs_exec_expr( _ppool, pscript, cnode, 0 );
					if( exec_expr_munit > 0 ){
						pret = (QS_FUNCTION_RETURN*)QS_GET_POINTER( _ppool, exec_expr_munit );
						if( vnode->pos == 1 ){
							qs_array_push( _ppool, &argsmunit, pret->id, pret->munit );
						}
						else{
							cnode = (QS_NODE*)QS_GET_POINTER( _ppool, vworkelemlist[1].element_munit );
							cworkelemlist = ( QS_NODE* )QS_GET_POINTER( _ppool, cnode->element_munit );
							QS_FUNCTION_RETURN tmpret;
							tmpret.munit = pret->munit;
							tmpret.id = pret->id;
							int32_t hash_value_exec_expr_munit = exec_expr_munit = qs_exec_expr( _ppool, pscript, cnode, 0 );
							if( hash_value_exec_expr_munit > 0 )
							{
								if( hash_munit <= 0){
									if( 0 >= ( hash_munit = qs_create_hash( _ppool, pscript->hash_alloc_size ) ) ){
										break;
									}
								}
								QS_FUNCTION_RETURN* pfhret = (QS_FUNCTION_RETURN*)QS_GET_POINTER( _ppool, hash_value_exec_expr_munit );
								qs_add_hash( _ppool, hash_munit, tmpret.munit, pfhret->munit, pfhret->id );
							}
						}
					}
				}
			}
		}
	}
	else{
		if( -1 == ( argsmunit = qs_create_array( _ppool, QS_ARRAY_SIZE_DEFAULT ) ) ){
			printf("empty?\n");
		}
	}
	if( hash_munit > 0 ){
		resultmunit = qs_create_return(_ppool, pscript, hash_munit, ELEMENT_HASH );
	}
	else{
		resultmunit = qs_create_return(_ppool, pscript, argsmunit, ELEMENT_ARRAY );
	}
	return resultmunit;
}

int32_t qs_exec_array_get( QS_MEMORY_POOL* _ppool, QS_SCRIPT *pscript, QS_NODE* node )
{
	char* hashname;
	int32_t resultmunit = -1;
	int32_t returnmunit;
	QS_FUNCTION_RETURN* pret = NULL;
	QS_NODE* workelemlist = ( QS_NODE* )QS_GET_POINTER( _ppool, node->element_munit );
	QS_NODE tmp_node;
	tmp_node.id = 0;
	tmp_node.element_munit = -1;
	hashname = (char*)QS_GET_POINTER( _ppool, workelemlist[0].element_munit );
	if( node->pos > 1 )
	{
		int32_t hash_id = qs_get_hash_id( _ppool, pscript->v_hash_munit, hashname );
		if( hash_id == ELEMENT_ARRAY|| hash_id == ELEMENT_HASH ){
			tmp_node.id = hash_id;
			tmp_node.element_munit = qs_get_hash( _ppool, pscript->v_hash_munit, hashname );
		}
		int i;
		QS_NODE* vnode;
		QS_NODE* vworkelemlist;
		for( i = 1; i < node->pos; i++ )
		{
			vnode = (QS_NODE*)QS_GET_POINTER( _ppool, workelemlist[i].element_munit );
			if( 0 >= vnode->element_munit ){
				continue;
			}
			vworkelemlist = ( QS_NODE* )QS_GET_POINTER( _ppool, vnode->element_munit );
			if( vworkelemlist[0].id == ELEMENT_CHILD )
			{
				QS_NODE* cnode = (QS_NODE*)QS_GET_POINTER( _ppool, vworkelemlist[0].element_munit );
				QS_NODE* cworkelemlist = ( QS_NODE* )QS_GET_POINTER( _ppool, cnode->element_munit );
				if( cworkelemlist[0].id == ELEMENT_FUNCTION )
				{
					returnmunit = qs_exec_function( _ppool, pscript, cnode );
					if( returnmunit > 0 ){
						pret = (QS_FUNCTION_RETURN*)QS_GET_POINTER( _ppool, returnmunit );
					}
				}
				else if( cworkelemlist[0].id == ELEMENT_ARRAY ){
					returnmunit = qs_exec_array_create( _ppool, pscript, cnode );
					if( returnmunit > 0 ){
						pret = (QS_FUNCTION_RETURN*)QS_GET_POINTER( _ppool, returnmunit );
					}
				}
			}
			else{
				if( vworkelemlist[0].id == ELEMENT_FUNCTION )
				{
					returnmunit = qs_exec_function( _ppool, pscript, vnode );
					if( returnmunit > 0 ){
						pret = (QS_FUNCTION_RETURN*)QS_GET_POINTER( _ppool, returnmunit );
					}
				}
				else if( vworkelemlist[0].id == ELEMENT_ARRAY ){
					returnmunit = qs_exec_array_create( _ppool, pscript, vnode );
					if( returnmunit > 0 ){
						pret = (QS_FUNCTION_RETURN*)QS_GET_POINTER( _ppool, returnmunit );
					}
				}
				else if( vworkelemlist[0].id == ELEMENT_ARRAY_REFERENCE ){
					QS_NODE* cnode = (QS_NODE*)QS_GET_POINTER( _ppool, vworkelemlist[1].element_munit );
					int32_t exec_expr_munit = qs_exec_expr( _ppool, pscript, cnode, 0 );
					if( exec_expr_munit > 0 ){
						pret = (QS_FUNCTION_RETURN*)QS_GET_POINTER( _ppool, exec_expr_munit );
					}
				}
				else{
					int32_t exec_expr_munit = qs_exec_expr( _ppool, pscript, vnode, 0 );
					if( exec_expr_munit > 0 ){
						pret = (QS_FUNCTION_RETURN*)QS_GET_POINTER( _ppool, exec_expr_munit );
					}
				}
			}
			if( tmp_node.id == ELEMENT_ARRAY )
			{
				int array_index = -1;
				QS_ARRAY *parray = (QS_ARRAY*)QS_GET_POINTER( _ppool, tmp_node.element_munit );
				if( parray == NULL || parray->len <= 0 ){
					printf("clean array\n");
					break;
				}
				if( pret != NULL ){
					array_index = atoi( (char*)QS_GET_POINTER( _ppool, pret->munit ) );
					pret = NULL;
				}
				if( array_index >= 0 && array_index < parray->len ){
					if( parray == NULL ){
						printf("invalid array access\n");
					}
					else{
						QS_ARRAY_ELEMENT* retelm = (QS_ARRAY_ELEMENT*)QS_GET_POINTER( _ppool, parray->memid )+( array_index );
						tmp_node.id =  retelm->id;
						tmp_node.element_munit = retelm->memid_array_element_data;
					}
				}
				else{
					printf("out of array range\n");
				}
			}
			else if( tmp_node.id == ELEMENT_HASH )
			{
				if( pret != NULL ){
					tmp_node.id = qs_get_hash_id( _ppool, tmp_node.element_munit, (char*)QS_GET_POINTER(_ppool,pret->munit) );
					tmp_node.element_munit = qs_get_hash( _ppool, tmp_node.element_munit, (char*)QS_GET_POINTER(_ppool,pret->munit) );
				}
			}
		}
	}
	if( tmp_node.element_munit > 0 ){
		resultmunit = qs_create_return(_ppool, pscript, tmp_node.element_munit, tmp_node.id );
	}
	return resultmunit;
}

int32_t qs_exec_expr( QS_MEMORY_POOL* _ppool, QS_SCRIPT *pscript, QS_NODE* node, int32_t result_cache_munit )
{
	QS_NODE *workelemlist;
	QS_NODE *tmpnode;
	int32_t tmpmunit;
	int is_increment = 0;
	int is_decrement = 0;
	int is_numeric = 1;
	char* pbuf;
	char* opbuf;
	int32_t tmpv1, tmpv2;
	size_t csize = 0;
	int32_t tmpid = 0;
	int32_t workingmunit = pscript->s_hash_munit;//-1;
	workelemlist = ( QS_NODE* )QS_GET_POINTER( _ppool, node->element_munit );
	int i;
	for( i = 0; i < node->pos; i++ )
	{
		tmpnode = &workelemlist[i];
		if( tmpnode->id == ELEMENT_LITERAL_NUM || tmpnode->id == ELEMENT_LITERAL_STR )
		{
			qs_array_push( _ppool, &workingmunit, tmpnode->id, tmpnode->element_munit );
			if( tmpnode->id != ELEMENT_LITERAL_NUM ){
				is_numeric = 0;
			}
		}
		else if( tmpnode->id == ELEMENT_INCREMENT_AFTER )
		{
			is_increment = ELEMENT_INCREMENT_AFTER;
		}
		else if( tmpnode->id == ELEMENT_DECREMENT_AFTER)
		{
			is_decrement = ELEMENT_DECREMENT_AFTER;
		}
		else if( tmpnode->id == ELEMENT_VALIABLE )
		{
			char* pname = (char*)QS_GET_POINTER( _ppool, tmpnode->element_munit );
			int32_t tmphashmunit = qs_get_hash( _ppool, pscript->v_hash_munit, pname );
			csize = 0;
			tmpid = 0;
			if( tmphashmunit > 0 ){
				csize = QS_PUNIT_USIZE( _ppool, tmphashmunit );
				tmpid = qs_get_hash_id( _ppool, pscript->v_hash_munit, pname );
				qs_array_push( _ppool, &workingmunit, tmpid, tmphashmunit );
				if(tmpid == ELEMENT_LITERAL_NUM){
					if(is_increment==ELEMENT_INCREMENT_AFTER){
						NUMERIC_CAST* pv = (NUMERIC_CAST*)QS_PNUMERIC( _ppool, tmphashmunit );
						NUMERIC_CAST v = *pv;
						qs_add_hash_integer( _ppool, pscript->v_hash_munit, pname, v+1 );
					}
					else if(is_decrement==ELEMENT_DECREMENT_AFTER){
						NUMERIC_CAST* pv = (NUMERIC_CAST*)QS_PNUMERIC( _ppool, tmphashmunit );
						NUMERIC_CAST v = *pv;
						qs_add_hash_integer( _ppool, pscript->v_hash_munit, pname, v-1 );
					}
				}
			}
			else{
				int32_t* pi;
				csize = NUMERIC_BUFFER_SIZE;
				tmpid = ELEMENT_LITERAL_NUM;
				if( 0 >= ( tmpmunit = qs_create_memory_block( _ppool, csize ) ) ){
					return QS_SYSTEM_ERROR;
				}
				pbuf = (char*)QS_GET_POINTER( _ppool, tmpmunit );
				pi = QS_PINT32(_ppool, tmpmunit);
				pbuf[0] = '0';
				pbuf[1] = '\0';
				*pi = 0;
				qs_array_push( _ppool, &workingmunit, tmpid, tmpmunit );
			}
			if( tmpid != ELEMENT_LITERAL_NUM ){
				is_numeric = 0;
			}
		}
		else if( tmpnode->id == ELEMENT_ARRAY_REFERENCE )
		{
			int32_t returnmunit = qs_exec_array_get( _ppool, pscript, tmpnode );
			if( returnmunit > 0 ){
				QS_FUNCTION_RETURN* pret = (QS_FUNCTION_RETURN*)QS_GET_POINTER( _ppool, returnmunit );
				if( pret != NULL )
				{
					qs_array_push( _ppool, &workingmunit, pret->id, pret->munit );
				}
			}
		}
		else if( tmpnode->id == ELEMENT_CHILD )
		{
			QS_NODE* cnode = ( QS_NODE* )QS_GET_POINTER( _ppool, tmpnode->element_munit );
			QS_NODE* cworkelemlist = ( QS_NODE* )QS_GET_POINTER( _ppool, cnode->element_munit );
			if( cworkelemlist[0].id == ELEMENT_ARRAY ){
				int32_t returnmunit = qs_exec_array_create( _ppool, pscript, cnode );
				if( returnmunit > 0 ){
					QS_FUNCTION_RETURN* pret = (QS_FUNCTION_RETURN*)QS_GET_POINTER( _ppool, returnmunit );
					if( pret != NULL )
					{
						qs_array_push( _ppool, &workingmunit, pret->id, pret->munit );
					}
				}
			}
			else if( cworkelemlist[0].id == ELEMENT_ARRAY_REFERENCE )
			{
				int32_t returnmunit = qs_exec_array_get( _ppool, pscript, cnode );
				if( returnmunit > 0 ){
					QS_FUNCTION_RETURN* pret = (QS_FUNCTION_RETURN*)QS_GET_POINTER( _ppool, returnmunit );
					if( pret != NULL )
					{
						qs_array_push( _ppool, &workingmunit, pret->id, pret->munit );
					}
				}
			}
			else if( cworkelemlist[0].id == ELEMENT_FUNCTION )
			{
				int32_t returnmunit = qs_exec_function( _ppool, pscript, cnode );
				if( returnmunit > 0 ){
					QS_FUNCTION_RETURN* pret = (QS_FUNCTION_RETURN*)QS_GET_POINTER( _ppool, returnmunit );
					if( pret != NULL )
					{
						qs_array_push( _ppool, &workingmunit, pret->id, pret->munit );
					}
				}
			}
			else if( cworkelemlist[0].id == ELEMENT_LITERAL_NUM || cworkelemlist[0].id == ELEMENT_LITERAL_STR )
			{
				qs_array_push( _ppool, &workingmunit, cworkelemlist[0].id, cworkelemlist[0].element_munit );
				if( cworkelemlist[0].id != ELEMENT_LITERAL_NUM ){
					is_numeric = 0;
				}
			}
			else if( cworkelemlist[0].id == ELEMENT_VALIABLE )
			{
				char* pname = (char*)QS_GET_POINTER( _ppool, cworkelemlist[0].element_munit );
				int32_t tmphashmunit = qs_get_hash( _ppool, pscript->v_hash_munit, pname );
				csize = 0;
				tmpid = 0;
				if( tmphashmunit > 0 ){
					csize = QS_PUNIT_USIZE( _ppool, tmphashmunit );
					tmpid = qs_get_hash_id( _ppool, pscript->v_hash_munit, pname );
					qs_array_push( _ppool, &workingmunit, tmpid, tmphashmunit );
				}
				else{
					int32_t* pi;
					csize = NUMERIC_BUFFER_SIZE;
					tmpid = ELEMENT_LITERAL_NUM;
					if( 0 >= ( tmpmunit = qs_create_memory_block( _ppool, csize ) ) ){
						return QS_SYSTEM_ERROR;
					}
					pbuf = (char*)QS_GET_POINTER( _ppool, tmpmunit );
					pi = QS_PINT32(_ppool, tmpmunit);
					pbuf[0] = '0';
					pbuf[1] = '\0';
					*pi = 0;
					qs_array_push( _ppool, &workingmunit, tmpid, tmpmunit );
				}
				if( tmpid != ELEMENT_LITERAL_NUM ){
					is_numeric = 0;
				}
			}
			else{
				printf("other...\n");
			}
		}
		else if( tmpnode->id == ELEMENT_OP )
		{
			opbuf = (char*)QS_GET_POINTER( _ppool, tmpnode->element_munit );
			if( is_numeric == 1 )
			{
				int array_len = qs_array_length( _ppool, workingmunit );
				if( array_len == 0 ){
					printf( "empty stack error\n" );
					return QS_SYSTEM_ERROR;
				}
				else if( array_len == 1 ){
					if( !strcmp( opbuf, "-" ) )
					{
						QS_ARRAY_ELEMENT* pelm;
						pelm = qs_array_pop( _ppool, workingmunit );
						pbuf = (char*)QS_GET_POINTER( _ppool, pelm->memid_array_element_data );
						tmpv1 = atoi( pbuf );
						tmpv1 = -tmpv1;
					}
					else if( !strcmp( opbuf, "+" ) )
					{
						QS_ARRAY_ELEMENT* pelm;
						pelm = qs_array_pop( _ppool, workingmunit );
						pbuf = (char*)QS_GET_POINTER( _ppool, pelm->memid_array_element_data );
						tmpv1 = atoi( pbuf );
					}
					else{
						printf( "invalid op error\n" );
						return QS_SYSTEM_ERROR;
					}
				}
				else{
					QS_ARRAY_ELEMENT* pelm;
					pelm = qs_array_pop( _ppool, workingmunit );
					tmpv1 = QS_INT32(_ppool, pelm->memid_array_element_data);
					pelm = qs_array_pop( _ppool, workingmunit );
					tmpv2 = QS_INT32(_ppool, pelm->memid_array_element_data);
					if( !strcmp( opbuf, "+" ) )
					{
						tmpv1 = tmpv2+tmpv1;
					}
					else if( !strcmp( opbuf, "-" ) )
					{
						tmpv1 = tmpv2-tmpv1;
					}
					else if( !strcmp( opbuf, "*" ) )
					{
						tmpv1 = tmpv2*tmpv1;
					}
					else if( !strcmp( opbuf, "/" ) )
					{
						tmpv1 = tmpv2/tmpv1;
					}
					else if( !strcmp( opbuf, "%" ) )
					{
						tmpv1 = tmpv2%tmpv1;
					}
					else if( !strcmp( opbuf, "==" ) )
					{
						tmpv1 = ( tmpv2 == tmpv1 ) ? 1 : 0;
					}
					else if( !strcmp( opbuf, ">=" ) )
					{
						tmpv1 = ( tmpv2 >= tmpv1 ) ? 1 : 0;
					}
					else if( !strcmp( opbuf, "<=" ) )
					{
						tmpv1 = ( tmpv2 <= tmpv1 ) ? 1 : 0;
					}
					else if( !strcmp( opbuf, ">" ) )
					{
						tmpv1 = ( tmpv2 > tmpv1 ) ? 1 : 0;
					}
					else if( !strcmp( opbuf, "<" ) )
					{
						tmpv1 = ( tmpv2 < tmpv1 ) ? 1 : 0;
					}
					else if( !strcmp( opbuf, "!=" ) )
					{
						tmpv1 = ( tmpv2 != tmpv1 ) ? 1 : 0;
					}
					else if( !strcmp( opbuf, "&&" ) )
					{
						tmpv1 = ( tmpv2 && tmpv1 ) ? 1 : 0;
					}
					else if( !strcmp( opbuf, "||" ) )
					{
						tmpv1 = ( tmpv2 || tmpv1 ) ? 1 : 0;
					}
					else{
						return QS_SYSTEM_ERROR;
					}
				}
				qs_array_push_integer( _ppool, &workingmunit, tmpv1 );
			}
			else{
				QS_ARRAY_ELEMENT *pelm1, *pelm2;
				pelm1 = qs_array_pop( _ppool, workingmunit );
				pelm2 = qs_array_pop( _ppool, workingmunit );
				if( pelm1 == NULL && pelm2 == NULL)
				{
					printf( "invalid element error\n" );
					return QS_SYSTEM_ERROR;
				}
				else if( pelm1 == NULL )
				{
					printf( "invalid element error\n" );
					return QS_SYSTEM_ERROR;
				}
				else{
					//if( !strcmp( opbuf, "+" ) )
					if( *opbuf == '+' && *(opbuf+1) == '\0' )
					{
						if( pelm2 == NULL )
						{
							tmpmunit = qs_create_memory_block( _ppool, QS_PUNIT_USIZE( _ppool, pelm1->memid_array_element_data ) + 5 );
							if( tmpmunit < 0 ){
								return QS_SYSTEM_ERROR;
							}
							pbuf = (char*)QS_GET_POINTER( _ppool, tmpmunit );
							snprintf( pbuf, QS_PUNIT_USIZE( _ppool, tmpmunit ), "%s%s", (char*)QS_GET_POINTER( _ppool, pelm1->memid_array_element_data ), "NULL" );
							qs_array_push( _ppool, &workingmunit, ELEMENT_LITERAL_STR, tmpmunit );
						}
						else{
							if( pelm2->id == ELEMENT_LITERAL_BIN || pelm1->id == ELEMENT_LITERAL_BIN )
							{
								size_t s1, s2;
								if( pelm2->id == ELEMENT_LITERAL_BIN ){
									s1=QS_PUNIT_USIZE( _ppool, pelm2->memid_array_element_data );
								}
								else{
									s1=qs_strlen( (char*)QS_GET_POINTER( _ppool, pelm2->memid_array_element_data ) );
								}
								if( pelm1->id == ELEMENT_LITERAL_BIN ){
									s2=QS_PUNIT_USIZE( _ppool, pelm1->memid_array_element_data );
								}
								else{
									s2=qs_strlen( (char*)QS_GET_POINTER( _ppool, pelm1->memid_array_element_data ) );
								}
								tmpmunit = qs_create_memory_block( _ppool, s1 + s2 );
								if( tmpmunit < 0 ){
									return QS_SYSTEM_ERROR;
								}
								pbuf = (char*)QS_GET_POINTER( _ppool, tmpmunit );
								if( pelm2->id == ELEMENT_LITERAL_BIN ){
									memcpy( pbuf, (char*)QS_GET_POINTER( _ppool, pelm2->memid_array_element_data ), QS_PUNIT_USIZE( _ppool, pelm2->memid_array_element_data ) );
									pbuf+=QS_PUNIT_USIZE( _ppool, pelm2->memid_array_element_data );
								}
								else{
									memcpy( pbuf, (char*)QS_GET_POINTER( _ppool, pelm2->memid_array_element_data ), qs_strlen( (char*)QS_GET_POINTER( _ppool, pelm2->memid_array_element_data ) ) );
									pbuf+=qs_strlen( (char*)QS_GET_POINTER( _ppool, pelm2->memid_array_element_data ) );
								}
								if( pelm1->id == ELEMENT_LITERAL_BIN ){
									memcpy( pbuf, (char*)QS_GET_POINTER( _ppool, pelm1->memid_array_element_data ), QS_PUNIT_USIZE( _ppool, pelm1->memid_array_element_data ) );
									pbuf+=QS_PUNIT_USIZE( _ppool, pelm1->memid_array_element_data );
								}
								else{
									memcpy( pbuf, (char*)QS_GET_POINTER( _ppool, pelm1->memid_array_element_data ), qs_strlen( (char*)QS_GET_POINTER( _ppool, pelm1->memid_array_element_data ) ) );
									pbuf+=qs_strlen( (char*)QS_GET_POINTER( _ppool, pelm1->memid_array_element_data ) );
								}
								*pbuf='\0';
							}
							else{
								tmpmunit = qs_create_memory_block( _ppool, QS_PUNIT_USIZE( _ppool, pelm1->memid_array_element_data ) + QS_PUNIT_USIZE( _ppool, pelm2->memid_array_element_data ) );
								if( tmpmunit < 0 ){
									return QS_SYSTEM_ERROR;
								}
								pbuf = (char*)QS_GET_POINTER( _ppool, tmpmunit );
								size_t elm1_len = qs_strlen((char*)QS_GET_POINTER( _ppool, pelm1->memid_array_element_data ));
								size_t elm2_len = qs_strlen((char*)QS_GET_POINTER( _ppool, pelm2->memid_array_element_data ));
								qs_strcopy( pbuf, (char*)QS_GET_POINTER( _ppool, pelm2->memid_array_element_data ), QS_PUNIT_USIZE( _ppool, tmpmunit ) );
								qs_strcopy( pbuf+elm2_len, (char*)QS_GET_POINTER( _ppool, pelm1->memid_array_element_data ), QS_PUNIT_USIZE( _ppool, tmpmunit ) );
								*( pbuf+elm2_len+elm1_len ) = '\0';
							}
							qs_array_push( _ppool, &workingmunit, ELEMENT_LITERAL_STR, tmpmunit );
						}
					}
					else if( !strcmp( opbuf, "==" ) ){
						if( pelm2 == NULL )
						{
							tmpv1 = 0;
						}
						else{
							tmpv1 = !strcmp( (char*)QS_GET_POINTER( _ppool, pelm2->memid_array_element_data ), (char*)QS_GET_POINTER( _ppool, pelm1->memid_array_element_data ) );
						}
						qs_array_push_integer( _ppool, &workingmunit, tmpv1 );
					}
					else if( !strcmp( opbuf, "!=" ) ){
						if( pelm2 == NULL )
						{
							tmpv1 = 1;
						}
						else{
							tmpv1 = strcmp( (char*)QS_GET_POINTER( _ppool, pelm2->memid_array_element_data ), (char*)QS_GET_POINTER( _ppool, pelm1->memid_array_element_data ) ) ? 0 : 1;
						}
						qs_array_push_integer( _ppool, &workingmunit, tmpv1 );
					}else{
						return QS_SYSTEM_ERROR;
					}
				}
			}
		}
		else if( tmpnode->id == ELEMENT_OP_LITERAL_MINUS ){
			opbuf = (char*)QS_GET_POINTER( _ppool, tmpnode->element_munit );
			if( is_numeric == 1 )
			{
				if( qs_array_length( _ppool, workingmunit ) == 0 ){
					printf( "empty stack error\n" );
					return QS_SYSTEM_ERROR;
				}
				else{
					QS_ARRAY_ELEMENT* pelm = qs_array_pop( _ppool, workingmunit );
					pbuf = (char*)QS_GET_POINTER( _ppool, pelm->memid_array_element_data );
					tmpv1 = atoi( pbuf );
					tmpv1 = -tmpv1;
				}
				qs_array_push_integer( _ppool, &workingmunit, tmpv1 );
			}
		}
	}
	int32_t resultmunit = -1;
	if( workingmunit > 0 )
	{
		QS_ARRAY_ELEMENT* resultelm = qs_array_pop( _ppool, workingmunit );
		if( resultelm != NULL )
		{
			if( resultelm->id == ELEMENT_LITERAL_NUM )
			{
				if( 0 < result_cache_munit )
				{
					if( QS_PUNIT_USIZE(_ppool,result_cache_munit) >= QS_PUNIT_USIZE(_ppool,resultelm->memid_array_element_data) ){
						tmpmunit = result_cache_munit;
					}
					else{
						tmpmunit = qs_create_memory_block( _ppool, QS_PUNIT_USIZE( _ppool, resultelm->memid_array_element_data ) );
						if( tmpmunit <= 0 ){
							return QS_SYSTEM_ERROR;
						}
					}
				}
				else{
					tmpmunit = qs_create_memory_block( _ppool, QS_PUNIT_USIZE( _ppool, resultelm->memid_array_element_data ) );
					if( tmpmunit <= 0 ){
						return QS_SYSTEM_ERROR;
					}
				}
				pbuf = (char*)QS_GET_POINTER( _ppool, tmpmunit );
				memcpy( pbuf, (char*)QS_GET_POINTER(_ppool,resultelm->memid_array_element_data), QS_PUNIT_USIZE( _ppool, resultelm->memid_array_element_data ) );
				int32_t* pv = QS_PINT32(_ppool,tmpmunit);
				*pv = QS_INT32(_ppool,resultelm->memid_array_element_data);
				resultmunit = qs_create_return(_ppool, pscript, tmpmunit, resultelm->id );
			}
			else if( resultelm->id == ELEMENT_ARRAY ){
				resultmunit = qs_create_return(_ppool, pscript, resultelm->memid_array_element_data, resultelm->id );
			}
			else if( resultelm->id == ELEMENT_HASH ){
				resultmunit = qs_create_return(_ppool, pscript, resultelm->memid_array_element_data, resultelm->id );
			}
			else{
				if( resultelm->memid_array_element_data > 0 )
				{
					tmpmunit = qs_create_memory_block( _ppool, QS_PUNIT_USIZE( _ppool, resultelm->memid_array_element_data ) );
					if( tmpmunit <= 0 ){
						return QS_SYSTEM_ERROR;
					}
					pbuf = (char*)QS_GET_POINTER( _ppool, tmpmunit );
					memcpy( pbuf, (char*)QS_GET_POINTER(_ppool,resultelm->memid_array_element_data), QS_PUNIT_USIZE( _ppool, tmpmunit ) );
					resultmunit = qs_create_return(_ppool, pscript, tmpmunit, resultelm->id );
				}
			}
		}
		else{
			printf("null result\n");
		}
	}
	else{
		printf("null result\n");
	}
	return resultmunit;
}

int32_t qs_exec_if( QS_MEMORY_POOL* _ppool, QS_SCRIPT *pscript, QS_NODE* node )
{
	QS_NODE* vnode;
	QS_NODE* workelemlist;
	int32_t resultmunit = -1;
	QS_NODE* vworkelemlist;
	int32_t exec_expr_munit;
	QS_NODE* cnode;
	QS_NODE* blocknode;
	QS_NODE* blockworkelemlist;
	do{
		if( node == NULL ){
			printf( "node is null\n" );
			break;
		}
		workelemlist = ( QS_NODE* )QS_GET_POINTER( _ppool, node->element_munit );
		if( workelemlist == NULL ){
			printf( "workelemlist is null\n" );
			break;
		}
		int nodeindex = 0;
		int is_success = 0;
		while( nodeindex < node->pos )
		{
			if( ( nodeindex == 0 && workelemlist[nodeindex].id == ELEMENT_IF ) || workelemlist[nodeindex].id == ELEMENT_ELSEIF )
			{
				if( workelemlist[nodeindex+1].id != ELEMENT_CHILD )
				{
					printf( "element not child : %d\n", workelemlist[nodeindex+1].id );
					break;
				}
				vnode = (QS_NODE*)QS_GET_POINTER( _ppool, workelemlist[nodeindex+1].element_munit );
				vworkelemlist = ( QS_NODE* )QS_GET_POINTER( _ppool, vnode->element_munit );
				cnode = (QS_NODE*)QS_GET_POINTER( _ppool, vworkelemlist[0].element_munit );
				exec_expr_munit = qs_exec_expr( _ppool, pscript, cnode, 0 );
				if( -1 == exec_expr_munit )
				{
					printf("exec if error1. \n");
					break;
				}
				QS_FUNCTION_RETURN* pfret = (QS_FUNCTION_RETURN*)QS_GET_POINTER( _ppool, exec_expr_munit );
				if( pfret->munit <= 0 )
				{
					printf("exec if error2. \n");
					break;
				}
				int32_t rv = QS_INT32(_ppool, pfret->munit);
				if( 1 == rv )
				//if( !strcmp( (char*)QS_GET_POINTER( _ppool, pfret->munit ), "1" ) )
				{
					is_success = 1;
				}
				nodeindex+=2;
			}
			else{
				nodeindex++;
				is_success = 1;
			}
			if( is_success == 1 )
			{
				blocknode = (QS_NODE*)QS_GET_POINTER( _ppool, workelemlist[nodeindex].element_munit );
				blockworkelemlist = ( QS_NODE* )QS_GET_POINTER( _ppool, blocknode->element_munit );
				int j;
				QS_NODE* cblocknode;
				for( j = 0; j < blocknode->pos; j++ )
				{
					cblocknode = (QS_NODE*)QS_GET_POINTER( _ppool, blockworkelemlist[j].element_munit );
					resultmunit = qs_exec_core( _ppool, pscript, cblocknode );
				}
				break;
			}
			else{
				nodeindex++;
			}
		}
	}while( false );
	return resultmunit;
}

int32_t qs_exec_while( QS_MEMORY_POOL* _ppool, QS_SCRIPT *pscript, QS_NODE* node )
{
	QS_NODE* vnode;
	QS_NODE* workelemlist;
	int32_t resultmunit = -1;
	int32_t exec_expr_munit;
	QS_NODE* vworkelemlist;
	QS_NODE* cnode;
	QS_NODE* blocknode;
	QS_NODE* blockworkelemlist;
	int j;
	QS_NODE* cblocknode;
	int32_t while_ret_munit = qs_create_memory_block( _ppool, NUMERIC_BUFFER_SIZE );
	workelemlist = ( QS_NODE* )QS_GET_POINTER( _ppool, node->element_munit );
	do{
		if( workelemlist[0].id == ELEMENT_WHILE ){
			if( workelemlist[1].id != ELEMENT_CHILD ){
				printf( "element not child : %d\n", workelemlist[1].id );
				break;
			}
			vnode = (QS_NODE*)QS_GET_POINTER( _ppool, workelemlist[1].element_munit );
			vworkelemlist = ( QS_NODE* )QS_GET_POINTER( _ppool, vnode->element_munit );
			exec_expr_munit = -1;
			cnode = (QS_NODE*)QS_GET_POINTER( _ppool, vworkelemlist[0].element_munit );
			QS_FUNCTION_RETURN* pfret;
			blocknode = (QS_NODE*)QS_GET_POINTER( _ppool, workelemlist[2].element_munit );
			blockworkelemlist = ( QS_NODE* )QS_GET_POINTER( _ppool, blocknode->element_munit );
			do{
				exec_expr_munit = qs_exec_expr( _ppool, pscript, cnode, while_ret_munit );
				if( exec_expr_munit > 0 ){
					pfret = (QS_FUNCTION_RETURN*)QS_GET_POINTER( _ppool, exec_expr_munit );
					int32_t rv = QS_INT32(_ppool, pfret->munit);
					if( 1 == rv ) {
						for( j = 0; j < blocknode->pos; j++ ){
							cblocknode = (QS_NODE*)QS_GET_POINTER( _ppool, blockworkelemlist[j].element_munit );
							qs_exec_core( _ppool, pscript, cblocknode );
						}
					}
					else{
						break;
					}
				}
				else{
					printf("exec error while. \n");
					break;
				}
			}while( true );
		}
	}while( false );
	return resultmunit;
}

int32_t qs_create_return( QS_MEMORY_POOL* _ppool, QS_SCRIPT *pscript, int32_t data_munit, int32_t id )
{
	QS_FUNCTION_RETURN* pret;
	pret = (QS_FUNCTION_RETURN*)QS_GET_POINTER( _ppool, pscript->return_munit );
	pret->refid = pscript->return_munit;
	pret->id	= id;
	pret->munit	= data_munit;
	return pscript->return_munit;
}

int qs_add_system_function( QS_MEMORY_POOL* _ppool, int32_t munit, char* functionname, QS_SCRIPT_FUNCTION func, int32_t id )
{
	char* pbuf;
	int32_t namemunit;
	int32_t datamunit;
	QS_FUNCTION_INFO* pfuncinfo;
	int error = 1;
	QS_SCRIPT *pscript;
	do{
		pscript = (QS_SCRIPT *)QS_GET_POINTER( _ppool, munit );
		namemunit = qs_get_hash_name( _ppool, pscript->f_hash_munit, functionname );
		if( namemunit >= 0 )
		{
			printf("name munit found error : %d\n", namemunit);
			break;
		}
		namemunit = qs_create_memory_block( _ppool, strlen( functionname )+1 );
		pbuf = (char*)QS_GET_POINTER( _ppool, namemunit );
		qs_strcopy( pbuf, functionname, QS_PUNIT_USIZE( _ppool, namemunit ) );
		datamunit = qs_create_memory_block( _ppool, sizeof( QS_FUNCTION_INFO ) );
		pfuncinfo = (QS_FUNCTION_INFO*)QS_GET_POINTER( _ppool, datamunit );
		pfuncinfo->type = QS_FUNCTION_TYPE_SYSTEM;
		pfuncinfo->func = func;
		pfuncinfo->userfunction_munit = -1;
		qs_add_hash( _ppool, pscript->f_hash_munit, namemunit, datamunit, id );
		error = 0;
	}while( false );
	return error;
}

int qs_add_user_function( QS_MEMORY_POOL* _ppool, int32_t munit, char* functionname, int32_t function_munit, int32_t id )
{
	char* pbuf;
	int32_t namemunit;
	int32_t datamunit;
	QS_FUNCTION_INFO* pfuncinfo;
	int error = 1;
	QS_SCRIPT *pscript;
	do{
		pscript = (QS_SCRIPT *)QS_GET_POINTER( _ppool, munit );
		namemunit = qs_get_hash_name( _ppool, pscript->f_hash_munit, functionname );
		if( namemunit >= 0 )
		{
			printf("name munit found error : %d\n", namemunit);
			break;
		}
		namemunit = qs_create_memory_block( _ppool, strlen( functionname )+1 );
		pbuf = (char*)QS_GET_POINTER( _ppool, namemunit );
		qs_strcopy( pbuf, functionname, QS_PUNIT_USIZE( _ppool, namemunit ) );
		datamunit = qs_create_memory_block( _ppool, sizeof( QS_FUNCTION_INFO ) );
		pfuncinfo = (QS_FUNCTION_INFO*)QS_GET_POINTER( _ppool, datamunit );
		pfuncinfo->type = QS_FUNCTION_TYPE_USER;
		pfuncinfo->func = NULL;
		pfuncinfo->userfunction_munit = function_munit;
		qs_add_hash( _ppool, pscript->f_hash_munit, namemunit, datamunit, id );
		error = 0;
	}while( false );
	return error;
}

void qs_set_return_string( QS_MEMORY_POOL* _ppool, int32_t memid_return, int32_t memid_value_string, const char* result)
{
	QS_FUNCTION_RETURN* pret = (QS_FUNCTION_RETURN*)qs_upointer( _ppool, memid_return );
	char* pbuf = (char*)qs_upointer( _ppool, memid_value_string );
	qs_strcopy( pbuf, result, QS_PUNIT_USIZE( _ppool, memid_value_string ) );
	pret->munit = memid_value_string;
	pret->id = ELEMENT_LITERAL_STR;
}

void* qs_script_system_function_echo( QS_MEMORY_POOL* _ppool, void* args )
{
	QS_ARRAY* parray = (QS_ARRAY*)args;
	QS_ARRAY_ELEMENT* elm;
	QS_FUNCTION_RETURN* pret;
	int32_t memid_return = -1;
	memid_return = qs_create_memory_block( _ppool, sizeof( QS_FUNCTION_RETURN ) );
	if( -1 == memid_return ){
		return NULL;
	}
	pret = (QS_FUNCTION_RETURN*)qs_upointer( _ppool, memid_return );
	pret->id	= 0;
	pret->munit	= -1;
	if( parray != NULL )
	{
		elm = (QS_ARRAY_ELEMENT*)qs_upointer( _ppool, parray->memid );
		if( elm != NULL )
		{
			if( parray->len > 0 )
			{
				if( elm[0].id == ELEMENT_LITERAL_STR )
				{
					printf( "%s", (char*)qs_upointer( _ppool, elm[0].memid_array_element_data ) );
				}
				else if( elm[0].id == ELEMENT_LITERAL_NUM )
				{
					printf( "%s", (char*)qs_upointer( _ppool, elm[0].memid_array_element_data ) );
				}
				else if( elm[0].id == ELEMENT_ARRAY )
				{
					qs_array_dump( _ppool, elm[0].memid_array_element_data, 0 );
				}
				else if( elm[0].id == ELEMENT_HASH )
				{
					qs_hash_dump( _ppool, elm[0].memid_array_element_data, 0 );
				}
			}
		}
	}
	return pret;
}

void* qs_script_system_function_count( QS_MEMORY_POOL* _ppool, void* args )
{
	QS_ARRAY* parray = (QS_ARRAY*)args;
	QS_ARRAY_ELEMENT* elm;
	QS_FUNCTION_RETURN* pret;
	int32_t memid_return = -1;
	memid_return = qs_create_memory_block( _ppool, sizeof( QS_FUNCTION_RETURN ) );
	if( -1 == memid_return ){
		return NULL;
	}
	pret = (QS_FUNCTION_RETURN*)qs_upointer( _ppool, memid_return );
	pret->id	= 0;
	pret->munit	= -1;
	pret->refid = memid_return;
	if( parray != NULL )
	{
		elm = (QS_ARRAY_ELEMENT*)qs_upointer( _ppool, parray->memid );
		if( elm != NULL )
		{
			if( parray->len > 0 )
			{
				int32_t len_munit = qs_create_memory_block( _ppool, NUMERIC_BUFFER_SIZE );
				int32_t len = 0;
				if( len_munit <= 0 ){
					return pret;
				}
				if( elm[0].id == ELEMENT_LITERAL_STR )
				{
					len = qs_strlen( (char*)QS_GET_POINTER(_ppool,elm[0].memid_array_element_data) );
				}
				else if( elm[0].id == ELEMENT_ARRAY )
				{
					len = qs_array_length( _ppool, elm[0].memid_array_element_data );
				}
				else if( elm[0].id == ELEMENT_HASH )
				{
					len = qs_hash_length( _ppool, elm[0].memid_array_element_data );
				}
				char* pbuf = (char*)QS_GET_POINTER(_ppool,len_munit);
				qs_itoa( len, pbuf, QS_PUNIT_USIZE(_ppool,len_munit) );
				int32_t* pv = QS_PINT32(_ppool,len_munit);
				*pv = len;
				pret->munit = len_munit;
				pret->id = ELEMENT_LITERAL_NUM;
			}
		}
	}
	return pret;
}

void* qs_script_system_function_file_exist( QS_MEMORY_POOL* _ppool, void* args )
{
	QS_ARRAY* parray = (QS_ARRAY*)args;
	QS_ARRAY_ELEMENT* elm;
	QS_FUNCTION_RETURN* pret;
	int32_t memid_return = -1;
	memid_return = qs_create_memory_block( _ppool, sizeof( QS_FUNCTION_RETURN ) );
	if( -1 == memid_return ){
		return NULL;
	}
	pret = (QS_FUNCTION_RETURN*)qs_upointer( _ppool, memid_return );
	pret->id	= 0;
	pret->munit	= -1;
	pret->refid = memid_return;
	if( parray != NULL )
	{
		elm = (QS_ARRAY_ELEMENT*)qs_upointer( _ppool, parray->memid );
		if( elm != NULL )
		{
			if( parray->len > 0 )
			{
				if( elm[0].id == ELEMENT_LITERAL_STR )
				{
					QS_FILE_INFO info;
					int32_t len_munit = qs_create_memory_block( _ppool, NUMERIC_BUFFER_SIZE );
					char* pbuf = (char*)QS_GET_POINTER(_ppool,len_munit);
					if( 0 != qs_fget_info( (char*)QS_GET_POINTER( _ppool, elm[0].memid_array_element_data ), &info ) ){
						qs_itoa( 0, pbuf, QS_PUNIT_USIZE(_ppool,len_munit) );
						int32_t* pv = QS_PINT32(_ppool,len_munit);
						*pv = 0;
					}
					else{
						qs_itoa( 1, pbuf, QS_PUNIT_USIZE(_ppool,len_munit) );
						int32_t* pv = QS_PINT32(_ppool,len_munit);
						*pv = 1;
					}
					pret->munit = len_munit;
					pret->id = ELEMENT_LITERAL_NUM;
				}
			}
		}
	}
	return pret;
}

void* qs_script_system_function_file_size( QS_MEMORY_POOL* _ppool, void* args )
{
	QS_ARRAY* parray = (QS_ARRAY*)args;
	QS_ARRAY_ELEMENT* elm;
	QS_FUNCTION_RETURN* pret;
	int32_t memid_return = -1;
	memid_return = qs_create_memory_block( _ppool, sizeof( QS_FUNCTION_RETURN ) );
	if( -1 == memid_return ){
		return NULL;
	}
	pret = (QS_FUNCTION_RETURN*)qs_upointer( _ppool, memid_return );
	pret->id	= 0;
	pret->munit	= -1;
	pret->refid = memid_return;
	if( parray != NULL )
	{
		elm = (QS_ARRAY_ELEMENT*)qs_upointer( _ppool, parray->memid );
		if( elm != NULL )
		{
			if( parray->len > 0 )
			{
				if( elm[0].id == ELEMENT_LITERAL_STR )
				{
					QS_FILE_INFO info;
					int32_t len_munit = qs_create_memory_block( _ppool, NUMERIC_BUFFER_SIZE );
					char* pbuf = (char*)QS_GET_POINTER(_ppool,len_munit);
					if( 0 != qs_fget_info( (char*)QS_GET_POINTER( _ppool, elm[0].memid_array_element_data ), &info ) ){
						qs_itoa( 0, pbuf, QS_PUNIT_USIZE(_ppool,len_munit) );
						int32_t* pv = QS_PINT32(_ppool,len_munit);
						*pv = 0;
					}
					else{
						qs_itoa( (int32_t)info.size, pbuf, QS_PUNIT_USIZE(_ppool,len_munit) );
						int32_t* pv = QS_PINT32(_ppool,len_munit);
						*pv = (int32_t)info.size;
					}
					pret->munit = len_munit;
					pret->id = ELEMENT_LITERAL_NUM;
				}
			}
		}
	}
	return pret;
}

void* qs_script_system_function_file_extension( QS_MEMORY_POOL* _ppool, void* args )
{
	QS_ARRAY* parray = (QS_ARRAY*)args;
	QS_ARRAY_ELEMENT* elm;
	QS_FUNCTION_RETURN* pret;
	int32_t memid_return = -1;
	memid_return = qs_create_memory_block( _ppool, sizeof( QS_FUNCTION_RETURN ) );
	if( -1 == memid_return ){
		return NULL;
	}
	pret = (QS_FUNCTION_RETURN*)qs_upointer( _ppool, memid_return );
	pret->id	= 0;
	pret->munit	= -1;
	pret->refid = memid_return;
	if( parray != NULL )
	{
		elm = (QS_ARRAY_ELEMENT*)qs_upointer( _ppool, parray->memid );
		if( elm != NULL )
		{
			if( parray->len > 0 )
			{
				if( elm[0].id == ELEMENT_LITERAL_STR )
				{
					int32_t string_munit = qs_create_memory_block( _ppool, 32 );
					if( string_munit <= 0 ){
						return pret;
					}
					if( QS_SYSTEM_OK != qs_get_extension( (char*)QS_GET_POINTER( _ppool, string_munit ), 32, (char*)QS_GET_POINTER( _ppool, elm[0].memid_array_element_data ) ) ){
						
					}
					pret->munit = string_munit;
					pret->id = ELEMENT_LITERAL_STR;
				}
			}
		}
	}
	return pret;
}

void* qs_script_system_function_file_get( QS_MEMORY_POOL* _ppool, void* args )
{
	QS_ARRAY* parray = (QS_ARRAY*)args;
	QS_ARRAY_ELEMENT* elm;
	QS_FUNCTION_RETURN* pret;
	int32_t memid_return = -1;
	memid_return = qs_create_memory_block( _ppool, sizeof( QS_FUNCTION_RETURN ) );
	if( -1 == memid_return ){
		return NULL;
	}
	pret = (QS_FUNCTION_RETURN*)qs_upointer( _ppool, memid_return );
	pret->id	= 0;
	pret->munit	= -1;
	pret->refid = memid_return;
	if( parray != NULL )
	{
		elm = (QS_ARRAY_ELEMENT*)qs_upointer( _ppool, parray->memid );
		if( elm != NULL )
		{
			if( parray->len > 0 )
			{
				if( elm[0].id == ELEMENT_LITERAL_STR )
				{
					QS_FILE_INFO info;
					if( 0 != qs_fget_info( (char*)QS_GET_POINTER( _ppool, elm[0].memid_array_element_data ), &info ) ){
						return pret;
					}
					if( parray->len > 1 && elm[1].id == ELEMENT_LITERAL_STR && !strcmp("b",(char*)QS_GET_POINTER( _ppool, elm[1].memid_array_element_data )) ){
						int32_t string_munit = qs_create_memory_block( _ppool, info.size );
						if( string_munit <= 0 ){
							return pret;
						}
						qs_fread_bin( (char*)QS_GET_POINTER( _ppool, elm[0].memid_array_element_data ), (char*)QS_GET_POINTER(_ppool,string_munit), QS_PUNIT_USIZE(_ppool,string_munit) );
						pret->munit = string_munit;
						pret->id = ELEMENT_LITERAL_BIN;
					}
					else{
						int32_t string_munit = qs_create_memory_block( _ppool, info.size+1 );
						if( string_munit <= 0 ){
							return pret;
						}
						qs_fread( (char*)QS_GET_POINTER( _ppool, elm[0].memid_array_element_data ), (char*)QS_GET_POINTER(_ppool,string_munit), QS_PUNIT_USIZE(_ppool,string_munit) );
						pret->munit = string_munit;
						pret->id = ELEMENT_LITERAL_STR;
					}
				}
			}
		}
	}
	return pret;
}

void* qs_script_system_function_file_put( QS_MEMORY_POOL* _ppool, void* args )
{
	QS_ARRAY* parray = (QS_ARRAY*)args;
	QS_ARRAY_ELEMENT* elm;
	QS_FUNCTION_RETURN* pret;
	int32_t memid_return = -1;
	memid_return = qs_create_memory_block( _ppool, sizeof( QS_FUNCTION_RETURN ) );
	if( -1 == memid_return ){
		return NULL;
	}
	pret = (QS_FUNCTION_RETURN*)qs_upointer( _ppool, memid_return );
	pret->id	= 0;
	pret->munit	= -1;
	pret->refid = memid_return;
	if( parray != NULL )
	{
		elm = (QS_ARRAY_ELEMENT*)qs_upointer( _ppool, parray->memid );
		if( elm != NULL )
		{
			if( parray->len > 1 )
			{
				if( elm[0].id == ELEMENT_LITERAL_STR && elm[1].id == ELEMENT_LITERAL_STR )
				{
					int32_t string_munit = qs_create_memory_block( _ppool, 8 );
					char* file_name = (char*)QS_GET_POINTER( _ppool, elm[0].memid_array_element_data );
					char* write_data = (char*)QS_GET_POINTER( _ppool, elm[1].memid_array_element_data );
					size_t write_size = QS_PUNIT_USIZE(_ppool,elm[1].memid_array_element_data);
					qs_set_return_string( 
						_ppool,
						memid_return,
						string_munit,
						(0 != qs_fwrite( file_name, write_data, write_size )) ? "0" : "1"
					);
				}
			}
		}
	}
	return pret;
}

void* qs_script_system_function_file_add( QS_MEMORY_POOL* _ppool, void* args )
{
	QS_ARRAY* parray = (QS_ARRAY*)args;
	QS_ARRAY_ELEMENT* elm;
	QS_FUNCTION_RETURN* pret;
	int32_t memid_return = -1;
	memid_return = qs_create_memory_block( _ppool, sizeof( QS_FUNCTION_RETURN ) );
	if( -1 == memid_return ){
		return NULL;
	}
	pret = (QS_FUNCTION_RETURN*)qs_upointer( _ppool, memid_return );
	pret->id	= 0;
	pret->munit	= -1;
	pret->refid = memid_return;
	if( parray != NULL )
	{
		elm = (QS_ARRAY_ELEMENT*)qs_upointer( _ppool, parray->memid );
		if( elm != NULL )
		{
			if( parray->len > 1 )
			{
				if( elm[0].id == ELEMENT_LITERAL_STR && elm[1].id == ELEMENT_LITERAL_STR )
				{
					int32_t string_munit = qs_create_memory_block( _ppool, 8 );
					if(-1 == string_munit){
						return pret;
					}
					char* file_name = (char*)QS_GET_POINTER( _ppool, elm[0].memid_array_element_data );
					char* write_data = (char*)QS_GET_POINTER( _ppool, elm[1].memid_array_element_data );
					size_t write_size = QS_PUNIT_USIZE(_ppool,elm[1].memid_array_element_data);
					qs_set_return_string( 
						_ppool,
						memid_return,
						string_munit,
						(0 != qs_fwrite_a( file_name, write_data, write_size )) ? "0" : "1"
					);
				}
			}
		}
	}
	return pret;
}

void* qs_script_system_function_json_encode( QS_MEMORY_POOL* _ppool, void* args )
{
	QS_ARRAY* parray = (QS_ARRAY*)args;
	QS_ARRAY_ELEMENT* elm;
	QS_FUNCTION_RETURN* pret;
	int32_t memid_return = -1;
	memid_return = qs_create_memory_block( _ppool, sizeof( QS_FUNCTION_RETURN ) );
	if( -1 == memid_return ){
		return NULL;
	}
	pret = (QS_FUNCTION_RETURN*)qs_upointer( _ppool, memid_return );
	pret->id	= 0;
	pret->munit	= -1;
	pret->refid = memid_return;
	if( parray != NULL )
	{
		elm = (QS_ARRAY_ELEMENT*)qs_upointer( _ppool, parray->memid );
		if( elm != NULL )
		{
			if( parray->len > 0 )
			{
				if( elm[0].id == ELEMENT_HASH || elm[0].id == ELEMENT_ARRAY )
				{
					int32_t r_munit = qs_make_json_root( _ppool, elm[0].memid_array_element_data, elm[0].id );
					if( r_munit == -1 ){
						return pret;
					}
					int32_t variable_munit = qs_json_encode( _ppool, (QS_NODE*)QS_GET_POINTER( _ppool, r_munit ), 1024);
					if( variable_munit == -1 ){
						return pret;
					}
					pret->munit = variable_munit;
					pret->id = ELEMENT_LITERAL_STR;
				}
			}
		}
	}
	return pret;
}

void* qs_script_system_function_json_decode( QS_MEMORY_POOL* _ppool, void* args )
{
	QS_ARRAY* parray = (QS_ARRAY*)args;
	QS_ARRAY_ELEMENT* elm;
	QS_FUNCTION_RETURN* pret;
	int32_t memid_return = -1;
	memid_return = qs_create_memory_block( _ppool, sizeof( QS_FUNCTION_RETURN ) );
	if( -1 == memid_return ){
		return NULL;
	}
	pret = (QS_FUNCTION_RETURN*)qs_upointer( _ppool, memid_return );
	pret->id	= 0;
	pret->munit	= -1;
	pret->refid = memid_return;
	if( parray != NULL )
	{
		elm = (QS_ARRAY_ELEMENT*)qs_upointer( _ppool, parray->memid );
		if( elm != NULL )
		{
			if( parray->len > 0 )
			{
				if( elm[0].id == ELEMENT_LITERAL_STR )
				{
					int32_t rootnode_munit = qs_json_decode( _ppool, (char*)QS_GET_POINTER( _ppool, elm[0].memid_array_element_data ) );
					QS_NODE *rootnode = (QS_NODE*)QS_GET_POINTER( _ppool, rootnode_munit );
					QS_NODE* workelemlist = ( QS_NODE* )QS_GET_POINTER( _ppool, rootnode->element_munit );
					pret->munit = workelemlist[0].element_munit;
					pret->id = workelemlist[0].id;
				}
			}
		}
	}
	return pret;
}

void* qs_script_system_function_gmtime( QS_MEMORY_POOL* _ppool, void* args )
{
	QS_FUNCTION_RETURN* pret;
	int32_t memid_return = -1;
	memid_return = qs_create_memory_block( _ppool, sizeof( QS_FUNCTION_RETURN ) );
	if( -1 == memid_return ){
		return NULL;
	}
	pret = (QS_FUNCTION_RETURN*)qs_upointer( _ppool, memid_return );
	pret->id	= 0;
	pret->munit	= -1;
	pret->refid = memid_return;
	int32_t string_munit = qs_create_memory_block( _ppool, 64 );
	if( -1 == string_munit ){
		printf( "string_munit error\n" );
		return pret;
	}
	qs_utc_time( (char*)QS_GET_POINTER(_ppool,string_munit), QS_PUNIT_USIZE(_ppool,string_munit) );
	pret->munit = string_munit;
	pret->id = ELEMENT_LITERAL_STR;
	return pret;
}

void* qs_script_system_function_rand( QS_MEMORY_POOL* _ppool, void* args )
{
	QS_FUNCTION_RETURN* pret;
	int32_t memid_return = -1;
	memid_return = qs_create_memory_block( _ppool, sizeof( QS_FUNCTION_RETURN ) );
	if( -1 == memid_return ){
		return NULL;
	}
	pret = (QS_FUNCTION_RETURN*)qs_upointer( _ppool, memid_return );
	pret->id	= 0;
	pret->munit	= -1;
	pret->refid = memid_return;
	int32_t string_munit = qs_create_memory_block( _ppool, NUMERIC_BUFFER_SIZE );
	if( -1 == string_munit ){
		printf( "string_munit error\n" );
		return pret;
	}
	char* pbuf = (char*)QS_GET_POINTER(_ppool,string_munit);
	int32_t rand_value = qs_rand_32();
	if( rand_value < 0 ){
		rand_value = -rand_value;
	}
	qs_itoa( rand_value, pbuf, QS_PUNIT_USIZE(_ppool,string_munit) );
	NUMERIC_CAST* pv = QS_PNUMERIC(_ppool,string_munit);
	*pv = rand_value;
	pret->munit = string_munit;
	pret->id = ELEMENT_LITERAL_NUM;
	return pret;
}