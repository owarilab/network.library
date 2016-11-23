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

#ifndef _GNT_IO_H_
#define _GNT_IO_H_

#include "core.h"
#include "gnt_system.h"
#include "gnt_memory_allocator.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <utime.h>

struct FILE_INFO
{
	size_t size;
};

typedef struct FILE_INFO FILE_INFO;

int gnt_fgetInfo( char* fileName, FILE_INFO* info );
int gnt_chown( char* fileName, uid_t owner, gid_t group );
int gnt_lchown( char* fileName, uid_t owner, gid_t group );
int gnt_utime( char *fileName, struct utimbuf *buf );
int gnt_chmod_char( char *fileName, char* mode );
int gnt_fputchar( FILE* f );
int gnt_fputcharLine( FILE* f, uint32_t start, uint32_t line );
int gnt_fout( char* fileName );
int gnt_foutLine( char* fileName, uint32_t start, uint32_t line );
int gnt_fread( char* fileName, char* dest, size_t size );
size_t gnt_fread_bin( char* fileName, char* dest, size_t size );
int gnt_fwrite( char* fileName, char* out, size_t size );
int gnt_fwrite_a( char* fileName, char* out, size_t size );
int gnt_fwrite_bin( char* fileName, char* out, size_t size );
int gnt_unlink( const char* fileName );
int gnt_frename( const char* src, const char* dest );
int gnt_ls( char* path );
int gnt_mkdir( char* path, mode_t mode );
int gnt_rmdir( char* path );
int gnt_link( const char* src, const char* dest );
int gnt_symlink( const char* src, const char* dest );
int32_t gnt_readlink( GNT_MEMORY_POOL* _ppool, const char* path );
int32_t gnt_lstate( GNT_MEMORY_POOL* _ppool, const char* path );
void gnt_lstateout( GNT_MEMORY_POOL* _ppool, const char* path );
char* gnt_filetype2char( mode_t mode );

#endif /*_GNT_IO_H_*/
