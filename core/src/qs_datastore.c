/*
 * Copyright (c) Katsuya Owari
 *
 * qs_datastore
 *   汎用データストア（DB）管理ライブラリ（低レベル実装）。
 *
 * 本ファイルは core の低レベル API（qs_create_storage_cache / qs_cache_string /
 * qs_json_decode / qs_get_json_root_hash / qs_opendir 等）のみを使用する。
 * qs_api.h の高レベル関数（api_qs_*）には依存しない。
 */

#include "qs_datastore.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef __WINDOWS__
#include <direct.h>
#include <io.h>
#else
#include <pthread.h>
#endif
#if defined(__LINUX__) || defined(__BSD_UNIX__) || defined(__ANDROID__) || defined(__IOS__)
#include <dirent.h>
#include <sys/file.h>
#include <unistd.h>
#endif

/* 内部で使うキーの最大長（永続化 KVS の max_key_size と一致させる） */
#define QS_DB_MAX_KEY 64
/* record 操作時のデフォルト容量（MB） */
#define QS_DB_DEFAULT_CAPACITY QS_DB_CAPACITY_B1MB
/* スクラッチメモリサイズ（JSON decode/encode 用） */
#define QS_DB_SCRATCH_SIZE (SIZE_MBYTE * 2)
#define QS_DB_OPEN_REGISTRY_SIZE 256
#define QS_DB_CAPACITY_KEY_PREFIX "table:"
#define QS_DB_MAX_NAME 45

#ifdef __WINDOWS__
static SRWLOCK db_open_registry_lock = SRWLOCK_INIT;
#define db_registry_lock() AcquireSRWLockExclusive(&db_open_registry_lock)
#define db_registry_unlock() ReleaseSRWLockExclusive(&db_open_registry_lock)
#else
static pthread_mutex_t db_open_registry_lock = PTHREAD_MUTEX_INITIALIZER;
#define db_registry_lock() pthread_mutex_lock(&db_open_registry_lock)
#define db_registry_unlock() pthread_mutex_unlock(&db_open_registry_lock)
#endif
static char db_open_registry[QS_DB_OPEN_REGISTRY_SIZE][4352];
static QS_DB_CONTEXT* db_open_registry_owner[QS_DB_OPEN_REGISTRY_SIZE];

#ifdef __WINDOWS__
typedef struct db_directory {
    HANDLE search;
    WIN32_FIND_DATAA entry;
    char pattern[4352];
    int first;
} DB_DIRECTORY;

static DB_DIRECTORY* db_directory_open(const char* path)
{
    DB_DIRECTORY* dir = malloc(sizeof(*dir));
    if (dir == NULL) return NULL;
    if (snprintf(dir->pattern, sizeof(dir->pattern), "%s/*", path) < 0) {
        free(dir);
        return NULL;
    }
    dir->search = FindFirstFileA(dir->pattern, &dir->entry);
    if (dir->search == INVALID_HANDLE_VALUE) {
        free(dir);
        return NULL;
    }
    dir->first = 1;
    return dir;
}

static const char* db_directory_read(DB_DIRECTORY* dir)
{
    if (dir->first) {
        dir->first = 0;
        return dir->entry.cFileName;
    }
    if (FindNextFileA(dir->search, &dir->entry)) return dir->entry.cFileName;
    if (GetLastError() != ERROR_NO_MORE_FILES) errno = EIO;
    return NULL;
}

static int db_directory_rewind(DB_DIRECTORY* dir)
{
    FindClose(dir->search);
    dir->search = FindFirstFileA(dir->pattern, &dir->entry);
    dir->first = 1;
    return dir->search == INVALID_HANDLE_VALUE ? -1 : 0;
}

static void db_directory_close(DB_DIRECTORY* dir)
{
    if (dir->search != INVALID_HANDLE_VALUE) FindClose(dir->search);
    free(dir);
}

static int db_path_type(const char* path)
{
    DWORD attributes = GetFileAttributesA(path);
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        DWORD error = GetLastError();
        errno = (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) ? ENOENT : EIO;
        return -1;
    }
    if (attributes & FILE_ATTRIBUTE_REPARSE_POINT) return 0;
    return (attributes & FILE_ATTRIBUTE_DIRECTORY) ? 2 : 1;
}
#else
typedef DIR DB_DIRECTORY;

static DB_DIRECTORY* db_directory_open(const char* path) { return opendir(path); }
static const char* db_directory_read(DB_DIRECTORY* dir)
{
    struct dirent* entry = readdir(dir);
    return entry == NULL ? NULL : entry->d_name;
}
static int db_directory_rewind(DB_DIRECTORY* dir) { rewinddir(dir); return 0; }
static void db_directory_close(DB_DIRECTORY* dir) { closedir(dir); }
static int db_path_type(const char* path)
{
    struct stat st;
    if (lstat(path, &st) != 0) return -1;
    if (S_ISREG(st.st_mode)) return 1;
    if (S_ISDIR(st.st_mode)) return 2;
    return 0;
}
#endif

static int db_resolve_path(const char* path, char* resolved, size_t size)
{
#ifdef __WINDOWS__
    HANDLE dir = CreateFileA(path, FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                             NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (dir == INVALID_HANDLE_VALUE) return -1;
    DWORD length = GetFinalPathNameByHandleA(dir, resolved, (DWORD)size, FILE_NAME_NORMALIZED);
    CloseHandle(dir);
    return length == 0 || length >= size ? -1 : 0;
#else
    (void)size;
    return realpath(path, resolved) == NULL ? -1 : 0;
#endif
}

static int db_paths_equal(const char* first, const char* second)
{
#ifdef __WINDOWS__
    return _stricmp(first, second) == 0;
#else
    return strcmp(first, second) == 0;
#endif
}

static void* db_lock_directory(const char* path)
{
#ifdef __WINDOWS__
    HANDLE dir = CreateFileA(path, FILE_READ_ATTRIBUTES, FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                             NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    return dir == INVALID_HANDLE_VALUE ? NULL : dir;
#else
    DIR* dir = opendir(path);
    if (dir == NULL) return NULL;
    if (flock(dirfd(dir), LOCK_EX | LOCK_NB) != 0) {
        closedir(dir);
        return NULL;
    }
    return dir;
#endif
}

static void db_unlock_directory(void* lock)
{
#ifdef __WINDOWS__
    CloseHandle((HANDLE)lock);
#else
    closedir((DIR*)lock);
#endif
}

/* ------------------------------------------------------------------ */
/* 内部ヘルパー                                                         */
/* ------------------------------------------------------------------ */

static int db_path_join(char* out, size_t out_size, const char* a, const char* b)
{
    int n = snprintf(out, out_size, "%s/%s", a, b);
    if (n <= 0 || (size_t)n >= out_size) {
        return -1;
    }
    return 0;
}

static int db_valid_name(const char* name)
{
    size_t len;
    size_t i;
    if (name == NULL) {
        return 0;
    }
    len = strlen(name);
    if (len == 0 || len > QS_DB_MAX_NAME || strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
        return 0;
    }
    for (i = 0; i < len; i++) {
        unsigned char c = (unsigned char)name[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-')) {
            return 0;
        }
    }
    return 1;
}

static int db_valid_key(const char* key)
{
    size_t len;
    size_t i;
    if (key == NULL) {
        return 0;
    }
    len = strlen(key);
    if (len == 0 || len >= QS_DB_MAX_KEY) {
        return 0;
    }
    for (i = 0; i < len; i++) {
        unsigned char c = (unsigned char)key[i];
        if (c < 0x20 || c == '/' || c == '\\') {
            return 0;
        }
    }
    return 1;
}

static int db_reserved_table(const char* table)
{
    return strcmp(table, "system") == 0 || strcmp(table, "schema") == 0 ||
           strcmp(table, "index") == 0;
}

static int db_open_registry_contains_locked(const char* resolved)
{
    int i;
    for (i = 0; i < QS_DB_OPEN_REGISTRY_SIZE; i++) {
        if (db_open_registry[i][0] != '\0' && db_paths_equal(db_open_registry[i], resolved)) {
            return 1;
        }
    }
    return 0;
}

static int db_open_registry_add(const char* path, QS_DB_CONTEXT* owner)
{
    char resolved[4352];
    int free_slot = -1;
    int i;
    if (db_resolve_path(path, resolved, sizeof(resolved)) != 0) {
        return -1;
    }
    db_registry_lock();
    for (i = 0; i < QS_DB_OPEN_REGISTRY_SIZE; i++) {
        if (db_open_registry[i][0] == '\0') {
            if (free_slot == -1) {
                free_slot = i;
            }
        } else if (db_open_registry_owner[i] == owner || db_paths_equal(db_open_registry[i], resolved)) {
            db_registry_unlock();
            return -1;
        }
    }
    if (free_slot == -1) {
        db_registry_unlock();
        return -1;
    }
    snprintf(db_open_registry[free_slot], sizeof(db_open_registry[free_slot]), "%s", resolved);
    db_open_registry_owner[free_slot] = owner;
    db_registry_unlock();
    return 0;
}

static void db_open_registry_remove(QS_DB_CONTEXT* owner)
{
    db_registry_lock();
    int i;
    for (i = 0; i < QS_DB_OPEN_REGISTRY_SIZE; i++) {
        if (db_open_registry_owner[i] == owner) {
            db_open_registry[i][0] = '\0';
            db_open_registry_owner[i] = NULL;
            break;
        }
    }
    db_registry_unlock();
}

static int db_capacity_key(char* out, size_t out_size, const char* table)
{
    int n = snprintf(out, out_size, QS_DB_CAPACITY_KEY_PREFIX "%s:capacity_mb", table);
    return (n > 0 && (size_t)n < out_size && (size_t)n < QS_DB_MAX_KEY) ? 0 : -1;
}

static int db_file_exists(const char* path)
{
    struct stat st;
    return (stat(path, &st) == 0);
}

static int db_is_dir(const char* path)
{
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    return S_ISDIR(st.st_mode);
}

/* 容量（MB）から永続化 KVS のパラメータを決定する。 */
static int db_capacity_params(int capacity_mb, size_t* p_cache, size_t* p_page, size_t* p_hash, size_t* p_key)
{
    switch (capacity_mb) {
        case QS_DB_CAPACITY_B1MB:
            *p_cache = 1; *p_page = SIZE_MBYTE * 980; *p_hash = 1000; *p_key = 64;
            return 0;
        case QS_DB_CAPACITY_B8MB:
            *p_cache = 1; *p_page = SIZE_MBYTE * 8; *p_hash = 1000; *p_key = 64;
            return 0;
        case QS_DB_CAPACITY_B16MB:
            *p_cache = 20000; *p_page = SIZE_MBYTE * 16; *p_hash = 65536; *p_key = 64;
            return 0;
        case QS_DB_CAPACITY_B32MB:
            *p_cache = 40000; *p_page = SIZE_MBYTE * 32; *p_hash = 131072; *p_key = 64;
            return 0;
        case QS_DB_CAPACITY_B64MB:
            *p_cache = 80000; *p_page = SIZE_MBYTE * 64; *p_hash = 262144; *p_key = 64;
            return 0;
        case QS_DB_CAPACITY_B128MB:
            *p_cache = 50000; *p_page = SIZE_MBYTE * 128; *p_hash = 1000000; *p_key = 64;
            return 0;
        case QS_DB_CAPACITY_B256MB:
            *p_cache = 50000; *p_page = SIZE_MBYTE * 256; *p_hash = 1500000; *p_key = 128;
            return 0;
        case QS_DB_CAPACITY_B512MB:
            *p_cache = 100000; *p_page = SIZE_MBYTE * 512; *p_hash = 1500000; *p_key = 128;
            return 0;
        case QS_DB_CAPACITY_B1024MB:
            *p_cache = 100000; *p_page = SIZE_MBYTE * 1024; *p_hash = 5000000; *p_key = 256;
            return 0;
        default:
            return -1;
    }
}

/* 永続化 KVS を作成（新規ファイル）。 */
static int db_store_create(QS_DB_STORE* store, const char* path, int capacity_mb)
{
    size_t cache, page, hash, key;
    memset(store, 0, sizeof(*store));
    store->cache_id = -1;
    if (db_capacity_params(capacity_mb, &cache, &page, &hash, &key) != 0) {
        return -1;
    }
    QS_MEMORY_POOL* memory = NULL;
    int32_t cache_id = qs_create_storage_cache(path, &memory, cache, page, hash, key);
    if (-1 == cache_id) {
        if (memory != NULL) {
            qs_free(memory);
        }
        return -1;
    }
    store->memory = memory;
    store->cache_id = cache_id;
    snprintf(store->path, sizeof(store->path), "%s", path);
    store->is_open = 1;
    return 0;
}

/* 既存の永続化 KVS をオープン。ファイルが存在しない場合は -1。 */
static int db_store_open(QS_DB_STORE* store, const char* path, int capacity_mb)
{
    size_t cache, page, hash, key;
#ifdef __WINDOWS__
    struct __stat64 st;
    if (db_capacity_params(capacity_mb, &cache, &page, &hash, &key) != 0 ||
        _stat64(path, &st) != 0 || !S_ISREG(st.st_mode)) {
#else
    struct stat st;
    if (db_capacity_params(capacity_mb, &cache, &page, &hash, &key) != 0 ||
        stat(path, &st) != 0 || !S_ISREG(st.st_mode)) {
#endif
        return -1;
    }
    size_t allocate_size = QS_ALIGNUP((cache * (key + 8)) + SIZE_KBYTE + page + SIZE_KBYTE * 4,
                                      MEMORY_ALIGNMENT_SIZE_BIT_64);
    if (st.st_size < 0 || (uintmax_t)st.st_size != (uintmax_t)allocate_size + 1) {
        return -1;
    }
    uint32_t header[14];
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        return -1;
    }
    size_t read_count = fread(header, sizeof(header[0]), 14, file);
    fclose(file);
    if (read_count != 14 || header[0] != allocate_size || header[3] != allocate_size ||
        header[4] != allocate_size || header[5] != MEMORY_ALIGNMENT_SIZE_BIT_64 ||
        header[12] != MEMORY_ALLOCATE_TYPE_MMAP) {
        return -1;
    }
    return db_store_create(store, path, capacity_mb);
}

static void db_store_close(QS_DB_STORE* store)
{
    if (store->is_open && store->memory != NULL && !store->borrowed) {
        qs_free(store->memory);
    }
    store->memory = NULL;
    store->cache_id = -1;
    store->is_open = 0;
    store->borrowed = 0;
}

static int db_store_set(QS_DB_STORE* store, const char* key, const char* value)
{
    if (strlen(key) >= QS_DB_MAX_KEY) {
        return -1;
    }
    QS_CACHE* cache = (QS_CACHE*)QS_GET_POINTER(store->memory, store->cache_id);
    return qs_cache_string(cache, (char*)key, (char*)value, 0);
}

static char* db_store_get(QS_DB_STORE* store, const char* key)
{
    QS_CACHE* cache = (QS_CACHE*)QS_GET_POINTER(store->memory, store->cache_id);
    QS_CACHE_PAGE page;
    qs_get_cache_page(cache, &page);
    int32_t memid = qs_get_hash(page.memory, page.hash_id, key);
    if (-1 == memid) {
        return NULL;
    }
    return (char*)QS_GET_POINTER(page.memory, memid);
}

static int db_store_delete(QS_DB_STORE* store, const char* key)
{
    QS_CACHE* cache = (QS_CACHE*)QS_GET_POINTER(store->memory, store->cache_id);
    QS_CACHE_PAGE page;
    qs_get_cache_page(cache, &page);
    return qs_remove_hash(page.memory, page.hash_id, key);
}

/* ストアの全キーを _ppool 上の配列に格納し、配列 munit を返す。 */
static int32_t db_store_keys(QS_MEMORY_POOL* _ppool, QS_DB_STORE* store)
{
    QS_CACHE* cache = (QS_CACHE*)QS_GET_POINTER(store->memory, store->cache_id);
    QS_CACHE_PAGE page;
    qs_get_cache_page(cache, &page);
    int32_t memid_array = qs_create_array(_ppool, 128);
    if (-1 == memid_array) {
        return -1;
    }
    QS_HASH_FOREACH hf;
    QS_HASH_ELEMENT* he;
    qs_init_hash_foreach(page.memory, page.hash_id, &hf);
    while (NULL != (he = qs_hash_foreach(page.memory, &hf))) {
        char* key = (char*)QS_GET_POINTER(page.memory, he->memid_hash_name);
        if (-1 == qs_array_push_string(_ppool, &memid_array, key)) {
            return -1;
        }
    }
    return memid_array;
}

/* JSON 文字列をデコードし、ルートがオブジェクト（hash）の場合はその munit を返す。 */
static int32_t db_decode_object(QS_MEMORY_POOL* _ppool, const char* json)
{
    int32_t root = qs_json_decode(_ppool, json);
    if (-1 == root) {
        return -1;
    }
    return qs_get_json_root_hash(_ppool, root);
}

/* JSON 文字列をデコードし、ルートが配列の場合はその munit を返す。 */
static int32_t db_decode_array(QS_MEMORY_POOL* _ppool, const char* json)
{
    int32_t root = qs_json_decode(_ppool, json);
    if (-1 == root) {
        return -1;
    }
    QS_NODE* node = qs_get_json_root(_ppool, root);
    if (NULL == node || node->id != ELEMENT_ARRAY) {
        return -1;
    }
    return node->element_munit;
}

static char* db_empty_json_array(QS_MEMORY_POOL* _ppool)
{
    int32_t memid = qs_create_memory_block(_ppool, 3);
    if (-1 == memid) {
        return NULL;
    }
    char* json = (char*)QS_GET_POINTER(_ppool, memid);
    memcpy(json, "[]", 3);
    return json;
}

/* index キー: index:{table}:{field}:{value} */
static int db_index_key(char* out, size_t out_size, const char* table, const char* field, const char* value)
{
    int n = snprintf(out, out_size, "index:%s:%s:%s", table, field, value);
    if (n <= 0 || (size_t)n >= out_size) {
        return -1;
    }
    return 0;
}

static int db_index_ids_fit(QS_MEMORY_POOL* pool, int32_t ids)
{
    size_t size = 3;
    int32_t count = qs_array_length(pool, ids);
    int32_t index;
    for (index = 0; index < count; index++) {
        QS_ARRAY_ELEMENT* element = qs_array_get(pool, ids, index);
        if (element == NULL || element->id != ELEMENT_LITERAL_STR) {
            return 0;
        }
        const unsigned char* value = (const unsigned char*)QS_GET_POINTER(pool, element->memid_array_element_data);
        size += 2 + (index > 0);
        while (*value != '\0') {
            size += (*value == '"' || *value == '\'' || *value == '\a' || *value == '\b' ||
                     *value == '\n' || *value == '\r' || *value == '\f' || *value == '\t' ||
                     *value == '\v' || *value == '\\') ? 2 : 1;
            if (size >= 64 * 1024) {
                return 0;
            }
            value++;
        }
        if (size >= 64 * 1024) {
            return 0;
        }
    }
    return 1;
}

/* index 値（id の JSON 配列）に record_id を追加して書き戻す。 */
static int db_index_add_id(QS_DB_CONTEXT* ctx, const char* index_key, const char* record_id)
{
    QS_MEMORY_POOL* scratch = NULL;
    if (0 == qs_initialize_memory_f64(&scratch, QS_DB_SCRATCH_SIZE)) {
        return -1;
    }

    char* old = db_store_get(&ctx->index, index_key);
    int32_t ids = -1;
    if (old != NULL) {
        ids = db_decode_array(scratch, old);
    } else {
        ids = qs_create_array(scratch, 16);
    }
    if (-1 == ids) {
        qs_free(scratch);
        return -1;
    }
    /* 重複チェック */
    int32_t len = qs_array_length(scratch, ids);
    int i;
    int dup = 0;
    for (i = 0; i < len; i++) {
        QS_ARRAY_ELEMENT* elm = qs_array_get(scratch, ids, i);
        if (elm == NULL || elm->id != ELEMENT_LITERAL_STR) {
            continue;
        }
        char* v = (char*)QS_GET_POINTER(scratch, elm->memid_array_element_data);
        if (strcmp(v, record_id) == 0) {
            dup = 1;
            break;
        }
    }
    if (!dup && -1 == qs_array_push_string(scratch, &ids, record_id)) {
        qs_free(scratch);
        return -1;
    }
    if (!db_index_ids_fit(scratch, ids)) {
        qs_free(scratch);
        return -1;
    }
    int32_t memid_json = qs_json_encode_array(scratch, ids, 64 * 1024);
    int rc = -1;
    if (-1 != memid_json) {
        char* encoded = (char*)QS_GET_POINTER(scratch, memid_json);
        rc = db_store_set(&ctx->index, index_key, encoded);
    }
    qs_free(scratch);
    return rc;
}

/* index 値（id の JSON 配列）から record_id を削除して書き戻す。空ならキーを削除。 */
static int db_index_remove_id(QS_DB_CONTEXT* ctx, const char* index_key, const char* record_id)
{
    char* old = db_store_get(&ctx->index, index_key);
    if (old == NULL) {
        return 0;
    }
    QS_MEMORY_POOL* scratch = NULL;
    if (0 == qs_initialize_memory_f64(&scratch, QS_DB_SCRATCH_SIZE)) {
        return -1;
    }
    int32_t ids = db_decode_array(scratch, old);
    if (-1 == ids) {
        qs_free(scratch);
        return -1;
    }
    int32_t filtered = qs_create_array(scratch, 16);
    if (-1 == filtered) {
        qs_free(scratch);
        return -1;
    }
    int32_t len = qs_array_length(scratch, ids);
    int i;
    for (i = 0; i < len; i++) {
        QS_ARRAY_ELEMENT* elm = qs_array_get(scratch, ids, i);
        if (elm == NULL || elm->id != ELEMENT_LITERAL_STR) {
            continue;
        }
        char* v = (char*)QS_GET_POINTER(scratch, elm->memid_array_element_data);
        if (strcmp(v, record_id) == 0) {
            continue;
        }
        if (-1 == qs_array_push_string(scratch, &filtered, v)) {
            qs_free(scratch);
            return -1;
        }
    }
    int rc;
    if (qs_array_length(scratch, filtered) == 0) {
        rc = db_store_delete(&ctx->index, index_key);
    } else {
        if (!db_index_ids_fit(scratch, filtered)) {
            qs_free(scratch);
            return -1;
        }
        int32_t memid_json = qs_json_encode_array(scratch, filtered, 64 * 1024);
        rc = -1;
        if (-1 != memid_json) {
            char* encoded = (char*)QS_GET_POINTER(scratch, memid_json);
            rc = db_store_set(&ctx->index, index_key, encoded);
        }
    }
    qs_free(scratch);
    return rc;
}

/* レコードのフィールド値がフィルタ値と一致するか（string 一致、または配列に含む）。 */
static int db_record_match(QS_MEMORY_POOL* _ppool, int32_t record_hash, const char* field, const char* value)
{
    int32_t fid = qs_get_hash_id(_ppool, record_hash, field);
    if (fid == ELEMENT_LITERAL_STR) {
        char* s = qs_get_hash_string(_ppool, record_hash, field);
        return (s != NULL && strcmp(s, value) == 0);
    }
    if (fid == ELEMENT_ARRAY) {
        int32_t arr = qs_get_hash(_ppool, record_hash, field);
        int32_t len = qs_array_length(_ppool, arr);
        int i;
        for (i = 0; i < len; i++) {
            QS_ARRAY_ELEMENT* elm = qs_array_get(_ppool, arr, i);
            if (elm == NULL || elm->id != ELEMENT_LITERAL_STR) {
                continue;
            }
            char* v = (char*)QS_GET_POINTER(_ppool, elm->memid_array_element_data);
            if (strcmp(v, value) == 0) {
                return 1;
            }
        }
        return 0;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Database                                                            */
/* ------------------------------------------------------------------ */

int qs_db_create_database(const char* base_dir, const char* database)
{
    char db_path[4352];
    if (!db_valid_name(database)) {
        return -1;
    }
    if (db_path_join(db_path, sizeof(db_path), base_dir, database) != 0) {
        return -1;
    }
#ifdef __WINDOWS__
    if (_mkdir(db_path) != 0) {
#else
    if (mkdir(db_path, 0755) != 0) {
#endif
        return -1;
    }
    QS_DB_STORE store;
    char path[4352];
    const char* files[] = {QS_DB_SYSTEM_FILE, QS_DB_SCHEMA_FILE, QS_DB_INDEX_FILE};
    size_t attempted = 0;
    size_t index;
    for (index = 0; index < sizeof(files) / sizeof(files[0]); index++) {
        if (db_path_join(path, sizeof(path), db_path, files[index]) != 0) {
            break;
        }
        attempted++;
        if (db_store_create(&store, path, QS_DB_CAPACITY_B1MB) != 0) {
            break;
        }
        db_store_close(&store);
    }
    if (index == sizeof(files) / sizeof(files[0])) {
        return 0;
    }
    while (attempted > 0) {
        attempted--;
        if (db_path_join(path, sizeof(path), db_path, files[attempted]) == 0) {
            unlink(path);
        }
    }
    rmdir(db_path);
    return -1;
}

int qs_db_drop_database(const char* base_dir, const char* database)
{
    char db_path[4352];
    char resolved[4352];
    if (!db_valid_name(database)) {
        return -1;
    }
    if (db_path_join(db_path, sizeof(db_path), base_dir, database) != 0) {
        return -1;
    }
    if (!db_is_dir(db_path) || db_resolve_path(db_path, resolved, sizeof(resolved)) != 0) {
        return -1;
    }
    db_registry_lock();
    if (db_open_registry_contains_locked(resolved)) {
        db_registry_unlock();
        return -1;
    }
    void* lock_dir = db_lock_directory(db_path);
    if (lock_dir == NULL) {
        db_registry_unlock();
        return -1;
    }
    DB_DIRECTORY* dir = db_directory_open(db_path);
    if (dir == NULL) {
        db_unlock_directory(lock_dir);
        db_registry_unlock();
        return -1;
    }
    int rc = 0;
    const char* name;
    errno = 0;
    while ((name = db_directory_read(dir)) != NULL) {
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue;
        }
        char path[4352];
        char table[256];
        size_t name_len = strlen(name);
        if (name_len <= 3 || name_len - 3 >= sizeof(table) ||
            strcmp(name + name_len - 3, ".db") != 0) {
            rc = -1;
            break;
        }
        memcpy(table, name, name_len - 3);
        table[name_len - 3] = '\0';
        if (!db_valid_name(table) || db_path_join(path, sizeof(path), db_path, name) != 0 ||
            db_path_type(path) != 1) {
            rc = -1;
            break;
        }
        errno = 0;
    }
    if (rc == 0 && errno != 0) {
        rc = -1;
    }
    if (rc == 0) {
        if (db_directory_rewind(dir) != 0) rc = -1;
        errno = 0;
        while (rc == 0 && (name = db_directory_read(dir)) != NULL) {
            if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
                continue;
            }
            char path[4352];
            char table[256];
            size_t name_len = strlen(name);
            if (name_len <= 3 || name_len - 3 >= sizeof(table) ||
                strcmp(name + name_len - 3, ".db") != 0) {
                rc = -1;
                break;
            }
            memcpy(table, name, name_len - 3);
            table[name_len - 3] = '\0';
            if (!db_valid_name(table) || db_path_join(path, sizeof(path), db_path, name) != 0 ||
                db_path_type(path) != 1 || unlink(path) != 0) {
                rc = -1;
                break;
            }
            errno = 0;
        }
        if (rc == 0 && errno != 0) {
            rc = -1;
        }
    }
    if (rc == 0 && rmdir(db_path) != 0) {
        rc = -1;
    }
    db_directory_close(dir);
    db_unlock_directory(lock_dir);
    db_registry_unlock();
    return rc;
}

int32_t qs_db_show_database(QS_MEMORY_POOL* _ppool, const char* base_dir)
{
    if (_ppool == NULL || base_dir == NULL) {
        return -1;
    }
    char normalized[4352];
    size_t base_len = strlen(base_dir);
    while (base_len > 1 && base_dir[base_len - 1] == '/') {
        base_len--;
    }
    if (base_len == 0 || base_len >= sizeof(normalized)) {
        return -1;
    }
    memcpy(normalized, base_dir, base_len);
    normalized[base_len] = '\0';
    DB_DIRECTORY* dir = db_directory_open(normalized);
    if (dir == NULL) {
        return -1;
    }
    int32_t result = qs_create_array(_ppool, 16);
    if (-1 == result) {
        db_directory_close(dir);
        return -1;
    }
    int rc = 0;
    const char* name;
    errno = 0;
    while ((name = db_directory_read(dir)) != NULL) {
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue;
        }
        if (!db_valid_name(name)) {
            continue;
        }
        char path[4352];
        char system_path[4352];
        if (db_path_join(path, sizeof(path), normalized, name) != 0) {
            rc = -1;
            break;
        }
        int path_type = db_path_type(path);
        if (path_type < 0) {
            rc = -1;
            break;
        }
        if (path_type != 2) {
            continue;
        }
        if (db_path_join(system_path, sizeof(system_path), path, QS_DB_SYSTEM_FILE) != 0) {
            rc = -1;
            break;
        }
        path_type = db_path_type(system_path);
        if (path_type == 1) {
            if (-1 == qs_array_push_string(_ppool, &result, name)) {
                rc = -1;
                break;
            }
        } else if (path_type < 0 && errno != ENOENT) {
            rc = -1;
            break;
        }
        errno = 0;
    }
    if (rc == 0 && errno != 0) rc = -1;
    db_directory_close(dir);
    if (rc != 0) return -1;
    return result;
}

int qs_db_open_database(QS_DB_CONTEXT* ctx, const char* base_dir, const char* database)
{
    char db_path[4352];
    if (ctx == NULL || base_dir == NULL || !db_valid_name(database)) {
        return -1;
    }
    if (db_path_join(db_path, sizeof(db_path), base_dir, database) != 0) {
        return -1;
    }
    if (!db_is_dir(db_path)) {
        return -1;
    }
    if (db_open_registry_add(db_path, ctx) != 0) {
        return -1;
    }
    void* lock_dir = db_lock_directory(db_path);
    if (lock_dir == NULL) {
        db_open_registry_remove(ctx);
        return -1;
    }
    memset(ctx, 0, sizeof(*ctx));
    ctx->lock_dir = lock_dir;
    snprintf(ctx->db_path, sizeof(ctx->db_path), "%s", db_path);
    snprintf(ctx->base_dir, sizeof(ctx->base_dir), "%s", base_dir);
    snprintf(ctx->database, sizeof(ctx->database), "%s", database);

    char path[4352];
    if (db_path_join(path, sizeof(path), ctx->db_path, QS_DB_SYSTEM_FILE) != 0 ||
        db_store_open(&ctx->system, path, QS_DB_CAPACITY_B1MB) != 0) {
        db_unlock_directory(ctx->lock_dir);
        ctx->lock_dir = NULL;
        db_open_registry_remove(ctx);
        return -1;
    }
    if (db_path_join(path, sizeof(path), ctx->db_path, QS_DB_SCHEMA_FILE) != 0 ||
        db_store_open(&ctx->schema, path, QS_DB_CAPACITY_B1MB) != 0) {
        db_store_close(&ctx->system);
        db_unlock_directory(ctx->lock_dir);
        ctx->lock_dir = NULL;
        db_open_registry_remove(ctx);
        return -1;
    }
    if (db_path_join(path, sizeof(path), ctx->db_path, QS_DB_INDEX_FILE) != 0 ||
        db_store_open(&ctx->index, path, QS_DB_CAPACITY_B1MB) != 0) {
        db_store_close(&ctx->system);
        db_store_close(&ctx->schema);
        db_unlock_directory(ctx->lock_dir);
        ctx->lock_dir = NULL;
        db_open_registry_remove(ctx);
        return -1;
    }
    ctx->is_open = 1;
    return 0;
}

int qs_db_close_database(QS_DB_CONTEXT* ctx)
{
    if (ctx == NULL) {
        return -1;
    }
    if (!ctx->is_open) {
        return 0;
    }
    if (ctx->open_table_count != 0) {
        return 1;
    }
    db_store_close(&ctx->system);
    db_store_close(&ctx->schema);
    db_store_close(&ctx->index);
    ctx->is_open = 0;
    db_unlock_directory(ctx->lock_dir);
    ctx->lock_dir = NULL;
    db_open_registry_remove(ctx);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Table                                                               */
/* ------------------------------------------------------------------ */

int qs_db_create_table(QS_DB_CONTEXT* ctx, const char* table, int capacity_mb)
{
    if (!ctx->is_open || !db_valid_name(table) || db_reserved_table(table) ||
        db_capacity_params(capacity_mb, &(size_t){0}, &(size_t){0}, &(size_t){0}, &(size_t){0}) != 0) {
        return -1;
    }
    char path[4352];
    char file_name[256];
    char capacity_key[QS_DB_MAX_KEY];
    char capacity_value[16];
    if (db_capacity_key(capacity_key, sizeof(capacity_key), table) != 0) {
        return -1;
    }
    snprintf(file_name, sizeof(file_name), "%s.db", table);
    if (db_path_join(path, sizeof(path), ctx->db_path, file_name) != 0) {
        return -1;
    }
    if (db_file_exists(path) || db_store_get(&ctx->system, capacity_key) != NULL) {
        return -1;
    }
    snprintf(capacity_value, sizeof(capacity_value), "%d", capacity_mb);
    if (db_store_set(&ctx->system, capacity_key, capacity_value) != 0) {
        return -1;
    }
    QS_DB_STORE store;
    memset(&store, 0, sizeof(store));
    if (db_store_create(&store, path, capacity_mb) != 0) {
        db_store_delete(&ctx->system, capacity_key);
        return -1;
    }
    db_store_close(&store);
    return 0;
}

int qs_db_drop_table(QS_DB_CONTEXT* ctx, const char* table)
{
    if (!ctx->is_open || !db_valid_name(table) || db_reserved_table(table) || ctx->open_table_count != 0) {
        return -1;
    }
    char path[4352];
    char file_name[256];
    snprintf(file_name, sizeof(file_name), "%s.db", table);
    if (db_path_join(path, sizeof(path), ctx->db_path, file_name) != 0) {
        return -1;
    }
    char capacity_key[QS_DB_MAX_KEY];
    if (db_capacity_key(capacity_key, sizeof(capacity_key), table) != 0) {
        return -1;
    }
    if (db_path_type(path) != 1) {
        return -1;
    }
    char prefix[256];
    snprintf(prefix, sizeof(prefix), "index:%s:", table);
    QS_MEMORY_POOL* scratch = NULL;
    if (0 == qs_initialize_memory_f64(&scratch, QS_DB_SCRATCH_SIZE)) {
        return -1;
    }
    int32_t keys = db_store_keys(scratch, &ctx->index);
    if (-1 == keys || unlink(path) != 0) {
        qs_free(scratch);
        return -1;
    }
    if (db_store_delete(&ctx->system, capacity_key) != 0 || db_store_delete(&ctx->schema, table) != 0) {
        qs_free(scratch);
        return -1;
    }
    int32_t len = qs_array_length(scratch, keys);
    int i;
    for (i = 0; i < len; i++) {
        QS_ARRAY_ELEMENT* elm = qs_array_get(scratch, keys, i);
        if (elm == NULL || elm->id != ELEMENT_LITERAL_STR) {
            qs_free(scratch);
            return -1;
        }
        char* key = (char*)QS_GET_POINTER(scratch, elm->memid_array_element_data);
        if (strncmp(key, prefix, strlen(prefix)) == 0 && db_store_delete(&ctx->index, key) != 0) {
            qs_free(scratch);
            return -1;
        }
    }
    qs_free(scratch);
    return 0;
}

int32_t qs_db_show_table(QS_MEMORY_POOL* _ppool, QS_DB_CONTEXT* ctx)
{
    if (!ctx->is_open) {
        return -1;
    }
    int32_t result = qs_create_array(_ppool, 16);
    if (-1 == result) {
        return -1;
    }
    DB_DIRECTORY* dir = db_directory_open(ctx->db_path);
    if (dir == NULL) {
        return -1;
    }
    const char* name;
    while ((name = db_directory_read(dir)) != NULL) {
        size_t n = strlen(name);
        if (n < 4 || strcmp(name + n - 3, ".db") != 0) {
            continue;
        }
        if (strcmp(name, QS_DB_SYSTEM_FILE) == 0 ||
            strcmp(name, QS_DB_SCHEMA_FILE) == 0 ||
            strcmp(name, QS_DB_INDEX_FILE) == 0) {
            continue;
        }
        char table_name[256];
        snprintf(table_name, sizeof(table_name), "%.*s", (int)(n - 3), name);
        qs_array_push_string(_ppool, &result, table_name);
    }
    db_directory_close(dir);
    return result;
}

static int db_open_table_internal(QS_DB_CONTEXT* ctx, const char* table, int capacity_mb, QS_DB_STORE* out_store)
{
    if (out_store == NULL) {
        return -1;
    }
    memset(out_store, 0, sizeof(*out_store));
    if (ctx == NULL || !ctx->is_open || ctx->open_table_count != 0 || !db_valid_name(table)) {
        return -1;
    }
    char path[4352];
    char file_name[256];
    snprintf(file_name, sizeof(file_name), "%s.db", table);
    if (db_path_join(path, sizeof(path), ctx->db_path, file_name) != 0) {
        return -1;
    }
    /*
     * system/schema/index は ctx が既に mmap している。
     * 同一ファイルを2つ目に mmap すると、共有ファイル内の QS_CACHE の
     * 生ポインタが上書きされ、先に開いた側が dangling になる。
     * したがって、同一ファイルは ctx 側の store を借用する（close 時に解放しない）。
     */
    if (strcmp(file_name, QS_DB_SYSTEM_FILE) == 0) {
        *out_store = ctx->system;
        out_store->borrowed = 1;
        return 0;
    }
    if (strcmp(file_name, QS_DB_SCHEMA_FILE) == 0) {
        *out_store = ctx->schema;
        out_store->borrowed = 1;
        return 0;
    }
    if (strcmp(file_name, QS_DB_INDEX_FILE) == 0) {
        *out_store = ctx->index;
        out_store->borrowed = 1;
        return 0;
    }
    char capacity_key[QS_DB_MAX_KEY];
    if (db_capacity_key(capacity_key, sizeof(capacity_key), table) != 0) {
        return -1;
    }
    char* stored_capacity = db_store_get(&ctx->system, capacity_key);
    if (stored_capacity == NULL) {
        return -1;
    }
    char* end;
    errno = 0;
    long parsed_capacity = strtol(stored_capacity, &end, 10);
    if (errno == ERANGE || end == stored_capacity || *end != '\0' || parsed_capacity <= 0 || parsed_capacity > INT_MAX ||
        db_capacity_params((int)parsed_capacity, &(size_t){0}, &(size_t){0}, &(size_t){0}, &(size_t){0}) != 0) {
        return -1;
    }
    int table_capacity = (int)parsed_capacity;
    (void)capacity_mb;
    if (db_store_open(out_store, path, table_capacity) != 0) {
        return -1;
    }
    return 0;
}

int qs_db_open_table(QS_DB_CONTEXT* ctx, const char* table, int capacity_mb, QS_DB_STORE* out_store)
{
    if (out_store == NULL) {
        return -1;
    }
    memset(out_store, 0, sizeof(*out_store));
    if (db_open_table_internal(ctx, table, capacity_mb, out_store) != 0) {
        memset(out_store, 0, sizeof(*out_store));
        return -1;
    }
    out_store->owner = ctx;
    ctx->open_table_count++;
    return 0;
}

void qs_db_close_table(QS_DB_STORE* store)
{
    QS_DB_CONTEXT* owner = store->owner;
    db_store_close(store);
    if (owner != NULL && owner->open_table_count > 0) {
        owner->open_table_count--;
    }
    store->owner = NULL;
}

/* ------------------------------------------------------------------ */
/* Schema                                                              */
/* ------------------------------------------------------------------ */

int qs_db_schema_define(QS_DB_CONTEXT* ctx, const char* table, const char* schema_json)
{
    if (!ctx->is_open || !db_valid_name(table) || schema_json == NULL) {
        return -1;
    }
    return db_store_set(&ctx->schema, table, schema_json);
}

int qs_db_schema_get(QS_DB_CONTEXT* ctx, const char* table, char* out_buf, size_t out_size)
{
    if (!ctx->is_open || !db_valid_name(table) || out_buf == NULL || out_size == 0) {
        return -1;
    }
    char* value = db_store_get(&ctx->schema, table);
    if (value == NULL) {
        return -1;
    }
    size_t len = strlen(value);
    if (len >= out_size) {
        return -1;
    }
    memcpy(out_buf, value, len + 1);
    return 0;
}

int qs_db_schema_validate(QS_MEMORY_POOL* _ppool, QS_DB_CONTEXT* ctx, const char* table, const char* record_json)
{
    if (!ctx->is_open || !db_valid_name(table) || record_json == NULL) {
        return -1;
    }
    char* schema_json = db_store_get(&ctx->schema, table);
    if (schema_json == NULL) {
        return -1;
    }
    int32_t schema = db_decode_object(_ppool, schema_json);
    if (-1 == schema) {
        return -1;
    }
    int32_t fields = qs_get_hash(_ppool, schema, "fields");
    if (-1 == fields) {
        return -1;
    }
    int32_t record = db_decode_object(_ppool, record_json);
    if (-1 == record) {
        return -1;
    }
    int32_t fcount = qs_array_length(_ppool, fields);
    int i;
    for (i = 0; i < fcount; i++) {
        QS_ARRAY_ELEMENT* felm = qs_array_get(_ppool, fields, i);
        if (felm == NULL || felm->id != ELEMENT_HASH) {
            continue;
        }
        int32_t fobj = felm->memid_array_element_data;
        char* name = qs_get_hash_string(_ppool, fobj, "name");
        char* type = qs_get_hash_string(_ppool, fobj, "type");
        if (name == NULL || type == NULL) {
            return -1;
        }
        int required = 0;
        int32_t* req = qs_get_hash_integer(_ppool, fobj, "required");
        if (req != NULL) {
            required = *req;
        }
        int has_max = 0;
        size_t max_len = 0;
        int64_t* ml = qs_get_hash_big_integer(_ppool, fobj, "maxLength");
        if (ml != NULL) {
            has_max = 1;
            max_len = (size_t)*ml;
        } else {
            /* 小さい数値は int32 として格納される */
            int32_t* mi = qs_get_hash_integer(_ppool, fobj, "maxLength");
            if (mi != NULL) {
                has_max = 1;
                max_len = (size_t)*mi;
            }
        }

        int exists = (qs_get_hash(_ppool, record, name) != -1);
        if (!exists) {
            if (required) {
                return -1;
            }
            continue;
        }
        if (strcmp(type, "int32") == 0) {
            if (qs_get_hash_integer(_ppool, record, name) == NULL) {
                return -1;
            }
        } else if (strcmp(type, "int64") == 0) {
            int32_t id = qs_get_hash_id(_ppool, record, name);
            if (id != ELEMENT_LITERAL_NUM && id != ELEMENT_LITERAL_NUM_64 && id != ELEMENT_LITERAL_NUM_U64) {
                return -1;
            }
        } else if (strcmp(type, "string") == 0) {
            char* s = qs_get_hash_string(_ppool, record, name);
            if (s == NULL) {
                return -1;
            }
            if (has_max && strlen(s) > max_len) {
                return -1;
            }
        } else {
            return -1;
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Record                                                              */
/* ------------------------------------------------------------------ */

int qs_db_record_insert(QS_DB_CONTEXT* ctx, const char* table, const char* id, const char* record_json)
{
    if (!db_valid_name(table) || id == NULL || id[0] == '\0' || record_json == NULL) {
        return -1;
    }
    QS_DB_STORE store;
    if (db_open_table_internal(ctx, table, QS_DB_DEFAULT_CAPACITY, &store) != 0) {
        return -1;
    }
    if (db_store_get(&store, id) != NULL) {
        db_store_close(&store);
        return -1; /* 既に存在 */
    }
    int rc = db_store_set(&store, id, record_json);
    db_store_close(&store);
    return rc;
}

int qs_db_record_update(QS_DB_CONTEXT* ctx, const char* table, const char* id, const char* record_json)
{
    if (!db_valid_name(table) || id == NULL || id[0] == '\0' || record_json == NULL) {
        return -1;
    }
    QS_DB_STORE store;
    if (db_open_table_internal(ctx, table, QS_DB_DEFAULT_CAPACITY, &store) != 0) {
        return -1;
    }
    if (db_store_get(&store, id) == NULL) {
        db_store_close(&store);
        return -1; /* 未存在 */
    }
    int rc = db_store_set(&store, id, record_json);
    db_store_close(&store);
    return rc;
}

int qs_db_record_delete(QS_DB_CONTEXT* ctx, const char* table, const char* id)
{
    if (!db_valid_name(table) || id == NULL || id[0] == '\0') {
        return -1;
    }
    QS_DB_STORE store;
    if (db_open_table_internal(ctx, table, QS_DB_DEFAULT_CAPACITY, &store) != 0) {
        return -1;
    }
    int rc = db_store_delete(&store, id);
    db_store_close(&store);
    return rc;
}

int qs_db_record_get(QS_DB_CONTEXT* ctx, const char* table, const char* id, char* out_buf, size_t out_size)
{
    if (!db_valid_name(table) || id == NULL || id[0] == '\0' || out_buf == NULL || out_size == 0) {
        return -1;
    }
    QS_DB_STORE store;
    if (db_open_table_internal(ctx, table, QS_DB_DEFAULT_CAPACITY, &store) != 0) {
        return -1;
    }
    char* value = db_store_get(&store, id);
    int rc = -1;
    if (value != NULL) {
        size_t len = strlen(value);
        if (len < out_size) {
            memcpy(out_buf, value, len + 1);
            rc = 0;
        }
    }
    db_store_close(&store);
    return rc;
}

char* qs_db_record_list(QS_MEMORY_POOL* _ppool, QS_DB_CONTEXT* ctx, const char* table, const char* filter_json, int scan_mode)
{
    if (!ctx->is_open) {
        return NULL;
    }
    QS_MEMORY_POOL* work_pool = NULL;
    if (0 == qs_initialize_memory_f64(&work_pool, QS_DB_SCRATCH_SIZE * 8)) {
        return NULL;
    }
    QS_DB_STORE store;
    if (db_open_table_internal(ctx, table, QS_DB_DEFAULT_CAPACITY, &store) != 0) {
        qs_free(work_pool);
        return NULL;
    }

    /* フィルタ条件をデコード（NULL/{} は全件） */
    int32_t filter = -1;
    if (filter_json != NULL && filter_json[0] != '\0') {
        filter = db_decode_object(work_pool, filter_json);
        if (-1 == filter) {
            db_store_close(&store);
            qs_free(work_pool);
            return NULL;
        }
    }

    int32_t result = qs_create_array(work_pool, 128);
    if (-1 == result) {
        db_store_close(&store);
        qs_free(work_pool);
        return NULL;
    }

    int32_t fkeys = -1;
    int32_t fcount = 0;
    if (filter != -1) {
        QS_HASH_FOREACH filter_iterator;
        qs_init_hash_foreach(work_pool, filter, &filter_iterator);
        if (qs_hash_foreach(work_pool, &filter_iterator) != NULL) {
            fkeys = qs_get_hash_keys(work_pool, work_pool, filter, 1);
            if (-1 == fkeys) {
                db_store_close(&store);
                qs_free(work_pool);
                return NULL;
            }
            fcount = qs_array_length(work_pool, fkeys);
        }
    }

    if (scan_mode == QS_DB_SCAN_INDEX && fcount > 0) {
        /* index 検索: 各フィルタの index 配列を取得し、最小を候補起点にする */
        int32_t idx_arrays[64];
        int32_t idx_count = 0;
        int32_t min_idx = -1;
        int32_t min_len = 0;
        int i;
        for (i = 0; i < fcount; i++) {
            QS_ARRAY_ELEMENT* felm = qs_array_get(work_pool, fkeys, i);
            char index_key[QS_DB_MAX_KEY];
            if (felm == NULL || felm->id != ELEMENT_LITERAL_STR) {
                db_store_close(&store);
                qs_free(work_pool);
                return NULL;
            }
            char* field = (char*)QS_GET_POINTER(work_pool, felm->memid_array_element_data);
            char* value = qs_get_hash_string(work_pool, filter, field);
            if (value == NULL || db_index_key(index_key, sizeof(index_key), table, field, value) != 0) {
                db_store_close(&store);
                qs_free(work_pool);
                return NULL;
            }
        }
        for (i = 0; i < fcount && idx_count < 64; i++) {
            QS_ARRAY_ELEMENT* felm = qs_array_get(work_pool, fkeys, i);
            if (felm == NULL || felm->id != ELEMENT_LITERAL_STR) {
                continue;
            }
            char* field = (char*)QS_GET_POINTER(work_pool, felm->memid_array_element_data);
            char* value = qs_get_hash_string(work_pool, filter, field);
            if (value == NULL) {
                continue;
            }
            char index_key[QS_DB_MAX_KEY];
            if (db_index_key(index_key, sizeof(index_key), table, field, value) != 0) {
                continue;
            }
            char* ids_json = db_store_get(&ctx->index, index_key);
            if (ids_json == NULL) {
                /* 該当 index なし => 空結果 */
                db_store_close(&store);
                qs_free(work_pool);
                return db_empty_json_array(_ppool);
            }
            int32_t ids = db_decode_array(work_pool, ids_json);
            if (-1 == ids) {
                db_store_close(&store);
                qs_free(work_pool);
                return NULL;
            }
            idx_arrays[idx_count] = ids;
            int32_t l = qs_array_length(work_pool, ids);
            if (min_idx == -1 || l < min_len) {
                min_idx = idx_count;
                min_len = l;
            }
            idx_count++;
        }
        if (min_idx < 0) {
            db_store_close(&store);
            qs_free(work_pool);
            return NULL;
        }
        int32_t cand_len = qs_array_length(work_pool, idx_arrays[min_idx]);
        int j;
        for (j = 0; j < cand_len; j++) {
            QS_ARRAY_ELEMENT* celm = qs_array_get(work_pool, idx_arrays[min_idx], j);
            if (celm == NULL || celm->id != ELEMENT_LITERAL_STR) {
                continue;
            }
            char* id = (char*)QS_GET_POINTER(work_pool, celm->memid_array_element_data);
            char* json = db_store_get(&store, id);
            if (json == NULL) {
                continue;
            }
            int32_t rec = db_decode_object(work_pool, json);
            if (-1 == rec) {
                db_store_close(&store);
                qs_free(work_pool);
                return NULL;
            }
            int match = 1;
            int k;
            for (k = 0; k < fcount && match; k++) {
                QS_ARRAY_ELEMENT* felm = qs_array_get(work_pool, fkeys, k);
                if (felm == NULL || felm->id != ELEMENT_LITERAL_STR) {
                    continue;
                }
                char* field = (char*)QS_GET_POINTER(work_pool, felm->memid_array_element_data);
                char* value = qs_get_hash_string(work_pool, filter, field);
                if (value == NULL) {
                    match = 0;
                    break;
                }
                match = db_record_match(work_pool, rec, field, value);
            }
            if (match) {
                if (-1 == qs_array_push(work_pool, &result, ELEMENT_HASH, rec)) {
                    db_store_close(&store);
                    qs_free(work_pool);
                    return NULL;
                }
            }
        }
    } else {
        /* フルスキャン（フィルタ有無を問わず全レコードを走査して照合） */
        int32_t keys = db_store_keys(work_pool, &store);
        if (-1 == keys) {
            db_store_close(&store);
            qs_free(work_pool);
            return NULL;
        }
        int32_t len = qs_array_length(work_pool, keys);
        int i;
        for (i = 0; i < len; i++) {
            QS_ARRAY_ELEMENT* elm = qs_array_get(work_pool, keys, i);
            if (elm == NULL || elm->id != ELEMENT_LITERAL_STR) {
                continue;
            }
            char* id = (char*)QS_GET_POINTER(work_pool, elm->memid_array_element_data);
            char* json = db_store_get(&store, id);
            if (json == NULL) {
                continue;
            }
            int32_t rec = db_decode_object(work_pool, json);
            if (-1 == rec) {
                db_store_close(&store);
                qs_free(work_pool);
                return NULL;
            }
            if (fcount > 0) {
                int match = 1;
                int k;
                for (k = 0; k < fcount && match; k++) {
                    QS_ARRAY_ELEMENT* felm = qs_array_get(work_pool, fkeys, k);
                    if (felm == NULL || felm->id != ELEMENT_LITERAL_STR) {
                        continue;
                    }
                    char* field = (char*)QS_GET_POINTER(work_pool, felm->memid_array_element_data);
                    char* value = qs_get_hash_string(work_pool, filter, field);
                    if (value == NULL) {
                        match = 0;
                        break;
                    }
                    match = db_record_match(work_pool, rec, field, value);
                }
                if (!match) {
                    continue;
                }
            }
            if (-1 == qs_array_push(work_pool, &result, ELEMENT_HASH, rec)) {
                db_store_close(&store);
                qs_free(work_pool);
                return NULL;
            }
        }
    }

    db_store_close(&store);

    if (qs_array_length(work_pool, result) == 0) {
        qs_free(work_pool);
        return db_empty_json_array(_ppool);
    }

    int32_t memid_json = qs_json_encode_array(work_pool, result, 64 * 1024);
    if (-1 == memid_json) {
        qs_free(work_pool);
        return NULL;
    }
    char* encoded = (char*)QS_GET_POINTER(work_pool, memid_json);
    size_t encoded_size = strlen(encoded) + 1;
    int32_t output = qs_create_memory_block(_ppool, encoded_size);
    if (-1 == output) {
        qs_free(work_pool);
        return NULL;
    }
    char* output_json = (char*)QS_GET_POINTER(_ppool, output);
    memcpy(output_json, encoded, encoded_size);
    qs_free(work_pool);
    return output_json;
}

/* ------------------------------------------------------------------ */
/* Index                                                               */
/* ------------------------------------------------------------------ */

int qs_db_index_add(QS_DB_CONTEXT* ctx, const char* table, const char* field, const char* value, const char* record_id)
{
    if (!ctx->is_open || !db_valid_name(table) || !db_valid_key(field) ||
        !db_valid_key(value) || !db_valid_key(record_id)) {
        return -1;
    }
    char index_key[QS_DB_MAX_KEY];
    if (db_index_key(index_key, sizeof(index_key), table, field, value) != 0) {
        return -1;
    }
    return db_index_add_id(ctx, index_key, record_id);
}

int qs_db_index_remove(QS_DB_CONTEXT* ctx, const char* table, const char* field, const char* value, const char* record_id)
{
    if (!ctx->is_open || !db_valid_name(table) || !db_valid_key(field) ||
        !db_valid_key(value) || !db_valid_key(record_id)) {
        return -1;
    }
    char index_key[QS_DB_MAX_KEY];
    if (db_index_key(index_key, sizeof(index_key), table, field, value) != 0) {
        return -1;
    }
    return db_index_remove_id(ctx, index_key, record_id);
}

int qs_db_index_rebuild(QS_MEMORY_POOL* _ppool, QS_DB_CONTEXT* ctx, const char* table, const char* indexed_fields_json)
{
    if (!ctx->is_open || !db_valid_name(table) || indexed_fields_json == NULL) {
        return -1;
    }
    int32_t fields = db_decode_array(_ppool, indexed_fields_json);
    if (-1 == fields) {
        return -1;
    }
    QS_DB_STORE store;
    if (db_open_table_internal(ctx, table, QS_DB_DEFAULT_CAPACITY, &store) != 0) {
        return -1;
    }
    /* 旧 index（index:{table}:*）を削除 */
    char prefix[256];
    snprintf(prefix, sizeof(prefix), "index:%s:", table);
    int32_t old_keys = db_store_keys(_ppool, &ctx->index);
    if (-1 == old_keys) {
        db_store_close(&store);
        return -1;
    }
    {
        int32_t len = qs_array_length(_ppool, old_keys);
        int i;
        for (i = 0; i < len; i++) {
            QS_ARRAY_ELEMENT* elm = qs_array_get(_ppool, old_keys, i);
            if (elm == NULL || elm->id != ELEMENT_LITERAL_STR) {
                continue;
            }
            char* key = (char*)QS_GET_POINTER(_ppool, elm->memid_array_element_data);
            if (strncmp(key, prefix, strlen(prefix)) == 0) {
                if (db_store_delete(&ctx->index, key) != 0) {
                    db_store_close(&store);
                    return -1;
                }
            }
        }
    }

    /* 全レコードを走査して index を再構築 */
    int32_t keys = db_store_keys(_ppool, &store);
    if (-1 == keys) {
        db_store_close(&store);
        return -1;
    }
    int32_t fcount = qs_array_length(_ppool, fields);
    int32_t len = qs_array_length(_ppool, keys);
    int i;
    for (i = 0; i < len; i++) {
        QS_ARRAY_ELEMENT* elm = qs_array_get(_ppool, keys, i);
        if (elm == NULL || elm->id != ELEMENT_LITERAL_STR) {
            continue;
        }
        char* id = (char*)QS_GET_POINTER(_ppool, elm->memid_array_element_data);
        char* json = db_store_get(&store, id);
        if (json == NULL) {
            db_store_close(&store);
            return -1;
        }
        int32_t rec = db_decode_object(_ppool, json);
        if (-1 == rec) {
            db_store_close(&store);
            return -1;
        }
        int j;
        for (j = 0; j < fcount; j++) {
            QS_ARRAY_ELEMENT* felm = qs_array_get(_ppool, fields, j);
            if (felm == NULL || felm->id != ELEMENT_LITERAL_STR) {
                continue;
            }
            char* field = (char*)QS_GET_POINTER(_ppool, felm->memid_array_element_data);
            int32_t fid = qs_get_hash_id(_ppool, rec, field);
            if (fid == ELEMENT_LITERAL_STR) {
                char* value = qs_get_hash_string(_ppool, rec, field);
                if (value != NULL) {
                    char index_key[QS_DB_MAX_KEY];
                    if (db_index_key(index_key, sizeof(index_key), table, field, value) != 0 ||
                        db_index_add_id(ctx, index_key, id) != 0) {
                        db_store_close(&store);
                        return -1;
                    }
                }
            } else if (fid == ELEMENT_ARRAY) {
                int32_t arr = qs_get_hash(_ppool, rec, field);
                int32_t alen = qs_array_length(_ppool, arr);
                int k;
                for (k = 0; k < alen; k++) {
                    QS_ARRAY_ELEMENT* aelm = qs_array_get(_ppool, arr, k);
                    if (aelm == NULL || aelm->id != ELEMENT_LITERAL_STR) {
                        continue;
                    }
                    char* value = (char*)QS_GET_POINTER(_ppool, aelm->memid_array_element_data);
                    char index_key[QS_DB_MAX_KEY];
                    if (db_index_key(index_key, sizeof(index_key), table, field, value) != 0 ||
                        db_index_add_id(ctx, index_key, id) != 0) {
                        db_store_close(&store);
                        return -1;
                    }
                }
            }
        }
    }
    db_store_close(&store);
    return 0;
}

/* ------------------------------------------------------------------ */
/* System（id 採番）                                                   */
/* ------------------------------------------------------------------ */

int qs_db_next_id(QS_DB_CONTEXT* ctx, const char* table, const char* prefix, char* out_id, size_t out_size)
{
    if (ctx == NULL || !ctx->is_open || !db_valid_name(table) || prefix == NULL ||
        prefix[0] == '\0' || out_id == NULL || out_size == 0) {
        return -1;
    }
    char counter_key[QS_DB_MAX_KEY];
    int n = snprintf(counter_key, sizeof(counter_key), "counter:%s", table);
    if (n <= 0 || (size_t)n >= sizeof(counter_key)) {
        return -1;
    }
    char* cur = db_store_get(&ctx->system, counter_key);
    int64_t count = 0;
    if (cur != NULL) {
        char* end = NULL;
        errno = 0;
        long long parsed = strtoll(cur, &end, 10);
        if (errno != 0 || end == cur || *end != '\0' || parsed < 0 || parsed == INT64_MAX) {
            return -1;
        }
        count = (int64_t)parsed;
    }
    count++;
    char next_id[QS_DB_MAX_KEY];
    n = snprintf(next_id, sizeof(next_id), "%s%lld", prefix, (long long)count);
    if (n <= 0 || (size_t)n >= sizeof(next_id) || (size_t)n >= out_size) {
        return -1;
    }
    char next_count[32];
    int count_len = snprintf(next_count, sizeof(next_count), "%lld", (long long)count);
    if (count_len <= 0 || (size_t)count_len >= sizeof(next_count) ||
        db_store_set(&ctx->system, counter_key, next_count) != 0) {
        return -1;
    }
    memcpy(out_id, next_id, (size_t)n + 1);
    return 0;
}
