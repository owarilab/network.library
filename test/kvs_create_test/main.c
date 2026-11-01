/*
 * Copyright (c) Katsuya Owari
 *
 * kvs_create_test
 *   高レベルKVS作成API（api_qs_kvs_create_b*mb）の全9サイズをテストする。
 *
 * 各サイズで検証する内容:
 *   - 作成APIが 0 を返すこと
 *   - api_qs_kvs_set / api_qs_kvs_get のラウンドトリップ
 *   - api_qs_kvs_key_length
 *   - api_qs_kvs_sorted_keys の件数が key_length と一致すること
 *   - api_qs_kvs_delete 後に get が NULL になること
 *
 * Build: make
 * Run:   ./qs_kvs_create_test
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "qs_api.h"

typedef int (*QS_KVS_CREATE_FN)(QS_MEMORY_CONTEXT* memory_context, QS_KVS_CONTEXT* kvs_context);

static int g_pass = 0;
static int g_fail = 0;

static void check(const char* label, int ok)
{
    if (ok) {
        printf("[PASS] %s\n", label);
        g_pass++;
    } else {
        printf("[FAIL] %s\n", label);
        g_fail++;
    }
}

static void check_str(const char* label, const char* got, const char* expected)
{
    if (got && expected && strcmp(got, expected) == 0) {
        printf("[PASS] %s\n", label);
        g_pass++;
    } else {
        printf("[FAIL] %s : expected=\"%s\" got=\"%s\"\n",
               label, expected ? expected : "(null)", got ? got : "(null)");
        g_fail++;
    }
}

static int run_kvs_test(int size_mb, QS_KVS_CREATE_FN create_fn)
{
    size_t alloc_size = (size_t)(1024 * 1024) * (size_t)(size_mb + 16);
    int entry_cap = size_mb * 12345;
    if(entry_cap > 10000){ entry_cap = 10000; }

    QS_MEMORY_CONTEXT context;
    if(-1==api_qs_memory_alloc(&context, alloc_size))
    {
        printf("api_qs_memory_alloc failed\n");
        return -1;
    }

    QS_MEMORY_CONTEXT temporary_memory_context;
    if(-1==api_qs_memory_alloc(&temporary_memory_context, 1024 * 1024 * 64))
    {
        printf("api_qs_memory_alloc temporary failed\n");
        api_qs_memory_free(&context);
        return -1;
    }

    QS_KVS_CONTEXT kvs;
    check("create returns 0", 0==create_fn(&context, &kvs));
    if(-1==kvs.memid_kvs)
    {
        api_qs_memory_free(&temporary_memory_context);
        api_qs_memory_free(&context);
        return -1;
    }

    // add test data
    {
        int i;
        int set_ok = 1;
        for(i=0;i<entry_cap;i++)
        {
            char* key = api_qs_uniqid(&temporary_memory_context,32);
            char* value = api_qs_uniqid(&temporary_memory_context,32);

            if(NULL==key || NULL==value)
            {
                set_ok = 0;
                break;
            }

            if(-1==api_qs_kvs_set(&kvs, key, value, 0))
            {
                set_ok = 0;
                break;
            }

            if(api_qs_memory_available_size(&temporary_memory_context) < 1024 * 1024 * 1){
                api_qs_memory_clean(&temporary_memory_context);
            }
        }
        check("bulk set", set_ok);
    }

    api_qs_memory_clean(&temporary_memory_context);

    // known keys で set/get/delete を検証
    {
        char key0[32], key1[32], key2[32];
        char val0[32], val1[32], val2[32];
        snprintf(key0, sizeof(key0), "kvs-test-key-0");
        snprintf(key1, sizeof(key1), "kvs-test-key-1");
        snprintf(key2, sizeof(key2), "kvs-test-key-2");
        snprintf(val0, sizeof(val0), "kvs-test-val-0");
        snprintf(val1, sizeof(val1), "kvs-test-val-1");
        snprintf(val2, sizeof(val2), "kvs-test-val-2");

        check("set key0", 0==api_qs_kvs_set(&kvs, key0, val0, 0));
        check("set key1", 0==api_qs_kvs_set(&kvs, key1, val1, 0));
        check("set key2", 0==api_qs_kvs_set(&kvs, key2, val2, 0));

        check_str("get key0", api_qs_kvs_get(&kvs, key0), val0);
        check_str("get key1", api_qs_kvs_get(&kvs, key1), val1);
        check_str("get key2", api_qs_kvs_get(&kvs, key2), val2);
        check("get missing returns NULL", NULL==api_qs_kvs_get(&kvs, "kvs-test-missing"));

        int32_t key_length = api_qs_kvs_key_length(&kvs);
        check("key_length >= 3", key_length >= 3);

        // sorted_keys の件数が key_length と一致すること
        {
            QS_JSON_ELEMENT_ARRAY keys;
            if(-1==api_qs_array_create(&temporary_memory_context, &keys))
            {
                api_qs_memory_free(&temporary_memory_context);
                api_qs_memory_free(&context);
                return -1;
            }
            int32_t len = api_qs_kvs_sorted_keys(&keys, &kvs, 1);
            check("sorted_keys count == key_length", len == key_length);
        }

        // delete 後に get が NULL になること
        check("delete key0", 0==api_qs_kvs_delete(&kvs, key0));
        check("get key0 after delete returns NULL", NULL==api_qs_kvs_get(&kvs, key0));
        check_str("get key1 after delete intact", api_qs_kvs_get(&kvs, key1), val1);
        check("key_length decreased", api_qs_kvs_key_length(&kvs) == key_length - 1);

        api_qs_memory_clean(&temporary_memory_context);
    }

    api_qs_memory_free(&temporary_memory_context);
    api_qs_memory_free(&context);
    return 0;
}

int main(int argc, char *argv[])
{
    api_qs_init();

    if(-1==run_kvs_test(1, api_qs_kvs_create_b1mb)){
        return -1;
    }
    if(-1==run_kvs_test(8, api_qs_kvs_create_b8mb)){
        return -1;
    }
    if(-1==run_kvs_test(16, api_qs_kvs_create_b16mb)){
        return -1;
    }
    if(-1==run_kvs_test(32, api_qs_kvs_create_b32mb)){
        return -1;
    }
    if(-1==run_kvs_test(64, api_qs_kvs_create_b64mb)){
        return -1;
    }
    if(-1==run_kvs_test(128, api_qs_kvs_create_b128mb)){
        return -1;
    }
    if(-1==run_kvs_test(256, api_qs_kvs_create_b256mb)){
        return -1;
    }
    if(-1==run_kvs_test(512, api_qs_kvs_create_b512mb)){
        return -1;
    }
    if(-1==run_kvs_test(1024, api_qs_kvs_create_b1024mb)){
        return -1;
    }

    printf("\n=== kvs_create_test summary ===\n");
    printf("pass: %d, fail: %d\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}
