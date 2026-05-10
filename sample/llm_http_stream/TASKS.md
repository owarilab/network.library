# Agent Framework 実装タスク計画

**対象サンプル**: `sample/llm_agent/`  
**参照設計書**: `sample/llm_http_stream/AGENT_DESIGN.md`  
**参照実装**: `sample/llm_http_stream/main.c`

---

## タスク一覧（Phase別）

### Phase 1: プロジェクト基盤

#### T-101: ディレクトリ構造・Makefile 作成
- **内容**: `sample/llm_agent/` のディレクトリ・ファイル雛形を作成
- **成果物**:
  - `sample/llm_agent/Makefile`
  - `sample/llm_agent/tools/` ディレクトリ
  - `sample/llm_agent/handlers/` ディレクトリ
  - `sample/llm_agent/www/` ディレクトリ
  - `sample/llm_agent/test/` ディレクトリ
- **Makefile 参考**: `sample/llm_http_stream/Makefile`
  - `LLAMA_ENABLE=1` のみ必要（Embedding は不要）
  - ビルドターゲット: `qs_llm_agent_server`
  - `OBJGROUP = main.o agent_core.o tools/tool_file_list.o tools/tool_file_read.o handlers/*.o`
- **完了条件**: `make clean && make` が通る（リンクエラー不問）

---

#### T-102: `agent_core.h` — 構造体・API 定義
- **内容**: エージェント全体で使う型・定数・関数プロトタイプを定義
- **成果物**: `sample/llm_agent/agent_core.h`
- **定義内容**:
  ```c
  #define AGENT_MAX_ITERATIONS     10
  #define AGENT_TIMEOUT_SECONDS    60
  #define AGENT_CONTEXT_MAX_SIZE   (1024 * 1024)   // 1MB
  #define AGENT_FILE_READ_MAX_SIZE (1024 * 1024)   // 1MB
  #define AGENT_FILE_READ_DEFAULT_LINES 100
  #define AGENT_CONV_ID_LEN        64

  typedef enum {
      AGENT_ACTION_USE_TOOL    = 0,
      AGENT_ACTION_FINAL_ANSWER = 1,
      AGENT_ACTION_UNKNOWN     = -1,
  } AGENT_ACTION;

  typedef struct AGENT_TOOL_CALL {
      char tool_name[64];
      char json_args[4096];
  } AGENT_TOOL_CALL;

  typedef struct AGENT_THINK_RESULT {
      AGENT_ACTION action;
      char thought[2048];
      AGENT_TOOL_CALL tool_call;
      char answer[4096];
  } AGENT_THINK_RESULT;

  typedef struct AGENT_CONVERSATION {
      char conversation_id[AGENT_CONV_ID_LEN];
      int  iteration;
      int  max_iterations;
      char user_query[2048];
      char* accumulated_context;
      size_t context_length;
      size_t context_capacity;
  } AGENT_CONVERSATION;

  // agent_core.c の公開API
  AGENT_CONVERSATION* agent_conversation_create(const char* user_query, int max_iterations);
  void                agent_conversation_destroy(AGENT_CONVERSATION* conv);
  int                 agent_conversation_append_context(AGENT_CONVERSATION* conv,
                          const char* label, const char* content);
  void                agent_conversation_generate_id(char* out, size_t len);
  AGENT_ACTION        agent_parse_action_string(const char* action_str);
  ```
- **完了条件**: `#include "agent_core.h"` がコンパイルエラーなし

---

#### T-103: `agent_core.c` — 会話管理実装
- **内容**: T-102 で定義した API の実装
- **成果物**: `sample/llm_agent/agent_core.c`
- **実装詳細**:

  | 関数 | 実装内容 |
  |------|----------|
  | `agent_conversation_create()` | `calloc` で確保。`context_capacity = 1024*64` で初期化 |
  | `agent_conversation_destroy()` | `free(conv->accumulated_context)` → `free(conv)` |
  | `agent_conversation_append_context()` | `realloc` で拡張、`label: content\n` 形式で追記 |
  | `agent_conversation_generate_id()` | `/dev/urandom` or `rand()` + 時刻で `conv-XXXXXXXX` 形式生成 |
  | `agent_parse_action_string()` | `strcmp("use_tool")`, `strcmp("final_answer")` で分岐 |

- **完了条件**: T-201 の tool テストから呼び出してクラッシュしない

---

### Phase 2: File System Tools 実装

#### T-201: `tools/tool_file_list.h` / `tools/tool_file_list.c`
- **内容**: `file_list` ツールの実装
- **成果物**: `tools/tool_file_list.h`, `tools/tool_file_list.c`
- **インターフェース**:
  ```c
  // tools/tool_file_list.h
  int tool_file_list_execute(const char* json_args, char* output, size_t output_size);
  ```
- **入力 JSON**:
  ```json
  { "path": "./src", "recursive": 0, "pattern": "*.c" }
  ```
- **実装手順**:
  1. `json_args` から `path`, `recursive`, `pattern` をパース（`strstr` で簡易実装可）
  2. `opendir(path)` でディレクトリを開く
  3. `readdir()` でエントリを列挙
  4. `pattern` が指定されていれば `fnmatch()` でフィルタ
  5. 各エントリで `stat()` を実行して `is_directory` を判定
  6. `output` に JSON 形式で出力:
     ```json
     {"entries":["main.c","utils.c"],"is_directory":[0,0],"count":2}
     ```
  7. `closedir()`
- **エラーハンドリング**:
  - `path` が NULL → `{"error":"path is required"}`
  - `opendir` 失敗 → `{"error":"cannot open directory: <path>"}`
  - output バッファオーバーフロー → エラー返却
- **完了条件**:
  - `path="."` で実行して `entries` にファイル名が含まれる
  - 存在しないパスで `error` フィールドが返る

---

#### T-202: `tools/tool_file_read.h` / `tools/tool_file_read.c`
- **内容**: `file_read` ツールの実装
- **成果物**: `tools/tool_file_read.h`, `tools/tool_file_read.c`
- **インターフェース**:
  ```c
  // tools/tool_file_read.h
  int tool_file_read_execute(const char* json_args, char* output, size_t output_size);
  ```
- **入力 JSON**:
  ```json
  { "path": "./src/main.c", "start_line": 1, "end_line": 50 }
  ```
- **実装手順**:
  1. `json_args` から `path`, `start_line`, `end_line`, `max_size` をパース
  2. `start_line` デフォルト 1、`end_line` デフォルト 100
  3. `max_size` デフォルト `AGENT_FILE_READ_MAX_SIZE`（1MB）
  4. `fopen(path, "r")` でファイルを開く
  5. `fgets()` ループで行単位読込
  6. `start_line`〜`end_line` の範囲のみ content バッファに追記
  7. JSON エスケープ処理（`\n` → `\\n`, `\"` → `\\\"` など）
  8. `output` に JSON 形式で出力:
     ```json
     {"content":"#include...\n","line_count":50,"file_size":2048}
     ```
  9. `fclose()`
- **セキュリティ制約**:
  - `path` に `../` が含まれる場合はエラー（パストラバーサル防止）
  - ファイルサイズが `max_size` を超えた場合は打ち切り
- **エラーハンドリング**:
  - `path` が NULL or 空 → `{"error":"path is required"}`
  - `fopen` 失敗 → `{"error":"cannot open file: <path>"}`
- **完了条件**:
  - `path="./main.c"` で 1〜10行が読み取れる
  - `start_line=5, end_line=10` で 5〜10行のみ返る
  - `../` を含むパスでエラーになる

---

#### T-203: Tool Registry 実装（`agent_core.c` 内）
- **内容**: ツールを名前で呼び出せるレジストリの実装
- **追加先**: `agent_core.h` と `agent_core.c`
- **実装**:
  ```c
  // agent_core.h に追加
  typedef int (*ToolExecutor)(const char* json_args, char* output, size_t output_size);
  typedef struct TOOL_ENTRY {
      const char* name;
      ToolExecutor executor;
  } TOOL_ENTRY;

  int  agent_tool_execute(const char* tool_name, const char* json_args,
                          char* output, size_t output_size);
  int  agent_tool_is_registered(const char* tool_name);
  ```
  ```c
  // agent_core.c に実装
  static TOOL_ENTRY g_tool_registry[] = {
      { "file_list", tool_file_list_execute },
      { "file_read", tool_file_read_execute },
      { NULL, NULL }
  };
  ```
- **完了条件**:
  - `agent_tool_execute("file_list", ...)` が呼び出せる
  - 未登録ツール名でエラー返却

---

### Phase 3: HTTP ハンドラー実装

#### T-301: `handlers/handler_init.c`
- **内容**: `/api/agent/init` — 会話セッション初期化
- **参考**: `sample/llm_http_stream/main.c` の `on_http_event` 内の分岐パターン
- **処理フロー**:
  1. リクエスト JSON から `max_iterations`（デフォルト 5）を取得
  2. `agent_conversation_generate_id()` で ID 生成
  3. **現フェーズはメモリ上で保持しない（ステートレス）**
     - `/api/agent/run` が一括処理するため init は ID 返却のみでOK
  4. レスポンス:
     ```json
     {"ok":true,"conversation_id":"conv-XXXXXXXX","max_iterations":5}
     ```
- **完了条件**: `curl -X POST /api/agent/init` で conversation_id が返る

---

#### T-302: `handlers/handler_think.c`
- **内容**: `/api/agent/think` — LLM に次のアクションを判定させる
- **処理フロー**:
  1. リクエスト JSON から `user_query`, `accumulated_context`, `available_tools` を取得
  2. `prompt.conf` の `[SYSTEM_AGENT_THINK]` ブロックを使ってプロンプト生成
     - `{available_tools}` を `["file_list","file_read"]` に置換
     - `{accumulated_context}` を蓄積コンテキストに置換
  3. `qs_llama_module_text()` で LLM 推論実行（非ストリーム）
  4. レスポンスのJSON文字列をパース（`action`, `thought`, `tool_name`, `tool_args`, `answer`）
  5. レスポンス返却:
     ```json
     {
       "ok":true,
       "decision":{
         "thought":"...","action":"use_tool",
         "tool_name":"file_list",
         "tool_args":{"path":"./","recursive":0}
       }
     }
     ```
- **エラーハンドリング**: LLM 出力が不正 JSON の場合は `action: "final_answer"` として扱う
- **完了条件**: `curl -X POST /api/agent/think` でアクション決定が返る

---

#### T-303: `handlers/handler_execute.c`
- **内容**: `/api/agent/execute` — ツールを実行する
- **処理フロー**:
  1. リクエスト JSON から `tool_name`, `tool_args` を取得
  2. `agent_tool_is_registered(tool_name)` でホワイトリスト確認
  3. `agent_tool_execute(tool_name, tool_args, output, sizeof(output))` 呼び出し
  4. レスポンス:
     ```json
     {"ok":true,"tool_name":"file_list","result":{...},"execution_time_ms":10}
     ```
- **完了条件**:
  - `file_list` と `file_read` が実行できる
  - 未登録ツールでエラーが返る

---

#### T-304: `handlers/handler_loop.c`
- **内容**: `/api/agent/loop` — Think → Execute を 1 回実行
- **処理フロー**:
  1. リクエスト JSON から `user_query`, `accumulated_context`, `available_tools`, `iteration` を取得
  2. T-302 相当の Think を内部実行
  3. `action == "use_tool"` なら T-303 相当の Execute を内部実行
  4. レスポンス:
     ```json
     {
       "ok":true,
       "iteration":2,
       "step":{
         "thought":"...","action":"use_tool",
         "tool_name":"file_list","tool_args":{...},
         "tool_result":{...}
       },
       "should_continue":true,
       "updated_context":"..."
     }
     ```
- **完了条件**: 1回のループが機能し、コンテキストが更新される

---

#### T-305: `handlers/handler_run.c`
- **内容**: `/api/agent/run` — 完全自動ループ（最重要）
- **処理フロー**:
  1. リクエスト JSON から `user_query`, `available_tools`, `max_iterations` を取得
  2. `agent_conversation_create()` でセッション作成
  3. ループ（最大 `max_iterations` 回）:
     a. プロンプト構築（`user_query` + `accumulated_context` + tool 一覧）
     b. `qs_llama_module_text()` で LLM 推論
     c. レスポンス JSON パース（AGENT_THINK_RESULT）
     d. `action == "final_answer"` → ループ終了
     e. `action == "use_tool"` → `agent_tool_execute()` 実行
     f. ツール結果を `agent_conversation_append_context()` で蓄積
  4. `agent_conversation_destroy()`
  5. レスポンス:
     ```json
     {
       "ok":true,
       "conversation_id":"conv-...",
       "final_answer":"バグ: null pointer dereference at line 45...",
       "total_iterations":5,
       "tool_usage":{"file_list":2,"file_read":2}
     }
     ```
- **エッジケース**:
  - max_iterations 到達時: LLM に「以上の情報から回答して」と強制プロンプト
  - ツール実行エラー: エラー内容をコンテキストに追記して継続
- **完了条件**:
  - `user_query="このディレクトリのファイルを教えて"` で file_list が実行され回答が得られる
  - max_iterations を超えないこと

---

### Phase 4: main.c と設定ファイル

#### T-401: `main.c` — サーバー起動・ルーティング
- **内容**: エージェントサーバーのエントリポイント
- **参考**: `sample/llm_http_stream/main.c` をベースにする
- **ルーティング**:
  ```c
  static int on_http_event(QS_EVENT_PARAMETER params)
  {
      const char* path = api_qs_get_http_request_path(params);
      if (strcmp(path, "/api/agent/init")    == 0) return handler_init(params);
      if (strcmp(path, "/api/agent/think")   == 0) return handler_think(params);
      if (strcmp(path, "/api/agent/execute") == 0) return handler_execute(params);
      if (strcmp(path, "/api/agent/loop")    == 0) return handler_loop(params);
      if (strcmp(path, "/api/agent/run")     == 0) return handler_run(params);
      return api_qs_http_response_send_file(params, "./www", path);
  }
  ```
- **初期化処理**:
  - `g_temporary_memory` 確保（8MB）
  - `settings.conf` 読込
  - `qs_llama_module_prepare()` 呼び出し
- **完了条件**: `./qs_llm_agent_server` が起動し `/api/agent/run` が HTTP 200 を返す

---

#### T-402: `settings.conf` / `prompt.conf` 作成
- **成果物**: `sample/llm_agent/settings.conf`, `sample/llm_agent/prompt.conf`
- **settings.conf**:
  ```
  server_port = 4445;
  scheduler_mode = 'middle';
  max_connection = 10;
  ```
  ※ llm_http_stream (4444) と被らないよう 4445 を使用
- **prompt.conf**:
  ```
  system_agent_think = "
  [SYSTEM]
  あなたは自律型コードエージェントです。
  ファイルシステムツールを使用してコードを探索・分析し、ユーザーの質問に答えてください。

  利用可能なツール: {available_tools}

  以下のJSON形式のみで返答してください:
  {
    \"thought\": \"現在の状況分析\",
    \"action\": \"use_tool\" or \"final_answer\",
    \"tool_name\": \"file_list\" or \"file_read\",
    \"tool_args\": {...},
    \"answer\": \"最終回答（action=final_answer の場合のみ）\"
  }

  コンテキスト:
  {accumulated_context}
  ";
  ```
- **完了条件**: サーバー起動時に設定値が読み込まれる

---

### Phase 5: テスト

#### T-501: curl テストスクリプト作成
- **成果物**: `sample/llm_agent/test/test_api.sh`
- **テストケース**:
  ```bash
  # T-501-1: init
  curl -s -X POST http://localhost:4445/api/agent/init \
    -H "Content-Type: application/json" \
    -d '{"max_iterations":5}'

  # T-501-2: execute (file_list)
  curl -s -X POST http://localhost:4445/api/agent/execute \
    -H "Content-Type: application/json" \
    -d '{"tool_name":"file_list","tool_args":{"path":"./"}}'

  # T-501-3: execute (file_read)
  curl -s -X POST http://localhost:4445/api/agent/execute \
    -H "Content-Type: application/json" \
    -d '{"tool_name":"file_read","tool_args":{"path":"./main.c","start_line":1,"end_line":10}}'

  # T-501-4: run (フルループ)
  curl -s -X POST http://localhost:4445/api/agent/run \
    -H "Content-Type: application/json" \
    -d '{"user_query":"このディレクトリにあるCファイルを教えて","max_iterations":3}'

  # T-501-5: セキュリティ (パストラバーサル)
  curl -s -X POST http://localhost:4445/api/agent/execute \
    -H "Content-Type: application/json" \
    -d '{"tool_name":"file_read","tool_args":{"path":"../../../etc/passwd"}}'
  # → error フィールドが返ること
  ```
- **完了条件**: 全テストが期待通りの JSON を返す

---

#### T-502: README.md 作成
- **成果物**: `sample/llm_agent/README.md`
- **内容**: ビルド方法、起動方法、curl テスト例、依存関係
- **完了条件**: README 通りにビルド・起動できること

---

### Phase 6: フロントエンド（任意・後回し可）

#### T-601: `www/index.html` — Agent UI
- **内容**: ブラウザからエージェントを操作できる簡易UI
- **機能**:
  - テキストエリアに質問入力
  - 「実行」ボタンで `/api/agent/run` を fetch
  - Iteration ごとのログ表示（thought, tool_name, tool_args, result）
  - 最終回答表示
- **依存**: なし（Vanilla JS のみ）
- **完了条件**: ブラウザから `user_query` を送信して回答が表示される

---

## タスク依存関係

```
T-101 (ディレクトリ/Makefile)
  └─► T-102 (agent_core.h)
        └─► T-103 (agent_core.c)
              └─► T-203 (Tool Registry)
                    ├─► T-201 (file_list)
                    │     └─► T-303 (handler_execute)
                    │           ├─► T-304 (handler_loop)
                    │           └─► T-305 (handler_run) ◄── T-302 (handler_think)
                    └─► T-202 (file_read)

T-101 ──────────────────────────────────────────► T-401 (main.c)
T-402 (設定ファイル) ─────────────────────────► T-401

T-401 (main.c) ──────────────────────────────► T-501 (テスト)
                                                └─► T-502 (README)
```

## 実装順序（推奨）

| 順序 | タスク ID | 説明 |
|------|-----------|------|
| 1    | T-101     | ディレクトリ・Makefile |
| 2    | T-102     | agent_core.h ヘッダ定義 |
| 3    | T-103     | agent_core.c 基本実装 |
| 4    | T-201     | tool_file_list 実装 |
| 5    | T-202     | tool_file_read 実装 |
| 6    | T-203     | Tool Registry 統合 |
| 7    | T-402     | 設定ファイル作成 |
| 8    | T-401     | main.c（ルーティングのみ、ハンドラーはスタブ） |
| 9    | T-303     | handler_execute（最も単純、ツールテストに有用） |
| 10   | T-302     | handler_think（LLM判定） |
| 11   | T-304     | handler_loop（think + execute の組み合わせ） |
| 12   | T-305     | handler_run（完全自動ループ） |
| 13   | T-301     | handler_init（シンプル、任意） |
| 14   | T-501     | curl テストスクリプト |
| 15   | T-502     | README.md |
| 16   | T-601     | フロントエンド UI（任意） |

## 各タスクのサイズ感

| タスク ID | 難易度 | コード量 | 備考 |
|-----------|--------|----------|------|
| T-101 | 低 | ~30 行 | Makefile |
| T-102 | 低 | ~60 行 | ヘッダ定義のみ |
| T-103 | 中 | ~120 行 | malloc/realloc管理 |
| T-201 | 中 | ~150 行 | opendir/readdir + JSON出力 |
| T-202 | 中 | ~150 行 | fopen/fgets + JSONエスケープ |
| T-203 | 低 | ~30 行 | 静的テーブル |
| T-301 | 低 | ~40 行 | ID生成のみ |
| T-302 | 高 | ~200 行 | LLM呼び出し + JSON解析 |
| T-303 | 中 | ~80 行 | ツール呼び出しラッパー |
| T-304 | 中 | ~100 行 | think + execute 組み合わせ |
| T-305 | 高 | ~250 行 | ループ制御 + コンテキスト管理 |
| T-401 | 中 | ~150 行 | llm_http_stream の main.c 参考 |
| T-402 | 低 | ~20 行 | 設定ファイル |
| T-501 | 低 | ~50 行 | Shell スクリプト |
| T-502 | 低 | ~80 行 | Markdown |
| T-601 | 中 | ~200 行 | HTML + JS |
