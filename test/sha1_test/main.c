/*
 * Copyright (c) Katsuya Owari
 * SHA1 test (qs_sha1 / api_qs_sha1_encode) with FIPS 180-1 test vectors
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "qs_api.h"
#include "qs_sha1.h"

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

/* 16進文字列をrawバイト列に変換する */
static int hex_to_bytes(const char* hex, uint8_t* out, size_t out_size)
{
    size_t hex_len = strlen(hex);
    if (hex_len / 2 > out_size) return -1;
    for (size_t i = 0; i < hex_len / 2; i++) {
        char hi = hex[i * 2];
        char lo = hex[i * 2 + 1];
        int hv = (hi >= 'a') ? hi - 'a' + 10 : (hi >= 'A') ? hi - 'A' + 10 : hi - '0';
        int lv = (lo >= 'a') ? lo - 'a' + 10 : (lo >= 'A') ? lo - 'A' + 10 : lo - '0';
        if (hv < 0 || hv > 15 || lv < 0 || lv > 15) return -1;
        out[i] = (uint8_t)((hv << 4) | lv);
    }
    return 0;
}

static void print_hex(const uint8_t* data, size_t size)
{
    for (size_t i = 0; i < size; i++) printf("%02x", data[i]);
    printf("\n");
}

/* qs_sha1: FIPS 180-1 テストベクトル */
static void test_qs_sha1_vectors(void)
{
    struct {
        const char* label;
        const char* input;
        size_t len;
        const char* expected_hex;
    } vectors[] = {
        { "empty string", "", 0,
          "da39a3ee5e6b4b0d3255bfef95601890afd80709" },
        { "\"abc\"", "abc", 3,
          "a9993e364706816aba3e25717850c26c9cd0d89d" },
        { "56 chars", "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 56,
          "84983e441c3bd26ebaae4aa1f95129e5e54670f1" },
        { "112 chars", "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu", 112,
          "a49b2446a02c645bf419f995b67091253a04a259" },
    };

    for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); i++) {
        uint8_t digest[20];
        uint8_t expected[20];
        qs_sha1(digest, vectors[i].input, (uint32_t)vectors[i].len);
        if (hex_to_bytes(vectors[i].expected_hex, expected, sizeof(expected)) != 0) {
            check(vectors[i].label, 0);
            continue;
        }
        int ok = (memcmp(digest, expected, 20) == 0);
        if (!ok) {
            printf("  expected: %s\n", vectors[i].expected_hex);
            printf("  got      : ");
            print_hex(digest, 20);
        }
        check(vectors[i].label, ok);
    }
}

/* qs_sha1: 100万文字の"A"（FIPS 180-1 最終ベクトル） */
static void test_qs_sha1_1m(void)
{
    size_t len = 1000000;
    char* input = (char*)malloc(len);
    if (input == NULL) {
        check("1 million 'A' (malloc failed)", 0);
        return;
    }
    memset(input, 'A', len);

    uint8_t digest[20];
    uint8_t expected[20];
    qs_sha1(digest, input, (uint32_t)len);
    hex_to_bytes("065e431442d313aa4c4345f1c7f3d3a84a9b201f", expected, sizeof(expected));

    int ok = (memcmp(digest, expected, 20) == 0);
    if (!ok) {
        printf("  expected: 065e431442d313aa4c4345f1c7f3d3a84a9b201f\n");
        printf("  got      : ");
        print_hex(digest, 20);
    }
    check("1 million 'A'", ok);
    free(input);
}

/* qs_sha1: ブロック境界（64バイトの倍数）付近の長さでハッシュが変化すること */
static void test_qs_sha1_block_boundary(void)
{
    /* 63 / 64 / 65 バイトで異なるdigestになること（パディング処理の簡易確認） */
    char buf[66];
    memset(buf, 'x', sizeof(buf));

    uint8_t d63[20], d64[20], d65[20];
    qs_sha1(d63, buf, 63);
    qs_sha1(d64, buf, 64);
    qs_sha1(d65, buf, 65);

    check("block boundary: 63/64/65 all differ",
          memcmp(d63, d64, 20) != 0 && memcmp(d64, d65, 20) != 0 && memcmp(d63, d65, 20) != 0);
}

/* api_qs_sha1_encode: 20バイトのraw digest + NUL終了を返すこと */
static void test_api_qs_sha1_encode(QS_MEMORY_CONTEXT* mem)
{
    char* result = api_qs_sha1_encode(mem, "abc", 3);
    check("api_qs_sha1_encode returns non-null", result != NULL);
    if (result != NULL) {
        uint8_t expected[20];
        hex_to_bytes("a9993e364706816aba3e25717850c26c9cd0d89d", expected, sizeof(expected));
        check("api_qs_sha1_encode digest matches \"abc\"", memcmp(result, expected, 20) == 0);
        check("api_qs_sha1_encode null terminated at [20]", result[20] == '\0');
    }
    api_qs_memory_clean(mem);
}

/* api_qs_sha1_encode: 空文字列 */
static void test_api_qs_sha1_encode_empty(QS_MEMORY_CONTEXT* mem)
{
    char* result = api_qs_sha1_encode(mem, "", 0);
    check("api_qs_sha1_encode empty returns non-null", result != NULL);
    if (result != NULL) {
        uint8_t expected[20];
        hex_to_bytes("da39a3ee5e6b4b0d3255bfef95601890afd80709", expected, sizeof(expected));
        check("api_qs_sha1_encode empty digest matches", memcmp(result, expected, 20) == 0);
    }
    api_qs_memory_clean(mem);
}

/* api_qs_sha1_encode: バイナリデータ（NULバイトを含む） */
static void test_api_qs_sha1_encode_binary(QS_MEMORY_CONTEXT* mem)
{
    uint8_t data[] = { 0x00, 0x01, 0x02, 0xff, 0xfe };
    char* result = api_qs_sha1_encode(mem, data, sizeof(data));
    check("api_qs_sha1_encode binary returns non-null", result != NULL);
    if (result != NULL) {
        /* 参照値: printf で確認した値ではなく、qs_sha1 との一致で検証 */
        uint8_t digest[20];
        qs_sha1(digest, data, sizeof(data));
        check("api_qs_sha1_encode binary matches qs_sha1", memcmp(result, digest, 20) == 0);
    }
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

    test_qs_sha1_vectors();
    test_qs_sha1_1m();
    test_qs_sha1_block_boundary();
    test_api_qs_sha1_encode(&memory);
    test_api_qs_sha1_encode_empty(&memory);
    test_api_qs_sha1_encode_binary(&memory);

    api_qs_memory_free(&memory);

    printf("========================================\n");
    printf("pass: %d, fail: %d\n", pass_count, fail_count);
    printf("========================================\n");
    return (fail_count > 0) ? 1 : 0;
}
