/*
 * Copyright (c) 2014-2016 Katsuya Owari
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

#ifdef __cplusplus
extern "C"{
#endif

#ifndef _GDT_SHA1_H_
#define _GDT_SHA1_H_

#include "gdt_core.h"
#include "gdt_system.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#if defined(__LINUX__) || defined(__BSD_UNIX__)
#include <stdint.h>
#endif
#include <errno.h>

#define GDT_SHA1_HASH_BITS 160
#define GDT_SHA1_HASH_BYTES ( GDT_SHA1_HASH_BITS / 8 )
#define GDT_SHA1_BLOCK_BITS 512
#define GDT_SHA1_BLOCK_BYTES ( GDT_SHA1_BLOCK_BITS / 8 )
#define GDT_SHA1_INTERMEDIAE_HASH_BYTES ( GDT_SHA1_HASH_BYTES / 4 )
#define GDT_SHA1_SEQUENCE_BUFFER_BYTES 80

typedef struct GDT_SHA1_CONTEXT
{
	uint32_t hash[GDT_SHA1_INTERMEDIAE_HASH_BYTES];
	uint8_t message_block[GDT_SHA1_BLOCK_BYTES];
	uint32_t length_high;
	uint32_t length_low;
} GDT_SHA1_CONTEXT;

void gdt_sha1_initialize( GDT_SHA1_CONTEXT* sha1_ctx );
uint32_t gdt_rotate_left( uint32_t v, uint8_t bits ); 
void gdt_sha1( void *dest, const void* src, uint32_t length );
void gdt_sha1_block_proc( GDT_SHA1_CONTEXT* sha1_ctx );

#endif /*_GDT_SHA1_H_*/

#ifdef __cplusplus
}
#endif
