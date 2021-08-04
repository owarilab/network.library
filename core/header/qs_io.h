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

#ifndef _QS_IO_H_
#define _QS_IO_H_

#include "qs_core.h"
#include "qs_memory_allocator.h"
#include "qs_string.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#if defined(__LINUX__) || defined(__BSD_UNIX__) || defined(__ANDROID__) || defined(__IOS__)
#include <dirent.h>
#include <unistd.h>
#include <utime.h>
#endif

typedef struct QS_FILE_INFO
{
	size_t size;
	int64_t update_usec;
	FILE* f;
	char path[MAXPATHLEN];
} QS_FILE_INFO;

int qs_finit( QS_FILE_INFO* info);
int qs_fopen( char* file_name, char* mode, QS_FILE_INFO* info );
int qs_fwrite2( QS_FILE_INFO* info, char* write_buffer, size_t write_buffer_size );
int qs_fclose( QS_FILE_INFO* info );
int qs_fget_info( char* file_name, QS_FILE_INFO* info );
int qs_fputchar( FILE* f );
int qs_fputchar_line( FILE* f, uint32_t start, uint32_t line );
int qs_fout( char* file_name );
int qs_fout_line( char* file_name, uint32_t start, uint32_t line );
size_t qs_fread( char* file_name, char* dest, size_t size );
size_t qs_fread_range( char* file_name, char* dest, size_t pos, size_t size );
size_t qs_fread_bin( char* file_name, char* dest, size_t size );
size_t qs_fread_bin_range( char* file_name, char* dest, size_t pos, size_t size );
int qs_fwrite( char* file_name, char* out, size_t size );
int qs_fwrite_a( char* file_name, char* out, size_t size );
int qs_fwrite_bin( char* file_name, char* out, size_t size );
int qs_fwrite_bin_a( char* file_name, char* out, size_t size );
int qs_frename( const char* old_name, const char* new_name );
int qs_unlink( const char* file_name );
int32_t qs_lstate( QS_MEMORY_POOL* _ppool, const char* path );
void qs_lstateout( QS_MEMORY_POOL* _ppool, const char* path );

#ifdef __WINDOWS__
#else
int qs_chown( char* file_name, uid_t owner, gid_t group );
int qs_lchown( char* file_name, uid_t owner, gid_t group );
int qs_utime( char *file_name, struct utimbuf *buf );
int qs_chmod_char( char *file_name, char* mode );
int qs_ls( char* path );
int qs_mkdir( char* path, mode_t mode );
int qs_rmdir( char* path );
int qs_link( const char* src, const char* dest );
int qs_symlink( const char* src, const char* dest );
int32_t qs_readlink( QS_MEMORY_POOL* _ppool, const char* path );
char* qs_filetype2char( mode_t mode );
#endif

// freopen

#endif /*_QS_IO_H_*/

#ifdef __cplusplus
}
#endif
