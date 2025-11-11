/*
 * Copyright (c) Katsuya Owari
 */

#ifdef __cplusplus
extern "C"{
#endif

#ifndef _QS_SHA1_H_
#define _QS_SHA1_H_

#include "qs_core.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#if defined(__LINUX__) || defined(__BSD_UNIX__) || defined(__ANDROID__) || defined(__IOS__)
#include <stdint.h>
#endif
#include <errno.h>

#define QS_SHA1_HASH_BITS 160
#define QS_SHA1_HASH_BYTES ( QS_SHA1_HASH_BITS / 8 )
#define QS_SHA1_BLOCK_BITS 512
#define QS_SHA1_BLOCK_BYTES ( QS_SHA1_BLOCK_BITS / 8 )
#define QS_SHA1_INTERMEDIAE_HASH_BYTES ( QS_SHA1_HASH_BYTES / 4 )
#define QS_SHA1_SEQUENCE_BUFFER_BYTES 80

typedef struct QS_SHA1_CONTEXT
{
	uint32_t hash[QS_SHA1_INTERMEDIAE_HASH_BYTES];
	uint8_t message_block[QS_SHA1_BLOCK_BYTES];
	uint32_t length_high;
	uint32_t length_low;
} QS_SHA1_CONTEXT;

void qs_sha1_initialize( QS_SHA1_CONTEXT* sha1_ctx );
uint32_t qs_rotate_left( uint32_t v, uint8_t bits ); 
void qs_sha1( void *dest, const void* src, uint32_t length );
void qs_sha1_block_proc( QS_SHA1_CONTEXT* sha1_ctx );

#endif /*_QS_SHA1_H_*/

#ifdef __cplusplus
}
#endif
