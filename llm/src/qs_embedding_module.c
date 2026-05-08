/*
 * Copyright (c) Katsuya Owari
 *
 * Embedding module: generate embeddings via llama.cpp and store/search
 * vectors in SQLite using sqlite-vec (vec0 virtual table).
 *
 * Build with: make QS_EMBEDDING_MODULE_ENABLED=1
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>

#include "qs_embedding_module.h"

/* Stub implementation when module is disabled. */
#if !QS_EMBEDDING_MODULE_ENABLED || !defined(QS_EMBEDDING_WITH_LLAMA_CPP) || QS_EMBEDDING_WITH_LLAMA_CPP != 1

int qs_embedding_prepare(const char* model_path, const char* db_path, QS_EMBEDDING_STORE** out_store) { (void)model_path; (void)db_path; if (out_store) *out_store = NULL; return 0; }
void qs_embedding_shutdown(QS_EMBEDDING_STORE* store) { (void)store; }
int qs_embedding_store(QS_EMBEDDING_STORE* store, const char* text, const char* content, int64_t id) { (void)store; (void)text; (void)content; (void)id; return 0; }
int qs_embedding_search(QS_EMBEDDING_STORE* store, const char* query, int top_k, int64_t* out_ids, float* out_scores, char** out_texts, int max_results) { (void)store; (void)query; (void)top_k; (void)out_ids; (void)out_scores; (void)out_texts; (void)max_results; return 0; }
int qs_embedding_delete(QS_EMBEDDING_STORE* store, int64_t id) { (void)store; (void)id; return 0; }
int qs_embedding_n_embd(QS_EMBEDDING_STORE* store) { (void)store; return 0; }

#else

#include <sqlite3.h>
#include <llama.h>

/* Forward declaration for sqlite-vec init */
int sqlite3_vec_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi);

#define EMBEDDING_TABLE "embedding_vecs"
#define SQLITE_MAX_VARCHAR 65536

typedef float (*QS_EMBEDDING_DISTANCE_FN)(const float* v1, const float* v2, int n);

typedef struct {
    const char* name;
    const char* query_prefix;
    const char* document_prefix;
    enum llama_pooling_type pooling_type;
    QS_EMBEDDING_DISTANCE_FN distance_fn;
} QS_EMBEDDING_PROFILE;

typedef struct {
    struct llama_model* model;
    char model_path[1024];
    int n_embd_out;
    int backend_initialized;
    int ref_count;
    pthread_mutex_t mutex;
} QS_EMBEDDING_RUNTIME;

struct QS_EMBEDDING_STORE {
    char db_path[1024];
    sqlite3* db;
    int n_embd_out;
    pthread_mutex_t mutex;
    int mutex_initialized;
};

static QS_EMBEDDING_RUNTIME g_embedding_runtime = {
    NULL,
    {0},
    0,
    0,
    0,
    PTHREAD_MUTEX_INITIALIZER
};

static float cosine_distance(const float* v1, const float* v2, int n);

static const QS_EMBEDDING_PROFILE g_embedding_profile_embeddinggemma = {
    "embeddinggemma",
    "task: search result | query: ",
    "title: none | text: ",
    LLAMA_POOLING_TYPE_MEAN,
    cosine_distance
};

static const QS_EMBEDDING_PROFILE* g_embedding_profile = &g_embedding_profile_embeddinggemma;

static char* vec_to_json(const float* v, int n, int* out_len) {
    const int est_per_elem = 20;
    const size_t buf_size = (size_t)n * est_per_elem + 4;
    if (buf_size > SQLITE_MAX_VARCHAR) {
        fprintf(stderr, "embedding: vector too large for JSON string (%d dims)\n", n);
        return NULL;
    }

    char* buf = (char*)malloc(buf_size);
    if (!buf) return NULL;

    int pos = 0;
    pos += snprintf(buf + pos, buf_size - (size_t)pos, "[");
    for (int i = 0; i < n; i++) {
        if (i > 0) {
            pos += snprintf(buf + pos, buf_size - (size_t)pos, ",");
        }
        pos += snprintf(buf + pos, buf_size - (size_t)pos, "%.7g", v[i]);
    }
    pos += snprintf(buf + pos, buf_size - (size_t)pos, "]");

    if (out_len) {
        *out_len = pos;
    }
    return buf;
}

static int create_table(sqlite3* db) {
    const char* sql = "CREATE TABLE IF NOT EXISTS " EMBEDDING_TABLE
                      " (id INTEGER PRIMARY KEY, "
                      "  embedding TEXT NOT NULL, "
                      "  text_content TEXT DEFAULT NULL, "
                      "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP)";

    char* err = NULL;
    int rc = sqlite3_exec(db, sql, NULL, NULL, &err);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "embedding: CREATE TABLE failed: %s\n", err ? err : "(unknown)");
        sqlite3_free(err);
        return -1;
    }

    sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS " EMBEDDING_TABLE "_id_idx ON " EMBEDDING_TABLE "(id)", NULL, NULL, NULL);
    return 0;
}

static void embedding_runtime_unload_model_locked(void) {
    if (g_embedding_runtime.model) {
        llama_model_free(g_embedding_runtime.model);
        g_embedding_runtime.model = NULL;
    }
    g_embedding_runtime.model_path[0] = '\0';
    g_embedding_runtime.n_embd_out = 0;
}

static int embedding_runtime_load_model_locked(const char* model_path) {
    if (!model_path || model_path[0] == '\0') {
        return -1;
    }

    if (g_embedding_runtime.model && strcmp(g_embedding_runtime.model_path, model_path) == 0) {
        return 0;
    }

    if (g_embedding_runtime.ref_count > 0 && g_embedding_runtime.model && strcmp(g_embedding_runtime.model_path, model_path) != 0) {
        fprintf(stderr, "embedding: model path mismatch while stores are active\n");
        return -1;
    }

    if (!g_embedding_runtime.backend_initialized) {
        llama_backend_init();
        g_embedding_runtime.backend_initialized = 1;
    }

    embedding_runtime_unload_model_locked();

    fprintf(stderr, "embedding: loading model '%s'\n", model_path);
    g_embedding_runtime.model = llama_model_load_from_file(model_path, llama_model_default_params());
    if (!g_embedding_runtime.model) {
        fprintf(stderr, "embedding: failed to load model\n");
        return -1;
    }

    snprintf(g_embedding_runtime.model_path, sizeof(g_embedding_runtime.model_path), "%s", model_path);
    g_embedding_runtime.n_embd_out = (int)llama_model_n_embd_out(g_embedding_runtime.model);
    fprintf(stderr, "embedding: model loaded, profile=%s, n_embd_out=%d\n",
            g_embedding_profile->name, g_embedding_runtime.n_embd_out);
    return 0;
}

static int embedding_runtime_acquire(const char* model_path, int* out_n_embd) {
    int rc;

    pthread_mutex_lock(&g_embedding_runtime.mutex);
    rc = embedding_runtime_load_model_locked(model_path);
    if (rc == 0) {
        g_embedding_runtime.ref_count++;
        if (out_n_embd) {
            *out_n_embd = g_embedding_runtime.n_embd_out;
        }
    } else if (out_n_embd) {
        *out_n_embd = 0;
    }
    pthread_mutex_unlock(&g_embedding_runtime.mutex);

    return rc;
}

static void embedding_runtime_release(void) {
    pthread_mutex_lock(&g_embedding_runtime.mutex);

    if (g_embedding_runtime.ref_count > 0) {
        g_embedding_runtime.ref_count--;
    }
    if (g_embedding_runtime.ref_count == 0) {
        embedding_runtime_unload_model_locked();
        if (g_embedding_runtime.backend_initialized) {
            llama_backend_free();
            g_embedding_runtime.backend_initialized = 0;
        }
    }

    pthread_mutex_unlock(&g_embedding_runtime.mutex);
}

static char* build_embedding_input(const char* prefix, const char* text) {
    const char* safe_text = text ? text : "";
    size_t prefix_len = strlen(prefix);
    size_t text_len = strlen(safe_text);
    char* input = (char*)malloc(prefix_len + text_len + 1);

    if (!input) {
        return NULL;
    }

    memcpy(input, prefix, prefix_len);
    memcpy(input + prefix_len, safe_text, text_len + 1);
    return input;
}

static float* generate_embedding_locked(const char* text) {
    struct llama_model* model = g_embedding_runtime.model;
    int32_t n_embd_out = (int32_t)g_embedding_runtime.n_embd_out;
    struct llama_context_params cparams = llama_context_default_params();

    cparams.n_ctx = 0;
    cparams.pooling_type = g_embedding_profile->pooling_type;
    cparams.embeddings = true;

    if (!model) {
        return NULL;
    }

    struct llama_context* ctx = llama_init_from_model(model, cparams);
    if (!ctx) {
        fprintf(stderr, "embedding: failed to create context\n");
        return NULL;
    }

    int32_t n_ctx = llama_n_ctx(ctx);
    llama_token* tokens = (llama_token*)malloc(sizeof(llama_token) * (size_t)n_ctx);
    if (!tokens) {
        llama_free(ctx);
        return NULL;
    }

    int32_t n_tokens = llama_tokenize(llama_model_get_vocab(model), text, strlen(text), tokens, n_ctx, true, true);
    if (n_tokens < 0) {
        fprintf(stderr, "embedding: tokenization failed (%d)\n", n_tokens);
        free(tokens);
        llama_free(ctx);
        return NULL;
    }

    struct llama_batch batch = llama_batch_get_one(tokens, (int32_t)n_tokens);
    if (llama_decode(ctx, batch) != 0) {
        fprintf(stderr, "embedding: decode failed\n");
        free(tokens);
        llama_free(ctx);
        return NULL;
    }

    float* embd = llama_get_embeddings_seq(ctx, 0);
    if (!embd) {
        fprintf(stderr, "embedding: no embeddings returned\n");
        free(tokens);
        llama_free(ctx);
        return NULL;
    }

    float* result = (float*)malloc(sizeof(float) * (size_t)n_embd_out);
    if (result) {
        memcpy(result, embd, sizeof(float) * (size_t)n_embd_out);
    }

    free(tokens);
    llama_free(ctx);
    return result;
}

static float* generate_prefixed_embedding_locked(const char* prefix, const char* text) {
    char* input = build_embedding_input(prefix, text);
    float* result;

    if (!input) {
        return NULL;
    }

    result = generate_embedding_locked(input);
    free(input);
    return result;
}

static float* generate_query_embedding_locked(const char* query) {
    return generate_prefixed_embedding_locked(g_embedding_profile->query_prefix, query);
}

static float* generate_document_embedding_locked(const char* text) {
    return generate_prefixed_embedding_locked(g_embedding_profile->document_prefix, text);
}

static int store_vector(sqlite3* db, int64_t id, const float* embd, int n_embd, const char* text_content) {
    char* vec_str = vec_to_json(embd, n_embd, NULL);
    if (!vec_str) return -1;

    const char* sql = "INSERT OR REPLACE INTO " EMBEDDING_TABLE "(id, embedding, text_content) VALUES (?, ?, ?)";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        free(vec_str);
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, id);
    sqlite3_bind_text(stmt, 2, vec_str, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, text_content ? text_content : "", -1, SQLITE_TRANSIENT);

    rc = (sqlite3_step(stmt) == SQLITE_DONE) ? 0 : -1;
    sqlite3_finalize(stmt);
    free(vec_str);
    return rc;
}

static float* json_array_to_floats(const char* json_str, int expected_len, int* out_len) {
    if (!json_str) return NULL;

    float* result = (float*)malloc(sizeof(float) * (size_t)expected_len);
    if (!result) return NULL;

    int count = 0;
    const char* p = json_str;
    while (*p && *p != '[') p++;
    if (!*p) { free(result); return NULL; }
    p++;

    while (count < expected_len && *p) {
        while (*p && (*p == ' ' || *p == ',' || *p == '\t' || *p == '\n')) p++;
        if (*p == ']') break;

        char* endp;
        result[count] = strtof(p, &endp);
        if (endp == p) break;
        count++;
        p = endp;
    }

    *out_len = count;
    if (count != expected_len) {
        free(result);
        return NULL;
    }
    return result;
}

static float cosine_distance(const float* v1, const float* v2, int n) {
    if (n <= 0) return 1.0f;

    float dot = 0.0f;
    float norm1 = 0.0f;
    float norm2 = 0.0f;
    for (int i = 0; i < n; i++) {
        dot += v1[i] * v2[i];
        norm1 += v1[i] * v1[i];
        norm2 += v2[i] * v2[i];
    }

    if (norm1 < 1e-6f || norm2 < 1e-6f) return 1.0f;
    return 1.0f - (dot / (sqrtf(norm1) * sqrtf(norm2)));
}

static int search_vectors(sqlite3* db, const float* query_embd, int n_embd,
                          int top_k, int64_t* out_ids, float* out_scores, char** out_texts, int max_results) {
    const char* sql = "SELECT id, embedding, text_content FROM " EMBEDDING_TABLE " ORDER BY id";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return 0;
    }

    typedef struct {
        int64_t id;
        float distance;
        char* text_content;
    } SearchResult;

    int capacity = 100;
    SearchResult* results = (SearchResult*)malloc(sizeof(SearchResult) * (size_t)capacity);
    if (!results) {
        sqlite3_finalize(stmt);
        return 0;
    }

    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int64_t id = sqlite3_column_int64(stmt, 0);
        const char* emb_json = (const char*)sqlite3_column_text(stmt, 1);
        if (!emb_json) continue;

        int parsed_len = 0;
        float* embd = json_array_to_floats(emb_json, n_embd, &parsed_len);
        if (!embd || parsed_len != n_embd) {
            free(embd);
            continue;
        }

        float distance = g_embedding_profile->distance_fn(query_embd, embd, n_embd);
        free(embd);

        if (count >= capacity) {
            capacity *= 2;
            SearchResult* new_results = (SearchResult*)realloc(results, sizeof(SearchResult) * (size_t)capacity);
            if (!new_results) {
                for (int i = 0; i < count; i++) {
                    free(results[i].text_content);
                }
                free(results);
                sqlite3_finalize(stmt);
                return 0;
            }
            results = new_results;
        }

        results[count].id = id;
        results[count].distance = distance;
        const char* txt = (const char*)sqlite3_column_text(stmt, 2);
        results[count].text_content = txt ? strdup(txt) : NULL;
        count++;
    }

    sqlite3_finalize(stmt);

    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (results[j].distance < results[i].distance) {
                SearchResult tmp = results[i];
                results[i] = results[j];
                results[j] = tmp;
            }
        }
    }

    int n = (count < max_results) ? count : max_results;
    if (n > top_k) n = top_k;
    for (int i = 0; i < n; i++) {
        out_ids[i] = results[i].id;
        out_scores[i] = results[i].distance;
        out_texts[i] = results[i].text_content;
    }
    for (int i = n; i < count; i++) {
        free(results[i].text_content);
    }

    free(results);
    return n;
}

int qs_embedding_prepare(const char* model_path, const char* db_path, QS_EMBEDDING_STORE** out_store) {
    QS_EMBEDDING_STORE* store;
    int n_embd_out = 0;
    int rc;

    if (!out_store) {
        return -1;
    }
    *out_store = NULL;

    if (!db_path || db_path[0] == '\0') {
        return -1;
    }

    if (embedding_runtime_acquire(model_path, &n_embd_out) != 0) {
        return -1;
    }

    store = (QS_EMBEDDING_STORE*)calloc(1, sizeof(*store));
    if (!store) {
        embedding_runtime_release();
        return -1;
    }

    snprintf(store->db_path, sizeof(store->db_path), "%s", db_path);
    store->n_embd_out = n_embd_out;

    rc = sqlite3_open(db_path, &store->db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "embedding: cannot open database: %s\n", sqlite3_errmsg(store->db));
        if (store->db) {
            sqlite3_close(store->db);
        }
        free(store);
        embedding_runtime_release();
        return -1;
    }

    sqlite3_exec(store->db, "PRAGMA journal_mode=WAL", NULL, NULL, NULL);
    if (create_table(store->db) != 0) {
        sqlite3_close(store->db);
        free(store);
        embedding_runtime_release();
        return -1;
    }

    pthread_mutex_init(&store->mutex, NULL);
    store->mutex_initialized = 1;
    *out_store = store;
    fprintf(stderr, "embedding: store prepared successfully: db=%s\n", db_path);
    return 0;
}

void qs_embedding_shutdown(QS_EMBEDDING_STORE* store) {
    sqlite3* db;

    if (!store) return;

    if (store->mutex_initialized) {
        pthread_mutex_lock(&store->mutex);
    }

    db = store->db;
    store->db = NULL;
    store->n_embd_out = 0;

    if (store->mutex_initialized) {
        pthread_mutex_unlock(&store->mutex);
        pthread_mutex_destroy(&store->mutex);
        store->mutex_initialized = 0;
    }

    if (db) {
        sqlite3_close(db);
    }

    embedding_runtime_release();
    free(store);
}

int qs_embedding_store(QS_EMBEDDING_STORE* store, const char* text, const char* content, int64_t id) {
    float* embd;
    int rc;
    const char* source_text;

    if (!store || !store->db) return -1;

    source_text = (text && text[0] != '\0') ? text : content;

    pthread_mutex_lock(&store->mutex);
    pthread_mutex_lock(&g_embedding_runtime.mutex);
    embd = generate_document_embedding_locked(source_text);
    pthread_mutex_unlock(&g_embedding_runtime.mutex);
    if (!embd) {
        pthread_mutex_unlock(&store->mutex);
        return -1;
    }

    rc = store_vector(store->db, id, embd, store->n_embd_out, content);
    free(embd);
    pthread_mutex_unlock(&store->mutex);
    return rc;
}

int qs_embedding_search(QS_EMBEDDING_STORE* store, const char* query, int top_k, int64_t* out_ids,
                        float* out_scores, char** out_texts, int max_results) {
    float* query_embd;
    int count;

    if (!store || !store->db) return 0;

    pthread_mutex_lock(&store->mutex);
    pthread_mutex_lock(&g_embedding_runtime.mutex);
    query_embd = generate_query_embedding_locked(query);
    pthread_mutex_unlock(&g_embedding_runtime.mutex);
    if (!query_embd) {
        pthread_mutex_unlock(&store->mutex);
        return 0;
    }

    count = search_vectors(store->db, query_embd, store->n_embd_out, top_k, out_ids, out_scores, out_texts, max_results);
    pthread_mutex_unlock(&store->mutex);
    free(query_embd);
    return count;
}

int qs_embedding_delete(QS_EMBEDDING_STORE* store, int64_t id) {
    const char* sql = "DELETE FROM " EMBEDDING_TABLE " WHERE id = ?";
    sqlite3_stmt* stmt;
    int rc;

    if (!store || !store->db) return -1;

    pthread_mutex_lock(&store->mutex);
    rc = sqlite3_prepare_v2(store->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&store->mutex);
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, id);
    rc = (sqlite3_step(stmt) == SQLITE_DONE) ? 0 : -1;
    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&store->mutex);
    return rc;
}

int qs_embedding_n_embd(QS_EMBEDDING_STORE* store) {
    if (!store) {
        return 0;
    }
    return store->n_embd_out;
}

#endif /* QS_EMBEDDING_MODULE_ENABLED */
