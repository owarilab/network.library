/*
 * Copyright (c) Katsuya Owari
 * Memory allocator behavior test (api_qs_memory_*)
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "qs_api.h"

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

    printf("\nSummary:\n");
    printf("  Total: %d, Pass: %d, Fail: %d\n", pass_count + fail_count, pass_count, fail_count);

    if (ok && fail_count == 0) {
        printf("  Status: PASS\n");
        return 0;
    }

    printf("  Status: FAIL\n");
    return -1;
}
