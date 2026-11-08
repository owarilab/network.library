/*
 * Copyright (c) Katsuya Owari
 *
 * datastore_test
 *   汎用データストア（qs_datastore）低レベル API の検証テスト。
 *
 * 検証グループ:
 *   G1 Database   G2 Table   G3 Schema   G4 Record CRUD
 *   G5 record_list G6 Index  G7 System   G8 永続化
 *
 * 低レベル API を使うため、スクラッチメモリは qs_initialize_memory_f64 で確保し、
 * QS_MEMORY_POOL* を直接渡す。一時ディレクトリは /tmp/qs_datastore_test_* を使い、
 * 終了時に削除する。
 *
 * Build: make
 * Run:   ./qs_datastore_test
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#if defined(__LINUX__) || defined(__BSD_UNIX__)
#include <dirent.h>
#endif

#include "qs_datastore.h"

static int g_pass = 0;
static int g_fail = 0;
static int g_fail_store_create = 0;
static const char* g_fail_remove_key = NULL;
static const char* g_fail_push_value = NULL;
static int g_encode_array_calls = 0;

int32_t __real_qs_json_encode_array(QS_MEMORY_POOL* memory, int32_t array, size_t size);

int32_t __wrap_qs_json_encode_array(QS_MEMORY_POOL* memory, int32_t array, size_t size)
{
    g_encode_array_calls++;
    return __real_qs_json_encode_array(memory, array, size);
}

int32_t __real_qs_array_push_string(QS_MEMORY_POOL* memory, int32_t* array, const char* value);

int32_t __wrap_qs_array_push_string(QS_MEMORY_POOL* memory, int32_t* array, const char* value)
{
    if (g_fail_push_value != NULL && strcmp(g_fail_push_value, value) == 0) {
        return -1;
    }
    return __real_qs_array_push_string(memory, array, value);
}


int32_t __real_qs_remove_hash(QS_MEMORY_POOL* memory, int32_t memid_hash, const char* hash_name);

int32_t __wrap_qs_remove_hash(QS_MEMORY_POOL* memory, int32_t memid_hash, const char* hash_name)
{
    if (g_fail_remove_key != NULL && strcmp(g_fail_remove_key, hash_name) == 0) {
        return -1;
    }
    return __real_qs_remove_hash(memory, memid_hash, hash_name);
}

static int run_lock_process(const char* base_dir, const char* operation)
{
    pid_t child = fork();
    if (child == 0) {
        execl("/proc/self/exe", "qs_datastore_test", "--lock-check", base_dir, operation, (char*)NULL);
        _exit(127);
    }
    if (child < 0) {
        return -1;
    }
    int status;
    if (waitpid(child, &status, 0) != child || !WIFEXITED(status)) {
        return -1;
    }
    return WEXITSTATUS(status);
}

typedef struct TEST_OPEN_THREAD
{
    const char* base_dir;
    int result;
} TEST_OPEN_THREAD;

static void* test_duplicate_open_thread(void* argument)
{
    TEST_OPEN_THREAD* state = (TEST_OPEN_THREAD*)argument;
        QS_DB_CONTEXT ctx;
    state->result = qs_db_open_database(&ctx, state->base_dir, "database1");
    if (state->result == 0) {
        (void)qs_db_close_database(&ctx);
    }
    return NULL;
}

int32_t __real_qs_create_storage_cache(const char* store_name, QS_MEMORY_POOL** memory,
                                       size_t max_cache_size, size_t page_allocate_size,
                                       size_t page_hash_size, size_t max_key_size);

int32_t __wrap_qs_create_storage_cache(const char* store_name, QS_MEMORY_POOL** memory,
                                       size_t max_cache_size, size_t page_allocate_size,
                                       size_t page_hash_size, size_t max_key_size)
{
    if (g_fail_store_create > 0 && --g_fail_store_create == 0) {
        int partial = open(store_name, O_CREAT | O_EXCL | O_WRONLY, 0600);
        if (partial >= 0) {
            close(partial);
        }
        return -1;
    }
    return __real_qs_create_storage_cache(store_name, memory, max_cache_size,
                                          page_allocate_size, page_hash_size, max_key_size);
}

static void check(const char* label, int ok)
{
    if (ok) {
        printf("[PASS] %s\n", label);
        g_pass++;
    } else {
        printf("[FAIL] %s\n", label);
        g_fail++;
    }
}

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

static void test_try_close_null_context(void)
{
    check("2.3 close rejects NULL context", -1 == qs_db_close_database(NULL));
}

/* スクラッチメモリ確保（malloc 型プール） */
static QS_MEMORY_POOL* scratch_alloc(void)
{
    QS_MEMORY_POOL* p = NULL;
    if (0 == qs_initialize_memory_f64(&p, SIZE_MBYTE * 4)) {
        return NULL;
    }
    return p;
}
static void scratch_free(QS_MEMORY_POOL* p)
{
    if (p != NULL) {
        qs_free(p);
    }
}

/* JSON 配列文字列をデコードし、配列 munit を返す */
static int32_t test_decode_array(QS_MEMORY_POOL* p, const char* json)
{
    int32_t root = qs_json_decode(p, json);
    if (-1 == root) {
        return -1;
    }
    QS_NODE* node = qs_get_json_root(p, root);
    if (NULL == node || node->id != ELEMENT_ARRAY) {
        return -1;
    }
    return node->element_munit;
}

static int32_t test_array_count(QS_MEMORY_POOL* p, const char* json)
{
    int32_t arr = test_decode_array(p, json);
    if (-1 == arr) {
        return -1;
    }
    return qs_array_length(p, arr);
}

/* 結果配列（オブジェクトの配列）に指定 id のレコードが含まれるか */
static int test_array_has_id(QS_MEMORY_POOL* p, const char* json, const char* id)
{
    int32_t arr = test_decode_array(p, json);
    if (-1 == arr) {
        return 0;
    }
    int32_t len = qs_array_length(p, arr);
    int i;
    for (i = 0; i < len; i++) {
        QS_ARRAY_ELEMENT* e = qs_array_get(p, arr, i);
        if (e == NULL || e->id != ELEMENT_HASH) {
            continue;
        }
        char* rid = qs_get_hash_string(p, e->memid_array_element_data, "id");
        if (rid != NULL && strcmp(rid, id) == 0) {
            return 1;
        }
    }
    return 0;
}

static int file_exists(const char* path)
{
    struct stat st;
    return (stat(path, &st) == 0);
}

static char* store_get(QS_DB_STORE* store, const char* key)
{
    QS_CACHE* cache = (QS_CACHE*)QS_GET_POINTER(store->memory, store->cache_id);
    QS_CACHE_PAGE page;
    qs_get_cache_page(cache, &page);
    int32_t value = qs_get_hash(page.memory, page.hash_id, key);
    return (value == -1) ? NULL : (char*)QS_GET_POINTER(page.memory, value);
}

static void rm_rf(const char* path)
{
    DIR* dir = opendir(path);
    if (dir != NULL) {
        struct dirent* dent;
        while ((dent = readdir(dir)) != NULL) {
            if (dent->d_name[0] == '.') {
                continue;
            }
            char sub[4096];
            snprintf(sub, sizeof(sub), "%s/%s", path, dent->d_name);
            rm_rf(sub);
        }
        closedir(dir);
    }
    rmdir(path);
}

/* ------------------------------------------------------------------ */
/* テストデータ                                                         */
/* ------------------------------------------------------------------ */

static const char* ISSUE_SCHEMA =
    "{\"fields\":["
    "{\"name\":\"id\",\"type\":\"string\",\"maxLength\":64,\"required\":0,\"unsettable\":0},"
    "{\"name\":\"title\",\"type\":\"string\",\"maxLength\":256,\"required\":1,\"unsettable\":1},"
    "{\"name\":\"status\",\"type\":\"string\",\"maxLength\":64,\"required\":1,\"unsettable\":1},"
    "{\"name\":\"priority\",\"type\":\"int32\",\"required\":0,\"unsettable\":1}"
    "]}";

/* 4 件の issue レコード */
static const char* REC_R1 = "{\"id\":\"r1\",\"title\":\"t1\",\"status\":\"open\",\"assigneeIds\":[\"u1\"],\"labelIds\":[\"bug\"],\"priority\":1}";
static const char* REC_R2 = "{\"id\":\"r2\",\"title\":\"t2\",\"status\":\"open\",\"assigneeIds\":[\"u2\"],\"labelIds\":[\"feature\"],\"priority\":2}";
static const char* REC_R3 = "{\"id\":\"r3\",\"title\":\"t3\",\"status\":\"closed\",\"assigneeIds\":[\"u1\"],\"labelIds\":[\"bug\"],\"priority\":3}";
static const char* REC_R4 = "{\"id\":\"r4\",\"title\":\"t4\",\"status\":\"in_progress\",\"assigneeIds\":[\"u3\"],\"labelIds\":[\"feature\"],\"priority\":4}";

static void insert_sample_issues(QS_DB_CONTEXT* ctx)
{
    qs_db_record_insert(ctx, "issue", "r1", REC_R1);
    qs_db_record_insert(ctx, "issue", "r2", REC_R2);
    qs_db_record_insert(ctx, "issue", "r3", REC_R3);
    qs_db_record_insert(ctx, "issue", "r4", REC_R4);
}

/* ------------------------------------------------------------------ */
/* G1: Database                                                        */
/* ------------------------------------------------------------------ */

static void test_g1_database(const char* base_dir)
{
    printf("\n--- G1 Database ---\n");

    char failure_path[4096];
    int stage;
    for (stage = 1; stage <= 3; stage++) {
        char database[32];
        char label[128];
        snprintf(database, sizeof(database), "failed_create_%d", stage);
        snprintf(failure_path, sizeof(failure_path), "%s/%s", base_dir, database);
        g_fail_store_create = stage;
        int failure_rc = qs_db_create_database(base_dir, database);
        g_fail_store_create = 0;
        snprintf(label, sizeof(label), "1.0 stage %d creation failure reported", stage);
        check(label, failure_rc == -1);
        snprintf(label, sizeof(label), "1.0 stage %d partial database removed", stage);
        check(label, !file_exists(failure_path));
    }
    check("1.0 recreate after failure", 0 == qs_db_create_database(base_dir, "failed_create_3"));
    check("1.0 remove recreated database", 0 == qs_db_drop_database(base_dir, "failed_create_3"));

    QS_MEMORY_POOL* failed_memory = NULL;
    snprintf(failure_path, sizeof(failure_path), "%s/no_such_parent/store.db", base_dir);
    check("1.0 mmap open failure reported", 0 == qs_initialize_mmapmemory_f(failure_path, &failed_memory, 4096));
    check("1.0 mmap open failure releases pool", failed_memory == NULL);

    /* 1.1 create_database */
    int rc = qs_db_create_database(base_dir, "database1");
    check("1.1 create_database returns 0", 0 == rc);
    check("1.1 reject database path traversal", -1 == qs_db_create_database(base_dir, "../escape"));
    check("1.1 reject database separator", -1 == qs_db_create_database(base_dir, "nested/db"));
    char database_name[48];
    memset(database_name, 'd', 45);
    database_name[45] = '\0';
    check("1.1 create maximum 45-byte database", 0 == qs_db_create_database(base_dir, database_name));
    check("1.1 drop maximum 45-byte database", 0 == qs_db_drop_database(base_dir, database_name));
    database_name[45] = 'd';
    database_name[46] = '\0';
    check("1.1 reject 46-byte database name", -1 == qs_db_create_database(base_dir, database_name));
    check("1.1 reject database invalid character", -1 == qs_db_create_database(base_dir, "database.name"));
    char path[4096];
    snprintf(path, sizeof(path), "%s/database1/system.db", base_dir);
    check("1.1 system.db exists", file_exists(path));
    snprintf(path, sizeof(path), "%s/database1/schema.db", base_dir);
    check("1.1 schema.db exists", file_exists(path));
    snprintf(path, sizeof(path), "%s/database1/index.db", base_dir);
    check("1.1 index.db exists", file_exists(path));

    /* 1.2 2 つ目の database */
    rc = qs_db_create_database(base_dir, "database2");
    check("1.2 create database2 returns 0", 0 == rc);
    snprintf(path, sizeof(path), "%s/database2/system.db", base_dir);
    check("1.2 database2/system.db exists", file_exists(path));

    char nested_dir[4096];
    snprintf(nested_dir, sizeof(nested_dir), "%s/nested", base_dir);
    check("1.2 create nested directory", mkdir(nested_dir, 0755) == 0);
    check("1.2 create nested database", 0 == qs_db_create_database(nested_dir, "child"));

    /* 1.3 show_database */
    QS_MEMORY_POOL* scratch = scratch_alloc();
    if (scratch != NULL) {
        int32_t dbs = qs_db_show_database(scratch, base_dir);
        int found1 = 0, found2 = 0, found_nested = 0;
        if (-1 != dbs) {
            int32_t len = qs_array_length(scratch, dbs);
            int i;
            for (i = 0; i < len; i++) {
                QS_ARRAY_ELEMENT* e = qs_array_get(scratch, dbs, i);
                if (e == NULL || e->id != ELEMENT_LITERAL_STR) {
                    continue;
                }
                char* name = (char*)QS_GET_POINTER(scratch, e->memid_array_element_data);
                if (strcmp(name, "database1") == 0) found1 = 1;
                if (strcmp(name, "database2") == 0) found2 = 1;
                if (strcmp(name, "nested/child") == 0) found_nested = 1;
            }
        }
        check("1.3 show_database contains database1", found1);
        check("1.3 show_database contains database2", found2);
        check("1.3 show_database excludes nested database", !found_nested);
        scratch_free(scratch);
    } else {
        check("1.3 scratch alloc", 0);
    }
    QS_MEMORY_POOL* slash_scratch = scratch_alloc();
    if (slash_scratch != NULL) {
        int32_t dbs = qs_db_show_database(slash_scratch, base_dir);
        int found1 = 0;
        if (dbs != -1) {
            int32_t len = qs_array_length(slash_scratch, dbs);
            int i;
            for (i = 0; i < len; i++) {
                QS_ARRAY_ELEMENT* element = qs_array_get(slash_scratch, dbs, i);
                if (element != NULL && element->id == ELEMENT_LITERAL_STR &&
                    strcmp((char*)QS_GET_POINTER(slash_scratch, element->memid_array_element_data), "database1") == 0) {
                    found1 = 1;
                }
            }
        }
        check("1.3 show_database accepts trailing slash", found1);
        scratch_free(slash_scratch);
    }
    check("1.3 drop nested database", 0 == qs_db_drop_database(nested_dir, "child"));
    check("1.3 remove nested directory", rmdir(nested_dir) == 0);

    /* 1.4 open/close */
    QS_DB_CONTEXT ctx;
    rc = qs_db_open_database(&ctx, base_dir, "database1");
    check("1.4 open_database returns 0", 0 == rc);
    check("1.4 is_open set", ctx.is_open == 1);
    QS_DB_CONTEXT duplicate_ctx;
    check("1.4 duplicate open rejected", -1 == qs_db_open_database(&duplicate_ctx, base_dir, "database1"));
    check("1.4 same context reopen rejected", -1 == qs_db_open_database(&ctx, base_dir, "database1"));
    check("1.4 same context different database rejected", -1 == qs_db_open_database(&ctx, base_dir, "database2"));
    check("1.4 same context invalid name rejected", -1 == qs_db_open_database(&ctx, base_dir, "../escape"));
    char alias_dir[4096];
    snprintf(alias_dir, sizeof(alias_dir), "%s/.", base_dir);
    check("1.4 canonical alias open rejected", -1 == qs_db_open_database(&duplicate_ctx, alias_dir, "database1"));
    check("1.4 different database open succeeds", 0 == qs_db_open_database(&duplicate_ctx, base_dir, "database2"));
    check("1.4 close database2 succeeds", 0 == qs_db_close_database(&duplicate_ctx));
    TEST_OPEN_THREAD thread_state = {base_dir, 0};
    pthread_t thread;
    int thread_started = pthread_create(&thread, NULL, test_duplicate_open_thread, &thread_state) == 0;
    check("1.4 duplicate open thread started", thread_started);
    if (thread_started) {
        pthread_join(thread, NULL);
        check("1.4 duplicate open from thread rejected", thread_state.result == -1);
    }
    check("1.4 original context remains open", ctx.is_open == 1);
    check("1.4 original context still usable", 0 == qs_db_schema_define(&ctx, "owner_test", "{\"fields\":[]}"));
    check("1.4 drop open database rejected", -1 == qs_db_drop_database(base_dir, "database1"));
    check("1.4 other process open rejected", 0 == run_lock_process(base_dir, "open-rejected"));
    check("1.4 other process drop rejected", 0 == run_lock_process(base_dir, "drop-rejected"));
    check("1.4 other process can open different database", 0 == run_lock_process(base_dir, "other-open"));
    check("1.4 close database1 succeeds", 0 == qs_db_close_database(&ctx));
    check("1.4 is_open cleared after close", ctx.is_open == 0);
    check("1.4 other process can open after close", 0 == run_lock_process(base_dir, "open-success"));
    g_fail_store_create = 2;
    check("1.4 injected open failure reported", -1 == qs_db_open_database(&ctx, base_dir, "database1"));
    g_fail_store_create = 0;
    check("1.4 failed open releases process lock", 0 == run_lock_process(base_dir, "open-success"));
    check("1.4 create truncated database fixture", 0 == qs_db_create_database(base_dir, "truncated"));
    snprintf(path, sizeof(path), "%s/truncated/system.db", base_dir);
    check("1.4 truncate database fixture", 0 == truncate(path, 0));
    check("1.4 truncated database open rejected", 0 == run_lock_process(base_dir, "truncated-rejected"));
    check("1.4 remove truncated database fixture", 0 == qs_db_drop_database(base_dir, "truncated"));
    check("1.4 reopen after close succeeds", 0 == qs_db_open_database(&duplicate_ctx, base_dir, "database1"));
    check("1.4 close reopened database succeeds", 0 == qs_db_close_database(&duplicate_ctx));
    check("1.4 duplicate create database rejected", -1 == qs_db_create_database(base_dir, "database1"));

    check("1.4 open database2 for drop safety", 0 == qs_db_open_database(&ctx, base_dir, "database2"));
    check("1.4 create data table for drop safety", 0 == qs_db_create_table(&ctx, "data", QS_DB_CAPACITY_B8MB));
    check("1.4 insert data for drop safety", 0 == qs_db_record_insert(&ctx, "data", "key", "{\"value\":\"kept\"}"));
    check("1.4 close database2 drop fixture succeeds", 0 == qs_db_close_database(&ctx));
    const char* unexpected[] = {"notes.txt", ".hidden", "child", "link.db", ".hidden.db"};
    size_t unexpected_index;
    for (unexpected_index = 0; unexpected_index < sizeof(unexpected) / sizeof(unexpected[0]); unexpected_index++) {
        char extra[4096];
        char label[128];
        int created;
        snprintf(extra, sizeof(extra), "%s/database2/%s", base_dir, unexpected[unexpected_index]);
        if (unexpected_index == 2) {
            created = mkdir(extra, 0700) == 0;
        } else if (unexpected_index == 3) {
            created = symlink("system.db", extra) == 0;
        } else {
            FILE* file = fopen(extra, "w");
            created = file != NULL;
            if (file != NULL) {
                fclose(file);
            }
        }
        snprintf(label, sizeof(label), "1.4 create unexpected %s", unexpected[unexpected_index]);
        check(label, created);
        snprintf(label, sizeof(label), "1.4 reject unexpected %s", unexpected[unexpected_index]);
        check(label, created && -1 == qs_db_drop_database(base_dir, "database2"));
        snprintf(path, sizeof(path), "%s/database2/system.db", base_dir);
        int intact = file_exists(path);
        snprintf(path, sizeof(path), "%s/database2/schema.db", base_dir);
        intact = intact && file_exists(path);
        snprintf(path, sizeof(path), "%s/database2/index.db", base_dir);
        intact = intact && file_exists(path);
        snprintf(path, sizeof(path), "%s/database2/data.db", base_dir);
        intact = intact && file_exists(path);
        snprintf(label, sizeof(label), "1.4 preserve files with %s", unexpected[unexpected_index]);
        check(label, created && intact && file_exists(extra));
        if (created) {
            if (unexpected_index == 2) {
                rmdir(extra);
            } else {
                unlink(extra);
            }
        }
    }
    check("1.4 reopen after rejected drops", 0 == qs_db_open_database(&ctx, base_dir, "database2"));
    char kept_record[128];
    check("1.4 record retained after rejected drops", 0 == qs_db_record_get(&ctx, "data", "key", kept_record, sizeof(kept_record)) &&
          strcmp(kept_record, "{\"value\":\"kept\"}") == 0);
    check("2.3 final close succeeds", 0 == qs_db_close_database(&ctx));

    /* 1.5 存在しない database の open */
    rc = qs_db_open_database(&ctx, base_dir, "no_such_db");
    check("1.5 open missing database returns -1", -1 == rc);

    /* 1.6 drop_database */
    rc = qs_db_drop_database(base_dir, "database2");
    check("1.6 drop_database returns 0", 0 == rc);
    snprintf(path, sizeof(path), "%s/database2", base_dir);
    check("1.6 database2 dir removed", !file_exists(path));
}

/* ------------------------------------------------------------------ */
/* G2: Table                                                           */
/* ------------------------------------------------------------------ */

static void test_g2_table(const char* base_dir)
{
    printf("\n--- G2 Table ---\n");
    QS_DB_CONTEXT ctx;
    if (-1 == qs_db_open_database(&ctx, base_dir, "database1")) {
        check("2.x open database1", 0);
        return;
    }

    /* 2.1 create_table */
    check("2.1 create table1", 0 == qs_db_create_table(&ctx, "table1", QS_DB_CAPACITY_B1MB));
    check("2.1 create table2", 0 == qs_db_create_table(&ctx, "table2", QS_DB_CAPACITY_B1MB));
    check("2.1 create 8MB table", 0 == qs_db_create_table(&ctx, "large", QS_DB_CAPACITY_B8MB));
        check("2.1 create index rejected", -1 == qs_db_create_table(&ctx, "index", QS_DB_CAPACITY_B1MB));
    check("2.1 reject table separator", -1 == qs_db_create_table(&ctx, "nested/table", QS_DB_CAPACITY_B1MB));
    char table_name[48];
    memset(table_name, 'a', 45);
    table_name[45] = '\0';
    check("2.1 create maximum 45-byte table", 0 == qs_db_create_table(&ctx, table_name, QS_DB_CAPACITY_B1MB));
    check("2.1 drop maximum 45-byte table", 0 == qs_db_drop_table(&ctx, table_name));
    table_name[45] = 'a';
    table_name[46] = '\0';
    check("2.1 reject 46-byte table name", -1 == qs_db_create_table(&ctx, table_name, QS_DB_CAPACITY_B1MB));
    check("2.1 duplicate create table rejected", -1 == qs_db_create_table(&ctx, "table1", QS_DB_CAPACITY_B8MB));
    check("2.1 create system rejected", -1 == qs_db_create_table(&ctx, "system", QS_DB_CAPACITY_B1MB));
    check("2.1 create schema rejected", -1 == qs_db_create_table(&ctx, "schema", QS_DB_CAPACITY_B1MB));
    check("2.1 create index rejected", -1 == qs_db_create_table(&ctx, "index", QS_DB_CAPACITY_B1MB));
    char path[4096];
        struct stat store_stat;
    snprintf(path, sizeof(path), "%s/database1/table1.db", base_dir);
    check("2.1 table1.db exists", file_exists(path));
        check("2.1 1MB legacy file size", stat(path, &store_stat) == 0 &&
            (uintmax_t)store_stat.st_size == (uintmax_t)QS_ALIGNUP(SIZE_MBYTE * 980 + SIZE_KBYTE * 5 + 72, MEMORY_ALIGNMENT_SIZE_BIT_64) + 1);
        snprintf(path, sizeof(path), "%s/database1/large.db", base_dir);
        check("2.1 8MB file size", stat(path, &store_stat) == 0 &&
            (uintmax_t)store_stat.st_size == (uintmax_t)QS_ALIGNUP(SIZE_MBYTE * 8 + SIZE_KBYTE * 5 + 72, MEMORY_ALIGNMENT_SIZE_BIT_64) + 1);
    snprintf(path, sizeof(path), "%s/database1/table2.db", base_dir);
    check("2.1 table2.db exists", file_exists(path));
    check_str("2.1 capacity metadata stored", store_get(&ctx.system, "table:large:capacity_mb"), "8");

    /* 2.2 show_table */
    QS_MEMORY_POOL* scratch = scratch_alloc();
    if (scratch != NULL) {
        int32_t tables = qs_db_show_table(scratch, &ctx);
        int found1 = 0, found2 = 0, found_system = 0;
        if (-1 != tables) {
            int32_t len = qs_array_length(scratch, tables);
            int i;
            for (i = 0; i < len; i++) {
                QS_ARRAY_ELEMENT* e = qs_array_get(scratch, tables, i);
                if (e == NULL || e->id != ELEMENT_LITERAL_STR) {
                    continue;
                }
                char* name = (char*)QS_GET_POINTER(scratch, e->memid_array_element_data);
                if (strcmp(name, "table1") == 0) found1 = 1;
                if (strcmp(name, "table2") == 0) found2 = 1;
                if (strcmp(name, "system") == 0 || strcmp(name, "schema") == 0 || strcmp(name, "index") == 0) found_system = 1;
            }
        }
        check("2.2 show_table contains table1", found1);
        check("2.2 show_table contains table2", found2);
        check("2.2 show_table excludes system/schema/index", !found_system);
        scratch_free(scratch);
    } else {
        check("2.2 scratch alloc", 0);
    }

    /* 2.3 open_table/close_table */
    QS_DB_STORE store;
    check("2.3 open_table table1", 0 == qs_db_open_table(&ctx, "table1", QS_DB_CAPACITY_B1MB, &store));
    check("2.3 store is_open", store.is_open == 1);
    QS_DB_STORE duplicate_store;
        memset(&duplicate_store, 0xA5, sizeof(duplicate_store));
    check("2.3 duplicate table open rejected", -1 == qs_db_open_table(&ctx, "table1", QS_DB_CAPACITY_B1MB, &duplicate_store));
        check("2.3 failed duplicate open clears output", duplicate_store.memory == NULL && duplicate_store.cache_id == 0 &&
            duplicate_store.path[0] == '\0' && duplicate_store.is_open == 0 && duplicate_store.owner == NULL);
    check("2.3 record insert while handle open rejected", -1 == qs_db_record_insert(&ctx, "table1", "id", "{\"id\":\"id\"}"));
    check("2.3 drop open table rejected", -1 == qs_db_drop_table(&ctx, "table1"));
    check("2.3 close reports deferred", 1 == qs_db_close_database(&ctx));
    check("2.3 close database with table handle deferred", ctx.is_open == 1);
    qs_db_close_table(&store);
    check("2.3 store closed", store.is_open == 0);
    check("2.3 table handle count released", ctx.open_table_count == 0);
    qs_db_close_table(&store);
    check("2.3 double table close does not underflow", ctx.open_table_count == 0 && ctx.is_open == 1);
    check("2.3 close succeeds after handle close", 0 == qs_db_close_database(&ctx));
    check("2.3 database close succeeds after handle close", ctx.is_open == 0);
    check("2.3 close is harmless when already closed", 0 == qs_db_close_database(&ctx));
    check("2.7 close succeeds after handles released", 0 == qs_db_close_database(&ctx));
    check("2.3 double database close is harmless", ctx.is_open == 0);
    check("2.3 reopen after deferred close", 0 == qs_db_open_database(&ctx, base_dir, "database1"));
    check("2.3 drop after close succeeds", 0 == qs_db_drop_table(&ctx, "table1"));

    /* 2.4 存在しない table の open */
    check("2.4 open missing table returns -1", -1 == qs_db_open_table(&ctx, "no_such", QS_DB_CAPACITY_B1MB, &store));

    /* 2.5 drop_table */
    check("2.5 drop_table table2", 0 == qs_db_drop_table(&ctx, "table2"));
    snprintf(path, sizeof(path), "%s/database1/table2.db", base_dir);
    check("2.5 table2.db removed", !file_exists(path));
    check("2.5 table2 capacity metadata removed", store_get(&ctx.system, "table:table2:capacity_mb") == NULL);
    check("2.5 missing table drop rejected", -1 == qs_db_drop_table(&ctx, "missing"));
    check("2.5 create failed drop fixture", 0 == qs_db_create_table(&ctx, "dropfail", QS_DB_CAPACITY_B8MB));
    g_fail_remove_key = "table:dropfail:capacity_mb";
    check("2.5 metadata removal failure reported", -1 == qs_db_drop_table(&ctx, "dropfail"));
    g_fail_remove_key = NULL;
    check("2.5 create schema failure fixture", 0 == qs_db_create_table(&ctx, "dropschema", QS_DB_CAPACITY_B8MB));
    check("2.5 define schema failure fixture", 0 == qs_db_schema_define(&ctx, "dropschema", "{\"fields\":[]}"));
    g_fail_remove_key = "dropschema";
    check("2.5 schema removal failure reported", -1 == qs_db_drop_table(&ctx, "dropschema"));
    g_fail_remove_key = NULL;
    check("2.5 create index failure fixture", 0 == qs_db_create_table(&ctx, "dropindex", QS_DB_CAPACITY_B8MB));
    check("2.5 add index failure fixture", 0 == qs_db_index_add(&ctx, "dropindex", "tag", "x", "item"));
    g_fail_remove_key = "index:dropindex:tag:x";
    check("2.5 index removal failure reported", -1 == qs_db_drop_table(&ctx, "dropindex"));
    g_fail_remove_key = NULL;
    check("2.5 create enumeration failure fixture", 0 == qs_db_create_table(&ctx, "dropkeys", QS_DB_CAPACITY_B8MB));
    check("2.5 add enumeration failure fixture", 0 == qs_db_index_add(&ctx, "dropkeys", "tag", "x", "item"));
    g_fail_push_value = "index:dropkeys:tag:x";
    check("2.5 index enumeration failure reported", -1 == qs_db_drop_table(&ctx, "dropkeys"));
    g_fail_push_value = NULL;

    snprintf(path, sizeof(path), "%s/database1/dropkeys.db", base_dir);
    check("2.5 table retained after enumeration failure", file_exists(path));
    check("2.5 drop after enumeration failure", 0 == qs_db_drop_table(&ctx, "dropkeys"));

        check("2.6 define schema before reserved drops", 0 == qs_db_schema_define(&ctx, "large", "{\"fields\":[]}"));
        check("2.6 add index before reserved drops", 0 == qs_db_index_add(&ctx, "large", "tag", "value", "large1"));
        check("2.6 drop system rejected", -1 == qs_db_drop_table(&ctx, "system"));
        check("2.6 drop schema rejected", -1 == qs_db_drop_table(&ctx, "schema"));
        check("2.6 drop index rejected", -1 == qs_db_drop_table(&ctx, "index"));
        snprintf(path, sizeof(path), "%s/database1/system.db", base_dir);
        check("2.6 system file remains", file_exists(path));
        snprintf(path, sizeof(path), "%s/database1/schema.db", base_dir);
        check("2.6 schema file remains", file_exists(path));
        snprintf(path, sizeof(path), "%s/database1/index.db", base_dir);
        check("2.6 index file remains", file_exists(path));
    check("2.6 insert into 8MB table", 0 == qs_db_record_insert(&ctx, "large", "large1", "{\"id\":\"large1\"}"));
    check("2.7 close recreated database succeeds", 0 == qs_db_close_database(&ctx));
    check("2.6 reopen database for 8MB table", 0 == qs_db_open_database(&ctx, base_dir, "database1"));
    char record[128];
    check("2.6 get from reopened 8MB table", 0 == qs_db_record_get(&ctx, "large", "large1", record, sizeof(record)) &&
          strcmp(record, "{\"id\":\"large1\"}") == 0);
        check("2.6 corrupt capacity metadata", 0 == qs_db_record_update(&ctx, "system", "table:large:capacity_mb", "8junk"));
        check("2.6 reject malformed capacity", -1 == qs_db_record_get(&ctx, "large", "large1", record, sizeof(record)));
        check("2.6 reject overflow capacity setup", 0 == qs_db_record_update(&ctx, "system", "table:large:capacity_mb", "999999999999999999999999"));
        check("2.6 reject overflow capacity", -1 == qs_db_record_get(&ctx, "large", "large1", record, sizeof(record)));
        check("2.6 mismatched capacity setup", 0 == qs_db_record_update(&ctx, "system", "table:large:capacity_mb", "1"));
        check("2.6 reject mismatched capacity", -1 == qs_db_record_get(&ctx, "large", "large1", record, sizeof(record)));
        check("2.6 missing capacity setup", 0 == qs_db_record_delete(&ctx, "system", "table:large:capacity_mb"));
        check("2.6 reject missing capacity", -1 == qs_db_record_get(&ctx, "large", "large1", record, sizeof(record)));
        check("2.6 restore capacity", 0 == qs_db_record_insert(&ctx, "system", "table:large:capacity_mb", "8"));
        check("2.6 record after capacity restore", 0 == qs_db_record_get(&ctx, "large", "large1", record, sizeof(record)));
        snprintf(path, sizeof(path), "%s/database1/large.db", base_dir);
        int large_fd = open(path, O_RDWR);
        uint32_t saved_header = 0;
        check("2.6 read store header", large_fd >= 0 && pread(large_fd, &saved_header, sizeof(saved_header), 0) == sizeof(saved_header));
        uint32_t bad_header = 0;
        check("2.6 corrupt store header", large_fd >= 0 && pwrite(large_fd, &bad_header, sizeof(bad_header), 0) == sizeof(bad_header));
        check("2.6 reject invalid store header", -1 == qs_db_record_get(&ctx, "large", "large1", record, sizeof(record)));
        check("2.6 restore store header", large_fd >= 0 && pwrite(large_fd, &saved_header, sizeof(saved_header), 0) == sizeof(saved_header));
        if (large_fd >= 0) close(large_fd);
        check("2.6 record after header restore", 0 == qs_db_record_get(&ctx, "large", "large1", record, sizeof(record)));
        check("2.6 create shortened table", 0 == qs_db_create_table(&ctx, "short", QS_DB_CAPACITY_B8MB));
        snprintf(path, sizeof(path), "%s/database1/short.db", base_dir);
        check("2.6 shorten table file", 0 == truncate(path, 0));
        check("2.6 reject shortened table", -1 == qs_db_open_table(&ctx, "short", QS_DB_CAPACITY_B8MB, &store));
        check("2.6 drop shortened table", 0 == qs_db_drop_table(&ctx, "short"));
        check_str("2.6 reopened capacity metadata retained", store_get(&ctx.system, "table:large:capacity_mb"), "8");
        check("2.6 reopened schema retained", 0 == qs_db_schema_get(&ctx, "large", record, sizeof(record)) &&
            strcmp(record, "{\"fields\":[]}") == 0);
        check("2.6 reopened index retained", 0 == qs_db_record_get(&ctx, "index", "index:large:tag:value", record, sizeof(record)) &&
            strcmp(record, "[\"large1\"]") == 0);

    check("2.7 database close after table handle succeeds", 0 == qs_db_close_database(&ctx));
    check("2.7 database closed after table handle", ctx.is_open == 0);
    check("2.7 database drop after close succeeds", 0 == qs_db_drop_database(base_dir, "database1"));
    check("2.7 recreate database after drop", 0 == qs_db_create_database(base_dir, "database1"));
    check("2.7 reopen recreated database", 0 == qs_db_open_database(&ctx, base_dir, "database1"));
    check("2.7 close recreated database succeeds", 0 == qs_db_close_database(&ctx));
}

/* ------------------------------------------------------------------ */
/* G3: Schema                                                          */
/* ------------------------------------------------------------------ */

static void test_g3_schema(const char* base_dir)
{
    printf("\n--- G3 Schema ---\n");
    QS_DB_CONTEXT ctx;
    if (-1 == qs_db_open_database(&ctx, base_dir, "database1")) {
        check("3.x open database1", 0);
        return;
    }

    /* 3.1 schema_define */
    check("3.1 schema_define issue", 0 == qs_db_schema_define(&ctx, "issue", ISSUE_SCHEMA));

    /* 3.2 schema_get */
    char schemabuf[4096];
    int src = qs_db_schema_get(&ctx, "issue", schemabuf, sizeof(schemabuf));
    check_str("3.2 schema_get returns defined json", (src == 0) ? schemabuf : NULL, ISSUE_SCHEMA);

    /* 3.3〜3.6 schema_validate */
    QS_MEMORY_POOL* scratch = scratch_alloc();
    if (scratch != NULL) {
        check("3.2 define malformed schema is stored as-is", 0 == qs_db_schema_define(&ctx, "raw_schema", "not-json"));
        check("3.2 malformed schema can be retrieved", 0 == qs_db_schema_get(&ctx, "raw_schema", schemabuf, sizeof(schemabuf)) && strcmp(schemabuf, "not-json") == 0);
        check("3.2 validation rejects malformed stored schema", -1 == qs_db_schema_validate(scratch, &ctx, "raw_schema", "{}"));
        check("3.3 validate valid record", 0 == qs_db_schema_validate(scratch, &ctx, "issue", REC_R1));
        /* required(title) 欠落 */
        check("3.4 validate missing required", -1 == qs_db_schema_validate(scratch, &ctx, "issue", "{\"id\":\"x\",\"status\":\"open\"}"));
        /* 型不一致: priority に文字列 */
        check("3.5 validate type mismatch", -1 == qs_db_schema_validate(scratch, &ctx, "issue", "{\"id\":\"x\",\"title\":\"t\",\"status\":\"open\",\"priority\":\"abc\"}"));
        /* maxLength 超過: title 257 文字 */
        char long_title[300];
        memset(long_title, 'a', 257);
        long_title[257] = '\0';
        char bad[400];
        snprintf(bad, sizeof(bad), "{\"id\":\"x\",\"title\":\"%s\",\"status\":\"open\"}", long_title);
        check("3.6 validate maxLength exceeded", -1 == qs_db_schema_validate(scratch, &ctx, "issue", bad));
        /* 3.7 スキーマ未定義 table */
        check("3.7 validate undefined schema table", -1 == qs_db_schema_validate(scratch, &ctx, "no_schema_table", REC_R1));
        scratch_free(scratch);
    } else {
        check("3.x scratch alloc", 0);
    }

    check("3.7 close schema database succeeds", 0 == qs_db_close_database(&ctx));
}

/* ------------------------------------------------------------------ */
/* G4: Record CRUD                                                     */
/* ------------------------------------------------------------------ */

static void test_g4_record(const char* base_dir)
{
    printf("\n--- G4 Record CRUD ---\n");
    QS_DB_CONTEXT ctx;
    if (-1 == qs_db_open_database(&ctx, base_dir, "database1")) {
        check("4.x open database1", 0);
        return;
    }
    qs_db_create_table(&ctx, "rec", QS_DB_CAPACITY_B1MB);

    /* 4.1 insert */
    check("4.1 insert rec1", 0 == qs_db_record_insert(&ctx, "rec", "rec1", "{\"id\":\"rec1\",\"v\":1}"));
    /* 4.2 重複 insert */
    check("4.2 duplicate insert returns -1", -1 == qs_db_record_insert(&ctx, "rec", "rec1", "{\"id\":\"rec1\",\"v\":2}"));
    /* 4.3 get */
    char recbuf[512];
    check("4.3 get rec1", 0 == qs_db_record_get(&ctx, "rec", "rec1", recbuf, sizeof(recbuf)) && strcmp(recbuf, "{\"id\":\"rec1\",\"v\":1}") == 0);
    /* 4.4 未存在 get */
    check("4.4 get missing returns -1", -1 == qs_db_record_get(&ctx, "rec", "nope", recbuf, sizeof(recbuf)));
    /* 4.5 update */
    check("4.5 update rec1", 0 == qs_db_record_update(&ctx, "rec", "rec1", "{\"id\":\"rec1\",\"v\":99}"));
    check("4.5 get after update", 0 == qs_db_record_get(&ctx, "rec", "rec1", recbuf, sizeof(recbuf)) && strcmp(recbuf, "{\"id\":\"rec1\",\"v\":99}") == 0);
    /* 4.6 未存在 update */
    check("4.6 update missing returns -1", -1 == qs_db_record_update(&ctx, "rec", "nope", "{\"id\":\"nope\"}"));
    /* 4.7 delete */
    check("4.7 delete rec1", 0 == qs_db_record_delete(&ctx, "rec", "rec1"));
    check("4.7 get after delete returns -1", -1 == qs_db_record_get(&ctx, "rec", "rec1", recbuf, sizeof(recbuf)));

    check("4.9 close record database succeeds", 0 == qs_db_close_database(&ctx));
}

/* ------------------------------------------------------------------ */
/* G5: record_list                                                     */
/* ------------------------------------------------------------------ */

static void test_g5_record_list(const char* base_dir)
{
    printf("\n--- G5 record_list ---\n");
    QS_DB_CONTEXT ctx;
    if (-1 == qs_db_open_database(&ctx, base_dir, "database1")) {
        check("5.x open database1", 0);
        return;
    }
    qs_db_create_table(&ctx, "issue", QS_DB_CAPACITY_B1MB);
    insert_sample_issues(&ctx);

    QS_MEMORY_POOL* scratch = scratch_alloc();
    if (scratch == NULL) {
        check("5.x scratch alloc", 0);
        check("5.x close database after allocation failure", 0 == qs_db_close_database(&ctx));
        return;
    }

    /* 5.1 フィルタなし全件 */
    char* res = qs_db_record_list(scratch, &ctx, "issue", NULL, QS_DB_SCAN_FULL);
    check("5.1 list all count==4", res != NULL && test_array_count(scratch, res) == 4);

    /* 5.2 string フィルタ（フルスキャン）status=open -> r1,r2 */
    res = qs_db_record_list(scratch, &ctx, "issue", "{\"status\":\"open\"}", QS_DB_SCAN_FULL);
    check("5.2 full status=open count==2", res != NULL && test_array_count(scratch, res) == 2);
    check("5.2 full status=open has r1", res != NULL && test_array_has_id(scratch, res, "r1"));
    check("5.2 full status=open has r2", res != NULL && test_array_has_id(scratch, res, "r2"));

    /* 5.3 複数フィルタ AND（フルスキャン）status=open AND labelIds=bug -> r1 */
    res = qs_db_record_list(scratch, &ctx, "issue", "{\"status\":\"open\",\"labelIds\":\"bug\"}", QS_DB_SCAN_FULL);
    check("5.3 full AND count==1", res != NULL && test_array_count(scratch, res) == 1);
    check("5.3 full AND has r1", res != NULL && test_array_has_id(scratch, res, "r1"));

    /* 5.4 配列フィールドフィルタ assigneeIds=u1 -> r1,r3 */
    res = qs_db_record_list(scratch, &ctx, "issue", "{\"assigneeIds\":\"u1\"}", QS_DB_SCAN_FULL);
    check("5.4 full assignee=u1 count==2", res != NULL && test_array_count(scratch, res) == 2);
    check("5.4 full assignee=u1 has r1", res != NULL && test_array_has_id(scratch, res, "r1"));
    check("5.4 full assignee=u1 has r3", res != NULL && test_array_has_id(scratch, res, "r3"));

    res = qs_db_record_list(scratch, &ctx, "issue", "{\"status\":\"open\"}", QS_DB_SCAN_INDEX);
    check("5.4 index before rebuild returns empty", res != NULL && test_array_count(scratch, res) == 0);

    /* index 構築（status, labelIds, assigneeIds） */
    qs_db_index_rebuild(scratch, &ctx, "issue", "[\"status\",\"labelIds\",\"assigneeIds\"]");

    /* 5.5 index 検索（単一フィルタ）status=open -> r1,r2 */
    res = qs_db_record_list(scratch, &ctx, "issue", "{\"status\":\"open\"}", QS_DB_SCAN_INDEX);
    check("5.5 index status=open count==2", res != NULL && test_array_count(scratch, res) == 2);
    check("5.5 index status=open has r1", res != NULL && test_array_has_id(scratch, res, "r1"));
    check("5.5 index status=open has r2", res != NULL && test_array_has_id(scratch, res, "r2"));

    /* 5.6 index 検索（複数フィルタ AND）status=open AND labelIds=bug -> r1 */
    res = qs_db_record_list(scratch, &ctx, "issue", "{\"status\":\"open\",\"labelIds\":\"bug\"}", QS_DB_SCAN_INDEX);
    check("5.6 index AND count==1", res != NULL && test_array_count(scratch, res) == 1);
    check("5.6 index AND has r1", res != NULL && test_array_has_id(scratch, res, "r1"));
    check("5.6 insert after rebuild", 0 == qs_db_record_insert(&ctx, "issue", "r5", "{\"id\":\"r5\",\"status\":\"open\"}"));
    res = qs_db_record_list(scratch, &ctx, "issue", "{\"status\":\"open\"}", QS_DB_SCAN_INDEX);
    check("5.6 index after insert omits new record", res != NULL && test_array_count(scratch, res) == 2 && !test_array_has_id(scratch, res, "r5"));
    check("5.6 remove added record", 0 == qs_db_record_delete(&ctx, "issue", "r5"));

    /* 5.7 該当なしフィルタ */
    res = qs_db_record_list(scratch, &ctx, "issue", "{\"status\":\"nonexistent\"}", QS_DB_SCAN_FULL);
    check("5.7 full no-match count==0", res != NULL && test_array_count(scratch, res) == 0);

    check("5.8 index numeric filter rejected", NULL == qs_db_record_list(scratch, &ctx, "issue", "{\"priority\":1}", QS_DB_SCAN_INDEX));
    check("5.8 index boolean filter rejected", NULL == qs_db_record_list(scratch, &ctx, "issue", "{\"priority\":true}", QS_DB_SCAN_INDEX));
    check("5.8 index array filter rejected", NULL == qs_db_record_list(scratch, &ctx, "issue", "{\"status\":[\"open\"]}", QS_DB_SCAN_INDEX));
    check("5.8 missing index with invalid filter rejected", NULL == qs_db_record_list(scratch, &ctx, "issue", "{\"status\":\"nonexistent\",\"priority\":1}", QS_DB_SCAN_INDEX));
    check("5.8 valid index with invalid filter rejected", NULL == qs_db_record_list(scratch, &ctx, "issue", "{\"status\":\"open\",\"priority\":1}", QS_DB_SCAN_INDEX));
    char boundary_value[48];
    char boundary_filter[100];
    memset(boundary_value, 'a', 44);
    boundary_value[44] = '\0';
    snprintf(boundary_filter, sizeof(boundary_filter), "{\"status\":\"%s\"}", boundary_value);
    res = qs_db_record_list(scratch, &ctx, "issue", boundary_filter, QS_DB_SCAN_INDEX);
    check("5.8 index key 63 bytes accepted", res != NULL && strcmp(res, "[]") == 0);
    boundary_value[44] = 'a';
    boundary_value[45] = '\0';
    snprintf(boundary_filter, sizeof(boundary_filter), "{\"status\":\"%s\"}", boundary_value);
    check("5.8 index key 64 bytes rejected", NULL == qs_db_record_list(scratch, &ctx, "issue", boundary_filter, QS_DB_SCAN_INDEX));
    res = qs_db_record_list(scratch, &ctx, "issue", "{}", QS_DB_SCAN_INDEX);
    check("5.8 empty index filter returns all", res != NULL && test_array_count(scratch, res) == 4);
    res = qs_db_record_list(scratch, &ctx, "issue", "{\"status\":\"open\"}", QS_DB_SCAN_INDEX);
    check("5.8 valid search after errors", res != NULL && test_array_count(scratch, res) == 2);

    check("5.9 insert invalid record", 0 == qs_db_record_insert(&ctx, "issue", "invalid", "[]"));
    check("5.9 full scan rejects invalid record", NULL == qs_db_record_list(scratch, &ctx, "issue", NULL, QS_DB_SCAN_FULL));
    check("5.9 index invalid record", 0 == qs_db_index_add(&ctx, "issue", "status", "open", "invalid"));
    check("5.9 index scan rejects invalid record", NULL == qs_db_record_list(scratch, &ctx, "issue", "{\"status\":\"open\"}", QS_DB_SCAN_INDEX));
    check("5.9 remove invalid index", 0 == qs_db_index_remove(&ctx, "issue", "status", "open", "invalid"));
    check("5.9 delete invalid record", 0 == qs_db_record_delete(&ctx, "issue", "invalid"));
    res = qs_db_record_list(scratch, &ctx, "issue", "{\"status\":\"open\"}", QS_DB_SCAN_INDEX);
    check("5.9 search recovers", res != NULL && test_array_count(scratch, res) == 2);
    g_fail_push_value = "r1";
    check("5.9 key enumeration failure reported", NULL == qs_db_record_list(scratch, &ctx, "issue", NULL, QS_DB_SCAN_FULL));
    g_fail_push_value = NULL;

    char large_record[70000];
    char boundary_record[65534];
    memset(boundary_record, 'x', sizeof(boundary_record));
    memcpy(boundary_record, "{\"data\":\"", 9);
    memcpy(boundary_record + sizeof(boundary_record) - 3, "\"}", 3);
    check("5.10 create boundary table", qs_db_create_table(&ctx, "boundary", QS_DB_CAPACITY_B8MB) == 0);
    check("5.10 insert large boundary record", qs_db_record_insert(&ctx, "boundary", "record", boundary_record) == 0);
    char* boundary_list = qs_db_record_list(scratch, &ctx, "boundary", NULL, QS_DB_SCAN_FULL);
    check("5.10 exact size list succeeds", boundary_list != NULL && strlen(boundary_list) == 65535);
    boundary_record[sizeof(boundary_record) - 3] = 'x';
    boundary_record[sizeof(boundary_record) - 2] = '"';
    boundary_record[sizeof(boundary_record) - 1] = '}';
    char overflow_record[sizeof(boundary_record) + 1];
    memcpy(overflow_record, boundary_record, sizeof(boundary_record));
    overflow_record[sizeof(boundary_record)] = '\0';
    check("5.10 update oversized boundary record", qs_db_record_update(&ctx, "boundary", "record", overflow_record) == 0);
    g_encode_array_calls = 0;
    check("5.10 one byte oversized list rejected", qs_db_record_list(scratch, &ctx, "boundary", NULL, QS_DB_SCAN_FULL) == NULL);
    check("5.10 oversized list reaches encoder", g_encode_array_calls == 1);
    check("5.10 drop boundary table", qs_db_drop_table(&ctx, "boundary") == 0);
    char large_id[32];
    memset(large_record, 'x', sizeof(large_record));
    memcpy(large_record, "{\"data\":\"", 9);
    memcpy(large_record + sizeof(large_record) - 3, "\"}", 3);
    int large_inserted = 1;
    int large_index;
    for (large_index = 0; large_index < 1; large_index++) {
        snprintf(large_id, sizeof(large_id), "large_%d", large_index);
        if (qs_db_record_insert(&ctx, "issue", large_id, large_record) != 0) {
            large_inserted = 0;
            break;
        }
    }
    check("5.10 insert large records", large_inserted);
    g_encode_array_calls = 0;
    check("5.10 large record list rejected", NULL == qs_db_record_list(scratch, &ctx, "issue", NULL, QS_DB_SCAN_FULL));
    check("5.10 large record reaches encoder", g_encode_array_calls == 1);
    for (large_index = 0; large_index < 1; large_index++) {
        snprintf(large_id, sizeof(large_id), "large_%d", large_index);
        qs_db_record_delete(&ctx, "issue", large_id);
    }
    QS_MEMORY_POOL* encode_pool = scratch_alloc();
    if (encode_pool != NULL) {
        check("5.10 large record decodes", qs_json_decode(encode_pool, large_record) != -1);
        int32_t encode_array = qs_create_array(encode_pool, 4);
        check("5.10 create encoder boundary array", encode_array != -1);
        check("5.10 add encoder boundary string", qs_array_push_string(encode_pool, &encode_array, large_record) != -1);
        check("5.10 encoder rejects output overflow", -1 == qs_json_encode_array(encode_pool, encode_array, 64 * 1024));
        scratch_free(encode_pool);
    } else {
        check("5.10 encoder scratch alloc", 0);
    }
    scratch_free(scratch);
    check("5.9 close record list database succeeds", 0 == qs_db_close_database(&ctx));
}

/* ------------------------------------------------------------------ */
/* G6: Index                                                           */
/* ------------------------------------------------------------------ */

static void test_g6_index(const char* base_dir)
{
    printf("\n--- G6 Index ---\n");
    QS_DB_CONTEXT ctx;
    if (-1 == qs_db_open_database(&ctx, base_dir, "database1")) {
        check("6.x open database1", 0);
        return;
    }
    qs_db_create_table(&ctx, "idx", QS_DB_CAPACITY_B1MB);
    qs_db_record_insert(&ctx, "idx", "a", "{\"id\":\"a\",\"tag\":\"x\"}");
    qs_db_record_insert(&ctx, "idx", "b", "{\"id\":\"b\",\"tag\":\"x\"}");
    qs_db_record_insert(&ctx, "idx", "c", "{\"id\":\"c\",\"tag\":\"y\"}");

    QS_MEMORY_POOL* scratch = scratch_alloc();
    if (scratch == NULL) {
        check("6.x scratch alloc", 0);
        check("6.x close database after allocation failure", 0 == qs_db_close_database(&ctx));
        return;
    }

    /* 6.1 index_add */
    check("6.1 index_add a", 0 == qs_db_index_add(&ctx, "idx", "tag", "x", "a"));
    check("6.1 index_add b", 0 == qs_db_index_add(&ctx, "idx", "tag", "x", "b"));

    /* 6.2 重複 add（a を再度）-> 重複なし */
    check("6.2 duplicate index_add a", 0 == qs_db_index_add(&ctx, "idx", "tag", "x", "a"));
    char valbuf[8192];
    int vrc = qs_db_record_get(&ctx, "index", "index:idx:tag:x", valbuf, sizeof(valbuf));
    check("6.2 index tag:x count==2 (no dup)", vrc == 0 && test_array_count(scratch, valbuf) == 2);

    /* 6.3 index_remove（a を削除 -> [b]） */
    check("6.3 index_remove a", 0 == qs_db_index_remove(&ctx, "idx", "tag", "x", "a"));
    vrc = qs_db_record_get(&ctx, "index", "index:idx:tag:x", valbuf, sizeof(valbuf));
    check("6.3 index tag:x count==1 after remove", vrc == 0 && test_array_count(scratch, valbuf) == 1);
    /* b も削除 -> 空でキー削除 */
    check("6.3 index_remove b", 0 == qs_db_index_remove(&ctx, "idx", "tag", "x", "b"));
    check("6.3 index key removed when empty", -1 == qs_db_record_get(&ctx, "index", "index:idx:tag:x", valbuf, sizeof(valbuf)));

    /* 6.4 index_rebuild */
    check("6.4 index_rebuild", 0 == qs_db_index_rebuild(scratch, &ctx, "idx", "[\"tag\"]"));
    vrc = qs_db_record_get(&ctx, "index", "index:idx:tag:x", valbuf, sizeof(valbuf));
    check("6.4 rebuild tag:x count==2", vrc == 0 && test_array_count(scratch, valbuf) == 2);
    vrc = qs_db_record_get(&ctx, "index", "index:idx:tag:y", valbuf, sizeof(valbuf));
    check("6.4 rebuild tag:y count==1", vrc == 0 && test_array_count(scratch, valbuf) == 1);

    QS_DB_STORE held_store;
    int held = qs_db_open_table(&ctx, "idx", QS_DB_CAPACITY_B1MB, &held_store);
    check("6.5 open table for rebuild failure", held == 0);
    if (held == 0) {
        check("6.5 rebuild with open handle rejected", -1 == qs_db_index_rebuild(scratch, &ctx, "idx", "[\"tag\"]"));
        qs_db_close_table(&held_store);
    }
    vrc = qs_db_record_get(&ctx, "index", "index:idx:tag:x", valbuf, sizeof(valbuf));
    check("6.5 failed open preserves index", held == 0 && vrc == 0 && test_array_count(scratch, valbuf) == 2);

    check("6.6 insert oversized index value", 0 == qs_db_record_insert(&ctx, "idx", "long", "{\"tag\":\"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789\"}"));
    check("6.6 rebuild reports oversized key", -1 == qs_db_index_rebuild(scratch, &ctx, "idx", "[\"tag\"]"));
    check("6.6 delete oversized value", 0 == qs_db_record_delete(&ctx, "idx", "long"));
    check("6.6 rebuild recovers", 0 == qs_db_index_rebuild(scratch, &ctx, "idx", "[\"tag\"]"));

    check("6.7 corrupt index value", 0 == qs_db_record_update(&ctx, "index", "index:idx:tag:x", "{}"));
    check("6.7 add rejects corrupt index", -1 == qs_db_index_add(&ctx, "idx", "tag", "x", "a"));
    check("6.7 corrupt index preserved after add", 0 == qs_db_record_get(&ctx, "index", "index:idx:tag:x", valbuf, sizeof(valbuf)) && strcmp(valbuf, "{}") == 0);
    check("6.7 remove rejects corrupt index", -1 == qs_db_index_remove(&ctx, "idx", "tag", "x", "a"));
    check("6.7 corrupt index preserved after remove", 0 == qs_db_record_get(&ctx, "index", "index:idx:tag:x", valbuf, sizeof(valbuf)) && strcmp(valbuf, "{}") == 0);
    check("6.7 rebuild restores corrupt index", 0 == qs_db_index_rebuild(scratch, &ctx, "idx", "[\"tag\"]"));
    g_fail_push_value = "newid";
    check("6.7 index add push failure reported", -1 == qs_db_index_add(&ctx, "idx", "tag", "x", "newid"));
    g_fail_push_value = NULL;
    check("6.7 index preserved after push failure", 0 == qs_db_record_get(&ctx, "index", "index:idx:tag:x", valbuf, sizeof(valbuf)) && test_array_count(scratch, valbuf) == 2);
    g_fail_push_value = "b";
    check("6.7 index remove push failure reported", -1 == qs_db_index_remove(&ctx, "idx", "tag", "x", "a"));
    g_fail_push_value = NULL;
    check("6.7 index preserved after remove failure", 0 == qs_db_record_get(&ctx, "index", "index:idx:tag:x", valbuf, sizeof(valbuf)) && test_array_count(scratch, valbuf) == 2);

    QS_MEMORY_POOL* limit_pool = scratch_alloc();
    if (limit_pool != NULL) {
        int32_t limit_ids = qs_create_array(limit_pool, 1024);
        char limit_id[64];
        int limit_ok = limit_ids != -1;
        int limit_index;
        for (limit_index = 0; limit_index < 1039 && limit_ok; limit_index++) {
            snprintf(limit_id, sizeof(limit_id), "%060d", limit_index);
            limit_ok = qs_array_push_string(limit_pool, &limit_ids, limit_id) != -1;
        }
        int32_t limit_json_id = limit_ok ? qs_json_encode_array(limit_pool, limit_ids, 64 * 1024) : -1;
        char* limit_json = limit_json_id == -1 ? NULL : (char*)QS_GET_POINTER(limit_pool, limit_json_id);
        check("6.8 create near-limit index", limit_json != NULL && 0 == qs_db_record_insert(&ctx, "index", "index:idx:tag:limit", limit_json));
        char before[65536];
        check("6.8 read near-limit index", 0 == qs_db_record_get(&ctx, "index", "index:idx:tag:limit", before, sizeof(before)));
        snprintf(limit_id, sizeof(limit_id), "%060d", 1039);
        check("6.8 adding id near limit succeeds", 0 == qs_db_index_add(&ctx, "idx", "tag", "limit", limit_id));
        check("6.8 read index before rejected add", 0 == qs_db_record_get(&ctx, "index", "index:idx:tag:limit", before, sizeof(before)));
        snprintf(limit_id, sizeof(limit_id), "%060d", 1040);
        check("6.8 adding id over limit rejected", -1 == qs_db_index_add(&ctx, "idx", "tag", "limit", limit_id));
        char after[65536];
        check("6.8 index preserved after rejected add", 0 == qs_db_record_get(&ctx, "index", "index:idx:tag:limit", after, sizeof(after)) && strcmp(before, after) == 0);
        scratch_free(limit_pool);
    } else {
        check("6.8 allocate index boundary pool", 0);
    }

    scratch_free(scratch);
    check("6.9 close index database succeeds", 0 == qs_db_close_database(&ctx));
}

/* ------------------------------------------------------------------ */
/* G7: System（id 採番）+ G8 永続化                                     */
/* ------------------------------------------------------------------ */

static void test_g7_g8(const char* base_dir)
{
    printf("\n--- G7 System / G8 Persistence ---\n");
    QS_DB_CONTEXT ctx;
    if (-1 == qs_db_open_database(&ctx, base_dir, "database1")) {
        check("7.x open database1", 0);
        return;
    }
    qs_db_create_table(&ctx, "seq", QS_DB_CAPACITY_B1MB);

    /* 7.1 next_id 連番 */
    char id1[128], id2[128];
    check("7.1 next_id #1", 0 == qs_db_next_id(&ctx, "seq", "seq_", id1, sizeof(id1)));
    check_str("7.1 id1 == seq_1", id1, "seq_1");
    check("7.1 next_id #2", 0 == qs_db_next_id(&ctx, "seq", "seq_", id2, sizeof(id2)));
    check_str("7.1 id2 == seq_2", id2, "seq_2");

    char short_id[2] = {'x', '\0'};
    check("7.1 next_id short output rejected", -1 == qs_db_next_id(&ctx, "seq", "seq_", short_id, sizeof(short_id)));
    check("7.1 next_id short output does not consume count", 0 == qs_db_next_id(&ctx, "seq", "seq_", id2, sizeof(id2)));
    check_str("7.1 next_id short output keeps next sequence", id2, "seq_3");
    check("7.1 next_id empty prefix rejected", -1 == qs_db_next_id(&ctx, "seq", "", id2, sizeof(id2)));
    check("7.1 next_id null prefix rejected", -1 == qs_db_next_id(&ctx, "seq", NULL, id2, sizeof(id2)));
    char long_prefix[128];
    memset(long_prefix, 'p', sizeof(long_prefix) - 1);
    long_prefix[sizeof(long_prefix) - 1] = '\0';
    check("7.1 next_id long prefix rejected", -1 == qs_db_next_id(&ctx, "seq", long_prefix, id2, sizeof(id2)));
    check("7.1 next_id long prefix does not consume count", 0 == qs_db_next_id(&ctx, "seq", "seq_", id2, sizeof(id2)));
    check_str("7.1 next_id long prefix keeps next sequence", id2, "seq_4");

    check("7.1 set malformed counter", 0 == qs_db_record_update(&ctx, "system", "counter:seq", "not-a-number"));
    check("7.1 malformed counter rejected", -1 == qs_db_next_id(&ctx, "seq", "seq_", id2, sizeof(id2)));
    check("7.1 reset malformed counter", 0 == qs_db_record_update(&ctx, "system", "counter:seq", "4"));
    check("7.1 set overflowing counter", 0 == qs_db_record_update(&ctx, "system", "counter:seq", "9223372036854775807"));
    check("7.1 overflowing counter rejected", -1 == qs_db_next_id(&ctx, "seq", "seq_", id2, sizeof(id2)));
    check("7.1 reset overflowing counter", 0 == qs_db_record_update(&ctx, "system", "counter:seq", "4"));
    /* 8.1 再 open でレコード復元 */
    qs_db_record_insert(&ctx, "seq", "persist1", "{\"id\":\"persist1\",\"v\":42}");
    check("8.1 close before reopen succeeds", 0 == qs_db_close_database(&ctx));
    check("8.1 reopen database1", 0 == qs_db_open_database(&ctx, base_dir, "database1"));
    char pbuf[512];
    check("8.1 get after reopen", 0 == qs_db_record_get(&ctx, "seq", "persist1", pbuf, sizeof(pbuf)) && strcmp(pbuf, "{\"id\":\"persist1\",\"v\":42}") == 0);

    /* 7.2 再 open で採番継続 */
    char id3[128];
    check("7.2 next_id after reopen", 0 == qs_db_next_id(&ctx, "seq", "seq_", id3, sizeof(id3)));
    check_str("7.2 id3 == seq_5 (continued)", id3, "seq_5");

    /* 8.2 再 open で index 復元 */
    qs_db_create_table(&ctx, "pidx", QS_DB_CAPACITY_B1MB);
    qs_db_record_insert(&ctx, "pidx", "p1", "{\"id\":\"p1\",\"tag\":\"z\"}");
    qs_db_index_add(&ctx, "pidx", "tag", "z", "p1");
    check("8.2 close before index reopen succeeds", 0 == qs_db_close_database(&ctx));
    check("8.2 reopen for index", 0 == qs_db_open_database(&ctx, base_dir, "database1"));
    QS_MEMORY_POOL* scratch = scratch_alloc();
    if (scratch != NULL) {
        char valbuf[8192];
        int vrc = qs_db_record_get(&ctx, "index", "index:pidx:tag:z", valbuf, sizeof(valbuf));
        check("8.2 index restored after reopen", vrc == 0 && test_array_count(scratch, valbuf) == 1);
        scratch_free(scratch);
    } else {
        check("8.2 scratch alloc", 0);
    }

    check("8.2 final close succeeds", 0 == qs_db_close_database(&ctx));
}

/* ------------------------------------------------------------------ */
/* main                                                                */
/* ------------------------------------------------------------------ */

int main(int argc, char* argv[])
{
    test_try_close_null_context();
    if (argc == 4 && strcmp(argv[1], "--lock-check") == 0) {
        QS_DB_CONTEXT ctx;
        if (strcmp(argv[3], "drop-rejected") == 0) {
            return qs_db_drop_database(argv[2], "database1") == -1 ? 0 : 1;
        }
        const char* database = strcmp(argv[3], "other-open") == 0 ? "database2" :
                       strcmp(argv[3], "truncated-rejected") == 0 ? "truncated" : "database1";
        int result = qs_db_open_database(&ctx, argv[2], database);
        if (result == 0) {
            if (qs_db_close_database(&ctx) != 0) {
                return 1;
            }
        }
        if (strcmp(argv[3], "open-rejected") == 0 || strcmp(argv[3], "truncated-rejected") == 0) {
            return result == -1 ? 0 : 1;
        }
        return result == 0 ? 0 : 1;
    }

    char tmpl[] = "/tmp/qs_datastore_test_XXXXXX";
    char* base_dir = mkdtemp(tmpl);
    if (base_dir == NULL) {
        fprintf(stderr, "failed to create temp dir\n");
        return -1;
    }
    printf("base_dir: %s\n", base_dir);

    test_g1_database(base_dir);
    test_g2_table(base_dir);
    test_g3_schema(base_dir);
    test_g4_record(base_dir);
    test_g5_record_list(base_dir);
    test_g6_index(base_dir);
    test_g7_g8(base_dir);

    printf("\n=== datastore_test summary ===\n");
    printf("pass: %d, fail: %d\n", g_pass, g_fail);

    rm_rf(base_dir);
    return (g_fail == 0) ? 0 : 1;
}
