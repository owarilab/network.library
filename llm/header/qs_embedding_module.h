/*
 * Copyright (c) Katsuya Owari
 */

#ifdef __cplusplus
extern "C"{
#endif

#ifndef _QS_EMBEDDING_MODULE_H_
#define _QS_EMBEDDING_MODULE_H_

#include <stdint.h>

typedef struct QS_EMBEDDING_STORE QS_EMBEDDING_STORE;

// build option (can be overridden by compiler definitions)
#ifndef QS_EMBEDDING_MODULE_ENABLED
#define QS_EMBEDDING_MODULE_ENABLED 0
#endif

// llama.cpp C header availability flag (set by Makefile when enabled)
// Not auto-detected here — controlled via QS_EMBEDDING_WITH_LLAMA_CPP in Makefile

/* 初期化 / シャットダウン */
int qs_embedding_prepare(const char* model_path, const char* db_path, QS_EMBEDDING_STORE** out_store);
void qs_embedding_shutdown(QS_EMBEDDING_STORE* store);

/* embedding 生成 + 保存 */
int qs_embedding_store(QS_EMBEDDING_STORE* store, const char* text, const char* content, int64_t id);

/* 類似検索 (top_k) */
int qs_embedding_search(QS_EMBEDDING_STORE* store, const char* query, int top_k, int64_t* out_ids, float* out_scores, char** out_texts, int max_results);

/* 削除 */
int qs_embedding_delete(QS_EMBEDDING_STORE* store, int64_t id);

/* ベクトル次元数の取得（モデル依存） */
int qs_embedding_n_embd(QS_EMBEDDING_STORE* store);

#endif /* _QS_EMBEDDING_MODULE_H_ */

#ifdef __cplusplus
}
#endif
