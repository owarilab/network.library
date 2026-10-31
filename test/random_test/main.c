/*
 * Copyright (c) Katsuya Owari
 * Random API test (api_qs_rand / api_qs_uniqid / qs_rand_32 / qs_rand_64 / qs_uniqid_r32 / qs_uniqid_r64)
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "qs_api.h"
#include "qs_random.h"

static int pass_count = 0;
static int fail_count = 0;

static void check(const char* name, int ok)
{
    if (ok) {
        printf("[PASS] %s\n", name);
        pass_count++;
    } else {
        printf("[FAIL] %s\n", name);
        fail_count++;
    }
}

/* api_qs_rand: 連続取得で値が変化すること（全値が同一でない） */
static void test_api_qs_rand_varies(void)
{
    uint32_t first = api_qs_rand();
    int all_same = 1;
    for (int i = 0; i < 1000; i++) {
        if (api_qs_rand() != first) {
            all_same = 0;
            break;
        }
    }
    check("api_qs_rand varies over 1000 calls", !all_same);
}

/* api_qs_rand: 0 のみが続かない（xorshift が 0 固定にならない） */
static void test_api_qs_rand_not_zero(void)
{
    int nonzero = 0;
    for (int i = 0; i < 1000; i++) {
        if (api_qs_rand() != 0) {
            nonzero++;
        }
    }
    check("api_qs_rand produces nonzero values", nonzero > 900);
}

/* qs_rand_32: シード固定で再現性があること */
static void test_qs_rand_32_reproducible(void)
{
    qs_xorshift_32_seed = 0x12345678;
    uint32_t seq1[10];
    for (int i = 0; i < 10; i++) seq1[i] = qs_rand_32();

    qs_xorshift_32_seed = 0x12345678;
    uint32_t seq2[10];
    for (int i = 0; i < 10; i++) seq2[i] = qs_rand_32();

    check("qs_rand_32 reproducible with fixed seed", memcmp(seq1, seq2, sizeof(seq1)) == 0);
}

/* qs_rand_32: 異なるシードで異なる列になること */
static void test_qs_rand_32_different_seed(void)
{
    qs_xorshift_32_seed = 1;
    uint32_t a = qs_rand_32();
    qs_xorshift_32_seed = 2;
    uint32_t b = qs_rand_32();
    check("qs_rand_32 different seeds give different values", a != b);
}

/* qs_srand_32: 呼び出し後シードが 0 にならないこと（0 だと列が 0 固定） */
static void test_qs_srand_32(void)
{
    qs_srand_32();
    check("qs_srand_32 sets nonzero seed", qs_xorshift_32_seed != 0);
}

/* qs_rand_64: シード固定で再現性があること */
static void test_qs_rand_64_reproducible(void)
{
    qs_xorshift_64_seed2 = 0x1234567890ABCDEFULL;
    uint64_t seq1[10];
    for (int i = 0; i < 10; i++) seq1[i] = qs_rand_64();

    qs_xorshift_64_seed2 = 0x1234567890ABCDEFULL;
    uint64_t seq2[10];
    for (int i = 0; i < 10; i++) seq2[i] = qs_rand_64();

    check("qs_rand_64 reproducible with fixed seed", memcmp(seq1, seq2, sizeof(seq1)) == 0);
}

/* qs_rand_64: 連続取得で値が変化すること */
static void test_qs_rand_64_varies(void)
{
    uint64_t first = qs_rand_64();
    int all_same = 1;
    for (int i = 0; i < 1000; i++) {
        if (qs_rand_64() != first) {
            all_same = 0;
            break;
        }
    }
    check("qs_rand_64 varies over 1000 calls", !all_same);
}

/* qs_srand_64: 呼び出し後シードが 0 にならないこと */
static void test_qs_srand_64(void)
{
    qs_srand_64();
    check("qs_srand_64 sets nonzero seed", qs_xorshift_64_seed2 != 0);
}

/* qs_uniqid_r32: 指定長分の文字列が生成され NUL 終了していること */
static void test_qs_uniqid_r32_length(void)
{
    char buf[64];
    memset(buf, 'X', sizeof(buf));
    qs_uniqid_r32(buf, 32);
    check("qs_uniqid_r32 length is 32", strlen(buf) == 32);
    check("qs_uniqid_r32 null terminated", buf[32] == '\0');

    int valid_chars = 1;
    for (int i = 0; i < 32; i++) {
        char c = buf[i];
        if (!( (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') )) {
            valid_chars = 0;
            break;
        }
    }
    check("qs_uniqid_r32 contains only [0-9a-zA-Z]", valid_chars);
}

/* qs_uniqid_r32: 連続生成で同一IDがでないこと（1000件） */
static void test_qs_uniqid_r32_unique(void)
{
    char prev[64];
    qs_uniqid_r32(prev, 32);
    int all_same = 1;
    for (int i = 0; i < 1000; i++) {
        char cur[64];
        qs_uniqid_r32(cur, 32);
        if (strcmp(prev, cur) != 0) {
            all_same = 0;
            break;
        }
        memcpy(prev, cur, sizeof(cur));
    }
    check("qs_uniqid_r32 generates different ids", !all_same);
}

/* qs_uniqid_r64: 指定長分の文字列が生成され NUL 終了していること */
static void test_qs_uniqid_r64_length(void)
{
    char buf[64];
    memset(buf, 'X', sizeof(buf));
    qs_uniqid_r64(buf, 32);
    check("qs_uniqid_r64 length is 32", strlen(buf) == 32);
    check("qs_uniqid_r64 null terminated", buf[32] == '\0');

    int valid_chars = 1;
    for (int i = 0; i < 32; i++) {
        char c = buf[i];
        if (!( (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') )) {
            valid_chars = 0;
            break;
        }
    }
    check("qs_uniqid_r64 contains only [0-9a-zA-Z]", valid_chars);
}

/* qs_uniqid_r64: 連続生成で同一IDがでないこと（1000件） */
static void test_qs_uniqid_r64_unique(void)
{
    char prev[64];
    qs_uniqid_r64(prev, 32);
    int all_same = 1;
    for (int i = 0; i < 1000; i++) {
        char cur[64];
        qs_uniqid_r64(cur, 32);
        if (strcmp(prev, cur) != 0) {
            all_same = 0;
            break;
        }
        memcpy(prev, cur, sizeof(cur));
    }
    check("qs_uniqid_r64 generates different ids", !all_same);
}

/* api_qs_uniqid: メモリプールから指定長のIDが返ること */
static void test_api_qs_uniqid(QS_MEMORY_CONTEXT* mem, int32_t length)
{
    char* id = api_qs_uniqid(mem, length);
    int ok = (id != NULL) && (strlen(id) == (size_t)length);
    if (ok) {
        int valid_chars = 1;
        for (int i = 0; i < length; i++) {
            char c = id[i];
            if (!( (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') )) {
                valid_chars = 0;
                break;
            }
        }
        ok = valid_chars;
    }
    char name[64];
    snprintf(name, sizeof(name), "api_qs_uniqid length %d", length);
    check(name, ok);
    api_qs_memory_clean(mem);
}

/* api_qs_uniqid: 連続生成で同一IDがでないこと（1000件・ハッシュなしの簡易比較） */
static void test_api_qs_uniqid_unique(QS_MEMORY_CONTEXT* mem)
{
    char prev[64];
    char* id = api_qs_uniqid(mem, 32);
    if (id == NULL) {
        check("api_qs_uniqid unique over 1000 calls", 0);
        return;
    }
    memcpy(prev, id, 33);
    api_qs_memory_clean(mem);

    int all_same = 1;
    for (int i = 0; i < 1000; i++) {
        id = api_qs_uniqid(mem, 32);
        if (id == NULL) {
            all_same = 0; /* NULL は別チェックで検出 */
            break;
        }
        if (strcmp(prev, id) != 0) {
            all_same = 0;
            break;
        }
        memcpy(prev, id, 33);
        api_qs_memory_clean(mem);
    }
    check("api_qs_uniqid unique over 1000 calls", !all_same);
    api_qs_memory_clean(mem);
}

int main(int argc, char* argv[])
{
    api_qs_init();

    QS_MEMORY_CONTEXT memory;
    if (api_qs_memory_alloc(&memory, 1024 * 1024 * 8) != 0) {
        printf("api_qs_memory_alloc failed\n");
        return -1;
    }

    test_api_qs_rand_varies();
    test_api_qs_rand_not_zero();
    test_qs_rand_32_reproducible();
    test_qs_rand_32_different_seed();
    test_qs_srand_32();
    test_qs_rand_64_reproducible();
    test_qs_rand_64_varies();
    test_qs_srand_64();
    test_qs_uniqid_r32_length();
    test_qs_uniqid_r32_unique();
    test_qs_uniqid_r64_length();
    test_qs_uniqid_r64_unique();
    test_api_qs_uniqid(&memory, 8);
    test_api_qs_uniqid(&memory, 16);
    test_api_qs_uniqid(&memory, 32);
    test_api_qs_uniqid_unique(&memory);

    api_qs_memory_free(&memory);

    printf("========================================\n");
    printf("pass: %d, fail: %d\n", pass_count, fail_count);
    printf("========================================\n");
    return (fail_count > 0) ? 1 : 0;
}
