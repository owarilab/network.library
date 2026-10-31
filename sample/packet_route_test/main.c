/*
 * Copyright (c) Katsuya Owari
 *
 * packet_route_test
 *   qs_packet_route (パケットルーティング / room機能の土台) の低レベルAPIをテストする。
 *
 * 対象API:
 *   qs_init_packet_route / qs_create_packet_route / qs_get_packet_route
 *   qs_set_packet_route_owner / qs_get_packet_route_owner
 *   qs_add_packet_route_connection / qs_get_packet_route_connection_chain
 *   qs_get_packet_route_connection_offset
 *   qs_change_packet_route_connection_id / qs_get_packet_route_connection_id
 *   qs_find_packet_route_connection_id
 *   qs_set_connection_data / qs_get_connection_data
 *   qs_set_route_data / qs_get_route_data
 *   qs_remove_packet_route_connection / qs_remove_packet_route
 *   qs_update_packet_route (life_time による自動クリーンアップ)
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "qs_packet_route.h"

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

#define MAX_CONNECTION 10
#define MAX_ROUTE_CHAIN 10
#define KEY_SIZE 20
#define DATA_SIZE 100
#define CON_DATA_SIZE 50
#define ROUTE_CAPACITY 5

int main(void)
{
    QS_MEMORY_POOL* memory = NULL;
    if (qs_initialize_memory_f64(&memory, 1024 * 1024 * 8) <= 0) {
        printf("memory init failed\n");
        return 1;
    }

    /* --- init --- */
    int32_t pr_id = qs_init_packet_route(memory, MAX_CONNECTION, MAX_ROUTE_CHAIN, KEY_SIZE, DATA_SIZE);
    check("init_packet_route returns valid id", pr_id != -1);

    /* --- create route --- */
    int32_t route_offset = qs_create_packet_route(memory, pr_id, (char*)"room1", ROUTE_CAPACITY, -1, CON_DATA_SIZE);
    check("create_packet_route returns valid offset", route_offset != -1);

    /* --- get route by name --- */
    int32_t found = qs_get_packet_route(memory, pr_id, (char*)"room1");
    check("get_packet_route finds room1", found == route_offset);

    int32_t not_found = qs_get_packet_route(memory, pr_id, (char*)"no_such_room");
    check("get_packet_route returns -1 for unknown", not_found == -1);

    /* --- set/get owner --- */
    int32_t set_owner_rc = qs_set_packet_route_owner(memory, pr_id, (char*)"room1", 42);
    check("set_packet_route_owner returns offset", set_owner_rc == route_offset);
    int64_t owner = qs_get_packet_route_owner(memory, pr_id, (char*)"room1");
    check("get_packet_route_owner returns 42", owner == 42);

    /* --- add connections --- */
    int32_t con0 = qs_add_packet_route_connection(memory, pr_id, route_offset, 0);
    check("add connection 0", con0 != -1);
    int32_t con1 = qs_add_packet_route_connection(memory, pr_id, route_offset, 1);
    check("add connection 1", con1 != -1);
    int32_t con2 = qs_add_packet_route_connection(memory, pr_id, route_offset, 2);
    check("add connection 2", con2 != -1);

    /* --- get connection chain --- */
    void* chain0 = qs_get_packet_route_connection_chain(memory, pr_id, 0);
    check("get connection chain 0 not NULL", chain0 != NULL);
    if (chain0 != NULL) {
        QS_PACKET_ROUTE_NODE_CONNECTION* con = (QS_PACKET_ROUTE_NODE_CONNECTION*)chain0;
        check("connection 0 index is 0", con->connection_index == 0);
    }

    /* --- get connection offset --- */
    int32_t con_offset = qs_get_packet_route_connection_offset(memory, pr_id, 0);
    check("get connection offset matches route", con_offset == route_offset);

    /* --- connection id --- */
    char* id0 = qs_change_packet_route_connection_id(memory, pr_id, 0);
    check("change connection id returns non-empty", id0 != NULL && strlen(id0) > 0);

    char* id0_get = qs_get_packet_route_connection_id(memory, pr_id, 0);
    check("get connection id matches", id0_get != NULL && strcmp(id0_get, id0) == 0);

    int32_t found_idx = qs_find_packet_route_connection_id(memory, pr_id, id0);
    check("find connection id returns 0", found_idx == 0);

    int32_t not_found_idx = qs_find_packet_route_connection_id(memory, pr_id, (char*)"nonexistent_id");
    check("find connection id returns -1 for unknown", not_found_idx == -1);

    /* --- set/get connection data --- */
    {
        uint8_t data[] = "hello con";
        int rc = qs_set_connection_data(memory, pr_id, 0, data, strlen((char*)data));
        check("set_connection_data returns 0", rc == 0);

        uint8_t* got = qs_get_connection_data(memory, pr_id, 0);
        check("get_connection_data not NULL", got != NULL);
        if (got != NULL) {
            check("connection data matches", memcmp(got, data, strlen((char*)data)) == 0);
        }
    }

    /* --- set/get route data --- */
    {
        uint8_t data[] = "route data";
        int rc = qs_set_route_data(memory, pr_id, route_offset, data, strlen((char*)data));
        check("set_route_data returns 0", rc == 0);

        uint8_t* got = qs_get_route_data(memory, pr_id, route_offset);
        check("get_route_data not NULL", got != NULL);
        if (got != NULL) {
            check("route data matches", memcmp(got, data, strlen((char*)data)) == 0);
        }
    }

    /* --- remove connection --- */
    {
        int rc = qs_remove_packet_route_connection(memory, pr_id, 2);
        check("remove connection 2 returns 0", rc == 0);

        void* chain2 = qs_get_packet_route_connection_chain(memory, pr_id, 2);
        check("removed connection chain is NULL", chain2 == NULL);
    }

    /* --- remove route --- */
    {
        int rc = qs_remove_packet_route(memory, pr_id, route_offset);
        check("remove_packet_route returns 0", rc == 0);

        int32_t found_after = qs_get_packet_route(memory, pr_id, (char*)"room1");
        check("route gone after remove", found_after == -1);
    }

    /* --- life_time auto cleanup --- */
    {
        /* life_time=0 の route を作成（接続なし） */
        int32_t rt2 = qs_create_packet_route(memory, pr_id, (char*)"room2", ROUTE_CAPACITY, 0, 0);
        check("create room2 (life_time=0)", rt2 != -1);

        int32_t found2 = qs_get_packet_route(memory, pr_id, (char*)"room2");
        check("room2 exists before update", found2 != -1);

        /* update で期限切れ route が削除される */
        int rc = qs_update_packet_route(memory, pr_id);
        check("update_packet_route returns 1 (cleaned up)", rc == 1);

        int32_t found2_after = qs_get_packet_route(memory, pr_id, (char*)"room2");
        check("room2 gone after update", found2_after == -1);
    }

    /* --- create second route (independent) --- */
    {
        int32_t rt3 = qs_create_packet_route(memory, pr_id, (char*)"room3", 3, -1, 0);
        check("create room3", rt3 != -1);

        int32_t con3 = qs_add_packet_route_connection(memory, pr_id, rt3, 3);
        check("add connection to room3", con3 != -1);

        int32_t found3 = qs_get_packet_route(memory, pr_id, (char*)"room3");
        check("room3 found", found3 == rt3);

        qs_remove_packet_route(memory, pr_id, rt3);
    }

    qs_free(memory);

    printf("\n=== packet_route_test summary ===\n");
    printf("pass: %d, fail: %d\n", pass_count, fail_count);
    return (fail_count > 0) ? 1 : 0;
}
