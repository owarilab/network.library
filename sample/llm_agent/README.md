# llm_agent — File System Agent Server

LLM を使って自律的にファイルを探索・分析し、質問に答える ReAct エージェントサーバーです。
`network.library` の HTTP サーバー基盤と llama.cpp を組み合わせたシンプルな実装です。

## アーキテクチャ

```
Client (curl / browser)
    │  HTTP POST /api/agent/*
    ▼
qs_llm_agent_server (port 4445)
    │
    ├── /api/agent/init        → 会話ID生成
    ├── /api/agent/think       → LLM推論（1ステップ）
    ├── /api/agent/execute     → ツール実行（1回）
    ├── /api/agent/loop        → think + execute（1サイクル）
    ├── /api/agent/run         → フルReActループ（最終回答まで）
    └── /api/agent/run/stream  → フルReActループ（SSEストリーミング）

利用可能ツール:
    ├── file_list    opendir/readdir でディレクトリ一覧取得
    ├── file_read    fopen/fgets でファイル内容読み取り（行範囲指定可）
    ├── file_search  fnmatch でファイル名パターン検索（再帰対応）
    └── grep_search  ファイル内容をリテラル文字列で検索（行番号付き）
```

## ビルド

### LLM なし（ツール単体テスト用）

```bash
cd sample/llm_agent
make LLAMA_ENABLE=0
```

### llama.cpp あり（LLM 推論有効）

```bash
cd sample/llm_agent
make LLAMA_ENABLE=1
```

初回は llama.cpp が cmake でビルドされます（数分かかります）。  
CUDA を使う場合: `make LLAMA_ENABLE=1 LLAMA_CUDA=1`

## 起動

```bash
cd sample/llm_agent

# モデルパスを指定して起動
export QS_LLM_MODEL_PATH=/path/to/your-model.gguf
export QS_LLM_MAX_TOKENS=512
export QS_LLM_N_CTX=4096

./qs_llm_agent_server
# → [Main] Loaded prompt template from ./prompt.conf (N bytes)
# → [Main] Starting agent server on port 4445...
```

ポートは `settings.conf` の `server_port` で変更できます。
ファイルツールの参照ルートは `settings.conf` の `agent_workspace_root` で変更できます。

### 環境変数一覧

| 変数 | デフォルト | 説明 |
|------|-----------|------|
| `QS_LLM_MODEL_PATH` | (なし) | GGUFモデルファイルのパス |
| `QS_LLM_MAX_TOKENS` | `128` | 最大生成トークン数 |
| `QS_LLM_N_CTX` | `2048` | コンテキストウィンドウサイズ |
| `QS_LLM_N_GPU_LAYERS` | `999` | GPUにオフロードするレイヤー数 |
| `QS_LLM_DEBUG` | (なし) | `1` でデバッグログ有効 |
| `QS_AGENT_PROMPT_FILE` | `./prompt.conf` | プロンプトテンプレートファイル |

## API リファレンス

### `POST /api/agent/init`

会話IDを生成して返します（ステートレス）。

```bash
curl -X POST http://localhost:4445/api/agent/init \
  -H "Content-Type: application/json" \
  -d '{"query":"main.cの関数一覧を教えて","max_iterations":5}'
```

```json
{"conversation_id":"conv-6820a4b312c4ff8a","status":"ready","max_iterations":5,"query":"main.cの関数一覧を教えて"}
```

---

### `POST /api/agent/execute`

ツールを1回実行します（LLM不要）。

```bash
# ディレクトリ一覧
curl -X POST http://localhost:4445/api/agent/execute \
  -H "Content-Type: application/json" \
  -d '{"tool_name":"file_list","tool_args":{"path":".","recursive":0,"pattern":"*.c"}}'

# ファイル読み取り（1〜20行）
curl -X POST http://localhost:4445/api/agent/execute \
  -H "Content-Type: application/json" \
  -d '{"tool_name":"file_read","tool_args":{"path":"./main.c","start_line":1,"end_line":20}}'

# ファイル名パターン検索
curl -X POST http://localhost:4445/api/agent/execute \
  -H "Content-Type: application/json" \
  -d '{"tool_name":"file_search","tool_args":{"path":".","pattern":"*.c","recursive":1,"max_results":50}}'

# ファイル内容テキスト検索
curl -X POST http://localhost:4445/api/agent/execute \
  -H "Content-Type: application/json" \
  -d '{"tool_name":"grep_search","tool_args":{"pattern":"TODO","path":".","file_pattern":"*.c","recursive":1}}'
```

**file_list レスポンス例:**
```json
{"status":"ok","tool_name":"file_list","result":{"entries":["main.c","agent_core.c"],"is_directory":[0,0],"count":2,"path":"."}}
```

**file_read レスポンス例:**
```json
{"status":"ok","tool_name":"file_read","result":{"content":"#include <stdio.h>\n...","lines_read":10,"start_line":1,"end_line":10,"file_size":3120,"truncated":false,"path":"./main.c"}}
```

**file_search レスポンス例:**
```json
{"status":"ok","tool_name":"file_search","result":{"matches":["tools/tool_file_list.c","tools/tool_file_read.c"],"count":2,"truncated":false,"query_path":".","query_pattern":"*.c"}}
```

**grep_search レスポンス例:**
```json
{"status":"ok","tool_name":"grep_search","result":{"matches":[{"file":"main.c","line":12,"text":"#include \"agent_core.h\""}],"count":1,"truncated":false,"query_pattern":"agent_core","query_path":".","query_file_pattern":"*.c"}}
```

---

### `POST /api/agent/run/stream`

フルReActループを実行し、各ステップをSSEイベントとしてストリーミングします。

```bash
curl -X POST http://localhost:4445/api/agent/run/stream \
  -H "Content-Type: application/json" \
  -d '{"query":"main.cの関数を教えて","max_iterations":5}'
```

**SSEイベント一覧:**

| event | データ例 |
|-------|---------|
| `thought` | `{"iteration":1,"thought":"まずディレクトリを確認する"}` |
| `tool_call` | `{"iteration":1,"tool":"file_list","args":{"path":"."}}` |
| `tool_result` | `{"iteration":1,"tool":"file_list","result":{...}}` |
| `answer` | `{"status":"completed","answer":"...","iterations":2}` |
| `done` | `[DONE]` |

```
event: thought
data: {"iteration":1,"thought":"まずディレクトリを確認する"}

event: tool_call
data: {"iteration":1,"tool":"file_list","args":{"path":"."}}

event: tool_result
data: {"iteration":1,"tool":"file_list","result":{"entries":[...]}}

event: answer
data: {"status":"completed","answer":"main.c には ...","iterations":2}

event: done
data: [DONE]
```

---

LLM にプロンプトを送り、次のアクション（ツール使用 or 最終回答）を決定させます。

```bash
curl -X POST http://localhost:4445/api/agent/think \
  -H "Content-Type: application/json" \
  -d '{"query":"このディレクトリのCファイルを教えて","context":""}'
```

```json
{"action":"use_tool","thought":"まずディレクトリ内容を確認する","tool_name":"file_list","tool_args":{"path":".","pattern":"*.c"},"answer":""}
```

---

### `POST /api/agent/loop`

think + execute を1サイクル実行します。クライアントがコンテキストを管理しながらループを回すための API です。

```bash
curl -X POST http://localhost:4445/api/agent/loop \
  -H "Content-Type: application/json" \
  -d '{"query":"Cファイルを教えて","context":"","iteration":0,"max_iterations":5}'
```

**ツール使用の場合:**
```json
{"status":"continue","action":"use_tool","thought":"...","tool_name":"file_list","tool_result":{...},"context":"[tool_call:1]\nfile_list(...)","iteration":1}
```

**最終回答の場合:**
```json
{"status":"done","action":"final_answer","thought":"...","answer":"main.c, agent_core.c の2つです。","iteration":2}
```

---

### `POST /api/agent/run`

最大 `max_iterations` 回のReActループを実行し、最終回答を返します。

```bash
curl -X POST http://localhost:4445/api/agent/run \
  -H "Content-Type: application/json" \
  -d '{"query":"このディレクトリにあるCファイルをすべて教えてください。","max_iterations":5}'
```

```json
{
  "status": "completed",
  "answer": "このディレクトリには main.c, agent_core.c ... があります。",
  "thought": "ディレクトリを一覧します。\nCファイルを確認しました。",
  "conversation_id": "conv-6820a4b312c4ff8a",
  "iterations": 3,
  "max_iterations": 5,
  "tool_use_file_list": 1,
  "tool_use_file_read": 1
}
```

`thought` フィールドには各イテレーションの推論内容が改行区切りで連結されます（Copilot の「Thinking...」に相当）。

---

## クエリ例 — ファイル検索と内容読み取り

### ディレクトリ内の特定ファイルを探す

```bash
curl -s -X POST http://localhost:4445/api/agent/run \
  -H "Content-Type: application/json" \
  -d '{"query":"tools/ ディレクトリにある .c ファイルをすべてリストアップして","max_iterations":5}' | jq
```

### ファイルの内容を読んで分析する

```bash
# バグ・問題点の発見
curl -s -X POST http://localhost:4445/api/agent/run \
  -H "Content-Type: application/json" \
  -d '{"query":"agent_core.c を読んでバグや問題点を見つけてください","max_iterations":8}' | jq

# 特定ファイルの関数一覧
curl -s -X POST http://localhost:4445/api/agent/run \
  -H "Content-Type: application/json" \
  -d '{"query":"main.c に定義されている関数を列挙して","max_iterations":5}' | jq

# サブディレクトリのファイルを読む
curl -s -X POST http://localhost:4445/api/agent/run \
  -H "Content-Type: application/json" \
  -d '{"query":"tools/tool_file_read.c の実装を読んで、start_line/end_line の扱いを説明して","max_iterations":6}' | jq
```

### 複数ファイルをまたいだ調査

```bash
curl -s -X POST http://localhost:4445/api/agent/run \
  -H "Content-Type: application/json" \
  -d '{"query":"handlers/ 以下のファイルを一覧し、handler_run.c の ReAct ループの処理を説明して","max_iterations":10}' | jq
```

> **注意:** サーバーの CWD (`sample/llm_agent/`) が起点です。`..` を含むパスはセキュリティ上拒否されます。
> サブディレクトリのファイルは `tools/tool_file_list.c` のように相対パスで指定できます。

`settings.conf` で `agent_workspace_root` を設定すると、`file_list` / `file_read` はそのディレクトリ配下だけを参照します。
例: `agent_workspace_root='/home/owari/network.library';`

---

## テストの実行

サーバーを起動した状態でテストスクリプトを実行します。

```bash
# ターミナル1: サーバー起動
cd sample/llm_agent
./qs_llm_agent_server

# ターミナル2: テスト実行
cd sample/llm_agent
./test/test_api.sh

# 別ポートの場合
./test/test_api.sh http://localhost:4445
```

LLM なしでも `/api/agent/init` と `/api/agent/execute` のテストは通ります。

## セキュリティ

- `file_list` / `file_read` は `..` を含むパスを**拒否**します（パストラバーサル防止）
- ホワイトリストにないツール名は拒否されます
- サーバーはデフォルトでローカルホストのみへのバインドを前提としています

## ファイル構成

```
sample/llm_agent/
├── Makefile
├── settings.conf        サーバー設定（ポート、agent_workspace_root 等）
├── prompt.conf          エージェント思考プロンプトテンプレート
├── main.c               サーバーエントリポイント・ルーティング
├── agent_core.h / .c    会話管理・ツールレジストリ・LLM出力パーサ
├── tools/
│   ├── tool_file_list.h / .c    file_list ツール実装
│   ├── tool_file_read.h / .c    file_read ツール実装
│   ├── tool_file_search.h / .c  file_search ツール実装
│   └── tool_grep_search.h / .c  grep_search ツール実装
├── handlers/
│   ├── agent_handlers.h         ハンドラ宣言
│   ├── handler_common.h / .c    共通ユーティリティ
│   ├── handler_init.c
│   ├── handler_think.c
│   ├── handler_execute.c
│   ├── handler_loop.c
│   ├── handler_run.c
│   └── handler_run_stream.c     SSEストリーミング対応版
├── test/
│   └── test_api.sh      API テストスクリプト
└── www/                 静的ファイル（index.html など）— http://localhost:4445/ で配信
```
