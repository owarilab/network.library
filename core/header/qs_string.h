/*
 * Copyright (c) Katsuya Owari
 */

#ifdef __cplusplus
extern "C"{
#endif

#ifndef _QS_STRING_H_
#define _QS_STRING_H_

#include "qs_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#if defined(__LINUX__) || defined(__BSD_UNIX__) || defined(__ANDROID__) || defined(__IOS__)
#include <stdint.h>
#include <sys/time.h>
#endif

int qs_ltoa( int64_t value, char* target, size_t size );
int qs_ultoa( uint64_t value, char* target, size_t size );
int qs_itoa( int32_t value, char* target, size_t size );
int32_t qs_find_char( char* target, size_t target_size, char delimiter_ch );
char* qs_readline( char* buf, size_t buffer_size,char* target, char delimiter_ch );
char* qs_read_line_delimiter( char* buf, size_t buffer_size,char* target, char delimiter_ch );
char* qs_read_line_delimiter_core( char* buf, size_t buffer_size,char* target, char delimiter_ch, char skip_ch );
char* qs_read_delimiter( char* buf, size_t buffer_size,char* target, char delimiter_ch );
size_t qs_strlcat( char *dst, const char *src, size_t size );
size_t qs_strlink( char *pmain, size_t mainsize, char *psub, size_t subsize, size_t max_size );
int qs_escape_directory_traversal( char* dest, const char *src, size_t size );
void qs_nl2br( char* dest, const char *src, size_t size );
void qs_nl2char( char* dest, const char *src, size_t size );
void qs_strcopy( char* dest, const char*src, size_t size );
size_t qs_strlen( const char* src );
uint32_t qs_ihash( const char* s, uint32_t range );
int qs_utc_time( char* dest, size_t dest_size );
int qs_urlencode( char* dest, size_t dest_size, char* src );
int qs_urldecode( char* dest, size_t dest_size, char* src );
int qs_get_extension( char* dest, size_t dest_size, char* src );
void qs_print_hex( uint8_t* hex, size_t size, size_t view_max );

#endif /*_QS_STRING_H_*/

#ifdef __cplusplus
}
#endif
