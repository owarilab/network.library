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

#ifdef __cplusplus
extern "C"{
#endif

#ifndef _GDT_CORE_H_
#define _GDT_CORE_H_

#ifdef _WINDOWS
#define __WINDOWS__
#endif

#ifndef __WINDOWS__
#define __LINUX__
//#define __BSD_UNIX__
#endif

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <inttypes.h>

#if defined(__LINUX__) || defined(__BSD_UNIX__)
#include <syslog.h>
#include <sys/param.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdint.h>
#endif

#ifdef __WINDOWS__
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdint.h>
#define NI_MAXHOST  1025
#define NI_MAXSERV	32
#define ssize_t int64_t
# define snprintf(s, n, format, ...) \
    _snprintf_s(s, n, _TRUNCATE, format, __VA_ARGS__)
# define gdt_sprintf(s, n, format, ...) \
    sprintf_s(s, n, format, __VA_ARGS__)
#else
# define gdt_sprintf(s, n, format, ...) \
    sprintf(s, format, __VA_ARGS__)
#endif

#ifdef __WINDOWS__
typedef SOCKET GDT_SOCKET_ID;
#else
typedef int GDT_SOCKET_ID;
#endif

#ifndef TRUE
    #define TRUE 1
#endif
#ifndef true
    #define true 1
#endif
#ifndef FALSE
    #define FALSE 0
#endif
#ifndef false
    #define false 0
#endif

// byte order
#define _LITTLE_ENDIAN_
//#define _BIG_ENDIAN_

#define SIZE_BYTE 1							// 1B
#define SIZE_KBYTE ( SIZE_BYTE  * 1024 )	// 1KB
#define SIZE_MBYTE ( SIZE_KBYTE * 1024 )	// 1MB
#define SIZE_GBYTE ( SIZE_MBYTE * 1024 )	// 1GB

// alignment size
#define MEMORY_ALIGNMENT_SIZE_BIT_64 ( SIZE_BYTE * 8 )
#define MEMORY_ALIGNMENT_SIZE_BIT_32 ( SIZE_BYTE * 4 )

// max file descriptor
#define MAXFD 1024

// system error code
#define SYS_ERROR ( -1 )

// build option
#define __GDT_DEBUG__
#ifndef __GDT_DEBUG__
#define __GDT_RELEASE__
#endif

// memory types
#define MEMORY_TYPE_DEFAULT 0

#endif /*_GDT_CORE_H_*/

#ifdef __cplusplus
} /* extern "C" */
#endif
