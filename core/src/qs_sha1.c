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

#include "qs_sha1.h"

void qs_sha1_initialize( QS_SHA1_CONTEXT* sha1_ctx )
{
	sha1_ctx->hash[0] = 0x67452301;
	sha1_ctx->hash[1] = 0xEFCDAB89;
	sha1_ctx->hash[2] = 0x98BADCFE;
	sha1_ctx->hash[3] = 0x10325476;
	sha1_ctx->hash[4] = 0xC3D2E1F0;
	memset( sha1_ctx->message_block, 0, QS_SHA1_BLOCK_BYTES );
	sha1_ctx->length_high = 0;
	sha1_ctx->length_low  = 0;
}

uint32_t qs_rotate_left( uint32_t v, uint8_t bits )
{
	return ( ( v << bits ) | ( v >> ( 32 - bits ) ) );
}

void qs_sha1( void *dest, const void* src, uint32_t length )
{
	QS_SHA1_CONTEXT sha1_ctx;
	int i;
	uint8_t tmplen;
	uint8_t* pd = (uint8_t*)dest;
	uint8_t* sps = (uint8_t*)src;
	uint8_t* ps = (uint8_t*)src;
	uint8_t* spmb = sha1_ctx.message_block;
	uint8_t* pmb = sha1_ctx.message_block;
	qs_sha1_initialize( &sha1_ctx );
	if( length > 0 ){
		do{
			*(pmb++) = ( *(ps++) & 0xFF );
			sha1_ctx.length_low += 8;
			if( sha1_ctx.length_low == 0 ){
				sha1_ctx.length_high++;
				if( sha1_ctx.length_high == 0 ){
					// too long
				}
			}
			if( pmb - spmb >= QS_SHA1_BLOCK_BYTES ){
				qs_sha1_block_proc( &sha1_ctx );
				pmb = sha1_ctx.message_block;
				//memset( sha1_ctx.message_block, 0, QS_SHA1_BLOCK_BYTES );
			}
		}while( ( ps - sps ) < length );
	}
	tmplen = ( pmb - spmb );
	if( tmplen > 55 ){
		*(pmb++) = 0x80;
		while( ( pmb - spmb ) < 64 ){
			*(pmb++) = 0;
		}
		qs_sha1_block_proc( &sha1_ctx );
		pmb = spmb;
		while( ( pmb - spmb ) < 56 ){
			*(pmb++) = 0;
		}
	}
	else{
		*(pmb++) = 0x80;
		while( ( pmb - spmb ) < 56 ){
			*(pmb++) = 0;
		}
	}
	sha1_ctx.message_block[56] = sha1_ctx.length_high >> 24;
	sha1_ctx.message_block[57] = sha1_ctx.length_high >> 16;
	sha1_ctx.message_block[58] = sha1_ctx.length_high >> 8;
	sha1_ctx.message_block[59] = sha1_ctx.length_high;
	sha1_ctx.message_block[60] = sha1_ctx.length_low >> 24;
	sha1_ctx.message_block[61] = sha1_ctx.length_low >> 16;
	sha1_ctx.message_block[62] = sha1_ctx.length_low >> 8;
	sha1_ctx.message_block[63] = sha1_ctx.length_low;
	qs_sha1_block_proc( &sha1_ctx );
	for( i = 0; i < QS_SHA1_HASH_BYTES; ++i ){
		pd[i] = sha1_ctx.hash[i>>2] >> 8 * ( 3 - ( i & 0x03 ) );
		//printf( "%02X", pd[i] );
	}
	//printf( "\n" );
	// memcpy( dest, sha1_ctx.hash, QS_SHA1_HASH_BYTES );
}

/*
 * sha1 process message block 
 */
void qs_sha1_block_proc( QS_SHA1_CONTEXT* sha1_ctx )
{
	const uint32_t keys[] = {
		  0x5A827999
		, 0x6ED9EBA1
		, 0x8F1BBCDC
		, 0xCA62C1D6
	};
	uint32_t sequence_buf[QS_SHA1_SEQUENCE_BUFFER_BYTES];
	uint32_t w[QS_SHA1_INTERMEDIAE_HASH_BYTES];
	uint32_t tmp;
	int i;
	for( i = 0; i < 16; i++ ){
		sequence_buf[i]  = sha1_ctx->message_block[i*4]   << 24;
		sequence_buf[i] |= sha1_ctx->message_block[i*4+1] << 16;
		sequence_buf[i] |= sha1_ctx->message_block[i*4+2] << 8;
		sequence_buf[i] |= sha1_ctx->message_block[i*4+3];
	}
	for( i = 16; i < 80; i++ ){
		sequence_buf[i] = qs_rotate_left( sequence_buf[i-3] ^ sequence_buf[i-8] ^ sequence_buf[i-14] ^ sequence_buf[i-16], 1 );
	}
	memcpy( w, sha1_ctx->hash, sizeof( uint32_t ) * QS_SHA1_INTERMEDIAE_HASH_BYTES );
	for( i = 0; i < 20; i++ ){
		tmp = qs_rotate_left( w[0], 5 ) + ( ( w[1] & w[2] ) | ( (~w[1]) & w[3] ) ) + w[4] + sequence_buf[i] + keys[0];
		w[4] = w[3];
		w[3] = w[2];
		w[2] = qs_rotate_left( w[1], 30 );
		w[1] = w[0];
		w[0] = tmp;
	}
	for( i = 20; i < 40; i++ ){
		tmp = qs_rotate_left( w[0], 5 ) + ( w[1] ^ w[2] ^ w[3] ) + w[4] + sequence_buf[i] + keys[1];
		w[4] = w[3];
		w[3] = w[2];
		w[2] = qs_rotate_left( w[1], 30 );
		w[1] = w[0];
		w[0] = tmp;
	}
	for( i = 40; i < 60; i++ ){
		tmp = qs_rotate_left( w[0], 5 ) + ( ( w[1] & w[2] ) | ( w[1] & w[3] ) | ( w[2] & w[3] ) ) + w[4] + sequence_buf[i] + keys[2];
		w[4] = w[3];
		w[3] = w[2];
		w[2] = qs_rotate_left( w[1], 30 );
		w[1] = w[0];
		w[0] = tmp;
	}
	for( i = 60; i < 80; i++ ){
		tmp = qs_rotate_left( w[0], 5 ) + ( w[1] ^ w[2] ^ w[3] ) + w[4] + sequence_buf[i] + keys[3];
		w[4] = w[3];
		w[3] = w[2];
		w[2] = qs_rotate_left( w[1], 30 );
		w[1] = w[0];
		w[0] = tmp;
	}
	for( i = 0; i < QS_SHA1_INTERMEDIAE_HASH_BYTES; i++ ){
		sha1_ctx->hash[i] += w[i];
	}
}

