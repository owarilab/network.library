/*
 * Copyright (c) Katsuya Owari
 *
 * hash_collision_delete_test
 *   ハッシュ衝突バケット内の要素削除後、後続要素の更新で重複要素が
 *   生成されないことを検証する最小単体テスト。
 *
 * 背景:
 *   qs_add_hash はバケット要素配列を走査し、最初の空きスロットで挿入を止める。
 *   衝突バケットの先頭要素を削除すると穴が先行し、既存キーの再追加が
 *   「更新」ではなく「重複挿入」になる。
 *   core/src/qs_hash.c の qs_remove_hash 修正 (commit b4c5c25) は、
 *   削除位置より後ろの同一バケット要素を再挿入してクラスタをコンパクト化し、
 *   この重複を防止する。
 *
 * 対象API:
 *   qs_create_hash / qs_add_hash_string / qs_remove_hash
 *   qs_get_hash_string / qs_hash_length / qs_ihash
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "qs_hash.h"
#include "qs_memory_allocator.h"
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

/*
 * qs_ihash で同一バケットに衝突する 3 つの異なるキーを探索する。
 * hash_size が小さいほど衝突しやすいため、hash_size=4 を想定。
 * 見つかった 3 キーを keys[0..2] に格納し、成功で 1 を返す。
 */
static int find_colliding_keys(uint32_t hash_size, char keys[3][64])
{
    static char bucket_names[256][3][64];
    static int bucket_count[256];
    int i;
    for (i = 0; i < 256; i++) {
        bucket_count[i] = 0;
    }
    for (i = 0; i < 1000000; i++) {
        char name[64];
        uint32_t b;
        snprintf(name, sizeof(name), "key_%d", i);
        b = qs_ihash(name, hash_size);
        if (b >= 256) {
            continue;
        }
        if (bucket_count[b] < 3) {
            strcpy(bucket_names[b][bucket_count[b]], name);
            bucket_count[b]++;
        }
        if (bucket_count[b] == 3) {
            strcpy(keys[0], bucket_names[b][0]);
            strcpy(keys[1], bucket_names[b][1]);
            strcpy(keys[2], bucket_names[b][2]);
            return 1;
        }
    }
    return 0;
}

int main(void)
{
    QS_MEMORY_POOL* memory = NULL;
    uint32_t hash_size = 4;
    int32_t memid_hash = -1;
    char keys[3][64];

    if (qs_initialize_memory_f64(&memory, 1024 * 1024 * 8) <= 0) {
        printf("[FAIL] init memory\n");
        return 1;
    }

    memid_hash = qs_create_hash(memory, hash_size);
    check("create hash", memid_hash != -1);
    if (memid_hash == -1) {
        qs_free(memory);
        return 1;
    }

    if (!find_colliding_keys(hash_size, keys)) {
        printf("[FAIL] find colliding keys\n");
        qs_free(memory);
        return 1;
    }
    printf("colliding keys: %s, %s, %s (bucket=%u)\n",
           keys[0], keys[1], keys[2], qs_ihash(keys[0], hash_size));

    /* 3 キーを同一バケットに追加 */
    qs_add_hash_string(memory, memid_hash, keys[0], "value0");
    qs_add_hash_string(memory, memid_hash, keys[1], "value1");
    qs_add_hash_string(memory, memid_hash, keys[2], "value2");
    check("3 keys added, length==3", qs_hash_length(memory, memid_hash) == 3);

    /* 衝突クラスタの先頭キーを削除 */
    qs_remove_hash(memory, memid_hash, keys[0]);
    check("after delete, length==2", qs_hash_length(memory, memid_hash) == 2);
    check("deleted key gone",
          qs_get_hash_string(memory, memid_hash, keys[0]) == NULL);

    /* 後続キーを更新（再追加）。修正前は穴の先頭に重複挿入される */
    qs_add_hash_string(memory, memid_hash, keys[1], "value1_updated");

    check("after update, length==2 (no duplicate)",
          qs_hash_length(memory, memid_hash) == 2);

    {
        char* v1 = qs_get_hash_string(memory, memid_hash, keys[1]);
        check("updated value readable",
              v1 != NULL && strcmp(v1, "value1_updated") == 0);
    }
    {
        char* v2 = qs_get_hash_string(memory, memid_hash, keys[2]);
        check("third key still readable",
              v2 != NULL && strcmp(v2, "value2") == 0);
    }

    qs_free(memory);

    printf("\nresult: pass=%d fail=%d\n", pass_count, fail_count);
    return fail_count == 0 ? 0 : 1;
}
