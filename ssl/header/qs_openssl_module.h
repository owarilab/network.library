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

#ifndef _QS_OPENSSL_MODULE_H_
#define _QS_OPENSSL_MODULE_H_

#include <sys/types.h>
#include <inttypes.h>
#include <stdio.h>

#include "qs_api.h"

// sudo apt install libssl-dev
#include <openssl/ssl.h>
#include <openssl/err.h>

#define QS_SSL_MODULE_PHASE_CONNECT 0
#define QS_SSL_MODULE_PHASE_READ_HEADER 1
#define QS_SSL_MODULE_PHASE_READ_BODY 2
#define QS_SSL_MODULE_PHASE_READ_CHUNKED_BODY 3
#define QS_SSL_MODULE_PHASE_DISCONNECT 4

typedef struct QS_SSL_MODULE_CONTEXT
{
    SSL_CTX *ctx;
    SSL *ssl;
    QS_CLIENT_CONTEXT* client_context;
    char host[1024];
    char port[16];
    char request_buffer[1024 * 1024];
    char read_buffer[1024 * 1024];
    int socket;

    int is_ssl;

    // working data
    int phase;
	size_t body_length;
	size_t total_read_body_length;
	size_t temp_chunked_size;
	size_t temp_chunked_read_size;
	char header_buffer[1024 * 1024];
	char body_buffer[1024 * 1024 * 4];
    char* body_buffer_ptr;
} QS_SSL_MODULE_CONTEXT;

int qs_openssl_module_connect(QS_SSL_MODULE_CONTEXT* context,const char* server_host, int server_port, int is_ssl);
SSL_CTX* qs_openssl_module_ssl_create_context();
SSL* qs_openssl_module_ssl_create(SSL_CTX* ctx, int sock);
int qs_openssl_module_update(QS_SSL_MODULE_CONTEXT* context);
int qs_openssl_module_recv(QS_SSL_MODULE_CONTEXT* context, char* payload, size_t payload_size);
int qs_openssl_module_free(QS_SSL_MODULE_CONTEXT* context);

#endif /*_QS_OPENSSL_MODULE_H_*/

#ifdef __cplusplus
}
#endif
