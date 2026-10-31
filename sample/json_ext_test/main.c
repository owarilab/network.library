/*
 * Copyright (c) Katsuya Owari
 *
 * json_ext_test
 *   高レベルJSON API（api_qs_object_* / api_qs_array_*）の未カバー機能をテストする。
 *
 * 対象API（既存 json サンプルで未カバー）:
 *   api_qs_object_exist / api_qs_object_get_integer_val
 *   api_qs_object_get_object / api_qs_object_get_array
 *   api_qs_array_push_integer / api_qs_array_push_array
 *   api_qs_array_get_array / api_qs_array_get_object
 *   api_qs_object_get_keys
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

    /* --- api_qs_object_exist --- */
    {
        QS_JSON_ELEMENT_OBJECT obj;
        api_qs_object_create(&memory_context, &obj);
        api_qs_object_push_integer(&obj, "key1", 42);
        api_qs_object_push_string(&obj, "key2", "hello");

        check("object_exist key1", api_qs_object_exist(&obj, "key1") == 1);
        check("object_exist key2", api_qs_object_exist(&obj, "key2") == 1);
        check("object_exist missing key", api_qs_object_exist(&obj, "key3") == 0);
    }

    /* --- api_qs_object_get_integer_val --- */
    {
        QS_JSON_ELEMENT_OBJECT obj;
        api_qs_object_create(&memory_context, &obj);
        api_qs_object_push_integer(&obj, "num", 12345);
        api_qs_object_push_big_integer(&obj, "bignum", 9999999999LL);

        int32_t val = api_qs_object_get_integer_val(&obj, "num");
        check("object_get_integer_val", val == 12345);

        int64_t bval = api_qs_object_get_big_integer_val(&obj, "bignum");
        check("object_get_big_integer_val", bval == 9999999999LL);
    }

    /* --- api_qs_object_get_object (nested object) --- */
    {
        QS_JSON_ELEMENT_OBJECT parent;
        QS_JSON_ELEMENT_OBJECT child;
        api_qs_object_create(&memory_context, &parent);
        api_qs_object_create(&memory_context, &child);

        api_qs_object_push_integer(&child, "inner", 77);
        api_qs_object_push_object(&parent, "child", &child);

        QS_JSON_ELEMENT_OBJECT dst;
        int rc = api_qs_object_get_object(&parent, "child", &dst);
        check("object_get_object returns 0", rc == 0);

        int32_t inner = api_qs_object_get_integer_val(&dst, "inner");
        check("nested object inner value", inner == 77);

        /* 存在しないキー */
        int rc2 = api_qs_object_get_object(&parent, "no_such", &dst);
        check("object_get_object missing returns non-zero", rc2 != 0);
    }

    /* --- api_qs_object_get_array (object -> array) --- */
    {
        QS_JSON_ELEMENT_OBJECT obj;
        QS_JSON_ELEMENT_ARRAY arr;
        api_qs_object_create(&memory_context, &obj);
        api_qs_array_create(&memory_context, &arr);

        api_qs_array_push_integer(&arr, 10);
        api_qs_array_push_integer(&arr, 20);
        api_qs_array_push_integer(&arr, 30);
        api_qs_object_push_array(&obj, "list", &arr);

        QS_JSON_ELEMENT_ARRAY dst;
        int rc = api_qs_object_get_array(&obj, "list", &dst);
        check("object_get_array returns 0", rc == 0);

        int32_t len = api_qs_array_get_length(&dst);
        check("object_get_array length is 3", len == 3);

        int32_t* v0 = api_qs_array_get_integer(&dst, 0);
        check("object_get_array[0] == 10", v0 != NULL && *v0 == 10);

        int32_t* v2 = api_qs_array_get_integer(&dst, 2);
        check("object_get_array[2] == 30", v2 != NULL && *v2 == 30);
    }

    /* --- api_qs_array_push_integer / api_qs_array_push_array --- */
    {
        QS_JSON_ELEMENT_ARRAY outer;
        QS_JSON_ELEMENT_ARRAY inner;
        api_qs_array_create(&memory_context, &outer);
        api_qs_array_create(&memory_context, &inner);

        api_qs_array_push_integer(&outer, 1);
        api_qs_array_push_integer(&outer, 2);
        api_qs_array_push_array(&outer, &inner);

        int32_t len = api_qs_array_get_length(&outer);
        check("array_push_integer + push_array length is 3", len == 3);

        int32_t* v0 = api_qs_array_get_integer(&outer, 0);
        check("outer[0] == 1", v0 != NULL && *v0 == 1);

        int32_t* v1 = api_qs_array_get_integer(&outer, 1);
        check("outer[1] == 2", v1 != NULL && *v1 == 2);
    }

    /* --- api_qs_array_get_array (nested array) --- */
    {
        QS_JSON_ELEMENT_ARRAY outer;
        QS_JSON_ELEMENT_ARRAY inner;
        api_qs_array_create(&memory_context, &outer);
        api_qs_array_create(&memory_context, &inner);

        api_qs_array_push_integer(&inner, 100);
        api_qs_array_push_integer(&inner, 200);
        api_qs_array_push_array(&outer, &inner);

        QS_JSON_ELEMENT_ARRAY dst;
        int rc = api_qs_array_get_array(&outer, 0, &dst);
        check("array_get_array returns 0", rc == 0);

        int32_t len = api_qs_array_get_length(&dst);
        check("nested array length is 2", len == 2);

        int32_t* v0 = api_qs_array_get_integer(&dst, 0);
        check("nested array[0] == 100", v0 != NULL && *v0 == 100);
    }

    /* --- api_qs_array_get_object (array of objects) --- */
    {
        QS_JSON_ELEMENT_ARRAY arr;
        QS_JSON_ELEMENT_OBJECT obj1;
        QS_JSON_ELEMENT_OBJECT obj2;
        api_qs_array_create(&memory_context, &arr);
        api_qs_object_create(&memory_context, &obj1);
        api_qs_object_create(&memory_context, &obj2);

        api_qs_object_push_string(&obj1, "name", "first");
        api_qs_object_push_string(&obj2, "name", "second");
        api_qs_array_push_object(&arr, &obj1);
        api_qs_array_push_object(&arr, &obj2);

        QS_JSON_ELEMENT_OBJECT dst;
        int rc = api_qs_array_get_object(&arr, 0, &dst);
        check("array_get_object returns 0", rc == 0);

        char* name = api_qs_object_get_string(&dst, "name");
        check_str("array[0].name == first", name, "first");

        int rc2 = api_qs_array_get_object(&arr, 1, &dst);
        check("array_get_object[1] returns 0", rc2 == 0);
        name = api_qs_object_get_string(&dst, "name");
        check_str("array[1].name == second", name, "second");
    }

    /* --- api_qs_object_get_keys --- */
    {
        QS_JSON_ELEMENT_OBJECT obj;
        api_qs_object_create(&memory_context, &obj);
        api_qs_object_push_integer(&obj, "alpha", 1);
        api_qs_object_push_integer(&obj, "beta", 2);
        api_qs_object_push_integer(&obj, "gamma", 3);

        QS_JSON_ELEMENT_ARRAY keys;
        int rc = api_qs_object_get_keys(&obj, &keys);
        check("object_get_keys returns 0", rc == 0);

        int32_t len = api_qs_array_get_length(&keys);
        check("object_get_keys length is 3", len == 3);

        /* 全キーが取得できること */
        int found_alpha = 0, found_beta = 0, found_gamma = 0;
        for (int i = 0; i < len; i++) {
            char* k = api_qs_array_get_string(&keys, i);
            if (k != NULL) {
                if (strcmp(k, "alpha") == 0) found_alpha = 1;
                if (strcmp(k, "beta") == 0) found_beta = 1;
                if (strcmp(k, "gamma") == 0) found_gamma = 1;
            }
        }
        check("object_get_keys contains all keys", found_alpha && found_beta && found_gamma);
    }

    /* --- encode/decode round trip with nested structures --- */
    {
        QS_JSON_ELEMENT_OBJECT root;
        QS_JSON_ELEMENT_OBJECT nested;
        QS_JSON_ELEMENT_ARRAY arr;
        api_qs_object_create(&memory_context, &root);
        api_qs_object_create(&memory_context, &nested);
        api_qs_array_create(&memory_context, &arr);

        api_qs_object_push_integer(&nested, "x", 111);
        api_qs_array_push_integer(&arr, 1);
        api_qs_array_push_integer(&arr, 2);
        api_qs_object_push_object(&root, "nested", &nested);
        api_qs_object_push_array(&root, "arr", &arr);
        api_qs_object_push_string(&root, "msg", "roundtrip");

        char* json = api_qs_json_encode_object(&root, 1024 * 1024);
        check("encode nested object not NULL", json != NULL);

        QS_JSON_ELEMENT_OBJECT decoded;
        int rc = api_qs_json_decode_object(&memory_context, &decoded, json);
        check("decode nested object returns 0", rc == 0);

        check_str("decoded msg", api_qs_object_get_string(&decoded, "msg"), "roundtrip");

        QS_JSON_ELEMENT_OBJECT d_nested;
        api_qs_object_get_object(&decoded, "nested", &d_nested);
        check("decoded nested.x == 111", api_qs_object_get_integer_val(&d_nested, "x") == 111);

        QS_JSON_ELEMENT_ARRAY d_arr;
        api_qs_object_get_array(&decoded, "arr", &d_arr);
        check("decoded arr length is 2", api_qs_array_get_length(&d_arr) == 2);
        int32_t* av = api_qs_array_get_integer(&d_arr, 1);
        check("decoded arr[1] == 2", av != NULL && *av == 2);
    }

    api_qs_memory_free(&memory_context);

    printf("\n=== json_ext_test summary ===\n");
    printf("pass: %d, fail: %d\n", pass_count, fail_count);
    return (fail_count > 0) ? 1 : 0;
}
