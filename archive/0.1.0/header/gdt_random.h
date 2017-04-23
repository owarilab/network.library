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

#ifdef __cplusplus
extern "C"{
#endif

#ifndef _GDT_RANDOM_H_
#define _GDT_RANDOM_H_

#include "gdt_core.h"
#include <time.h>

#if defined(__LINUX__) || defined(__BSD_UNIX__) || defined(__ANDROID__) || defined(__IOS__)
#include <sys/time.h>
#endif

extern uint32_t gdt_xorshift_32_seed;
extern uint64_t gdt_xorshift_64_seed1;
extern uint64_t gdt_xorshift_64_seed2;

void gdt_srand_32();
void gdt_srand_64();
uint32_t gdt_rand_32();
uint64_t gdt_rand_64();
void gdt_uniqid_r32( char* idbuf, size_t size );
void gdt_uniqid_r64( char* idbuf, size_t size );

#endif /*_GDT_RANDOM_H_*/

#ifdef __cplusplus
}
#endif
