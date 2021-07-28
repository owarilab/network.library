/*
 * Copyright (c) 2014-2017 Katsuya Owari
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

#include "qs_random.h"

uint32_t gdt_xorshift_32_seed;
uint64_t gdt_xorshift_64_seed1;
uint64_t gdt_xorshift_64_seed2;

void gdt_srand_32()
{
#ifdef __WINDOWS__
	uint32_t seed = 0;
	time_t now = time( NULL );
	struct tm mytm;
	struct tm *ptm = &mytm;
	gdt_localtime( &mytm, &now );
	int32_t my = ptm->tm_year + 1;
	int32_t mm = ptm->tm_mon + 1;
	int32_t md = ptm->tm_mday + 1;
	int32_t mh = ptm->tm_hour + 1;
	int32_t mmin = ptm->tm_min + 1;
	int32_t msec = ptm->tm_sec + 1;
	int32_t clc = clock() + 1;
	seed = my * mm * md * clc;
	gdt_xorshift_32_seed = ( seed * msec ) * mh * mmin * msec * clc;
	seed = seed % 0xFF;
	while( seed-- > 0 ){
		gdt_rand_32();
	}
#else
	int i;
	int finish;
	struct timeval startTime, endTime;
	gettimeofday(&startTime, NULL);
	finish = startTime.tv_usec % 256;
	gdt_xorshift_32_seed = startTime.tv_usec;
	for( i = 0; i < finish; i++ ){ gdt_rand_32(); }
	gettimeofday(&endTime, NULL);
	gdt_xorshift_32_seed = gdt_xorshift_32_seed * endTime.tv_usec;
#endif
}

void gdt_srand_64()
{
#ifdef __WINDOWS__
	uint64_t seed = 0;
	time_t now = time( NULL );
	struct tm mytm;
	struct tm *ptm = &mytm;
	gdt_localtime( &mytm, &now );
	int32_t my = ptm->tm_year + 1;
	int32_t mm = ptm->tm_mon + 1;
	int32_t md = ptm->tm_mday + 1;
	int32_t mh = ptm->tm_hour + 1;
	int32_t mmin = ptm->tm_min + 1;
	int32_t msec = ptm->tm_sec + 1;
	int32_t clc = clock() + 1;
	seed = my * mm * md * clc;
	gdt_xorshift_64_seed1 = ( seed * msec ) * mh * mmin * msec * clc;
	seed = seed % 0xFF;
	while( seed-- > 0 ){
		gdt_rand_64();
	}
	gdt_xorshift_64_seed2 = ( gdt_xorshift_64_seed1 * msec ) + mh + mmin + msec * clc;
#else
	int i;
	int finish;
	struct timeval startTime, endTime;
	gettimeofday(&startTime, NULL);
	finish = startTime.tv_usec % 256;
	gdt_xorshift_64_seed1 = startTime.tv_usec;
	gdt_xorshift_64_seed2 = startTime.tv_usec + 1;
	for( i = 0; i < finish; i++ ){ gdt_xorshift_64_seed1 = gdt_rand_64(); }
	gettimeofday(&endTime, NULL);
	gdt_xorshift_64_seed2 = gdt_xorshift_64_seed1 * endTime.tv_usec;
#endif
}

uint32_t gdt_rand_32()
{
	gdt_xorshift_32_seed = gdt_xorshift_32_seed ^ ( gdt_xorshift_32_seed << 13 );
	gdt_xorshift_32_seed = gdt_xorshift_32_seed ^ ( gdt_xorshift_32_seed >> 17 );
	gdt_xorshift_32_seed = gdt_xorshift_32_seed ^ ( gdt_xorshift_32_seed << 15 );
	return gdt_xorshift_32_seed;
}

uint64_t gdt_rand_64()
{
//	uint64_t s1 = gdt_xorshift_64_seed1;
//	uint64_t s2 = gdt_xorshift_64_seed2;
//	s1 = s1 ^ ( s1 >> 26 );
//	s2 = s2 ^ ( s2 << 23 );
//	s2 = s1 ^ ( s2 >> 17 );
//	gdt_xorshift_64_seed1 = gdt_xorshift_64_seed2;
//	gdt_xorshift_64_seed2 = s1 ^ s2;
//	return ( gdt_xorshift_64_seed1 + gdt_xorshift_64_seed2 -1 );
	gdt_xorshift_64_seed2 = gdt_xorshift_64_seed2 ^ ( gdt_xorshift_64_seed2 << 13 );
	gdt_xorshift_64_seed2 = gdt_xorshift_64_seed2 ^ ( gdt_xorshift_64_seed2 >> 7 );
	gdt_xorshift_64_seed2 = gdt_xorshift_64_seed2 ^ ( gdt_xorshift_64_seed2 >> 17 );
	return gdt_xorshift_64_seed2;
}

void gdt_uniqid_r32( char* idbuf, size_t size )
{
	int i = 0;
	uint32_t r = 0;
	while( i < size )
	{
		//r = gdt_rand_32() % 35;
		r = gdt_rand_32() % 61;
		if( r < 10 ){
			idbuf[i++] = 48 + r; // 0-9
		}
		else if( r < 35 ){
			idbuf[i++] = 97 + ( r - 9 ); // a-z
		}
		else{
			idbuf[i++] = 65 + ( r - 35 ); // A-Z
		}
	}
	idbuf[i] = '\0';
}

void gdt_uniqid_r64( char* idbuf, size_t size )
{
	int i = 0;
	uint64_t r = 0;
	while( i < size )
	{
		//r = gdt_rand_64() % 35;
		r = gdt_rand_64() % 61;
		if( r < 10 ){
			idbuf[i++] = (int8_t)(48 + r); // 0-9
		}
		else if( r < 35 ){
			idbuf[i++] = (int8_t)(97 + ( r - 9 )); // a-z
		}
		else{
			idbuf[i++] = (int8_t)(65 + ( r - 35 )); // A-Z
		}
	}
	idbuf[i] = '\0';
}
