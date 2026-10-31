/*
 * Copyright (c) Katsuya Owari
 *
 * kvs_ext_test
 *   高レベルKVS API（api_qs_kvs_*）の未カバー機能をテストする。
 *
 * 対象API（既存 cache / cache_api サンプルで未カバー）:
 *   api_qs_kvs_get_buffer_size
 *   api_qs_kvs_sorted_keys（降順 is_sort_asc=0）
 *   api_qs_kvs_keys
 *   api_qs_kvs_key_length
 *   api_qs_kvs_set / api_qs_kvs_get / api_qs_kvs_delete（pass/fail 方式での検証）
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "qs_api.h"

static int pass_count = 0;
static int fail_count = 0;

static void check(const char* name, int ok)
{
    if (ok) {
        pass_count++;
        printf("[PASS] %s\n", name);
    } else {
        fail_count++;
        printf("[FAIL] %s\n", name);
    }
}

static void check_str(const char* name, const char* actual, const char* expected)
{
    int ok = (actual != NULL) && (expected != NULL) && (strcmp(actual, expected) == 0);
    if (!ok) {
        printf("       expected: \"%s\"\n       actual:   \"%s\"\n",
               expected ? expected : "(null)", actual ? actual : "(null)");
    }
    check(name, ok);
}

int main(void)
{
    api_qs_init();
    QS_MEMORY_CONTEXT memory_context;
    api_qs_memory_alloc(&memory_context, 1024 * 1024 * 8);

    QS_KVS_CONTEXT kvs;
    int rc = api_qs_kvs_create_b1mb(&memory_context, &kvs);
    check("kvs_create_b1mb returns 0", rc == 0);

    /* --- set / get --- */
    rc = api_qs_kvs_set(&kvs, "key1", "value1", 0);
    check("kvs_set key1 returns 0", rc == 0);

    rc = api_qs_kvs_set(&kvs, "key2", "value2", 0);
    check("kvs_set key2 returns 0", rc == 0);

    rc = api_qs_kvs_set(&kvs, "key3", "value3", 0);
    check("kvs_set key3 returns 0", rc == 0);

    char* v1 = api_qs_kvs_get(&kvs, "key1");
    check_str("kvs_get key1", v1, "value1");

    char* v2 = api_qs_kvs_get(&kvs, "key2");
    check_str("kvs_get key2", v2, "value2");

    /* --- get missing key --- */
    char* missing = api_qs_kvs_get(&kvs, "no_such_key");
    check("kvs_get missing returns NULL", missing == NULL);

    /* --- api_qs_kvs_get_buffer_size (未カバーAPI) ---
       注: qs_usize はアラインメント後のメモリブロックサイズを返すため、
       strlen(value) 以上であることを確認する。 */
    size_t bs1 = api_qs_kvs_get_buffer_size(&kvs, "key1");
    check("kvs_get_buffer_size key1 >= 6", bs1 >= strlen("value1"));

    size_t bs2 = api_qs_kvs_get_buffer_size(&kvs, "key2");
    check("kvs_get_buffer_size key2 >= 6", bs2 >= strlen("value2"));

    size_t bs_missing = api_qs_kvs_get_buffer_size(&kvs, "no_such_key");
    check("kvs_get_buffer_size missing == 0", bs_missing == 0);

    /* --- api_qs_kvs_key_length --- */
    int32_t key_len = api_qs_kvs_key_length(&kvs);
    check("kvs_key_length == 3", key_len == 3);

    /* --- api_qs_kvs_keys --- */
    {
        QS_JSON_ELEMENT_ARRAY keys;
        api_qs_array_create(&memory_context, &keys);
        int32_t len = api_qs_kvs_keys(&keys, &kvs);
        check("kvs_keys returns 3", len == 3);

        int found_key1 = 0, found_key2 = 0, found_key3 = 0;
        for (int i = 0; i < len; i++) {
            char* k = api_qs_array_get_string(&keys, i);
            if (k != NULL) {
                if (strcmp(k, "key1") == 0) found_key1 = 1;
                if (strcmp(k, "key2") == 0) found_key2 = 1;
                if (strcmp(k, "key3") == 0) found_key3 = 1;
            }
        }
        check("kvs_keys contains all keys", found_key1 && found_key2 && found_key3);
    }

    /* --- api_qs_kvs_sorted_keys (昇順) --- */
    {
        QS_JSON_ELEMENT_ARRAY keys;
        api_qs_array_create(&memory_context, &keys);
        int32_t len = api_qs_kvs_sorted_keys(&keys, &kvs, 1);
        check("kvs_sorted_keys asc returns 3", len == 3);

        char* k0 = api_qs_array_get_string(&keys, 0);
        char* k1 = api_qs_array_get_string(&keys, 1);
        char* k2 = api_qs_array_get_string(&keys, 2);
        check_str("sorted asc [0] == key1", k0, "key1");
        check_str("sorted asc [1] == key2", k1, "key2");
        check_str("sorted asc [2] == key3", k2, "key3");
    }

    /* --- api_qs_kvs_sorted_keys (降順・未カバー) --- */
    {
        QS_JSON_ELEMENT_ARRAY keys;
        api_qs_array_create(&memory_context, &keys);
        int32_t len = api_qs_kvs_sorted_keys(&keys, &kvs, 0);
        check("kvs_sorted_keys desc returns 3", len == 3);

        char* k0 = api_qs_array_get_string(&keys, 0);
        char* k1 = api_qs_array_get_string(&keys, 1);
        char* k2 = api_qs_array_get_string(&keys, 2);
        check_str("sorted desc [0] == key3", k0, "key3");
        check_str("sorted desc [1] == key2", k1, "key2");
        check_str("sorted desc [2] == key1", k2, "key1");
    }

    /* --- overwrite value --- */
    rc = api_qs_kvs_set(&kvs, "key1", "updated", 0);
    check("kvs_set overwrite returns 0", rc == 0);
    char* v1_updated = api_qs_kvs_get(&kvs, "key1");
    check_str("kvs_get key1 after overwrite", v1_updated, "updated");
    size_t bs_updated = api_qs_kvs_get_buffer_size(&kvs, "key1");
    check("kvs_get_buffer_size after overwrite >= 7", bs_updated >= strlen("updated"));

    /* --- delete --- */
    rc = api_qs_kvs_delete(&kvs, "key2");
    check("kvs_delete key2 returns 0", rc == 0);

    char* v2_after = api_qs_kvs_get(&kvs, "key2");
    check("kvs_get key2 after delete returns NULL", v2_after == NULL);

    int32_t key_len_after = api_qs_kvs_key_length(&kvs);
    check("kvs_key_length after delete == 2", key_len_after == 2);

    /* --- delete missing key (qs_remove_hash は存在しない場合も QS_SYSTEM_OK を返す) --- */
    rc = api_qs_kvs_delete(&kvs, "no_such_key");
    check("kvs_delete missing returns 0", rc == 0);

    /* --- empty value --- */
    rc = api_qs_kvs_set(&kvs, "empty_key", "", 0);
    check("kvs_set empty value returns 0", rc == 0);
    char* v_empty = api_qs_kvs_get(&kvs, "empty_key");
    check("kvs_get empty value returns empty string", v_empty != NULL && strlen(v_empty) == 0);

    api_qs_memory_free(&memory_context);

    printf("\n=== kvs_ext_test summary ===\n");
    printf("pass: %d, fail: %d\n", pass_count, fail_count);
    return (fail_count > 0) ? 1 : 0;
}
