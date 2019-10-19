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

#include "gdt_token_analyzer.h"

/*
 * allocate memory of GDT_TOKENS and initialize parameters
 */
int32_t gdt_inittoken( GDT_MEMORY_POOL* _ppool, int32_t allocsize )
{
	int32_t tokens_munit = -1;
	if( -1 == ( tokens_munit = gdt_create_munit( _ppool, sizeof( GDT_TOKENS ), MEMORY_TYPE_DEFAULT ) ) ){
		return -1;
	}
	GDT_TOKENS *ptokens = (GDT_TOKENS*)GDT_POINTER(_ppool,tokens_munit);
	ptokens->token_munit = -1;
	ptokens->size = 0;
	ptokens->currentpos = 0;
	ptokens->workpos = 0;
	ptokens->allocsize = allocsize;
	return tokens_munit;
}

/*
 * analyze input strings
 */
int gdt_token_analyzer( GDT_MEMORY_POOL* _ppool, int32_t tokens_munit, char* pstr )
{
	int32_t tmpbuf_munit = -1;
	char* tokenbuf = NULL;
	char* ptoken = NULL;
	int tokensize = 0;
	int token_type = ID_UNK;
	int ret = 0;
	if( -1 == ( tmpbuf_munit = gdt_create_munit( _ppool, sizeof( char ) * STRBUF_SIZE, MEMORY_TYPE_DEFAULT ) ) ){
#ifdef __GDT_DEBUG__
		printf("can not allocate memory of tokenbuf\n");
#endif
		return GDT_SYSTEM_ERROR;
	}
	tokenbuf = (char*)gdt_upointer( _ppool, tmpbuf_munit );
	ptoken = tokenbuf;
	for(;;)
	{
		if( *pstr == '\0' )
		{
			if( tokensize > 0 ){
				if( 0 == gdt_addtoken(_ppool,tokens_munit,tokenbuf,&tokensize,token_type) ){
					printf( "add token error.\n" );
					break;
				}
			}
			break;
		}
		if( tokensize >= STRBUF_SIZE-8){
			printf( "token buffer size over\n" );
			if( 0 == gdt_addtoken(_ppool,tokens_munit,tokenbuf,&tokensize,token_type) ){
				printf( "add token error.\n" );
				break;
			}
		}
		// "//"
		if( *pstr == '/' && *(pstr+1) == '/')
		{
			while( *pstr != '\0' && *pstr != '\n' )
				++pstr;
			continue;
		}// "/* */"
		if( *pstr == '/' && *(pstr+1) == '*')
		{
			pstr+=2;
			while( *pstr != '*' || *(pstr+1) != '/' )
			{
				if( *pstr == '\0' )
				{
					printf("syntax error : comment\n");
					break;
				}
				++pstr;
			}
			pstr+=2;
			continue;
		}
//		if( *pstr == '\n' )
//		{
//			if( tokensize > 0 ){
//				if( 0 == gdt_addtoken(_ppool,tokens_munit,tokenbuf,&tokensize,token_type) ){
//					printf("add token error.\n");
//					break;
//				}
//				pstr++;
//				continue;
//			}
//			pstr++;
//		}
//		else if( !isascii(*pstr) )
//		{
//			printf("invalid char : %c\n", (*pstr) );
//			break;
//		}
		if( ( tokensize == 0 && isdigit( *pstr ) ) || ( tokensize > 0 && token_type == ID_NUM ) )
		{
			int dot = 0;
			token_type = ID_NUM;
			while( isdigit(*pstr) || ( tokensize > 0 && *pstr == '.' && dot == 0 )  )
			{
				if( *pstr == '.' )
				{
					token_type = ID_FLOAT;
					dot = 1;
				}
				*(ptoken+(tokensize++)) = *(pstr++);
			}
			if( 0 == gdt_addtoken(_ppool,tokens_munit,tokenbuf,&tokensize,token_type) )
			{
				printf("add token error.\n");
				break;
			}
			continue;
		}
		else if( *pstr == '\"')
		{
			token_type = ID_STR;
			++pstr;
			while(*pstr != '\"')
			{
				//if( *pstr == '\0' || *pstr == '\n' || *pstr == '\r' )
				if( *pstr == '\0' )
				{
					printf("syntax error : \"\n");
					return 1;
				}
				if( *pstr == '\\' )
				{
					++pstr;
					if( *pstr == 'n' ){
						*(ptoken+(tokensize++)) = '\n';
						++pstr;
					}else if( *pstr == 'r' ){
						*(ptoken+(tokensize++)) = '\r';
						++pstr;
					}else{
						*(ptoken+(tokensize++)) = *(pstr++);
					}
				}
				// multi byte
				//else if( ( *pstr >= (char)(0x81) && *pstr <= (char)(0x9f) ) || ( *pstr >= (char)(0xe0) && *pstr <= (char)(0xfc) ) )
				//{
				//	int i;
				//	for( i=0; i<2; i++ ){
				//		if( *pstr == '\"' ){
				//			printf( "%d %d\n",(uint8_t)(*(pstr-2)), (uint8_t)(*(pstr-1)) );
				//			break;
				//		}
				//		*(ptoken+(tokensize++)) = *(pstr++);
				//	}
				//}
				else {
					*(ptoken+(tokensize++)) = *(pstr++);
				}
				if( tokensize >= STRBUF_SIZE-8){
					printf( "string buffer size over\n" );
					if( 0 == gdt_addtoken(_ppool,tokens_munit,tokenbuf,&tokensize,token_type) )
					{
						printf( "add token error.\n" );
						break;
					}
				}
			}
			if( 0 == gdt_addtoken(_ppool,tokens_munit,tokenbuf,&tokensize,token_type) )
			{
				printf("add token error.\n");
				break;
			}
			pstr++;
			continue;
		}
		else if( *pstr == '\''){
			token_type = ID_STR;
			++pstr;
			while(*pstr != '\''){
				//if( *pstr == '\0' || *pstr == '\n' || *pstr == '\r' )
				if( *pstr == '\0' ){
					printf("syntax error : \'\n");
					return 1;
				}
//				// multi byte
//				if( ( *pstr >= (char)(0x81) && *pstr <= (char)(0x9f) ) || ( *pstr >= (char)(0xe0) && *pstr <= (char)(0xfc) ) ){
//					int i;
//					for( i=0; i<2; i++ ){
//						if( *pstr == '\'' ){
//							//printf( "%d %d\n",(uint8_t)(*(pstr-2)), (uint8_t)(*(pstr-1)) );
//							break;
//						}
//						*(ptoken+(tokensize++)) = *(pstr++);
//					}
//				}
//				else {
					*(ptoken+(tokensize++)) = *(pstr++);
//				}
				if( tokensize >= STRBUF_SIZE-8){
					printf( "string buffer size over\n" );
					if( 0 == gdt_addtoken(_ppool,tokens_munit,tokenbuf,&tokensize,token_type) )
					{
						printf( "add token error.\n" );
						break;
					}
				}
			}
			if( 0 == gdt_addtoken(_ppool,tokens_munit,tokenbuf,&tokensize,token_type) )
			{
				printf("add token error.\n");
				break;
			}
			pstr++;
			continue;
		}
		else if( *pstr == '\n' || isspace(*pstr)){
			if( tokensize > 0 ){
				if( 0 == gdt_addtoken(_ppool,tokens_munit,tokenbuf,&tokensize,token_type) ){
					printf("add token error.\n");
					break;
				}
				pstr++;
			}
			else{++pstr;}
		}
		else {
			if( tokensize == 0 ){
				if( isalpha(*pstr) || *pstr == '_'){
					token_type = ID_SYMBOL;
				}
				else if( *pstr == '+' || *pstr == '-' || *pstr == '=' || *pstr == '<' || *pstr == '>' || *pstr == '|' || *pstr == '&' )
				{
					char c = ' ';
					token_type = ID_OP;
					c = *pstr;
					*(ptoken+(tokensize++)) = *(pstr++);
					if( *pstr == '=' ){
						*(ptoken+(tokensize++)) = *(pstr++);
					}
					else if( *pstr == c ){
						*(ptoken+(tokensize++)) = *(pstr++);
						if( *pstr == '=' && ( c == '<' || c == '>' ) )
						{
							*(ptoken+(tokensize++)) = *(pstr++);
						}
					}
					else if( c == '-' && *pstr == '>' ){
						*(ptoken+(tokensize++)) = *(pstr++);
					}
					else if( c == '=' && *pstr == '>' ){
						*(ptoken+(tokensize++)) = *(pstr++);
					}
					if( 0 == gdt_addtoken(_ppool,tokens_munit,tokenbuf,&tokensize,token_type) )
					{
						printf("add token error.\n");
						break;
					}
					continue;
				}
				else if( *pstr == '*' || *pstr == '/' || *pstr == '%' || *pstr == '!' || *pstr == '^' ){
					token_type = ID_OP;
					*(ptoken+(tokensize++)) = *(pstr++);
					if( *pstr == '=' ){
						*(ptoken+(tokensize++)) = *(pstr++);
					}
					if( 0 == gdt_addtoken(_ppool,tokens_munit,tokenbuf,&tokensize,token_type) )
					{
						printf("add token error.\n");
						break;
					}
					continue;
				}
				else if( *pstr == ':' ){
					token_type = ID_SIGN;
					*(ptoken+(tokensize++)) = *(pstr++);
					if( *pstr == ':' ){
						*(ptoken+(tokensize++)) = *(pstr++);
					}
					if( 0 == gdt_addtoken(_ppool,tokens_munit,tokenbuf,&tokensize,token_type) )
					{
						printf("add token error.\n");
						break;
					}
					continue;
				}
				else{
					token_type = ID_SIGN;
					*(ptoken+(tokensize++)) = *(pstr++);
					if( 0 == gdt_addtoken(_ppool,tokens_munit,tokenbuf,&tokensize,token_type) )
					{
						printf("add token error.\n");
						break;
					}
					continue;
				}
			}
			else{
				if( isalpha(*pstr) || *pstr == '_'){
					
				}
				else if( *pstr =='(' || *pstr == '{' || *pstr == '[' ){
					if( 0 == gdt_addtoken(_ppool,tokens_munit,tokenbuf,&tokensize,token_type) ){
						printf("add token error.\n");
						break;
					}
					token_type = ID_SIGN;
					*(ptoken+(tokensize++)) = *(pstr++);
					if( 0 == gdt_addtoken(_ppool,tokens_munit,tokenbuf,&tokensize,token_type) )
					{
						printf("add token error.\n");
						break;
					}
					continue;
				}
				else if( *pstr ==';' || *pstr == ')' || *pstr == '}' || *pstr == ']' || *pstr == ',' ){
					if( 0 == gdt_addtoken(_ppool,tokens_munit,tokenbuf,&tokensize,token_type) ){
						printf("add token error.\n");
						break;
					}
					token_type = ID_SIGN;
					*(ptoken+(tokensize++)) = *(pstr++);
					if( 0 == gdt_addtoken(_ppool,tokens_munit,tokenbuf,&tokensize,token_type) )
					{
						printf("add token error.\n");
						break;
					}
					continue;
				}
				else if( *pstr == '+' || *pstr == '-' || *pstr == '=' || *pstr == '<' || *pstr == '>' || *pstr == '|' || *pstr == '&' ){
					char c = ' ';
					if( 0 == gdt_addtoken(_ppool,tokens_munit,tokenbuf,&tokensize,token_type) ){
						printf("add token error.\n");
						break;
					}
					token_type = ID_OP;
					c = *pstr;
					*(ptoken+(tokensize++)) = *(pstr++);
					if( *pstr == '=' && c != '|' && c != '&' ){
						*(ptoken+(tokensize++)) = *(pstr++);
					}
					else if( c == *pstr ){
						*(ptoken+(tokensize++)) = *(pstr++);
					}
					else if( c == '-' && *pstr == '>' ){
						*(ptoken+(tokensize++)) = *(pstr++);
					}
					if( 0 == gdt_addtoken(_ppool,tokens_munit,tokenbuf,&tokensize,token_type) )
					{
						printf("add token error.\n");
						break;
					}
					continue;
				}
				else if( *pstr == '*' || *pstr == '/' || *pstr == '%' || *pstr == '!' || *pstr == '^' )
				{
					if( 0 == gdt_addtoken(_ppool,tokens_munit,tokenbuf,&tokensize,token_type) )
					{
						printf("add token error.\n");
						break;
					}
					token_type = ID_OP;
					*(ptoken+(tokensize++)) = *(pstr++);
					if( *pstr == '=' ){
						*(ptoken+(tokensize++)) = *(pstr++);
					}
					if( 0 == gdt_addtoken(_ppool,tokens_munit,tokenbuf,&tokensize,token_type) )
					{
						printf("add token error.\n");
						break;
					}
					continue;
				}
				else if( *pstr == ':' ){
					if( 0 == gdt_addtoken(_ppool,tokens_munit,tokenbuf,&tokensize,token_type) ){
						printf("add token error.\n");
						break;
					}
					token_type = ID_SIGN;
					*(ptoken+(tokensize++)) = *(pstr++);
					if( *pstr == ':' ){
						*(ptoken+(tokensize++)) = *(pstr++);
					}
					if( 0 == gdt_addtoken(_ppool,tokens_munit,tokenbuf,&tokensize,token_type) ){
						printf("add token error.\n");
						break;
					}
					continue;
				}
			}
			*(ptoken+(tokensize++)) = *(pstr++);
		}
	}
	return ret;
}

int gdt_addtoken( GDT_MEMORY_POOL* _ppool, int32_t tokens_munit, char* tokenbuf, int* tokensize, int type )
{
	char *pbuf = NULL;
	int tmptype = 0;
	int32_t tmpmunit;
	GDT_TOKENS *ptokens = (GDT_TOKENS*)GDT_POINTER( _ppool, tokens_munit );
	GDT_TOKEN *ptoken = NULL;
	if( ptokens->token_munit == -1 )
	{
		if( -1 == ( ptokens->token_munit = gdt_create_munit( _ppool, sizeof( GDT_TOKEN ) * ptokens->allocsize, MEMORY_TYPE_DEFAULT ) ) ){
			return -1;
		}
		ptokens->size = ptokens->allocsize;
	}
	else{
		if( ptokens->currentpos >= ptokens->size )
		{
			int32_t resize = ptokens->size * 2;
			if( -1 == ( tmpmunit = gdt_create_munit( _ppool, sizeof( GDT_TOKEN ) * ( resize ), MEMORY_TYPE_DEFAULT ) ) ){
				printf("realloc token memory error\n");
				return -1;
			}
			memcpy( 
				  (GDT_TOKEN*)GDT_POINTER(_ppool,tmpmunit)
				, (GDT_TOKEN*)GDT_POINTER(_ppool,ptokens->token_munit)
				, sizeof( GDT_TOKEN )*ptokens->size
			);
			gdt_free_memory_unit( _ppool, &ptokens->token_munit );
			ptokens->token_munit = tmpmunit;
			ptokens->size = resize;
		}
	}
	ptoken = ( GDT_TOKEN* )GDT_POINTER(_ppool,ptokens->token_munit)+ptokens->currentpos;
	*(tokenbuf+((*tokensize)++)) = '\0';
	if( type == ID_NUM ){
		if( -1 == ( ptoken->buf_munit = gdt_create_munit( _ppool, sizeof(char)*(*tokensize) + sizeof( int32_t ), MEMORY_TYPE_DEFAULT ) ) ){
			printf("allocate token buf munit error.\n");
			return -1;
		}
		pbuf = (char*)GDT_POINTER(_ppool,ptoken->buf_munit);
		ptoken->size = (*tokensize);
		memcpy(pbuf,tokenbuf,(*tokensize));
		(*(int32_t*)(GDT_POINTER(_ppool,ptoken->buf_munit)+gdt_usize(_ppool,ptoken->buf_munit)-sizeof(int32_t))) = atoi(tokenbuf);
	}
	else{
		if( -1 == ( ptoken->buf_munit = gdt_create_munit( _ppool, sizeof(char)*(*tokensize), MEMORY_TYPE_DEFAULT ) ) ){
			printf("allocate token buf munit error.\n");
			return -1;
		}
		pbuf = (char*)GDT_POINTER(_ppool,ptoken->buf_munit);
		ptoken->size = (*tokensize);
		memcpy(pbuf,tokenbuf,(*tokensize));
	}
	if( type == ID_SYMBOL ){
		tmptype = gdt_check_systemword( pbuf );
	}
	if( tmptype != 0 ){
		ptoken->type = tmptype;
	}
	else{
		ptoken->type = type;
	}
	ptokens->currentpos++;
	*tokenbuf = '\0';
	*tokensize = 0;
	return 1;
}

int gdt_check_systemword( char* token )
{
	int is_sys = 0;
	if( !strcmp(token, SYS_IF ) ){
		is_sys = ID_SYS_IF;
	}
	else if( !strcmp(token, SYS_ELSE ) ){
		is_sys = ID_SYS_ELSE;
	}
	else if( !strcmp(token, SYS_ELSEIF ) ){
		is_sys = ID_SYS_ELSEIF;
	}
	else if( !strcmp(token, SYS_WHILE ) ){
		is_sys = ID_SYS_WHILE;
	}
	else if( !strcmp(token, SYS_LOOP ) ){
		is_sys = ID_SYS_LOOP;
	}
	return is_sys;
}

void gdt_tokendump( GDT_MEMORY_POOL* _ppool, int32_t tokens_munit )
{
	int i;
	int type;
	char* pbuf;
	char typelist[][16]={
		"unknown"
		,"numeric"
		,"calc_op"
		,"sign"
		,"string"
		,"symbol"
		,"if"
		,"else"
		,"elseif"
		,"float"
		,"loop"
		,"while"
	};
	GDT_TOKENS *ptokens = (GDT_TOKENS*)GDT_POINTER( _ppool, tokens_munit );
	GDT_TOKEN *token_list;
	printf("-----------------------------------------------------------------------\n");
	printf("- token info\n");
	printf("-----------------------------------------------------------------------\n");
	token_list = ( GDT_TOKEN* )gdt_upointer( _ppool, ptokens->token_munit );
	for( i = 0; i < ptokens->currentpos; i++ )
	{
		type = token_list[i].type;
		pbuf = (char*)gdt_upointer( _ppool, token_list[i].buf_munit );
		printf("token(%s , %d byte) : %s\n",typelist[type],token_list[i].size,pbuf);
	}
	printf("-----------------------------------------------------------------------\n");
}

