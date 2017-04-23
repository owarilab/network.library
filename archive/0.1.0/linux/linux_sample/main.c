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

#include <sysexits.h>
#include "gdt_core.h"
#include "gdt_memory_allocator.h"
#include "gdt_script.h"

void sample_byte_array(GDT_MEMORY_POOL* memory_pool);
void sample_array_hash(GDT_MEMORY_POOL* memory_pool);
void sample_script(GDT_MEMORY_POOL* memory_pool);

int main( int argc, char *argv[], char *envp[] )
{
	int exe_code = EX_OK;
	GDT_MEMORY_POOL* memory_pool = NULL;
#ifdef __GDT_DEBUG__
#ifdef __WINDOWS__
	LARGE_INTEGER start_pc, end_pc, freq_pc;
	double sec_pc;
	QueryPerformanceFrequency( &freq_pc );
	QueryPerformanceCounter( &start_pc );
#else
	struct timeval start_timeval, end_timeval;
	double sec_timeofday;
	gettimeofday( &start_timeval, NULL );
#endif
#endif
	do{
		if( gdt_initialize_memory( &memory_pool, SIZE_MBYTE * 2, SIZE_MBYTE * 2, MEMORY_ALIGNMENT_SIZE_BIT_64, 0, 0, 0) <= 0 )
		{
			printf(  "gdt_initialize_memory error\n" );
			break;
		}
		sample_array_hash(memory_pool);
		sample_byte_array(memory_pool);
		sample_script(memory_pool);
		gdt_free( memory_pool );
	}while( false );
#ifdef __GDT_DEBUG__
#ifdef __WINDOWS__
	QueryPerformanceCounter( &end_pc );
	sec_pc = (end_pc.QuadPart - start_pc.QuadPart) / (double)freq_pc.QuadPart;
	printf(  "## gdt_exec time = %lf[s]\n", sec_pc );
#else
	gettimeofday( &end_timeval, NULL );
	sec_timeofday = (end_timeval.tv_sec - start_timeval.tv_sec)
			+ (end_timeval.tv_usec - start_timeval.tv_usec) / 1000000.0;
	printf(  "## gdt_exec time = %lf[s]\n", sec_timeofday );
#endif
#endif
	return exe_code;
}

void sample_byte_array(GDT_MEMORY_POOL* memory_pool)
{
	int32_t tmpmunit = gdt_create_munit( memory_pool, sizeof(uint8_t) * 12, MEMORY_TYPE_DEFAULT );
	if( tmpmunit == -1 ){
		return;
	}
	GDT_BYTE_BUFFER bbuffer;
	gdt_set_buffer( memory_pool, tmpmunit, &bbuffer );
	uint8_t payload_type = 0x01;
	uint8_t float_size = sizeof( float );
	uint16_t size = sizeof( float ) * 3;
	float px = 0.1f;
	float py = 0.2f;
	float pz = 0.3f;
	*(bbuffer.pos++) = payload_type;
	*(bbuffer.pos++) = float_size;
	MEMORY_PUSH_BIT16_B( memory_pool, bbuffer.pos, size );
	MEMORY_PUSH_CAST_BIT32_B( memory_pool, bbuffer.pos, px );
	MEMORY_PUSH_CAST_BIT32_B( memory_pool, bbuffer.pos, py );
	MEMORY_PUSH_CAST_BIT32_B( memory_pool, bbuffer.pos, pz );
	bbuffer.pos = bbuffer.buffer;
	bbuffer.pos+=4;
	uint32_t val1 = gdt_pop_big_to_host_bit32( &bbuffer );
	uint32_t val2 = gdt_pop_big_to_host_bit32( &bbuffer );
	uint32_t val3 = gdt_pop_big_to_host_bit32( &bbuffer );
	printf("ret float : %f\n",*(float*)(&val1));
	printf("ret float : %f\n",*(float*)(&val2));
	printf("ret float : %f\n",*(float*)(&val3));
	int i;
	for( i = 0; i < bbuffer.size; i++ ){
		printf("%02x ", bbuffer.buffer[i]);
	}
	printf("\n");
}

void sample_array_hash(GDT_MEMORY_POOL* memory_pool)
{
	int i;
	int32_t array_munit = -1;
	for( i = 0; i < 8; i++ ){
		int32_t hash_munin = gdt_create_hash( memory_pool, 1 );
		char tmpbuf[255];
		snprintf( tmpbuf, sizeof( tmpbuf ) -1, "dummy%d", i );
		gdt_add_hash_string( memory_pool, hash_munin, "name", tmpbuf );
		gdt_add_hash_integer( memory_pool, hash_munin, "point", i );
		gdt_array_push( memory_pool, &array_munit, ELEMENT_HASH, hash_munin );
	}
	int32_t root_munit = gdt_make_json_root( memory_pool, array_munit, ELEMENT_ARRAY ); 
	int32_t string_munit = gdt_json_encode( memory_pool, (GDT_NODE*)GDT_POINTER( memory_pool, root_munit ), 1024);
	printf( "json : %s\n", (char*)GDT_POINTER(memory_pool,string_munit) );
}

void sample_script(GDT_MEMORY_POOL* memory_pool)
{
	int32_t script_munit = -1;
	const char* script_file = "test.gs";
	if( 0 >=( script_munit = gdt_init_script( memory_pool, 128, 64 ) ) )
	{
		printf( "gdt_init_script error\n" );
		return;
	}
	//GDT_SCRIPT *pscript = (GDT_SCRIPT *)GDT_POINTER( memory_pool, script_munit );
	gdt_add_system_function( memory_pool, script_munit, "echo", gdt_script_system_function_echo, 0 );
	gdt_add_system_function( memory_pool, script_munit, "count", gdt_script_system_function_count, 0 );
	gdt_add_system_function( memory_pool, script_munit, "file_exist", gdt_script_system_function_file_exist, 0 );
	gdt_add_system_function( memory_pool, script_munit, "file_size", gdt_script_system_function_file_size, 0 );
	gdt_add_system_function( memory_pool, script_munit, "file_extension", gdt_script_system_function_file_extension, 0 );
	gdt_add_system_function( memory_pool, script_munit, "file_get", gdt_script_system_function_file_get, 0 );
	gdt_add_system_function( memory_pool, script_munit, "file_put", gdt_script_system_function_file_put, 0 );
	gdt_add_system_function( memory_pool, script_munit, "file_add", gdt_script_system_function_file_add, 0 );
	gdt_add_system_function( memory_pool, script_munit, "json_encode", gdt_script_system_function_json_encode, 0 );
	gdt_add_system_function( memory_pool, script_munit, "json_decode", gdt_script_system_function_json_decode, 0 );
	gdt_add_system_function( memory_pool, script_munit, "gmtime", gdt_script_system_function_gmtime, 0 );
	gdt_import_script( memory_pool, &script_munit, (char*)script_file );
	gdt_exec( memory_pool, &script_munit );	
}
