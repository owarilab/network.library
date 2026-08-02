#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "qs_api.h"

int main() {
    // 1. Initialize the library
    if (api_qs_init() != 0) {
        fprintf(stderr, "Failed to initialize qs_api\n");
        return 1;
    }

    // 2. Create a memory context with 1MB
    QS_MEMORY_CONTEXT mem_ctx;
    size_t pool_size = 1024 * 1024; // 1MB
    if (api_qs_memory_alloc(&mem_ctx, pool_size) != 0) {
        fprintf(stderr, "Failed to allocate memory context\n");
        return 1;
    }

    printf("[QS Memory Info] Initial state:\n");
    api_qs_memory_info(&mem_ctx);

    // 3. Create blocks
    // Block A: String storage
    int32_t memid_a = api_qs_memory_create_block(&mem_ctx, 64);
    if (memid_a < 0) {
        fprintf(stderr, "Failed to create block A\n");
        return 1;
    }
    printf("Created block A (memid: %d, size: 64)\n", memid_a);

    // Block B: Integer array storage
    int32_t memid_b = api_qs_memory_create_block(&mem_ctx, 128);
    if (memid_b < 0) {
        fprintf(stderr, "Failed to create block B\n");
        return 1;
    }
    printf("Created block B (memid: %d, size: 128)\n", memid_b);

    printf("\n[QS Memory Info] After creating blocks:\n");
    api_qs_memory_info(&mem_ctx);

    // 4. Write data to blocks
    // Write string to Block A
    char* ptr_a = (char*)api_qs_memory_get_pointer(&mem_ctx, memid_a);
    if (ptr_a) {
        const char* msg = "Hello, QS Memory!";
        strncpy(ptr_a, msg, 64);
        printf("Data in block A: %s\n", ptr_a);
    }

    // Write integers to Block B
    int32_t* ptr_b = (int32_t*)api_qs_memory_get_pointer(&mem_ctx, memid_b);
    if (ptr_b) {
        ptr_b[0] = 10;
        ptr_b[1] = 20;
        ptr_b[2] = 30;
        printf("Data in block B: %d, %d, %d\n", ptr_b[0], ptr_b[1], ptr_b[2]);
    }

    // 5. Free Block A
    printf("\nFreeing block A...\n");
    if (api_qs_memory_free_block(&mem_ctx, &memid_a) != 0) {
        fprintf(stderr, "Failed to free block A\n");
    }

    // 6. Reset the memory pool (Clean)
    printf("\nResetting the memory pool (api_qs_memory_clean)...\n");
    if (api_qs_memory_clean(&mem_ctx) != 0) {
        fprintf(stderr, "Failed to clean memory context\n");
    }

    printf("[QS Memory Info] After cleaning:\n");
    api_qs_memory_info(&mem_ctx);

    // 7. Free the entire memory pool
    printf("\nFreeing the entire memory pool (api_qs_memory_free)...\n");
    if (api_qs_memory_free(&mem_ctx) != 0) {
        fprintf(stderr, "Failed to free memory pool\n");
    }

    printf("Done.\n");

    printf("\n--- Starting Struct Array Offset Pointer Test ---\n");

    // 8. Struct Array Offset Pointer Test
    typedef struct {
        int32_t id;
        char name[16];
        uint32_t score;
    } User;

    QS_MEMORY_CONTEXT struct_ctx;
    if (api_qs_memory_alloc(&struct_ctx, 1024 * 1024) != 0) {
        fprintf(stderr, "Failed to allocate memory context for struct test\n");
        return 1;
    }

    int num_users = 3;
    size_t array_size = sizeof(User) * num_users;
    int32_t memid_structs = api_qs_memory_create_block(&struct_ctx, array_size);
    if (memid_structs < 0) {
        fprintf(stderr, "Failed to create struct block\n");
        return 1;
    }
    printf("Created struct array block (memid: %d, size: %zu)\n", memid_structs, array_size);

    // Initialize users
    User user_data[] = {
        {1, "Alice", 100},
        {2, "Bob", 200},
        {3, "Charlie", 300}
    };

    for (int i = 0; i < num_users; i++) {
        // qs_offsetpointer() uses size * offset internally, so offset is an element index.
        User* p = (User*)api_qs_memory_get_offset_pointer(&struct_ctx, memid_structs, sizeof(User), i);
        if (p) {
            memcpy(p, &user_data[i], sizeof(User));
            printf("User %d written at index %d\n", i, i);
        } else {
            fprintf(stderr, "Failed to get pointer for user %d at index %d\n", i, i);
        }
    }

    // Verify users
    printf("\nVerifying struct array:\n");
    for (int i = 0; i < num_users; i++) {
        User* p = (User*)api_qs_memory_get_offset_pointer(&struct_ctx, memid_structs, sizeof(User), i);
        if (p) {
            printf("User %d: ID=%d, Name=%s, Score=%u\n", i, p->id, p->name, p->score);
        }
    }

    // Cleanup struct test
    api_qs_memory_clean(&struct_ctx);
    api_qs_memory_free(&struct_ctx);
    printf("Struct Array Offset Pointer Test Done.\n");

    return 0;
}
