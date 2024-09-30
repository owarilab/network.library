/*
 * Copyright (c) 2014-2024 Katsuya Owari
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

#ifndef _QS_CORE_H_
#define _QS_CORE_H_

#ifdef _WINDOWS
#define __WINDOWS__
#endif

#ifdef _LINUX
#define __LINUX__
#endif

#ifdef _BSD_UNIX
#define __BSD_UNIX__
#endif

#ifdef _ANDROID
#define __ANDROID__
#endif

#ifdef _IOS
#define __IOS__
#endif

// default target os is linux
#if !defined( __WINDOWS__) && !defined( __LINUX__) && !defined( __BSD_UNIX__) && !defined( __ANDROID__) && !defined(__IOS__)
#define __LINUX__
#endif

#if defined(__BSD_UNIX__) || defined(__IOS__)
#define USE_KQUEUE
#endif // ifndef __BSD_UNIX__
#ifdef __LINUX__
#define USE_EPOOL
#endif // ifndef __LINUX__

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <inttypes.h>

#if defined(__LINUX__) || defined(__BSD_UNIX__) || defined(__ANDROID__) || defined(__IOS__)
#include <syslog.h>
#include <sys/param.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdint.h>
#endif

#ifdef __WINDOWS__
#pragma comment(lib,"ws2_32.lib")
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdint.h>
#define NI_MAXHOST  1025
#define NI_MAXSERV	32
#ifndef MAXPATHLEN
#define MAXPATHLEN MAX_PATH
#endif

#ifndef ssize_t
#define ssize_t int64_t
#endif

# define snprintf(s, n, format, ...) \
	_snprintf_s(s, n, _TRUNCATE, format, ##__VA_ARGS__)
# define qs_sprintf(s, n, format, ...) \
	sprintf_s(s, n, format, __VA_ARGS__)
#else //  !__WINDOWS__
# define qs_sprintf(s, n, format, ...) \
	sprintf(s, format, __VA_ARGS__)
#endif // ifdef __WINDOWS__

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

#define SIZE_BYTE 1							// 1B
#define SIZE_KBYTE ( SIZE_BYTE  * 1024 )	// 1KB
#define SIZE_MBYTE ( SIZE_KBYTE * 1024 )	// 1MB
#define SIZE_GBYTE ( SIZE_MBYTE * 1024 )	// 1GB

//#define NUMERIC_CAST_64

// (-9223372036854775808 ~ 9223372036854775807)
#define I64_CAST int64_t
#define I64_CHAR_BYTE 24
#define I64_BUFFER_SIZE (I64_CHAR_BYTE+sizeof(I64_CAST))

// (0 ~ 18446744073709551615)
#define UI64_CAST uint64_t
#define UI64_CHAR_BYTE 24
#define UI64_BUFFER_SIZE (UI64_CHAR_BYTE+sizeof(UI64_CAST))

#ifdef NUMERIC_CAST_64
#define NUMERIC_CAST I64_CAST
#define NUMERIC_CHAR_BYTE I64_CHAR_BYTE
#define NUMERIC_BUFFER_SIZE I64_BUFFER_SIZE
#define QS_PNUMERIC QS_PINT64
#else
#define NUMERIC_CAST int32_t
#define NUMERIC_CHAR_BYTE 20
#define NUMERIC_BUFFER_SIZE (NUMERIC_CHAR_BYTE+sizeof(NUMERIC_CAST))
#define QS_PNUMERIC QS_PINT32
#endif

#define QS_MAX_FILE_DESCRIPTOR 1024

// error code
#define QS_SYSTEM_ERROR -1
#define QS_SYSTEM_OK 0

// build option
#define __QS_DEBUG__
#ifndef __QS_DEBUG__
#define __QS_RELEASE__
#endif

#endif /*_QS_CORE_H_*/

#ifdef __cplusplus
} /* extern "C" */
#endif
