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

#include "qs_api.h"

#define EXIT_OK 0
#define EXIT_USAGE 2
#define EXIT_IO 3
#define EXIT_KVS 4
#define EXIT_NOT_FOUND 5

#define DEFAULT_BASE_DIR "./issue_data"
#define INDEX_JSON_BUFFER_SIZE (64 * 1024)

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
} APP_STORES;

static const STORE_CONFIG STORE_USER = {"user", "user.db", api_qs_kvs_create_b1mb_persistence};
static const STORE_CONFIG STORE_LABEL = {"label", "label.db", api_qs_kvs_create_b1mb_persistence};
static const STORE_CONFIG STORE_ISSUE = {"issue", "issue.db", api_qs_kvs_create_b8mb_persistence};
static const STORE_CONFIG STORE_INDEX = {"index", "issue_index.db", api_qs_kvs_create_b1mb_persistence};

static void print_usage(const char* program_name)
{
    printf("Usage:\n");
    printf("  %s init [--base-dir <dir>] [--force]\n", program_name);
    printf("  %s user  <create|get|update|delete> --id <id> [--json <json>] [--base-dir <dir>]\n", program_name);
    printf("  %s user  list [--base-dir <dir>]\n", program_name);
    printf("  %s label <create|get|update|delete> --id <id> [--json <json>] [--base-dir <dir>]\n", program_name);
    printf("  %s label list [--base-dir <dir>]\n", program_name);
    printf("  %s issue <create|get|update|delete> --id <id> [--json <json>] [--base-dir <dir>]\n", program_name);
    printf("  %s issue list [--status <status> | --assignee <userId> | --label <label>] [--base-dir <dir>]\n", program_name);
    printf("  %s compact [--store user|label|issue|index|all] [--base-dir <dir>]\n", program_name);
    printf("\n");
    printf("Examples:\n");
    printf("  %s init --base-dir ./issue_data\n", program_name);
    printf("  %s issue create --id iss_1 --json '{\"title\":\"t\",\"status\":\"open\",\"assigneeIds\":[\"u1\"],\"labelIds\":[\"bug\"]}'\n", program_name);
    printf("  %s issue list --status open\n", program_name);
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
    return 0;
}

static void close_all_stores(APP_STORES* stores)
{
    api_qs_persistence_kvs_memory_free(&stores->user);
    api_qs_persistence_kvs_memory_free(&stores->label);
    api_qs_persistence_kvs_memory_free(&stores->issue);
    api_qs_persistence_kvs_memory_free(&stores->index);
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

static int entity_create(QS_KVS_CONTEXT* store, const char* key, const char* json)
{
    if (api_qs_kvs_get(store, key) != NULL) {
        fprintf(stderr, "error: key already exists: %s\n", key);
        return EXIT_KVS;
    }
    if (-1 == api_qs_kvs_set(store, key, json, 0)) {
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

static int entity_update(QS_KVS_CONTEXT* store, const char* key, const char* json)
{
    if (api_qs_kvs_get(store, key) == NULL) {
        fprintf(stderr, "error: key not found: %s\n", key);
        return EXIT_NOT_FOUND;
    }
    if (-1 == api_qs_kvs_set(store, key, json, 0)) {
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

static int index_add_id(QS_KVS_CONTEXT* index_store, const char* index_key, const char* issue_id)
{
    QS_MEMORY_CONTEXT context;
    if (create_temp_memory(&context, 1024 * 1024) != 0) {
        return -1;
    }

    char* old_value = api_qs_kvs_get(index_store, index_key);
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

    int rc = api_qs_kvs_set(index_store, index_key, encoded, 0);
    api_qs_memory_free(&context);
    return (rc == -1) ? -1 : 0;
}

static int index_remove_id(QS_KVS_CONTEXT* index_store, const char* index_key, const char* issue_id)
{
    char* old_value = api_qs_kvs_get(index_store, index_key);
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
        rc = api_qs_kvs_delete(index_store, index_key);
    } else {
        char* encoded = api_qs_json_encode_array(&filtered, INDEX_JSON_BUFFER_SIZE);
        if (encoded == NULL) {
            api_qs_memory_free(&context);
            return -1;
        }
        rc = api_qs_kvs_set(index_store, index_key, encoded, 0);
    }

    api_qs_memory_free(&context);
    return (rc == -1) ? -1 : 0;
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

typedef struct ISSUE_INDEX_DATA {
    QS_KVS_CONTEXT* index_store;
    const char* issue_id;
} ISSUE_INDEX_DATA;

static int cb_add_assignee_index(const char* value, void* user_data)
{
    ISSUE_INDEX_DATA* d = (ISSUE_INDEX_DATA*)user_data;
    char key[1024];
    if (snprintf(key, sizeof(key), "index:assignee:%s", value) <= 0) {
        return -1;
    }
    return index_add_id(d->index_store, key, d->issue_id);
}

static int cb_add_label_index(const char* value, void* user_data)
{
    ISSUE_INDEX_DATA* d = (ISSUE_INDEX_DATA*)user_data;
    char key[1024];
    if (snprintf(key, sizeof(key), "index:label:%s", value) <= 0) {
        return -1;
    }
    return index_add_id(d->index_store, key, d->issue_id);
}

static int cb_remove_assignee_index(const char* value, void* user_data)
{
    ISSUE_INDEX_DATA* d = (ISSUE_INDEX_DATA*)user_data;
    char key[1024];
    if (snprintf(key, sizeof(key), "index:assignee:%s", value) <= 0) {
        return -1;
    }
    return index_remove_id(d->index_store, key, d->issue_id);
}

static int cb_remove_label_index(const char* value, void* user_data)
{
    ISSUE_INDEX_DATA* d = (ISSUE_INDEX_DATA*)user_data;
    char key[1024];
    if (snprintf(key, sizeof(key), "index:label:%s", value) <= 0) {
        return -1;
    }
    return index_remove_id(d->index_store, key, d->issue_id);
}

static int issue_apply_indexes(QS_KVS_CONTEXT* index_store, const char* issue_id, const char* issue_json, int is_add)
{
    QS_MEMORY_CONTEXT context;
    if (create_temp_memory(&context, 2 * 1024 * 1024) != 0) {
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
            if (index_add_id(index_store, status_key, issue_id) != 0) {
                api_qs_memory_free(&context);
                return -1;
            }
        } else {
            if (index_remove_id(index_store, status_key, issue_id) != 0) {
                api_qs_memory_free(&context);
                return -1;
            }
        }
    }

    ISSUE_INDEX_DATA data = {index_store, issue_id};
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
    if (-1 == api_qs_kvs_set(&stores->issue, key, issue_json, 0)) {
        fprintf(stderr, "error: failed to create issue: %s\n", key);
        return EXIT_KVS;
    }
    if (issue_apply_indexes(&stores->index, id, issue_json, 1) != 0) {
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

    char* old_copy = strdup(old_json);
    if (old_copy == NULL) {
        return EXIT_IO;
    }

    if (issue_apply_indexes(&stores->index, id, old_copy, 0) != 0) {
        free(old_copy);
        fprintf(stderr, "error: failed to remove old index on issue update\n");
        return EXIT_KVS;
    }
    if (-1 == api_qs_kvs_set(&stores->issue, key, issue_json, 0)) {
        issue_apply_indexes(&stores->index, id, old_copy, 1);
        free(old_copy);
        fprintf(stderr, "error: failed to update issue: %s\n", key);
        return EXIT_KVS;
    }
    if (issue_apply_indexes(&stores->index, id, issue_json, 1) != 0) {
        api_qs_kvs_set(&stores->issue, key, old_copy, 0);
        issue_apply_indexes(&stores->index, id, old_copy, 1);
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

    if (issue_apply_indexes(&stores->index, id, old_copy, 0) != 0) {
        free(old_copy);
        fprintf(stderr, "error: failed to remove issue index\n");
        return EXIT_KVS;
    }
    if (-1 == api_qs_kvs_delete(&stores->issue, key)) {
        issue_apply_indexes(&stores->index, id, old_copy, 1);
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

    const STORE_CONFIG* stores[] = {&STORE_USER, &STORE_LABEL, &STORE_ISSUE, &STORE_INDEX};
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
    printf("init done: base_dir=%s\n", base_dir);
    return EXIT_OK;
}

static int command_entity_common(int argc, char* argv[], const char* entity_name, QS_KVS_CONTEXT* store)
{
    if (argc < 3) {
        return EXIT_USAGE;
    }
    const char* action = argv[2];
    const char* id = get_option_value(argc, argv, "--id");
    const char* json = get_option_value(argc, argv, "--json");
    char key[1024];

    if (strcmp(action, "list") == 0) {
        return entity_list_all(store, entity_name);
    }

    if (id == NULL) {
        fprintf(stderr, "error: --id is required\n");
        return EXIT_USAGE;
    }
    if (build_entity_key(key, sizeof(key), entity_name, id) != 0) {
        return EXIT_IO;
    }

    if (strcmp(action, "create") == 0) {
        if (json == NULL) {
            fprintf(stderr, "error: --json is required for create\n");
            return EXIT_USAGE;
        }
        return entity_create(store, key, json);
    }
    if (strcmp(action, "get") == 0) {
        return entity_get(store, key);
    }
    if (strcmp(action, "update") == 0) {
        if (json == NULL) {
            fprintf(stderr, "error: --json is required for update\n");
            return EXIT_USAGE;
        }
        return entity_update(store, key, json);
    }
    if (strcmp(action, "delete") == 0) {
        return entity_delete(store, key);
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

    if (id == NULL) {
        fprintf(stderr, "error: --id is required\n");
        return EXIT_USAGE;
    }

    if (strcmp(action, "create") == 0) {
        if (json == NULL) {
            fprintf(stderr, "error: --json is required for create\n");
            return EXIT_USAGE;
        }
        return issue_create(stores, id, json);
    }
    if (strcmp(action, "get") == 0) {
        char key[1024];
        if (build_entity_key(key, sizeof(key), "issue", id) != 0) {
            return EXIT_IO;
        }
        return entity_get(&stores->issue, key);
    }
    if (strcmp(action, "update") == 0) {
        if (json == NULL) {
            fprintf(stderr, "error: --json is required for update\n");
            return EXIT_USAGE;
        }
        return issue_update(stores, id, json);
    }
    if (strcmp(action, "delete") == 0) {
        return issue_delete(stores, id);
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
        const STORE_CONFIG* stores[] = {&STORE_USER, &STORE_LABEL, &STORE_ISSUE, &STORE_INDEX};
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
    fprintf(stderr, "error: invalid --store value: %s\n", store_arg);
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

    const char* base_dir = DEFAULT_BASE_DIR;
    const char* base_dir_arg = get_option_value(argc, argv, "--base-dir");
    if (base_dir_arg != NULL) {
        base_dir = base_dir_arg;
    }

    APP_STORES stores;
    if (open_all_stores(base_dir, &stores) != 0) {
        return EXIT_IO;
    }

    int rc = EXIT_USAGE;
    if (strcmp(argv[1], "user") == 0) {
        rc = command_entity_common(argc, argv, "user", &stores.user);
    } else if (strcmp(argv[1], "label") == 0) {
        rc = command_entity_common(argc, argv, "label", &stores.label);
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
