/*
 * Copyright (c) Katsuya Owari
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

    /* --- 7. 全フィールドが空 (,,) ---
     * パーサー仕様: 先頭にデータなしで始まるカンマ区切りは
     * 末尾の空フィールドが追加されないため 1フィールドになる */
    {
        int32_t m = qs_csv_parse(memory, ",,");
        check_int("all empty ,, : row count (1 due to trailing-empty limitation)", qs_csv_get_row_length(memory, m, 0), 1);
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

    printf("\n=== Result: %d passed, %d failed ===\n", g_pass, g_fail);

    qs_free(memory);
    return (g_fail == 0) ? 0 : 1;
}
