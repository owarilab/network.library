/*
 * Copyright (c) Katsuya Owari
 */

#ifdef __cplusplus
extern "C"{
#endif

#ifndef _QS_RANDOM_H_
#define _QS_RANDOM_H_

#include "qs_core.h"
#include "qs_system.h"
#include <time.h>

#if defined(__LINUX__) || defined(__BSD_UNIX__) || defined(__ANDROID__) || defined(__IOS__)
#include <sys/time.h>
#endif

extern uint32_t qs_xorshift_32_seed;
extern uint64_t qs_xorshift_64_seed1;
extern uint64_t qs_xorshift_64_seed2;

void qs_srand_32();
void qs_srand_64();
uint32_t qs_rand_32();
uint64_t qs_rand_64();
void qs_uniqid_r32( char* idbuf, size_t size );
void qs_uniqid_r64( char* idbuf, size_t size );

#endif /*_QS_RANDOM_H_*/

#ifdef __cplusplus
}
#endif
