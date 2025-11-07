/*
 * Copyright (c) Katsuya Owari
 */

#include "qs_hash.h"
#include "qs_variable.h"

int main(int argc, char *argv[])
{
    QS_MEMORY_POOL* memory = NULL;
	size_t size;
	if( (size = qs_initialize_memory_f64( &memory, 1024 * 1024 * 8) ) <= 0 ){
		return -1;
	}

    int32_t memid_hash = qs_create_hash( memory, 8 );

    qs_add_hash_integer( memory, memid_hash, "key1", 12345 );
    qs_add_hash_big_integer( memory, memid_hash, "key2", 12345678901234567 );
    qs_add_hash_unsigned_big_integer( memory, memid_hash, "key3", 18446744073709551615ULL );

    qs_hash_dump( memory, memid_hash, 0 );
    
    qs_free(memory);
    return 0;
}
