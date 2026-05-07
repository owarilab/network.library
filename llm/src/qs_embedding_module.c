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

/* Stub implementation when module is disabled. */
#if !QS_EMBEDDING_MODULE_ENABLED || !defined(QS_EMBEDDING_WITH_LLAMA_CPP) || QS_EMBEDDING_WITH_LLAMA_CPP != 1

int qs_embedding_prepare(const char* model_path, const char* db_path) { (void)model_path; (void)db_path; return 0; }
void qs_embedding_shutdown(void) {}
int qs_embedding_store(const char* text, const char* content, int64_t id) { (void)text; (void)content; (void)id; return 0; }
int qs_embedding_search(const char* query, int top_k, int64_t* out_ids, float* out_scores, char** out_texts, int max_results) { (void)query; (void)top_k; (void)out_ids; (void)out_scores; (void)out_texts; (void)max_results; return 0; }
int qs_embedding_delete(int64_t id) { (void)id; return 0; }
int qs_embedding_n_embd(void) { return 0; }

#else

#include <sqlite3.h>
#include <llama.h>

/* Forward declaration for sqlite-vec init */
int sqlite3_vec_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi);

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
    int mutex_initialized;
} g_emb;

/* ── Vector <-> JSON string helpers ───────────────────────────── */

static char* vec_to_json(const float* v, int n, int* out_len) {
    /* %.7g can produce up to 14 chars (e.g. "-1.234567e+38") + comma = 15 */
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

/* ── Internal API ─────────────────────────────────────────────── */

static int create_table(sqlite3* db) {
    /* Create a simple standard SQLite table for storing embeddings as JSON */
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
    
    /* Create index on id for faster lookups */
    sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS " EMBEDDING_TABLE "_id_idx ON " EMBEDDING_TABLE "(id)", 
                 NULL, NULL, NULL);
    
    return 0;
}

static float* generate_embedding(const char* text) {
    struct llama_context_params cparams = llama_context_default_params();

    /* For embedding model, use smaller context and embeddings mode */
    cparams.n_ctx = 512;
    cparams.pooling_type = LLAMA_POOLING_TYPE_MEAN;
    cparams.embeddings = true;

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

/* Helper: Parse JSON array of floats into array */
static float* json_array_to_floats(const char* json_str, int expected_len, int* out_len) {
    if (!json_str) return NULL;
    
    float* result = (float*)malloc(sizeof(float) * expected_len);
    if (!result) return NULL;
    
    int count = 0;
    const char* p = json_str;
    
    /* Skip opening bracket */
    while (*p && *p != '[') p++;
    if (!*p) { free(result); return NULL; }
    p++;
    
    /* Parse numbers */
    while (count < expected_len && *p) {
        while (*p && (*p == ' ' || *p == ',' || *p == '\t' || *p == '\n')) p++;
        if (*p == ']') break;
        
        char* endp;
        result[count] = strtof(p, &endp);
        if (endp == p) break;  /* No valid number found */
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

/* Helper: Calculate cosine distance between two vectors (1 - cosine_similarity) */
static float cosine_distance(const float* v1, const float* v2, int n) {
    if (n <= 0) return 1.0f;
    
    float dot = 0.0f, norm1 = 0.0f, norm2 = 0.0f;
    for (int i = 0; i < n; i++) {
        dot += v1[i] * v2[i];
        norm1 += v1[i] * v1[i];
        norm2 += v2[i] * v2[i];
    }
    
    if (norm1 < 1e-6f || norm2 < 1e-6f) return 1.0f;
    
    float similarity = dot / (sqrtf(norm1) * sqrtf(norm2));
    return 1.0f - similarity;  /* Distance = 1 - similarity */
}

static int search_vectors(sqlite3* db, const float* query_embd, int n_embd,
                          int top_k, int64_t* out_ids, float* out_scores, char** out_texts, int max_results) {
    const char* sql = "SELECT id, embedding, text_content FROM " EMBEDDING_TABLE " ORDER BY id";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return 0;
    }

    /* Collect all embeddings with their IDs and distances */
    typedef struct {
        int64_t id;
        float distance;
        char* text_content;
    } SearchResult;
    
    int capacity = 100;
    SearchResult* results = (SearchResult*)malloc(sizeof(SearchResult) * capacity);
    if (!results) {
        sqlite3_finalize(stmt);
        return 0;
    }
    
    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int64_t id = sqlite3_column_int64(stmt, 0);
        const char* emb_json = (const char*)sqlite3_column_text(stmt, 1);
        
        if (!emb_json) continue;
        
        int parsed_len;
        float* embd = json_array_to_floats(emb_json, n_embd, &parsed_len);
        if (!embd || parsed_len != n_embd) {
            free(embd);
            continue;
        }
        
        float distance = cosine_distance(query_embd, embd, n_embd);
        free(embd);
        
        /* Resize array if needed */
        if (count >= capacity) {
            capacity *= 2;
            SearchResult* new_results = (SearchResult*)realloc(results, sizeof(SearchResult) * capacity);
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
    
    /* Sort by distance */
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (results[j].distance < results[i].distance) {
                SearchResult tmp = results[i];
                results[i] = results[j];
                results[j] = tmp;
            }
        }
    }
    
    /* Return top-k results */
    int n = (count < max_results) ? count : max_results;
    if (n > top_k) n = top_k;
    for (int i = 0; i < n; i++) {
        out_ids[i] = results[i].id;
        out_scores[i] = results[i].distance;
        out_texts[i] = results[i].text_content;
    }

    /* Free unused text_content entries */
    for (int i = n; i < count; i++) {
        free(results[i].text_content);
    }

    free(results);
    return n;
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
    g_emb.mutex_initialized = 1;
    fprintf(stderr, "embedding: prepared successfully\n");
    return 0;
}

void qs_embedding_shutdown(void) {
    if (!g_emb.db && !g_emb.model) return;

    if (g_emb.mutex_initialized) {
        pthread_mutex_lock(&g_emb.mutex);
    }

    sqlite3* db = g_emb.db;
    struct llama_model* model = g_emb.model;
    g_emb.db = NULL;
    g_emb.model = NULL;
    g_emb.n_embd_out = 0;

    if (g_emb.mutex_initialized) {
        pthread_mutex_unlock(&g_emb.mutex);
        pthread_mutex_destroy(&g_emb.mutex);
        g_emb.mutex_initialized = 0;
    }

    if (db) {
        sqlite3_close(db);
    }
    if (model) {
        llama_model_free(model);
    }
    llama_backend_free();
}

int qs_embedding_store(const char* text, const char* content, int64_t id) {
    if (!g_emb.db || !g_emb.model) return -1;

    pthread_mutex_lock(&g_emb.mutex);

    float* embd = generate_embedding(text);
    if (!embd) {
        pthread_mutex_unlock(&g_emb.mutex);
        return -1;
    }

    int rc = store_vector(g_emb.db, id, embd, g_emb.n_embd_out, content);
    free(embd);

    pthread_mutex_unlock(&g_emb.mutex);
    return rc;
}

int qs_embedding_search(const char* query, int top_k, int64_t* out_ids,
                        float* out_scores, char** out_texts, int max_results) {
    if (!g_emb.db || !g_emb.model) return 0;

    pthread_mutex_lock(&g_emb.mutex);

    float* query_embd = generate_embedding(query);
    if (!query_embd) {
        pthread_mutex_unlock(&g_emb.mutex);
        return 0;
    }

    int count = search_vectors(g_emb.db, query_embd, g_emb.n_embd_out,
                               top_k, out_ids, out_scores, out_texts, max_results);

    pthread_mutex_unlock(&g_emb.mutex);
    free(query_embd);
    return count;
}

int qs_embedding_delete(int64_t id) {
    if (!g_emb.db) return -1;

    pthread_mutex_lock(&g_emb.mutex);

    const char* sql = "DELETE FROM " EMBEDDING_TABLE " WHERE id = ?";
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
