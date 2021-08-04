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

#ifndef _QS_SYSTEM_H_
#define _QS_SYSTEM_H_

#include "qs_core.h"
#include "qs_memory_allocator.h"
#include "qs_string.h"
#include "qs_io.h"
#include <time.h>

#ifndef __WINDOWS__
#include <sysexits.h>
#include <pwd.h>
#include <grp.h>
#endif

#if !defined(__WINDOWS__) && !defined(__BSD_UNIX__) && !defined(__IOS__)
//#include <sys/prctl.h>
#endif

#define MEMORY_UNIT_PATH_BUFFER 0
#define MEMORY_UNIT_CURRENT_DIR_PATH 1
#define MEMORY_UNIT_SIGACTION 2
#define MEMORY_UNIT_HASH_STRING_TABLE 3

#define MAX_CHILD_PROCESS 100

typedef void (*SIG_HANDLER)(int sig);

extern int *argc_;
extern char ***argv_;
extern char ***envp_;

#ifndef __WINDOWS__
extern pid_t child_pid[MAX_CHILD_PROCESS];
#endif

typedef struct SYSTEM_UPDATE_SCHEDULER
{
	int64_t counter;
	int64_t last_counter;
	time_t last_update_time;
	int sleep_time;
	int update_interval_sec;
	
	int64_t counter_high;
	int64_t counter_middle;
	int64_t counter_low;
	
	int update_max;
	int update_high;
	int update_middle;
	int update_low;
	int update_idle;
	
	int8_t on_update;
} SYSTEM_UPDATE_SCHEDULER;

typedef struct SYSTEM_SERVER_OPTION
{
	char* phostname;
	char hostname[256];
	char portnum[32];
	char pid_file_path[1024];
	char log_file_path[1024];
	char execute_user_name[32];
	int inetflag;
	int is_daemonize;
} SYSTEM_SERVER_OPTION;

void gnt_set_argv( int* argc, char ***_argvp_, char *** _envp_ );
void gnt_print_argv();
int gnt_get_argc();
char* gnt_get_argv( int index );

int qs_error( char* err );
#ifdef __WINDOWS__
#else
int qs_get_current_directory( QS_MEMORY_POOL* _ppool );
int qs_daemonize( int nochdir , int noclose );
int qs_descriptor_flags_on( int fd, int flag );
int qs_descriptor_flags_off( int fd, int flag );
int qs_set_defaultsignal();
int qs_set_sigaction( int signum, SIG_HANDLER sh, int sa_flags );
void qs_sig_hangup_handler( int sig );
void qs_sig_chld_handler( int sig );
void qs_sig_int_handler( int sig );
void qs_init_child_pid();
void qs_set_child_pid(pid_t pid,int32_t offset);
int qs_set_execute_user(SYSTEM_SERVER_OPTION* sys_option);
#endif

void qs_localtime(struct tm* ptm, time_t* ptime );
void qs_getopt(SYSTEM_SERVER_OPTION* sys_option, const char* hostname, const char* portnum, const char* pid_file_path, const char* log_file_path, const char* execute_user_name);
void qs_set_pid(SYSTEM_SERVER_OPTION* sys_option);
void qs_sleep(int time);
void qs_initialize_scheduler(SYSTEM_UPDATE_SCHEDULER* scheduler);
void qs_update_scheduler(SYSTEM_UPDATE_SCHEDULER* scheduler);

#endif /*_QS_SYSTEM_H_*/

#ifdef __cplusplus
}
#endif
