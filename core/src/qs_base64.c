/*
 * Copyright (c) Katsuya Owari
 */

#include "qs_base64.h"

static int qs_base64_char2ascii( uint8_t c )
{
	int cnv = -1;
	if( c >= 0x30 && c <= 0x39 ){ // 0~9
		cnv = 0x34 + ( c - 0x30 );
	}
	else if( c >= 0x41 && c <= 0x5a ){// A~Z
		cnv = c - 0x41;
	}
	else if( c >= 0x61 && c <= 0x7a ){// a~z
		cnv = 0x1a + ( c - 0x61 );
	}
	else if( c == 0x2b ){// '+'
		cnv = 62;
	}
	else if( c == 0x2f ){// '/'
		cnv = 63;
	}
	return cnv;
}

/*
 * base64 encode
 * @param dest
 * @param destlength
 * @param src
 * @param length
 */
void qs_base64_encode(char* dest, uint16_t destlength, const void* src, uint16_t length)
{
	if (!dest || !src || destlength == 0 || length == 0) {
		if (dest && destlength > 0) {
			*dest = '\0';
		}
		return;
	}

	const char* basestring = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	const uint8_t *bytes = (const uint8_t*)src;
	size_t output_length = ( ( (size_t)length + 2 ) / 3 ) * 4;
	size_t writable = destlength - 1;
	size_t written = 0;
	for( size_t offset = 0; offset < length && written < writable; offset += 3 ){
		uint32_t value = (uint32_t)bytes[offset] << 16;
		size_t remaining = length - offset;
		if( remaining > 1 ) value |= (uint32_t)bytes[offset + 1] << 8;
		if( remaining > 2 ) value |= bytes[offset + 2];
		dest[written++] = basestring[(value >> 18) & 0x3f];
		if( written < writable ) dest[written++] = basestring[(value >> 12) & 0x3f];
		if( written < writable ) dest[written++] = remaining > 1 ? basestring[(value >> 6) & 0x3f] : '=';
		if( written < writable ) dest[written++] = remaining > 2 ? basestring[value & 0x3f] : '=';
	}
	if( written > output_length ) written = output_length;
	dest[written] = '\0';
}

/*
 * base64 decode
 * @param dest
 * @param destlength
 * @param src
 * @param length( \0までのサイズではなく文字数を指定 )
 */
void qs_base64_decode( char* dest, uint16_t destlength, const void* src, uint16_t length )
{
	if (!dest || !src || destlength == 0 || length == 0) {
		if (dest && destlength > 0) {
			*dest = '\0';
		}
		return;
	}

	if( length % 4 != 0 ){
		dest[0] = '\0';
		return;
	}

	const uint8_t *encoded = (const uint8_t*)src;
	size_t decoded_length = (length / 4) * 3;
	if( length >= 1 && encoded[length - 1] == '=' ) decoded_length--;
	if( length >= 2 && encoded[length - 2] == '=' ) decoded_length--;
	size_t writable = destlength - 1;
	size_t written = 0;
	for( size_t offset = 0; offset < length; offset += 4 ){
		int c1 = qs_base64_char2ascii(encoded[offset]);
		int c2 = qs_base64_char2ascii(encoded[offset + 1]);
		int c3 = encoded[offset + 2] == '=' ? 0 : qs_base64_char2ascii(encoded[offset + 2]);
		int c4 = encoded[offset + 3] == '=' ? 0 : qs_base64_char2ascii(encoded[offset + 3]);
		int last = offset + 4 == length;
		if( c1 < 0 || c2 < 0 || c3 < 0 || c4 < 0 ||
			(encoded[offset + 2] == '=' && encoded[offset + 3] != '=') ||
			(!last && (encoded[offset + 2] == '=' || encoded[offset + 3] == '=')) ||
			(encoded[offset + 2] == '=' && (c2 & 0x0f) != 0) ||
			(encoded[offset + 3] == '=' && encoded[offset + 2] != '=' && (c3 & 0x03) != 0) ){
			dest[0] = '\0';
			return;
		}
		uint32_t value = ((uint32_t)c1 << 18) | ((uint32_t)c2 << 12) |
			((uint32_t)c3 << 6) | (uint32_t)c4;
		if( written < writable && written < decoded_length ) dest[written++] = (value >> 16) & 0xff;
		if( encoded[offset + 2] != '=' && written < writable && written < decoded_length ) dest[written++] = (value >> 8) & 0xff;
		if( encoded[offset + 3] != '=' && written < writable && written < decoded_length ) dest[written++] = value & 0xff;
	}
	dest[written] = '\0';
}
