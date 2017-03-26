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

#ifndef _GDT_TIMEOUT_H_
#define _GDT_TIMEOUT_H_

#include "gdt_core.h"
#include "gdt_socket.h"

ssize_t recv_with_timeout( char mode, int32_t timeoutsec, GDT_SOCKET_ID soc, char *buf, size_t bufsize, int flag );
ssize_t recv_timeout_by_select(GDT_SOCKET_ID soc, char *buf, size_t bufsize, int flag, int32_t timeoutsec );
#ifdef __WINDOWS__
#else
ssize_t recv_timeout_by_nonblocking(GDT_SOCKET_ID soc, char *buf, size_t bufsize, int flag, int32_t timeoutsec );
ssize_t recv_timeout_by_poll(GDT_SOCKET_ID soc, char *buf, size_t bufsize, int flag, int32_t timeoutsec );
ssize_t recv_timeout_by_epoll(GDT_SOCKET_ID soc, char *buf, size_t bufsize, int flag, int32_t timeoutsec );
ssize_t recv_timeout_by_ioctl(GDT_SOCKET_ID soc, char *buf, size_t bufsize, int flag, int32_t timeoutsec );
ssize_t recv_timeout_by_setsockopt(GDT_SOCKET_ID soc, char *buf, size_t bufsize, int flag, int32_t timeoutsec );
#endif

#endif /*_GDT_TIMEOUT_H_*/

#ifdef __cplusplus
}
#endif
