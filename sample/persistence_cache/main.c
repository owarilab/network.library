/*
 * Copyright (c) Katsuya Owari
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <time.h>

#include "qs_api.h"

int main(int argc, char *argv[])
{
    api_qs_init();

    QS_MEMORY_CONTEXT context;
    if(-1==api_qs_memory_alloc(&context, 1024 * 1024 * 16))
    {
        printf("api_qs_memory_alloc failed\n");
        return -1;
    }

    QS_KVS_CONTEXT kvs;
    
    if(-1==api_qs_kvs_create_b8mb_persistence(&kvs, "./kvs.db"))
    {
        printf("api_qs_kvs_create failed\n");
        return -1;
    }

    // add test data
    {
        int i;
        for(i=0;i<10000;i++)
        {
            char* key = api_qs_uniqid(&context,32);
            char* value = api_qs_uniqid(&context,2048);

            if(NULL==key || NULL==value)
            {
                printf("api_qs_uniqid failed\n");
                return -1;
            }

            if(-1==api_qs_kvs_set(&kvs, key, value, 0))
            {
                printf("api_qs_kvs_set memory full\n");
                break;
            }

            printf("%zu\n",api_qs_memory_available_size(&context));
            if(api_qs_memory_available_size(&context) < 1024 * 1024 * 8){
                printf("api_qs_memory_available_size warning i:%d , available memory:%zu\n", i , api_qs_memory_available_size(&context));
                api_qs_memory_clean(&context);
            }
        }
    }

    // get sorted keys
    printf("api_qs_kvs_sorted_keys\n");
    QS_JSON_ELEMENT_ARRAY keys;
    api_qs_array_create(&context, &keys);
    
    clock_t start = clock();
    
    int len = api_qs_kvs_sorted_keys(&keys, &kvs, 1);
    
    // show keys
    if(0)
    {
        printf("keys:\n");
        int i;
        for(i=0;i<len;i++)
        {
            char* key = api_qs_array_get_string(&keys, i);
            char* value = api_qs_kvs_get(&kvs, key);
            printf("%s=%s\n", key, value);
        }
    }
    
    clock_t end = clock();
    
    printf("len:%d\n", len);
    
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Execution time: %f seconds\n", time_spent);

    api_qs_memory_free(&context);
    api_qs_persistence_kvs_memory_free(&kvs);
    return 0;
}
