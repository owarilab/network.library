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

#ifndef _GNT_STRING_H_
#define _GNT_STRING_H_

#include "core.h"
#include "gnt_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#ifdef LINUX_OS
#include <stdint.h>
#endif

#define NLEN 1024

int gnt_itoa( int32_t value, char* target, size_t size );
char* gnt_readline( char* buf, size_t bufsize,char* target, char delimiter_ch );
char* gnt_read_by_delimiter( char* buf, size_t bufsize,char* target, char delimiter_ch );
size_t gnt_strlcat( char *dst, const char *src, size_t size );
size_t gnt_strlink( char *pmain, size_t mainsize, char *psub, size_t subsize, size_t maxsize );
void gnt_escape_directory_traversal( char* dest, const char *src, size_t size );
void gnt_nl2br( char* dest, const char *src, size_t size );
void gnt_nl2char( char* dest, const char *src, size_t size );
void gnt_strcopy( char* dest, const char*src, size_t size );
int gnt_strlen( const char* src );
uint32_t gnt_ihash( const char* s, uint32_t range );

void gnt_calc_add( char* n, const char* t, size_t size );
void gnt_calc_sub( char* n, const char* t, size_t size );
void gnt_calc_mul( char* n, const char* t, size_t size );
void gnt_calc_div( char* n, const char* t, size_t size );

int gnt_calc_zero( const char* n, size_t size );

void gnt_calc_rem( const char* n, const char* t );

// debug
void gnt_print_ch( const char *prefix, const char * s, size_t size );

#endif /*_GNT_STRING_H_*/
