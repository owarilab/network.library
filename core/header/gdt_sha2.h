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

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// TODO : sha2 do not work yet...
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#ifdef __cplusplus
extern "C"{
#endif

#ifndef _GDT_SHA2_H_
#define _GDT_SHA2_H_

#include "gdt_core.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#if defined(__LINUX__) || defined(__BSD_UNIX__) || defined(__ANDROID__) || defined(__IOS__)
#include <stdint.h>
#endif
#include <errno.h>

#define GDT_SHA256_HASH_BITS 256
#define GDT_SHA256_HASH_BYTES ( GDT_SHA256_HASH_BITS / 8 )
#define GDT_SHA256_BLOCK_BITS 512
#define GDT_SHA256_BLOCK_BYTES ( GDT_SHA256_BLOCK_BITS / 8 )
#define GDT_SHA256_INTERMEDIAE_HASH_BYTES ( GDT_SHA256_HASH_BYTES / 4 )
#define GDT_SHA256_SEQUENCE_BUFFER_BYTES 64

typedef struct GDT_SHA256_CONTEXT
{
	uint32_t hash[GDT_SHA256_INTERMEDIAE_HASH_BYTES];
	uint8_t message_block[GDT_SHA256_BLOCK_BYTES];
	uint32_t length_high;
	uint32_t length_low;
} GDT_SHA256_CONTEXT;

void gdt_sha256_initialize( GDT_SHA256_CONTEXT* sha256_ctx );
uint32_t gdt_sha256_rotate_left( uint32_t v, uint8_t bits );
uint32_t gdt_sha256_rotate_right( uint32_t v, uint8_t bits );
uint32_t gdt_sha256_sigma0( uint32_t v );
uint32_t gdt_sha256_sigma1( uint32_t v );
uint32_t gdt_sha256_sigma2( uint32_t v );
uint32_t gdt_sha256_sigma3( uint32_t v );
void gdt_sha256( void *dest, const void* src, uint32_t length );
void gdt_sha256_block_proc( GDT_SHA256_CONTEXT* sha256_ctx );

#endif /*_GDT_SHA2_H_*/

#ifdef __cplusplus
}
#endif
