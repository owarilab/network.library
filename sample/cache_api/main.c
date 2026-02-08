/*
 * Copyright (c) Katsuya Owari
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "qs_api.h"

typedef int (*QS_KVS_CREATE_FN)(QS_MEMORY_CONTEXT* memory_context, QS_KVS_CONTEXT* kvs_context);

static int run_kvs_test(int size_mb, const char* label, QS_KVS_CREATE_FN create_fn)
{
    size_t alloc_size = (size_t)(1024 * 1024) * (size_t)(size_mb + 16);
    int entry_cap = size_mb * 12345;
    //if(entry_cap > 300000){ entry_cap = 300000; }

    QS_MEMORY_CONTEXT context;
    if(-1==api_qs_memory_alloc(&context, alloc_size))
    {
        printf("api_qs_memory_alloc failed (%s)\n", label);
        return -1;
    }

    QS_MEMORY_CONTEXT temporary_memory_context;
    if(-1==api_qs_memory_alloc(&temporary_memory_context, 1024 * 1024 * 64))
    {
        printf("api_qs_memory_alloc temporary failed (%s)\n", label);
        api_qs_memory_free(&context);
        return -1;
    }

    QS_KVS_CONTEXT kvs;
    if(-1==create_fn(&context, &kvs))
    {
        printf("kvs create failed (%s)\n", label);
        api_qs_memory_free(&temporary_memory_context);
        api_qs_memory_free(&context);
        return -1;
    }

    printf("---- %s ----\n", label);
    api_qs_memory_info(&context);

    // add test data
    {
        int i;
        for(i=0;i<entry_cap;i++)
        {
            char* key = api_qs_uniqid(&temporary_memory_context,32);
            char* value = api_qs_uniqid(&temporary_memory_context,32);

            if(NULL==key || NULL==value)
            {
                printf("api_qs_uniqid failed (%s)\n", label);
                api_qs_memory_free(&temporary_memory_context);
                api_qs_memory_free(&context);
                return -1;
            }

            if(-1==api_qs_kvs_set(&kvs, key, value, 0))
            {
                printf("api_qs_kvs_set failed (%s) i:%d\n", label, i);
                api_qs_memory_free(&temporary_memory_context);
                api_qs_memory_free(&context);
                return -1;
            }

            if(api_qs_memory_available_size(&temporary_memory_context) < 1024 * 1024 * 1){
                api_qs_memory_clean(&temporary_memory_context);
            }
        }
    }

    api_qs_memory_clean(&temporary_memory_context);

    int is_keys = 0;

    if(is_keys)
    {
        QS_JSON_ELEMENT_ARRAY keys;
        if(-1==api_qs_array_create(&temporary_memory_context, &keys))
        {
            printf("api_qs_array_create failed (%s)\n", label);
            api_qs_memory_free(&temporary_memory_context);
            api_qs_memory_free(&context);
            return -1;
        }

        int len = api_qs_kvs_sorted_keys(&keys, &kvs, 1);
        printf("keys:%d\n", len);

        // read back a few values
        {
            int i;
            int max_show = (len < 3) ? len : 3;
            for(i=0;i<max_show;i++)
            {
                char* key = api_qs_array_get_string(&keys, i);
                char* value = api_qs_kvs_get(&kvs, key);
                printf("%s=%s\n", key, value);
            }
        }

        // delete a few keys and confirm size
        {
            int i;
            int delete_count = (len < 50) ? len : 50;
            for(i=0;i<delete_count;i++)
            {
                char* key = api_qs_array_get_string(&keys, i);
                api_qs_kvs_delete(&kvs, key);
            }
        }

        api_qs_memory_clean(&temporary_memory_context);

        QS_JSON_ELEMENT_ARRAY keys_after;
        if(-1==api_qs_array_create(&temporary_memory_context, &keys_after))
        {
            printf("api_qs_array_create failed (after) (%s)\n", label);
            api_qs_memory_free(&temporary_memory_context);
            api_qs_memory_free(&context);
            return -1;
        }
        int len_after = api_qs_kvs_keys(&keys_after, &kvs);
        printf("keys_after_delete:%d\n", len_after);
    }

    int32_t key_length = api_qs_kvs_key_length(&kvs);
    printf("key_length:%d\n", key_length);

    api_qs_memory_free(&temporary_memory_context);
    api_qs_memory_free(&context);
    return 0;
}

int main(int argc, char *argv[])
{
    api_qs_init();

    if(-1==run_kvs_test(1, "1MB", api_qs_kvs_create_b1mb)){
        return -1;
    }
    if(-1==run_kvs_test(8, "8MB", api_qs_kvs_create_b8mb)){
        return -1;
    }
    if(-1==run_kvs_test(16, "16MB", api_qs_kvs_create_b16mb)){
        return -1;
    }
    if(-1==run_kvs_test(32, "32MB", api_qs_kvs_create_b32mb)){
        return -1;
    }
    if(-1==run_kvs_test(64, "64MB", api_qs_kvs_create_b64mb)){
        return -1;
    }
    if(-1==run_kvs_test(128, "128MB", api_qs_kvs_create_b128mb)){
        return -1;
    }
    if(-1==run_kvs_test(256, "256MB", api_qs_kvs_create_b256mb)){
        return -1;
    }
    if(-1==run_kvs_test(512, "512MB", api_qs_kvs_create_b512mb)){
        return -1;
    }
    if(-1==run_kvs_test(1024, "1024MB", api_qs_kvs_create_b1024mb)){
        return -1;
    }

    return 0;
}
