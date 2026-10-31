/*
 * Copyright (c) Katsuya Owari
 *
 * ranking_ext_test
 *   高レベルランキングAPI（api_qs_ranking_*）の未カバー機能をテストする。
 *
 * 対象API（既存 ranking サンプルで未カバー）:
 *   api_qs_ranking_add_value（set_value のみ使用されていた）
 *   api_qs_ranking_get の offset / length 指定
 *
 * 検証内容:
 *   - entry / set_value / add_value / sort / get
 *   - 降順ソート後の順位（ranking 1..N）
 *   - add_value による値の加算
 *   - offset / length による部分取得
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

/* JSON 内で a が b より先に出ているか */
static int json_before(const char* json, const char* a, const char* b)
{
    const char* pa = strstr(json, a);
    const char* pb = strstr(json, b);
    if (pa == NULL || pb == NULL) return 0;
    return pa < pb;
}

int main(void)
{
    api_qs_init();
    QS_MEMORY_CONTEXT memory;
    QS_MEMORY_CONTEXT dest_memory;
    QS_RANKING_CONTEXT ranking;

    if (api_qs_memory_alloc(&memory, 1024 * 1024 * 8) != 0) return 1;
    if (api_qs_memory_alloc(&dest_memory, 1024 * 1024 * 2) != 0) {
        api_qs_memory_free(&memory);
        return 1;
    }

    /* --- create --- */
    int rc = api_qs_ranking_create(&memory, &ranking, 100, 32, 10, 10);
    check("ranking_create returns 0", rc == 0);

    /* --- entry + set_value --- */
    const char* users[] = {"player1", "player2", "player3", "player4", "player5"};
    uint32_t scores[] = {100, 250, 180, 320, 95};
    int all_entry = 1;
    for (int i = 0; i < 5; i++) {
        if (api_qs_ranking_entry(&ranking, users[i]) != 0) all_entry = 0;
        if (api_qs_ranking_set_value(&ranking, users[i], scores[i]) != 0) all_entry = 0;
    }
    check("entry + set_value for 5 users", all_entry);

    /* --- add_value (未カバーAPI) --- */
    rc = api_qs_ranking_add_value(&ranking, "player1", 50);
    check("add_value returns 0", rc == 0);
    /* player1: 100 + 50 = 150 */

    /* --- sort --- */
    rc = api_qs_ranking_sort(&ranking);
    check("ranking_sort returns 0", rc == 0);

    /* --- get (全件) --- */
    char* json = api_qs_ranking_get(&ranking, 0, 5, &dest_memory);
    check("ranking_get returns JSON", json != NULL);
    if (json != NULL) {
        /* 降順: player4(320) > player2(250) > player3(180) > player1(150) > player5(95) */
        check("rank order player4 > player2", json_before(json, "player4", "player2"));
        check("rank order player2 > player3", json_before(json, "player2", "player3"));
        check("rank order player3 > player1", json_before(json, "player3", "player1"));
        check("rank order player1 > player5", json_before(json, "player1", "player5"));

        /* 値の確認 */
        check("player4 value 320 present", strstr(json, "320") != NULL);
        check("player1 value 150 present (after add)", strstr(json, "150") != NULL);
        check("player5 value 95 present", strstr(json, "95") != NULL);

        /* ranking フィールドの確認（1位〜5位） */
        check("ranking 1 present", strstr(json, "\"ranking\":1") != NULL || strstr(json, "\"ranking\": 1") != NULL);
    }

    /* --- get (offset / length 指定) --- */
    api_qs_memory_clean(&dest_memory);
    char* json_top2 = api_qs_ranking_get(&ranking, 0, 2, &dest_memory);
    check("ranking_get offset=0 length=2 returns JSON", json_top2 != NULL);
    if (json_top2 != NULL) {
        check("top2 contains player4", strstr(json_top2, "player4") != NULL);
        check("top2 contains player2", strstr(json_top2, "player2") != NULL);
        check("top2 does not contain player5", strstr(json_top2, "player5") == NULL);
    }

    api_qs_memory_clean(&dest_memory);
    char* json_mid = api_qs_ranking_get(&ranking, 2, 2, &dest_memory);
    check("ranking_get offset=2 length=2 returns JSON", json_mid != NULL);
    if (json_mid != NULL) {
        check("mid contains player3", strstr(json_mid, "player3") != NULL);
        check("mid contains player1", strstr(json_mid, "player1") != NULL);
        check("mid does not contain player4", strstr(json_mid, "player4") == NULL);
    }

    /* --- get (無効パラメータ) --- */
    check("ranking_get length=0 returns NULL", api_qs_ranking_get(&ranking, 0, 0, &dest_memory) == NULL);
    check("ranking_get offset<0 returns NULL", api_qs_ranking_get(&ranking, -1, 1, &dest_memory) == NULL);

    api_qs_memory_free(&memory);
    api_qs_memory_free(&dest_memory);

    printf("\n=== ranking_ext_test summary ===\n");
    printf("pass: %d, fail: %d\n", pass_count, fail_count);
    return (fail_count > 0) ? 1 : 0;
}
