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

/*
 * システム共通
 */
#ifndef _GNT_CORE_H_
#define _GNT_CORE_H_

// ビルドOSの定義
//#define WINDOWS_OS
#define LINUX_OS
//#define NETBSD_OS


#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>

#ifdef LINUX_OS
#include <syslog.h>
#include <sys/param.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdint.h>
#endif

#ifdef WINDOWS_OS
#define uint8_t unsigned char
#define uint32_t unsigned int
#define int32_t int
#define uint16_t unsigned short
#define size_t unsigned long
#endif

// 真偽値
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

// ビッグエンディアン、リトルエンディアンの選択
#define _LITTLE_ENDIAN_
//#define _BIG_ENDIAN_

// データサイズ
#define SIZE_BYTE 1							// 1バイト
#define SIZE_KBYTE ( SIZE_BYTE  * 1024 )	// 1キロバイト
#define SIZE_MBYTE ( SIZE_KBYTE * 1024 )	// 1メガバイト
#define SIZE_GBYTE ( SIZE_MBYTE * 1024 )	// 1ギガバイト

// アラインメントサイズ ( 64ビット )
#define MEMORY_ALIGNMENT_SIZE_BIT_64 ( SIZE_BYTE * 8 )

// 最大ディスクリプタ数
#define MAXFD 1024

// 共通システムエラーコード
#define SYS_ERROR ( -1 )

// ビルドオプション
#define __GNT_DEBUG__
#ifndef __GNT_DEBUG__
#define __GNT_RELEASE__
#endif

// 共通変数型の宣言( 各CPU毎に変更する )
#ifdef LINUX_OS
	typedef unsigned int  gnt_uint32;
	typedef int           gnt_int32;
	typedef unsigned char gnt_uint8;
	typedef char          gnt_int8;
#endif
#ifdef WINDOWS_OS
	typedef unsigned int  gnt_uint32;
	typedef int           gnt_int32;
	typedef unsigned char gnt_uint8;
	typedef char          gnt_int8;
#endif

// メモリ識別番号
#define MEMORY_TYPE_DEFAULT 0

#endif /*_GNT_CORE_H_*/
