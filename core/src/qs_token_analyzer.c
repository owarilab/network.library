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

#include "qs_token_analyzer.h"

/*
 * allocate memory of QS_TOKENS and initialize parameters
 */
int32_t qs_inittoken( QS_MEMORY_POOL* _ppool, int32_t allocsize, size_t read_buffer_size )
{
	int32_t tokens_munit = -1;
	if( -1 == ( tokens_munit = qs_create_memory_block( _ppool, sizeof( QS_TOKENS ) ) ) ){
		return -1;
	}
	QS_TOKENS *ptokens = (QS_TOKENS*)QS_GET_POINTER(_ppool,tokens_munit);
	ptokens->token_munit = -1;
	if(read_buffer_size<QS_TOKEN_READ_BUFFER_SIZE_MIN){
		read_buffer_size = QS_TOKEN_READ_BUFFER_SIZE_MIN;
	}
	ptokens->read_buffer_size = read_buffer_size;
	ptokens->size = 0;
	ptokens->currentpos = 0;
	ptokens->workpos = 0;
	ptokens->allocsize = allocsize;
	ptokens->enable_newline = 0;
	return tokens_munit;
}

int32_t qs_resize_token_buffer(QS_MEMORY_POOL* _ppool, int32_t tokens_munit, int32_t memid_currend_buffer, int tokensize)
{
	QS_TOKENS *ptokens = (QS_TOKENS*)QS_GET_POINTER(_ppool,tokens_munit);
	if(memid_currend_buffer==-1){
		return qs_create_memory_block( _ppool, sizeof( char ) * ptokens->read_buffer_size );
	}
#ifdef __QS_DEBUG__
	printf( "resize token buffer, %zd bytes -> %zd bytes\n" , ptokens->read_buffer_size, ptokens->read_buffer_size*2 );
#endif
	ptokens->read_buffer_size = ptokens->read_buffer_size*2;
	int32_t memid_new_buffer = qs_create_memory_block( _ppool, sizeof( char ) * ptokens->read_buffer_size );
	if(-1==memid_new_buffer){
		return -1;
	}
	char *tokenbuf = (char*)qs_upointer( _ppool, memid_currend_buffer );
	char *new_tokenbuf = (char*)qs_upointer( _ppool, memid_new_buffer );
	memcpy(new_tokenbuf,tokenbuf,tokensize);
	return memid_new_buffer;
}

/*
 * analyze input strings
 */
int qs_token_analyzer( QS_MEMORY_POOL* _ppool, int32_t tokens_munit, char* pstr )
{
	int32_t tmpbuf_munit = -1;
	char* tokenbuf = NULL;
	char* ptoken = NULL;
	int tokensize = 0;
	int token_type = ID_UNK;
	int ret = 0;
	QS_TOKENS *ptokens = (QS_TOKENS*)QS_GET_POINTER(_ppool,tokens_munit);
	if( -1 == ( tmpbuf_munit = qs_create_memory_block( _ppool, sizeof( char ) * ptokens->read_buffer_size ) ) ){
#ifdef __QS_DEBUG__
		printf("can not allocate memory of tokenbuf\n");
#endif
		return QS_SYSTEM_ERROR;
	}
	tokenbuf = (char*)qs_upointer( _ppool, tmpbuf_munit );
	ptoken = tokenbuf;
	for(;;)
	{
		if( *pstr == '\0' )
		{
			if( tokensize > 0 ){
				if( 0 == qs_addtoken(_ppool,ptokens,tokenbuf,&tokensize,token_type) ){
					printf( "add token error.\n" );
					break;
				}
			}
			break;
		}
		// buffer size over
		if( tokensize >= ptokens->read_buffer_size-8){
			int32_t memid_new_buffer;
			if(-1==(memid_new_buffer=qs_resize_token_buffer(_ppool,tokens_munit,tmpbuf_munit,tokensize))){
#ifdef __QS_DEBUG__
				printf( "token buffer size over\n" );
#endif
				return QS_SYSTEM_ERROR;
			}
			char *new_tokenbuf = (char*)qs_upointer( _ppool, memid_new_buffer );
			tokenbuf = new_tokenbuf;
			ptoken = tokenbuf;
			qs_free_memory_block(_ppool,&tmpbuf_munit);
			tmpbuf_munit = memid_new_buffer;
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
					//printf("syntax error : comment\n");
					break;
				}
				++pstr;
			}
			pstr+=2;
			continue;
		}
		if (*pstr < 0) // ascii = 0 ~ 127
		{
			pstr++;
			continue;
		}
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
			if( 0 == qs_addtoken(_ppool,ptokens,tokenbuf,&tokensize,token_type) )
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
				if( *pstr == '\0' )
				{
					//printf("syntax error : \"\n");
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
				else {
					*(ptoken+(tokensize++)) = *(pstr++);
				}
				if( tokensize >= ptokens->read_buffer_size-8){
					int32_t memid_new_buffer;
					if(-1==(memid_new_buffer=qs_resize_token_buffer(_ppool,tokens_munit,tmpbuf_munit,tokensize))){
						return QS_SYSTEM_ERROR;
					}
					char *new_tokenbuf = (char*)qs_upointer( _ppool, memid_new_buffer );
					tokenbuf = new_tokenbuf;
					ptoken = tokenbuf;
					qs_free_memory_block(_ppool,&tmpbuf_munit);
					tmpbuf_munit = memid_new_buffer;
				}
			}
			if( 0 == qs_addtoken(_ppool,ptokens,tokenbuf,&tokensize,token_type) )
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
				if( *pstr == '\0' ){
					//printf("syntax error : \'\n");
					return 1;
				}
				*(ptoken+(tokensize++)) = *(pstr++);
				if( tokensize >= ptokens->read_buffer_size-8){
					int32_t memid_new_buffer;
					if(-1==(memid_new_buffer=qs_resize_token_buffer(_ppool,tokens_munit,tmpbuf_munit,tokensize))){
						return QS_SYSTEM_ERROR;
					}
					char *new_tokenbuf = (char*)qs_upointer( _ppool, memid_new_buffer );
					tokenbuf = new_tokenbuf;
					ptoken = tokenbuf;
					qs_free_memory_block(_ppool,&tmpbuf_munit);
					tmpbuf_munit = memid_new_buffer;
				}
			}
			if( 0 == qs_addtoken(_ppool,ptokens,tokenbuf,&tokensize,token_type) )
			{
				printf("add token error.\n");
				break;
			}
			pstr++;
			continue;
		}
		else if( *pstr == '\n' || isspace(*pstr)){
			if( tokensize > 0 ){
				if( 0 == qs_addtoken(_ppool,ptokens,tokenbuf,&tokensize,token_type) ){
					printf("add token error.\n");
					break;
				}
			}
			if(ptokens->enable_newline==1)
			{
				if( *pstr == '\n' ){
					tokenbuf[tokensize++] = '\n';
					if( 0 == qs_addtoken(_ppool,ptokens,tokenbuf,&tokensize,ID_SYS_NEWLINE) ){
						printf("add token error.\n");
						break;
					}
				}
			}
			++pstr;
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
					if( 0 == qs_addtoken(_ppool,ptokens,tokenbuf,&tokensize,token_type) )
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
					if( 0 == qs_addtoken(_ppool,ptokens,tokenbuf,&tokensize,token_type) )
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
					if( 0 == qs_addtoken(_ppool,ptokens,tokenbuf,&tokensize,token_type) )
					{
						printf("add token error.\n");
						break;
					}
					continue;
				}
				else{
					token_type = ID_SIGN;
					*(ptoken+(tokensize++)) = *(pstr++);
					if( 0 == qs_addtoken(_ppool,ptokens,tokenbuf,&tokensize,token_type) )
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
					if( 0 == qs_addtoken(_ppool,ptokens,tokenbuf,&tokensize,token_type) ){
						printf("add token error.\n");
						break;
					}
					token_type = ID_SIGN;
					*(ptoken+(tokensize++)) = *(pstr++);
					if( 0 == qs_addtoken(_ppool,ptokens,tokenbuf,&tokensize,token_type) )
					{
						printf("add token error.\n");
						break;
					}
					continue;
				}
				else if( *pstr ==';' || *pstr == ')' || *pstr == '}' || *pstr == ']' || *pstr == ',' ){
					if( 0 == qs_addtoken(_ppool,ptokens,tokenbuf,&tokensize,token_type) ){
						printf("add token error.\n");
						break;
					}
					token_type = ID_SIGN;
					*(ptoken+(tokensize++)) = *(pstr++);
					if( 0 == qs_addtoken(_ppool,ptokens,tokenbuf,&tokensize,token_type) )
					{
						printf("add token error.\n");
						break;
					}
					continue;
				}
				else if( *pstr == '+' || *pstr == '-' || *pstr == '=' || *pstr == '<' || *pstr == '>' || *pstr == '|' || *pstr == '&' ){
					char c = ' ';
					if( 0 == qs_addtoken(_ppool,ptokens,tokenbuf,&tokensize,token_type) ){
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
					if( 0 == qs_addtoken(_ppool,ptokens,tokenbuf,&tokensize,token_type) )
					{
						printf("add token error.\n");
						break;
					}
					continue;
				}
				else if( *pstr == '*' || *pstr == '/' || *pstr == '%' || *pstr == '!' || *pstr == '^' )
				{
					if( 0 == qs_addtoken(_ppool,ptokens,tokenbuf,&tokensize,token_type) )
					{
						printf("add token error.\n");
						break;
					}
					token_type = ID_OP;
					*(ptoken+(tokensize++)) = *(pstr++);
					if( *pstr == '=' ){
						*(ptoken+(tokensize++)) = *(pstr++);
					}
					if( 0 == qs_addtoken(_ppool,ptokens,tokenbuf,&tokensize,token_type) )
					{
						printf("add token error.\n");
						break;
					}
					continue;
				}
				else if( *pstr == ':' ){
					if( 0 == qs_addtoken(_ppool,ptokens,tokenbuf,&tokensize,token_type) ){
						printf("add token error.\n");
						break;
					}
					token_type = ID_SIGN;
					*(ptoken+(tokensize++)) = *(pstr++);
					if( *pstr == ':' ){
						*(ptoken+(tokensize++)) = *(pstr++);
					}
					if( 0 == qs_addtoken(_ppool,ptokens,tokenbuf,&tokensize,token_type) ){
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

int qs_addtoken( QS_MEMORY_POOL* _ppool, QS_TOKENS *ptokens, char* tokenbuf, int* tokensize, int type )
{
	char *pbuf = NULL;
	int tmptype = 0;
	int32_t tmpmunit;
	QS_TOKEN *ptoken = NULL;
	if( ptokens->token_munit == -1 )
	{
		if( -1 == ( ptokens->token_munit = qs_create_memory_block( _ppool, sizeof( QS_TOKEN ) * ptokens->allocsize ) ) ){
			return -1;
		}
		ptokens->size = ptokens->allocsize;
	}
	else{
		if( ptokens->currentpos >= ptokens->size )
		{
			int32_t resize = ptokens->size * QS_TOKEN_RESIZE_QUANTITY;
			if( -1 == ( tmpmunit = qs_create_memory_block( _ppool, sizeof( QS_TOKEN ) * ( resize ) ) ) ){
				printf("realloc token memory error\n");
				return -1;
			}
			memcpy( 
				  (QS_TOKEN*)QS_GET_POINTER(_ppool,tmpmunit)
				, (QS_TOKEN*)QS_GET_POINTER(_ppool,ptokens->token_munit)
				, sizeof( QS_TOKEN )*ptokens->size
			);
			qs_free_memory_block( _ppool, &ptokens->token_munit );
			ptokens->token_munit = tmpmunit;
			ptokens->size = resize;
		}
	}
	ptoken = ( QS_TOKEN* )QS_GET_POINTER(_ppool,ptokens->token_munit)+ptokens->currentpos;
	*(tokenbuf+((*tokensize)++)) = '\0';
	if( type == ID_NUM ){

		char* endptr;
		uint64_t num = strtoul(tokenbuf,&endptr,10);
		if( *endptr == '\0' ){
			if( num > INT32_MAX ){
				//printf("token : %s(%lu) is uint64\n",tokenbuf,num);
				type = ID_NUM_U64;
			}
			else{
				//printf("token : %s(%lu) is uint32\n",tokenbuf,num);
			}
		}
		else{
			printf("token : %s is not numeric\n",tokenbuf);
			return -1;
		}

		// TODO : fix 64bit
		if(sizeof( NUMERIC_CAST ) == sizeof( int32_t ) && num > INT32_MAX){
			//printf("is u64 cast\n");
			type = ID_NUM_U64;
			if( -1 == ( ptoken->buf_munit = qs_create_memory_block( _ppool, sizeof(char)*(*tokensize) + sizeof( UI64_CAST ) ) ) ){
				printf("allocate token buf munit error.\n");
				return -1;
			}
			pbuf = (char*)QS_GET_POINTER(_ppool,ptoken->buf_munit);
			ptoken->size = (*tokensize);
			memcpy(pbuf,tokenbuf,(*tokensize));
			UI64_CAST* pv = QS_PUINT64(_ppool,ptoken->buf_munit);
			*pv = (UI64_CAST)num;
		}else{
			if( -1 == ( ptoken->buf_munit = qs_create_memory_block( _ppool, sizeof(char)*(*tokensize) + sizeof( NUMERIC_CAST ) ) ) ){
				printf("allocate token buf munit error.\n");
				return -1;
			}
			pbuf = (char*)QS_GET_POINTER(_ppool,ptoken->buf_munit);
			ptoken->size = (*tokensize);
			memcpy(pbuf,tokenbuf,(*tokensize));
			NUMERIC_CAST* pv = QS_PNUMERIC(_ppool,ptoken->buf_munit);
			*pv = (NUMERIC_CAST)num;
		}
	}
	else{
		if( -1 == ( ptoken->buf_munit = qs_create_memory_block( _ppool, sizeof(char)*(*tokensize) ) ) ){
			printf("allocate token buf munit error.\n");
			return -1;
		}
		pbuf = (char*)QS_GET_POINTER(_ppool,ptoken->buf_munit);
		ptoken->size = (*tokensize);
		memcpy(pbuf,tokenbuf,(*tokensize));
	}
	if( type == ID_SYMBOL ){
		tmptype = qs_check_systemword( pbuf );
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

int qs_check_systemword( char* token )
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

void qs_tokendump( QS_MEMORY_POOL* _ppool, int32_t tokens_munit )
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
		,"newline"
	};
	QS_TOKENS *ptokens = (QS_TOKENS*)QS_GET_POINTER( _ppool, tokens_munit );
	QS_TOKEN *token_list;
	printf("-----------------------------------------------------------------------\n");
	printf("- token info\n");
	printf("-----------------------------------------------------------------------\n");
	token_list = ( QS_TOKEN* )qs_upointer( _ppool, ptokens->token_munit );
	for( i = 0; i < ptokens->currentpos; i++ )
	{
		type = token_list[i].type;
		pbuf = (char*)qs_upointer( _ppool, token_list[i].buf_munit );
		printf("token(%s , %d byte) : %s\n",typelist[type],token_list[i].size,pbuf);
	}
	printf("-----------------------------------------------------------------------\n");
}

