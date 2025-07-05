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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "qs_api.h"

int main( int argc, char *argv[], char *envp[] )
{
#ifdef __WINDOWS__
	SetConsoleOutputCP(CP_UTF8);
#endif
    QS_MEMORY_CONTEXT temporary_memory_context;
    if(-1==api_qs_memory_alloc(&temporary_memory_context, 1024 * 1024 * 16))
    {
        printf("api_qs_memory_alloc failed\n");
        return -1;
    }

	// 文字列のエンコードとデコード
	char* encoded = api_qs_base64_encode(&temporary_memory_context, "Hello, World!", 13);
	if (encoded) {
		printf("Encoded: %s\n", encoded);
	} else {
		printf("Base64 encoding failed\n");
	}

	char* decoded = api_qs_base64_decode(&temporary_memory_context, encoded);
	if (decoded) {
		printf("Decoded: %s\n", decoded);
	} else {
		printf("Base64 decoding failed\n");
	}

	// byte配列のエンコードとデコード
	const char data[] = { 0x12, 0x22, 0x32, 0x42, 0x52 };
	size_t data_length = sizeof(data);
	char* encoded_data = api_qs_base64_encode(&temporary_memory_context, data, data_length);
	if (encoded_data) {
		printf("Encoded Data: %s\n", encoded_data);
	} else {
		printf("Base64 encoding of byte array failed\n");
	}

	char* decoded_data = api_qs_base64_decode(&temporary_memory_context, encoded_data);
	if (decoded_data) {
		printf("Decoded Data: ");
		for (size_t i = 0; i < data_length; i++) {
			printf("%02X ", (unsigned char)decoded_data[i]);
		}
		printf("\n");
	} else {
		printf("Base64 decoding of byte array failed\n");
	}

	// 文字列のSHA1エンコード
	// echo -n "Hello, World!" | sha1sum
	const char* sha1_data = "Hello, World!";
	char* sha1_encoded = api_qs_sha1_encode(&temporary_memory_context, sha1_data, strlen(sha1_data));
	if (sha1_encoded) {
		printf("SHA1 Encoded: ");
		for (size_t i = 0; i < 20; i++) {
			printf("%02X", (unsigned char)sha1_encoded[i]);
		}
		printf("\n");
	} else {
		printf("SHA1 encoding failed\n");
	}

	// byte配列のSHA1エンコード
	// echo -n -e '\x12\x22\x32\x42\x52' | sha1sum
	const char sha1_byte_data[] = { 0x12, 0x22, 0x32, 0x42, 0x52 };
	size_t sha1_byte_data_length = sizeof(sha1_byte_data);
	char* sha1_encoded_byte = api_qs_sha1_encode(&temporary_memory_context, sha1_byte_data, sha1_byte_data_length);
	if (sha1_encoded_byte) {
		printf("SHA1 Encoded Byte Array: ");
		for (size_t i = 0; i < 20; i++) {
			printf("%02X", (unsigned char)sha1_encoded_byte[i]);
		}
		printf("\n");
	} else {
		printf("SHA1 encoding of byte array failed\n");
	}

	api_qs_memory_free(&temporary_memory_context);

	return 0;
}
