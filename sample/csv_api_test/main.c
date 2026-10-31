/*
 * Copyright (c) Katsuya Owari
 * High-level CSV API test (api_qs_csv_read_file / api_qs_csv_parse /
 * api_qs_csv_get_line_length / api_qs_csv_get_row_length / api_qs_csv_get_row)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "qs_api.h"

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

static void check_str(const char* name, const char* got, const char* expected)
{
    if (got != NULL && expected != NULL && strcmp(got, expected) == 0) {
        printf("[PASS] %s\n", name);
        pass_count++;
    } else {
        printf("[FAIL] %s : expected=\"%s\" got=\"%s\"\n",
               name, expected ? expected : "(null)", got ? got : "(null)");
        fail_count++;
    }
}

static void check_int(const char* name, int32_t got, int32_t expected)
{
    if (got == expected) {
        printf("[PASS] %s\n", name);
        pass_count++;
    } else {
        printf("[FAIL] %s : expected=%d got=%d\n", name, expected, got);
        fail_count++;
    }
}

/* api_qs_csv_read_file: ファイル読み込みと内容取得 */
static void test_read_file(QS_MEMORY_CONTEXT* mem)
{
    QS_CSV_CONTEXT csv = {0};
    int rc = api_qs_csv_read_file(mem, &csv, "sample.csv");
    check("api_qs_csv_read_file returns 0", rc == 0);

    check_int("read_file: line length", api_qs_csv_get_line_length(&csv), 5);
    check_int("read_file: row length [0]", api_qs_csv_get_row_length(&csv, 0), 4);
    check_str("read_file: [0][0]", api_qs_csv_get_row(&csv, 0, 0), "id");
    check_str("read_file: [0][3]", api_qs_csv_get_row(&csv, 0, 3), "city");
    check_str("read_file: [1][1] quoted", api_qs_csv_get_row(&csv, 1, 1), "John Doe");
    check_str("read_file: [1][2] numeric", api_qs_csv_get_row(&csv, 1, 2), "35");
    check_str("read_file: [4][3]", api_qs_csv_get_row(&csv, 4, 3), "San Francisco");

    api_qs_memory_clean(mem);
}

/* api_qs_csv_read_file: 存在しないファイルで -1 を返す */
static void test_read_file_not_found(QS_MEMORY_CONTEXT* mem)
{
    QS_CSV_CONTEXT csv = {0};
    int rc = api_qs_csv_read_file(mem, &csv, "no_such_file_xyz.csv");
    check("api_qs_csv_read_file missing file returns -1", rc == -1);
    api_qs_memory_clean(mem);
}

/* api_qs_csv_parse: 基本の複数行・複数列 */
static void test_parse_basic(QS_MEMORY_CONTEXT* mem)
{
    QS_CSV_CONTEXT csv = {0};
    int rc = api_qs_csv_parse(mem, &csv, "a,1\nb,2\nc,3");
    check("api_qs_csv_parse returns 0", rc == 0);

    check_int("parse basic: line length", api_qs_csv_get_line_length(&csv), 3);
    check_int("parse basic: row length [0]", api_qs_csv_get_row_length(&csv, 0), 2);
    check_str("parse basic: [0][0]", api_qs_csv_get_row(&csv, 0, 0), "a");
    check_str("parse basic: [1][1]", api_qs_csv_get_row(&csv, 1, 1), "2");
    check_str("parse basic: [2][0]", api_qs_csv_get_row(&csv, 2, 0), "c");

    api_qs_memory_clean(mem);
}

/* api_qs_csv_parse: 引用符フィールド（カンマ・改行・エスケープクォート） */
static void test_parse_quoted(QS_MEMORY_CONTEXT* mem)
{
    QS_CSV_CONTEXT csv = {0};
    int rc = api_qs_csv_parse(mem, &csv, "a,\"b,c\",\"line1\nline2\",\"say \"\"hi\"\"\"");
    check("api_qs_csv_parse quoted returns 0", rc == 0);

    check_int("parse quoted: field count", api_qs_csv_get_row_length(&csv, 0), 4);
    check_str("parse quoted: comma field", api_qs_csv_get_row(&csv, 0, 1), "b,c");
    check_str("parse quoted: embedded newline", api_qs_csv_get_row(&csv, 0, 2), "line1\nline2");
    check_str("parse quoted: escaped quote", api_qs_csv_get_row(&csv, 0, 3), "say \"hi\"");

    api_qs_memory_clean(mem);
}

/* api_qs_csv_parse: 空フィールド・末尾空フィールド */
static void test_parse_empty_fields(QS_MEMORY_CONTEXT* mem)
{
    QS_CSV_CONTEXT csv = {0};
    int rc = api_qs_csv_parse(mem, &csv, "a,,c\nx,y,");
    check("api_qs_csv_parse empty fields returns 0", rc == 0);

    check_int("parse empty: field count [0]", api_qs_csv_get_row_length(&csv, 0), 3);
    check_str("parse empty: [0][1] is empty", api_qs_csv_get_row(&csv, 0, 1), "");
    check_int("parse empty: field count [1]", api_qs_csv_get_row_length(&csv, 1), 3);
    check_str("parse empty: [1][2] trailing empty", api_qs_csv_get_row(&csv, 1, 2), "");

    api_qs_memory_clean(mem);
}

/* api_qs_csv_parse: 単一セル */
static void test_parse_single_cell(QS_MEMORY_CONTEXT* mem)
{
    QS_CSV_CONTEXT csv = {0};
    int rc = api_qs_csv_parse(mem, &csv, "solo");
    check("api_qs_csv_parse single cell returns 0", rc == 0);

    check_int("parse single: line length", api_qs_csv_get_line_length(&csv), 1);
    check_int("parse single: row length", api_qs_csv_get_row_length(&csv, 0), 1);
    check_str("parse single: value", api_qs_csv_get_row(&csv, 0, 0), "solo");

    api_qs_memory_clean(mem);
}

/* api_qs_csv_parse: 日本語（マルチバイト）フィールド */
static void test_parse_multibyte(QS_MEMORY_CONTEXT* mem)
{
    QS_CSV_CONTEXT csv = {0};
    int rc = api_qs_csv_parse(mem, &csv, "id,name,comment\n1,\"山田,太郎\",\"東京在住\"");
    check("api_qs_csv_parse multibyte returns 0", rc == 0);

    check_int("parse multibyte: line length", api_qs_csv_get_line_length(&csv), 2);
    check_str("parse multibyte: [1][1]", api_qs_csv_get_row(&csv, 1, 1), "山田,太郎");
    check_str("parse multibyte: [1][2]", api_qs_csv_get_row(&csv, 1, 2), "東京在住");

    api_qs_memory_clean(mem);
}

/* 未初期化の QS_CSV_CONTEXT で安全に -1 / NULL を返す */
static void test_uninitialized_context(void)
{
    QS_CSV_CONTEXT csv = {-1, NULL};
    check_int("uninitialized: get_line_length returns -1", api_qs_csv_get_line_length(&csv), -1);
    check_int("uninitialized: get_row_length returns -1", api_qs_csv_get_row_length(&csv, 0), -1);
    check("uninitialized: get_row returns NULL", api_qs_csv_get_row(&csv, 0, 0) == NULL);
}

int main(int argc, char* argv[])
{
    api_qs_init();

    QS_MEMORY_CONTEXT memory;
    if (api_qs_memory_alloc(&memory, 1024 * 1024 * 8) != 0) {
        printf("api_qs_memory_alloc failed\n");
        return -1;
    }

    test_read_file(&memory);
    test_read_file_not_found(&memory);
    test_parse_basic(&memory);
    test_parse_quoted(&memory);
    test_parse_empty_fields(&memory);
    test_parse_single_cell(&memory);
    test_parse_multibyte(&memory);
    test_uninitialized_context();

    api_qs_memory_free(&memory);

    printf("========================================\n");
    printf("pass: %d, fail: %d\n", pass_count, fail_count);
    printf("========================================\n");
    return (fail_count > 0) ? 1 : 0;
}
