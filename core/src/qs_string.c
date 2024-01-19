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

#include "qs_string.h"

int qs_ltoa( int64_t value, char* target, size_t size )
{
	int error_code = 0;
	int isminus = false;
	int i,index,len;
	char c;
	char* tmpp = target;
	if( value < 0 ){
		isminus = true;
		value = -value;
		*tmpp++ = '-';
	}
	do{
		*tmpp++ = ( ( value % 10 ) + 0x30 );
		if( value <= 9 ){
			break;
		}
		if( ( tmpp - target ) >= size-2 ){
			error_code = -1;
			break;
		}
		value = value / 10;
	}while( true );
	*tmpp++ = '\0';
	i = isminus;
	index = 0;
	len = ( tmpp - target ) - 1;
	// reverse
	if( len > 1 ){
		do{
			c = *(target+index+i);
			*(target+index+i) = *(target+(len-1)-index);
			*(target+(len-1)-index) = c;
			index++;
		}while( index < len/2 );
	}
	return error_code;
}

int qs_itoa( int32_t value, char* target, size_t size )
{
	int64_t v64 = value;
	return qs_ltoa( v64, target, size );
}

int32_t qs_find_char( char* target, size_t target_size, char delimiter_ch )
{
	char* ps = target;
	while( *ps != '\0' ){
		if( *ps == delimiter_ch ){
			return ps - target;
		}
		++ps;
	}
	return -1;
}

char* qs_readline( char* buf, size_t buffer_size, char* target, char delimiter_ch )
{
	char *bufstart = buf;
	if( buf == NULL || target == NULL ){
		return target;
	}
	while( (*target) != '\0' )
	{
		if( buf - bufstart >= buffer_size -1 ){
			printf("qs_readline : buffer size over\n");
			break;
		}
		if( (*target) == delimiter_ch ){
			target++;
			break;
		}
		*(buf++) = *(target++);
	}
	(*buf) = '\0';
	if( (*target) == '\r' ){ target++; }
	if( (*target) == '\n' ){ target++; }
	return target;
}

char* qs_read_line_delimiter( char* buf, size_t buffer_size,char* target, char delimiter_ch )
{
	return qs_read_line_delimiter_core(buf,buffer_size,target,delimiter_ch,0);
}
char* qs_read_line_delimiter_core( char* buf, size_t buffer_size,char* target, char delimiter_ch, char skip_ch )
{
	char *bufstart = buf;
	if( buf == NULL || target == NULL ){
		return target;
	}
	while( (*target) != '\r' && (*target) != '\n' && (*target) != '\0' )
	{
		if( buf - bufstart >= buffer_size -1 ){
			printf("qs_read_line_delimiter : buffer size over\n");
			break;
		}
		if(skip_ch!=0 && (*target) == skip_ch)
		{
			target++;
			continue;
		}
		if( (*target) == delimiter_ch )
		{
			target++;
			break;
		}
		*(buf++) = *(target++);
	}
	(*buf) = '\0';
	if( (*target) == '\r' ){ target++; }
	if( (*target) == '\n' ){ target++; }
	return target;
}

char* qs_read_delimiter( char* buf, size_t buffer_size,char* target, char delimiter_ch )
{
	char *bufstart = buf;
	if( buf == NULL || target == NULL ){
		return target;
	}
	while( (*target) != '\0' ){
		if( buf - bufstart >= buffer_size -1 ){
			printf("qs_read_delimiter : buffer size over\n");
			break;
		}
		if( (*target) == delimiter_ch ){
			target++;
			break;
		}
		*(buf++) = *(target++);
	}
	(*buf) = '\0';
	return target;
}

size_t qs_strlcat( char *dst, const char *src, size_t size )
{
	size_t s = 0;
	const char *ps;
	char *pd, *pde;
	size_t dlen ,lest;
	do{
		if( dst == NULL || src == NULL ){
			break;
		}
		for( pd = dst, lest = size; *pd != '\0' && lest != 0; pd++, lest-- );
		dlen = pd - dst;
		if( size - dlen == 0 ){
			s = (dlen + strlen( src ) );
			break;
		}
		pde = dst + size -1;
		for( ps = src; *ps != '\0' && pd < pde; pd++, ps++ ){
			*pd = *ps;
		}
		for( ; pd < pde; pd++ ){
			*pd = '\0';
		}
		while( *ps++ );
		s = ( dlen + (ps - src -1 ) );
	}while( false );
	return s;
}

size_t qs_strlink( char *pmain, size_t mainsize, char *psub, size_t subsize, size_t max_size )
{
	size_t s = 0;
	char *pm = pmain;
	char *ps = psub;
	pm += mainsize;
	if( *ps == '\0' ){
		return s;
	}
	if( mainsize + subsize >= max_size-1 ){
		printf("qs_strlink : buffer size over\n");
		return s;
	}
	do{
		*(pm++) = *(ps++);
	}while( ps < ( psub + subsize ) && *ps != '\0' );
	s = mainsize + ( ps - psub );
	pmain[s] = '\0';
	return s;
}

int qs_escape_directory_traversal( char* dest, const char *src, size_t size )
{
	int error_code = QS_SYSTEM_OK;
	const char *ps;
	char *dstart;
	do{
		if( dest == NULL || src == NULL ){
			error_code = QS_SYSTEM_ERROR;
			break;
		}
		for( ps = src , dstart = dest; *ps != '\0' && ( dest - dstart ) < (size-1); ps++ )
		{
			if( *ps == '.' && *(ps+1) == '.' ){
				if( *(ps+2) == '/' ){
					ps+=2;
				}
				else{
					ps++;
				}
				error_code = QS_SYSTEM_ERROR;
				continue;
			}
			if( ( *ps >= 'a' && *ps <= 'z' ) 
				|| ( *ps >= 'A' && *ps <= 'Z' )
				|| ( *ps >= '0' && *ps <= '9' )
				|| *ps == '.'
				|| *ps == '_'
				|| *ps == '-'
				|| *ps == '/'
			){
				*(dest++) = *ps;
			}
			else{
				error_code = QS_SYSTEM_ERROR;
				continue;
			}
		}
		*dest='\0';
	}while( false );
	return error_code;
}

void qs_nl2br( char* dest, const char *src, size_t size )
{
	const char *ps;
	char* dstart;
	int convBr;
	do{
		if( dest == NULL || src == NULL ){
			break;
		}
		for( ps = src, dstart = dest; *ps != '\0' && ( dest - dstart ) < (size-1); ps++ )
		{
			convBr = false;
			if( *ps == '\r' && *(ps+1) != '\n' ){
				convBr = true;
			}
			else if( *ps == '\r' && *(ps+1) == '\n' ){
				ps+=1;
				convBr = true;
			}
			else if( *ps == '\n' ){
				convBr = true;
			}
			if( convBr == true && ( dest - dstart ) + 4 < (size-1) ){
				*(dest++) = '<';
				*(dest++) = 'b';
				*(dest++) = 'r';
				*(dest++) = '/';
				*(dest++) = '>';
			}
			else{
				*(dest++) = *ps;
			}
		}
		*dest='\0';
	}while( false );
}

void qs_nl2char( char* dest, const char *src, size_t size )
{
	const char *ps;
	char* dstart;
	int convBr;
	do{
		if( dest == NULL || src == NULL ){
			break;
		}
		for( ps = src, dstart = dest; *ps != '\0' && ( dest - dstart ) < (size-1); ps++ )
		{
			convBr = false;
			if( *ps == '\r' && *(ps+1) != '\n' ){
				convBr = 1;
			}
			else if( *ps == '\r' && *(ps+1) == '\n' ){
				ps+=1;
				convBr = 2;
			}
			else if( *ps == '\n' ){
				convBr = 3;
			}
			if( convBr != false ){
				switch( convBr )
				{
					case 1:
						if( ( dest - dstart ) + 1 < (size-1) ){
							*(dest++) = '\\';
							*(dest++) = 'r';
						}
						break;
					case 2:
						if( ( dest - dstart ) + 3 < (size-1) ){
							*(dest++) = '\\';
							*(dest++) = 'r';
							*(dest++) = '\\';
							*(dest++) = 'n';
						}
						break;
					case 3:
						if( ( dest - dstart ) + 1 < (size-1) ){
							*(dest++) = '\\';
							*(dest++) = 'n';
						}
						break;
				}
			}
			else{
				*(dest++) = *ps;
			}
		}
		*dest='\0';
	}while( false );
}

void qs_strcopy( char* dest, const char*src, size_t size )
{
	char* dstart = dest;
	const char* ps = src;
	if( dest == NULL || src == NULL ){
		return;
	}
	if( size > 0 ){
		do{
			*(dest++) = (*ps);
		}while( *(ps++) != '\0' && ( dest - dstart ) < size-1 );
	}
	*dest = '\0';
}

size_t qs_strlen( const char* src )
{
	size_t s = 0;
	const char* ps = src;
	if( *ps != '\0' ){
		while( *(ps++) != '\0' ){}
		s = ( ( ps - src ) -1 );
	}
	return s;
}

uint32_t qs_ihash( const char* s, uint32_t range )
{
	uint32_t v = 0;
	const char* ps = s;
	int cnt = 0;
	while( *ps != '\0' ){
		v += (*ps)*((++cnt)+v);
		++ps;
	}
	return ( v % range );
}

// example : Sun Sep 16 01:03:52 1973\n\0 = 26byte
int qs_utc_time( char* dest, size_t dest_size )
{
#ifdef __WINDOWS__
	__time64_t long_time;
	struct tm gm_time;
	_time64(&long_time);
	gmtime_s(&gm_time, &long_time);
	asctime_s(dest, dest_size, &gm_time);
	dest[strlen(dest) - 1] = '\0';
#else
	time_t t = time(NULL);
	struct tm *gm_time = gmtime( &t );
	snprintf( dest, dest_size, "%s", asctime( gm_time ) );
	dest[strlen(dest)-1] = '\0';
#endif
	return QS_SYSTEM_OK;
}

int qs_urlencode( char* dest, size_t dest_size, char* src )
{
	int error_code = QS_SYSTEM_OK;
	char* ps = src;
	char* ds = dest;
	const char* hextable = "0123456789ABCDEF";
	while( *ps != '\0' ){
		if( ( ds - dest ) + 3 >= dest_size-1 ){
			error_code = QS_SYSTEM_ERROR;
			break;
		}
		if( 
			   ( *ps >= '0' && *ps <= '9' ) 
			|| ( *ps >= 'a' && *ps <= 'z' ) 
			|| ( *ps >= 'A' && *ps <= 'Z' ) 
			|| *ps == '-'
			|| *ps == '_'
			|| *ps == '.'
			|| *ps == '~'
		){
			*(ds++) = *(ps++);
		}
		else if( *ps == ' ' ){
			*(ds++) = '+';
			ps++;
		}
		else{
			*(ds++) = '%';
			*(ds++) = hextable[((*ps)&0xf0)>>4];
			*(ds++) = hextable[(*ps)&0x0f];
			ps++;
		}
	}
	*ds = '\0';
	return error_code;
}

int qs_urldecode( char* dest, size_t dest_size, char* src )
{
	int error_code = QS_SYSTEM_OK;
	char* ps = src;
	char* ds = dest;
	char h1, h2;
	while( *ps != '\0' ){
		if( ( ds - dest ) + 3 >= dest_size-1 ){
			error_code = QS_SYSTEM_ERROR;
			break;
		}
		if( *ps == '%' ){
			h1 = (*(ps+1)) <= '9' ? (*(ps+1)) - '0' : ( (*(ps+1)) - 'A' ) + 10;
			h2 = (*(ps+2)) <= '9' ? (*(ps+2)) - '0' : ( (*(ps+2)) - 'A' ) + 10;
			*(ds++) = (h1<<4) + h2;
			ps+=3;
		}else if( *ps == '+' ){
			*(ds++) = ' ';
			ps++;
		}else{
			*(ds++) = *(ps++);
		}
	}
	*ds = '\0';
	return error_code;
}

int qs_get_extension( char* dest, size_t dest_size, char* src )
{
	int error_code = QS_SYSTEM_OK;
	size_t s_pos = qs_strlen( src )-1;
	size_t pos = s_pos;
	while( pos > 0 ){
		if( *(src+pos) == '.' ){
			pos++;
			if( s_pos - pos >= dest_size-1 ){
				error_code = QS_SYSTEM_ERROR;
				break;
			}
			while( pos <= s_pos ){
				*(dest++) = *(src+(pos++));
			}
			*dest = '\0';
			break;
		}
		pos--;
	}
	return error_code;
}

void qs_print_hex( uint8_t* hex, size_t size, size_t view_max )
{
	if( size <= 0 || view_max <= 0 ){
		return;
	}
	int i;
	for( i = 0; i < view_max && i < size; i++ ){
		//printf("%c",*(((char*)hex)+i));
		printf("%x",*(hex+i));
	}
	if( size > view_max ){
		printf("...");
	}
	printf("\n");
}