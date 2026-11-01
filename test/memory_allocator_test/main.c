/*
 * Copyright (c) Katsuya Owari
 * Memory allocator behavior test (api_qs_memory_*)
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "qs_api.h"
#include "qs_memory_allocator.h"

static int pass_count = 0;
static int fail_count = 0;

static void check_pass(const char* name, int ok)
{
    if (ok) {
        printf("[PASS] %s\n", name);
        pass_count++;
    } else {
        printf("[FAIL] %s\n", name);
        fail_count++;
    }
}

static int scenario_basic_memory_operations(void)
{
    QS_MEMORY_CONTEXT context;
    int result = -1;
    int32_t memid = -1;
    size_t request_size = 1024;
    uint8_t expected = 0x5a;
    uint8_t* p = NULL;

    if (-1 == api_qs_memory_alloc(&context, 1024 * 1024 * 2)) {
        printf("api_qs_memory_alloc failed (scenario_basic_memory_operations)\n");
        return -1;
    }

    memid = api_qs_memory_create_block(&context, request_size);
    check_pass("basic: create block", memid != -1);
    if (memid == -1) {
        goto cleanup;
    }

    p = (uint8_t*)api_qs_memory_get_pointer(&context, memid);
    check_pass("basic: get pointer", p != NULL);
    if (p == NULL) {
        goto cleanup;
    }

    memset(p, expected, request_size);
    check_pass("basic: write and read", p[0] == expected && p[request_size - 1] == expected);

    {
        size_t real_size = api_qs_memory_get_size(&context, memid);
        check_pass("basic: block size >= requested", real_size >= request_size);
    }

    check_pass("basic: free block", api_qs_memory_free_block(&context, &memid) == 0);
    result = 0;

cleanup:
    api_qs_memory_free(&context);
    return result;
}

static int scenario_temporary_memory_reuse(void)
{
    QS_MEMORY_CONTEXT context;
    int result = -1;
    size_t available_before = 0;
    size_t available_after_alloc = 0;
    size_t available_after_clean = 0;
    size_t block_size = 512 * 1024;
    int32_t memid1 = -1;
    int32_t memid2 = -1;

    if (-1 == api_qs_memory_alloc(&context, 1024 * 1024 * 8)) {
        printf("api_qs_memory_alloc failed (scenario_temporary_memory_reuse)\n");
        return -1;
    }

    available_before = api_qs_memory_available_size(&context);
    memid1 = api_qs_memory_create_block(&context, block_size);
    check_pass("reuse: first allocation", memid1 != -1);
    if (memid1 == -1) {
        goto cleanup;
    }

    available_after_alloc = api_qs_memory_available_size(&context);
    check_pass("reuse: available size decreased", available_after_alloc < available_before);

    api_qs_memory_clean(&context);
    available_after_clean = api_qs_memory_available_size(&context);
    check_pass("reuse: available size restored", available_after_clean >= available_before);

    memid2 = api_qs_memory_create_block(&context, block_size);
    check_pass("reuse: second allocation after clean", memid2 != -1);
    if (memid2 == -1) {
        goto cleanup;
    }

    result = 0;

cleanup:
    api_qs_memory_free(&context);
    return result;
}

static int scenario_persistent_and_temporary_separation(void)
{
    QS_MEMORY_CONTEXT persistent;
    QS_MEMORY_CONTEXT temporary;
    int result = -1;
    int32_t persistent_memid = -1;
    int32_t temporary_memid = -1;
    char* persistent_data = NULL;
    const char* marker = "persistent-data";

    if (-1 == api_qs_memory_alloc(&persistent, 1024 * 1024 * 4)) {
        printf("api_qs_memory_alloc failed (persistent)\n");
        return -1;
    }
    if (-1 == api_qs_memory_alloc(&temporary, 1024 * 1024 * 4)) {
        printf("api_qs_memory_alloc failed (temporary)\n");
        api_qs_memory_free(&persistent);
        return -1;
    }

    persistent_memid = api_qs_memory_create_block(&persistent, 256);
    check_pass("separation: create persistent block", persistent_memid != -1);
    if (persistent_memid == -1) {
        goto cleanup;
    }

    persistent_data = (char*)api_qs_memory_get_pointer(&persistent, persistent_memid);
    check_pass("separation: get persistent pointer", persistent_data != NULL);
    if (persistent_data == NULL) {
        goto cleanup;
    }
    snprintf(persistent_data, 256, "%s", marker);

    temporary_memid = api_qs_memory_create_block(&temporary, 512);
    check_pass("separation: create temporary block", temporary_memid != -1);
    if (temporary_memid == -1) {
        goto cleanup;
    }

    api_qs_memory_clean(&temporary);
    check_pass("separation: persistent data remains", strcmp(persistent_data, marker) == 0);

    result = 0;

cleanup:
    api_qs_memory_free(&temporary);
    api_qs_memory_free(&persistent);
    return result;
}

static int scenario_memory_unit_binary_operations(void)
{
    QS_MEMORY_CONTEXT context;
    QS_MEMORY_POOL* pool = NULL;
    int result = -1;
    int32_t memid = -1;
    uint8_t out[8];
    uint8_t append_data[] = {0x11, 0x22, 0x33};
    uint8_t write_data[] = {0xaa, 0xbb};
    uint8_t* p = NULL;
    size_t cap = 0;

    if (-1 == api_qs_memory_alloc(&context, 1024 * 1024 * 2)) {
        printf("api_qs_memory_alloc failed (scenario_memory_unit_binary_operations)\n");
        return -1;
    }
    pool = (QS_MEMORY_POOL*)context.memory;

    memid = api_qs_memory_create_block(&context, 64);
    check_pass("unit: create block", memid != -1);
    if (memid == -1) {
        goto cleanup;
    }

    p = (uint8_t*)qs_memory_unit_data(pool, memid);
    check_pass("unit: data pointer", p != NULL);
    if (p == NULL) {
        goto cleanup;
    }

    cap = qs_memory_unit_capacity(pool, memid);
    check_pass("unit: capacity", cap >= 64);
    check_pass("unit: used initial 0", qs_memory_unit_used(pool, memid) == 0);
    check_pass("unit: remaining initial", qs_memory_unit_remaining(pool, memid) == cap);

    check_pass("unit: append", qs_memory_unit_append(pool, memid, append_data, sizeof(append_data)) == QS_SYSTEM_OK);
    check_pass("unit: used after append", qs_memory_unit_used(pool, memid) == sizeof(append_data));

    check_pass("unit: write with expand used", qs_memory_unit_write(pool, memid, 5, write_data, sizeof(write_data)) == QS_SYSTEM_OK);
    check_pass("unit: used after write", qs_memory_unit_used(pool, memid) == 7);

    memset(out, 0, sizeof(out));
    check_pass("unit: read range", qs_memory_unit_read(pool, memid, 0, out, sizeof(append_data)) == QS_SYSTEM_OK);
    check_pass("unit: read value", out[0] == append_data[0] && out[1] == append_data[1] && out[2] == append_data[2]);
    check_pass("unit: read out of used range fails", qs_memory_unit_read(pool, memid, 7, out, 1) == QS_SYSTEM_ERROR);

    check_pass("unit: reset", qs_memory_unit_reset(pool, memid) == QS_SYSTEM_OK);
    check_pass("unit: used after reset", qs_memory_unit_used(pool, memid) == 0);

    check_pass("unit: append for clear", qs_memory_unit_append(pool, memid, append_data, sizeof(append_data)) == QS_SYSTEM_OK);
    check_pass("unit: clear", qs_memory_unit_clear(pool, memid) == QS_SYSTEM_OK);
    check_pass("unit: used after clear", qs_memory_unit_used(pool, memid) == 0);
    check_pass("unit: clear zero fill", p[0] == 0 && p[1] == 0 && p[2] == 0);

    check_pass("api block: append", api_qs_memory_block_append(&context, memid, append_data, sizeof(append_data)) == 0);
    check_pass("api block: used", api_qs_memory_block_used(&context, memid) == sizeof(append_data));
    check_pass("api block: remaining", api_qs_memory_block_remaining(&context, memid) == (cap - sizeof(append_data)));
    memset(out, 0, sizeof(out));
    check_pass("api block: read", api_qs_memory_block_read(&context, memid, 0, out, sizeof(append_data)) == 0);
    check_pass("api block: read value", out[0] == append_data[0] && out[1] == append_data[1] && out[2] == append_data[2]);
    check_pass("api block: write", api_qs_memory_block_write(&context, memid, 1, write_data, sizeof(write_data)) == 0);
    check_pass("api block: clear", api_qs_memory_block_clear(&context, memid) == 0);
    check_pass("api block: used after clear", api_qs_memory_block_used(&context, memid) == 0);

    result = 0;

cleanup:
    api_qs_memory_free(&context);
    return result;
}

int main(int argc, char* argv[], char* envp[])
{
    int ok = 1;
    (void)argc;
    (void)argv;
    (void)envp;

    api_qs_init();

    printf("=== Memory Allocator Test ===\n\n");

    printf("[Scenario 1] Basic Memory Operations\n");
    if (0 != scenario_basic_memory_operations()) {
        ok = 0;
    }

    printf("\n[Scenario 2] Temporary Memory Reuse\n");
    if (0 != scenario_temporary_memory_reuse()) {
        ok = 0;
    }

    printf("\n[Scenario 3] Persistent and Temporary Separation\n");
    if (0 != scenario_persistent_and_temporary_separation()) {
        ok = 0;
    }

    printf("\n[Scenario 4] Memory Unit Binary Operations\n");
    if (0 != scenario_memory_unit_binary_operations()) {
        ok = 0;
    }

    printf("\nSummary:\n");
    printf("  Total: %d, Pass: %d, Fail: %d\n", pass_count + fail_count, pass_count, fail_count);

    if (ok && fail_count == 0) {
        printf("  Status: PASS\n");
        return 0;
    }

    printf("  Status: FAIL\n");
    return -1;
}
