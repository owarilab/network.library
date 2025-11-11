/*
 * Copyright (c) Katsuya Owari
 */

#ifdef __cplusplus
extern "C"{
#endif

#ifndef _QS_BASE64_H_
#define _QS_BASE64_H_

#include "qs_core.h"

void qs_base64_encode(char* dest, uint16_t destlength, const void* src, uint16_t length);
void qs_base64_decode( char* dest, uint16_t destlength, const void* src, uint16_t length );

#endif /*_QS_BASE64_H_*/

#ifdef __cplusplus
}
#endif
