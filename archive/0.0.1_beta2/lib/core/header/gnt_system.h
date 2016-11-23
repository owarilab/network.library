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

#ifndef _GNT_SYSTEM_H_
#define _GNT_SYSTEM_H_

#include "core.h"
#include "gnt_memory_allocator.h"
#include "gnt_string.h"
#include <time.h>

#ifndef NETBSD_OS
#include <sys/prctl.h>
#endif // ifndef NETBSD_OS

#define MEMORY_UNIT_PATH_BUFFER 0
#define MEMORY_UNIT_CURRENT_DIR_PATH 1
#define MEMORY_UNIT_SIGACTION 2
#define MEMORY_UNIT_HASH_STRING_TABLE 3

typedef void (*sig_handler)(int sig);

//------------------------------------------------------------
// global param
//------------------------------------------------------------
int *argc_;
char ***argv_;
char ***envp_;

void gnt_set_argv( int* argc, char ***_argvp_, char *** _envp_ );
void gnt_print_argv();
int gnt_get_argc();
char* gnt_get_argv( int index );
int gnt_get_env();
void setProcName( const char* name, size_t size );
int getCurrentDirectory( GNT_MEMORY_POOL* _ppool );
int daemonize( int nochdir , int noclose );
int descriptorFlagsOn( int fd, int flag );
int descriptorFlagsOff( int fd, int flag );
int gnt_set_defaultsignal();
int gnt_set_sigaction( int signum, sig_handler sh, int sa_flags );
void sig_hangup_handler( int sig );
void sig_chld_handler( int sig );
void sig_int_handler( int sig );
int gnt_error( char* err );

#endif /*_GNT_SYSTEM_H_*/
