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

#include "gdt_script.h"

int32_t gdt_init_script( GDT_MEMORY_POOL* _ppool, size_t valiablehash_size, size_t functionhash_size )
{
	GDT_SCRIPT *pscript;
	int32_t munit = -1;
	do{
		munit = gdt_create_munit( _ppool, sizeof( GDT_SCRIPT ), MEMORY_TYPE_DEFAULT );
		if( munit <= 0 ){
			break;
		}
		pscript = (GDT_SCRIPT *)GDT_POINTER( _ppool, munit );
		pscript->self_munit = munit;
		pscript->textbuf_munit = -1;
		pscript->hash_alloc_size = 64;
		if( 0 >= ( pscript->tokens_munit = gdt_inittoken( _ppool ) ) ){
			break;
		}
		if( 0 >= ( pscript->rootnode_munit = gdt_createrootnode( _ppool ) ) ){
			break;
		}
		if( 0 >= ( pscript->v_hash_munit = gdt_create_hash( _ppool, valiablehash_size ) ) ){
			break;
		}
		if( 0 >= ( pscript->f_hash_munit = gdt_create_hash( _ppool, functionhash_size ) ) ){
			break;
		}
		if( 0 >= ( pscript->s_hash_munit = gdt_create_array( _ppool, 128, NUMERIC_BUFFER_SIZE ) ) ){
			break;
		}
		if( 0 >= ( pscript->return_munit = gdt_create_munit( _ppool, sizeof( GDT_FUNCTION_RETURN ), MEMORY_TYPE_DEFAULT ) ) ){
			break;
		}
		if( 0 >= ( pscript->int_cache_munit = gdt_create_munit( _ppool, sizeof(int32_t) * 32, MEMORY_TYPE_DEFAULT ) ) ){
			break;
		}
	}while( false );
	return munit;
}

void gdt_import_script( GDT_MEMORY_POOL* _ppool, int32_t *p_unitid, char* filename )
{
	FILE* file;
	int fsize = 0;
	char *pstr;
	GDT_SCRIPT *pscript;
	char* p;
#ifdef __WINDOWS__
	char c;
#else
	int8_t c;
#endif

#ifdef __GDT_DEBUG__
#ifdef __WINDOWS__
	LARGE_INTEGER start_pc, end_pc, freq_pc;
	double sec_pc;
	QueryPerformanceFrequency( &freq_pc );
	QueryPerformanceCounter( &start_pc );
#else
	struct timeval start_timeval, end_timeval;
	double sec_timeofday;
	gettimeofday( &start_timeval, NULL );
#endif
#endif
	pscript = (GDT_SCRIPT *)GDT_POINTER( _ppool, ( *p_unitid ) );
	//gdt_lstateout( _ppool, filename );
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
#ifdef __GDT_DEBUG__
//	printf("filse size of %s : %dbytes\n", filename, fsize );
#endif
	pscript->textbuf_munit = gdt_create_munit( _ppool, sizeof( char ) * ( fsize+1 ), MEMORY_TYPE_DEFAULT );
	pstr = (char*)GDT_POINTER( _ppool, pscript->textbuf_munit );
	if( pstr == NULL )
	{
		printf("allocate error : token buf\n");
		return;
	}
	p = pstr;
	while( EOF != (c=fgetc(file)) ){ *(p++) = c; }
	*p = '\0';
	fclose(file);
#ifdef __GDT_DEBUG__
#ifdef __WINDOWS__
	QueryPerformanceCounter( &end_pc );
	sec_pc = (end_pc.QuadPart - start_pc.QuadPart) / (double)freq_pc.QuadPart;
	//printf(  "## file read time = %lf[s]\n", sec_pc );
#else
	gettimeofday( &end_timeval, NULL );
	sec_timeofday = (end_timeval.tv_sec - start_timeval.tv_sec)
			+ (end_timeval.tv_usec - start_timeval.tv_usec) / 1000000.0;
	//printf(  "## file read time = %lf[s]\n", sec_timeofday );
#endif
#endif

#ifdef __GDT_DEBUG__
#ifdef __WINDOWS__
	QueryPerformanceFrequency( &freq_pc );
	QueryPerformanceCounter( &start_pc );
#else
	gettimeofday( &start_timeval, NULL );
#endif
#endif
	gdt_token_analyzer( _ppool, pscript->tokens_munit, pstr );
#ifdef __GDT_DEBUG__
#ifdef __WINDOWS__
	QueryPerformanceCounter( &end_pc );
	sec_pc = (end_pc.QuadPart - start_pc.QuadPart) / (double)freq_pc.QuadPart;
	//printf(  "## token analyse time = %lf[s]\n", sec_pc );
#else
	gettimeofday( &end_timeval, NULL );
	sec_timeofday = (end_timeval.tv_sec - start_timeval.tv_sec)
			+ (end_timeval.tv_usec - start_timeval.tv_usec) / 1000000.0;
	//printf(  "## token analyse time = %lf[s]\n", sec_timeofday );
#endif
#endif
	//gdt_tokendump( _ppool, pscript->tokens_munit );
#ifdef __GDT_DEBUG__
#ifdef __WINDOWS__
	QueryPerformanceFrequency( &freq_pc );
	QueryPerformanceCounter( &start_pc );
#else
	gettimeofday( &start_timeval, NULL );
#endif
#endif
	gdt_parse_code( _ppool, pscript );
#ifdef __GDT_DEBUG__
#ifdef __WINDOWS__
	QueryPerformanceCounter( &end_pc );
	sec_pc = (end_pc.QuadPart - start_pc.QuadPart) / (double)freq_pc.QuadPart;
	//printf(  "## parse script time = %lf[s]\n", sec_pc );
#else
	gettimeofday( &end_timeval, NULL );
	sec_timeofday = (end_timeval.tv_sec - start_timeval.tv_sec)
			+ (end_timeval.tv_usec - start_timeval.tv_usec) / 1000000.0;
	//printf(  "## parse script time = %lf[s]\n", sec_timeofday );
#endif
#endif
}

void gdt_input_script( GDT_MEMORY_POOL* _ppool, int32_t *p_unitid, char* pstr )
{
	GDT_SCRIPT *pscript;
#ifdef __GDT_DEBUG__
#ifdef __WINDOWS__
	LARGE_INTEGER start_pc, end_pc, freq_pc;
	double sec_pc;
	QueryPerformanceFrequency( &freq_pc );
	QueryPerformanceCounter( &start_pc );
#else
	struct timeval start_timeval, end_timeval;
	double sec_timeofday;
	gettimeofday( &start_timeval, NULL );
#endif
#endif
	pscript = (GDT_SCRIPT *)GDT_POINTER( _ppool, ( *p_unitid ) );
	gdt_token_analyzer( _ppool, pscript->tokens_munit, pstr );
#ifdef __GDT_DEBUG__
#ifdef __WINDOWS__
	QueryPerformanceCounter( &end_pc );
	sec_pc = (end_pc.QuadPart - start_pc.QuadPart) / (double)freq_pc.QuadPart;
	printf(  "## token analyse time = %lf[s]\n", sec_pc );
#else
	gettimeofday( &end_timeval, NULL );
	sec_timeofday = (end_timeval.tv_sec - start_timeval.tv_sec)
			+ (end_timeval.tv_usec - start_timeval.tv_usec) / 1000000.0;
	printf(  "## token analyse time = %lf[s]\n", sec_timeofday );
#endif
#endif
	// gdt_tokendump( _ppool, pscript );
#ifdef __GDT_DEBUG__
#ifdef __WINDOWS__
	QueryPerformanceFrequency( &freq_pc );
	QueryPerformanceCounter( &start_pc );
#else
	gettimeofday( &start_timeval, NULL );
#endif
#endif
	gdt_parse_code( _ppool, pscript );
#ifdef __GDT_DEBUG__
#ifdef __WINDOWS__
	QueryPerformanceCounter( &end_pc );
	sec_pc = (end_pc.QuadPart - start_pc.QuadPart) / (double)freq_pc.QuadPart;
	printf(  "## parse script time = %lf[s]\n", sec_pc );
#else
	gettimeofday( &end_timeval, NULL );
	sec_timeofday = (end_timeval.tv_sec - start_timeval.tv_sec)
			+ (end_timeval.tv_usec - start_timeval.tv_usec) / 1000000.0;
	printf(  "## parse script time = %lf[s]\n", sec_timeofday );
#endif
#endif
}

int gdt_parse_code( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT* pscript )
{
	int error = 0;
	int i = 0;
	int tmpi = 0;
	GDT_NODE *rootnode;
	GDT_TOKENS *ptokens = (GDT_TOKENS*)GDT_POINTER( _ppool, pscript->tokens_munit );
	rootnode = (GDT_NODE*)GDT_POINTER( _ppool, pscript->rootnode_munit );
	do
	{
		tmpi = i;
		i = gdt_parse_code_core( _ppool, pscript, rootnode, i );
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

int gdt_parse_code_core( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT* pscript, GDT_NODE* node, int i )
{
	GDT_TOKENS *ptokens = (GDT_TOKENS*)GDT_POINTER( _ppool, pscript->tokens_munit );
	GDT_TOKEN *token_list;
	char *pbuf;
	int error = 0;
	token_list = ( GDT_TOKEN* )GDT_POINTER( _ppool, ptokens->token_munit );
	pbuf = (char*)GDT_POINTER( _ppool, token_list[i].buf_munit );
	switch( token_list[i].type )
	{
		case ID_SYMBOL:
			i = gdt_parse_symbol( _ppool, pscript, node, i );
			break;
		case ID_SYS_IF:
			i = gdt_parse_if( _ppool, pscript, node, i );
			break;
		case ID_SYS_WHILE:
			i = gdt_parse_while( _ppool, pscript, node, i );
			break;
		case ID_SIGN:
			if( !strcmp( ";", pbuf ) ){
				i++;
			}
			else if( strcmp( "{", pbuf ) ){
				error = 1;
			}
			else{
				i = gdt_parse_block( _ppool, pscript, node, i );
			}
			break;
		case ID_SYS_ELSE:
		case ID_SYS_ELSEIF:
		case ID_UNK:
		case ID_NUM:
		case ID_STR:
		case ID_OP:
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
int gdt_parse_block( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT* pscript, GDT_NODE* node, int index )
{
	GDT_TOKENS *ptokens = (GDT_TOKENS*)GDT_POINTER( _ppool, pscript->tokens_munit );
	GDT_TOKEN *token_list;
	GDT_NODE *childnode;
	int32_t childmunit;
	char* pbuf;
	token_list = ( GDT_TOKEN* )GDT_POINTER( _ppool, ptokens->token_munit );
	childmunit = gdt_addnodeelement( _ppool, node );
	childnode = (GDT_NODE*)GDT_POINTER( _ppool, childmunit );
	index++;
	while( index < ptokens->currentpos )
	{
		if( token_list[index].type == ID_SIGN )
		{
			pbuf = (char*)GDT_POINTER( _ppool, token_list[index].buf_munit );
			if( !strcmp( pbuf, "}" ) )
			{
				index++;
				break;
			}
		}
		index = gdt_parse_code_core( _ppool, pscript, childnode, index );
		if( index < 0 ){
			printf( "parse error\n" );
			break;
		}
		if( token_list[index].type == ID_SIGN )
		{
			pbuf = (char*)GDT_POINTER( _ppool, token_list[index].buf_munit );
			if( !strcmp( pbuf, "}" ) )
			{
				index++;
				break;
			}
		}
	}
	return index;
}

int gdt_parse_array( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT* pscript, GDT_NODE* node, GDT_TOKEN* pt, int index )
{
	GDT_TOKENS *ptokens = (GDT_TOKENS*)GDT_POINTER( _ppool, pscript->tokens_munit );
	GDT_TOKEN *token_list;
	GDT_NODE *childnode;
	GDT_NODE *stmtnode;
	int32_t childmunit;
	int32_t tmpmunit;
	int32_t stmtmunit;
	int tmpindex;
	char* pbuf;
	if( index >= ptokens->currentpos ){
		printf("invalid index2\n");
		return index;
	}
	token_list = ( GDT_TOKEN* )GDT_POINTER( _ppool, ptokens->token_munit );
	pbuf = (char*)GDT_POINTER( _ppool, token_list[index].buf_munit );
	do{
		if( !strcmp( "[", pbuf ) )
		{
			childmunit = gdt_addnodeelement( _ppool, node );
			childnode = (GDT_NODE*)GDT_POINTER( _ppool, childmunit );
			tmpmunit = gdt_create_munit( _ppool, 8, MEMORY_TYPE_DEFAULT );
			pbuf = (char*)GDT_POINTER( _ppool, tmpmunit );
			gdt_strcopy( pbuf, "array", GDT_PUNIT_USIZE( _ppool, tmpmunit ) );
			gdt_addelement( _ppool, childnode, ELEMENT_ARRAY, tmpmunit );
			index++;
			if( index >= ptokens->currentpos ){
				printf("invalid index parse_array\n");
				return index;
			}
			if( !strcmp( (char*)GDT_POINTER( _ppool, token_list[index].buf_munit ), "]" ) )
			{
				printf("empty array\n");
				index++;
				return index;
			}
			else{
				stmtmunit = gdt_addnodeelement( _ppool, childnode );
				stmtnode = (GDT_NODE*)GDT_POINTER( _ppool, stmtmunit );
				while( index < ptokens->currentpos )
				{
					tmpindex = index;
					if( token_list[index].type == ID_SIGN )
					{
						pbuf = (char*)GDT_POINTER( _ppool, token_list[index].buf_munit );
						if( !strcmp( pbuf, "," ) ){
							childnode = (GDT_NODE*)GDT_POINTER( _ppool, childmunit );
							stmtmunit = gdt_addnodeelement( _ppool, childnode );
							stmtnode = (GDT_NODE*)GDT_POINTER( _ppool, stmtmunit );
							index++;
						}
						else if( !strcmp( pbuf, "]" ) ){
							index++;
							break;
						}
					}
					stmtnode = (GDT_NODE*)GDT_POINTER( _ppool, stmtmunit );
					if( token_list[index].type == ID_SIGN ){
						pbuf = (char*)GDT_POINTER( _ppool, token_list[index].buf_munit );
						if( !strcmp( (char*)GDT_POINTER( _ppool, token_list[index].buf_munit ), "[" ) )
						{
							index = gdt_parse_array( _ppool, pscript, stmtnode, token_list, index );
							pbuf = (char*)GDT_POINTER( _ppool, token_list[index].buf_munit );
							if( !strcmp( pbuf, "]" ) ){
								index++;
								break;
							}
						}
						else if( !strcmp( ":", pbuf ) ){
							int32_t stmtchildmunit = gdt_addnodeelement( _ppool, stmtnode );
							GDT_NODE* stmtchildnode = (GDT_NODE*)GDT_POINTER( _ppool, stmtchildmunit );
							gdt_addelement( _ppool, stmtnode, ELEMENT_HASH_OP, token_list[index].buf_munit );
							index++;
							if( !strcmp( (char*)GDT_POINTER( _ppool, token_list[index].buf_munit ), "[" ) )
							{
								index = gdt_parse_array( _ppool, pscript, stmtchildnode, token_list, index );
								pbuf = (char*)GDT_POINTER( _ppool, token_list[index].buf_munit );
								if( !strcmp( pbuf, "]" ) ){
									index++;
									break;
								}
							}
							else{
								index = gdt_parse_rel( _ppool, pscript, stmtchildnode, token_list, index );
								pbuf = (char*)GDT_POINTER( _ppool, token_list[index].buf_munit );
								if( !strcmp( pbuf, "]" ) ){
									index++;
									break;
								}
							}
						}
					}
					else{
						int32_t stmtchildmunit = gdt_addnodeelement( _ppool, stmtnode );
						GDT_NODE* stmtchildnode = (GDT_NODE*)GDT_POINTER( _ppool, stmtchildmunit );
						index = gdt_parse_rel( _ppool, pscript, stmtchildnode, token_list, index );
						if( token_list[index].type == ID_OP )
						{
							pbuf = (char*)GDT_POINTER( _ppool, token_list[index].buf_munit );
							if( !strcmp( ">=",pbuf ) 
								|| !strcmp( "<=",pbuf )
								|| !strcmp( ">",pbuf )
								|| !strcmp( "<",pbuf )
								|| !strcmp( "||",pbuf )
								|| !strcmp( "&&",pbuf )
							)
							{
								stmtchildnode = (GDT_NODE*)GDT_POINTER( _ppool, stmtchildmunit );
								gdt_addelement( _ppool, stmtchildnode, ELEMENT_CMP, token_list[index].buf_munit );
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

int gdt_parse_rel( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT* pscript, GDT_NODE* node, GDT_TOKEN* pt, int index )
{
	int32_t tmpmunit;
	char* pbuf;
	GDT_TOKENS *ptokens = (GDT_TOKENS*)GDT_POINTER( _ppool, pscript->tokens_munit );
	index = gdt_parse_cmp( _ppool, pscript, node, pt, index );
	if( index >= ptokens->currentpos ){
		return index;
	}
	pbuf = (char*)GDT_POINTER( _ppool, pt[index].buf_munit );
	if( !strcmp( "||", pbuf ) || !strcmp( "&&", pbuf ) )
	{
		tmpmunit = pt[index].buf_munit;
		index++;
		index = gdt_parse_cmp( _ppool, pscript, node, pt, index );
		gdt_addelement( _ppool, node, ELEMENT_OP, tmpmunit );
	}
	return index;
}

int gdt_parse_cmp( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT* pscript, GDT_NODE* node, GDT_TOKEN* pt, int index )
{
	int32_t tmpmunit;
	char* pbuf;
	GDT_TOKENS *ptokens = (GDT_TOKENS*)GDT_POINTER( _ppool, pscript->tokens_munit );
	index = gdt_parse_addsub( _ppool, pscript, node, pt, index );
	if( index >= ptokens->currentpos ){
		return index;
	}
	pbuf = (char*)GDT_POINTER( _ppool, pt[index].buf_munit );
	if( !strcmp( ">=",pbuf ) 
		|| !strcmp( "<=",pbuf )
		|| !strcmp( ">",pbuf )
		|| !strcmp( "<",pbuf )
		|| !strcmp( "==",pbuf )
	)
	{
		tmpmunit = pt[index].buf_munit;
		index++;
		index = gdt_parse_addsub( _ppool, pscript, node, pt, index );
		gdt_addelement( _ppool, node, ELEMENT_OP, tmpmunit );
	}
	return index;
}

int gdt_parse_addsub( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT* pscript, GDT_NODE* node, GDT_TOKEN* pt, int index )
{
	int32_t tmpmunit;
	char* pbuf;
	GDT_TOKENS *ptokens = (GDT_TOKENS*)GDT_POINTER( _ppool, pscript->tokens_munit );
	index = gdt_parse_muldiv( _ppool, pscript, node, pt, index );
	if( index >= ptokens->currentpos ){
		return index;
	}
	pbuf = (char*)GDT_POINTER( _ppool, pt[index].buf_munit );
	if( !strcmp( "+", pbuf ) )
	{
		tmpmunit = pt[index].buf_munit;
		index++;
		index = gdt_parse_muldiv( _ppool, pscript, node, pt, index );
		gdt_addelement( _ppool, node, ELEMENT_OP, tmpmunit );
	}
	else if( !strcmp( "-", pbuf ) )
	{
		tmpmunit = pt[index].buf_munit;
		index++;
		index = gdt_parse_muldiv( _ppool, pscript, node, pt, index );
		gdt_addelement( _ppool, node, ELEMENT_OP, tmpmunit );
	}
	return index;
}

int gdt_parse_muldiv( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT* pscript, GDT_NODE* node, GDT_TOKEN* pt, int index )
{
	int32_t tmpmunit;
	char* pbuf;
	GDT_TOKENS *ptokens = (GDT_TOKENS*)GDT_POINTER( _ppool, pscript->tokens_munit );
	index = gdt_parse_expr( _ppool, pscript, node, pt, index );
	if( index >= ptokens->currentpos ){
		return index;
	}
	pbuf = (char*)GDT_POINTER( _ppool, pt[index].buf_munit );
	if( !strcmp( "*", pbuf ) )
	{
		tmpmunit = pt[index].buf_munit;
		index++;
		index = gdt_parse_expr( _ppool, pscript, node, pt, index );
		gdt_addelement( _ppool, node, ELEMENT_OP, tmpmunit );
	}
	else if( !strcmp( "/", pbuf ) )
	{
		tmpmunit = pt[index].buf_munit;
		index++;
		index = gdt_parse_expr( _ppool, pscript, node, pt, index );
		gdt_addelement( _ppool, node, ELEMENT_OP, tmpmunit );
	}
	else if( !strcmp( "%", pbuf ) )
	{
		tmpmunit = pt[index].buf_munit;
		index++;
		index = gdt_parse_expr( _ppool, pscript, node, pt, index );
		gdt_addelement( _ppool, node, ELEMENT_OP, tmpmunit );
	}
	return index;
}

int gdt_parse_expr( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT* pscript, GDT_NODE* node, GDT_TOKEN* pt, int index )
{
	if( !strcmp( "(", (char*)GDT_POINTER( _ppool, pt[index].buf_munit ) ) )
	{
		index++;
		index = gdt_parse_rel( _ppool, pscript, node, pt, index );
		if( !strcmp( ")", (char*)GDT_POINTER( _ppool, pt[index].buf_munit ) ) )
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
			gdt_addelement( _ppool, node, ELEMENT_LITERAL_NUM, pt[index].buf_munit );
			index++;
		}
		else if( pt[index].type == ID_STR )
		{
			gdt_addelement( _ppool, node, ELEMENT_LITERAL_STR, pt[index].buf_munit );
			index++;
		}
		else if( pt[index].type == ID_FLOAT )
		{
			gdt_addelement( _ppool, node, ELEMENT_LITERAL_FLOAT, pt[index].buf_munit );
			index++;
		}
		else if( pt[index].type == ID_SYMBOL )
		{
			index = gdt_parse_symbol( _ppool, pscript, node, index );
		}
		else{
			if( !strcmp((char*)GDT_POINTER( _ppool, pt[index].buf_munit ),"-") ){
				int32_t tmpmunit = pt[index].buf_munit;
				index++;
				index = gdt_parse_expr( _ppool, pscript, node, pt, index );
				gdt_addelement( _ppool, node, ELEMENT_OP_LITERAL_MINUS, tmpmunit );
			}
			else{

			}
		}
	}
	return index;
}

int gdt_parse_symbol( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT* pscript, GDT_NODE* node, int index )
{
	GDT_TOKENS *ptokens = (GDT_TOKENS*)GDT_POINTER( _ppool, pscript->tokens_munit );
	GDT_TOKEN *token_list;
	GDT_NODE *childnode;
	GDT_NODE *stmtnode;
	int32_t childmunit;
	int32_t stmtmunit;
	int32_t tmpmunit;
	char* pbuf;
	int tmpindex;
	if( index >= ptokens->currentpos ){
		printf("invalid index3\n");
		return index;
	}
	token_list = ( GDT_TOKEN* )GDT_POINTER( _ppool, ptokens->token_munit );
	tmpmunit = token_list[index].buf_munit;
	pbuf = (char*)GDT_POINTER( _ppool, tmpmunit );
	if( !strcmp( pbuf, "def" ) ){
		index++;
		if( index >= ptokens->currentpos ){
			printf("invalid index4\n");
			return index;
		}
		index = gdt_parse_function( _ppool, pscript, node, index );
		return index;
	}
	index++;
	if( index >= ptokens->currentpos ){
		printf("invalid index5\n");
		return index;
	}
	pbuf = (char*)GDT_POINTER( _ppool, token_list[index].buf_munit );
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
		childmunit = gdt_addnodeelement( _ppool, node );
		childnode = (GDT_NODE*)GDT_POINTER( _ppool, childmunit );
		gdt_addelement( _ppool, childnode, ELEMENT_VALIABLE, tmpmunit );
		stmtmunit = gdt_addnodeelement( _ppool, childnode );
		stmtnode = (GDT_NODE*)GDT_POINTER( _ppool, stmtmunit );
		tmpmunit = token_list[index].buf_munit;
		index++;

		if( token_list[index].type == ID_SIGN && !strcmp( "[", (char*)GDT_POINTER( _ppool, token_list[index].buf_munit ) ) ){
			index = gdt_parse_array( _ppool, pscript, stmtnode, token_list, index );
		}
		else{
			while( index < ptokens->currentpos )
			{
				tmpindex = index;
				stmtnode = (GDT_NODE*)GDT_POINTER( _ppool, stmtmunit );
				index = gdt_parse_rel( _ppool, pscript, stmtnode, token_list, index );
				if( token_list[index].type == ID_SIGN )
				{
					pbuf = (char*)GDT_POINTER( _ppool, token_list[index].buf_munit );
					if( !strcmp( pbuf, ")" ) )
					{
						index++;
					}
				}
				if( token_list[index].type == ID_SIGN )
				{
					pbuf = (char*)GDT_POINTER( _ppool, token_list[index].buf_munit );
					if( !strcmp( ";",pbuf ) )
					{
						index++;
						break;
					}
				}
				else if( token_list[index].type == ID_OP )
				{
					pbuf = (char*)GDT_POINTER( _ppool, token_list[index].buf_munit );
					if( !strcmp( ">=",pbuf ) 
						|| !strcmp( "<=",pbuf )
						|| !strcmp( ">",pbuf )
						|| !strcmp( "<",pbuf )
						|| !strcmp( "||",pbuf )
						|| !strcmp( "&&",pbuf )
					)
					{
						gdt_addelement( _ppool, stmtnode, ELEMENT_CMP, token_list[index].buf_munit );
						index++;
					}
					// 無名関数
					if( !strcmp( "=>",pbuf ) )
					{
						gdt_addelement( _ppool, stmtnode, ELEMENT_CMP, token_list[index].buf_munit );
						index++;
						index = gdt_parse_block( _ppool, pscript, stmtnode, index );
					}
				}
				if( index == tmpindex ){
					printf( "invalid index6\n" );
					break;
				}
			}
		}
		childnode = (GDT_NODE*)GDT_POINTER( _ppool, childmunit );
		gdt_addelement( _ppool, childnode, ELEMENT_ASSIGNMENT, tmpmunit );
	}
	else if( !strcmp( "(", pbuf ) )
	{
		index--;
		index = gdt_parse_function_args( _ppool, pscript, node, index );
	}
	else if( !strcmp( "[", pbuf ) )
	{
		index--;
		index = gdt_parse_array_index( _ppool, pscript, node, index );
	}
	else if( !strcmp( "return", (char*)GDT_POINTER( _ppool, tmpmunit ) ) ){
		childmunit = gdt_addnodeelement( _ppool, node );
		childnode = (GDT_NODE*)GDT_POINTER( _ppool, childmunit );
		gdt_addelement( _ppool, childnode, ELEMENT_RETURN, tmpmunit );
		stmtmunit = gdt_addnodeelement( _ppool, childnode );
		stmtnode = (GDT_NODE*)GDT_POINTER( _ppool, stmtmunit );
		index = gdt_parse_rel( _ppool, pscript, stmtnode, token_list, index );
	}
	else{
		gdt_addelement( _ppool, node, ELEMENT_VALIABLE, tmpmunit );
	}
	return index;
}

int gdt_parse_function( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT* pscript, GDT_NODE* node, int index )
{
	int32_t rootnode_munit = -1;
	GDT_NODE *functionrootnode;
	GDT_TOKENS *ptokens = (GDT_TOKENS*)GDT_POINTER( _ppool, pscript->tokens_munit );
	GDT_TOKEN *token_list;
	char* pfunctionname;
	if( index >= ptokens->currentpos ){
		printf("invalid index7\n");
		return index;
	}
	token_list = ( GDT_TOKEN* )GDT_POINTER( _ppool, ptokens->token_munit );
	pfunctionname = (char*)GDT_POINTER( _ppool, token_list[index].buf_munit );
	do{
		rootnode_munit = gdt_createrootnode( _ppool );
		if( rootnode_munit <= 0 ){
			break;
		}
		functionrootnode = (GDT_NODE*)GDT_POINTER( _ppool, rootnode_munit );
		index = gdt_parse_function_args( _ppool, pscript, functionrootnode, index );
		index = gdt_parse_block( _ppool, pscript, functionrootnode, index );
		gdt_add_user_function( _ppool, pscript->self_munit, pfunctionname, rootnode_munit, 0 );
	}while( false );
	return index;
}

int gdt_parse_function_args( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT* pscript, GDT_NODE* node, int index )
{
	GDT_TOKENS *ptokens = (GDT_TOKENS*)GDT_POINTER( _ppool, pscript->tokens_munit );
	GDT_TOKEN *token_list;
	GDT_NODE *childnode;
	GDT_NODE *stmtnode;
	int32_t childmunit;
	int32_t stmtmunit;
	int32_t tmpmunit;
	char* pbuf;
	int tmpindex;
	if( index >= ptokens->currentpos ){
		printf("invalid index8\n");
		return index;
	}
	token_list = ( GDT_TOKEN* )GDT_POINTER( _ppool, ptokens->token_munit );
	tmpmunit = token_list[index].buf_munit;
	index++;
	if( index >= ptokens->currentpos ){
		printf("invalid index9\n");
		return index;
	}
	pbuf = (char*)GDT_POINTER( _ppool, token_list[index].buf_munit );
	do{
		childmunit = gdt_addnodeelement( _ppool, node );
		childnode = (GDT_NODE*)GDT_POINTER( _ppool, childmunit );
		gdt_addelement( _ppool, childnode, ELEMENT_FUNCTION, tmpmunit );
		if( !strcmp( "(", pbuf ) )
		{
			index++;
			if( token_list[index].type == ID_SIGN && !strcmp( (char*)GDT_POINTER( _ppool, token_list[index].buf_munit ), ")" ) )
			{
				index++;
			}
			else{
				stmtmunit = gdt_addnodeelement( _ppool, childnode );
				stmtnode = (GDT_NODE*)GDT_POINTER( _ppool, stmtmunit );
				while( index < ptokens->currentpos )
				{
					tmpindex = index;
					if( token_list[index].type == ID_SIGN )
					{
						pbuf = (char*)GDT_POINTER( _ppool, token_list[index].buf_munit );
						if( !strcmp( pbuf, "," ) ){
							childnode = (GDT_NODE*)GDT_POINTER( _ppool, childmunit );
							stmtmunit = gdt_addnodeelement( _ppool, childnode );
							stmtnode = (GDT_NODE*)GDT_POINTER( _ppool, stmtmunit );
							index++;
						}
						else if( !strcmp( pbuf, ")" ) ){
							index++;
							break;
						}
					}
					stmtnode = (GDT_NODE*)GDT_POINTER( _ppool, stmtmunit );
					index = gdt_parse_rel( _ppool, pscript, stmtnode, token_list, index );
					if( token_list[index].type == ID_OP )
					{
						pbuf = (char*)GDT_POINTER( _ppool, token_list[index].buf_munit );
						if( !strcmp( ">=",pbuf ) 
							|| !strcmp( "<=",pbuf )
							|| !strcmp( ">",pbuf )
							|| !strcmp( "<",pbuf )
							|| !strcmp( "||",pbuf )
							|| !strcmp( "&&",pbuf )
						)
						{
							stmtnode = (GDT_NODE*)GDT_POINTER( _ppool, stmtmunit );
							gdt_addelement( _ppool, stmtnode, ELEMENT_CMP, token_list[index].buf_munit );
							index++;
						}
					}
					if( index == tmpindex ){
						printf( "invalid index10\n" );
						break;
					}
				}
			}
		}
	}while( false );
	return index;
}

int gdt_parse_array_index( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT* pscript, GDT_NODE* node, int index )
{
	GDT_TOKENS *ptokens = (GDT_TOKENS*)GDT_POINTER( _ppool, pscript->tokens_munit );
	GDT_TOKEN *token_list;
	GDT_NODE *childnode;
	GDT_NODE *stmtnode;
	int32_t childmunit;
	int32_t stmtmunit;
	int32_t tmpmunit;
	char* pbuf;
	int tmpindex;
	if( index >= ptokens->currentpos ){
		printf("invalid index11\n");
		return index;
	}
	token_list = ( GDT_TOKEN* )GDT_POINTER( _ppool, ptokens->token_munit );
	tmpmunit = token_list[index].buf_munit;
	index++;
	if( index >= ptokens->currentpos ){
		printf("invalid index12\n");
		return index;
	}
	pbuf = (char*)GDT_POINTER( _ppool, token_list[index].buf_munit );
	do{
		childmunit = gdt_addnodeelement( _ppool, node );
		childnode = (GDT_NODE*)GDT_POINTER( _ppool, childmunit );
		gdt_addelement( _ppool, childnode, ELEMENT_ARRAY_REFERENCE, tmpmunit );
		if( !strcmp( "[", pbuf ) )
		{
			stmtmunit = gdt_addnodeelement( _ppool, childnode );
			stmtnode = (GDT_NODE*)GDT_POINTER( _ppool, stmtmunit );
			index++;
			while( index < ptokens->currentpos )
			{
				tmpindex = index;
				if( token_list[index].type == ID_SIGN )
				{
					pbuf = (char*)GDT_POINTER( _ppool, token_list[index].buf_munit );
					if( !strcmp( pbuf, "]" ) ){
						index++;
						break;
					}
				}
				else if( token_list[index].type == ID_OP ){
					break;
				}
				stmtnode = (GDT_NODE*)GDT_POINTER( _ppool, stmtmunit );
				index = gdt_parse_rel( _ppool, pscript, stmtnode, token_list, index );
				if( token_list[index].type == ID_OP )
				{
					pbuf = (char*)GDT_POINTER( _ppool, token_list[index].buf_munit );
					if( !strcmp( ">=",pbuf ) 
						|| !strcmp( "<=",pbuf )
						|| !strcmp( ">",pbuf )
						|| !strcmp( "<",pbuf )
						|| !strcmp( "||",pbuf )
						|| !strcmp( "&&",pbuf )
					)
					{
						stmtnode = (GDT_NODE*)GDT_POINTER( _ppool, stmtmunit );
						gdt_addelement( _ppool, stmtnode, ELEMENT_CMP, token_list[index].buf_munit );
						index++;
					}
				}
				if( token_list[index].type == ID_SIGN )
				{
					pbuf = (char*)GDT_POINTER( _ppool, token_list[index].buf_munit );
					if( !strcmp( pbuf, "]" ) ){
						index++;
						if( index >= ptokens->currentpos ){
							printf( "invalid index14\n" );
							break;
						}
						pbuf = (char*)GDT_POINTER( _ppool, token_list[index].buf_munit );
						if( !strcmp( pbuf, "[" ) ){
							childnode = (GDT_NODE*)GDT_POINTER( _ppool, childmunit );
							stmtmunit = gdt_addnodeelement( _ppool, childnode );
							stmtnode = (GDT_NODE*)GDT_POINTER( _ppool, stmtmunit );
							index++;
						}
					}
					else{
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
					pbuf = (char*)GDT_POINTER( _ppool, token_list[index].buf_munit );
					if( !strcmp( pbuf, ";" ) ){
						break;
					}
				}
			}
		}
	}while( false );
	return index;
}

int gdt_parse_if( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT* pscript, GDT_NODE* node, int index )
{
	char* pbuf;
	int tmpindex;
	int32_t tmpmunit;
	GDT_TOKENS *ptokens = (GDT_TOKENS*)GDT_POINTER( _ppool, pscript->tokens_munit );
	GDT_TOKEN *token_list;
	GDT_NODE *childnode;
	int32_t childmunit;
	GDT_NODE *ifstmtnode;
	int32_t ifstmtmunit;
	GDT_NODE *ifexprnode;
	int32_t ifexprmunit;
	token_list = ( GDT_TOKEN* )GDT_POINTER( _ppool, ptokens->token_munit );
	childmunit = gdt_addnodeelement( _ppool, node );
	childnode = (GDT_NODE*)GDT_POINTER( _ppool, childmunit );
	gdt_addelement( _ppool, childnode, ELEMENT_IF, token_list[index].buf_munit );
	index++;
	do{
		if( token_list[index].type != ID_SIGN )
		{
			break;
		}
		if( !strcmp( "(",(char*)GDT_POINTER( _ppool, token_list[index].buf_munit ) ) )
		{
			index++;
			ifstmtmunit = gdt_addnodeelement( _ppool, childnode );
			ifstmtnode = (GDT_NODE*)GDT_POINTER( _ppool, ifstmtmunit );
			ifexprmunit = gdt_addnodeelement( _ppool, ifstmtnode );
			ifexprnode = (GDT_NODE*)GDT_POINTER( _ppool, ifexprmunit );
			while( index < ptokens->currentpos )
			{
				tmpindex = index;
				index = gdt_parse_rel( _ppool, pscript, ifexprnode, token_list, index );
				if( token_list[index].type == ID_SIGN )
				{
					if( !strcmp( ")",(char*)GDT_POINTER( _ppool, token_list[index].buf_munit ) ) ){
						index++;
						break;
					}
				}
				else if( token_list[index].type == ID_OP )
				{
					pbuf = (char*)GDT_POINTER( _ppool, token_list[index].buf_munit );
					if( !strcmp ( "==", pbuf )
						|| !strcmp( ">=",pbuf ) 
						|| !strcmp( "<=",pbuf )
						|| !strcmp( ">",pbuf )
						|| !strcmp( "<",pbuf )
						|| !strcmp( "||",pbuf )
						|| !strcmp( "&&",pbuf )
					)
					{
						ifstmtnode = (GDT_NODE*)GDT_POINTER( _ppool, ifstmtmunit );
						gdt_addelement( _ppool, ifstmtnode, ELEMENT_CMP, token_list[index].buf_munit );
						index++;
						ifexprmunit = gdt_addnodeelement( _ppool, ifstmtnode );
						ifexprnode = (GDT_NODE*)GDT_POINTER( _ppool, ifexprmunit );
					}
				}
				if( index == tmpindex ){
					printf( "gdt_parse_if invalid index\n" );
					break;
				}
			}
			if( index < ptokens->currentpos )
			{
				childnode = (GDT_NODE*)GDT_POINTER( _ppool, childmunit );
				index = gdt_parse_code_core( _ppool, pscript, childnode, index );
				if( index < 0 ){
					break;
				}
			}
		}
		else if( !strcmp( "{",(char*)GDT_POINTER( _ppool, token_list[index].buf_munit ) ) )
		{
			childnode = (GDT_NODE*)GDT_POINTER( _ppool, childmunit );
			if( index < ptokens->currentpos )
			{
				childnode = (GDT_NODE*)GDT_POINTER( _ppool, childmunit );
				index = gdt_parse_code_core( _ppool, pscript, childnode, index );
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
				tmpmunit = gdt_create_munit( _ppool, 8, MEMORY_TYPE_DEFAULT );
				pbuf = (char*)GDT_POINTER( _ppool, tmpmunit );
				gdt_strcopy( pbuf, "elseif", GDT_PUNIT_USIZE( _ppool, tmpmunit ) );
				childnode = (GDT_NODE*)GDT_POINTER( _ppool, childmunit );
				gdt_addelement( _ppool, childnode, ELEMENT_ELSEIF, tmpmunit );
				index++;
			}
			else{
				if( token_list[index-1].type == ID_SYS_ELSE ){
					childnode = (GDT_NODE*)GDT_POINTER( _ppool, childmunit );
					gdt_addelement( _ppool, childnode, ELEMENT_ELSE, tmpmunit );
				}else{
					childnode = (GDT_NODE*)GDT_POINTER( _ppool, childmunit );
					gdt_addelement( _ppool, childnode, ELEMENT_ELSEIF, tmpmunit );
				}
			}
		}
	}while( index < ptokens->currentpos );
	return index;
}

int gdt_parse_while( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT* pscript, GDT_NODE* node, int index )
{
	int tmpindex;
	GDT_TOKENS *ptokens = (GDT_TOKENS*)GDT_POINTER( _ppool, pscript->tokens_munit );
	GDT_TOKEN *token_list;
	GDT_NODE *childnode;
	int32_t childmunit;
	GDT_NODE *whilestmtnode;
	int32_t whilestmtmunit;
	GDT_NODE *whileexprnode;
	int32_t whileexprmunit;
	token_list = ( GDT_TOKEN* )GDT_POINTER( _ppool, ptokens->token_munit );
	childmunit = gdt_addnodeelement( _ppool, node );
	childnode = (GDT_NODE*)GDT_POINTER( _ppool, childmunit );
	gdt_addelement( _ppool, childnode, ELEMENT_WHILE, token_list[index].buf_munit );
	index++;
	do{
		if( token_list[index].type != ID_SIGN )
		{
			break;
		}
		if( !strcmp( "(",(char*)GDT_POINTER( _ppool, token_list[index].buf_munit ) ) )
		{
			index++;
			childnode = (GDT_NODE*)GDT_POINTER( _ppool, childmunit );
			whilestmtmunit = gdt_addnodeelement( _ppool, childnode );
			whilestmtnode = (GDT_NODE*)GDT_POINTER( _ppool, whilestmtmunit );
			whileexprmunit = gdt_addnodeelement( _ppool, whilestmtnode );
			whileexprnode = (GDT_NODE*)GDT_POINTER( _ppool, whileexprmunit );
			while( index < ptokens->currentpos )
			{
				tmpindex = index;
				index = gdt_parse_rel( _ppool, pscript, whileexprnode, token_list, index );
				if( token_list[index].type == ID_SIGN )
				{
					if( !strcmp( ")",(char*)GDT_POINTER( _ppool, token_list[index].buf_munit ) ) ){
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
				childnode = (GDT_NODE*)GDT_POINTER( _ppool, childmunit );
				index = gdt_parse_code_core( _ppool, pscript, childnode, index );
				if( index < 0 ){
					break;
				}
			}
		}
	}while( index < ptokens->currentpos );
	return index;
}

void gdt_exec( GDT_MEMORY_POOL* _ppool, int32_t *p_unitid )
{
	GDT_SCRIPT *pscript;
	GDT_NODE *rootnode;
	GDT_NODE *childnode;
	GDT_NODE *node;
	int i;
	GDT_NODE* workelemlist;
#ifdef __GDT_DEBUG__
#ifdef __WINDOWS__
	LARGE_INTEGER start_pc, end_pc, freq_pc;
	double sec_pc;
	QueryPerformanceFrequency( &freq_pc );
	QueryPerformanceCounter( &start_pc );
#else
	struct timeval start_timeval, end_timeval;
	double sec_timeofday;
	gettimeofday( &start_timeval, NULL );
#endif
#endif
	pscript = (GDT_SCRIPT *)GDT_POINTER( _ppool, ( *p_unitid ) );
	rootnode = (GDT_NODE*)GDT_POINTER( _ppool, pscript->rootnode_munit );
	node = rootnode;
	//gdt_elementdump( _ppool, node );
	do{
		workelemlist = ( GDT_NODE* )GDT_POINTER( _ppool, node->element_munit );
		if( workelemlist == NULL )
		{
			printf("workelemlist is null\n");
			break;
		}
		for( i = 0; i < node->pos; i++ )
		{
			childnode = (GDT_NODE*)GDT_POINTER( _ppool, workelemlist[i].element_munit );
			gdt_exec_core( _ppool, pscript, childnode );
		}
	}while( false );
#ifdef __GDT_DEBUG__
#ifdef __WINDOWS__
	QueryPerformanceCounter( &end_pc );
	sec_pc = (end_pc.QuadPart - start_pc.QuadPart) / (double)freq_pc.QuadPart;
	//printf(  "## gdt_exec time = %lf[s]\n", sec_pc );
#else
	gettimeofday( &end_timeval, NULL );
	sec_timeofday = (end_timeval.tv_sec - start_timeval.tv_sec)
			+ (end_timeval.tv_usec - start_timeval.tv_usec) / 1000000.0;
	//printf(  "## gdt_exec time = %lf[s]\n", sec_timeofday );
#endif
#endif
}

int32_t gdt_exec_core( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT *pscript, GDT_NODE* node )
{
	int32_t returnmunit = -1;
	int32_t exec_expr_munit;
	GDT_NODE* workelemlist;
	workelemlist = ( GDT_NODE* )GDT_POINTER( _ppool, node->element_munit );
	switch( workelemlist[0].id )
	{
		case ELEMENT_VALIABLE:
		{
			GDT_NODE* vnode = (GDT_NODE*)GDT_POINTER( _ppool, workelemlist[1].element_munit );
			exec_expr_munit = gdt_exec_expr( _ppool, pscript, vnode, pscript->int_cache_munit );
			if( exec_expr_munit > 0 )
			{
				GDT_FUNCTION_RETURN* pfret = (GDT_FUNCTION_RETURN*)GDT_POINTER( _ppool, exec_expr_munit );
				if( pfret->munit > 0)
				{
					char* value = (char*)GDT_POINTER( _ppool, pfret->munit );
					if( value != NULL )
					{
						char* opname = (char*)GDT_POINTER( _ppool, workelemlist[2].element_munit );
						if( !strcmp( opname, "=" ) )
						{
							int32_t myhash = gdt_get_hash( _ppool, pscript->v_hash_munit, (char*)GDT_POINTER( _ppool, workelemlist[0].element_munit ) );
							if( myhash > 0 && GDT_PUNIT_USIZE(_ppool,myhash) >= GDT_PUNIT_USIZE(_ppool,pfret->munit) ){
								memcpy( (char*)(GDT_POINTER(_ppool,myhash)), (char*)(GDT_POINTER(_ppool,pfret->munit)),GDT_PUNIT_USIZE(_ppool,pfret->munit) );
							}
							else{
								if( 0 >= ( myhash = gdt_create_munit( _ppool, GDT_PUNIT_USIZE(_ppool,pfret->munit), MEMORY_TYPE_DEFAULT ) ) ){
									break;
								}
								memcpy( (char*)(GDT_POINTER(_ppool,myhash)), (char*)(GDT_POINTER(_ppool,pfret->munit)),GDT_PUNIT_USIZE(_ppool,pfret->munit) );
								gdt_add_hash( _ppool, pscript->v_hash_munit, workelemlist[0].element_munit, myhash, pfret->id );
							}
						}
						else{
							printf("unsupported opname : %s\n", opname);
						}
					}
				}
			}
			else{
				printf("null result : %s \n", (char*)GDT_POINTER( _ppool, workelemlist[0].element_munit ));
			}
		}
		break;
		case ELEMENT_FUNCTION:
		{
			int32_t returnmunit = gdt_exec_function( _ppool, pscript, node );
			if( returnmunit > 0 ){
				// TODO : check return
			}
		}
		break;
		case ELEMENT_IF:
		{
			int32_t returnmunit = gdt_exec_if( _ppool, pscript, node );
			if( returnmunit > 0 ){
				// TODO : check return
			}
		}
		break;
		case ELEMENT_WHILE:
		{
			int32_t returnmunit = gdt_exec_while( _ppool, pscript, node );
			if( returnmunit > 0 ){
				// TODO : check return
			}
		}
		break;
		case ELEMENT_CHILD:
		{
			int i;
			GDT_NODE* childnode = NULL;
			for( i = 0; i < node->pos; i++ )
			{
				if( workelemlist[i].element_munit == -1 ){
					printf( "workelemlist[%d].element_munit == -1\n" , i );
				}
				else{
					childnode = (GDT_NODE*)GDT_POINTER( _ppool, workelemlist[i].element_munit );
					if( childnode != NULL ){
						returnmunit = gdt_exec_core( _ppool, pscript, childnode );
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
			GDT_NODE* vnode = (GDT_NODE*)GDT_POINTER( _ppool, workelemlist[1].element_munit );
			returnmunit = gdt_exec_expr( _ppool, pscript, vnode, 0 );
		}
		break;
		default:
			printf("unsupported element : %d\n", workelemlist[0].id );
		break;
	}
	return returnmunit;
}

int32_t gdt_exec_function( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT *pscript, GDT_NODE* node )
{
	int32_t resultmunit = -1;
	int32_t funcmunit = -1;
	int32_t argsmunit = -1;
	int32_t exec_expr_munit;
	GDT_NODE* vnode;
	GDT_FUNCTION_RETURN* pret;
	GDT_FUNCTION_INFO* pfuncinfo;
	GDT_ARRAY* parray = NULL;
	int i;
	GDT_NODE* workelemlist = ( GDT_NODE* )GDT_POINTER( _ppool, node->element_munit );
	do{
		funcmunit = gdt_get_hash( _ppool, pscript->f_hash_munit, (char*)GDT_POINTER( _ppool, workelemlist[0].element_munit ) );
		if( funcmunit <=0 ){
			printf("call undefined function : %s\n", (char*)GDT_POINTER( _ppool, workelemlist[0].element_munit ) );
			break;
		}
		if( node->pos > 1 )
		{
			for( i = 1; i < node->pos; i++ )
			{
				vnode = (GDT_NODE*)GDT_POINTER( _ppool, workelemlist[i].element_munit );
				if( 0 >= vnode->element_munit ){
					continue;
				}
				GDT_NODE* vworkelemlist = ( GDT_NODE* )GDT_POINTER( _ppool, vnode->element_munit );
				if( vworkelemlist[0].id == ELEMENT_CHILD )
				{
					GDT_NODE* cnode = (GDT_NODE*)GDT_POINTER( _ppool, vworkelemlist[0].element_munit );
					GDT_NODE* cworkelemlist = ( GDT_NODE* )GDT_POINTER( _ppool, cnode->element_munit );
					if( cworkelemlist[0].id == ELEMENT_FUNCTION )
					{
						int32_t returnmunit = gdt_exec_function( _ppool, pscript, cnode );
						if( returnmunit > 0 ){
							pret = (GDT_FUNCTION_RETURN*)GDT_POINTER( _ppool, returnmunit );
							if( pret != NULL )
							{
								gdt_array_push( _ppool, &argsmunit, pret->id, pret->munit );
							}
						}
					}
					else if( cworkelemlist[0].id == ELEMENT_ARRAY_REFERENCE ){
						int32_t returnmunit = gdt_exec_array_get( _ppool, pscript, cnode );
						if( returnmunit > 0 ){
							pret = (GDT_FUNCTION_RETURN*)GDT_POINTER( _ppool, returnmunit );
							if( pret != NULL )
							{
								gdt_array_push( _ppool, &argsmunit, pret->id, pret->munit );
							}
						}
					}
				}
				else{
					exec_expr_munit = gdt_exec_expr( _ppool, pscript, vnode, 0 );
					if( exec_expr_munit > 0 ){
						GDT_FUNCTION_RETURN* pfret = (GDT_FUNCTION_RETURN*)GDT_POINTER( _ppool, exec_expr_munit );
						gdt_array_push( _ppool, &argsmunit, pfret->id, pfret->munit );
					}
				}
			}
		}
		pfuncinfo = (GDT_FUNCTION_INFO*)GDT_POINTER( _ppool, funcmunit );
		if( pfuncinfo->type == FUNCTION_TYPE_SYSTEM ){
			parray = (GDT_ARRAY*)GDT_POINTER( _ppool, argsmunit );
			pret = (GDT_FUNCTION_RETURN*)pfuncinfo->func( _ppool, parray );
			if( pret != NULL )
			{
				resultmunit = pret->refid;
			}
		}
		else if( pfuncinfo->type == FUNCTION_TYPE_USER ){
			GDT_NODE* function_node = NULL;
			GDT_NODE* function_header_node = NULL;
			GDT_NODE* function_body_node = NULL;
			GDT_NODE* function_workelemlist = NULL;
			GDT_NODE* function_headeremlist = NULL;
			GDT_ARRAY_ELEMENT* elm = NULL;
			if( 0 >= pfuncinfo->userfunction_munit ){
				printf("null user function error\n");
				break;
			}
			function_node = (GDT_NODE*)GDT_POINTER( _ppool, pfuncinfo->userfunction_munit );
			function_workelemlist = (GDT_NODE*)GDT_POINTER( _ppool, function_node->element_munit );
			function_header_node = (GDT_NODE*)GDT_POINTER( _ppool, function_workelemlist[0].element_munit );
			function_headeremlist = (GDT_NODE*)GDT_POINTER( _ppool, function_header_node->element_munit );
			if( argsmunit > 0 )
			{
				parray = (GDT_ARRAY*)GDT_POINTER( _ppool, argsmunit );
				if( parray != NULL ){
					elm = (GDT_ARRAY_ELEMENT*)GDT_POINTER( _ppool, parray->munit );
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
							GDT_NODE* argnode = (GDT_NODE*)GDT_POINTER( _ppool, function_headeremlist[i].element_munit );
							GDT_NODE* argelmlist = (GDT_NODE*)GDT_POINTER( _ppool, argnode->element_munit );
							gdt_add_hash_value( 
								_ppool
								, pscript->v_hash_munit
								, (char*)GDT_POINTER( _ppool, argelmlist[0].element_munit )
								, (char*)GDT_POINTER( _ppool, elm[i-1].munit )
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
			function_body_node = (GDT_NODE*)GDT_POINTER( _ppool, function_workelemlist[1].element_munit );
			resultmunit = gdt_exec_core( _ppool, pscript, function_body_node );
		}
	}while( false );
	return resultmunit;
}

int32_t gdt_exec_array_create( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT *pscript, GDT_NODE* node )
{
	int32_t resultmunit = -1;
	int32_t argsmunit = -1;
	int32_t hash_munit = -1;
	GDT_NODE* vnode;
	GDT_NODE* workelemlist;
	GDT_NODE* vworkelemlist;
	GDT_NODE* cworkelemlist;
	GDT_NODE* cnode;
	GDT_FUNCTION_RETURN* pret;
	workelemlist = ( GDT_NODE* )GDT_POINTER( _ppool, node->element_munit );
	if( node->pos > 1 )
	{
		int i;
		for( i = 1; i < node->pos; i++ )
		{
			vnode = (GDT_NODE*)GDT_POINTER( _ppool, workelemlist[i].element_munit );
			if( 0 >= vnode->element_munit ){
				continue;
			}
			vworkelemlist = ( GDT_NODE* )GDT_POINTER( _ppool, vnode->element_munit );
			if( vworkelemlist[0].id == ELEMENT_CHILD )
			{
				cnode = (GDT_NODE*)GDT_POINTER( _ppool, vworkelemlist[0].element_munit );
				cworkelemlist = ( GDT_NODE* )GDT_POINTER( _ppool, cnode->element_munit );
				if( cworkelemlist[0].id == ELEMENT_FUNCTION )
				{
					int32_t returnmunit = gdt_exec_function( _ppool, pscript, cnode );
					if( returnmunit > 0 ){
						pret = (GDT_FUNCTION_RETURN*)GDT_POINTER( _ppool, returnmunit );
						if( pret != NULL )
						{
							gdt_array_push( _ppool, &argsmunit, pret->id, pret->munit );
						}
					}
				}
				else if( cworkelemlist[0].id == ELEMENT_ARRAY ){
					int32_t returnmunit = gdt_exec_array_create( _ppool, pscript, cnode );
					if( returnmunit > 0 ){
						pret = (GDT_FUNCTION_RETURN*)GDT_POINTER( _ppool, returnmunit );
						if( pret != NULL )
						{
							gdt_array_push( _ppool, &argsmunit, pret->id, pret->munit );
						}
					}
				}
				else{
					int32_t exec_expr_munit = gdt_exec_expr( _ppool, pscript, cnode, 0 );
					if( exec_expr_munit > 0 ){
						pret = (GDT_FUNCTION_RETURN*)GDT_POINTER( _ppool, exec_expr_munit );
						if( vnode->pos == 1 ){
							gdt_array_push( _ppool, &argsmunit, pret->id, pret->munit );
						}
						else{
							cnode = (GDT_NODE*)GDT_POINTER( _ppool, vworkelemlist[1].element_munit );
							cworkelemlist = ( GDT_NODE* )GDT_POINTER( _ppool, cnode->element_munit );
							GDT_FUNCTION_RETURN tmpret;
							tmpret.munit = pret->munit;
							tmpret.id = pret->id;
							int32_t hash_value_exec_expr_munit = exec_expr_munit = gdt_exec_expr( _ppool, pscript, cnode, 0 );
							if( hash_value_exec_expr_munit > 0 )
							{
								if( hash_munit <= 0){
									if( 0 >= ( hash_munit = gdt_create_hash( _ppool, pscript->hash_alloc_size ) ) ){
										break;
									}
								}
								GDT_FUNCTION_RETURN* pfhret = (GDT_FUNCTION_RETURN*)GDT_POINTER( _ppool, hash_value_exec_expr_munit );
								gdt_add_hash( _ppool, hash_munit, tmpret.munit, pfhret->munit, pfhret->id );
							}
						}
					}
				}
			}
		}
	}
	if( hash_munit > 0 ){
		resultmunit = gdt_create_return(_ppool, pscript, hash_munit, ELEMENT_HASH );
	}
	else{
		resultmunit = gdt_create_return(_ppool, pscript, argsmunit, ELEMENT_ARRAY );
	}
	return resultmunit;
}

int32_t gdt_exec_array_get( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT *pscript, GDT_NODE* node )
{
	char* hashname;
	int32_t resultmunit = -1;
	int32_t returnmunit;
	GDT_FUNCTION_RETURN* pret = NULL;
	GDT_NODE* workelemlist = ( GDT_NODE* )GDT_POINTER( _ppool, node->element_munit );
	GDT_NODE tmp_node;
	tmp_node.id = 0;
	tmp_node.element_munit = -1;
	hashname = (char*)GDT_POINTER( _ppool, workelemlist[0].element_munit );
	if( node->pos > 1 )
	{
		int32_t hash_id = gdt_get_hash_id( _ppool, pscript->v_hash_munit, hashname );
		if( hash_id == ELEMENT_ARRAY|| hash_id == ELEMENT_HASH ){
			tmp_node.id = hash_id;
			tmp_node.element_munit = gdt_get_hash( _ppool, pscript->v_hash_munit, hashname );
		}
		int i;
		GDT_NODE* vnode;
		GDT_NODE* vworkelemlist;
		for( i = 1; i < node->pos; i++ )
		{
			vnode = (GDT_NODE*)GDT_POINTER( _ppool, workelemlist[i].element_munit );
			if( 0 >= vnode->element_munit ){
				continue;
			}
			vworkelemlist = ( GDT_NODE* )GDT_POINTER( _ppool, vnode->element_munit );
			if( vworkelemlist[0].id == ELEMENT_CHILD )
			{
				GDT_NODE* cnode = (GDT_NODE*)GDT_POINTER( _ppool, vworkelemlist[0].element_munit );
				GDT_NODE* cworkelemlist = ( GDT_NODE* )GDT_POINTER( _ppool, cnode->element_munit );
				if( cworkelemlist[0].id == ELEMENT_FUNCTION )
				{
					returnmunit = gdt_exec_function( _ppool, pscript, cnode );
					if( returnmunit > 0 ){
						pret = (GDT_FUNCTION_RETURN*)GDT_POINTER( _ppool, returnmunit );
					}
				}
				else if( cworkelemlist[0].id == ELEMENT_ARRAY ){
					returnmunit = gdt_exec_array_create( _ppool, pscript, cnode );
					if( returnmunit > 0 ){
						pret = (GDT_FUNCTION_RETURN*)GDT_POINTER( _ppool, returnmunit );
					}
				}
			}
			else{
				if( vworkelemlist[0].id == ELEMENT_FUNCTION )
				{
					returnmunit = gdt_exec_function( _ppool, pscript, vnode );
					if( returnmunit > 0 ){
						pret = (GDT_FUNCTION_RETURN*)GDT_POINTER( _ppool, returnmunit );
					}
				}
				else if( vworkelemlist[0].id == ELEMENT_ARRAY ){
					returnmunit = gdt_exec_array_create( _ppool, pscript, vnode );
					if( returnmunit > 0 ){
						pret = (GDT_FUNCTION_RETURN*)GDT_POINTER( _ppool, returnmunit );
					}
				}
				else{
					int32_t exec_expr_munit = gdt_exec_expr( _ppool, pscript, vnode, 0 );
					if( exec_expr_munit > 0 ){
						pret = (GDT_FUNCTION_RETURN*)GDT_POINTER( _ppool, exec_expr_munit );
					}
				}
			}
			if( tmp_node.id == ELEMENT_ARRAY )
			{
				int array_index = -1;
				GDT_ARRAY *parray = (GDT_ARRAY*)GDT_POINTER( _ppool, tmp_node.element_munit );
				if( parray == NULL || parray->len <= 0 ){
					break;
				}
				if( pret != NULL ){
					array_index = atoi( (char*)GDT_POINTER( _ppool, pret->munit ) );
					pret = NULL;
				}
				if( array_index >= 0 && array_index < parray->len ){
					if( parray == NULL ){
						printf("invalid array access\n");
					}
					else{
						GDT_ARRAY_ELEMENT* retelm = (GDT_ARRAY_ELEMENT*)GDT_POINTER( _ppool, parray->munit )+( array_index );
						tmp_node.id =  retelm->id;
						tmp_node.element_munit = retelm->munit;
					}
				}
			}
			else if( tmp_node.id == ELEMENT_HASH )
			{
				if( pret != NULL ){
					tmp_node.id = gdt_get_hash_id( _ppool, tmp_node.element_munit, (char*)GDT_POINTER(_ppool,pret->munit) );
					tmp_node.element_munit = gdt_get_hash( _ppool, tmp_node.element_munit, (char*)GDT_POINTER(_ppool,pret->munit) );
				}
			}
		}
	}
	if( tmp_node.element_munit > 0 ){
		resultmunit = gdt_create_return(_ppool, pscript, tmp_node.element_munit, tmp_node.id );
	}
	return resultmunit;
}

int32_t gdt_exec_expr( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT *pscript, GDT_NODE* node, int32_t result_cache_munit )
{
	GDT_NODE *workelemlist;
	GDT_NODE *tmpnode;
	int32_t tmpmunit;
	int is_numeric = 1;
	char* pbuf;
	char* opbuf;
	int32_t tmpv1, tmpv2;
	size_t csize = 0;
	int32_t tmpid = 0;
	int32_t workingmunit = pscript->s_hash_munit;//-1;
	workelemlist = ( GDT_NODE* )GDT_POINTER( _ppool, node->element_munit );
	int i;
	for( i = 0; i < node->pos; i++ )
	{
		tmpnode = &workelemlist[i];
		if( tmpnode->id == ELEMENT_LITERAL_NUM || tmpnode->id == ELEMENT_LITERAL_STR )
		{
			gdt_array_push( _ppool, &workingmunit, tmpnode->id, tmpnode->element_munit );
			if( tmpnode->id != ELEMENT_LITERAL_NUM ){
				is_numeric = 0;
			}
		}
		else if( tmpnode->id == ELEMENT_VALIABLE )
		{
			char* pname = (char*)GDT_POINTER( _ppool, tmpnode->element_munit );
			int32_t tmphashmunit = gdt_get_hash( _ppool, pscript->v_hash_munit, pname );
			csize = 0;
			tmpid = 0;
			if( tmphashmunit > 0 ){
				csize = GDT_PUNIT_USIZE( _ppool, tmphashmunit );
				tmpid = gdt_get_hash_id( _ppool, pscript->v_hash_munit, pname );
				gdt_array_push( _ppool, &workingmunit, tmpid, tmphashmunit );
			}
			//int32_t tmphashmunit;
			//csize = 0;
			//tmpid = 0;
			//if( tmpnode->exec_tmp_munit == -1 )
			//{
			//	char* pname;
			//	pname = (char*)GDT_POINTER( _ppool, tmpnode->element_munit );
			//	tmphashmunit = gdt_get_hash( _ppool, pscript->v_hash_munit, pname );
			//	if( tmphashmunit > 0 ){
			//		tmpid = gdt_get_hash_id( _ppool, pscript->v_hash_munit, pname );
			//		tmpnode->exec_tmp_id = tmpid;
			//	}
			//	tmpnode->exec_tmp_munit = tmphashmunit;
			//}
			//else{
			//	tmphashmunit = tmpnode->exec_tmp_munit;
			//	tmpid = tmpnode->exec_tmp_id;
			//}
			//if( tmphashmunit > 0 ){
			//	csize = GDT_PUNIT_USIZE( _ppool, tmphashmunit );
			//	gdt_array_push( _ppool, &workingmunit, tmpid, tmphashmunit );
			//}
			else{
				int32_t *pi;
				csize = NUMERIC_BUFFER_SIZE;
				tmpid = ELEMENT_LITERAL_NUM;
				if( 0 >= ( tmpmunit = gdt_create_munit( _ppool, csize, MEMORY_TYPE_DEFAULT ) ) ){
					return GDT_SYSTEM_ERROR;
				}
				pbuf = (char*)GDT_POINTER( _ppool, tmpmunit );
				pi = (int32_t*)(GDT_POINTER( _ppool, tmpmunit )+GDT_PUNIT_USIZE(_ppool,tmpmunit)-sizeof(int32_t));
				pbuf[0] = '0';
				pbuf[1] = '\0';
				*pi = 0;
				gdt_array_push( _ppool, &workingmunit, tmpid, tmpmunit );
			}
			if( tmpid != ELEMENT_LITERAL_NUM ){
				is_numeric = 0;
			}
		}
		else if( tmpnode->id == ELEMENT_CHILD )
		{
			GDT_NODE* cnode = ( GDT_NODE* )GDT_POINTER( _ppool, tmpnode->element_munit );
			GDT_NODE* cworkelemlist = ( GDT_NODE* )GDT_POINTER( _ppool, cnode->element_munit );
			if( cworkelemlist[0].id == ELEMENT_ARRAY ){
				int32_t returnmunit = gdt_exec_array_create( _ppool, pscript, cnode );
				if( returnmunit > 0 ){
					GDT_FUNCTION_RETURN* pret = (GDT_FUNCTION_RETURN*)GDT_POINTER( _ppool, returnmunit );
					if( pret != NULL )
					{
						gdt_array_push( _ppool, &workingmunit, pret->id, pret->munit );
					}
				}
			}
			else if( cworkelemlist[0].id == ELEMENT_ARRAY_REFERENCE )
			{
				int32_t returnmunit = gdt_exec_array_get( _ppool, pscript, cnode );
				if( returnmunit > 0 ){
					GDT_FUNCTION_RETURN* pret = (GDT_FUNCTION_RETURN*)GDT_POINTER( _ppool, returnmunit );
					if( pret != NULL )
					{
						gdt_array_push( _ppool, &workingmunit, pret->id, pret->munit );
					}
				}
			}
			else if( cworkelemlist[0].id == ELEMENT_FUNCTION )
			{
				int32_t returnmunit = gdt_exec_function( _ppool, pscript, cnode );
				if( returnmunit > 0 ){
					GDT_FUNCTION_RETURN* pret = (GDT_FUNCTION_RETURN*)GDT_POINTER( _ppool, returnmunit );
					if( pret != NULL )
					{
						gdt_array_push( _ppool, &workingmunit, pret->id, pret->munit );
					}
				}
			}
		}
		else if( tmpnode->id == ELEMENT_OP )
		{
			opbuf = (char*)GDT_POINTER( _ppool, tmpnode->element_munit );
			if( is_numeric == 1 )
			{
				int array_len = gdt_array_length( _ppool, workingmunit );
				if( array_len == 0 ){
					printf( "empty stack error\n" );
					return GDT_SYSTEM_ERROR;
				}
				else if( array_len == 1 ){
					if( !strcmp( opbuf, "-" ) )
					{
						GDT_ARRAY_ELEMENT* pelm;
						pelm = gdt_array_pop( _ppool, workingmunit );
						pbuf = (char*)GDT_POINTER( _ppool, pelm->munit );
						tmpv1 = atoi( pbuf );
						tmpv1 = -tmpv1;
					}
					else if( !strcmp( opbuf, "+" ) )
					{
						GDT_ARRAY_ELEMENT* pelm;
						pelm = gdt_array_pop( _ppool, workingmunit );
						pbuf = (char*)GDT_POINTER( _ppool, pelm->munit );
						tmpv1 = atoi( pbuf );
					}
					else{
						printf( "invalid op error\n" );
						return GDT_SYSTEM_ERROR;
					}
				}
				else{
					GDT_ARRAY_ELEMENT* pelm;
					pelm = gdt_array_pop( _ppool, workingmunit );
					tmpv1 = (*(int32_t*)( GDT_POINTER( _ppool, pelm->munit ) + GDT_PUNIT_USIZE( _ppool, pelm->munit ) - sizeof( int32_t) ) );
					pelm = gdt_array_pop( _ppool, workingmunit );
					tmpv2 = (*(int32_t*)( GDT_POINTER( _ppool, pelm->munit ) + GDT_PUNIT_USIZE( _ppool, pelm->munit ) - sizeof( int32_t) ) );
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
						return GDT_SYSTEM_ERROR;
					}
				}
				gdt_array_push_integer( _ppool, &workingmunit, tmpv1 );
			}
			else{
				GDT_ARRAY_ELEMENT *pelm1, *pelm2;
				pelm1 = gdt_array_pop( _ppool, workingmunit );
				pelm2 = gdt_array_pop( _ppool, workingmunit );
				if( pelm1 == NULL && pelm2 == NULL)
				{
					printf( "invalid element error\n" );
					return GDT_SYSTEM_ERROR;
				}
				else if( pelm1 == NULL )
				{
					printf( "invalid element error\n" );
					return GDT_SYSTEM_ERROR;
				}
				else{
					//if( !strcmp( opbuf, "+" ) )
					if( *opbuf == '+' && *(opbuf+1) == '\0' )
					{
						if( pelm2 == NULL )
						{
							tmpmunit = gdt_create_munit( _ppool, GDT_PUNIT_USIZE( _ppool, pelm1->munit ) + 5, MEMORY_TYPE_DEFAULT );
							if( tmpmunit < 0 ){
								return GDT_SYSTEM_ERROR;
							}
							pbuf = (char*)GDT_POINTER( _ppool, tmpmunit );
							snprintf( pbuf, GDT_PUNIT_USIZE( _ppool, tmpmunit ), "%s%s", (char*)GDT_POINTER( _ppool, pelm1->munit ), "NULL" );
							gdt_array_push( _ppool, &workingmunit, ELEMENT_LITERAL_STR, tmpmunit );
						}
						else{
							if( pelm2->id == ELEMENT_LITERAL_BIN || pelm1->id == ELEMENT_LITERAL_BIN )
							{
								size_t s1, s2;
								if( pelm2->id == ELEMENT_LITERAL_BIN ){
									s1=GDT_PUNIT_USIZE( _ppool, pelm2->munit );
								}
								else{
									s1=gdt_strlen( (char*)GDT_POINTER( _ppool, pelm2->munit ) );
								}
								if( pelm1->id == ELEMENT_LITERAL_BIN ){
									s2=GDT_PUNIT_USIZE( _ppool, pelm1->munit );
								}
								else{
									s2=gdt_strlen( (char*)GDT_POINTER( _ppool, pelm1->munit ) );
								}
								tmpmunit = gdt_create_munit( _ppool, s1 + s2, MEMORY_TYPE_DEFAULT );
								if( tmpmunit < 0 ){
									return GDT_SYSTEM_ERROR;
								}
								pbuf = (char*)GDT_POINTER( _ppool, tmpmunit );
								if( pelm2->id == ELEMENT_LITERAL_BIN ){
									memcpy( pbuf, (char*)GDT_POINTER( _ppool, pelm2->munit ), GDT_PUNIT_USIZE( _ppool, pelm2->munit ) );
									pbuf+=GDT_PUNIT_USIZE( _ppool, pelm2->munit );
								}
								else{
									memcpy( pbuf, (char*)GDT_POINTER( _ppool, pelm2->munit ), gdt_strlen( (char*)GDT_POINTER( _ppool, pelm2->munit ) ) );
									pbuf+=gdt_strlen( (char*)GDT_POINTER( _ppool, pelm2->munit ) );
								}
								if( pelm1->id == ELEMENT_LITERAL_BIN ){
									memcpy( pbuf, (char*)GDT_POINTER( _ppool, pelm1->munit ), GDT_PUNIT_USIZE( _ppool, pelm1->munit ) );
									pbuf+=GDT_PUNIT_USIZE( _ppool, pelm1->munit );
								}
								else{
									memcpy( pbuf, (char*)GDT_POINTER( _ppool, pelm1->munit ), gdt_strlen( (char*)GDT_POINTER( _ppool, pelm1->munit ) ) );
									pbuf+=gdt_strlen( (char*)GDT_POINTER( _ppool, pelm1->munit ) );
								}
								*pbuf='\0';
							}
							else{
								tmpmunit = gdt_create_munit( _ppool, GDT_PUNIT_USIZE( _ppool, pelm1->munit ) + GDT_PUNIT_USIZE( _ppool, pelm2->munit ), MEMORY_TYPE_DEFAULT );
								if( tmpmunit < 0 ){
									return GDT_SYSTEM_ERROR;
								}
								pbuf = (char*)GDT_POINTER( _ppool, tmpmunit );
								size_t elm1_len = gdt_strlen((char*)GDT_POINTER( _ppool, pelm1->munit ));
								size_t elm2_len = gdt_strlen((char*)GDT_POINTER( _ppool, pelm2->munit ));
								gdt_strcopy( pbuf, (char*)GDT_POINTER( _ppool, pelm2->munit ), GDT_PUNIT_USIZE( _ppool, tmpmunit ) );
								gdt_strcopy( pbuf+elm2_len, (char*)GDT_POINTER( _ppool, pelm1->munit ), GDT_PUNIT_USIZE( _ppool, tmpmunit ) );
								*( pbuf+elm2_len+elm1_len ) = '\0';
								//memset(pbuf, 0, GDT_PUNIT_USIZE(_ppool,tmpmunit));
								//snprintf( pbuf, GDT_PUNIT_USIZE( _ppool, tmpmunit ), "%s%s", (char*)GDT_POINTER( _ppool, pelm2->munit ), (char*)GDT_POINTER( _ppool, pelm1->munit ) );
							}
							gdt_array_push( _ppool, &workingmunit, ELEMENT_LITERAL_STR, tmpmunit );
						}
					}
					else if( !strcmp( opbuf, "==" ) ){
						if( pelm2 == NULL )
						{
							tmpv1 = 0;
						}
						else{
							tmpv1 = !strcmp( (char*)GDT_POINTER( _ppool, pelm2->munit ), (char*)GDT_POINTER( _ppool, pelm1->munit ) );
						}
						gdt_array_push_integer( _ppool, &workingmunit, tmpv1 );
					}
					else if( !strcmp( opbuf, "!=" ) ){
						if( pelm2 == NULL )
						{
							tmpv1 = 1;
						}
						else{
							tmpv1 = strcmp( (char*)GDT_POINTER( _ppool, pelm2->munit ), (char*)GDT_POINTER( _ppool, pelm1->munit ) ) ? 0 : 1;
						}
						gdt_array_push_integer( _ppool, &workingmunit, tmpv1 );
					}else{
						return GDT_SYSTEM_ERROR;
					}
				}
			}
		}
		else if( tmpnode->id == ELEMENT_OP_LITERAL_MINUS ){
			opbuf = (char*)GDT_POINTER( _ppool, tmpnode->element_munit );
			if( is_numeric == 1 )
			{
				if( gdt_array_length( _ppool, workingmunit ) == 0 ){
					printf( "empty stack error\n" );
					return GDT_SYSTEM_ERROR;
				}
				else{
					GDT_ARRAY_ELEMENT* pelm = gdt_array_pop( _ppool, workingmunit );
					pbuf = (char*)GDT_POINTER( _ppool, pelm->munit );
					tmpv1 = atoi( pbuf );
					tmpv1 = -tmpv1;
				}
				gdt_array_push_integer( _ppool, &workingmunit, tmpv1 );
			}
		}
	}
	int32_t resultmunit = -1;
	if( workingmunit > 0 )
	{
		GDT_ARRAY_ELEMENT* resultelm = gdt_array_pop( _ppool, workingmunit );
		if( resultelm != NULL )
		{
			if( resultelm->id == ELEMENT_LITERAL_NUM )
			{
				if( 0 < result_cache_munit )
				{
					if( GDT_PUNIT_USIZE(_ppool,result_cache_munit) >= GDT_PUNIT_USIZE(_ppool,resultelm->munit) ){
						tmpmunit = result_cache_munit;
					}
					else{
						tmpmunit = gdt_create_munit( _ppool, GDT_PUNIT_USIZE( _ppool, resultelm->munit ), MEMORY_TYPE_DEFAULT );
						if( tmpmunit <= 0 ){
							return GDT_SYSTEM_ERROR;
						}
					}
				}
				else{
					tmpmunit = gdt_create_munit( _ppool, GDT_PUNIT_USIZE( _ppool, resultelm->munit ), MEMORY_TYPE_DEFAULT );
					if( tmpmunit <= 0 ){
						return GDT_SYSTEM_ERROR;
					}
				}
				pbuf = (char*)GDT_POINTER( _ppool, tmpmunit );
				memcpy( pbuf, (char*)GDT_POINTER(_ppool,resultelm->munit), GDT_PUNIT_USIZE( _ppool, resultelm->munit ) );
				(*(int32_t*)(GDT_POINTER(_ppool,tmpmunit)+GDT_PUNIT_USIZE(_ppool,tmpmunit)-sizeof(int32_t))) = (*(int32_t*)(GDT_POINTER(_ppool,resultelm->munit)+GDT_PUNIT_USIZE(_ppool,resultelm->munit)-sizeof(int32_t)));
				resultmunit = gdt_create_return(_ppool, pscript, tmpmunit, resultelm->id );
			}
			else if( resultelm->id == ELEMENT_ARRAY ){
				resultmunit = gdt_create_return(_ppool, pscript, resultelm->munit, resultelm->id );
			}
			else if( resultelm->id == ELEMENT_HASH ){
				resultmunit = gdt_create_return(_ppool, pscript, resultelm->munit, resultelm->id );
			}
			else{
				if( resultelm->munit > 0 )
				{
					tmpmunit = gdt_create_munit( _ppool, GDT_PUNIT_USIZE( _ppool, resultelm->munit ), MEMORY_TYPE_DEFAULT );
					if( tmpmunit <= 0 ){
						return GDT_SYSTEM_ERROR;
					}
					pbuf = (char*)GDT_POINTER( _ppool, tmpmunit );
					memcpy( pbuf, (char*)GDT_POINTER(_ppool,resultelm->munit), GDT_PUNIT_USIZE( _ppool, tmpmunit ) );
					resultmunit = gdt_create_return(_ppool, pscript, tmpmunit, resultelm->id );
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

int32_t gdt_exec_if( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT *pscript, GDT_NODE* node )
{
	GDT_NODE* vnode;
	GDT_NODE* workelemlist;
	int32_t resultmunit = -1;
	GDT_NODE* vworkelemlist;
	int32_t exec_expr_munit;
	GDT_NODE* cnode;
	GDT_NODE* blocknode;
	GDT_NODE* blockworkelemlist;
	do{
		if( node == NULL ){
			printf( "node is null\n" );
			break;
		}
		workelemlist = ( GDT_NODE* )GDT_POINTER( _ppool, node->element_munit );
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
				vnode = (GDT_NODE*)GDT_POINTER( _ppool, workelemlist[nodeindex+1].element_munit );
				vworkelemlist = ( GDT_NODE* )GDT_POINTER( _ppool, vnode->element_munit );
				cnode = (GDT_NODE*)GDT_POINTER( _ppool, vworkelemlist[0].element_munit );
				exec_expr_munit = gdt_exec_expr( _ppool, pscript, cnode, 0 );
				if( exec_expr_munit <= 0 )
				{
					printf("exec if error1. \n");
					break;
				}
				GDT_FUNCTION_RETURN* pfret = (GDT_FUNCTION_RETURN*)GDT_POINTER( _ppool, exec_expr_munit );
				if( pfret->munit <= 0 )
				{
					printf("exec if error2. \n");
					break;
				}
				if( 1 == (*(int32_t*)( GDT_POINTER( _ppool, pfret->munit ) + GDT_PUNIT_USIZE( _ppool, pfret->munit ) - sizeof( int32_t) ) ) )
				//if( !strcmp( (char*)GDT_POINTER( _ppool, pfret->munit ), "1" ) )
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
				blocknode = (GDT_NODE*)GDT_POINTER( _ppool, workelemlist[nodeindex].element_munit );
				blockworkelemlist = ( GDT_NODE* )GDT_POINTER( _ppool, blocknode->element_munit );
				int j;
				GDT_NODE* cblocknode;
				for( j = 0; j < blocknode->pos; j++ )
				{
					cblocknode = (GDT_NODE*)GDT_POINTER( _ppool, blockworkelemlist[j].element_munit );
					resultmunit = gdt_exec_core( _ppool, pscript, cblocknode );
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

int32_t gdt_exec_while( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT *pscript, GDT_NODE* node )
{
	GDT_NODE* vnode;
	GDT_NODE* workelemlist;
	int32_t resultmunit = -1;
	int32_t exec_expr_munit;
	GDT_NODE* vworkelemlist;
	GDT_NODE* cnode;
	GDT_NODE* blocknode;
	GDT_NODE* blockworkelemlist;
	int j;
	GDT_NODE* cblocknode;
	int32_t while_ret_munit = gdt_create_munit( _ppool, NUMERIC_BUFFER_SIZE, MEMORY_TYPE_DEFAULT );
	workelemlist = ( GDT_NODE* )GDT_POINTER( _ppool, node->element_munit );
	do{
		if( workelemlist[0].id == ELEMENT_WHILE )
		{
			if( workelemlist[1].id != ELEMENT_CHILD )
			{
				printf( "element not child : %d\n", workelemlist[1].id );
				break;
			}
			vnode = (GDT_NODE*)GDT_POINTER( _ppool, workelemlist[1].element_munit );
			vworkelemlist = ( GDT_NODE* )GDT_POINTER( _ppool, vnode->element_munit );
			exec_expr_munit = -1;
			cnode = (GDT_NODE*)GDT_POINTER( _ppool, vworkelemlist[0].element_munit );
			GDT_FUNCTION_RETURN* pfret;
			blocknode = (GDT_NODE*)GDT_POINTER( _ppool, workelemlist[2].element_munit );
			blockworkelemlist = ( GDT_NODE* )GDT_POINTER( _ppool, blocknode->element_munit );
			do{
				exec_expr_munit = gdt_exec_expr( _ppool, pscript, cnode, while_ret_munit );
				if( exec_expr_munit > 0 )
				{
					pfret = (GDT_FUNCTION_RETURN*)GDT_POINTER( _ppool, exec_expr_munit );
					if( 1 == (*(int32_t*)( GDT_POINTER( _ppool, pfret->munit ) + GDT_PUNIT_USIZE( _ppool, pfret->munit ) - sizeof( int32_t) ) ) )
					//if( !strcmp( (char*)GDT_POINTER( _ppool, pfret->munit ), "1" ) )
					//if( *((char*)GDT_POINTER( _ppool, pfret->munit )) == 0x31 )
					{
						for( j = 0; j < blocknode->pos; j++ )
						{
							cblocknode = (GDT_NODE*)GDT_POINTER( _ppool, blockworkelemlist[j].element_munit );
							gdt_exec_core( _ppool, pscript, cblocknode );
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

int32_t gdt_create_return( GDT_MEMORY_POOL* _ppool, GDT_SCRIPT *pscript, int32_t data_munit, int32_t id )
{
	GDT_FUNCTION_RETURN* pret;
	pret = (GDT_FUNCTION_RETURN*)GDT_POINTER( _ppool, pscript->return_munit );
	pret->refid = pscript->return_munit;
	pret->id	= id;
	pret->munit	= data_munit;
	return pscript->return_munit;
}

int gdt_add_system_function( GDT_MEMORY_POOL* _ppool, int32_t munit, char* functionname, GDT_SCRIPT_FUNCTION func, int32_t id )
{
	char* pbuf;
	int32_t namemunit;
	int32_t datamunit;
	GDT_FUNCTION_INFO* pfuncinfo;
	int error = 1;
	GDT_SCRIPT *pscript;
	do{
		pscript = (GDT_SCRIPT *)GDT_POINTER( _ppool, munit );
		namemunit = gdt_get_hash_name( _ppool, pscript->f_hash_munit, functionname );
		if( namemunit >= 0 )
		{
			printf("name munit found error : %d\n", namemunit);
			break;
		}
		namemunit = gdt_create_munit( _ppool, strlen( functionname )+1, MEMORY_TYPE_DEFAULT );
		pbuf = (char*)GDT_POINTER( _ppool, namemunit );
		gdt_strcopy( pbuf, functionname, GDT_PUNIT_USIZE( _ppool, namemunit ) );
		datamunit = gdt_create_munit( _ppool, sizeof( GDT_FUNCTION_INFO ), MEMORY_TYPE_DEFAULT );
		pfuncinfo = (GDT_FUNCTION_INFO*)GDT_POINTER( _ppool, datamunit );
		pfuncinfo->type = FUNCTION_TYPE_SYSTEM;
		pfuncinfo->func = func;
		pfuncinfo->userfunction_munit = -1;
		gdt_add_hash( _ppool, pscript->f_hash_munit, namemunit, datamunit, id );
		error = 0;
	}while( false );
	return error;
}

int gdt_add_user_function( GDT_MEMORY_POOL* _ppool, int32_t munit, char* functionname, int32_t function_munit, int32_t id )
{
	char* pbuf;
	int32_t namemunit;
	int32_t datamunit;
	GDT_FUNCTION_INFO* pfuncinfo;
	int error = 1;
	GDT_SCRIPT *pscript;
	do{
		pscript = (GDT_SCRIPT *)GDT_POINTER( _ppool, munit );
		namemunit = gdt_get_hash_name( _ppool, pscript->f_hash_munit, functionname );
		if( namemunit >= 0 )
		{
			printf("name munit found error : %d\n", namemunit);
			break;
		}
		namemunit = gdt_create_munit( _ppool, strlen( functionname )+1, MEMORY_TYPE_DEFAULT );
		pbuf = (char*)GDT_POINTER( _ppool, namemunit );
		gdt_strcopy( pbuf, functionname, GDT_PUNIT_USIZE( _ppool, namemunit ) );
		datamunit = gdt_create_munit( _ppool, sizeof( GDT_FUNCTION_INFO ), MEMORY_TYPE_DEFAULT );
		pfuncinfo = (GDT_FUNCTION_INFO*)GDT_POINTER( _ppool, datamunit );
		pfuncinfo->type = FUNCTION_TYPE_USER;
		pfuncinfo->func = NULL;
		pfuncinfo->userfunction_munit = function_munit;
		gdt_add_hash( _ppool, pscript->f_hash_munit, namemunit, datamunit, id );
		error = 0;
	}while( false );
	return error;
}

void* gdt_script_system_function_echo( GDT_MEMORY_POOL* _ppool, void* args )
{
	GDT_ARRAY* parray = (GDT_ARRAY*)args;
	GDT_ARRAY_ELEMENT* elm;
	GDT_FUNCTION_RETURN* pret;
	int32_t retmunit = -1;
	retmunit = gdt_create_munit( _ppool, sizeof( GDT_FUNCTION_RETURN ), MEMORY_TYPE_DEFAULT );
	pret = (GDT_FUNCTION_RETURN*)gdt_upointer( _ppool, retmunit );
	pret->id	= 0;
	pret->munit	= -1;
	if( parray != NULL )
	{
		elm = (GDT_ARRAY_ELEMENT*)gdt_upointer( _ppool, parray->munit );
		if( elm != NULL )
		{
			if( parray->len > 0 )
			{
				if( elm[0].id == ELEMENT_LITERAL_STR )
				{
					printf( "%s", (char*)gdt_upointer( _ppool, elm[0].munit ) );
				}
				else if( elm[0].id == ELEMENT_ARRAY )
				{
					gdt_array_dump( _ppool, elm[0].munit, 0 );
				}
				else if( elm[0].id == ELEMENT_HASH )
				{
					gdt_dump_hash( _ppool, elm[0].munit, 0 );
				}
			}
		}
	}
	return pret;
}

void* gdt_script_system_function_count( GDT_MEMORY_POOL* _ppool, void* args )
{
	GDT_ARRAY* parray = (GDT_ARRAY*)args;
	GDT_ARRAY_ELEMENT* elm;
	GDT_FUNCTION_RETURN* pret;
	int32_t retmunit = -1;
	retmunit = gdt_create_munit( _ppool, sizeof( GDT_FUNCTION_RETURN ), MEMORY_TYPE_DEFAULT );
	pret = (GDT_FUNCTION_RETURN*)gdt_upointer( _ppool, retmunit );
	pret->id	= 0;
	pret->munit	= -1;
	pret->refid = retmunit;
	if( parray != NULL )
	{
		elm = (GDT_ARRAY_ELEMENT*)gdt_upointer( _ppool, parray->munit );
		if( elm != NULL )
		{
			if( parray->len > 0 )
			{
				int32_t len_munit = gdt_create_munit( _ppool, NUMERIC_BUFFER_SIZE, MEMORY_TYPE_DEFAULT );
				int32_t len = 0;
				if( len_munit <= 0 ){
					return pret;
				}
				if( elm[0].id == ELEMENT_LITERAL_STR )
				{
					len = gdt_strlen( (char*)GDT_POINTER(_ppool,elm[0].munit) );
				}
				else if( elm[0].id == ELEMENT_ARRAY )
				{
					len = gdt_array_length( _ppool, elm[0].munit );
				}
				else if( elm[0].id == ELEMENT_HASH )
				{
					len = gdt_hash_length( _ppool, elm[0].munit );
				}
				char* pbuf = (char*)GDT_POINTER(_ppool,len_munit);
				gdt_itoa( len, pbuf, GDT_PUNIT_USIZE(_ppool,len_munit) );
				(*(int32_t*)(GDT_POINTER(_ppool,len_munit)+GDT_PUNIT_USIZE(_ppool,len_munit)-sizeof(int32_t))) = len;
				pret->munit = len_munit;
				pret->id = ELEMENT_LITERAL_NUM;
			}
		}
	}
	return pret;
}

void* gdt_script_system_function_file_exist( GDT_MEMORY_POOL* _ppool, void* args )
{
	GDT_ARRAY* parray = (GDT_ARRAY*)args;
	GDT_ARRAY_ELEMENT* elm;
	GDT_FUNCTION_RETURN* pret;
	int32_t retmunit = -1;
	retmunit = gdt_create_munit( _ppool, sizeof( GDT_FUNCTION_RETURN ), MEMORY_TYPE_DEFAULT );
	pret = (GDT_FUNCTION_RETURN*)gdt_upointer( _ppool, retmunit );
	pret->id	= 0;
	pret->munit	= -1;
	pret->refid = retmunit;
	if( parray != NULL )
	{
		elm = (GDT_ARRAY_ELEMENT*)gdt_upointer( _ppool, parray->munit );
		if( elm != NULL )
		{
			if( parray->len > 0 )
			{
				if( elm[0].id == ELEMENT_LITERAL_STR )
				{
					GDT_FILE_INFO info;
					int32_t len_munit = gdt_create_munit( _ppool, NUMERIC_BUFFER_SIZE, MEMORY_TYPE_DEFAULT );
					char* pbuf = (char*)GDT_POINTER(_ppool,len_munit);
					if( 0 != gdt_fget_info( (char*)GDT_POINTER( _ppool, elm[0].munit ), &info ) ){
						gdt_itoa( 0, pbuf, GDT_PUNIT_USIZE(_ppool,len_munit) );
						(*(int32_t*)(GDT_POINTER(_ppool,len_munit)+GDT_PUNIT_USIZE(_ppool,len_munit)-sizeof(int32_t))) = 0;
					}
					else{
						gdt_itoa( 1, pbuf, GDT_PUNIT_USIZE(_ppool,len_munit) );
						(*(int32_t*)(GDT_POINTER(_ppool,len_munit)+GDT_PUNIT_USIZE(_ppool,len_munit)-sizeof(int32_t))) = 1;
					}
					pret->munit = len_munit;
					pret->id = ELEMENT_LITERAL_NUM;
				}
			}
		}
	}
	return pret;
}

void* gdt_script_system_function_file_size( GDT_MEMORY_POOL* _ppool, void* args )
{
	GDT_ARRAY* parray = (GDT_ARRAY*)args;
	GDT_ARRAY_ELEMENT* elm;
	GDT_FUNCTION_RETURN* pret;
	int32_t retmunit = -1;
	retmunit = gdt_create_munit( _ppool, sizeof( GDT_FUNCTION_RETURN ), MEMORY_TYPE_DEFAULT );
	pret = (GDT_FUNCTION_RETURN*)gdt_upointer( _ppool, retmunit );
	pret->id	= 0;
	pret->munit	= -1;
	pret->refid = retmunit;
	if( parray != NULL )
	{
		elm = (GDT_ARRAY_ELEMENT*)gdt_upointer( _ppool, parray->munit );
		if( elm != NULL )
		{
			if( parray->len > 0 )
			{
				if( elm[0].id == ELEMENT_LITERAL_STR )
				{
					GDT_FILE_INFO info;
					int32_t len_munit = gdt_create_munit( _ppool, NUMERIC_BUFFER_SIZE, MEMORY_TYPE_DEFAULT );
					char* pbuf = (char*)GDT_POINTER(_ppool,len_munit);
					if( 0 != gdt_fget_info( (char*)GDT_POINTER( _ppool, elm[0].munit ), &info ) ){
						gdt_itoa( 0, pbuf, GDT_PUNIT_USIZE(_ppool,len_munit) );
						(*(int32_t*)(GDT_POINTER(_ppool,len_munit)+GDT_PUNIT_USIZE(_ppool,len_munit)-sizeof(int32_t))) = 0;
					}
					else{
						gdt_itoa( (int32_t)info.size, pbuf, GDT_PUNIT_USIZE(_ppool,len_munit) );
						(*(int32_t*)(GDT_POINTER(_ppool,len_munit)+GDT_PUNIT_USIZE(_ppool,len_munit)-sizeof(int32_t))) = (int32_t)info.size;
					}
					pret->munit = len_munit;
					pret->id = ELEMENT_LITERAL_NUM;
				}
			}
		}
	}
	return pret;
}

void* gdt_script_system_function_file_extension( GDT_MEMORY_POOL* _ppool, void* args )
{
	GDT_ARRAY* parray = (GDT_ARRAY*)args;
	GDT_ARRAY_ELEMENT* elm;
	GDT_FUNCTION_RETURN* pret;
	int32_t retmunit = -1;
	retmunit = gdt_create_munit( _ppool, sizeof( GDT_FUNCTION_RETURN ), MEMORY_TYPE_DEFAULT );
	pret = (GDT_FUNCTION_RETURN*)gdt_upointer( _ppool, retmunit );
	pret->id	= 0;
	pret->munit	= -1;
	pret->refid = retmunit;
	if( parray != NULL )
	{
		elm = (GDT_ARRAY_ELEMENT*)gdt_upointer( _ppool, parray->munit );
		if( elm != NULL )
		{
			if( parray->len > 0 )
			{
				if( elm[0].id == ELEMENT_LITERAL_STR )
				{
					int32_t string_munit = gdt_create_munit( _ppool, 32, MEMORY_TYPE_DEFAULT );
					if( string_munit <= 0 ){
						return pret;
					}
					if( GDT_SYSTEM_OK != gdt_get_extension( (char*)GDT_POINTER( _ppool, string_munit ), 32, (char*)GDT_POINTER( _ppool, elm[0].munit ) ) ){
						
					}
					pret->munit = string_munit;
					pret->id = ELEMENT_LITERAL_STR;
				}
			}
		}
	}
	return pret;
}

void* gdt_script_system_function_file_get( GDT_MEMORY_POOL* _ppool, void* args )
{
	GDT_ARRAY* parray = (GDT_ARRAY*)args;
	GDT_ARRAY_ELEMENT* elm;
	GDT_FUNCTION_RETURN* pret;
	int32_t retmunit = -1;
	retmunit = gdt_create_munit( _ppool, sizeof( GDT_FUNCTION_RETURN ), MEMORY_TYPE_DEFAULT );
	pret = (GDT_FUNCTION_RETURN*)gdt_upointer( _ppool, retmunit );
	pret->id	= 0;
	pret->munit	= -1;
	pret->refid = retmunit;
	if( parray != NULL )
	{
		elm = (GDT_ARRAY_ELEMENT*)gdt_upointer( _ppool, parray->munit );
		if( elm != NULL )
		{
			if( parray->len > 0 )
			{
				if( elm[0].id == ELEMENT_LITERAL_STR )
				{
					GDT_FILE_INFO info;
					if( 0 != gdt_fget_info( (char*)GDT_POINTER( _ppool, elm[0].munit ), &info ) ){
						return pret;
					}
					if( parray->len > 1 && elm[1].id == ELEMENT_LITERAL_STR && !strcmp("b",(char*)GDT_POINTER( _ppool, elm[1].munit )) ){
						int32_t string_munit = gdt_create_munit( _ppool, info.size, MEMORY_TYPE_DEFAULT );
						if( string_munit <= 0 ){
							return pret;
						}
						gdt_fread_bin( (char*)GDT_POINTER( _ppool, elm[0].munit ), (char*)GDT_POINTER(_ppool,string_munit), GDT_PUNIT_USIZE(_ppool,string_munit) );
						pret->munit = string_munit;
						pret->id = ELEMENT_LITERAL_BIN;
					}
					else{
						int32_t string_munit = gdt_create_munit( _ppool, info.size+1, MEMORY_TYPE_DEFAULT );
						if( string_munit <= 0 ){
							return pret;
						}
						gdt_fread( (char*)GDT_POINTER( _ppool, elm[0].munit ), (char*)GDT_POINTER(_ppool,string_munit), GDT_PUNIT_USIZE(_ppool,string_munit) );
						pret->munit = string_munit;
						pret->id = ELEMENT_LITERAL_STR;
					}
				}
			}
		}
	}
	return pret;
}

void* gdt_script_system_function_file_put( GDT_MEMORY_POOL* _ppool, void* args )
{
	GDT_ARRAY* parray = (GDT_ARRAY*)args;
	GDT_ARRAY_ELEMENT* elm;
	GDT_FUNCTION_RETURN* pret;
	int32_t retmunit = -1;
	retmunit = gdt_create_munit( _ppool, sizeof( GDT_FUNCTION_RETURN ), MEMORY_TYPE_DEFAULT );
	pret = (GDT_FUNCTION_RETURN*)gdt_upointer( _ppool, retmunit );
	pret->id	= 0;
	pret->munit	= -1;
	pret->refid = retmunit;
	if( parray != NULL )
	{
		elm = (GDT_ARRAY_ELEMENT*)gdt_upointer( _ppool, parray->munit );
		if( elm != NULL )
		{
			if( parray->len > 1 )
			{
				if( elm[0].id == ELEMENT_LITERAL_STR && elm[1].id == ELEMENT_LITERAL_STR )
				{
					int32_t string_munit = gdt_create_munit( _ppool, 8, MEMORY_TYPE_DEFAULT );
					if( 0 != gdt_fwrite( (char*)GDT_POINTER( _ppool, elm[0].munit ), (char*)GDT_POINTER( _ppool, elm[1].munit ), GDT_PUNIT_USIZE(_ppool,elm[1].munit) ) )
					{
						*((char*)GDT_POINTER(_ppool,string_munit)) = '0';
					}
					else{
						*((char*)GDT_POINTER(_ppool,string_munit)) = '1';
					}
					*((char*)GDT_POINTER(_ppool,string_munit)+1) = '\0';
					pret->munit = string_munit;
					pret->id = ELEMENT_LITERAL_STR;
				}
			}
		}
	}
	return pret;
}

void* gdt_script_system_function_file_add( GDT_MEMORY_POOL* _ppool, void* args )
{
	GDT_ARRAY* parray = (GDT_ARRAY*)args;
	GDT_ARRAY_ELEMENT* elm;
	GDT_FUNCTION_RETURN* pret;
	int32_t retmunit = -1;
	retmunit = gdt_create_munit( _ppool, sizeof( GDT_FUNCTION_RETURN ), MEMORY_TYPE_DEFAULT );
	pret = (GDT_FUNCTION_RETURN*)gdt_upointer( _ppool, retmunit );
	pret->id	= 0;
	pret->munit	= -1;
	pret->refid = retmunit;
	if( parray != NULL )
	{
		elm = (GDT_ARRAY_ELEMENT*)gdt_upointer( _ppool, parray->munit );
		if( elm != NULL )
		{
			if( parray->len > 1 )
			{
				if( elm[0].id == ELEMENT_LITERAL_STR && elm[1].id == ELEMENT_LITERAL_STR )
				{
					int32_t string_munit = gdt_create_munit( _ppool, 8, MEMORY_TYPE_DEFAULT );
					if( 0 != gdt_fwrite_a( (char*)GDT_POINTER( _ppool, elm[0].munit ), (char*)GDT_POINTER( _ppool, elm[1].munit ), GDT_PUNIT_USIZE(_ppool,elm[1].munit) ) )
					{
						*((char*)GDT_POINTER(_ppool,string_munit)) = '0';
					}
					else{
						*((char*)GDT_POINTER(_ppool,string_munit)) = '1';
					}
					*((char*)GDT_POINTER(_ppool,string_munit)+1) = '\0';
					pret->munit = string_munit;
					pret->id = ELEMENT_LITERAL_STR;
				}
			}
		}
	}
	return pret;
}

void* gdt_script_system_function_json_encode( GDT_MEMORY_POOL* _ppool, void* args )
{
	GDT_ARRAY* parray = (GDT_ARRAY*)args;
	GDT_ARRAY_ELEMENT* elm;
	GDT_FUNCTION_RETURN* pret;
	int32_t retmunit = -1;
	retmunit = gdt_create_munit( _ppool, sizeof( GDT_FUNCTION_RETURN ), MEMORY_TYPE_DEFAULT );
	pret = (GDT_FUNCTION_RETURN*)gdt_upointer( _ppool, retmunit );
	pret->id	= 0;
	pret->munit	= -1;
	pret->refid = retmunit;
	if( parray != NULL )
	{
		elm = (GDT_ARRAY_ELEMENT*)gdt_upointer( _ppool, parray->munit );
		if( elm != NULL )
		{
			if( parray->len > 0 )
			{
				if( elm[0].id == ELEMENT_HASH || elm[0].id == ELEMENT_ARRAY )
				{
					int32_t r_munit = gdt_make_json_root( _ppool, elm[0].munit, elm[0].id ); 
					int32_t variable_munit = gdt_json_encode( _ppool, (GDT_NODE*)GDT_POINTER( _ppool, r_munit ), 1024);
					pret->munit = variable_munit;
					pret->id = ELEMENT_LITERAL_STR;
				}
			}
		}
	}
	return pret;
}

void* gdt_script_system_function_json_decode( GDT_MEMORY_POOL* _ppool, void* args )
{
	GDT_ARRAY* parray = (GDT_ARRAY*)args;
	GDT_ARRAY_ELEMENT* elm;
	GDT_FUNCTION_RETURN* pret;
	int32_t retmunit = -1;
	retmunit = gdt_create_munit( _ppool, sizeof( GDT_FUNCTION_RETURN ), MEMORY_TYPE_DEFAULT );
	pret = (GDT_FUNCTION_RETURN*)gdt_upointer( _ppool, retmunit );
	pret->id	= 0;
	pret->munit	= -1;
	pret->refid = retmunit;
	if( parray != NULL )
	{
		elm = (GDT_ARRAY_ELEMENT*)gdt_upointer( _ppool, parray->munit );
		if( elm != NULL )
		{
			if( parray->len > 0 )
			{
				if( elm[0].id == ELEMENT_LITERAL_STR )
				{
					int32_t rootnode_munit = gdt_json_decode( _ppool, (char*)GDT_POINTER( _ppool, elm[0].munit ) );
					GDT_NODE *rootnode = (GDT_NODE*)GDT_POINTER( _ppool, rootnode_munit );
					GDT_NODE* workelemlist = ( GDT_NODE* )GDT_POINTER( _ppool, rootnode->element_munit );
					pret->munit = workelemlist[0].element_munit;
					pret->id = workelemlist[0].id;
				}
			}
		}
	}
	return pret;
}

void* gdt_script_system_function_gmtime( GDT_MEMORY_POOL* _ppool, void* args )
{
	GDT_FUNCTION_RETURN* pret;
	int32_t retmunit = -1;
	retmunit = gdt_create_munit( _ppool, sizeof( GDT_FUNCTION_RETURN ), MEMORY_TYPE_DEFAULT );
	pret = (GDT_FUNCTION_RETURN*)gdt_upointer( _ppool, retmunit );
	pret->id	= 0;
	pret->munit	= -1;
	pret->refid = retmunit;
	int32_t string_munit = gdt_create_munit( _ppool, 256, MEMORY_TYPE_DEFAULT );
	if( string_munit <= 0 ){
		printf( "string_munit error\n" );
		return pret;
	}
	gdt_utc_time( (char*)GDT_POINTER(_ppool,string_munit), GDT_PUNIT_USIZE(_ppool,string_munit) );
	pret->munit = string_munit;
	pret->id = ELEMENT_LITERAL_STR;
	return pret;
}

int32_t gdt_init_http_script( GDT_MEMORY_POOL* _ppool, const char* script_file, const char* ini_json_file )
{
	int32_t script_munit = -1;
	do{
		if( 0 >=( script_munit = gdt_init_script( _ppool, 128, 64 ) ) )
		{
			printf( "gdt_init_script error\n" );
			break;
		}
		GDT_SCRIPT *pscript = (GDT_SCRIPT *)GDT_POINTER( _ppool, script_munit );
		GDT_FILE_INFO info;
		if( 0 == gdt_fget_info( (char*)ini_json_file, &info ) ){
			int32_t json_string_munit = gdt_create_munit( _ppool, sizeof( char )*info.size+1, MEMORY_TYPE_DEFAULT );
			if( 0 == gdt_fread( (char*)ini_json_file, (char*)GDT_POINTER(_ppool,json_string_munit), GDT_PUNIT_USIZE(_ppool,json_string_munit) ) ){
				int32_t root_munit = gdt_json_decode( _ppool, (char*)GDT_POINTER(_ppool,json_string_munit) );
				GDT_NODE* rootnode = (GDT_NODE*)GDT_POINTER(_ppool,root_munit);
				GDT_NODE* workelemlist = ( GDT_NODE* )GDT_POINTER( _ppool, rootnode->element_munit );
				gdt_add_hash_value_kstring( _ppool, pscript->v_hash_munit, "_INI", workelemlist[0].element_munit, workelemlist[0].id );
			}
		}
		gdt_add_system_function( _ppool, script_munit, "echo", gdt_script_system_function_echo, 0 );
		gdt_add_system_function( _ppool, script_munit, "count", gdt_script_system_function_count, 0 );
		gdt_add_system_function( _ppool, script_munit, "file_exist", gdt_script_system_function_file_exist, 0 );
		gdt_add_system_function( _ppool, script_munit, "file_size", gdt_script_system_function_file_size, 0 );
		gdt_add_system_function( _ppool, script_munit, "file_extension", gdt_script_system_function_file_extension, 0 );
		gdt_add_system_function( _ppool, script_munit, "file_get", gdt_script_system_function_file_get, 0 );
		gdt_add_system_function( _ppool, script_munit, "file_put", gdt_script_system_function_file_put, 0 );
		gdt_add_system_function( _ppool, script_munit, "file_add", gdt_script_system_function_file_add, 0 );
		gdt_add_system_function( _ppool, script_munit, "json_encode", gdt_script_system_function_json_encode, 0 );
		gdt_add_system_function( _ppool, script_munit, "json_decode", gdt_script_system_function_json_decode, 0 );
		gdt_add_system_function( _ppool, script_munit, "gmtime", gdt_script_system_function_gmtime, 0 );
		gdt_import_script( _ppool, &script_munit, (char*)script_file );
	}while( false );
	return script_munit;
}

int32_t gdt_add_http_request( GDT_MEMORY_POOL* _ppool, int32_t script_munit, char* arg, int32_t header_munit, int32_t get_parameter_munit, int32_t post_parameter_munit )
{
	if( script_munit==-1){
		return GDT_SYSTEM_ERROR;
	}
	GDT_SCRIPT *pscript = (GDT_SCRIPT *)GDT_POINTER( _ppool, script_munit );
	if( header_munit > 0 ){
		gdt_add_hash_value_kstring( _ppool, pscript->v_hash_munit, "_HEADER", header_munit, ELEMENT_HASH );
	}
	if( get_parameter_munit > 0 ){
		gdt_add_hash_value_kstring( _ppool, pscript->v_hash_munit, "_GET", get_parameter_munit, ELEMENT_HASH );
	}
	if( post_parameter_munit > 0 ){
		gdt_add_hash_value_kstring( _ppool, pscript->v_hash_munit, "_POST", post_parameter_munit, ELEMENT_HASH );
	}
	if( arg != NULL ){
		gdt_add_hash_value( 
			_ppool
			, pscript->v_hash_munit
			, "arg"
			, arg
			, ELEMENT_LITERAL_STR
		);
	}
	return GDT_SYSTEM_OK;
}

int32_t gdt_script_run( GDT_MEMORY_POOL* _ppool, const char* script_file, const char* ini_json_file, char* arg, int32_t header_munit, int32_t get_parameter_munit, int32_t post_parameter_munit )
{
	int32_t script_munit = gdt_init_http_script( _ppool, script_file, ini_json_file );
	if( script_munit==-1){
		return -1;
	}
	if( GDT_SYSTEM_OK != gdt_add_http_request( _ppool, script_munit, arg, header_munit, get_parameter_munit, post_parameter_munit ) )
	{
		return -1;
	}
	gdt_exec( _ppool, &script_munit );
	return script_munit;
}
