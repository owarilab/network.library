/*
 * Copyright (c) Katsuya Owari
 */

#ifdef __cplusplus
extern "C"{
#endif

#ifndef _QS_EMBEDDING_MODULE_H_
#define _QS_EMBEDDING_MODULE_H_

#include <stdint.h>

// build option (can be overridden by compiler definitions)
#ifndef QS_EMBEDDING_MODULE_ENABLED
#define QS_EMBEDDING_MODULE_ENABLED 0
#endif

// llama.cpp C header availability flag (set by Makefile when enabled)
// Not auto-detected here — controlled via QS_EMBEDDING_WITH_LLAMA_CPP in Makefile

/* 初期化 / シャットダウン */
int qs_embedding_prepare(const char* model_path, const char* db_path);
void qs_embedding_shutdown(void);

/* embedding 生成 + 保存 */
int qs_embedding_store(const char* text, int64_t id);

/* 類似検索 (top_k) */
int qs_embedding_search(const char* query, int top_k, int64_t* out_ids, float* out_scores, int max_results);

/* 削除 */
int qs_embedding_delete(int64_t id);

/* ベクトル次元数の取得（モデル依存） */
int qs_embedding_n_embd(void);

#endif /* _QS_EMBEDDING_MODULE_H_ */

#ifdef __cplusplus
}
#endif
