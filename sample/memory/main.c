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

#include "gdt_memory_allocator.h"
int main( int argc, char *argv[], char *envp[] )
{
	GDT_MEMORY_POOL* memory_pool = NULL;
	do{
		if( 0 >= gdt_initialize_memory( 
			&memory_pool,
			SIZE_KBYTE * 8,
			SIZE_KBYTE * 8,
			MEMORY_ALIGNMENT_SIZE_BIT_64,
			FIX_MUNIT_SIZE,
			1,
			SIZE_KBYTE * 8 ) 
		){
			printf( "gdt_initialize_memory error\n" );
			break;
		}
		int32_t mem_id1 = gdt_create_munit( memory_pool, SIZE_KBYTE, MEMORY_TYPE_DEFAULT );
		int32_t mem_id2 = gdt_create_munit( memory_pool, SIZE_KBYTE, MEMORY_TYPE_DEFAULT );
		uint8_t* ptr1 = gdt_upointer( memory_pool, mem_id1 );
		uint8_t* ptr2 = gdt_upointer( memory_pool, mem_id2 );
		printf("ptr1 = %p, ptr2 = %p\n",ptr1, ptr2);
		gdt_memory_info( memory_pool );
		gdt_free( memory_pool );
	}while( false );
	return 0;
}
