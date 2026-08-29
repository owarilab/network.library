/*
 * Copyright (c) Katsuya Owari
 * Python生成のbase64をCでデコードして検証するプログラム
 * stdin からフォーマット: base64\thex_original\tlabel を読み込む
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "qs_api.h"

static int hex2bytes(const char* hex, unsigned char* out, size_t* out_len)
{
    size_t len = strlen(hex);
    if (len % 2 != 0) return -1;
    for (size_t i = 0; i < len; i += 2) {
        unsigned int b;
        if (sscanf(hex + i, "%02x", &b) != 1) return -1;
        out[i / 2] = (unsigned char)b;
    }
    *out_len = len / 2;
    return 0;
}

int main(int argc, char* argv[], char* envp[])
{
    QS_MEMORY_CONTEXT mem;
    if (-1 == api_qs_memory_alloc(&mem, 1024 * 1024)) {
        fprintf(stderr, "api_qs_memory_alloc failed\n");
        return -1;
    }

    int ok = 0, ng = 0;
    char line[4096];

    while (fgets(line, sizeof(line), stdin)) {
        /* 末尾の改行を除去 */
        line[strcspn(line, "\n")] = '\0';
        if (line[0] == '\0') continue;

        char b64[2048] = {0};
        char hex[2048] = {0};
        char label[1024] = {0};
        if (sscanf(line, "%2047s\t%2047s\t%1023[^\n]", b64, hex, label) < 2) continue;

        /* Python生成のbase64をCでデコード */
        char* decoded = api_qs_base64_decode(&mem, b64);

        unsigned char expected[1024];
        size_t expected_len = 0;
        if (hex2bytes(hex, expected, &expected_len) != 0) {
            fprintf(stderr, "hex parse error: %s\n", hex);
            ng++;
            api_qs_memory_clean(&mem);
            continue;
        }

        size_t decoded_len = decoded ? strlen(decoded) : 0;
        /* バイナリ比較（NUL含む場合は memcmp） */
        int match = (decoded != NULL)
                    && (decoded_len == expected_len || expected_len == 0)
                    && (expected_len == 0 || memcmp(decoded, expected, expected_len) == 0);

        /* NULを含むケースは長さが合わないのでmemcmpで再確認 */
        if (!match && decoded) {
            /* decoded_lenはstrcspnで正確に取れないので expected_len で比較 */
            match = memcmp(decoded, expected, expected_len) == 0;
        }

        if (match) {
            printf("[PASS] %-40s  %s\n", b64, label);
            ok++;
        } else {
            printf("[FAIL] %-40s  %s\n", b64, label);
            /* 失敗時はデコード結果をhexダンプ */
            printf("       expected(%zu): %s\n", expected_len, hex);
            if (decoded) {
                printf("       got    (%zu): ", expected_len);
                for (size_t i = 0; i < expected_len; i++) {
                    printf("%02x", (unsigned char)decoded[i]);
                }
                printf("\n");
            } else {
                printf("       got: (null)\n");
            }
            ng++;
        }
        api_qs_memory_clean(&mem);
    }

    printf("\nC decode verification: %d passed, %d failed\n", ok, ng);

    api_qs_memory_free(&mem);
    return (ng == 0) ? 0 : 1;
}
