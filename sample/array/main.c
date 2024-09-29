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
    if(-1 == qs_array_push_unsigned_big_integer( memory, &memid_array, 18446744073709551615ULL )) { return -1; }

    QS_ARRAY_ELEMENT* elm_str = qs_array_get( memory, memid_array, 0 ); // array index 0
    if( elm_str == NULL ){
        return -1;
    }
    printf("elm_str = %s\n", (char*)QS_GET_POINTER( memory, elm_str->memid_array_element_data ));
    printf("\n");

    // array index 1
    {
        QS_ARRAY_ELEMENT* elm_int32 = qs_array_get( memory, memid_array, 1 );
        if( elm_int32 == NULL ){
            return -1;
        }

        int32_t iv = QS_INT32( memory, elm_int32->memid_array_element_data );
        printf("iv = %d\n", iv);

        int32_t* piv = QS_PINT32( memory, elm_int32->memid_array_element_data );
        printf("*piv = %d\n", *piv);

        printf("iv string = %s\n", (char*)QS_GET_POINTER( memory, elm_int32->memid_array_element_data ));
        printf("\n");
    }

    // array index 2
    {
        QS_ARRAY_ELEMENT* elm_str = qs_array_get( memory, memid_array, 2 );
        if( elm_str == NULL ){
            return -1;
        }
        // print qs_usize( memory, elm_str->memid_array_element_data );
        printf("elm_str size = %lu\n", qs_usize( memory, elm_str->memid_array_element_data ));
        printf("elm_str = %s\n", (char*)QS_GET_POINTER( memory, elm_str->memid_array_element_data ));
        printf("\n");
    }

    // array index 3
    {
        QS_ARRAY_ELEMENT* elm_int64 = qs_array_get( memory, memid_array, 3 ); 
        if( elm_int64 == NULL ){
            return -1;
        }

        int64_t lv = QS_INT64( memory, elm_int64->memid_array_element_data );
        printf("lv = %ld\n", lv);

        int64_t* plv = QS_PINT64( memory, elm_int64->memid_array_element_data );
        printf("*plv = %ld\n", *plv);

        printf("lv string = %s\n", (char*)QS_GET_POINTER( memory, elm_int64->memid_array_element_data ));
        printf("\n");
    }

    // array index 4
    {
        QS_ARRAY_ELEMENT* elm_uint64 = qs_array_get( memory, memid_array, 4 );
        if( elm_uint64 == NULL ){
            return -1;
        }

        uint64_t uv = QS_UINT64( memory, elm_uint64->memid_array_element_data );
        printf("uv = %lu\n", uv);

        uint64_t* puv = QS_PUINT64( memory, elm_uint64->memid_array_element_data );
        printf("*puv = %lu\n", *puv);

        printf("uv string = %s\n", (char*)QS_GET_POINTER( memory, elm_uint64->memid_array_element_data ));
        printf("\n");
    }

    qs_array_dump( memory, memid_array, 0 );

    qs_free(memory);
    return 0;
}
