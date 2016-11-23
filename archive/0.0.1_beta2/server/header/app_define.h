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

#ifndef _APP_DEFINE_H_
#define _APP_DEFINE_H_
//------------------------------------------------------------
// include
//------------------------------------------------------------
#include "core.h"
#include "gnt_system.h"
#include "gnt_socket.h"
#include "gnt_memory_allocator.h"
#include "gnt_io.h"
#include "gnt_string.h"
#include "gnt_random.h"

//------------------------------------------------------------
// global
//------------------------------------------------------------
GNT_MEMORY_POOL* __mp;
GNT_MEMORY_POOL* __mmapp;

//------------------------------------------------------------
// define
//------------------------------------------------------------

typedef struct GNT_SERVER_THREAD_INFO STRUCT_SERVER_INFO;

//------------------------------------------------------------
// struct
//------------------------------------------------------------

//------------------------------------------------------------
// function
//------------------------------------------------------------

int main_proc();

void* _connection_start_callback( void* args );
void* _send_finish_callback( void* args );
void* _recv_callback( void* args );


#endif /*_APP_DEFINE_H_*/

