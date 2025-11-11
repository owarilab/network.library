/*
 * Copyright (c) Katsuya Owari
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
	_snprintf_s(s, n, _TRUNCATE, format, __VA_ARGS__)
# define qs_sprintf(s, n, format, ...) \
	sprintf_s(s, n, format, __VA_ARGS__)
#else
# define qs_sprintf(s, n, format, ...) \
	sprintf(s, format, __VA_ARGS__)
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

#define SIZE_BYTE 1							// 1B
#define SIZE_KBYTE ( SIZE_BYTE  * 1024 )	// 1KB
#define SIZE_MBYTE ( SIZE_KBYTE * 1024 )	// 1MB
#define SIZE_GBYTE ( SIZE_MBYTE * 1024 )	// 1GB

#define NUMERIC_BUFFER_SIZE (20+sizeof(int32_t))

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
