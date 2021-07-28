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

#include "qs_base64.h"

uint8_t gdt_base64_char2ascii( uint8_t c )
{
	uint8_t cnv = '\0';
	if( c >= 0x30 && c <= 0x39 ){ // 0~9
		cnv = 0x34 + ( c - 0x30 );
	}
	else if( c >= 0x41 && c <= 0x5a ){// A~Z
		cnv = c - 0x41;
	}
	else if( c >= 0x61 && c <= 0x7a ){// a~z
		cnv = 0x1a + ( c - 0x61 );
	}
	else if( c == 0x2b ){// '+'
		cnv = c - 0x62;
	}
	else if( c == 0x2f ){// '/'
		cnv = c - 0x63;
	}
	return cnv;
}

/*
 * base64 encode
 * @param dest
 * @param destlength
 * @param src
 * @param length
 */
void gdt_base64_encode(char* dest, uint16_t destlength, const void* src, uint16_t length)
{
	int i = -1;
	int tmpv = 0;
	char *p = dest;
	const uint8_t *ps = (uint8_t*)src;
	const char* basestring = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	do{
		i++;
		tmpv = i % 4;
//		if( tmpv == 0 ){
//			if( *ps == '\0' ){
//				break;
//			}
//		}
		switch( tmpv )
		{
			case 0:
				*p = basestring[ (uint8_t)(*ps >> 2) ];
				break;
			case 1:
				*p = basestring[ (uint8_t)( ( ( ( *(ps-1) << 6 ) & 0xFF ) + ( *ps >> 2 ) ) >> 2 ) & 0x3F ];
				break;
			case 2:
				*p = basestring[ (uint8_t)( ( ( ( *(ps-1) << 4 ) & 0xFF ) + ( *ps >> 4 ) ) >> 2 ) & 0x3F ];
				break;
			case 3:
				*p = basestring[ (uint8_t)(*(ps-1) & 0x3F) ];
				ps--;
				break;
		}
	}while( (++p) - dest < destlength-1 && ( (ps++) - ( (uint8_t*)src ) ) < length );
	if( i % 4 != 0 ){
		while( ( i % 4 ) != 3 ){ *(p++) = '='; i++; }
	}
	*p = '\0';
}

/*
 * base64 decode
 * @param dest
 * @param destlength
 * @param src
 * @param length( \0までのサイズではなく文字数を指定 )
 */
void gdt_base64_decode( char* dest, uint16_t destlength, const void* src, uint16_t length )
{
	int i = -1;
	int tmpv = 0;
	char *p = dest;
	const uint8_t *ps = (uint8_t*)src;
	uint8_t c1;
	uint8_t c2;
	do{
		i++;
		tmpv = i % 3;
		c1 = gdt_base64_char2ascii( *ps );
		c2 = gdt_base64_char2ascii( *(ps+1) );
		switch( tmpv )
		{
			case 0:
				*p = (uint8_t)( ( c1 << 2 ) + ( c2 >> 4 ) );
				break;
			case 1:
				*p = (uint8_t)( ( c1 << 4 ) + ( c2 >> 2 ) );
				break;
			case 2:
				*p = (uint8_t)( ( c1 << 6 ) + ( c2 ) );
				++ps;
				break;
		}
	}while( (++p) - dest < destlength-1 && ( (++ps) - ( (uint8_t*)src ) ) < length );
	*p = '\0';
}
