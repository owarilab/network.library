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

#include "gdt_system.h"

int *argc_;
char ***argv_;
char ***envp_;

void gnt_set_argv( int* _pargc_, char ***_argvp_, char *** _envp_ )
{
	argc_ = _pargc_;
	argv_ = _argvp_;
	envp_ = _envp_;
}

void gnt_print_argv()
{
#ifdef __GDT_DEBUG__
	int i;
#ifdef WINDOWS_OS
#else
	printf( "pid=%d\n", getpid() );
#endif
	printf( "argc=%d\n", (*argc_) );
	for( i = 0; (*argv_)[i] != NULL; i++ ){
		printf( "argv[%d]=%s\n", i, (*argv_)[i] );
	}
	for( i = 0; (*envp_)[i] != NULL; i++ ){
		printf( "envp[%d]=%s\n", i, (*envp_)[i] );
	}
#endif
}

int gnt_get_argc()
{
	return (*argc_);
}

char* gnt_get_argv( int index )
{
	if( index >= 0 && index <= (*argc_) ){
		return (*argv_)[index];
	}
	return NULL;
}

int gdt_error( char* error_str )
{
	perror( error_str );
	return GDT_SYSTEM_ERROR;
}

#ifdef __WINDOWS__
#else
int32_t gdt_get_current_directory( GDT_MEMORY_POOL* _ppool )
{
	int32_t munit = -1;
	do{
		int32_t muPathBuffer = gdt_create_munit( _ppool, MAXPATHLEN, MEMORY_TYPE_DEFAULT );
		char* pbuf = (char*)gdt_upointer( _ppool,muPathBuffer );
		if( pbuf == NULL ){
			printf( "[gdt_get_current_directory]gdt_upointer error\n" );
			break;
		}
		if( NULL == getcwd( pbuf, MAXPATHLEN ) )
		{
			printf( "[gdt_get_current_directory]getcwd error\n" );
			munit = GDT_SYSTEM_ERROR;
			break;
		}
		size_t length = gdt_strlen( pbuf );
		size_t msize = gdt_mgetsize( _ppool, length + 1 );
		munit = gdt_create_munit( _ppool, msize, MEMORY_TYPE_DEFAULT );
		if( munit < 0 ){
			printf( "[gdt_get_current_directory]gdt_create_munit error = %d\n", munit );
			break;
		}
		gdt_strcopy( (char *)gdt_upointer(_ppool,munit), pbuf, gdt_usize(_ppool,munit) );
		gdt_free_memory_unit( _ppool, &muPathBuffer );
#ifdef __GDT_DEBUG__
//		syslog( LOG_USER|LOG_NOTICE, "daemon:cwd=%s\n", (char *)gdt_upointer(_ppool,munit) );
//		printf(  "daemon:cwd=%s\n", (char *)gdt_upointer(_ppool,munit) );
//		printf(  "length=%zd\n", length );
#endif
	}while( false );
	return munit;
}

int gdt_daemonize( int nochdir , int noclose )
{
	int i,fd,error_code;
	pid_t pid;
	do{
		if( ( pid = fork() ) == -1 ){
			error_code = GDT_SYSTEM_ERROR;
			break;
		}else if( pid != 0 ){
			// exit parent process
			_exit(0);
		}
		// session leader
		(void) setsid();
		(void) signal( SIGHUP, SIG_IGN );
		if( ( pid = fork() ) != 0 ){
			// exit first child process
			_exit(0);
		}
		
		if( nochdir == 0 ){
			// move current directory
			error_code = chdir("/");
			if( error_code != 0 ){
				printf( "chdir error" );
				_exit(0);
			}
		}
		
		if( noclose == 0 ){
			for( i = 0; i < GDT_MAX_FILE_DESCRIPTOR; i++ ){
				(void) close(i);
			}
			// stdin,stdout,stderrを/dev/nullでオープン
			if( (fd = open("/dev/null", O_RDWR, 0 )) != -1 ) {
				(void) dup2( fd, 0 );
				(void) dup2( fd, 1 );
				(void) dup2( fd, 2 );
				if( fd > 2 ){
					(void) close(fd);
				}
			}
		}
	}while( false );
	return 0;
}

int gdt_descriptor_flags_on( int fd, int flag )
{
	int flags;
	int error_code = 0;
	do{
		if( ( flags = fcntl( fd, F_GETFL, 0 ) ) == -1 ){
			error_code = -1;
			break;
		}
		(void) fcntl( fd, F_GETFL, flags | flag );
	}while( false );
	return error_code;
}

int gdt_descriptor_flags_off( int fd, int flag )
{
	int flags;
	int error_code = 0;
	do{
		if( ( flags = fcntl( fd, F_GETFL, 0 ) ) == -1 ){
			error_code = -1;
			break;
		}
		(void) fcntl( fd, F_GETFL, ( flags & ( ~flag ) ) );
	}while( false );
	return error_code;
}

int gdt_set_defaultsignal()
{
	(void) gdt_set_sigaction( SIGALRM, SIG_IGN, SA_NODEFER );
	(void) gdt_set_sigaction( SIGPIPE, SIG_IGN, SA_NODEFER );
	(void) gdt_set_sigaction( SIGUSR1, SIG_IGN, SA_NODEFER );
	(void) gdt_set_sigaction( SIGUSR2, SIG_IGN, SA_NODEFER );
	(void) gdt_set_sigaction( SIGTTIN, SIG_IGN, SA_NODEFER );
	(void) gdt_set_sigaction( SIGTTOU, SIG_IGN, SA_NODEFER );
	(void) gdt_set_sigaction( SIGINT, gdt_sig_int_handler, SA_NODEFER );
	(void) gdt_set_sigaction( SIGHUP, gdt_sig_hangup_handler, SA_NODEFER );
	(void) gdt_set_sigaction( SIGCHLD, gdt_sig_chld_handler, SA_NODEFER );
	return 1;
}

int gdt_set_sigaction( int signum, SIG_HANDLER sh, int sa_flags )
{
	struct sigaction sa;
	(void) sigaction( signum, (struct sigaction *) NULL, &sa );
	sa.sa_handler = sh;
	sa.sa_flags = sa_flags;
	(void) sigaction( signum, &sa, (struct sigaction *) NULL );
#ifdef __GDT_DEBUG__
//	printf( "SA_ONSTACK=%d\n"	, (sa.sa_flags&SA_ONSTACK)		? 1 : 0 );
//	printf( "SA_RESETHAND=%d\n"	, (sa.sa_flags&SA_RESETHAND)	? 1 : 0 );
//	printf( "SA_NODEFER=%d\n"	, (sa.sa_flags&SA_NODEFER)		? 1 : 0 );
//	printf( "SA_RESTART=%d\n"	, (sa.sa_flags&SA_RESTART)		? 1 : 0 );
//	printf( "SA_SIGINFO=%d\n"	, (sa.sa_flags&SA_SIGINFO)		? 1 : 0 );
#endif
	return 1;
}

/*
 * SIGHUP
 */
void gdt_sig_hangup_handler( int sig )
{
	int i;
#ifdef __GDT_DEBUG__
	printf( "gdt_sig_hangup_handler(%d)\n", sig );
#endif
	for( i = 3; i < GDT_MAX_FILE_DESCRIPTOR; i++ ){
		(void) close(i);
	}
	if( execve( (*argv_)[0], (*argv_), (*envp_) ) == -1 ) {
		perror("execve");
	}
}

/*
 * SIGCHILD ( fork -> exit )
 */
void gdt_sig_chld_handler( int sig )
{
	int status;
	pid_t pid;
	pid = wait( &status );
	if( pid > 0 )
	{
		
	}
#ifdef __GDT_DEBUG__
	printf( "gdt_sig_chld_handler:wait:pid%d,status=%d\n", pid, status );
	printf( " WIFEXITED:%d, WEXITSTATUS:%d, WIFSIGNALED:%d,WTERMSIG:%d,WIFSTOPPED:%d,WSTOPSIG:%d\n",
					WIFEXITED(status),
					WEXITSTATUS(status),
					WIFSIGNALED(status),
					WTERMSIG(status),
					WIFSTOPPED(status),
					WSTOPSIG(status)
			);
#endif
}

/*
 * SIGINT( Ctrl + c )
 */
void gdt_sig_int_handler( int sig )
{
#ifdef __GDT_DEBUG__
	printf("\nsigint handler\n");
#endif
	exit(0);
}

#endif // ifndef __WINDOWS__

void gdt_sleep(int time)
{
#ifdef __WINDOWS__
	if(time<=1000){
		time=1;
	}
	else{
		time=time/1000;
	}
	Sleep(time);
#else
	usleep(time);
#endif
}