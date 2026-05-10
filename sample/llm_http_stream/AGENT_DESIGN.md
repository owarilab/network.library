# Agent Framework Design

LLM HTTP Stream サーバーのエージェント基盤の設計ドキュメント

## 概要

LLMを中心とした自律型エージェント・フレームワーク。複数のツールを組み合わせて複雑なタスクを解決します。

## アーキテクチャ

```
┌─────────────────────────────────────────────────────────────┐
│                        HTTP Request                          │
└────────────────────────┬────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────┐
│                   Agent Orchestrator                         │
│  - Routing (/api/agent/*)                                   │
│  - State Management (conversation_id)                        │
│  - Iteration Control (max_iterations)                        │
│  - Context Accumulation                                      │
└────────┬──────────────┬──────────────┬──────────────────────┘
         │              │              │
         ▼              ▼              ▼
    ┌────────┐    ┌──────────┐    ┌──────────┐
    │ Agent  │    │  Agent   │    │  Agent   │
    │  Init  │    │  Think   │    │ Execute  │
    └────────┘    └──────────┘    └──────────┘
                       │
         ┌─────────────┼─────────────┐
         ▼             ▼             ▼
    ┌────────┐   ┌─────────┐   ┌──────────┐
    │  LLM   │   │ prompt  │   │  Tool    │
    │ (Think)│   │  .conf  │   │ Decision │
    └────────┘   └─────────┘   └──────────┘
                       │
         ┌─────────────┼─────────────┐
         ▼             ▼
    ┌─────────────────────────────┐
    │     Tool Executor           │
    │  - Tool Registry (2 tools)  │
    │  - Parameter Validation     │
    │  - Resource Limits          │
    └──────────┬──────────────────┘
               │
        ┌──────┴────────┐
        ▼               ▼
    ┌──────────┐   ┌──────────┐
    │file_list │   │file_read │
    │(ls)      │   │(cat)     │
    └──────────┘   └──────────┘
         │
         └─► LLM がツール実行結果を受け取る
             → ファイル構造/内容をコンテキストに蓄積
             → 次のステップを決定
             → 終了条件チェック
```

## Core Components

### 1. Agent State Structure

```c
typedef struct AGENT_CONVERSATION {
    char conversation_id[64];           // ユニークな会話ID
    int iteration;                      // 現在のイテレーション
    int max_iterations;                 // 最大イテレーション数
    
    char* user_query;                   // ユーザーの元質問
    char* accumulated_context;          // ステップごとの結果蓄積
    size_t context_length;
    size_t context_capacity;
    
    time_t created_at;                  // 会話開始時刻
    time_t last_access_at;              // 最終アクセス時刻
    
} AGENT_CONVERSATION;
```

### 2. Tool Registry

```c
typedef enum {
    TOOL_FILE_LIST,            // ファイルリスト（ls相当）
    TOOL_FILE_READ,            // ファイル読込（cat/head相当）
} TOOL_TYPE;

typedef struct TOOL_DEFINITION {
    TOOL_TYPE type;
    const char* name;                   // "file_list", "file_read"
    const char* description;            // ツール説明
    const char* required_params[];      // 必須パラメータ
    int param_count;
    
    int (*executor)(const char* json_args, char* output, size_t output_size);
} TOOL_DEFINITION;
```

### 2.1 File System Tools Detail

#### file_list - ディレクトリ内容表示
```c
typedef struct FILE_LIST_RESULT {
    char** entries;             // ファイル/ディレクトリ名
    int* is_directory;          // ディレクトリフラグ
    int count;
} FILE_LIST_RESULT;
```

**パラメータ**:
- `path`: 対象ディレクトリ（デフォルト: ./）
- `recursive`: 再帰的表示（0=no, 1=yes）
- `pattern`: ファイルパターン（デフォルト: "*"）

#### file_read - ファイル内容読込
```c
typedef struct FILE_READ_RESULT {
    char* content;              // ファイル内容
    int line_count;             // 行数
    size_t file_size;
} FILE_READ_RESULT;
```

**パラメータ**:
- `path`: ファイルパス（必須）
- `start_line`: 開始行（デフォルト: 1）
- `end_line`: 終了行（デフォルト: 100）
- `max_size`: 最大読込サイズ（デフォルト: 1MB）

### 3. LLM Decision Output Format

LLMは以下のJSON形式で判定を返す：

```json
{
  "thought": "現在の状況分析と次のステップについての考察",
  "action": "use_tool" | "final_answer",
  
  "tool_name": "file_list" | "file_read",  // action="use_tool" の場合のみ
  "tool_args": {
    "path": "/path/to/file",
    "start_line": 1,
    "end_line": 50
  },
  
  "answer": "ユーザーへの最終回答",      // action="final_answer" の場合のみ
  "confidence": 0.85                     // 回答の信頼度（オプション）
}
```

### 4. Conversation Storage

```
/conversations/
├── {conversation_id}/
│   ├── metadata.json         # 会話メタデータ
│   ├── history.jsonl         # ステップごとの履歴
│   └── context.txt           # 累積コンテキスト
```

## API Endpoints

### 4.1 `/api/agent/init` - 会話初期化

**Request**
```
POST /api/agent/init
Content-Type: application/json

{
  "user_id": "user123",
  "max_iterations": 5,
  "timeout_seconds": 30
}
```

**Response**
```json
{
  "ok": true,
  "conversation_id": "conv-12345-abcde",
  "created_at": "2026-05-10T12:34:56Z"
}
```

### 4.2 `/api/agent/think` - 思考・判定

**Request**
```
POST /api/agent/think
Content-Type: application/json

{
  "conversation_id": "conv-12345-abcde",
  "user_query": "東京の天気と気温を教えて",
  "available_tools": ["rag_search", "web_search"]
}
```

**Response**
```json
{
  "ok": true,
  "iteration": 1,
  "decision": {
    "thought": "...分析...",
    "action": "use_tool",
    "tool_name": "web_search",
    "tool_args": { "query": "東京 天気" }
  }
}
```

### 4.3 `/api/agent/execute` - ツール実行

**Request (file_list)**
```
POST /api/agent/execute
Content-Type: application/json

{
  "conversation_id": "conv-12345-abcde",
  "tool_name": "file_list",
  "tool_args": {
    "path": "./src",
    "recursive": 0,
    "pattern": "*.c"
  }
}
```

**Request (file_read)**
```
POST /api/agent/execute
Content-Type: application/json

{
  "conversation_id": "conv-12345-abcde",
  "tool_name": "file_read",
  "tool_args": {
    "path": "./src/main.c",
    "start_line": 1,
    "end_line": 50
  }
}
```

**Response**
```json
{
  "ok": true,
  "tool_name": "file_list",
  "result": {
    "entries": ["main.c", "utils.c", "utils.h"],
    "is_directory": [0, 0, 0],
    "count": 3
  },
  "execution_time_ms": 10
}
```

### 4.4 `/api/agent/loop` - 1回のループ実行

**Request**
```
POST /api/agent/loop
Content-Type: application/json

{
  "conversation_id": "conv-12345-abcde",
  "available_tools": ["file_list", "file_read"]
}
```

**Response**
```json
{
  "ok": true,
  "iteration": 2,
  "step": {
    "thought": "src/ディレクトリの内容を確認",
    "action": "use_tool",
    "tool_name": "file_list",
    "result": {
      "entries": ["main.c", "utils.c"],
      "count": 2
    }
  },
  "should_continue": true
}
```

### 4.5 `/api/agent/run` - 完全自動実行

**Request**
```
POST /api/agent/run
Content-Type: application/json

{
  "user_query": "このプロジェクトのC言語のバグを探して",
  "available_tools": ["file_list", "file_read"],
  "max_iterations": 5
}
```

**Response**
```json
{
  "ok": true,
  "conversation_id": "conv-...",
  "final_answer": "null pointer dereference at line 45 in src/main.c...",
  "total_iterations": 3,
  "tool_usage": [
    { "tool": "file_list", "count": 1 },
    { "tool": "file_read", "count": 2 }
  ]
}
```

## Tool Interface

### Tool Executor Signature

```c
typedef int (*ToolExecutor)(
    const char* json_args,      // {"query": "...", "top_k": 5}
    char* output,               // 結果出力バッファ
    size_t output_size          // バッファサイズ
);
// 返値: 0=成功, -1=失敗
```

### ビルトイン Tool Example: File List

```c
static int tool_file_list(const char* json_args, char* output, size_t output_size)
{
    // JSON パース: path, recursive, pattern
    // opendir() でディレクトリ読込
    // JSON形式で結果出力: {"entries": [...], "is_directory": [...], "count": N}
    // 返値: 0 成功 / -1 失敗
}
```

### ビルトイン Tool Example: File Read

```c
static int tool_file_read(const char* json_args, char* output, size_t output_size)
{
    // JSON パース: path, start_line, end_line, max_size
    // fopen() でファイル読込
    // 指定行範囲のみ抽出
    // JSON形式で結果出力: {"content": "...", "line_count": N, "file_size": ...}
    // 返値: 0 成功 / -1 失敗
}
```

## Flow: "Simple Loop" (File System Tools Only)

```
1. ユーザー質問送信
   curl -X POST /api/agent/run -d '{"user_query": "...", ...}'

2. Agent初期化
   - conversation_id 生成
   - max_iterations チェック
   - ツール利用可能リスト検証: ["file_list", "file_read"]

3. Iteration Loop (i=1..max_iterations)
   a) 状態構築
      - 現在のコンテキスト組み立て
      - prompt.conf で思考プロンプト生成
      - 既に読み込んだファイル内容をコンテキストに含める
   
   b) LLM 実行（思考）
      - decide_next_action プロンプト実行
      - JSON 結果解析
   
   c) Action 判定
      - action == "use_tool" → ツール実行（d へ）
      - action == "final_answer" → 回答返却（終了）
   
   d) Tool 実行
      - パラメータバリデーション
      - file_list OR file_read 実行
      - 結果（ファイルリスト/ファイル内容）をコンテキストに追加
      - ループへ戻る（次のファイル探索 or 読込）
   
4. タイムアウト/Max iterations チェック
   - 到達 → 最後の LLM で回答を無理やり生成
   - 未到達 → 継続

5. レスポンス返却
   - conversation_id
   - final_answer
   - tool_usage_log: {"file_list": N, "file_read": M}
   - iterations_used
```

## Safety & Constraints

### 1. Iteration Limits
```c
#define AGENT_MAX_ITERATIONS 10
#define AGENT_TIMEOUT_SECONDS 60
#define AGENT_CONTEXT_MAX_SIZE (1024 * 1024)  // 1MB
```

### 2. Tool Whitelist
```c
// available_tools の検証
// 登録されていないツール呼び出しを拒否
if (!is_tool_registered(tool_name)) {
    return AGENT_ERROR_UNKNOWN_TOOL;
}
```

### 3. Parameter Validation
```c
// JSON Args の型チェック
// 必須フィールド確認
// 値の範囲チェック（top_k=1..100 など）
```

### 4. Resource Limits
```c
// メモリ制限: 各ツール実行は max_output_size を超えない
// タイムアウト: 個別ツール実行 5秒, 全体 60秒
// 重複実行防止: 同じツール呼び出しを n回以上実行しない
```

## State Persistence

### オプション1: メモリ上保持（簡易版）
```c
static AGENT_CONVERSATION* g_agent_conversations[100];
// 利点: 高速
// 欠点: サーバ再起動で消失、スケール困難
```

### オプション2: ファイル保存（推奨）
```
/conversations/{conversation_id}/
├── metadata.json
└── history.jsonl
// 利点: 永続化, 監査可能
// 欠点: I/O オーバーヘッド
```

## Configuration

### `settings.conf` に追加

```
agent_enabled = true
agent_max_iterations = 5
agent_timeout_seconds = 30
agent_context_max_size = 1048576
agent_storage_path = ./conversations/
```

### `prompt.conf` に追加

```
[SYSTEM_AGENT_THINK]
あなたは自律型コードエージェントです。
ファイルシステムツールを使用してコードを探索・分析し、
ユーザーの質問に答えてください。

利用可能なツール:
- file_list: ディレクトリ内容を表示
- file_read: ファイル内容を読込（行範囲指定可能）

返却フォーマット（JSON）:
{
  "thought": "現在のコンテキストを分析し、次のステップを説明",
  "action": "use_tool" | "final_answer",
  "tool_name": "file_list" | "file_read",
  "tool_args": {
    "path": "ファイル/ディレクトリパス",
    "start_line": 1,
    "end_line": 100
  },
  "answer": "ユーザーへの最終回答（action=final_answer の場合のみ）"
}

利用可能ツール: {available_tools}
コンテキスト:
{accumulated_context}
```

## Implementation Phases

### Phase 1: Foundation with File System Tools (Current)
- [ ] `AGENT_CONVERSATION` 構造体実装
- [ ] Tool Registry 実装 (file_list, file_read のみ)
- [ ] `tool_file_list` 実装 (opendir/readdir)
- [ ] `tool_file_read` 実装 (fopen/fgets with line range)
- [ ] `/api/agent/init` エンドポイント
- [ ] `/api/agent/think` エンドポイント（LLM 判定）
- [ ] `/api/agent/execute` エンドポイント（file_list/file_read のみ）
- [ ] `/api/agent/loop` エンドポイント
- [ ] Iteration loop 実装
- [ ] Context accumulation ロジック
- [ ] Timeout/max_iterations チェック

### Phase 2: Code Analysis Tools (Later)
- [ ] `code_analyze` tool（Static Analysis）
- [ ] `generate_tests` tool
- [ ] `run_tests` tool

### Phase 3: Code Generation & Debugging (Later)
- [ ] `generate_code` tool
- [ ] `analyze_error` tool
- [ ] `suggest_fix` tool

### Phase 4: Advanced Features (Later)
- [ ] Conversation persistence
- [ ] Memory optimization
- [ ] Agentic RAG integration
- [ ] Multi-agent coordination

## Data Flow Example: "バグを見つけて"

```
User: "このプロジェクトのC言語コードにあるバグを見つけて"
  │
  ├─ /api/agent/run
  │   └─ conversation_id: "conv-xxx" 生成
  │
  ├─ [Iteration 1] ディレクトリ構造を探索
  │   ├─ LLM Think: "プロジェクト構造を理解する必要がある"
  │   ├─ Action: use_tool (file_list)
  │   ├─ Tool Args: {"path": "./", "recursive": 0}
  │   ├─ Tool Result: {entries: ["src", "include", "test"], is_directory: [1,1,1]}
  │   └─ Context Update: ディレクトリ構造を蓄積
  │
  ├─ [Iteration 2] ソースファイルを探索
  │   ├─ LLM Think: "ソースファイルを検索する"
  │   ├─ Action: use_tool (file_list)
  │   ├─ Tool Args: {"path": "./src", "pattern": "*.c"}
  │   ├─ Tool Result: {entries: ["main.c", "utils.c"], count: 2}
  │   └─ Context Update: ソースファイル一覧を蓄積
  │
  ├─ [Iteration 3] メインファイルを読み込み
  │   ├─ LLM Think: "main.c を読み込んで分析する"
  │   ├─ Action: use_tool (file_read)
  │   ├─ Tool Args: {"path": "./src/main.c", "start_line": 1, "end_line": 100}
  │   ├─ Tool Result: {content: "...code...", line_count: 78, file_size: 2145}
  │   └─ Context Update: main.c の内容を蓄積
  │
  ├─ [Iteration 4] 追加ファイルを読み込み
  │   ├─ LLM Think: "utils.c も読み込んで確認する"
  │   ├─ Action: use_tool (file_read)
  │   ├─ Tool Args: {"path": "./src/utils.c", "start_line": 1, "end_line": 50}
  │   ├─ Tool Result: {content: "...code...", line_count: 45}
  │   └─ Context Update: utils.c の内容を蓄積
  │
  ├─ [Iteration 5] 分析完了
  │   ├─ LLM Think: "十分なコード情報が揃った。null pointer dereference を発見"
  │   ├─ Action: final_answer
  │   └─ Answer: "main.c の line 45: ptr->field にアクセスする前に null チェックがない"
  │
  └─ Response to User
      {
        "conversation_id": "conv-xxx",
        "final_answer": "バグ: null pointer dereference at line 45...",
        "iterations": 5,
        "tool_usage": {"file_list": 2, "file_read": 2}
      }
```

## Sample Implementation

### ディレクトリ構造

新規サンプルは `sample/llm_agent/` 以下に作成します：

```
sample/llm_agent/
├── README.md                    # サンプル説明
├── main.c                       # メインエージェント実装
├── Makefile                     # ビルド設定
├── agent_core.c                 # エージェント コア機能
├── agent_core.h                 # エージェント ヘッダ
├── tools/
│   ├── tool_file_list.c         # file_list ツール実装
│   ├── tool_file_list.h
│   ├── tool_file_read.c         # file_read ツール実装
│   └── tool_file_read.h
├── handlers/
│   ├── handler_init.c           # /api/agent/init ハンドラー
│   ├── handler_think.c          # /api/agent/think ハンドラー
│   ├── handler_execute.c        # /api/agent/execute ハンドラー
│   ├── handler_loop.c           # /api/agent/loop ハンドラー
│   └── handler_run.c            # /api/agent/run ハンドラー
├── www/                         # HTML/JavaScript フロント
│   ├── index.html               # エージェント UI
│   ├── agent.js                 # エージェント クライアント
│   └── style.css                # スタイル
└── test/
    ├── test_file_tools.c        # ツールユニットテスト
    └── Makefile                 # テストビルド
```

### 実装方針

#### Phase 1: 基本実装
1. **agent_core.c / agent_core.h**
   - `AGENT_CONVERSATION` 構造体の実装
   - `create_conversation()`, `destroy_conversation()`
   - Context 管理関数
   - Tool Registry 登録機構

2. **tools/tool_file_list.c**
   - `opendir()` / `readdir()` 使用
   - JSON 形式で結果出力
   - エラーハンドリング

3. **tools/tool_file_read.c**
   - `fopen()` / `fgets()` 使用
   - 行範囲指定機能
   - ファイルサイズチェック（1MB制限）

4. **handlers/*.c**
   - HTTP リクエスト処理
   - JSON パース・バリデーション
   - レスポンス生成
   - エラーハンドリング

5. **main.c**
   - llm_http_stream の main.c をベースに
   - Agent ハンドラーを `/api/agent/*` に登録
   - 既存の `/api/llm/*` エンドポイントと共存

#### Phase 2: フロント実装
- index.html: Agent 対話UI
- agent.js: API クライアント実装
- 会話履歴表示
- ツール実行状況のビジュアル化

#### Phase 3: テスト・ドキュメント
- ユニットテスト
- 統合テスト curl コマンド例
- 使用例・チュートリアル

### ビルド・実行

```bash
# sample/llm_agent にて
make clean
make

# サーバー起動
./agent_server

# テストクライアント (curl)
curl -X POST http://localhost:4444/api/agent/run \
  --data-urlencode 'user_query=このプロジェクトのバグを探して' \
  -H "Content-Type: application/x-www-form-urlencoded"

# または JSON 形式
curl -X POST http://localhost:4444/api/agent/run \
  -H "Content-Type: application/json" \
  -d '{"user_query": "バグを探して", "max_iterations": 5}'
```

## Future Extensions

### Agentic RAG
- Tool の実行結果に基づいて LLM が追加検索の必要性を判定
- 反復的な知識検索

### Multi-Agent Systems
- 複数エージェントの協調作業
- Agent A → Agent B へのタスク委譲

### Dynamic Tool Loading
- プラグイン形式でツール追加
- Runtime での tool registry 更新

### Observability
- Agent の思考プロセスを可視化
- ステップバイステップの説明生成
