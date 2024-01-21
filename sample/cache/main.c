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
    if(-1==api_qs_memory_alloc(&context, 1024 * 1024 * 16))
    {
        printf("api_qs_memory_alloc failed\n");
        return -1;
    }
    api_qs_memory_info(&context);

    QS_KVS_CONTEXT kvs;
    if(-1==api_qs_kvs_create_b1mb(&context, &kvs))
    {
        printf("api_qs_kvs_create failed\n");
        return -1;
    }

    // add test data
    {
        int i;
        for(i=0;i<1000000;i++)
        {
            char* key = api_qs_uniqid(&context,32);
            char* value = api_qs_uniqid(&context,32);

            printf("%zu\n",api_qs_memory_available_size(&context));

            if(api_qs_memory_available_size(&context) < 1024 * 1024 * 8){
                printf("api_qs_memory_available_size warning i:%d , available memory:%zu\n", i , api_qs_memory_available_size(&context));
                break;
            }

            if(NULL==key || NULL==value)
            {
                printf("api_qs_uniqid failed\n");
                return -1;
            }
            api_qs_kvs_set(&kvs, key, value, 0);
        }
    }

    // get sorted keys
    QS_JSON_ELEMENT_ARRAY keys;
    api_qs_array_create(&context, &keys);
    int len = api_qs_kvs_sorted_keys(&keys, &kvs, 1);

    // show keys
    int i;
    for(i=0;i<len;i++)
    {
        char* key = api_qs_array_get_string(&keys, i);
        char* value = api_qs_kvs_get(&kvs, key);
        printf("%s=%s\n", key, value);
    }

    printf("len:%d\n", len);

    api_qs_memory_free(&context);
    return 0;
}
