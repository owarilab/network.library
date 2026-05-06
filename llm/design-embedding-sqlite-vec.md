# Embedding 生成 + sqlite-vec 保存 設計方針

## 概要

llama.cpp の C API を使ってテキストから embedding ベクトルを生成し、sqlite-vec で SQLite に保存する。既存の `qs_llama_module.c`（テキストストリーミング実装済み）に embedding モジュールを追加する形式とする。

## アーキテクチャ

```
┌─────────────────────────────────────────────┐
│              呼び出し元 (Cアプリ)             │
│  qs_embedding_store(text, id)               │
│  qs_embedding_search(query, top_k, results) │
│  qs_embedding_delete(id)                    │
└───────────┬─────────────────────────────────┘
            │
┌───────────▼─────────────────────────────────┐
│         qs_embedding_module.c               │
│                                             │
│  1. llama.cpp C API で embedding 生成        │
│  2. ベクトルを sqlite-vec に INSERT/検索     │
└───────────┬─────────────────────────────────┘
            │
    ┌───────▼───────┐
    │  llama.cpp    │  (embedding モデル)
    │  libllama     │
    └───────┬───────┘
            │
    ┌───────▼───────┐
    │  sqlite-vec   │  (SQLite extension)
    │  .db ファイル  │
    └───────────────┘
```

## ビルド条件

embedding モジュールはオプション。`QS_EMBEDDING_MODULE_ENABLED=1` が Makefile で指定されていない場合、ビルドに含まれない。既存の `qs_llama_module.c` と同じガードパターンに従う。

### コンパイル時の分岐

```c
/* qs_embedding_module.c の先頭 */

#if QS_EMBEDDING_MODULE_ENABLED && defined(QS_EMBEDDING_WITH_LLAMA_CPP) && (QS_EMBEDDING_WITH_LLAMA_CPP == 1)

#include <sqlite3.h>
// ... sqlite-vec / llama.cpp の実装

#else

/* オプション無効時はスタブ実装。常に0を返す（成功扱い） */
int qs_embedding_prepare(const char* model_path, const char* db_path) { return 0; }
void qs_embedding_shutdown(void) {}
int qs_embedding_store(const char* text, int64_t id) { (void)text; (void)id; return 0; }
int qs_embedding_search(const char* query, int top_k, int64_t* out_ids, float* out_scores, int max_results) { (void)query; (void)top_k; (void)out_ids; (void)out_scores; (void)max_results; return 0; }
int qs_embedding_delete(int64_t id) { (void)id; return 0; }
int qs_embedding_n_embd(void) { return 0; }

#endif /* QS_EMBEDDING_MODULE_ENABLED */
```

### Makefile での制御

```makefile
# QS_EMBEDDING_MODULE_ENABLED=1 を指定した場合のみ embedding モジュールをビルド
ifeq ($(QS_EMBEDDING_MODULE_ENABLED),1)
CFLAGS += -DQS_EMBEDDING_MODULE_ENABLED=1 -DQS_EMBEDDING_WITH_LLAMA_CPP=1
OBJS += qs_embedding_module.o
LDFLAGS += -lsqlite3 -lm
endif
```

### ビルド例

```bash
# embedding なし（デフォルト）
make

# embedding あり
make QS_EMBEDDING_MODULE_ENABLED=1
```

## 依存関係

| コンポーネント | バージョン | 配置場所 |
|---|---|---|
| llama.cpp | latest | `third_party/llama.cpp/` |
| sqlite-vec | latest | `third_party/sqlite-vec/` |
| SQLite3 | 3.45.1 (システムランタイム) | システム (`/usr/bin/sqlite3`) |
| libsqlite3-dev | 必須 | **未インストール** (`sudo apt-get install libsqlite3-dev`) |

sqlite-vec の取得:
```bash
git clone https://github.com/asg017/sqlite-vec third_party/sqlite-vec

# header生成
cd sqlite-vec/llm/third_party/sqlite-vec
make sqlite-vec.h
```

## sqlite-vec の組み込み方法

### 単一Cファイル方式（推奨）

sqlite-vec は **依存なしの単一Cファイル** (`sqlite-vec.c`, 約10K行) で構成される。シェアードライブラリ化不要で、プロジェクトの他のCファイルと一緒に直接コンパイルして静的ライブラリに含める。

```makefile
# Makefileでの組み込み例
EMBEDDING_OBJS += third_party/sqlite-vec/sqlite-vec.o

$(PROGRAM): $(OBJGROUP) $(EMBEDDING_OBJS)
	$(AR) r $(PROGRAMOUT)$(PROGRAM) $(OBJGROUP) $(EMBEDDING_OBJS)

third_party/sqlite-vec/sqlite-vec.o: third_party/sqlite-vec/sqlite-vec.c
	$(CC) $(CFLAG) -c $< -o $@
```

### sqlite-vec の API 仕様

sqlite-vec に独立の C API は存在しない。すべての操作は標準 SQLite3 C API (`sqlite3_*` 関数) で行う。

**ベクトル値の表現形式**: JSON配列文字列（`[v1,v2,...]`）。`(v1,v2,...)` ではない。

```c
// データベースを開く
sqlite3_open(db_path, &db);

// vec0 virtual table作成
sqlite3_exec(db, "CREATE VIRTUAL TABLE t USING vec0(embedding float[768])", NULL, NULL, NULL);

// INSERT (ベクトルは JSON配列形式の文字列)
sqlite3_prepare_v2(db, "INSERT INTO t(rowid, embedding) VALUES (?, ?)", -1, &stmt, NULL);
sqlite3_bind_int64(stmt, 1, id);
sqlite3_bind_text(stmt, 2, "[0.1,-0.2,...]", -1, SQLITE_STATIC);
sqlite3_step(stmt);

// KNN検索 (cosine distance) — MATCH句 + ORDER BY distance LIMIT
const char* sql = "SELECT rowid, distance FROM t WHERE embedding MATCH ? ORDER BY distance LIMIT ?";
sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
sqlite3_bind_text(stmt, 1, "[0.1,-0.2,...]", -1, SQLITE_STATIC); // クエリベクトル
sqlite3_bind_int(stmt, 2, top_k);                                 // K値
```

**重要**: 誤って `<->` operator を使わない。正しくは `WHERE embedding MATCH ? ORDER BY distance LIMIT N` 形式。

## API 設計

### ヘッダ (`qs_embedding_module.h`)

```c
#pragma once
#include <stdint.h>

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
```

### llama.cpp embedding API の呼び出しフロー

```c
// 既存の g_llama_model を使い回す（qs_llama_module.c から公開する必要あり）

static float* generate_embedding(const char* text) {
    // 1. context 作成 (pooling_type=MEAN, embeddings=true)
    llama_context_params cparams = llama_context_default_params();
    cparams.pooling_type = LLAMA_POOLING_TYPE_MEAN;
    cparams.embeddings   = true;
    struct llama_context* ctx = llama_init_from_model(g_llama_model, cparams);

    // 2. トークン化 + バッチ構築
    int32_t n_ctx = llama_n_ctx(ctx);
    llama_token* tokens = malloc(sizeof(llama_token) * n_ctx);
    int32_t n_tokens = llama_tokenize(vocab, text, strlen(text), tokens, n_ctx, true, true);
    struct llama_batch batch = llama_batch_get_one(tokens, n_tokens);

    // 3. 推論
    llama_decode(ctx, batch);

    // 4. embedding 取得 (mean pooling済み)
    float* embd = llama_get_embeddings_seq(ctx, 0);
    int32_t n_embd = llama_model_n_embd_out(g_llama_model);

    // 5. コピーして返す（context 解放後に使えるよう）
    float* result = malloc(sizeof(float) * n_embd);
    memcpy(result, embd, sizeof(float) * n_embd);

    free(tokens);
    llama_free(ctx);
    return result;
}
```

### sqlite-vec の SQL スキーマ

```sql
-- テーブル定義（初回のみ）
CREATE VIRTUAL TABLE embedding_vecs USING vec0(
    embedding float[768]  -- モデルの n_embd_out に合わせる
);

-- インデックスは不要。sqlite-vec が内部的に HNSW を構築する。

-- INSERT (embedding は JSON配列形式 "[v1,v2,...]" で渡す)
INSERT INTO embedding_vecs(rowid, embedding) VALUES (?, ?);

-- KNN検索: MATCH句 + ORDER BY distance LIMIT
SELECT rowid, distance FROM embedding_vecs
WHERE embedding MATCH '[q1,q2,...]'
ORDER BY distance LIMIT 5;
```

### sqlite-vec INSERT/SELECT の呼び出し例

```c
static int store_vector(sqlite3* db, int64_t id, const float* embd, int n_embd) {
    // ベクトルを JSON配列形式 "[0.123,-0.456,...]" に変換
    char* vec_str = malloc(n_embd * 10 + 4);  // "[v,v,...]" ≈ 9byte/token + brackets
    vec_str[0] = '[';
    for (int i = 0; i < n_embd; i++) {
        if (i > 0) vec_str[i*9+1] = ',';
        int written = snprintf(vec_str + i*9, 10, "%.7g", embd[i]);
        // ※簡易実装では固定幅が安全でないため、sprintf_s/snprintfで動的調整が必要
    }
    vec_str[n_embd * 9] = ']';
    vec_str[n_embd * 9 + 1] = '\0';

    const char* sql = "INSERT INTO embedding_vecs(rowid, embedding) VALUES (?, ?)";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_int64(stmt, 1, id);
    sqlite3_bind_text(stmt, 2, vec_str, -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    free(vec_str);
    return (rc == SQLITE_DONE) ? 0 : -1;
}

static int search_vectors(sqlite3* db, const float* query_embd, int n_embd,
                          int top_k, int64_t* out_ids, float* out_scores, int max_results) {
    // クエリベクトルも同様に JSON配列形式に変換
    char* vec_str = malloc(n_embd * 10 + 4);
    vec_str[0] = '[';
    for (int i = 0; i < n_embd; i++) {
        if (i > 0) vec_str[i*9+1] = ',';
        snprintf(vec_str + i*9, 10, "%.7g", query_embd[i]);
    }
    vec_str[n_embd * 9] = ']';
    vec_str[n_embd * 9 + 1] = '\0';

    // MATCH句 + ORDER BY distance LIMIT（<-> operatorは使わない）
    const char* sql =
        "SELECT rowid, distance FROM embedding_vecs "
        "WHERE embedding MATCH ? ORDER BY distance LIMIT ?";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

    sqlite3_bind_text(stmt, 1, vec_str, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, max_results);

    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && count < max_results) {
        out_ids[count]   = sqlite3_column_int64(stmt, 0);
        out_scores[count]= sqlite3_column_double(stmt, 1);
        count++;
    }
    sqlite3_finalize(stmt);
    free(vec_str);
    return count;
}
```

### vec0 の内部構造（参考）

sqlite-vec は `vec0` virtual table を作成すると、以下の shadow tables も自動生成する:

| Shadow Table | 内容 |
|---|---|
| `xyz_chunks` | チャンク管理（rowidのバッチ単位） |
| `xyz_rowids` | rowid → chunk_id/chunk_offset のマッピング |
| `xyz_vector_chunksNN` | ベクトルデータ本体（BLOB形式で保存） |

ベクトルはメモリ上で HNSW インデックスを保持し、KNN検索時は近似最近傍探索を行う。正確な全件検索ではないが、「fast enough」の設計思想。

## 実装計画

### Step 0: sqlite3-dev のインストール

```bash
sudo apt-get install libsqlite3-dev
```

### Step 1: sqlite-vec.c の直接コンパイル

`third_party/sqlite-vec/sqlite-vec.c` をプロジェクトのオブジェクトに追加。独立ヘッダは不要（`sqlite-vec.h` は内部用）。Makefile に `EMBEDDING_OBJS` を追加。

### Step 2: qs_embedding_module.c の実装

- `qs_embedding_prepare()` で llama.cpp モデル + sqlite3 データベースを開く
- テーブル存在チェック + CREATE IF NOT EXISTS
- `qs_embedding_store()` で embedding 生成 + INSERT
- `qs_embedding_search()` でクエリ生成 + 類似検索
- `qs_embedding_delete()` で DELETE

### Step 3: qs_llama_module.c との連携

既存のストリーミング用モデル（`g_llama_model`）を共有するため、`llama.h` のポインタを公開する必要がある。方法としては:

- **Option A**: `qs_llama_module.h` に `struct llama_model* qs_llama_get_model(void);` を追加
- **Option B**: embedding 用の独立したモデルロード関数を `qs_embedding_module.c` に持つ（推奨：embedding と generation で別のモデルを使う場合が多い）

### Step 4: Makefile の更新

`src/Makefile` を修正。sqlite-vec.c は直接コンパイルして静的ライブラリに含むため、LDFLAGSでの `-lsqlite3` は不要（sqlite3はランタイム時の動的リンクで十分）。

```makefile
# embedding モジュール有効化時
ifeq ($(QS_EMBEDDING_MODULE_ENABLED),1)
DEFINE  += -DQS_EMBEDDING_MODULE_ENABLED=1 -DQS_EMBEDDING_WITH_LLAMA_CPP=1
EMBEDDING_OBJS += third_party/sqlite-vec/sqlite-vec.o
OBJGROUP += qs_embedding_module.o
INCDIR += -I$(LLAMA_CPP_DIR) -I$(LLAMA_CPP_DIR)/include -I$(LLAMA_CPP_DIR)/ggml/include
endif

build: $(LLAMA_RUNTIME_TARGET) $(PROGRAM)

$(PROGRAM): $(OBJGROUP) $(EMBEDDING_OBJS)
	$(AR) r  $(PROGRAMOUT)$(PROGRAM) $(OBJGROUP) $(EMBEDDING_OBJS)

third_party/sqlite-vec/sqlite-vec.o: third_party/sqlite-vec/sqlite-vec.c
	$(CC) $(CFLAG) -c $< -o $@

qs_embedding_module.o: qs_embedding_module.c
	$(CC) $(DEFINE) $(CFLAG) $(INCDIR) -c $< -o $@
```

## 考慮事項

### モデルの共有 vs 独立

- **共有**: メモリ節約。generation モデルが embedding 対応なら可能
- **独立**: BERT系embeddingモデルとLLMは別ファイルになるため、現実的には独立ロードが必要

### トランザクション

大量INSERT時は `BEGIN` / `COMMIT` で囲むことで大幅に高速化される:

```c
sqlite3_exec(db, "BEGIN", NULL, NULL, NULL);
for (...) {
    // INSERT
}
sqlite3_exec(db, "COMMIT", NULL, NULL, NULL);
```

### 正規化

llama.cpp の `common_embd_normalize()` で L2 ノーマライズを適用すると、cosine similarity が内積計算に簡略化できる。sqlite-vec は cosine distance をサポートしているので、正規化の有無は検索精度に影響するが、基本的にはそのまま cosine distance で十分。

### スレッドセーフティ

- llama.cpp の context 作成/推論は mutex で保護済み
- sqlite3 は `open` 時のモード次第で複数スレッドから安全に読める（WAL モード推奨）
- INSERT と SELECT の同時実行には `sqlite3_open_v2()` で `SQLITE_OPEN_FULLMUTEX` を設定
