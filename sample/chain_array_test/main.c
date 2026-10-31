/*
 * Copyright (c) Katsuya Owari
 *
 * chain_array_test
 *   qs_chain_array (free list ベースの doubly-linked list) の低レベルAPIをテストする。
 *
 * 対象API:
 *   qs_create_chain_array / qs_get_chain_size / qs_get_chain_length / qs_get_chain_start
 *   qs_add_chain / qs_add_chain_i / qs_get_chain / qs_get_chain_i
 *   qs_remove_chain / qs_resize_chain_array
 *
 * 補足:
 *   qs_create_chain_array(memory, N, data_size) は物理スロット N+1 個を確保する。
 *   qs_add_chain は N 回まで成功し、N+1 回目で NULL を返す。
 *   qs_get_chain_i は物理インデックスアクセス（0..N）。
 */

#include <stdio.h>
#include <string.h>

#include "qs_chain_array.h"

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

#define DATA_SIZE sizeof(int32_t)

int main(void)
{
    QS_MEMORY_POOL* memory = NULL;
    if (qs_initialize_memory_f64(&memory, 1024 * 1024 * 8) <= 0) {
        printf("memory init failed\n");
        return 1;
    }

    /* --- create --- */
    int32_t chain_id = qs_create_chain_array(memory, 8, DATA_SIZE);
    check("create returns valid id", chain_id != -1);

    /* --- initial state --- */
    check("initial length is 0", qs_get_chain_length(memory, chain_id) == 0);
    check("initial size is 9 (8+1)", qs_get_chain_size(memory, chain_id) == 9);
    check("initial start is 0", qs_get_chain_start(memory, chain_id) == 0);

    /* --- add 8 slots --- */
    int32_t* slots[8];
    int all_added = 1;
    int i;
    for (i = 0; i < 8; i++) {
        void* p = qs_add_chain(memory, chain_id);
        if (p == NULL) { all_added = 0; break; }
        slots[i] = (int32_t*)p;
        *slots[i] = i * 100;
    }
    check("add 8 slots all succeed", all_added);
    check("length is 8 after add", qs_get_chain_length(memory, chain_id) == 8);

    /* --- add overflow --- */
    check("9th add returns NULL", qs_add_chain(memory, chain_id) == NULL);

    /* --- iterate (order + values) --- */
    {
        int idx = 0;
        int ok = 1;
        void* current = NULL;
        while (NULL != (current = qs_get_chain(memory, chain_id, current))) {
            if (idx >= 8) { ok = 0; break; }
            if (*(int32_t*)current != idx * 100) { ok = 0; break; }
            idx++;
        }
        check("iterate returns 8 slots in order with correct values", ok && idx == 8);
    }

    /* --- get by physical index --- */
    {
        int ok = 1;
        for (i = 0; i < 8; i++) {
            void* p = qs_get_chain_i(memory, chain_id, i);
            if (p == NULL || *(int32_t*)p != i * 100) { ok = 0; break; }
        }
        check("get_chain_i returns correct slot by index", ok);
    }

    /* --- remove middle (physical index 3) --- */
    {
        void* to_remove = qs_get_chain_i(memory, chain_id, 3);
        int32_t removed_value = *(int32_t*)to_remove;
        check("slot 3 value is 300 before remove", removed_value == 300);

        int rm = qs_remove_chain(memory, chain_id, to_remove);
        check("remove_chain returns 1", rm == 1);
        check("length is 7 after remove", qs_get_chain_length(memory, chain_id) == 7);

        /* 300 が消え、他は残っていること（集合として検証） */
        int seen[8];
        int count = 0;
        int ok = 1;
        void* current = NULL;
        memset(seen, 0, sizeof(seen));
        while (NULL != (current = qs_get_chain(memory, chain_id, current))) {
            int32_t v = *(int32_t*)current;
            if (v == 300) { ok = 0; }
            int slot_idx = v / 100;
            if (slot_idx >= 0 && slot_idx < 8) seen[slot_idx] = 1;
            count++;
        }
        check("after remove: 7 slots, 300 absent, others present",
              ok && count == 7 &&
              seen[0] && seen[1] && seen[2] && !seen[3] &&
              seen[4] && seen[5] && seen[6] && seen[7]);
    }

    /* --- add after remove --- */
    {
        void* p = qs_add_chain(memory, chain_id);
        check("add after remove returns valid slot", p != NULL);
        if (p != NULL) {
            *(int32_t*)p = 999;
        }
        check("length is 8 after re-add", qs_get_chain_length(memory, chain_id) == 8);
    }

    /* --- remove start slot --- */
    {
        void* start = qs_get_chain(memory, chain_id, NULL);
        check("start slot is not NULL", start != NULL);
        if (start != NULL) {
            int rm = qs_remove_chain(memory, chain_id, start);
            check("remove start slot returns 1", rm == 1);
            check("length is 7 after removing start", qs_get_chain_length(memory, chain_id) == 7);
        }
    }

    /* --- resize --- */
    {
        int32_t rc = qs_create_chain_array(memory, 4, DATA_SIZE);
        check("resize: create chain (4)", rc != -1);
        int32_t* rslots[4];
        for (i = 0; i < 3; i++) {
            rslots[i] = (int32_t*)qs_add_chain(memory, rc);
            *rslots[i] = i + 1;
        }
        check("resize: added 3 slots", qs_get_chain_length(memory, rc) == 3);

        int32_t new_rc = qs_resize_chain_array(memory, rc);
        check("resize: returns valid id", new_rc != -1);
        check("resize: new size is 9 (8+1)", qs_get_chain_size(memory, new_rc) == 9);
        check("resize: length preserved (3)", qs_get_chain_length(memory, new_rc) == 3);

        /* 値が保持されていること */
        int ok = 1;
        void* current = NULL;
        int count = 0;
        while (NULL != (current = qs_get_chain(memory, new_rc, current))) {
            int32_t v = *(int32_t*)current;
            if (v != 1 && v != 2 && v != 3) { ok = 0; }
            count++;
        }
        check("resize: values preserved", ok && count == 3);

        /* 合計 8 個まで追加できること */
        int can_add = 1;
        for (i = 0; i < 5; i++) {
            if (qs_add_chain(memory, new_rc) == NULL) { can_add = 0; break; }
        }
        check("resize: can add 5 more (total 8)", can_add && qs_get_chain_length(memory, new_rc) == 8);

        /* 旧ブロックを解放 */
        qs_free_memory_block(memory, &rc);
    }

    qs_free(memory);

    printf("\n=== chain_array_test summary ===\n");
    printf("pass: %d, fail: %d\n", pass_count, fail_count);
    return (fail_count > 0) ? 1 : 0;
}
