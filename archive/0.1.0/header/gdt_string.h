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

#ifndef _GDT_STRING_H_
#define _GDT_STRING_H_

#include "gdt_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#if defined(__LINUX__) || defined(__BSD_UNIX__) || defined(__ANDROID__) || defined(__IOS__)
#include <stdint.h>
#include <sys/time.h>
#endif

int gdt_itoa( int32_t value, char* target, size_t size );
int32_t gdt_find_char( char* target, size_t target_size, char delimiter_ch );
char* gdt_readline( char* buf, size_t buffer_size,char* target, char delimiter_ch );
char* gdt_readdelimiter( char* buf, size_t buffer_size,char* target, char delimiter_ch );
size_t gdt_strlcat( char *dst, const char *src, size_t size );
size_t gdt_strlink( char *pmain, size_t mainsize, char *psub, size_t subsize, size_t max_size );
int gdt_escape_directory_traversal( char* dest, const char *src, size_t size );
void gdt_nl2br( char* dest, const char *src, size_t size );
void gdt_nl2char( char* dest, const char *src, size_t size );
void gdt_strcopy( char* dest, const char*src, size_t size );
size_t gdt_strlen( const char* src );
uint32_t gdt_ihash( const char* s, uint32_t range );
int gdt_utc_time( char* dest, size_t dest_size );
int gdt_urlencode( char* dest, size_t dest_size, char* src );
int gdt_urldecode( char* dest, size_t dest_size, char* src );
int gdt_get_extension( char* dest, size_t dest_size, char* src );

#endif /*_GDT_STRING_H_*/

#ifdef __cplusplus
}
#endif
