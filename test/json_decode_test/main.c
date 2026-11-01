/*
 * Copyright (c) Katsuya Owari
 *
 * json_decode_test
 *   JSON デコード（qs_json_decode_parser_hash 経由の api_qs_json_decode_object）の
 *   リグレッションテスト。
 *
 * 背景:
 *   qs_json_decode_parser_hash において、文字列値処理の if ブロックで
 *   workpos += 2 した直後に、負数チェックの if が同じループ内で再評価され、
 *   トークン位置がずれて以降のフィールドを誤解析するバグがあった。
 *   修正は if -> else if。
 *   本テストは「文字列値の後に続くフィールド」「複数フィールド」「配列フィールドの
 *   複数個」など、フィールド数に依存して壊れるケースを手厚くカバーする。
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

static void check_int(const char* name, int32_t actual, int32_t expected)
{
    if (actual != expected) {
        printf("       expected: %d\n       actual:   %d\n", expected, actual);
    }
    check(name, actual == expected);
}

static void check_int64(const char* name, int64_t actual, int64_t expected)
{
    if (actual != expected) {
        printf("       expected: %lld\n       actual:   %lld\n",
               (long long)expected, (long long)actual);
    }
    check(name, actual == expected);
}

static void check_u64(const char* name, uint64_t actual, uint64_t expected)
{
    if (actual != expected) {
        printf("       expected: %llu\n       actual:   %llu\n",
               (unsigned long long)expected, (unsigned long long)actual);
    }
    check(name, actual == expected);
}

/* デコードして失敗したら 0 を返す */
static int decode(QS_MEMORY_CONTEXT* ctx, QS_JSON_ELEMENT_OBJECT* obj, const char* json)
{
    return api_qs_json_decode_object(ctx, obj, json) == 0;
}

int main(void)
{
    api_qs_init();
    QS_MEMORY_CONTEXT memory_context;
    api_qs_memory_alloc(&memory_context, 1024 * 1024 * 8);

    /* ============================================================
     * 1. リグレッション本体: 文字列値の後に続くフィールド
     *    文字列フィールドの直後に別のフィールドが続くことで、
     *    旧バグでは workpos がずれて以降が壊れた。
     * ============================================================ */
    {
        QS_JSON_ELEMENT_OBJECT obj;
        const char* json =
            "{\"name\":\"alice\",\"age\":30,\"city\":\"tokyo\",\"score\":99}";
        check("decode multi string+int fields", decode(&memory_context, &obj, json));
        check_str("field1 name", api_qs_object_get_string(&obj, "name"), "alice");
        check_int("field2 age", api_qs_object_get_integer_val(&obj, "age"), 30);
        check_str("field3 city (after int)", api_qs_object_get_string(&obj, "city"), "tokyo");
        check_int("field4 score (after string)", api_qs_object_get_integer_val(&obj, "score"), 99);
    }

    /* ============================================================
     * 2. 文字列フィールドが連続するケース
     *    旧バグは「文字列値の直後」に発火するため、連続文字列が最も危険。
     * ============================================================ */
    {
        QS_JSON_ELEMENT_OBJECT obj;
        const char* json =
            "{\"a\":\"1\",\"b\":\"2\",\"c\":\"3\",\"d\":\"4\",\"e\":\"5\"}";
        check("decode 5 consecutive string fields", decode(&memory_context, &obj, json));
        check_str("a", api_qs_object_get_string(&obj, "a"), "1");
        check_str("b", api_qs_object_get_string(&obj, "b"), "2");
        check_str("c", api_qs_object_get_string(&obj, "c"), "3");
        check_str("d", api_qs_object_get_string(&obj, "d"), "4");
        check_str("e", api_qs_object_get_string(&obj, "e"), "5");
    }

    /* ============================================================
     * 3. issue create と同型の JSON（配列フィールドが2個）
     *    実際の不具合が顕在化した形: 文字列 + 整数 + 配列x2 + 整数
     * ============================================================ */
    {
        QS_JSON_ELEMENT_OBJECT obj;
        const char* json =
            "{"
            "\"id\":\"iss_000001\","
            "\"title\":\"fix bug\","
            "\"status\":\"open\","
            "\"priority\":2,"
            "\"assigneeIds\":[\"usr_000001\",\"usr_000002\"],"
            "\"labelIds\":[\"lbl_000001\",\"lbl_000002\",\"lbl_000003\"],"
            "\"createdAt\":1700000000,"
            "\"updatedAt\":1700000001"
            "}";
        check("decode issue-like json (8 fields, 2 arrays)", decode(&memory_context, &obj, json));
        check_str("id", api_qs_object_get_string(&obj, "id"), "iss_000001");
        check_str("title", api_qs_object_get_string(&obj, "title"), "fix bug");
        check_str("status", api_qs_object_get_string(&obj, "status"), "open");
        check_int("priority", api_qs_object_get_integer_val(&obj, "priority"), 2);

        QS_JSON_ELEMENT_ARRAY assignees;
        check("get assigneeIds array", api_qs_object_get_array(&obj, "assigneeIds", &assignees) == 0);
        check_int("assigneeIds length is 2", api_qs_array_get_length(&assignees), 2);
        check_str("assigneeIds[0]", api_qs_array_get_string(&assignees, 0), "usr_000001");
        check_str("assigneeIds[1]", api_qs_array_get_string(&assignees, 1), "usr_000002");

        QS_JSON_ELEMENT_ARRAY labels;
        check("get labelIds array", api_qs_object_get_array(&obj, "labelIds", &labels) == 0);
        check_int("labelIds length is 3", api_qs_array_get_length(&labels), 3);
        check_str("labelIds[0]", api_qs_array_get_string(&labels, 0), "lbl_000001");
        check_str("labelIds[2]", api_qs_array_get_string(&labels, 2), "lbl_000003");

        check_int("createdAt", api_qs_object_get_integer_val(&obj, "createdAt"), 1700000000);
        check_int("updatedAt (last field after arrays)", api_qs_object_get_integer_val(&obj, "updatedAt"), 1700000001);
    }

    /* ============================================================
     * 4. 負数値（文字列値の直後に負数が来るケース）
     *    旧バグの負数チェック if が誤発火しやすかった位置を直接叩く。
     * ============================================================ */
    {
        QS_JSON_ELEMENT_OBJECT obj;
        const char* json =
            "{\"label\":\"neg\",\"delta\":-5,\"ratio\":-12345,\"note\":\"after\"}";
        check("decode negative int fields", decode(&memory_context, &obj, json));
        check_str("label", api_qs_object_get_string(&obj, "label"), "neg");
        check_int("delta == -5", api_qs_object_get_integer_val(&obj, "delta"), -5);
        check_int("ratio == -12345", api_qs_object_get_integer_val(&obj, "ratio"), -12345);
        check_str("note (after negatives)", api_qs_object_get_string(&obj, "note"), "after");
    }

    /* ============================================================
     * 5. 大整数（正: unsigned big / 負: signed big）
     *    ID_NUM_U64 経路と負の ID_NUM_U64 経路を両方カバー。
     * ============================================================ */
    {
        QS_JSON_ELEMENT_OBJECT obj;
        const char* json =
            "{\"big\":4294967296,\"negbig\":-4294967296,\"tail\":\"ok\"}";
        check("decode big integer fields", decode(&memory_context, &obj, json));
        check_u64("big == 4294967296", api_qs_object_get_unsigned_big_integer_val(&obj, "big"), 4294967296ULL);
        check_int64("negbig == -4294967296", api_qs_object_get_big_integer_val(&obj, "negbig"), -4294967296LL);
        check_str("tail (after big ints)", api_qs_object_get_string(&obj, "tail"), "ok");
    }

    /* ============================================================
     * 6. 各種値タイプを1つのオブジェクトに混在
     *    string / int / negative / bool / null / array / object
     * ============================================================ */
    {
        QS_JSON_ELEMENT_OBJECT obj;
        const char* json =
            "{"
            "\"s\":\"str\","
            "\"i\":7,"
            "\"n\":-8,"
            "\"t\":true,"
            "\"f\":false,"
            "\"z\":null,"
            "\"arr\":[1,2,3],"
            "\"obj\":{\"k\":\"v\"}"
            "}";
        check("decode mixed-type object", decode(&memory_context, &obj, json));
        check_str("s", api_qs_object_get_string(&obj, "s"), "str");
        check_int("i", api_qs_object_get_integer_val(&obj, "i"), 7);
        check_int("n", api_qs_object_get_integer_val(&obj, "n"), -8);
        check_int("t (true->1)", api_qs_object_get_integer_val(&obj, "t"), 1);
        check_int("f (false->0)", api_qs_object_get_integer_val(&obj, "f"), 0);

        QS_JSON_ELEMENT_ARRAY arr;
        check("get arr", api_qs_object_get_array(&obj, "arr", &arr) == 0);
        check_int("arr length is 3", api_qs_array_get_length(&arr), 3);
        check_int("arr[2] == 3", *api_qs_array_get_integer(&arr, 2), 3);

        QS_JSON_ELEMENT_OBJECT sub;
        check("get obj", api_qs_object_get_object(&obj, "obj", &sub) == 0);
        check_str("obj.k", api_qs_object_get_string(&sub, "k"), "v");
    }

    /* ============================================================
     * 7. 多数フィールド（10個）で workpos 進みの累積誤差を検出
     * ============================================================ */
    {
        QS_JSON_ELEMENT_OBJECT obj;
        const char* json =
            "{"
            "\"f1\":\"v1\",\"f2\":1,"
            "\"f3\":\"v3\",\"f4\":4,"
            "\"f5\":\"v5\",\"f6\":6,"
            "\"f7\":\"v7\",\"f8\":8,"
            "\"f9\":\"v9\",\"f10\":10"
            "}";
        check("decode 10-field object", decode(&memory_context, &obj, json));
        check_str("f1", api_qs_object_get_string(&obj, "f1"), "v1");
        check_int("f2", api_qs_object_get_integer_val(&obj, "f2"), 1);
        check_str("f5", api_qs_object_get_string(&obj, "f5"), "v5");
        check_int("f8", api_qs_object_get_integer_val(&obj, "f8"), 8);
        check_str("f9", api_qs_object_get_string(&obj, "f9"), "v9");
        check_int("f10 (last)", api_qs_object_get_integer_val(&obj, "f10"), 10);
    }

    /* ============================================================
     * 8. 入れ子オブジェクト（各層が複数フィールド）
     * ============================================================ */
    {
        QS_JSON_ELEMENT_OBJECT obj;
        const char* json =
            "{"
            "\"top\":\"t\","
            "\"mid\":{\"a\":\"1\",\"b\":2,\"c\":\"3\"},"
            "\"deep\":{\"inner\":{\"x\":\"9\",\"y\":10}},"
            "\"end\":\"e\""
            "}";
        check("decode nested multi-field objects", decode(&memory_context, &obj, json));
        check_str("top", api_qs_object_get_string(&obj, "top"), "t");

        QS_JSON_ELEMENT_OBJECT mid;
        check("get mid", api_qs_object_get_object(&obj, "mid", &mid) == 0);
        check_str("mid.a", api_qs_object_get_string(&mid, "a"), "1");
        check_int("mid.b", api_qs_object_get_integer_val(&mid, "b"), 2);
        check_str("mid.c", api_qs_object_get_string(&mid, "c"), "3");

        QS_JSON_ELEMENT_OBJECT deep;
        check("get deep", api_qs_object_get_object(&obj, "deep", &deep) == 0);
        QS_JSON_ELEMENT_OBJECT inner;
        check("get deep.inner", api_qs_object_get_object(&deep, "inner", &inner) == 0);
        check_str("deep.inner.x", api_qs_object_get_string(&inner, "x"), "9");
        check_int("deep.inner.y", api_qs_object_get_integer_val(&inner, "y"), 10);

        check_str("end (after nested)", api_qs_object_get_string(&obj, "end"), "e");
    }

    /* ============================================================
     * 9. オブジェクトの配列（各要素が複数フィールド）
     * ============================================================ */
    {
        QS_JSON_ELEMENT_OBJECT obj;
        const char* json =
            "{\"list\":[{\"id\":\"a\",\"n\":1},{\"id\":\"b\",\"n\":2},{\"id\":\"c\",\"n\":3}]}";
        check("decode array of multi-field objects", decode(&memory_context, &obj, json));

        QS_JSON_ELEMENT_ARRAY list;
        check("get list", api_qs_object_get_array(&obj, "list", &list) == 0);
        check_int("list length is 3", api_qs_array_get_length(&list), 3);

        QS_JSON_ELEMENT_OBJECT e0, e2;
        check("get list[0]", api_qs_array_get_object(&list, 0, &e0) == 0);
        check_str("list[0].id", api_qs_object_get_string(&e0, "id"), "a");
        check_int("list[0].n", api_qs_object_get_integer_val(&e0, "n"), 1);

        check("get list[2]", api_qs_array_get_object(&list, 2, &e2) == 0);
        check_str("list[2].id", api_qs_object_get_string(&e2, "id"), "c");
        check_int("list[2].n", api_qs_object_get_integer_val(&e2, "n"), 3);
    }

    /* ============================================================
     * 10. 空オブジェクト / 空配列 / 単一フィールド
     * ============================================================ */
    {
        QS_JSON_ELEMENT_OBJECT obj;
        check("decode empty object", decode(&memory_context, &obj, "{}"));

        QS_JSON_ELEMENT_OBJECT obj2;
        check("decode single field", decode(&memory_context, &obj2, "{\"only\":1}"));
        check_int("only", api_qs_object_get_integer_val(&obj2, "only"), 1);

        QS_JSON_ELEMENT_OBJECT obj3;
        const char* json3 = "{\"emptyArr\":[],\"emptyObj\":{},\"after\":\"x\"}";
        check("decode with empty array/object", decode(&memory_context, &obj3, json3));
        QS_JSON_ELEMENT_ARRAY ea;
        check("get emptyArr", api_qs_object_get_array(&obj3, "emptyArr", &ea) == 0);
        check_int("emptyArr length is 0", api_qs_array_get_length(&ea), 0);
        check_str("after (after empty containers)", api_qs_object_get_string(&obj3, "after"), "x");
    }

    /* ============================================================
     * 10b. issue_tracker_cli が実際に使う JSON をそのままデコード
     *      create / update の実 JSON（整数なし・配列1要素）を再現。
     * ============================================================ */
    {
        QS_JSON_ELEMENT_OBJECT obj;
        const char* create_json =
            "{\"id\":\"iss1\",\"title\":\"bug\",\"status\":\"open\",\"assigneeIds\":[\"u1\"],\"labelIds\":[\"bug\"]}";
        check("decode CLI create json", decode(&memory_context, &obj, create_json));
        check_str("cli id", api_qs_object_get_string(&obj, "id"), "iss1");
        check_str("cli title", api_qs_object_get_string(&obj, "title"), "bug");
        check_str("cli status", api_qs_object_get_string(&obj, "status"), "open");
        QS_JSON_ELEMENT_ARRAY ca;
        check("cli assigneeIds", api_qs_object_get_array(&obj, "assigneeIds", &ca) == 0);
        check_int("cli assigneeIds len 1", api_qs_array_get_length(&ca), 1);
        check_str("cli assigneeIds[0]", api_qs_array_get_string(&ca, 0), "u1");
        QS_JSON_ELEMENT_OBJECT obj2;
        const char* update_json =
            "{\"id\":\"iss1\",\"title\":\"bug2\",\"status\":\"in_progress\",\"assigneeIds\":[\"u2\"],\"labelIds\":[\"feature\"]}";
        check("decode CLI update json", decode(&memory_context, &obj2, update_json));
        check_str("cli2 title", api_qs_object_get_string(&obj2, "title"), "bug2");
        check_str("cli2 status", api_qs_object_get_string(&obj2, "status"), "in_progress");
        QS_JSON_ELEMENT_ARRAY la;
        check("cli2 labelIds", api_qs_object_get_array(&obj2, "labelIds", &la) == 0);
        check_str("cli2 labelIds[0]", api_qs_array_get_string(&la, 0), "feature");
    }

    /* ============================================================
     * 10c. バグ再現の正確な JSON（ユーザー報告）
     *      整数フィールド(updatedAt/priority)と配列が混在し、
     *      文字列値の直後に整数が続く配置が旧バグをトリガーした。
     * ============================================================ */
    {
        QS_JSON_ELEMENT_OBJECT obj;
        const char* bug_json =
            "{\"labelIds\":[\"bug\"],\"updatedAt\":0,\"description\":\"\","
            "\"priority\":0,\"id\":\"iss5\",\"title\":\"bug\","
            "\"assigneeIds\":[\"u1\"],\"status\":\"open\"}";
        check("decode reported bug json", decode(&memory_context, &obj, bug_json));
        check_str("bug id", api_qs_object_get_string(&obj, "id"), "iss5");
        check_str("bug title", api_qs_object_get_string(&obj, "title"), "bug");
        check_str("bug status", api_qs_object_get_string(&obj, "status"), "open");
        check_str("bug description (empty)", api_qs_object_get_string(&obj, "description"), "");
        check_int("bug priority == 0", api_qs_object_get_integer_val(&obj, "priority"), 0);
        check_int("bug updatedAt == 0", api_qs_object_get_integer_val(&obj, "updatedAt"), 0);
        QS_JSON_ELEMENT_ARRAY bl;
        check("bug labelIds", api_qs_object_get_array(&obj, "labelIds", &bl) == 0);
        check_str("bug labelIds[0]", api_qs_array_get_string(&bl, 0), "bug");
        QS_JSON_ELEMENT_ARRAY ba;
        check("bug assigneeIds", api_qs_object_get_array(&obj, "assigneeIds", &ba) == 0);
        check_str("bug assigneeIds[0]", api_qs_array_get_string(&ba, 0), "u1");
    }

    /* ============================================================
     * 10d. バグ再現の正確な JSON（長い文字列 title を含む・ユーザー報告）
     *      長い文字列値の直後に別のフィールドが続く配置が旧バグをトリガーした。
     * ============================================================ */
    {
        QS_JSON_ELEMENT_OBJECT obj;
        const char* bug_json =
            "{\"labelIds\":[\"bug\"],\"updatedAt\":0,\"description\":\"\","
            "\"priority\":0,\"id\":\"iss5\","
            "\"title\":\"bugaaaaaaaaaaaaaaaaaaaaaaaaaaa\","
            "\"assigneeIds\":[\"u1\"],\"status\":\"open\"}";
        check("decode reported bug json (long title)", decode(&memory_context, &obj, bug_json));
        check_str("bug id", api_qs_object_get_string(&obj, "id"), "iss5");
        check_str("bug title (long)", api_qs_object_get_string(&obj, "title"),
                  "bugaaaaaaaaaaaaaaaaaaaaaaaaaaa");
        check_str("bug status", api_qs_object_get_string(&obj, "status"), "open");
        check_int("bug priority == 0", api_qs_object_get_integer_val(&obj, "priority"), 0);
        check_int("bug updatedAt == 0", api_qs_object_get_integer_val(&obj, "updatedAt"), 0);
        QS_JSON_ELEMENT_ARRAY bl;
        check("bug labelIds", api_qs_object_get_array(&obj, "labelIds", &bl) == 0);
        check_str("bug labelIds[0]", api_qs_array_get_string(&bl, 0), "bug");
        QS_JSON_ELEMENT_ARRAY ba;
        check("bug assigneeIds", api_qs_object_get_array(&obj, "assigneeIds", &ba) == 0);
        check_str("bug assigneeIds[0]", api_qs_array_get_string(&ba, 0), "u1");
    }

    /* ============================================================
     * 11. 不正 JSON の扱い
     *     本デコーダは構文検証が緩く、コロン欠落・閉じ括弧なしは
     *     現状「受入」される（既存の寛容性。今回の修正とは無関係）。
     *     ここでは実際の挙動を固定し、将来パーサが厳格化された際に
     *     挙動変化を検知できるようにする。
     * ============================================================ */
    {
        QS_JSON_ELEMENT_OBJECT obj;
        check("trailing garbage is rejected", decode(&memory_context, &obj, "{\"a\":1}x") != 0);
        /* 現状は寛容に受入される（厳格化されたらこの行が失敗する） */
        check("missing colon currently accepted (lenient)", decode(&memory_context, &obj, "{\"a\" 1}") == 0);
        check("unclosed object currently accepted (lenient)", decode(&memory_context, &obj, "{\"a\":1") == 0);
    }

    api_qs_memory_free(&memory_context);

    printf("\n=== json_decode_test summary ===\n");
    printf("pass: %d, fail: %d\n", pass_count, fail_count);
    return (fail_count > 0) ? 1 : 0;
}
