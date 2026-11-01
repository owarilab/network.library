/*
 * Copyright (c) Katsuya Owari
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>

#include "qs_api.h"

#define EXIT_OK 0
#define EXIT_USAGE 2
#define EXIT_IO 3
#define EXIT_KVS 4
#define EXIT_NOT_FOUND 5

#define DEFAULT_BASE_DIR "./issue_data"
#define INDEX_JSON_BUFFER_SIZE (64 * 1024)
#define MAX_SCHEMA_FIELDS 128
#define MAX_ISSUE_STATUS_VALUES 64

typedef enum SCHEMA_FIELD_TYPE {
    SCHEMA_FIELD_TYPE_INT32 = 1,
    SCHEMA_FIELD_TYPE_INT64 = 2,
    SCHEMA_FIELD_TYPE_STRING = 3
} SCHEMA_FIELD_TYPE;

typedef struct STORE_CONFIG {
    const char* name;
    const char* file_name;
    int (*create_fn)(QS_KVS_CONTEXT* kvs_context, const char* file_path);
} STORE_CONFIG;

typedef struct APP_STORES {
    QS_KVS_CONTEXT user;
    QS_KVS_CONTEXT label;
    QS_KVS_CONTEXT issue;
    QS_KVS_CONTEXT index;
    QS_KVS_CONTEXT system;
    int issue_status_count;
    char issue_statuses[MAX_ISSUE_STATUS_VALUES][64];
    char base_dir[4096];
} APP_STORES;

typedef struct SCHEMA_FIELD_DEF {
    char name[128];
    SCHEMA_FIELD_TYPE type;
    int required;
    int unsettable;
    int has_default;
    int has_max_length;
    int32_t default_i32;
    int64_t default_i64;
    char default_str[1024];
    size_t max_length;
} SCHEMA_FIELD_DEF;

typedef struct SCHEMA_DEF {
    SCHEMA_FIELD_DEF fields[MAX_SCHEMA_FIELDS];
    int field_count;
} SCHEMA_DEF;

static const STORE_CONFIG STORE_USER = {"user", "user.db", api_qs_kvs_create_b1mb_persistence};
static const STORE_CONFIG STORE_LABEL = {"label", "label.db", api_qs_kvs_create_b1mb_persistence};
static const STORE_CONFIG STORE_ISSUE = {"issue", "issue.db", api_qs_kvs_create_b8mb_persistence};
static const STORE_CONFIG STORE_INDEX = {"index", "issue_index.db", api_qs_kvs_create_b1mb_persistence};
static const STORE_CONFIG STORE_SYSTEM = {"system", "system.db", api_qs_kvs_create_b1mb_persistence};

static int compact_store(const STORE_CONFIG* cfg, const char* base_dir);
static SCHEMA_FIELD_DEF* schema_find_field(SCHEMA_DEF* schema, const char* name);
static int load_schema(const char* base_dir, const char* schema_file_name, SCHEMA_DEF* out_schema);
static int is_allowed_extra_field(const char* entity_name, const char* key);
static int apply_schema_on_write(const char* entity_name, const char* base_dir, const char* schema_file_name, const char* src_json, char** out_json);
static int validate_and_fill_schema_object(const char* entity_name, SCHEMA_DEF* schema, QS_JSON_ELEMENT_OBJECT* obj, QS_MEMORY_CONTEXT* context);
static int load_system_settings(const char* base_dir, APP_STORES* stores);

static void print_usage(const char* program_name)
{
    printf("Usage:\n");
    printf("  %s init [--base-dir <dir>] [--force]\n", program_name);
    printf("  %s user  <create|get|update|delete|unset> --id <id> [--json <json>] [--field <name>] [--base-dir <dir>]\n", program_name);
    printf("  %s user  list [--base-dir <dir>]\n", program_name);
    printf("  %s label <create|get|update|delete|unset> --id <id> [--json <json>] [--field <name>] [--base-dir <dir>]\n", program_name);
    printf("  %s label list [--base-dir <dir>]\n", program_name);
    printf("  %s issue <create|get|update|delete|unset> --id <id> [--json <json>] [--field <name>] [--base-dir <dir>]\n", program_name);
    printf("  %s issue list [--status <status> | --assignee <userId> | --label <label>] [--base-dir <dir>]\n", program_name);
    printf("  %s schema <list|get> [--entity user|label|issue] [--base-dir <dir>]\n", program_name);
    printf("  %s compact [--store user|label|issue|index|system|all] [--base-dir <dir>]\n", program_name);
    printf("\n");
    printf("Examples:\n");
    printf("  %s init --base-dir ./issue_data\n", program_name);
    printf("  %s issue create --json '{\"title\":\"t\",\"status\":\"open\",\"assigneeIds\":[\"u1\"],\"labelIds\":[\"bug\"]}'\n", program_name);
    printf("  %s issue list --status open\n", program_name);
    printf("  %s schema get --entity issue\n", program_name);
    printf("  %s issue unset --id iss1 --field description\n", program_name);
}

static const char* get_option_value(int argc, char* argv[], const char* key)
{
    int i;
    for (i = 0; i < argc; i++) {
        if (strcmp(argv[i], key) == 0) {
            if (i + 1 >= argc) {
                return NULL;
            }
            return argv[i + 1];
        }
    }
    return NULL;
}

static int has_option(int argc, char* argv[], const char* key)
{
    int i;
    for (i = 0; i < argc; i++) {
        if (strcmp(argv[i], key) == 0) {
            return 1;
        }
    }
    return 0;
}

static int ensure_directory(const char* dir_path)
{
    struct stat st;
    if (stat(dir_path, &st) == 0) {
        if (!S_ISDIR(st.st_mode)) {
            fprintf(stderr, "error: path exists but is not directory: %s\n", dir_path);
            return -1;
        }
        return 0;
    }
    if (mkdir(dir_path, 0755) != 0) {
        fprintf(stderr, "error: mkdir failed: %s (%s)\n", dir_path, strerror(errno));
        return -1;
    }
    return 0;
}

static int build_store_path(char* out_path, size_t out_size, const char* base_dir, const char* file_name)
{
    int written = snprintf(out_path, out_size, "%s/%s", base_dir, file_name);
    if (written <= 0 || (size_t)written >= out_size) {
        return -1;
    }
    return 0;
}

static int file_exists(const char* file_path)
{
    return access(file_path, F_OK) == 0;
}

static int create_store_file(const STORE_CONFIG* config, const char* path, int force)
{
    if (file_exists(path)) {
        if (!force) {
            fprintf(stderr, "error: store already exists: %s\n", path);
            return -1;
        }
        if (unlink(path) != 0) {
            fprintf(stderr, "error: failed to remove existing store: %s (%s)\n", path, strerror(errno));
            return -1;
        }
    }

    QS_KVS_CONTEXT kvs;
    if (-1 == config->create_fn(&kvs, path)) {
        fprintf(stderr, "error: failed to create store(%s): %s\n", config->name, path);
        return -1;
    }
    api_qs_persistence_kvs_memory_free(&kvs);
    return 0;
}

static int read_text_file(const char* path, char** out_text)
{
    FILE* fp = fopen(path, "rb");
    if (fp == NULL) {
        return -1;
    }
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return -1;
    }
    long sz = ftell(fp);
    if (sz < 0) {
        fclose(fp);
        return -1;
    }
    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return -1;
    }
    char* buf = (char*)malloc((size_t)sz + 1);
    if (buf == NULL) {
        fclose(fp);
        return -1;
    }
    size_t read_size = fread(buf, 1, (size_t)sz, fp);
    fclose(fp);
    if (read_size != (size_t)sz) {
        free(buf);
        return -1;
    }
    buf[sz] = '\0';
    *out_text = buf;
    return 0;
}

static int write_text_file(const char* path, const char* text)
{
    FILE* fp = fopen(path, "wb");
    if (fp == NULL) {
        return -1;
    }
    size_t len = strlen(text);
    size_t written = fwrite(text, 1, len, fp);
    fclose(fp);
    return (written == len) ? 0 : -1;
}

static int write_schema_file(const char* base_dir, const char* file_name, const char* content, int force)
{
    char path[4096];
    if (build_store_path(path, sizeof(path), base_dir, file_name) != 0) {
        return -1;
    }
    if (!force && file_exists(path)) {
        return 0;
    }
    if (write_text_file(path, content) != 0) {
        fprintf(stderr, "error: failed to write schema file: %s\n", path);
        return -1;
    }
    return 0;
}

static int write_system_settings_file(const char* base_dir, int force)
{
    static const char* SYSTEM_SETTINGS =
        "{\n"
        "  \"issue\": {\n"
        "    \"statuses\": [\"open\", \"in_progress\", \"closed\", \"review\"]\n"
        "  }\n"
        "}\n";
    char path[4096];
    if (build_store_path(path, sizeof(path), base_dir, "system_settings.json") != 0) {
        return -1;
    }
    if (!force && file_exists(path)) {
        return 0;
    }
    if (write_text_file(path, SYSTEM_SETTINGS) != 0) {
        fprintf(stderr, "error: failed to write system settings file: %s\n", path);
        return -1;
    }
    return 0;
}

static int init_schema_files(const char* base_dir, int force)
{
    static const char* USER_SCHEMA =
        "{\n"
        "  \"fields\": [\n"
        "    {\"name\":\"id\",\"type\":\"string\",\"maxLength\":64,\"required\":0,\"unsettable\":0},\n"
        "    {\"name\":\"name\",\"type\":\"string\",\"maxLength\":128,\"default\":\"\",\"required\":1,\"unsettable\":1}\n"
        "  ]\n"
        "}\n";
    static const char* LABEL_SCHEMA =
        "{\n"
        "  \"fields\": [\n"
        "    {\"name\":\"id\",\"type\":\"string\",\"maxLength\":64,\"required\":0,\"unsettable\":0},\n"
        "    {\"name\":\"name\",\"type\":\"string\",\"maxLength\":128,\"default\":\"\",\"required\":1,\"unsettable\":1}\n"
        "  ]\n"
        "}\n";
    static const char* ISSUE_SCHEMA =
        "{\n"
        "  \"fields\": [\n"
        "    {\"name\":\"id\",\"type\":\"string\",\"maxLength\":64,\"required\":0,\"unsettable\":0},\n"
        "    {\"name\":\"title\",\"type\":\"string\",\"maxLength\":256,\"default\":\"\",\"required\":1,\"unsettable\":1},\n"
        "    {\"name\":\"description\",\"type\":\"string\",\"maxLength\":4096,\"default\":\"\",\"required\":0,\"unsettable\":1},\n"
        "    {\"name\":\"status\",\"type\":\"string\",\"maxLength\":64,\"default\":\"open\",\"required\":1,\"unsettable\":1},\n"
        "    {\"name\":\"priority\",\"type\":\"int32\",\"default\":0,\"required\":0,\"unsettable\":1},\n"
        "    {\"name\":\"updatedAt\",\"type\":\"int64\",\"default\":0,\"required\":0,\"unsettable\":1}\n"
        "  ]\n"
        "}\n";

    if (write_schema_file(base_dir, "schema_user.json", USER_SCHEMA, force) != 0) {
        return -1;
    }
    if (write_schema_file(base_dir, "schema_label.json", LABEL_SCHEMA, force) != 0) {
        return -1;
    }
    if (write_schema_file(base_dir, "schema_issue.json", ISSUE_SCHEMA, force) != 0) {
        return -1;
    }
    return 0;
}

static const char* schema_file_name_from_entity(const char* entity_name)
{
    if (strcmp(entity_name, "user") == 0) {
        return "schema_user.json";
    }
    if (strcmp(entity_name, "label") == 0) {
        return "schema_label.json";
    }
    if (strcmp(entity_name, "issue") == 0) {
        return "schema_issue.json";
    }
    return NULL;
}

static int open_store_checked(const STORE_CONFIG* config, const char* base_dir, QS_KVS_CONTEXT* out)
{
    char path[4096];
    if (build_store_path(path, sizeof(path), base_dir, config->file_name) != 0) {
        return -1;
    }
    if (!file_exists(path)) {
        fprintf(stderr, "error: store file not found: %s\n", path);
        return -1;
    }
    if (-1 == config->create_fn(out, path)) {
        fprintf(stderr, "error: failed to open store(%s): %s\n", config->name, path);
        return -1;
    }
    return 0;
}

static int open_all_stores(const char* base_dir, APP_STORES* stores)
{
    snprintf(stores->base_dir, sizeof(stores->base_dir), "%s", base_dir);
    if (open_store_checked(&STORE_USER, base_dir, &stores->user) != 0) {
        return -1;
    }
    if (open_store_checked(&STORE_LABEL, base_dir, &stores->label) != 0) {
        api_qs_persistence_kvs_memory_free(&stores->user);
        return -1;
    }
    if (open_store_checked(&STORE_ISSUE, base_dir, &stores->issue) != 0) {
        api_qs_persistence_kvs_memory_free(&stores->user);
        api_qs_persistence_kvs_memory_free(&stores->label);
        return -1;
    }
    if (open_store_checked(&STORE_INDEX, base_dir, &stores->index) != 0) {
        api_qs_persistence_kvs_memory_free(&stores->user);
        api_qs_persistence_kvs_memory_free(&stores->label);
        api_qs_persistence_kvs_memory_free(&stores->issue);
        return -1;
    }
    if (open_store_checked(&STORE_SYSTEM, base_dir, &stores->system) != 0) {
        api_qs_persistence_kvs_memory_free(&stores->user);
        api_qs_persistence_kvs_memory_free(&stores->label);
        api_qs_persistence_kvs_memory_free(&stores->issue);
        api_qs_persistence_kvs_memory_free(&stores->index);
        return -1;
    }
    return 0;
}

static void close_all_stores(APP_STORES* stores)
{
    api_qs_persistence_kvs_memory_free(&stores->user);
    api_qs_persistence_kvs_memory_free(&stores->label);
    api_qs_persistence_kvs_memory_free(&stores->issue);
    api_qs_persistence_kvs_memory_free(&stores->index);
    api_qs_persistence_kvs_memory_free(&stores->system);
}

static int create_temp_memory(QS_MEMORY_CONTEXT* context, size_t size)
{
    if (-1 == api_qs_memory_alloc(context, size)) {
        fprintf(stderr, "error: failed to allocate temporary memory\n");
        return -1;
    }
    return 0;
}

static int build_entity_key(char* out_key, size_t out_size, const char* entity_name, const char* id)
{
    int written = snprintf(out_key, out_size, "%s:%s", entity_name, id);
    if (written <= 0 || (size_t)written >= out_size) {
        return -1;
    }
    return 0;
}

static int reopen_store(APP_STORES* stores, QS_KVS_CONTEXT* store, const STORE_CONFIG* cfg)
{
    api_qs_persistence_kvs_memory_free(store);
    if (open_store_checked(cfg, stores->base_dir, store) != 0) {
        return -1;
    }
    return 0;
}

static int kvs_set_with_compact_retry(APP_STORES* stores, QS_KVS_CONTEXT* store, const STORE_CONFIG* cfg, const char* key, const char* value)
{
    if (-1 != api_qs_kvs_set(store, key, value, 0)) {
        return 0;
    }
    if (compact_store(cfg, stores->base_dir) != 0) {
        return -1;
    }
    if (reopen_store(stores, store, cfg) != 0) {
        return -1;
    }
    return (api_qs_kvs_set(store, key, value, 0) == -1) ? -1 : 0;
}

static int json_with_forced_id(const char* src_json, const char* id, int strict_id_match, char** out_json)
{
    QS_MEMORY_CONTEXT temp;
    if (-1 == api_qs_memory_alloc(&temp, 1024 * 1024)) {
        return -1;
    }
    QS_JSON_ELEMENT_OBJECT obj;
    if (-1 == api_qs_json_decode_object(&temp, &obj, src_json)) {
        api_qs_memory_free(&temp);
        return -1;
    }
    if (strict_id_match) {
        char* existing_id = api_qs_object_get_string(&obj, "id");
        if (existing_id != NULL && strcmp(existing_id, id) != 0) {
            api_qs_memory_free(&temp);
            return -2;
        }
    }
    if (-1 == api_qs_object_push_string(&obj, "id", id)) {
        api_qs_memory_free(&temp);
        return -1;
    }
    size_t src_len = strlen(src_json);
    size_t buffer_size = (src_len + 4096 > 65536) ? (src_len + 4096) : 65536;
    char* encoded = api_qs_json_encode_object(&obj, buffer_size);
    if (encoded == NULL) {
        api_qs_memory_free(&temp);
        return -1;
    }
    *out_json = strdup(encoded);
    api_qs_memory_free(&temp);
    return (*out_json == NULL) ? -1 : 0;
}

static int copy_object_field(QS_JSON_ELEMENT_OBJECT* dst, QS_JSON_ELEMENT_OBJECT* src, const char* key)
{
    int32_t* i32 = api_qs_object_get_integer(src, key);
    if (i32 != NULL) {
        return api_qs_object_push_integer(dst, key, *i32);
    }
    int64_t* i64 = api_qs_object_get_big_integer(src, key);
    if (i64 != NULL) {
        return api_qs_object_push_big_integer(dst, key, *i64);
    }
    uint64_t* u64 = api_qs_object_get_unsigned_big_integer(src, key);
    if (u64 != NULL) {
        return api_qs_object_push_unsigned_big_integer(dst, key, *u64);
    }
    char* s = api_qs_object_get_string(src, key);
    if (s != NULL) {
        return api_qs_object_push_string(dst, key, s);
    }
    QS_JSON_ELEMENT_ARRAY arr;
    if (api_qs_object_get_array(src, key, &arr) == 0) {
        return api_qs_object_push_array(dst, key, &arr);
    }
    QS_JSON_ELEMENT_OBJECT obj;
    if (api_qs_object_get_object(src, key, &obj) == 0) {
        return api_qs_object_push_object(dst, key, &obj);
    }
    fprintf(stderr, "error: unsupported json field type: %s\n", key);
    return -1;
}

static int copy_missing_object_fields(QS_MEMORY_CONTEXT* context, QS_JSON_ELEMENT_OBJECT* dst, QS_JSON_ELEMENT_OBJECT* src)
{
    QS_JSON_ELEMENT_ARRAY keys;
    if (-1 == api_qs_array_create(context, &keys)) {
        return -1;
    }
    if (-1 == api_qs_object_get_keys(src, &keys)) {
        return -1;
    }
    int32_t len = api_qs_array_get_length(&keys);
    int32_t i;
    for (i = 0; i < len; i++) {
        char* key = api_qs_array_get_string(&keys, i);
        if (key == NULL) {
            continue;
        }
        if (api_qs_object_exist(dst, key)) {
            continue;
        }
        if (-1 == copy_object_field(dst, src, key)) {
            return -1;
        }
    }
    return 0;
}

static int copy_object_fields_except(QS_MEMORY_CONTEXT* context, QS_JSON_ELEMENT_OBJECT* dst, QS_JSON_ELEMENT_OBJECT* src, const char* skip_key)
{
    QS_JSON_ELEMENT_ARRAY keys;
    if (-1 == api_qs_array_create(context, &keys)) {
        return -1;
    }
    if (-1 == api_qs_object_get_keys(src, &keys)) {
        return -1;
    }
    int32_t len = api_qs_array_get_length(&keys);
    int32_t i;
    for (i = 0; i < len; i++) {
        char* key = api_qs_array_get_string(&keys, i);
        if (key == NULL) {
            continue;
        }
        if (strcmp(key, skip_key) == 0) {
            continue;
        }
        if (-1 == copy_object_field(dst, src, key)) {
            return -1;
        }
    }
    return 0;
}

static int build_merged_update_json_with_schema(const char* entity_name, const char* base_dir, const char* schema_file_name, const char* old_json, const char* patch_json, char** out_json)
{
    SCHEMA_DEF schema;
    if (load_schema(base_dir, schema_file_name, &schema) != 0) {
        return -1;
    }

    QS_MEMORY_CONTEXT context;
    if (create_temp_memory(&context, 4 * 1024 * 1024) != 0) {
        return -1;
    }
    QS_JSON_ELEMENT_OBJECT old_obj;
    QS_JSON_ELEMENT_OBJECT patch_obj;
    if (-1 == api_qs_json_decode_object(&context, &old_obj, old_json)) {
        api_qs_memory_free(&context);
        return -1;
    }
    if (-1 == api_qs_json_decode_object(&context, &patch_obj, patch_json)) {
        api_qs_memory_free(&context);
        return -1;
    }
    if (copy_missing_object_fields(&context, &patch_obj, &old_obj) != 0) {
        api_qs_memory_free(&context);
        return -1;
    }
    if (validate_and_fill_schema_object(entity_name, &schema, &patch_obj, &context) != 0) {
        api_qs_memory_free(&context);
        return -1;
    }

    size_t old_len = strlen(old_json);
    size_t patch_len = strlen(patch_json);
    char* encoded = api_qs_json_encode_object(&patch_obj, old_len + patch_len + 8192);
    if (encoded == NULL) {
        api_qs_memory_free(&context);
        return -1;
    }
    *out_json = strdup(encoded);
    api_qs_memory_free(&context);
    return (*out_json == NULL) ? -1 : 0;
}

static int build_unset_json_with_schema(const char* entity_name, const char* base_dir, const char* schema_file_name, const char* src_json, const char* field_name, char** out_json)
{
    SCHEMA_DEF schema;
    if (strcmp(field_name, "id") == 0) {
        fprintf(stderr, "error: id field cannot be unset\n");
        return -1;
    }
    if (load_schema(base_dir, schema_file_name, &schema) != 0) {
        return -1;
    }

    QS_MEMORY_CONTEXT context;
    if (create_temp_memory(&context, 4 * 1024 * 1024) != 0) {
        return -1;
    }

    QS_JSON_ELEMENT_OBJECT src_obj;
    QS_JSON_ELEMENT_OBJECT dst_obj;
    if (-1 == api_qs_json_decode_object(&context, &src_obj, src_json)) {
        api_qs_memory_free(&context);
        return -1;
    }
    if (!api_qs_object_exist(&src_obj, field_name)) {
        fprintf(stderr, "error: field not found: %s\n", field_name);
        api_qs_memory_free(&context);
        return -1;
    }
    if (-1 == api_qs_object_create(&context, &dst_obj)) {
        api_qs_memory_free(&context);
        return -1;
    }

    SCHEMA_FIELD_DEF* field = schema_find_field(&schema, field_name);
    if (field == NULL) {
        fprintf(stderr, "error: field not defined in schema for unset: %s\n", field_name);
        api_qs_memory_free(&context);
        return -1;
    }
    if (!field->unsettable) {
        fprintf(stderr, "error: field is not unsettable by schema: %s\n", field_name);
        api_qs_memory_free(&context);
        return -1;
    }

    if (copy_object_fields_except(&context, &dst_obj, &src_obj, field_name) != 0) {
        api_qs_memory_free(&context);
        return -1;
    }

    if (field != NULL && field->has_default) {
        int rc = 0;
        if (field->type == SCHEMA_FIELD_TYPE_INT32) {
            rc = api_qs_object_push_integer(&dst_obj, field->name, field->default_i32);
        } else if (field->type == SCHEMA_FIELD_TYPE_INT64) {
            rc = api_qs_object_push_big_integer(&dst_obj, field->name, field->default_i64);
        } else {
            rc = api_qs_object_push_string(&dst_obj, field->name, field->default_str);
        }
        if (rc == -1) {
            api_qs_memory_free(&context);
            return -1;
        }
    }

    if (validate_and_fill_schema_object(entity_name, &schema, &dst_obj, &context) != 0) {
        api_qs_memory_free(&context);
        return -1;
    }
    char* merged = api_qs_json_encode_object(&dst_obj, strlen(src_json) + 8192);
    if (merged == NULL) {
        api_qs_memory_free(&context);
        return -1;
    }
    *out_json = strdup(merged);
    api_qs_memory_free(&context);
    return (*out_json == NULL) ? -1 : 0;
}

static int next_counter_id(APP_STORES* stores, const char* counter_key, const char* id_prefix, char* out_id, size_t out_size)
{
    char* cur_text = api_qs_kvs_get(&stores->system, counter_key);
    long current = 0;
    if (cur_text != NULL) {
        current = strtol(cur_text, NULL, 10);
        if (current < 0) {
            current = 0;
        }
    }
    long next = current + 1;
    char next_text[64];
    if (snprintf(next_text, sizeof(next_text), "%ld", next) <= 0) {
        return -1;
    }
    if (kvs_set_with_compact_retry(stores, &stores->system, &STORE_SYSTEM, counter_key, next_text) != 0) {
        return -1;
    }
    if (snprintf(out_id, out_size, "%s%06ld", id_prefix, next) <= 0) {
        return -1;
    }
    return 0;
}

static SCHEMA_FIELD_DEF* schema_find_field(SCHEMA_DEF* schema, const char* name)
{
    int i;
    for (i = 0; i < schema->field_count; i++) {
        if (strcmp(schema->fields[i].name, name) == 0) {
            return &schema->fields[i];
        }
    }
    return NULL;
}

static int parse_schema_type(const char* type_text, SCHEMA_FIELD_TYPE* out_type)
{
    if (strcmp(type_text, "int32") == 0) {
        *out_type = SCHEMA_FIELD_TYPE_INT32;
        return 0;
    }
    if (strcmp(type_text, "int64") == 0) {
        *out_type = SCHEMA_FIELD_TYPE_INT64;
        return 0;
    }
    if (strcmp(type_text, "string") == 0) {
        *out_type = SCHEMA_FIELD_TYPE_STRING;
        return 0;
    }
    return -1;
}

static int load_system_settings(const char* base_dir, APP_STORES* stores)
{
    char path[4096];
    char* text = NULL;
    stores->issue_status_count = 0;
    if (build_store_path(path, sizeof(path), base_dir, "system_settings.json") != 0) {
        return -1;
    }
    if (read_text_file(path, &text) != 0) {
        fprintf(stderr, "error: failed to read system settings file: %s\n", path);
        return -1;
    }

    QS_MEMORY_CONTEXT context;
    if (create_temp_memory(&context, 2 * 1024 * 1024) != 0) {
        free(text);
        return -1;
    }
    QS_JSON_ELEMENT_OBJECT root;
    if (-1 == api_qs_json_decode_object(&context, &root, text)) {
        free(text);
        api_qs_memory_free(&context);
        fprintf(stderr, "error: invalid system settings json: %s\n", path);
        return -1;
    }
    free(text);

    QS_JSON_ELEMENT_OBJECT issue_obj;
    if (-1 == api_qs_object_get_object(&root, "issue", &issue_obj)) {
        api_qs_memory_free(&context);
        fprintf(stderr, "error: issue settings not found: %s\n", path);
        return -1;
    }
    QS_JSON_ELEMENT_ARRAY statuses;
    if (-1 == api_qs_object_get_array(&issue_obj, "statuses", &statuses)) {
        api_qs_memory_free(&context);
        fprintf(stderr, "error: issue.statuses not found: %s\n", path);
        return -1;
    }
    int32_t len = api_qs_array_get_length(&statuses);
    if (len <= 0 || len > MAX_ISSUE_STATUS_VALUES) {
        api_qs_memory_free(&context);
        fprintf(stderr, "error: issue.statuses count is invalid: %d\n", len);
        return -1;
    }
    int32_t i;
    for (i = 0; i < len; i++) {
        char* s = api_qs_array_get_string(&statuses, i);
        if (s == NULL || s[0] == '\0') {
            api_qs_memory_free(&context);
            fprintf(stderr, "error: issue.statuses contains non-string or empty value\n");
            return -1;
        }
        snprintf(stores->issue_statuses[stores->issue_status_count], sizeof(stores->issue_statuses[0]), "%s", s);
        stores->issue_status_count++;
    }
    api_qs_memory_free(&context);
    return 0;
}

static int is_allowed_issue_status(APP_STORES* stores, const char* status)
{
    int i;
    for (i = 0; i < stores->issue_status_count; i++) {
        if (strcmp(stores->issue_statuses[i], status) == 0) {
            return 1;
        }
    }
    return 0;
}

static int load_schema(const char* base_dir, const char* schema_file_name, SCHEMA_DEF* out_schema)
{
    memset(out_schema, 0, sizeof(*out_schema));

    char schema_path[4096];
    if (build_store_path(schema_path, sizeof(schema_path), base_dir, schema_file_name) != 0) {
        return -1;
    }
    char* schema_text = NULL;
    if (read_text_file(schema_path, &schema_text) != 0) {
        fprintf(stderr, "error: failed to read schema file: %s\n", schema_path);
        return -1;
    }

    QS_MEMORY_CONTEXT context;
    if (create_temp_memory(&context, 2 * 1024 * 1024) != 0) {
        free(schema_text);
        return -1;
    }
    QS_JSON_ELEMENT_OBJECT root;
    if (-1 == api_qs_json_decode_object(&context, &root, schema_text)) {
        free(schema_text);
        api_qs_memory_free(&context);
        fprintf(stderr, "error: invalid schema json: %s\n", schema_path);
        return -1;
    }
    free(schema_text);

    QS_JSON_ELEMENT_ARRAY fields;
    if (-1 == api_qs_object_get_array(&root, "fields", &fields)) {
        api_qs_memory_free(&context);
        fprintf(stderr, "error: schema fields not found: %s\n", schema_path);
        return -1;
    }
    int32_t len = api_qs_array_get_length(&fields);
    if (len > MAX_SCHEMA_FIELDS) {
        api_qs_memory_free(&context);
        fprintf(stderr, "error: schema field count over limit: %d\n", len);
        return -1;
    }

    int32_t i;
    for (i = 0; i < len; i++) {
        QS_JSON_ELEMENT_OBJECT fobj;
        if (-1 == api_qs_array_get_object(&fields, i, &fobj)) {
            api_qs_memory_free(&context);
            return -1;
        }
        SCHEMA_FIELD_DEF* f = &out_schema->fields[out_schema->field_count];
        memset(f, 0, sizeof(*f));

        char* name = api_qs_object_get_string(&fobj, "name");
        char* type = api_qs_object_get_string(&fobj, "type");
        if (name == NULL || type == NULL) {
            api_qs_memory_free(&context);
            return -1;
        }
        snprintf(f->name, sizeof(f->name), "%s", name);
        if (parse_schema_type(type, &f->type) != 0) {
            api_qs_memory_free(&context);
            fprintf(stderr, "error: unsupported schema type: %s\n", type);
            return -1;
        }

        if (api_qs_object_exist(&fobj, "required")) {
            f->required = api_qs_object_get_integer_val(&fobj, "required") ? 1 : 0;
        }
        if (api_qs_object_exist(&fobj, "unsettable")) {
            f->unsettable = api_qs_object_get_integer_val(&fobj, "unsettable") ? 1 : 0;
        }

        if (f->type == SCHEMA_FIELD_TYPE_STRING && api_qs_object_exist(&fobj, "maxLength")) {
            f->has_max_length = 1;
            f->max_length = (size_t)api_qs_object_get_big_integer_val(&fobj, "maxLength");
        }

        if (api_qs_object_exist(&fobj, "default")) {
            f->has_default = 1;
            if (f->type == SCHEMA_FIELD_TYPE_INT32) {
                int32_t* p = api_qs_object_get_integer(&fobj, "default");
                if (p == NULL) {
                    api_qs_memory_free(&context);
                    return -1;
                }
                f->default_i32 = *p;
            } else if (f->type == SCHEMA_FIELD_TYPE_INT64) {
                f->default_i64 = api_qs_object_get_big_integer_val(&fobj, "default");
            } else {
                char* s = api_qs_object_get_string(&fobj, "default");
                if (s == NULL) {
                    api_qs_memory_free(&context);
                    return -1;
                }
                snprintf(f->default_str, sizeof(f->default_str), "%s", s);
            }
        }

        out_schema->field_count++;
    }

    api_qs_memory_free(&context);
    return 0;
}

static int is_allowed_extra_field(const char* entity_name, const char* key)
{
    if (strcmp(entity_name, "issue") == 0) {
        if (strcmp(key, "assigneeIds") == 0 || strcmp(key, "labelIds") == 0) {
            return 1;
        }
    }
    return 0;
}

static int validate_schema_value(QS_JSON_ELEMENT_OBJECT* obj, SCHEMA_FIELD_DEF* field)
{
    if (field->type == SCHEMA_FIELD_TYPE_INT32) {
        if (api_qs_object_get_integer(obj, field->name) == NULL) {
            fprintf(stderr, "error: schema type mismatch(int32): %s\n", field->name);
            return -1;
        }
        return 0;
    }
    if (field->type == SCHEMA_FIELD_TYPE_INT64) {
        if (api_qs_object_get_integer(obj, field->name) == NULL &&
            api_qs_object_get_big_integer(obj, field->name) == NULL &&
            api_qs_object_get_unsigned_big_integer(obj, field->name) == NULL) {
            fprintf(stderr, "error: schema type mismatch(int64): %s\n", field->name);
            return -1;
        }
        return 0;
    }

    char* s = api_qs_object_get_string(obj, field->name);
    if (s == NULL) {
        fprintf(stderr, "error: schema type mismatch(string): %s\n", field->name);
        return -1;
    }
    if (field->has_max_length && strlen(s) > field->max_length) {
        fprintf(stderr, "error: schema maxLength exceeded: %s\n", field->name);
        return -1;
    }
    return 0;
}

static int apply_schema_on_write(const char* entity_name, const char* base_dir, const char* schema_file_name, const char* src_json, char** out_json)
{
    SCHEMA_DEF schema;
    if (load_schema(base_dir, schema_file_name, &schema) != 0) {
        return -1;
    }

    QS_MEMORY_CONTEXT context;
    if (create_temp_memory(&context, 2 * 1024 * 1024) != 0) {
        return -1;
    }
    QS_JSON_ELEMENT_OBJECT obj;
    if (-1 == api_qs_json_decode_object(&context, &obj, src_json)) {
        fprintf(stderr, "error: failed to decode target json for schema apply\n");
        api_qs_memory_free(&context);
        return -1;
    }

    if (validate_and_fill_schema_object(entity_name, &schema, &obj, &context) != 0) {
        api_qs_memory_free(&context);
        return -1;
    }

    char* encoded = api_qs_json_encode_object(&obj, strlen(src_json) + 8192);
    if (encoded == NULL) {
        api_qs_memory_free(&context);
        return -1;
    }
    *out_json = strdup(encoded);
    api_qs_memory_free(&context);
    return (*out_json == NULL) ? -1 : 0;
}

static int validate_and_fill_schema_object(const char* entity_name, SCHEMA_DEF* schema, QS_JSON_ELEMENT_OBJECT* obj, QS_MEMORY_CONTEXT* context)
{
    QS_JSON_ELEMENT_ARRAY keys;
    if (-1 == api_qs_array_create(context, &keys)) {
        return -1;
    }
    if (-1 == api_qs_object_get_keys(obj, &keys)) {
        return -1;
    }
    int32_t klen = api_qs_array_get_length(&keys);
    int32_t i;
    for (i = 0; i < klen; i++) {
        char* key = api_qs_array_get_string(&keys, i);
        if (key == NULL) {
            continue;
        }
        if (schema_find_field(schema, key) == NULL && !is_allowed_extra_field(entity_name, key)) {
            fprintf(stderr, "error: field not defined in schema: %s\n", key);
            return -1;
        }
    }

    for (i = 0; i < schema->field_count; i++) {
        SCHEMA_FIELD_DEF* f = &schema->fields[i];
        int exists = api_qs_object_exist(obj, f->name);
        if (!exists && f->has_default) {
            if (f->type == SCHEMA_FIELD_TYPE_INT32) {
                if (-1 == api_qs_object_push_integer(obj, f->name, f->default_i32)) {
                    return -1;
                }
            } else if (f->type == SCHEMA_FIELD_TYPE_INT64) {
                if (-1 == api_qs_object_push_big_integer(obj, f->name, f->default_i64)) {
                    return -1;
                }
            } else {
                if (-1 == api_qs_object_push_string(obj, f->name, f->default_str)) {
                    return -1;
                }
            }
            exists = 1;
        }

        if (!exists && f->required) {
            fprintf(stderr, "error: required field missing: %s\n", f->name);
            return -1;
        }
        if (exists && validate_schema_value(obj, f) != 0) {
            return -1;
        }
    }
    return 0;
}

static int entity_create(APP_STORES* stores, QS_KVS_CONTEXT* store, const STORE_CONFIG* cfg, const char* key, const char* json)
{
    if (api_qs_kvs_get(store, key) != NULL) {
        fprintf(stderr, "error: key already exists: %s\n", key);
        return EXIT_KVS;
    }
    if (kvs_set_with_compact_retry(stores, store, cfg, key, json) != 0) {
        fprintf(stderr, "error: failed to set key: %s\n", key);
        return EXIT_KVS;
    }
    printf("created: %s\n", key);
    return EXIT_OK;
}

static int entity_get(QS_KVS_CONTEXT* store, const char* key)
{
    char* value = api_qs_kvs_get(store, key);
    if (value == NULL) {
        fprintf(stderr, "error: key not found: %s\n", key);
        return EXIT_NOT_FOUND;
    }
    printf("%s\n", value);
    return EXIT_OK;
}

static int entity_update(APP_STORES* stores, QS_KVS_CONTEXT* store, const STORE_CONFIG* cfg, const char* key, const char* json)
{
    if (api_qs_kvs_get(store, key) == NULL) {
        fprintf(stderr, "error: key not found: %s\n", key);
        return EXIT_NOT_FOUND;
    }
    if (kvs_set_with_compact_retry(stores, store, cfg, key, json) != 0) {
        fprintf(stderr, "error: failed to update key: %s\n", key);
        return EXIT_KVS;
    }
    printf("updated: %s\n", key);
    return EXIT_OK;
}

static int entity_delete(QS_KVS_CONTEXT* store, const char* key)
{
    if (api_qs_kvs_get(store, key) == NULL) {
        fprintf(stderr, "error: key not found: %s\n", key);
        return EXIT_NOT_FOUND;
    }
    if (-1 == api_qs_kvs_delete(store, key)) {
        fprintf(stderr, "error: failed to delete key: %s\n", key);
        return EXIT_KVS;
    }
    printf("deleted: %s\n", key);
    return EXIT_OK;
}

static int entity_list_all(QS_KVS_CONTEXT* store, const char* entity_name)
{
    QS_MEMORY_CONTEXT temp;
    if (create_temp_memory(&temp, 4 * 1024 * 1024) != 0) {
        return EXIT_IO;
    }

    QS_JSON_ELEMENT_ARRAY keys;
    if (-1 == api_qs_array_create(&temp, &keys)) {
        api_qs_memory_free(&temp);
        return EXIT_IO;
    }
    if (-1 == api_qs_kvs_sorted_keys(&keys, store, 1)) {
        api_qs_memory_free(&temp);
        return EXIT_KVS;
    }

    char prefix[128];
    if (snprintf(prefix, sizeof(prefix), "%s:", entity_name) <= 0) {
        api_qs_memory_free(&temp);
        return EXIT_IO;
    }
    size_t prefix_len = strlen(prefix);

    printf("[\n");
    int printed = 0;
    int32_t len = api_qs_array_get_length(&keys);
    int32_t i;
    for (i = 0; i < len; i++) {
        char* key = api_qs_array_get_string(&keys, i);
        char* value;
        if (key == NULL || strncmp(key, prefix, prefix_len) != 0) {
            continue;
        }
        value = api_qs_kvs_get(store, key);
        if (value == NULL) {
            continue;
        }
        if (printed > 0) {
            printf(",\n");
        }
        printf("%s", value);
        printed++;
    }
    printf("\n]\n");

    api_qs_memory_free(&temp);
    return EXIT_OK;
}

static int decode_index_array(QS_MEMORY_CONTEXT* context, const char* json_array_text, QS_JSON_ELEMENT_ARRAY* out_array)
{
    if (json_array_text == NULL) {
        return api_qs_array_create(context, out_array);
    }

    size_t src_len = strlen(json_array_text);
    size_t wrapper_len = src_len + 32;
    char* wrapper = (char*)malloc(wrapper_len);
    if (wrapper == NULL) {
        return -1;
    }
    snprintf(wrapper, wrapper_len, "{\"ids\":%s}", json_array_text);

    QS_JSON_ELEMENT_OBJECT obj;
    if (-1 == api_qs_json_decode_object(context, &obj, wrapper)) {
        free(wrapper);
        return -1;
    }
    free(wrapper);
    if (-1 == api_qs_object_get_array(&obj, "ids", out_array)) {
        return -1;
    }
    return 0;
}

static int array_has_string(QS_JSON_ELEMENT_ARRAY* array, const char* value)
{
    int32_t len = api_qs_array_get_length(array);
    int32_t i;
    for (i = 0; i < len; i++) {
        char* item = api_qs_array_get_string(array, i);
        if (item != NULL && strcmp(item, value) == 0) {
            return 1;
        }
    }
    return 0;
}

static int append_unique_string(QS_JSON_ELEMENT_ARRAY* array, const char* value)
{
    if (array_has_string(array, value)) {
        return 0;
    }
    return api_qs_array_push_string(array, value);
}

static int remove_string_and_build(QS_MEMORY_CONTEXT* context, QS_JSON_ELEMENT_ARRAY* src, const char* remove_value, QS_JSON_ELEMENT_ARRAY* dst)
{
    if (-1 == api_qs_array_create(context, dst)) {
        return -1;
    }
    int32_t len = api_qs_array_get_length(src);
    int32_t i;
    for (i = 0; i < len; i++) {
        char* item = api_qs_array_get_string(src, i);
        if (item == NULL || strcmp(item, remove_value) == 0) {
            continue;
        }
        if (-1 == api_qs_array_push_string(dst, item)) {
            return -1;
        }
    }
    return 0;
}

static int index_add_id(APP_STORES* stores, const char* index_key, const char* issue_id)
{
    QS_MEMORY_CONTEXT context;
    if (create_temp_memory(&context, 1024 * 1024) != 0) {
        return -1;
    }

    char* old_value = api_qs_kvs_get(&stores->index, index_key);
    QS_JSON_ELEMENT_ARRAY ids;
    if (decode_index_array(&context, old_value, &ids) != 0) {
        api_qs_memory_free(&context);
        return -1;
    }
    if (-1 == append_unique_string(&ids, issue_id)) {
        api_qs_memory_free(&context);
        return -1;
    }

    char* encoded = api_qs_json_encode_array(&ids, INDEX_JSON_BUFFER_SIZE);
    if (encoded == NULL) {
        api_qs_memory_free(&context);
        return -1;
    }

    int rc = kvs_set_with_compact_retry(stores, &stores->index, &STORE_INDEX, index_key, encoded);
    api_qs_memory_free(&context);
    return (rc != 0) ? -1 : 0;
}

static int index_remove_id(APP_STORES* stores, const char* index_key, const char* issue_id)
{
    char* old_value = api_qs_kvs_get(&stores->index, index_key);
    if (old_value == NULL) {
        return 0;
    }

    QS_MEMORY_CONTEXT context;
    if (create_temp_memory(&context, 1024 * 1024) != 0) {
        return -1;
    }

    QS_JSON_ELEMENT_ARRAY ids;
    if (decode_index_array(&context, old_value, &ids) != 0) {
        api_qs_memory_free(&context);
        return -1;
    }

    QS_JSON_ELEMENT_ARRAY filtered;
    if (remove_string_and_build(&context, &ids, issue_id, &filtered) != 0) {
        api_qs_memory_free(&context);
        return -1;
    }

    int32_t len = api_qs_array_get_length(&filtered);
    int rc;
    if (len == 0) {
        rc = api_qs_kvs_delete(&stores->index, index_key);
    } else {
        char* encoded = api_qs_json_encode_array(&filtered, INDEX_JSON_BUFFER_SIZE);
        if (encoded == NULL) {
            api_qs_memory_free(&context);
            return -1;
        }
        rc = kvs_set_with_compact_retry(stores, &stores->index, &STORE_INDEX, index_key, encoded);
    }

    api_qs_memory_free(&context);
    return (rc == -1 || rc != 0) ? -1 : 0;
}

static int for_each_string_in_array(QS_JSON_ELEMENT_ARRAY* array, int (*cb)(const char* value, void* user_data), void* user_data)
{
    int32_t len = api_qs_array_get_length(array);
    int32_t i;
    for (i = 0; i < len; i++) {
        char* item = api_qs_array_get_string(array, i);
        if (item == NULL) {
            continue;
        }
        if (cb(item, user_data) != 0) {
            return -1;
        }
    }
    return 0;
}

typedef struct REF_VALIDATE_DATA {
    QS_KVS_CONTEXT* store;
    const char* entity_name;
} REF_VALIDATE_DATA;

static int cb_validate_reference_id(const char* value, void* user_data)
{
    REF_VALIDATE_DATA* data = (REF_VALIDATE_DATA*)user_data;
    char key[1024];
    if (snprintf(key, sizeof(key), "%s:%s", data->entity_name, value) <= 0) {
        return -1;
    }
    if (api_qs_kvs_get(data->store, key) == NULL) {
        fprintf(stderr, "error: %s id not found: %s\n", data->entity_name, value);
        return -1;
    }
    return 0;
}

static int validate_reference_array(QS_JSON_ELEMENT_OBJECT* obj, const char* field_name, QS_KVS_CONTEXT* store, const char* entity_name)
{
    QS_JSON_ELEMENT_ARRAY ids;
    if (api_qs_object_get_array(obj, field_name, &ids) != 0) {
        return 0;
    }
    REF_VALIDATE_DATA data = {store, entity_name};
    if (for_each_string_in_array(&ids, cb_validate_reference_id, &data) != 0) {
        return -1;
    }
    return 0;
}

static int validate_issue_references(APP_STORES* stores, const char* issue_json)
{
    QS_MEMORY_CONTEXT context;
    if (create_temp_memory(&context, 1024 * 1024) != 0) {
        return -1;
    }
    QS_JSON_ELEMENT_OBJECT obj;
    if (-1 == api_qs_json_decode_object(&context, &obj, issue_json)) {
        fprintf(stderr, "error: invalid issue json\n");
        api_qs_memory_free(&context);
        return -1;
    }
    char* status = api_qs_object_get_string(&obj, "status");
    if (status != NULL && !is_allowed_issue_status(stores, status)) {
        fprintf(stderr, "error: issue status is not allowed: %s\n", status);
        api_qs_memory_free(&context);
        return -1;
    }
    if (validate_reference_array(&obj, "assigneeIds", &stores->user, "user") != 0) {
        api_qs_memory_free(&context);
        return -1;
    }
    if (validate_reference_array(&obj, "labelIds", &stores->label, "label") != 0) {
        api_qs_memory_free(&context);
        return -1;
    }
    api_qs_memory_free(&context);
    return 0;
}

typedef struct ISSUE_INDEX_DATA {
    APP_STORES* stores;
    const char* issue_id;
} ISSUE_INDEX_DATA;

static int cb_add_assignee_index(const char* value, void* user_data)
{
    ISSUE_INDEX_DATA* d = (ISSUE_INDEX_DATA*)user_data;
    char key[1024];
    if (snprintf(key, sizeof(key), "index:assignee:%s", value) <= 0) {
        return -1;
    }
    return index_add_id(d->stores, key, d->issue_id);
}

static int cb_add_label_index(const char* value, void* user_data)
{
    ISSUE_INDEX_DATA* d = (ISSUE_INDEX_DATA*)user_data;
    char key[1024];
    if (snprintf(key, sizeof(key), "index:label:%s", value) <= 0) {
        return -1;
    }
    return index_add_id(d->stores, key, d->issue_id);
}

static int cb_remove_assignee_index(const char* value, void* user_data)
{
    ISSUE_INDEX_DATA* d = (ISSUE_INDEX_DATA*)user_data;
    char key[1024];
    if (snprintf(key, sizeof(key), "index:assignee:%s", value) <= 0) {
        return -1;
    }
    return index_remove_id(d->stores, key, d->issue_id);
}

static int cb_remove_label_index(const char* value, void* user_data)
{
    ISSUE_INDEX_DATA* d = (ISSUE_INDEX_DATA*)user_data;
    char key[1024];
    if (snprintf(key, sizeof(key), "index:label:%s", value) <= 0) {
        return -1;
    }
    return index_remove_id(d->stores, key, d->issue_id);
}

static int issue_apply_indexes(APP_STORES* stores, const char* issue_id, const char* issue_json, int is_add)
{
    QS_MEMORY_CONTEXT context;
    if (create_temp_memory(&context, 1024 * 1024) != 0) {
        return -1;
    }

    QS_JSON_ELEMENT_OBJECT obj;
    if (-1 == api_qs_json_decode_object(&context, &obj, issue_json)) {
        fprintf(stderr, "error: invalid issue json\n");
        api_qs_memory_free(&context);
        return -1;
    }

    char* status = api_qs_object_get_string(&obj, "status");
    if (status != NULL) {
        char status_key[1024];
        snprintf(status_key, sizeof(status_key), "index:status:%s", status);
        if (is_add) {
            if (index_add_id(stores, status_key, issue_id) != 0) {
                api_qs_memory_free(&context);
                return -1;
            }
        } else {
            if (index_remove_id(stores, status_key, issue_id) != 0) {
                api_qs_memory_free(&context);
                return -1;
            }
        }
    }

    ISSUE_INDEX_DATA data = {stores, issue_id};
    QS_JSON_ELEMENT_ARRAY assignee_ids;
    if (api_qs_object_get_array(&obj, "assigneeIds", &assignee_ids) == 0) {
        if (is_add) {
            if (for_each_string_in_array(&assignee_ids, cb_add_assignee_index, &data) != 0) {
                api_qs_memory_free(&context);
                return -1;
            }
        } else {
            if (for_each_string_in_array(&assignee_ids, cb_remove_assignee_index, &data) != 0) {
                api_qs_memory_free(&context);
                return -1;
            }
        }
    }

    QS_JSON_ELEMENT_ARRAY label_ids;
    if (api_qs_object_get_array(&obj, "labelIds", &label_ids) == 0) {
        if (is_add) {
            if (for_each_string_in_array(&label_ids, cb_add_label_index, &data) != 0) {
                api_qs_memory_free(&context);
                return -1;
            }
        } else {
            if (for_each_string_in_array(&label_ids, cb_remove_label_index, &data) != 0) {
                api_qs_memory_free(&context);
                return -1;
            }
        }
    }

    api_qs_memory_free(&context);
    return 0;
}

static int issue_create(APP_STORES* stores, const char* id, const char* issue_json)
{
    char key[1024];
    if (build_entity_key(key, sizeof(key), "issue", id) != 0) {
        return EXIT_IO;
    }
    if (api_qs_kvs_get(&stores->issue, key) != NULL) {
        fprintf(stderr, "error: issue already exists: %s\n", key);
        return EXIT_KVS;
    }
    if (validate_issue_references(stores, issue_json) != 0) {
        return EXIT_USAGE;
    }
    if (kvs_set_with_compact_retry(stores, &stores->issue, &STORE_ISSUE, key, issue_json) != 0) {
        fprintf(stderr, "error: failed to create issue: %s\n", key);
        return EXIT_KVS;
    }
    if (issue_apply_indexes(stores, id, issue_json, 1) != 0) {
        api_qs_kvs_delete(&stores->issue, key);
        fprintf(stderr, "error: failed to update index on issue create\n");
        return EXIT_KVS;
    }
    printf("created: %s\n", key);
    return EXIT_OK;
}

static int issue_update(APP_STORES* stores, const char* id, const char* issue_json)
{
    char key[1024];
    if (build_entity_key(key, sizeof(key), "issue", id) != 0) {
        return EXIT_IO;
    }
    char* old_json = api_qs_kvs_get(&stores->issue, key);
    if (old_json == NULL) {
        fprintf(stderr, "error: issue not found: %s\n", key);
        return EXIT_NOT_FOUND;
    }
    if (validate_issue_references(stores, issue_json) != 0) {
        return EXIT_USAGE;
    }
    char* old_copy = strdup(old_json);
    if (old_copy == NULL) {
        return EXIT_IO;
    }

    if (issue_apply_indexes(stores, id, old_copy, 0) != 0) {
        free(old_copy);
        fprintf(stderr, "error: failed to remove old index on issue update\n");
        return EXIT_KVS;
    }
    if (kvs_set_with_compact_retry(stores, &stores->issue, &STORE_ISSUE, key, issue_json) != 0) {
        issue_apply_indexes(stores, id, old_copy, 1);
        free(old_copy);
        fprintf(stderr, "error: failed to update issue: %s\n", key);
        return EXIT_KVS;
    }
    if (issue_apply_indexes(stores, id, issue_json, 1) != 0) {
        kvs_set_with_compact_retry(stores, &stores->issue, &STORE_ISSUE, key, old_copy);
        issue_apply_indexes(stores, id, old_copy, 1);
        free(old_copy);
        fprintf(stderr, "error: failed to add new index on issue update\n");
        return EXIT_KVS;
    }

    free(old_copy);
    printf("updated: %s\n", key);
    return EXIT_OK;
}

static int issue_delete(APP_STORES* stores, const char* id)
{
    char key[1024];
    if (build_entity_key(key, sizeof(key), "issue", id) != 0) {
        return EXIT_IO;
    }
    char* old_json = api_qs_kvs_get(&stores->issue, key);
    if (old_json == NULL) {
        fprintf(stderr, "error: issue not found: %s\n", key);
        return EXIT_NOT_FOUND;
    }
    char* old_copy = strdup(old_json);
    if (old_copy == NULL) {
        return EXIT_IO;
    }

    if (issue_apply_indexes(stores, id, old_copy, 0) != 0) {
        free(old_copy);
        fprintf(stderr, "error: failed to remove issue index\n");
        return EXIT_KVS;
    }
    if (-1 == api_qs_kvs_delete(&stores->issue, key)) {
        issue_apply_indexes(stores, id, old_copy, 1);
        free(old_copy);
        fprintf(stderr, "error: failed to delete issue key\n");
        return EXIT_KVS;
    }
    free(old_copy);
    printf("deleted: %s\n", key);
    return EXIT_OK;
}

static int print_issue_list_from_ids(APP_STORES* stores, QS_JSON_ELEMENT_ARRAY* ids)
{
    int32_t len = api_qs_array_get_length(ids);
    int32_t i;
    int printed = 0;
    printf("[\n");
    for (i = 0; i < len; i++) {
        char* id = api_qs_array_get_string(ids, i);
        char key[1024];
        char* issue_json;
        if (id == NULL) {
            continue;
        }
        if (build_entity_key(key, sizeof(key), "issue", id) != 0) {
            continue;
        }
        issue_json = api_qs_kvs_get(&stores->issue, key);
        if (issue_json == NULL) {
            continue;
        }
        if (printed > 0) {
            printf(",\n");
        }
        printf("%s", issue_json);
        printed++;
    }
    printf("\n]\n");
    return EXIT_OK;
}

static int issue_list_all(APP_STORES* stores)
{
    QS_MEMORY_CONTEXT temp;
    if (create_temp_memory(&temp, 4 * 1024 * 1024) != 0) {
        return EXIT_IO;
    }

    QS_JSON_ELEMENT_ARRAY keys;
    if (-1 == api_qs_array_create(&temp, &keys)) {
        api_qs_memory_free(&temp);
        return EXIT_IO;
    }
    if (-1 == api_qs_kvs_sorted_keys(&keys, &stores->issue, 1)) {
        api_qs_memory_free(&temp);
        return EXIT_KVS;
    }

    printf("[\n");
    int printed = 0;
    int32_t len = api_qs_array_get_length(&keys);
    int32_t i;
    for (i = 0; i < len; i++) {
        char* key = api_qs_array_get_string(&keys, i);
        char* issue_json;
        if (key == NULL || strncmp(key, "issue:", 6) != 0) {
            continue;
        }
        issue_json = api_qs_kvs_get(&stores->issue, key);
        if (issue_json == NULL) {
            continue;
        }
        if (printed > 0) {
            printf(",\n");
        }
        printf("%s", issue_json);
        printed++;
    }
    printf("\n]\n");

    api_qs_memory_free(&temp);
    return EXIT_OK;
}

static int issue_list_by_index(APP_STORES* stores, const char* index_key)
{
    char* ids_json = api_qs_kvs_get(&stores->index, index_key);
    if (ids_json == NULL) {
        printf("[]\n");
        return EXIT_OK;
    }

    QS_MEMORY_CONTEXT temp;
    if (create_temp_memory(&temp, 2 * 1024 * 1024) != 0) {
        return EXIT_IO;
    }
    QS_JSON_ELEMENT_ARRAY ids;
    if (decode_index_array(&temp, ids_json, &ids) != 0) {
        api_qs_memory_free(&temp);
        fprintf(stderr, "error: invalid index data for key: %s\n", index_key);
        return EXIT_KVS;
    }

    int rc = print_issue_list_from_ids(stores, &ids);
    api_qs_memory_free(&temp);
    return rc;
}

static int copy_store_data(QS_KVS_CONTEXT* src, QS_KVS_CONTEXT* dst)
{
    QS_MEMORY_CONTEXT temp;
    if (create_temp_memory(&temp, 8 * 1024 * 1024) != 0) {
        return -1;
    }
    QS_JSON_ELEMENT_ARRAY keys;
    if (-1 == api_qs_array_create(&temp, &keys)) {
        api_qs_memory_free(&temp);
        return -1;
    }
    if (-1 == api_qs_kvs_keys(&keys, src)) {
        api_qs_memory_free(&temp);
        return -1;
    }

    int32_t len = api_qs_array_get_length(&keys);
    int32_t i;
    for (i = 0; i < len; i++) {
        char* key = api_qs_array_get_string(&keys, i);
        char* value;
        if (key == NULL) {
            continue;
        }
        value = api_qs_kvs_get(src, key);
        if (value == NULL) {
            continue;
        }
        if (-1 == api_qs_kvs_set(dst, key, value, 0)) {
            api_qs_memory_free(&temp);
            return -1;
        }
    }
    api_qs_memory_free(&temp);
    return 0;
}

static int compact_store(const STORE_CONFIG* cfg, const char* base_dir)
{
    char path[4096];
    char tmp_path[4096];
    char bak_path[4096];

    if (build_store_path(path, sizeof(path), base_dir, cfg->file_name) != 0) {
        return -1;
    }
    if (!file_exists(path)) {
        fprintf(stderr, "error: store file not found: %s\n", path);
        return -1;
    }
    if (snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path) <= 0) {
        return -1;
    }
    if (snprintf(bak_path, sizeof(bak_path), "%s.bak", path) <= 0) {
        return -1;
    }

    unlink(tmp_path);
    unlink(bak_path);

    QS_KVS_CONTEXT src;
    QS_KVS_CONTEXT dst;
    if (-1 == cfg->create_fn(&src, path)) {
        fprintf(stderr, "error: failed to open source store: %s\n", path);
        return -1;
    }
    if (-1 == cfg->create_fn(&dst, tmp_path)) {
        api_qs_persistence_kvs_memory_free(&src);
        fprintf(stderr, "error: failed to create temp store: %s\n", tmp_path);
        return -1;
    }
    if (copy_store_data(&src, &dst) != 0) {
        api_qs_persistence_kvs_memory_free(&src);
        api_qs_persistence_kvs_memory_free(&dst);
        unlink(tmp_path);
        fprintf(stderr, "error: failed to copy store data: %s\n", cfg->name);
        return -1;
    }

    api_qs_persistence_kvs_memory_free(&src);
    api_qs_persistence_kvs_memory_free(&dst);

    if (rename(path, bak_path) != 0) {
        unlink(tmp_path);
        fprintf(stderr, "error: failed to backup store: %s\n", path);
        return -1;
    }
    if (rename(tmp_path, path) != 0) {
        rename(bak_path, path);
        unlink(tmp_path);
        fprintf(stderr, "error: failed to switch compacted store: %s\n", path);
        return -1;
    }
    unlink(bak_path);
    printf("compacted: %s\n", path);
    return 0;
}

static int command_init(int argc, char* argv[])
{
    const char* base_dir = DEFAULT_BASE_DIR;
    int force = has_option(argc, argv, "--force");
    const char* base_dir_arg = get_option_value(argc, argv, "--base-dir");
    if (base_dir_arg != NULL) {
        base_dir = base_dir_arg;
    }

    if (ensure_directory(base_dir) != 0) {
        return EXIT_IO;
    }

    const STORE_CONFIG* stores[] = {&STORE_USER, &STORE_LABEL, &STORE_ISSUE, &STORE_INDEX, &STORE_SYSTEM};
    size_t store_len = sizeof(stores) / sizeof(stores[0]);
    size_t i;
    for (i = 0; i < store_len; i++) {
        char path[4096];
        if (build_store_path(path, sizeof(path), base_dir, stores[i]->file_name) != 0) {
            fprintf(stderr, "error: failed to build path for %s\n", stores[i]->name);
            return EXIT_IO;
        }
        if (create_store_file(stores[i], path, force) != 0) {
            return EXIT_KVS;
        }
        printf("created: %s (%s)\n", stores[i]->name, path);
    }
    if (init_schema_files(base_dir, force) != 0) {
        return EXIT_IO;
    }
    if (write_system_settings_file(base_dir, force) != 0) {
        return EXIT_IO;
    }
    printf("init done: base_dir=%s\n", base_dir);
    return EXIT_OK;
}

static int command_entity_common(int argc, char* argv[], APP_STORES* stores, const char* entity_name, const char* id_prefix, const char* counter_key, QS_KVS_CONTEXT* store, const STORE_CONFIG* cfg)
{
    if (argc < 3) {
        return EXIT_USAGE;
    }
    const char* action = argv[2];
    const char* id = get_option_value(argc, argv, "--id");
    const char* json = get_option_value(argc, argv, "--json");
    const char* field = get_option_value(argc, argv, "--field");
    char key[1024];

    if (strcmp(action, "list") == 0) {
        return entity_list_all(store, entity_name);
    }

    if (strcmp(action, "create") == 0) {
        char generated_id[128];
        const char* final_id = id;
        char* schema_json = NULL;
        char* final_json = NULL;
        if (json == NULL) {
            fprintf(stderr, "error: --json is required for create\n");
            return EXIT_USAGE;
        }
        if (final_id == NULL) {
            if (next_counter_id(stores, counter_key, id_prefix, generated_id, sizeof(generated_id)) != 0) {
                fprintf(stderr, "error: failed to generate id for %s\n", entity_name);
                return EXIT_KVS;
            }
            final_id = generated_id;
            printf("generated id: %s\n", final_id);
        }
        if (build_entity_key(key, sizeof(key), entity_name, final_id) != 0) {
            return EXIT_IO;
        }
        const char* schema_file = (strcmp(entity_name, "user") == 0) ? "schema_user.json" : "schema_label.json";
        if (apply_schema_on_write(entity_name, stores->base_dir, schema_file, json, &schema_json) != 0) {
            fprintf(stderr, "error: schema validation failed for create\n");
            return EXIT_USAGE;
        }
        int json_rc = json_with_forced_id(schema_json, final_id, id != NULL, &final_json);
        free(schema_json);
        if (json_rc == -2) {
            fprintf(stderr, "error: --id and json.id mismatch for create\n");
            return EXIT_USAGE;
        }
        if (json_rc != 0) {
            fprintf(stderr, "error: invalid json for create\n");
            return EXIT_USAGE;
        }
        int rc = entity_create(stores, store, cfg, key, final_json);
        free(final_json);
        return rc;
    }

    if (id == NULL) {
        fprintf(stderr, "error: --id is required\n");
        return EXIT_USAGE;
    }
    if (build_entity_key(key, sizeof(key), entity_name, id) != 0) {
        return EXIT_IO;
    }

    if (strcmp(action, "get") == 0) {
        return entity_get(store, key);
    }
    if (strcmp(action, "update") == 0) {
        char* patch_probe = NULL;
        char* final_json = NULL;
        char* old_json;
        if (json == NULL) {
            fprintf(stderr, "error: --json is required for update\n");
            return EXIT_USAGE;
        }
        int probe_rc = json_with_forced_id(json, id, 1, &patch_probe);
        free(patch_probe);
        if (probe_rc == -2) {
            fprintf(stderr, "error: --id and json.id mismatch for update\n");
            return EXIT_USAGE;
        }
        if (probe_rc != 0) {
            fprintf(stderr, "error: invalid json for update\n");
            return EXIT_USAGE;
        }
        old_json = api_qs_kvs_get(store, key);
        if (old_json == NULL) {
            fprintf(stderr, "error: key not found: %s\n", key);
            return EXIT_NOT_FOUND;
        }
        const char* schema_file = (strcmp(entity_name, "user") == 0) ? "schema_user.json" : "schema_label.json";
        if (build_merged_update_json_with_schema(entity_name, stores->base_dir, schema_file, old_json, json, &final_json) != 0) {
            fprintf(stderr, "error: invalid json for update\n");
            return EXIT_USAGE;
        }
        if (json_with_forced_id(final_json, id, 0, &patch_probe) != 0) {
            free(final_json);
            fprintf(stderr, "error: invalid json for update\n");
            return EXIT_USAGE;
        }
        free(final_json);
        final_json = patch_probe;
        patch_probe = NULL;
        int rc = entity_update(stores, store, cfg, key, final_json);
        free(final_json);
        return rc;
    }
    if (strcmp(action, "delete") == 0) {
        return entity_delete(store, key);
    }
    if (strcmp(action, "unset") == 0) {
        char* old_json;
        char* schema_json = NULL;
        char* final_json = NULL;
        const char* schema_file = (strcmp(entity_name, "user") == 0) ? "schema_user.json" : "schema_label.json";
        if (field == NULL) {
            fprintf(stderr, "error: --field is required for unset\n");
            return EXIT_USAGE;
        }
        old_json = api_qs_kvs_get(store, key);
        if (old_json == NULL) {
            fprintf(stderr, "error: key not found: %s\n", key);
            return EXIT_NOT_FOUND;
        }
        if (build_unset_json_with_schema(entity_name, stores->base_dir, schema_file, old_json, field, &schema_json) != 0) {
            fprintf(stderr, "error: failed to unset field: %s\n", field);
            return EXIT_USAGE;
        }
        if (json_with_forced_id(schema_json, id, 0, &final_json) != 0) {
            free(schema_json);
            fprintf(stderr, "error: invalid json for unset\n");
            return EXIT_USAGE;
        }
        free(schema_json);
        int rc = entity_update(stores, store, cfg, key, final_json);
        free(final_json);
        return rc;
    }

    fprintf(stderr, "error: unsupported action for %s: %s\n", entity_name, action);
    return EXIT_USAGE;
}

static int command_issue(int argc, char* argv[], APP_STORES* stores)
{
    if (argc < 3) {
        return EXIT_USAGE;
    }
    const char* action = argv[2];
    const char* id = get_option_value(argc, argv, "--id");
    const char* json = get_option_value(argc, argv, "--json");
    const char* field = get_option_value(argc, argv, "--field");

    if (strcmp(action, "list") == 0) {
        const char* status = get_option_value(argc, argv, "--status");
        const char* assignee = get_option_value(argc, argv, "--assignee");
        const char* label = get_option_value(argc, argv, "--label");
        int filters = (status != NULL) + (assignee != NULL) + (label != NULL);
        char index_key[1024];

        if (filters == 0) {
            return issue_list_all(stores);
        }
        if (filters > 1) {
            fprintf(stderr, "error: list supports only one filter at once\n");
            return EXIT_USAGE;
        }
        if (status != NULL) {
            snprintf(index_key, sizeof(index_key), "index:status:%s", status);
            return issue_list_by_index(stores, index_key);
        }
        if (assignee != NULL) {
            snprintf(index_key, sizeof(index_key), "index:assignee:%s", assignee);
            return issue_list_by_index(stores, index_key);
        }
        snprintf(index_key, sizeof(index_key), "index:label:%s", label);
        return issue_list_by_index(stores, index_key);
    }

    if (strcmp(action, "create") == 0) {
        char generated_id[128];
        char issue_seed_json[256];
        const char* final_id = id;
        char* id_probe = NULL;
        char* final_json = NULL;
        if (json == NULL) {
            fprintf(stderr, "error: --json is required for create\n");
            return EXIT_USAGE;
        }
        if (final_id == NULL) {
            if (next_counter_id(stores, "counter:issue", "iss_", generated_id, sizeof(generated_id)) != 0) {
                fprintf(stderr, "error: failed to generate issue id\n");
                return EXIT_KVS;
            }
            final_id = generated_id;
            printf("generated id: %s\n", final_id);
        }
        if (id != NULL) {
            int probe_rc = json_with_forced_id(json, final_id, 1, &id_probe);
            free(id_probe);
            if (probe_rc == -2) {
                fprintf(stderr, "error: --id and json.id mismatch for issue create\n");
                return EXIT_USAGE;
            }
            if (probe_rc != 0) {
                fprintf(stderr, "error: invalid json for issue create\n");
                return EXIT_USAGE;
            }
        }
        if (snprintf(issue_seed_json, sizeof(issue_seed_json), "{\"id\":\"%s\"}", final_id) <= 0) {
            return EXIT_IO;
        }
        if (build_merged_update_json_with_schema("issue", stores->base_dir, "schema_issue.json", issue_seed_json, json, &final_json) != 0) {
            fprintf(stderr, "error: schema validation failed for issue create\n");
            return EXIT_USAGE;
        }
        int rc = issue_create(stores, final_id, final_json);
        free(final_json);
        return rc;
    }

    if (id == NULL) {
        fprintf(stderr, "error: --id is required\n");
        return EXIT_USAGE;
    }
    if (strcmp(action, "get") == 0) {
        char key[1024];
        if (build_entity_key(key, sizeof(key), "issue", id) != 0) {
            return EXIT_IO;
        }
        return entity_get(&stores->issue, key);
    }
    if (strcmp(action, "update") == 0) {
        char key[1024];
        char* old_json;
        char* patch_probe = NULL;
        char* final_json = NULL;
        if (json == NULL) {
            fprintf(stderr, "error: --json is required for update\n");
            return EXIT_USAGE;
        }
        int probe_rc = json_with_forced_id(json, id, 1, &patch_probe);
        free(patch_probe);
        if (probe_rc == -2) {
            fprintf(stderr, "error: --id and json.id mismatch for issue update\n");
            return EXIT_USAGE;
        }
        if (probe_rc != 0) {
            fprintf(stderr, "error: invalid json for issue update\n");
            return EXIT_USAGE;
        }
        if (build_entity_key(key, sizeof(key), "issue", id) != 0) {
            return EXIT_IO;
        }
        old_json = api_qs_kvs_get(&stores->issue, key);
        if (old_json == NULL) {
            fprintf(stderr, "error: issue not found: %s\n", key);
            return EXIT_NOT_FOUND;
        }
        if (build_merged_update_json_with_schema("issue", stores->base_dir, "schema_issue.json", old_json, json, &final_json) != 0) {
            fprintf(stderr, "error: invalid json for issue update\n");
            return EXIT_USAGE;
        }
        int json_rc = json_with_forced_id(final_json, id, 0, &patch_probe);
        free(final_json);
        final_json = NULL;
        if (json_rc != 0) {
            fprintf(stderr, "error: invalid json for issue update\n");
            return EXIT_USAGE;
        }
        final_json = patch_probe;
        patch_probe = NULL;
        int rc = issue_update(stores, id, final_json);
        free(final_json);
        return rc;
    }
    if (strcmp(action, "delete") == 0) {
        return issue_delete(stores, id);
    }
    if (strcmp(action, "unset") == 0) {
        char key[1024];
        char* old_json;
        char* schema_json = NULL;
        char* final_json = NULL;
        if (field == NULL) {
            fprintf(stderr, "error: --field is required for unset\n");
            return EXIT_USAGE;
        }
        if (build_entity_key(key, sizeof(key), "issue", id) != 0) {
            return EXIT_IO;
        }
        old_json = api_qs_kvs_get(&stores->issue, key);
        if (old_json == NULL) {
            fprintf(stderr, "error: issue not found: %s\n", key);
            return EXIT_NOT_FOUND;
        }
        if (build_unset_json_with_schema("issue", stores->base_dir, "schema_issue.json", old_json, field, &schema_json) != 0) {
            fprintf(stderr, "error: failed to unset issue field: %s\n", field);
            return EXIT_USAGE;
        }
        if (json_with_forced_id(schema_json, id, 0, &final_json) != 0) {
            free(schema_json);
            fprintf(stderr, "error: invalid json for issue unset\n");
            return EXIT_USAGE;
        }
        free(schema_json);
        int rc = issue_update(stores, id, final_json);
        free(final_json);
        return rc;
    }

    fprintf(stderr, "error: unsupported issue action: %s\n", action);
    return EXIT_USAGE;
}

static int command_compact(int argc, char* argv[])
{
    const char* base_dir = DEFAULT_BASE_DIR;
    const char* base_dir_arg = get_option_value(argc, argv, "--base-dir");
    const char* store_arg = get_option_value(argc, argv, "--store");
    if (base_dir_arg != NULL) {
        base_dir = base_dir_arg;
    }
    if (store_arg == NULL) {
        store_arg = "all";
    }

    if (strcmp(store_arg, "all") == 0) {
        const STORE_CONFIG* stores[] = {&STORE_USER, &STORE_LABEL, &STORE_ISSUE, &STORE_INDEX, &STORE_SYSTEM};
        size_t i;
        for (i = 0; i < sizeof(stores) / sizeof(stores[0]); i++) {
            if (compact_store(stores[i], base_dir) != 0) {
                return EXIT_KVS;
            }
        }
        return EXIT_OK;
    }
    if (strcmp(store_arg, "user") == 0) {
        return (compact_store(&STORE_USER, base_dir) == 0) ? EXIT_OK : EXIT_KVS;
    }
    if (strcmp(store_arg, "label") == 0) {
        return (compact_store(&STORE_LABEL, base_dir) == 0) ? EXIT_OK : EXIT_KVS;
    }
    if (strcmp(store_arg, "issue") == 0) {
        return (compact_store(&STORE_ISSUE, base_dir) == 0) ? EXIT_OK : EXIT_KVS;
    }
    if (strcmp(store_arg, "index") == 0) {
        return (compact_store(&STORE_INDEX, base_dir) == 0) ? EXIT_OK : EXIT_KVS;
    }
    if (strcmp(store_arg, "system") == 0) {
        return (compact_store(&STORE_SYSTEM, base_dir) == 0) ? EXIT_OK : EXIT_KVS;
    }
    fprintf(stderr, "error: invalid --store value: %s\n", store_arg);
    return EXIT_USAGE;
}

static int command_schema(int argc, char* argv[])
{
    if (argc < 3) {
        fprintf(stderr, "error: schema action is required\n");
        return EXIT_USAGE;
    }
    const char* action = argv[2];
    const char* base_dir = DEFAULT_BASE_DIR;
    const char* base_dir_arg = get_option_value(argc, argv, "--base-dir");
    if (base_dir_arg != NULL) {
        base_dir = base_dir_arg;
    }

    if (strcmp(action, "list") == 0) {
        printf("{\"schemas\":[");
        printf("{\"entity\":\"user\",\"file\":\"schema_user.json\"},");
        printf("{\"entity\":\"label\",\"file\":\"schema_label.json\"},");
        printf("{\"entity\":\"issue\",\"file\":\"schema_issue.json\"}");
        printf("]}\n");
        return EXIT_OK;
    }

    if (strcmp(action, "get") == 0) {
        char path[4096];
        char* schema_text = NULL;
        const char* entity_name = get_option_value(argc, argv, "--entity");
        if (entity_name == NULL) {
            fprintf(stderr, "error: --entity is required for schema get\n");
            return EXIT_USAGE;
        }
        const char* file_name = schema_file_name_from_entity(entity_name);
        if (file_name == NULL) {
            fprintf(stderr, "error: invalid --entity value: %s\n", entity_name);
            return EXIT_USAGE;
        }
        if (build_store_path(path, sizeof(path), base_dir, file_name) != 0) {
            return EXIT_IO;
        }
        if (read_text_file(path, &schema_text) != 0) {
            fprintf(stderr, "error: schema file not found or unreadable: %s\n", path);
            return EXIT_IO;
        }
        printf("%s", schema_text);
        free(schema_text);
        return EXIT_OK;
    }

    fprintf(stderr, "error: unsupported schema action: %s\n", action);
    return EXIT_USAGE;
}

int main(int argc, char* argv[])
{
    api_qs_init();

    if (argc < 2) {
        print_usage(argv[0]);
        return EXIT_USAGE;
    }

    if (strcmp(argv[1], "help") == 0 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        print_usage(argv[0]);
        return EXIT_OK;
    }

    if (strcmp(argv[1], "init") == 0) {
        return command_init(argc, argv);
    }
    if (strcmp(argv[1], "compact") == 0) {
        return command_compact(argc, argv);
    }
    if (strcmp(argv[1], "schema") == 0) {
        return command_schema(argc, argv);
    }

    const char* base_dir = DEFAULT_BASE_DIR;
    const char* base_dir_arg = get_option_value(argc, argv, "--base-dir");
    if (base_dir_arg != NULL) {
        base_dir = base_dir_arg;
    }

    APP_STORES stores;
    if (open_all_stores(base_dir, &stores) != 0) {
        return EXIT_IO;
    }
    if (load_system_settings(base_dir, &stores) != 0) {
        close_all_stores(&stores);
        return EXIT_IO;
    }

    int rc = EXIT_USAGE;
    if (strcmp(argv[1], "user") == 0) {
        rc = command_entity_common(argc, argv, &stores, "user", "usr_", "counter:user", &stores.user, &STORE_USER);
    } else if (strcmp(argv[1], "label") == 0) {
        rc = command_entity_common(argc, argv, &stores, "label", "lbl_", "counter:label", &stores.label, &STORE_LABEL);
    } else if (strcmp(argv[1], "issue") == 0) {
        rc = command_issue(argc, argv, &stores);
    } else {
        fprintf(stderr, "error: unknown command: %s\n", argv[1]);
    }

    close_all_stores(&stores);
    if (rc == EXIT_USAGE) {
        print_usage(argv[0]);
    }
    return rc;
}
