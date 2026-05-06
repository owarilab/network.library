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

/* Stub implementation when module is disabled. */
#if !QS_EMBEDDING_MODULE_ENABLED || !defined(QS_EMBEDDING_WITH_LLAMA_CPP) || QS_EMBEDDING_WITH_LLAMA_CPP != 1

int qs_embedding_prepare(const char* model_path, const char* db_path) { (void)model_path; (void)db_path; return 0; }
void qs_embedding_shutdown(void) {}
int qs_embedding_store(const char* text, int64_t id) { (void)text; (void)id; return 0; }
int qs_embedding_search(const char* query, int top_k, int64_t* out_ids, float* out_scores, int max_results) { (void)query; (void)top_k; (void)out_ids; (void)out_scores; (void)max_results; return 0; }
int qs_embedding_delete(int64_t id) { (void)id; return 0; }
int qs_embedding_n_embd(void) { return 0; }

#else

#include <sqlite3.h>
#include <llama.h>

#define EMBEDDING_TABLE "embedding_vecs"
#define SQLITE_MAX_VARCHAR 65536

/* ── Global state ─────────────────────────────────────────────── */

static struct {
    const char* model_path;
    const char* db_path;
    struct llama_model* model;
    sqlite3* db;
    int n_embd_out;
    pthread_mutex_t mutex;
} g_emb;

/* ── Vector <-> JSON string helpers ───────────────────────────── */

static char* vec_to_json(const float* v, int n, int* out_len) {
    const int est_per_elem = 10;
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

    *out_len = pos;
    return buf;
}

/* ── Internal API ─────────────────────────────────────────────── */

static int create_table(sqlite3* db) {
    const char* sql = "CREATE VIRTUAL TABLE IF NOT EXISTS " EMBEDDING_TABLE
                      " USING vec0(embedding float[%d])";
    char buf[256];
    snprintf(buf, sizeof(buf), sql, g_emb.n_embd_out);

    char* err = NULL;
    int rc = sqlite3_exec(db, buf, NULL, NULL, &err);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "embedding: CREATE TABLE failed: %s\n", err ? err : "(unknown)");
        sqlite3_free(err);
        return -1;
    }
    return 0;
}

static float* generate_embedding(const char* text) {
    struct llama_context_params cparams = llama_context_default_params();
    cparams.pooling_type = LLAMA_POOLING_TYPE_MEAN;
    cparams.embeddings   = true;
    cparams.kv_unified = true;

    struct llama_context* ctx = llama_init_from_model(g_emb.model, cparams);
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

    int32_t n_tokens = llama_tokenize(llama_model_get_vocab(g_emb.model), text, strlen(text),
                                       tokens, n_ctx, true, true);
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

    int32_t n_embd_out = llama_model_n_embd_out(g_emb.model);

    float* result = (float*)malloc(sizeof(float) * (size_t)n_embd_out);
    if (result) {
        memcpy(result, embd, sizeof(float) * (size_t)n_embd_out);
    }

    free(tokens);
    llama_free(ctx);
    return result;
}

static int store_vector(sqlite3* db, int64_t id, const float* embd, int n_embd) {
    char* vec_str = vec_to_json(embd, n_embd, NULL);
    if (!vec_str) return -1;

    const char* sql = "INSERT INTO " EMBEDDING_TABLE "(rowid, embedding) VALUES (?, ?)";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        free(vec_str);
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, id);
    sqlite3_bind_text(stmt, 2, vec_str, -1, SQLITE_STATIC);

    rc = (sqlite3_step(stmt) == SQLITE_DONE) ? 0 : -1;
    sqlite3_finalize(stmt);
    free(vec_str);
    return rc;
}

static int search_vectors(sqlite3* db, const float* query_embd, int n_embd,
                          int top_k, int64_t* out_ids, float* out_scores, int max_results) {
    char* vec_str = vec_to_json(query_embd, n_embd, NULL);
    if (!vec_str) return 0;

    const char* sql =
        "SELECT rowid, distance FROM " EMBEDDING_TABLE
        " WHERE embedding MATCH ? ORDER BY distance LIMIT ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        free(vec_str);
        return 0;
    }

    sqlite3_bind_text(stmt, 1, vec_str, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, max_results);

    int count = 0;
    while (count < max_results && sqlite3_step(stmt) == SQLITE_ROW) {
        out_ids[count]   = sqlite3_column_int64(stmt, 0);
        out_scores[count]= (float)sqlite3_column_double(stmt, 1);
        count++;
    }

    sqlite3_finalize(stmt);
    free(vec_str);
    return count;
}

/* ── Public API ───────────────────────────────────────────────── */

int qs_embedding_prepare(const char* model_path, const char* db_path) {
    if (g_emb.db) {
        return 0;
    }

    memset(&g_emb, 0, sizeof(g_emb));
    g_emb.model_path = model_path;
    g_emb.db_path = db_path;

    fprintf(stderr, "embedding: loading model '%s'\n", model_path);
    llama_backend_init();
    g_emb.model = llama_model_load_from_file(model_path,
        llama_model_default_params());
    if (!g_emb.model) {
        fprintf(stderr, "embedding: failed to load model\n");
        llama_backend_free();
        return -1;
    }

    g_emb.n_embd_out = (int)llama_model_n_embd_out(g_emb.model);
    fprintf(stderr, "embedding: model loaded, n_embd_out=%d\n", g_emb.n_embd_out);

    int rc = sqlite3_open(db_path, &g_emb.db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "embedding: cannot open database: %s\n", sqlite3_errmsg(g_emb.db));
        llama_model_free(g_emb.model);
        g_emb.model = NULL;
        llama_backend_free();
        return -1;
    }

    sqlite3_exec(g_emb.db, "PRAGMA journal_mode=WAL", NULL, NULL, NULL);

    if (create_table(g_emb.db) != 0) {
        sqlite3_close(g_emb.db);
        g_emb.db = NULL;
        llama_model_free(g_emb.model);
        g_emb.model = NULL;
        llama_backend_free();
        return -1;
    }

    pthread_mutex_init(&g_emb.mutex, NULL);
    fprintf(stderr, "embedding: prepared successfully\n");
    return 0;
}

void qs_embedding_shutdown(void) {
    if (!g_emb.db && !g_emb.model) return;

    pthread_mutex_destroy(&g_emb.mutex);

    if (g_emb.db) {
        sqlite3_close(g_emb.db);
        g_emb.db = NULL;
    }
    if (g_emb.model) {
        llama_model_free(g_emb.model);
        g_emb.model = NULL;
    }
    llama_backend_free();
}

int qs_embedding_store(const char* text, int64_t id) {
    if (!g_emb.db || !g_emb.model) return -1;

    pthread_mutex_lock(&g_emb.mutex);

    float* embd = generate_embedding(text);
    if (!embd) {
        pthread_mutex_unlock(&g_emb.mutex);
        return -1;
    }

    int rc = store_vector(g_emb.db, id, embd, g_emb.n_embd_out);
    free(embd);

    pthread_mutex_unlock(&g_emb.mutex);
    return rc;
}

int qs_embedding_search(const char* query, int top_k, int64_t* out_ids,
                        float* out_scores, int max_results) {
    if (!g_emb.db || !g_emb.model) return 0;

    float* query_embd = generate_embedding(query);
    if (!query_embd) return 0;

    pthread_mutex_lock(&g_emb.mutex);

    int count = search_vectors(g_emb.db, query_embd, g_emb.n_embd_out,
                               top_k, out_ids, out_scores, max_results);

    pthread_mutex_unlock(&g_emb.mutex);
    free(query_embd);
    return count;
}

int qs_embedding_delete(int64_t id) {
    if (!g_emb.db) return -1;

    pthread_mutex_lock(&g_emb.mutex);

    const char* sql = "DELETE FROM " EMBEDDING_TABLE " WHERE rowid = ?";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_emb.db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_emb.mutex);
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, id);
    rc = (sqlite3_step(stmt) == SQLITE_DONE) ? 0 : -1;
    sqlite3_finalize(stmt);

    pthread_mutex_unlock(&g_emb.mutex);
    return rc;
}

int qs_embedding_n_embd(void) {
    return g_emb.n_embd_out;
}

#endif /* QS_EMBEDDING_MODULE_ENABLED */
