/*
 * Copyright (c) Katsuya Owari
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
	const char* original_string = "Hello, World!";
	size_t original_length = strlen(original_string);
	char* encoded = api_qs_base64_encode(&temporary_memory_context, original_string, original_length);
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
