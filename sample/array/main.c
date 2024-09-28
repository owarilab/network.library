// qs_csv sample

#include "qs_array.h"
#include "qs_variable.h"

int main(int argc, char *argv[])
{
    QS_MEMORY_POOL* memory = NULL;
	size_t size;
	if( (size = qs_initialize_memory_f64( &memory, 1024 * 1024 * 8) ) <= 0 ){
		return -1;
	}

    int32_t memid_array = qs_create_array( memory, 8 );
    if( memid_array == -1 ){
        return -1;
    }

    if(-1 == qs_array_push_string( memory, &memid_array, "Hello, World!" )) { return -1; }
    if(-1 == qs_array_push_integer( memory, &memid_array, 12345 )) { return -1; }
    if(-1 == qs_array_push_empty_string( memory, &memid_array, 1024 )) { return -1; }
    if(-1 == qs_array_push_big_integer( memory, &memid_array, 12345678901234567 )) { return -1; }

    QS_ARRAY_ELEMENT* elm_str = qs_array_get( memory, memid_array, 0 ); // array index 0
    if( elm_str == NULL ){
        return -1;
    }
    printf("elm_str = %s\n", (char*)QS_GET_POINTER( memory, elm_str->memid_array_element_data ));

    QS_ARRAY_ELEMENT* elm_int64 = qs_array_get( memory, memid_array, 3 ); // array index 3
    if( elm_int64 == NULL ){
        return -1;
    }

    int64_t v = QS_INT64( memory, elm_int64->memid_array_element_data );
    printf("v = %ld\n", v);

    int64_t* pv = QS_PINT64( memory, elm_int64->memid_array_element_data );
    printf("*pv = %ld\n", *pv);

    printf("v string = %s\n", (char*)QS_GET_POINTER( memory, elm_int64->memid_array_element_data ));

    qs_array_dump( memory, memid_array, 0 );

    qs_free(memory);
    return 0;
}
