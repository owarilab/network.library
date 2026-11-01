/*
 * Copyright (c) Katsuya Owari
 *
 * kvs_stress_test
 *   高レベルKVS（api_qs_kvs_create_b1mb）への100万エントリ投入をテストする。
 *
 * 検証する内容:
 *   - 100万件の api_qs_kvs_set が成功すること
 *   - api_qs_kvs_key_length が >0 であること
 *     （b1mb は LRU キャッシュのため 100万件は保持できず、
 *      古いエントリは退避される）
 *   - api_qs_kvs_sorted_keys の件数が key_length と一致すること
 *   - sorted_keys のキーの api_qs_kvs_get ラウンドトリップ
 *
 * Build: make
 * Run:   ./qs_kvs_stress_test
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#include "qs_api.h"

#define STRESS_ENTRY_COUNT 1000000
#define SAVE_KEY_COUNT 3

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
        api_qs_memory_free(&context);
        return -1;
    }

    QS_KVS_CONTEXT kvs;
    check("kvs_create_b1mb returns 0", 0==api_qs_kvs_create_b1mb(&context, &kvs));
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

        for(i=0;i<STRESS_ENTRY_COUNT;i++)
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
        check("bulk set 1000000 entries", set_ok);
    }

    // key_length / sorted_keys / get roundtrip
    // 注: b1mb は LRU キャッシュのため 100万件は保持できず、
    // 古いエントリは退避される。したがって key_length は
    // 保持上限（数百〜数千件）になる。ここでは「>0 かつ
    // sorted_keys と一致」を検証し、get は sorted_keys から
    // 取得したキー（キャッシュに存在保証あり）で行う。
    {
        int32_t key_length = api_qs_kvs_key_length(&kvs);
        check("key_length > 0", key_length > 0);

        // get sorted keys
        QS_JSON_ELEMENT_ARRAY keys;
        if(-1==api_qs_array_create(&context, &keys))
        {
            api_qs_memory_free(&temporary_memory_context);
            api_qs_memory_free(&context);
            return -1;
        }
        int32_t len = api_qs_kvs_sorted_keys(&keys, &kvs, 1);
        check("sorted_keys count == key_length", len == key_length);

        // get roundtrip（sorted_keys の先頭数件）
        {
            int i;
            int max_check = (len < SAVE_KEY_COUNT) ? len : SAVE_KEY_COUNT;
            for(i=0;i<max_check;i++)
            {
                char* key = api_qs_array_get_string(&keys, i);
                char* value = api_qs_kvs_get(&kvs, key);
                char label[64];
                snprintf(label, sizeof(label), "get roundtrip sorted key[%d]", i);
                check(label, key != NULL && value != NULL);
            }
        }
    }

    printf("\n=== kvs_stress_test summary ===\n");
    printf("pass: %d, fail: %d\n", g_pass, g_fail);

    api_qs_memory_free(&context);
    api_qs_memory_free(&temporary_memory_context);
    return (g_fail == 0) ? 0 : 1;
}
