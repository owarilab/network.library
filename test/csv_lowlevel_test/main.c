/*
 * Copyright (c) Katsuya Owari
 *
 * CSV low-level API test (qs_csv.h)
 *
 * Verifies the following:
 *  - qs_csv_file_load / qs_csv_parse
 *  - qs_csv_get_line_length / qs_csv_get_row_length / qs_csv_get_row
 *  - qs_csv_build_csv / qs_csv_build_csv_memid
 *  - qs_csv_init / qs_csv_add_row / qs_csv_add_line
 *  - Edge cases: empty fields, quoted commas, newlines inside quotes,
 *    escaped quotes, negative numbers, round-trip
 *
 * Build: make
 * Run:   ./qs_csv_lowlevel_test
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#include "qs_core.h"
#include "qs_memory_allocator.h"
#include "qs_csv.h"

/* ---- 簡易アサートヘルパー ---- */
static int g_pass = 0;
static int g_fail = 0;

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

static void check_int(const char* label, int32_t got, int32_t expected)
{
    if (got == expected) {
        printf("[PASS] %s\n", label);
        g_pass++;
    } else {
        printf("[FAIL] %s : expected=%d got=%d\n", label, expected, got);
        g_fail++;
    }
}

int main(int argc, char *argv[])
{
    QS_MEMORY_POOL* memory = NULL;
	size_t size;
	if( (size = qs_initialize_memory_f64( &memory, 1024 * 1024 * 8) ) <= 0 ){
		return -1;
	}
    
    int32_t memid_csv = qs_csv_file_load(memory,"sample.csv");
    if(-1==memid_csv){
        return -1;
    }

    printf("qs_csv_build_csv:\n");
    char* csv = qs_csv_build_csv(memory,memid_csv,1024*1024*2);
    if(NULL==csv){
        return -1;
    }
    printf("%s\n",csv);
    
    printf("\n");
    printf("qs_csv_parse:\n");
    int32_t memid_csv2 = qs_csv_parse(memory,csv);
    if(-1==memid_csv2){
        return -1;
    }

    // add line
    int32_t memid_array = -1;
    if(-1==qs_array_push_string(memory,&memid_array,"1")){
        return -1;
    }
    if(-1==qs_array_push_string(memory,&memid_array,"2")){
        return -1;
    }
    if(-1==qs_array_push_string(memory,&memid_array,"3")){
        return -1;
    }
    qs_csv_add_line(memory,memid_csv2,memid_array);

    // add row
    qs_csv_add_row(memory,memid_csv2,0,"add row 1");
    qs_csv_add_row(memory,memid_csv2,1,"add row 2");
    qs_csv_add_row(memory,memid_csv2,2,"add row 3");
    qs_csv_add_row(memory,memid_csv2,10,"add row 4");
    qs_csv_add_row(memory,memid_csv2,7,"add row 5");
    qs_csv_add_row(memory,memid_csv2,7,"add row 6");
    
    int32_t i;
    for(i=0;i<qs_csv_get_line_length(memory,memid_csv2);i++){
        int32_t j;
        for(j=0;j<qs_csv_get_row_length(memory,memid_csv2,i);j++){
            printf("%s,",qs_csv_get_row(memory,memid_csv2,i,j));
        }
        printf("\n");
    }

    /* ============================================================
     * 不具合が起きやすいパターンの追加テスト
     * ============================================================ */
    printf("\n=== Edge case tests ===\n");

    /* --- 1. 空フィールド (a,,c) --- */
    {
        int32_t m = qs_csv_parse(memory, "a,,c");
        check_int("empty field: row count",  qs_csv_get_row_length(memory,m,0), 3);
        check_str("empty field: col[0]",     qs_csv_get_row(memory,m,0,0), "a");
        check_str("empty field: col[1]",     qs_csv_get_row(memory,m,0,1), "");
        check_str("empty field: col[2]",     qs_csv_get_row(memory,m,0,2), "c");
    }

    /* --- 2. 空文字列フィールド ("") → build時にクォートされるか --- */
    {
        /* build_csv で "" フィールドが "" と出力されることを確認 */
        int32_t m = qs_csv_parse(memory, "x,,z");
        char* built = qs_csv_build_csv(memory, m, 1024);
        check_str("empty field build_csv", built, "\"x\",\"\",\"z\"");
    }

    /* --- 3. 数値・小数 → クォートなしで出力されるか ---
     * 注意: トークナイザーが '-' を独立トークンに分割するため
     * 負数 (-1) は 2フィールド('-', '1') として扱われる */
    {
        int32_t m = qs_csv_parse(memory, "0,1,3.14");
        char* built = qs_csv_build_csv(memory, m, 1024);
        check_str("numeric fields: 0/1/3.14", built, "0,1,3.14");
    }

    /* --- 3b. 負数は '-' と次の数値を結合して1フィールドとして扱われる --- */
    {
        int32_t m = qs_csv_parse(memory, "0,-1,3.14");
        check_int("negative number: field count", qs_csv_get_row_length(memory, m, 0), 3);
        check_str("negative number: [1] is '-1'", qs_csv_get_row(memory, m, 0, 1), "-1");
    }

    /* --- 4. 文字列と数値混在 --- */
    {
        int32_t m = qs_csv_parse(memory, "name,42,hello");
        char* built = qs_csv_build_csv(memory, m, 1024);
        check_str("mixed fields", built, "\"name\",42,\"hello\"");
    }

    /* --- 5. 複数行のラウンドトリップ (parse→build→parse→get) --- */
    {
        const char* src = "a,1\nb,2\nc,3";
        int32_t m1 = qs_csv_parse(memory, src);
        char* built = qs_csv_build_csv(memory, m1, 4096);
        int32_t m2 = qs_csv_parse(memory, built);
        check_int("roundtrip: line count",   qs_csv_get_line_length(memory, m2), 3);
        check_str("roundtrip: [0][0]",       qs_csv_get_row(memory, m2, 0, 0), "a");
        check_str("roundtrip: [1][1]",       qs_csv_get_row(memory, m2, 1, 1), "2");
        check_str("roundtrip: [2][0]",       qs_csv_get_row(memory, m2, 2, 0), "c");
    }

    /* --- 6. 1列のみ・1行のみ --- */
    {
        int32_t m = qs_csv_parse(memory, "solo");
        check_int("single cell: line count", qs_csv_get_line_length(memory, m), 1);
        check_int("single cell: row count",  qs_csv_get_row_length(memory, m, 0), 1);
        check_str("single cell: value",      qs_csv_get_row(memory, m, 0, 0), "solo");
    }

    /* --- 7. 全フィールドが空 (,,) --- */
    {
        int32_t m = qs_csv_parse(memory, ",,");
        check_int("all empty ,, : row count", qs_csv_get_row_length(memory, m, 0), 3);
        check_str("all empty ,, : col[0]", qs_csv_get_row(memory, m, 0, 0), "");
    }

    /* --- 8. ファイルロード→build→再parse ラウンドトリップ --- */
    {
        int32_t m1 = qs_csv_file_load(memory, "sample.csv");
        char* built = qs_csv_build_csv(memory, m1, 1024*64);
        int32_t m2 = qs_csv_parse(memory, built);
        check_int("file roundtrip: line count", qs_csv_get_line_length(memory, m2), 5);
        check_str("file roundtrip: [0][0]",     qs_csv_get_row(memory, m2, 0, 0), "id");
        check_str("file roundtrip: [1][1]",     qs_csv_get_row(memory, m2, 1, 1), "John Doe");
        check_str("file roundtrip: [4][3]",     qs_csv_get_row(memory, m2, 4, 3), "San Francisco");
    }

    /* --- 9. quoted comma and embedded newline --- */
    {
        int32_t m = qs_csv_parse(memory, "a,\"b,c\",\"line1\nline2\"");
        check_int("quoted fields: field count", qs_csv_get_row_length(memory, m, 0), 3);
        check_str("quoted comma", qs_csv_get_row(memory, m, 0, 1), "b,c");
        check_str("embedded newline", qs_csv_get_row(memory, m, 0, 2), "line1\nline2");
    }

    /* --- 10. escaped quote must survive parse/build roundtrip --- */
    {
        int32_t m1 = qs_csv_parse(memory, "name,\"say \"\"hello\"\"\"");
        char* built = qs_csv_build_csv(memory, m1, 1024);
        int32_t m2 = qs_csv_parse(memory, built);
        check_str("escaped quote: parsed value", qs_csv_get_row(memory, m2, 0, 1), "say \"hello\"");
    }

    /* --- 11. trailing empty fields and blank records are significant --- */
    {
        int32_t m = qs_csv_parse(memory, "a,b,\n\n");
        check_int("trailing empty: field count", qs_csv_get_row_length(memory, m, 0), 3);
        check_str("trailing empty: last field", qs_csv_get_row(memory, m, 0, 2), "");
        check_int("blank record: line count", qs_csv_get_line_length(memory, m), 2);
        check_int("blank record: field count", qs_csv_get_row_length(memory, m, 1), 1);
        check_str("blank record: value", qs_csv_get_row(memory, m, 1, 0), "");
    }

    /* --- 12. qs_csv_init + qs_csv_add_row / qs_csv_add_line ---
     * 注意: qs_csv_init 直後は内部配列が未初期化（memid_csv_array=-1）のため
     * qs_csv_get_line_length は -1 を返す。
     * 最初の qs_csv_add_row(0, ...) では current_length=-1 となり
     * for(i=-1; i<0; i++) が1回実行されて先頭に空行が1つ挿入される。 */
    {
        int32_t m = qs_csv_init(memory);
        check_int("csv_init: memid valid", (m >= 0) ? 1 : 0, 1);
        check_int("csv_init: line count (uninit)", qs_csv_get_line_length(memory, m), -1);

        // add_row で行を追加（line_pos 0,1,2）
        if(QS_SYSTEM_OK != qs_csv_add_row(memory, m, 0, "a")){ g_fail++; }
        if(QS_SYSTEM_OK != qs_csv_add_row(memory, m, 1, "b")){ g_fail++; }
        if(QS_SYSTEM_OK != qs_csv_add_row(memory, m, 2, "c")){ g_pass++; }
        check_int("csv_add_row: line count", qs_csv_get_line_length(memory, m), 3);
        check_str("csv_add_row: [0][0] (leading empty)", qs_csv_get_row(memory, m, 0, 0), "");
        check_str("csv_add_row: [1][0]", qs_csv_get_row(memory, m, 1, 0), "a");
        check_str("csv_add_row: [2][0]", qs_csv_get_row(memory, m, 2, 0), "c");

        // add_row で既存行に列を追加
        if(QS_SYSTEM_OK != qs_csv_add_row(memory, m, 0, "a2")){ g_fail++; }
        check_int("csv_add_row: row count [0]", qs_csv_get_row_length(memory, m, 0), 2);
        check_str("csv_add_row: [0][1]", qs_csv_get_row(memory, m, 0, 1), "a2");

        // add_line で行を追加
        int32_t arr = -1;
        if(-1==qs_array_push_string(memory, &arr, "x")){ g_fail++; }
        if(-1==qs_array_push_string(memory, &arr, "y")){ g_fail++; }
        if(0 != qs_csv_add_line(memory, m, arr)){ g_fail++; }
        check_int("csv_add_line: line count", qs_csv_get_line_length(memory, m), 4);
        check_int("csv_add_line: row count [3]", qs_csv_get_row_length(memory, m, 3), 2);
        check_str("csv_add_line: [3][1]", qs_csv_get_row(memory, m, 3, 1), "y");

        // build_csv で文字列化
        char* built = qs_csv_build_csv(memory, m, 1024);
        check_str("csv_init build_csv", built, "\"\",\"a2\"\n\"a\",\"b\"\n\"c\"\n\"x\",\"y\"");
    }

    /* --- 13. qs_csv_build_csv_memid --- */
    {
        int32_t m = qs_csv_parse(memory, "a,1\nb,2");
        int32_t memid_str = qs_csv_build_csv_memid(memory, m, 1024);
        check_int("build_csv_memid: memid valid", (memid_str >= 0) ? 1 : 0, 1);
        char* str = (char*)QS_GET_POINTER(memory, memid_str);
        check_str("build_csv_memid: content", str, "\"a\",1\n\"b\",2");
        // build_csv と同じ内容になること
        char* built = qs_csv_build_csv(memory, m, 1024);
        check_str("build_csv_memid: same as build_csv", str, built);
    }

    printf("\n=== csv_lowlevel_test summary ===\n");
    printf("pass: %d, fail: %d\n", g_pass, g_fail);

    qs_free(memory);
    return (g_fail == 0) ? 0 : 1;
}
