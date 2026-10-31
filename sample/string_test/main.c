/*
 * Copyright (c) Katsuya Owari
 *
 * string_test
 *   qs_string (文字列ユーティリティ) の全関数をテストする。
 *
 * 対象API:
 *   qs_ltoa / qs_ultoa / qs_itoa / qs_find_char
 *   qs_readline / qs_read_line_delimiter / qs_read_delimiter
 *   qs_strlcat / qs_strlink / qs_strcopy / qs_strlen
 *   qs_escape_directory_traversal / qs_nl2br / qs_nl2char
 *   qs_ihash / qs_utc_time / qs_urlencode / qs_urldecode
 *   qs_get_extension / qs_print_hex / qs_json_escape
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "qs_string.h"

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

/* 文字列比較チェック */
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
    char buf[256];
    char buf2[256];

    /* --- qs_ltoa (int64 -> string) --- */
    qs_ltoa(0, buf, sizeof(buf));
    check_str("ltoa 0", buf, "0");

    qs_ltoa(12345, buf, sizeof(buf));
    check_str("ltoa 12345", buf, "12345");

    qs_ltoa(-12345, buf, sizeof(buf));
    check_str("ltoa -12345", buf, "-12345");

    qs_ltoa(INT64_MAX, buf, sizeof(buf));
    check_str("ltoa INT64_MAX", buf, "9223372036854775807");

    qs_ltoa(1234567890123456789LL, buf, sizeof(buf));
    check_str("ltoa 1234567890123456789", buf, "1234567890123456789");

    /* --- qs_ultoa (uint64 -> string) --- */
    qs_ultoa(0, buf, sizeof(buf));
    check_str("ultoa 0", buf, "0");

    qs_ultoa(12345, buf, sizeof(buf));
    check_str("ultoa 12345", buf, "12345");

    qs_ultoa(UINT64_MAX, buf, sizeof(buf));
    check_str("ultoa UINT64_MAX", buf, "18446744073709551615");

    /* --- qs_itoa (int32 -> string) --- */
    qs_itoa(0, buf, sizeof(buf));
    check_str("itoa 0", buf, "0");

    qs_itoa(12345, buf, sizeof(buf));
    check_str("itoa 12345", buf, "12345");

    qs_itoa(-12345, buf, sizeof(buf));
    check_str("itoa -12345", buf, "-12345");

    qs_itoa(INT32_MAX, buf, sizeof(buf));
    check_str("itoa INT32_MAX", buf, "2147483647");

    /* --- qs_find_char --- */
    check("find_char space in 'hello world'", qs_find_char((char*)"hello world", 12, ' ') == 5);
    check("find_char 'a' at start", qs_find_char((char*)"abc", 4, 'a') == 0);
    check("find_char not found returns -1", qs_find_char((char*)"hello", 6, 'x') == -1);

    /* --- qs_readline --- */
    {
        char src[] = "hello\nworld\n";
        char out[64] = {0};
        char* next = qs_readline(out, sizeof(out), src, '\n');
        check_str("readline first line", out, "hello");
        check("readline advances past newline", next != NULL && strcmp(next, "world\n") == 0);
    }

    /* --- qs_read_line_delimiter --- */
    {
        char src[] = "abc,def\nghi";
        char out[64] = {0};
        char* next = qs_read_line_delimiter(out, sizeof(out), src, ',');
        check_str("read_line_delimiter stops at delimiter", out, "abc");
        check("read_line_delimiter advances", next != NULL && strcmp(next, "def\nghi") == 0);
    }

    /* --- qs_read_delimiter --- */
    {
        char src[] = "hello,world";
        char out[64] = {0};
        char* next = qs_read_delimiter(out, sizeof(out), src, ',');
        check_str("read_delimiter first token", out, "hello");
        check("read_delimiter advances", next != NULL && strcmp(next, "world") == 0);
    }

    /* --- qs_strlcat --- */
    {
        char dst[32];
        strcpy(dst, "hello");
        size_t ret = qs_strlcat(dst, " world", sizeof(dst));
        check_str("strlcat result", dst, "hello world");
        check("strlcat returns total length", ret == 11);
    }
    {
        /* 空きが足りない場合: 変更されない */
        char dst[6];
        strcpy(dst, "hello");
        size_t ret = qs_strlcat(dst, " world", sizeof(dst));
        check_str("strlcat no room keeps dst", dst, "hello");
        check("strlcat no room returns dlen+srclen", ret == 11);
    }

    /* --- qs_strlink --- */
    {
        char main_buf[32];
        strcpy(main_buf, "hello");
        char sub[] = " world";
        size_t ret = qs_strlink(main_buf, 5, sub, 6, sizeof(main_buf));
        check_str("strlink result", main_buf, "hello world");
        check("strlink returns new length", ret == 11);
    }

    /* --- qs_strcopy --- */
    {
        char dst[32];
        qs_strcopy(dst, "hello", sizeof(dst));
        check_str("strcopy full", dst, "hello");
    }
    {
        char dst[4];
        qs_strcopy(dst, "hello", sizeof(dst));
        check_str("strcopy truncated", dst, "hel");
    }

    /* --- qs_strlen --- */
    check("strlen 'hello'", qs_strlen("hello") == 5);
    check("strlen empty", qs_strlen("") == 0);

    /* --- qs_escape_directory_traversal --- */
    {
        char dst[64];
        int rc = qs_escape_directory_traversal(dst, "hello/world.txt", sizeof(dst));
        check_str("escape_dir safe path", dst, "hello/world.txt");
        check("escape_dir safe path returns OK", rc == QS_SYSTEM_OK);
    }
    {
        char dst[64];
        int rc = qs_escape_directory_traversal(dst, "../../etc/passwd", sizeof(dst));
        check("escape_dir traversal returns ERROR", rc == QS_SYSTEM_ERROR);
        check("escape_dir removes '..'", strstr(dst, "..") == NULL);
    }

    /* --- qs_nl2br --- */
    {
        char dst[128];
        qs_nl2br(dst, "hello\nworld", sizeof(dst));
        check_str("nl2br LF", dst, "hello<br/>world");

        qs_nl2br(dst, "hello\r\nworld", sizeof(dst));
        check_str("nl2br CRLF", dst, "hello<br/>world");

        qs_nl2br(dst, "hello\rworld", sizeof(dst));
        check_str("nl2br CR", dst, "hello<br/>world");
    }

    /* --- qs_nl2char --- */
    {
        char dst[128];
        qs_nl2char(dst, "hello\nworld", sizeof(dst));
        check_str("nl2char LF", dst, "hello\\nworld");

        qs_nl2char(dst, "hello\r\nworld", sizeof(dst));
        check_str("nl2char CRLF", dst, "hello\\r\\nworld");
    }

    /* --- qs_ihash --- */
    {
        uint32_t h1 = qs_ihash("abc", 100);
        uint32_t h2 = qs_ihash("abc", 100);
        uint32_t h3 = qs_ihash("abd", 100);
        check("ihash deterministic", h1 == h2);
        check("ihash within range", h1 < 100);
        check("ihash different inputs differ", h1 != h3);
    }

    /* --- qs_utc_time --- */
    {
        char dst[64] = {0};
        int rc = qs_utc_time(dst, sizeof(dst));
        check("utc_time returns OK", rc == QS_SYSTEM_OK);
        check("utc_time non-empty", qs_strlen(dst) > 0);
        /* 例: "Sun Sep 16 01:03:52 1973" (24文字) */
        check("utc_time length ~24", qs_strlen(dst) >= 20 && qs_strlen(dst) <= 26);
    }

    /* --- qs_urlencode --- */
    {
        char dst[128];
        int rc = qs_urlencode(dst, sizeof(dst), (char*)"hello world");
        check_str("urlencode space->+", dst, "hello+world");
        check("urlencode returns OK", rc == QS_SYSTEM_OK);

        rc = qs_urlencode(dst, sizeof(dst), (char*)"a/b?c=d");
        check_str("urlencode special chars", dst, "a%2Fb%3Fc%3Dd");

        rc = qs_urlencode(dst, sizeof(dst), (char*)"abc123");
        check_str("urlencode unreserved", dst, "abc123");
    }

    /* --- qs_urldecode --- */
    {
        char dst[128];
        int rc = qs_urldecode(dst, sizeof(dst), (char*)"hello+world");
        check_str("urldecode +->space", dst, "hello world");
        check("urldecode returns OK", rc == QS_SYSTEM_OK);

        rc = qs_urldecode(dst, sizeof(dst), (char*)"a%2Fb%3Fc%3Dd");
        check_str("urldecode percent", dst, "a/b?c=d");
    }

    /* --- urlencode/urldecode round trip --- */
    {
        char encoded[128];
        char decoded[128];
        const char* original = "hello world/a?b=c&d=e";
        qs_urlencode(encoded, sizeof(encoded), (char*)original);
        qs_urldecode(decoded, sizeof(decoded), encoded);
        check_str("url encode/decode round trip", decoded, original);
    }

    /* --- qs_get_extension --- */
    {
        char dst[32];
        int rc = qs_get_extension(dst, sizeof(dst), (char*)"file.txt");
        check_str("get_extension txt", dst, "txt");
        check("get_extension returns OK", rc == QS_SYSTEM_OK);

        qs_get_extension(dst, sizeof(dst), (char*)"archive.tar.gz");
        check_str("get_extension gz", dst, "gz");

        memset(dst, 0, sizeof(dst));
        qs_get_extension(dst, sizeof(dst), (char*)"noext");
        check_str("get_extension none", dst, "");
    }

    /* --- qs_print_hex (クラッシュしないこと・出力があること) --- */
    {
        uint8_t data[] = {0x00, 0x01, 0x02, 0xAB, 0xFF};
        /* 出力はstdoutに出る。ここでは呼び出しが安全に完了することを確認 */
        qs_print_hex(data, sizeof(data), 3);
        check("print_hex runs without crash", 1);
    }

    /* --- qs_json_escape --- */
    {
        char dst[128];
        int rc = qs_json_escape("hello", dst, sizeof(dst));
        check_str("json_escape plain", dst, "hello");
        check("json_escape returns OK", rc == QS_SYSTEM_OK);

        rc = qs_json_escape("he\"llo", dst, sizeof(dst));
        check_str("json_escape quote", dst, "he\\\"llo");

        rc = qs_json_escape("line1\nline2", dst, sizeof(dst));
        check_str("json_escape newline", dst, "line1\\nline2");

        rc = qs_json_escape("tab\there", dst, sizeof(dst));
        check_str("json_escape tab", dst, "tab\\there");

        rc = qs_json_escape("back\\slash", dst, sizeof(dst));
        check_str("json_escape backslash", dst, "back\\\\slash");
    }

    printf("\n=== string_test summary ===\n");
    printf("pass: %d, fail: %d\n", pass_count, fail_count);
    return (fail_count > 0) ? 1 : 0;
}
