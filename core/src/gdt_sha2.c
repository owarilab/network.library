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

#include "gdt_sha2.h"

void gdt_sha256_initialize( GDT_SHA256_CONTEXT* sha256_ctx )
{
	sha256_ctx->hash[0] = 0x6A09E667;
	sha256_ctx->hash[1] = 0xBB67AE85;
	sha256_ctx->hash[2] = 0x3C6EF372;
	sha256_ctx->hash[3] = 0xA54FF53A;
	sha256_ctx->hash[4] = 0x510E527F;
	sha256_ctx->hash[5] = 0x9B05688C;
	sha256_ctx->hash[6] = 0x1F83D9AB;
	sha256_ctx->hash[7] = 0x5BE0CD19;
	memset( sha256_ctx->message_block, 0, GDT_SHA256_BLOCK_BYTES );
	sha256_ctx->length_high = 0;
	sha256_ctx->length_low  = 0;
}

uint32_t gdt_sha256_rotate_left( uint32_t v, uint8_t bits )
{
	return ( ( v << bits ) | ( v >> ( 32 - bits ) ) );
}

uint32_t gdt_sha256_rotate_right( uint32_t v, uint8_t bits )
{
	return ( ( v >> bits ) | ( v << ( 32 - bits ) ) );
}

uint32_t gdt_sha256_sigma0( uint32_t v )
{
	return gdt_sha256_rotate_right( v, 2 ) ^ gdt_sha256_rotate_right( v, 13 ) ^ gdt_sha256_rotate_right( v, 22 );
}
uint32_t gdt_sha256_sigma1( uint32_t v )
{
	return gdt_sha256_rotate_right( v, 6 ) ^ gdt_sha256_rotate_right( v, 11 ) ^ gdt_sha256_rotate_right( v, 25 );
}

uint32_t gdt_sha256_sigma2( uint32_t v )
{
	return gdt_sha256_rotate_right( v, 7 ) ^ gdt_sha256_rotate_right( v, 18 ) ^ ( v >> 3 );
}

uint32_t gdt_sha256_sigma3( uint32_t v )
{
	return gdt_sha256_rotate_right( v, 17 ) ^ gdt_sha256_rotate_right( v, 19 ) ^ ( v >> 10 );
}

void gdt_sha256( void *dest, const void* src, uint32_t length )
{
	GDT_SHA256_CONTEXT sha256_ctx;
	int i;
	uint8_t tmplen;
	uint8_t* pd = (uint8_t*)dest;
	uint8_t* sps = (uint8_t*)src;
	uint8_t* ps = (uint8_t*)src;
	uint8_t* spmb = sha256_ctx.message_block;
	uint8_t* pmb = sha256_ctx.message_block;
	gdt_sha256_initialize( &sha256_ctx );
	if( length > 0 ){
		do{
			*(pmb++) = ( *(ps++) & 0xFF );
			sha256_ctx.length_low += 8;
			if( sha256_ctx.length_low == 0 ){
				sha256_ctx.length_high++;
				if( sha256_ctx.length_high == 0 ){
					// too long
				}
			}
			if( pmb - spmb >= GDT_SHA256_BLOCK_BYTES ){
				gdt_sha256_block_proc( &sha256_ctx );
				pmb = sha256_ctx.message_block;
				//memset( sha256_ctx.message_block, 0, GDT_SHA256_BLOCK_BYTES );
			}
		}while( ( ps - sps ) < length );
	}
	tmplen = ( pmb - spmb );
	if( tmplen >= GDT_SHA256_BLOCK_BYTES-8 ){
		*(pmb++) = 0x80;
		while( ( pmb - spmb ) < GDT_SHA256_BLOCK_BYTES ){
			*(pmb++) = 0;
		}
		gdt_sha256_block_proc( &sha256_ctx );
	}
	else{
		*(pmb++) = 0x80;
		while( ( pmb - spmb ) < GDT_SHA256_BLOCK_BYTES-8 ){
			*(pmb++) = 0;
		}
	}
	sha256_ctx.message_block[56] = (uint8_t)(sha256_ctx.length_high >> 24);
	sha256_ctx.message_block[57] = (uint8_t)(sha256_ctx.length_high >> 16);
	sha256_ctx.message_block[58] = (uint8_t)(sha256_ctx.length_high >> 8);
	sha256_ctx.message_block[59] = (uint8_t)(sha256_ctx.length_high);
	sha256_ctx.message_block[60] = (uint8_t)(sha256_ctx.length_low >> 24);
	sha256_ctx.message_block[61] = (uint8_t)(sha256_ctx.length_low >> 16);
	sha256_ctx.message_block[62] = (uint8_t)(sha256_ctx.length_low >> 8);
	sha256_ctx.message_block[63] = (uint8_t)(sha256_ctx.length_low);
	gdt_sha256_block_proc( &sha256_ctx );
	for( i = 0; i < GDT_SHA256_HASH_BYTES; ++i ){
		pd[i] = (uint8_t)( sha256_ctx.hash[i>>2] >> 8 * ( 3 - ( i & 0x03 ) ) );
		printf( "%02X", pd[i] );
	}
	printf( "\n" );
//	memcpy( dest, sha256_ctx.hash, GDT_SHA256_HASH_BYTES );
}

/*
 * sha256 process message block 
 */
void gdt_sha256_block_proc( GDT_SHA256_CONTEXT* sha256_ctx )
{
	const uint32_t keys[64] = {
		0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b,
		0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01,
		0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7,
		0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
		0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152,
		0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
		0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
		0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
		0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819,
		0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08,
		0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f,
		0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
		0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
	};

	uint32_t sequence_buf[GDT_SHA256_SEQUENCE_BUFFER_BYTES];
	uint32_t w[GDT_SHA256_INTERMEDIAE_HASH_BYTES];
	uint32_t tmp1,tmp2;
	int i, v4;
	for( i = 0, v4 = 0; i < 16; i++, v4+=4 ){
		sequence_buf[i]  = ( (uint32_t)(sha256_ctx->message_block[v4])   << 24 ) | 
						   ( (uint32_t)(sha256_ctx->message_block[v4+1]) << 16 ) |
						   ( (uint32_t)(sha256_ctx->message_block[v4+2]) << 8 )  |
						   ( (uint32_t)(sha256_ctx->message_block[v4+3]) );
	}
	for( i = 16; i < 64; i++ ){
		sequence_buf[i] = gdt_sha256_sigma3( sequence_buf[i-2] ) + sequence_buf[i-7] + gdt_sha256_sigma2( sequence_buf[i-15] ) + sequence_buf[i-16];
	}
	memcpy( w, sha256_ctx->hash, sizeof( uint32_t ) * GDT_SHA256_INTERMEDIAE_HASH_BYTES );
	for( i = 0; i < 64; i++ ){
		tmp1 = w[7] + gdt_sha256_sigma1( w[4] ) + ( ( w[4] & w[5] ) ^ ( (~w[4]) & w[6] ) ) + keys[i] + sequence_buf[i];
		tmp2 = gdt_sha256_sigma0( w[0] ) + ( ( w[0] & w[1] ) ^ ( w[0] & w[3] ) ^ ( w[1] & w[3] ) );
		w[7] = w[6];
		w[6] = w[5];
		w[5] = w[4];
		w[4] = w[3] + tmp1;
		w[3] = w[2];
		w[2] = w[1];
		w[1] = w[0];
		w[0] = tmp1 + tmp2;
	}
	for( i = 0; i < GDT_SHA256_INTERMEDIAE_HASH_BYTES; i++ ){
		sha256_ctx->hash[i] += w[i];
	}
}
