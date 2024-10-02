// qs_csv sample

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#include "qs_api.h"

int main(int argc, char *argv[])
{
    api_qs_init();

    QS_MEMORY_CONTEXT context;
    if(-1==api_qs_memory_alloc(&context, 1024 * 1024 * 128))
    {
        printf("api_qs_memory_alloc 1 failed\n");
        return -1;
    }

    QS_MEMORY_CONTEXT temporary_memory_context;
    if(-1==api_qs_memory_alloc(&temporary_memory_context, 1024 * 1024 * 16))
    {
        printf("api_qs_memory_alloc 2 failed\n");
        return -1;
    }

    QS_KVS_CONTEXT kvs;
    if(-1==api_qs_kvs_create_b1mb(&context, &kvs))
    {
        printf("api_qs_kvs_create failed\n");
        return -1;
    }

    api_qs_memory_info(&context);

    // add test data
    {
        int i;
        for(i=0;i<1000000;i++)
        {
            char* key = api_qs_uniqid(&temporary_memory_context,32);
            char* value = api_qs_uniqid(&temporary_memory_context,32);

            if(NULL==key || NULL==value)
            {
                printf("api_qs_uniqid failed\n");
                return -1;
            }

            api_qs_kvs_set(&kvs, key, value, 0);

            //printf("%zu\n",api_qs_memory_available_size(&temporary_memory_context));
            if(api_qs_memory_available_size(&temporary_memory_context) < 1024 * 1024 * 1){
                printf("api_qs_memory_available_size warning i:%d , available memory:%zu\n", i , api_qs_memory_available_size(&temporary_memory_context));
                api_qs_memory_clean(&temporary_memory_context);
            }
        }
    }

    // get sorted keys
    QS_JSON_ELEMENT_ARRAY keys;
    api_qs_array_create(&context, &keys);
    int len = api_qs_kvs_sorted_keys(&keys, &kvs, 1);

    // show keys
    if(0)
    {
        int i;
        for(i=0;i<len;i++)
        {
            char* key = api_qs_array_get_string(&keys, i);
            char* value = api_qs_kvs_get(&kvs, key);
            printf("%s=%s\n", key, value);
        }
    }

    printf("len:%d\n", len);

    api_qs_memory_free(&context);
    api_qs_memory_free(&temporary_memory_context);
    return 0;
}
