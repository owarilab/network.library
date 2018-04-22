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

#ifndef _GDT_SYSTEM_H_
#define _GDT_SYSTEM_H_

#include "gdt_core.h"
#include "gdt_memory_allocator.h"
#include "gdt_string.h"
#include <time.h>

#if !defined(__WINDOWS__) && !defined(__BSD_UNIX__) && !defined(__IOS__)
//#include <sys/prctl.h>
#endif

#define MEMORY_UNIT_PATH_BUFFER 0
#define MEMORY_UNIT_CURRENT_DIR_PATH 1
#define MEMORY_UNIT_SIGACTION 2
#define MEMORY_UNIT_HASH_STRING_TABLE 3

typedef void (*SIG_HANDLER)(int sig);

extern int *argc_;
extern char ***argv_;
extern char ***envp_;

void gnt_set_argv( int* argc, char ***_argvp_, char *** _envp_ );
void gnt_print_argv();
int gnt_get_argc();
char* gnt_get_argv( int index );

int gdt_error( char* err );
#ifdef __WINDOWS__
#else
int gdt_get_current_directory( GDT_MEMORY_POOL* _ppool );
int gdt_daemonize( int nochdir , int noclose );
int gdt_descriptor_flags_on( int fd, int flag );
int gdt_descriptor_flags_off( int fd, int flag );
int gdt_set_defaultsignal();
int gdt_set_sigaction( int signum, SIG_HANDLER sh, int sa_flags );
void gdt_sig_hangup_handler( int sig );
void gdt_sig_chld_handler( int sig );
void gdt_sig_int_handler( int sig );
#endif

#endif /*_GDT_SYSTEM_H_*/

#ifdef __cplusplus
}
#endif
