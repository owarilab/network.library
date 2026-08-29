/*
 * Copyright (c) Katsuya Owari
 * Base64 encode/decode test
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "qs_api.h"
#include "qs_base64.h"

static int pass_count = 0;
static int fail_count = 0;

static void check_encode(QS_MEMORY_CONTEXT* mem, const char* label, const void* input, size_t len, const char* expected)
{
    char* result = api_qs_base64_encode(mem, input, len);
    if (result && strcmp(result, expected) == 0) {
        printf("[PASS] encode %s\n", label);
        pass_count++;
    } else {
        printf("[FAIL] encode %s : expected=\"%s\" got=\"%s\"\n", label, expected, result ? result : "(null)");
        fail_count++;
    }
    api_qs_memory_clean(mem);
}

static void check_decode(QS_MEMORY_CONTEXT* mem, const char* label, const char* input, const void* expected, size_t expected_len)
{
    char* result = api_qs_base64_decode(mem, input);
    if (result && memcmp(result, expected, expected_len) == 0) {
        printf("[PASS] decode %s\n", label);
        pass_count++;
    } else {
        printf("[FAIL] decode %s\n", label);
        fail_count++;
    }
    api_qs_memory_clean(mem);
}

static void check_roundtrip(QS_MEMORY_CONTEXT* mem, const char* label, const void* input, size_t len)
{
    char* encoded = api_qs_base64_encode(mem, input, len);
    if (!encoded) {
        printf("[FAIL] roundtrip %s : encode returned null\n", label);
        fail_count++;
        api_qs_memory_clean(mem);
        return;
    }
    char* decoded = api_qs_base64_decode(mem, encoded);
    if (decoded && memcmp(decoded, input, len) == 0) {
        printf("[PASS] roundtrip %s\n", label);
        pass_count++;
    } else {
        printf("[FAIL] roundtrip %s\n", label);
        fail_count++;
    }
    api_qs_memory_clean(mem);
}

static void check_encode_capacity(const char* label, const void* input, uint16_t input_len,
                                  uint16_t dest_len, const char* expected, size_t expected_len)
{
    char dest[32];
    memset(dest, 'X', sizeof(dest));
    qs_base64_encode(dest, dest_len, input, input_len);
    if (memcmp(dest, expected, expected_len) == 0 && dest[expected_len] == '\0' &&
        dest[dest_len] == 'X') {
        printf("[PASS] encode capacity %s\n", label);
        pass_count++;
    } else {
        printf("[FAIL] encode capacity %s\n", label);
        fail_count++;
    }
}

static void check_decode_capacity(const char* label, const char* input, uint16_t input_len,
                                  uint16_t dest_len, const void* expected, size_t expected_len)
{
    unsigned char dest[32];
    memset(dest, 0x5a, sizeof(dest));
    qs_base64_decode((char*)dest, dest_len, input, input_len);
    if (memcmp(dest, expected, expected_len) == 0 && dest[expected_len] == '\0' &&
        dest[dest_len] == 0x5a) {
        printf("[PASS] decode capacity %s\n", label);
        pass_count++;
    } else {
        printf("[FAIL] decode capacity %s\n", label);
        fail_count++;
    }
}

static void check_decode_rejected(QS_MEMORY_CONTEXT* mem, const char* label, const char* input)
{
    char* result = api_qs_base64_decode(mem, input);
    if (result && result[0] == '\0') {
        printf("[PASS] reject decode %s\n", label);
        pass_count++;
    } else {
        printf("[FAIL] reject decode %s\n", label);
        fail_count++;
    }
    api_qs_memory_clean(mem);
}

static void check_size(const char* label, size_t actual, size_t expected)
{
    if (actual == expected) {
        printf("[PASS] size %s\n", label);
        pass_count++;
    } else {
        printf("[FAIL] size %s : expected=%zu got=%zu\n", label, expected, actual);
        fail_count++;
    }
}

static void check_api_null_arguments(QS_MEMORY_CONTEXT* mem)
{
    QS_MEMORY_CONTEXT empty_context = { NULL };
    const char input[] = "test";

    if (api_qs_base64_encode(NULL, input, sizeof(input) - 1) == NULL &&
        api_qs_base64_encode(mem, NULL, sizeof(input) - 1) == NULL &&
        api_qs_base64_encode(&empty_context, input, sizeof(input) - 1) == NULL &&
        api_qs_base64_decode(NULL, "dGVzdA==") == NULL &&
        api_qs_base64_decode(mem, NULL) == NULL &&
        api_qs_base64_decode(&empty_context, "dGVzdA==") == NULL) {
        printf("[PASS] API wrapper NULL argument validation\n");
        pass_count++;
    } else {
        printf("[FAIL] API wrapper NULL argument validation\n");
        fail_count++;
    }

    char* empty_encoded = api_qs_base64_encode(mem, NULL, 0);
    if (empty_encoded && strcmp(empty_encoded, "") == 0) {
        printf("[PASS] API wrapper empty input\n");
        pass_count++;
    } else {
        printf("[FAIL] API wrapper empty input\n");
        fail_count++;
    }
    api_qs_memory_clean(mem);
}

int main(int argc, char* argv[], char* envp[])
{
    QS_MEMORY_CONTEXT mem;
    if (-1 == api_qs_memory_alloc(&mem, 1024 * 1024)) {
        printf("api_qs_memory_alloc failed\n");
        return -1;
    }

    printf("=== Base64 encode tests ===\n");
    check_api_null_arguments(&mem);
    check_size("encode empty", qs_base64_encode_size(0), 1);
    check_size("encode one byte", qs_base64_encode_size(1), 5);
    check_size("encode three bytes", qs_base64_encode_size(3), 5);
    check_size("encode four bytes", qs_base64_encode_size(4), 9);
    /* RFC 4648 standard vectors */
    /* empty input (length=0) is explicitly rejected by the API; skip */
    check_encode(&mem, "\"f\"",        "f",     1, "Zg==");
    check_encode(&mem, "\"fo\"",       "fo",    2, "Zm8=");
    check_encode(&mem, "\"foo\"",      "foo",   3, "Zm9v");
    check_encode(&mem, "\"foob\"",     "foob",  4, "Zm9vYg==");
    check_encode(&mem, "\"fooba\"",    "fooba", 5, "Zm9vYmE=");
    check_encode(&mem, "\"foobar\"",   "foobar",6, "Zm9vYmFy");
    check_encode(&mem, "\"Man\"",      "Man",   3, "TWFu");
    check_encode(&mem, "\"Ma\"",       "Ma",    2, "TWE=");
    check_encode(&mem, "\"M\"",        "M",     1, "TQ==");
    check_encode(&mem, "Hello World!", "Hello, World!", 13, "SGVsbG8sIFdvcmxkIQ==");

    printf("\n=== Base64 encode boundary tests ===\n");
    {
        const char input[] = "f";
        check_encode_capacity("truncated output", input, 1, 3, "Zg", 2);
    }
    {
        const char input[] = "foob";
        check_encode_capacity("truncated padded output", input, 4, 5, "Zm9v", 4);
    }

    printf("\n=== Base64 decode tests ===\n");
    check_size("decode one byte", qs_base64_decode_size("Zg==", 4), 2);
    check_size("decode two bytes", qs_base64_decode_size("Zm8=", 4), 3);
    check_size("decode three bytes", qs_base64_decode_size("Zm9v", 4), 4);
    check_size("decode invalid length", qs_base64_decode_size("Zg", 2), 0);
    check_size("decode null input", qs_base64_decode_size(NULL, 4), 0);
    check_size("decode invalid characters", qs_base64_decode_size("!!!!", 4), 0);
    check_size("decode invalid padding", qs_base64_decode_size("Zm=8", 4), 0);
    check_decode(&mem, "\"Zg==\"",   "Zg==",   "f",      1);
    check_decode(&mem, "\"Zm8=\"",   "Zm8=",   "fo",     2);
    check_decode(&mem, "\"Zm9v\"",   "Zm9v",   "foo",    3);
    check_decode(&mem, "\"TWFu\"",   "TWFu",   "Man",    3);
    check_decode(&mem, "\"TWE=\"",   "TWE=",   "Ma",     2);
    check_decode(&mem, "\"TQ==\"",   "TQ==",   "M",      1);

    printf("\n=== Base64 decode validation tests ===\n");
    check_decode_rejected(&mem, "short input", "Zg");
    check_decode_rejected(&mem, "single character", "Z");
    check_decode_rejected(&mem, "incomplete padding", "Zg=");
    check_decode_rejected(&mem, "invalid characters", "!!!!");
    check_decode_rejected(&mem, "invalid padding order", "Zm=8");
    check_decode_rejected(&mem, "non-zero padding bits", "Zh==");
    check_decode_rejected(&mem, "padding before final block", "Zg==Zm8=");

    printf("\n=== Base64 decode boundary tests ===\n");
    {
        const unsigned char expected[] = {'f'};
        check_decode_capacity("truncated output", "Zg==", 4, 2, expected, 1);
    }

    printf("\n=== Binary boundary tests ===\n");
    {
        unsigned char input[256];
        char encoded[349];
        unsigned char decoded[257];
        for (size_t i = 0; i < sizeof(input); i++) input[i] = (unsigned char)i;
        qs_base64_encode(encoded, sizeof(encoded), input, sizeof(input));
        qs_base64_decode((char*)decoded, sizeof(decoded), encoded, (uint16_t)strlen(encoded));
        if (strlen(encoded) == 344 && memcmp(decoded, input, sizeof(input)) == 0 &&
            decoded[sizeof(input)] == '\0') {
            printf("[PASS] all byte values roundtrip\n");
            pass_count++;
        } else {
            printf("[FAIL] all byte values roundtrip\n");
            fail_count++;
        }
    }

    printf("\n=== Roundtrip tests ===\n");
    check_roundtrip(&mem, "ascii string",     "Hello, World!", 13);
    check_roundtrip(&mem, "single byte",      "\xFF", 1);
    check_roundtrip(&mem, "two bytes",        "\x00\xFF", 2);
    check_roundtrip(&mem, "three bytes",      "\x12\x34\x56", 3);
    check_roundtrip(&mem, "binary sequence",  "\x00\x01\x02\x03\x04\x05\x06\x07", 8);

    printf("\n=== Result: %d passed, %d failed ===\n", pass_count, fail_count);

    /* --- Python 検証用出力: base64\thex_original --- */
    printf("\n=== PYTHON_VERIFY_BEGIN ===\n");
    static const struct { const char* data; size_t len; } vcases[] = {
        {"f",             1},
        {"fo",            2},
        {"foo",           3},
        {"foob",          4},
        {"fooba",         5},
        {"foobar",        6},
        {"Man",           3},
        {"Hello, World!", 13},
        {"\x00\x01\x02\x03\x04\x05\x06\x07", 8},
        {"\xFF\x00\xAB\xCD", 4},
    };
    for (size_t k = 0; k < sizeof(vcases)/sizeof(vcases[0]); k++) {
        char* enc = api_qs_base64_encode(&mem, vcases[k].data, vcases[k].len);
        if (enc) {
            printf("%s\t", enc);
            for (size_t m = 0; m < vcases[k].len; m++) {
                printf("%02x", (unsigned char)vcases[k].data[m]);
            }
            printf("\n");
        }
        api_qs_memory_clean(&mem);
    }
    printf("=== PYTHON_VERIFY_END ===\n");

    api_qs_memory_free(&mem);
    return (fail_count == 0) ? 0 : 1;
}
