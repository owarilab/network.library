# http_request Tool Design Document

## Overview

HTTP リクエストを送信するツール。ホワイトリスト制約付きで、エージェントが外部APIを安全に呼び出せるようにする。

---

## File Structure

```
sample/llm_agent/
├── url_whitelist.json              # 新規 — ホワイトリスト定義ファイル（JSON）
├── api_docs.json                   # 新規 — API詳細情報静的ファイル（JSON）
├── agent_core.h                    # 変更 — Whitelist管理関数を追加
├── agent_core.c                    # 変更 — Whitelist実装 + レジトリ登録
├── tools/tool_http_request.h       # 新規 — HTTPリクエストツール定義
├── tools/tool_http_request.c      # 新規 — HTTPリクエスト実行部
├── tools/tool_url_whitelist_get.h # 新規 — ホワイトリスト一覧取得ツール定義
├── tools/tool_url_whitelist_get.c # 新規 — ホワイトリスト一覧取得実装
└── prompt.conf                     # 変更 — ツール説明を追記
```

---

## URL Whitelist Configuration (`url_whitelist.json`)

JSONオブジェクト形式。Cコードでは `api_qs_json_decode_object()`（`qs_api.h`）、qscript では `json_decode()` でパースする共通フォーマット。

```json
{
    "hosts": [
        {
            "host": "api.github.com",
            "description": "GitHub REST API v3"
        },
        {
            "host": "localhost",
            "description": "Local development server (same host)"
        }
    ]
}
```

- `host`: 許可済みホスト名（ポート番号は含まない）
- `description`: エージェント向けの簡易説明（1行）
- 将来的に `max_retries`, `base_path` などのキーを追加可能
- ワイルドカード対応予定: `*.example.com`

---

## API Documentation (`api_docs.json`)

各ホストのAPI詳細を静的JSONで定義。エージェントはオンデマンドで取得する。

```json
{
    "api.github.com": [
        {
            "method": "GET",
            "path_pattern": "/repos/{owner}/{repo}",
            "description": "List repositories for a user",
            "query_params": {
                "type": ["all", "owner", "public", "member", "private"],
                "sort": ["created", "updated", "pushed", "full_name"],
                "per_page": "1-30 (default 30)"
            }
        },
        {
            "method": "GET",
            "path_pattern": "/repos/{owner}/{repo}/issues",
            "description": "List issues for a repository",
            "query_params": {
                "state": ["open", "closed", "all"],
                "per_page": "1-100 (default 30)"
            }
        }
    ]
}
```

- ホスト名をキーとする配列（エンドポイント一覧）
- クエリパラメータの値域やデフォルト値を含める
- ファイルは巨大化しうるため、フィルタリング対応が必要（後述）

---

## Agent Core Changes

### `agent_core.h` 追加関数宣言

```c
/* URL whitelist management */
int         agent_load_url_whitelist(const char* config_path);
const char** agent_get_allowed_hosts(int* count);
int          agent_is_host_allowed(const char* host);
void         agent_free_url_whitelist(void);

/* API documentation loading */
int         agent_load_api_docs(const char* json_path);
char*       agent_get_api_doc_for_host(const char* host, size_t* out_size);
char*       agent_filter_api_docs(const char* host_json, const char* filter_pattern, size_t* out_size);
void        agent_free_api_docs(void);
```

### `agent_core.c` 実装概要

**Whitelistロード（サーバー起動時）**:
1. `url_whitelist.json` をファイル読み込み（全文を char* に確保）
2. `api_qs_json_decode_object(&memory, &obj, json_str)` でデコード
3. `api_qs_object_get_array(&obj, "hosts", &array)` で配列取得
4. ループで各要素から `api_qs_object_get_string(&elem, "host")` を抽出 → 文字列リストに保持
5. `agent_is_host_allowed()` で完全一致チェック（ワイルドカード対応時はパターンマッチ）

**API docsロード（サーバー起動時）**:
1. `api_docs.json` をファイル読み込み → 静的メモリバッファに配置（コピーして所有）
2. 初回アクセス時に `api_qs_json_decode_object()` でデコードし、オブジェクトキー列挙 + 部分一致比較でホスト検索

※ JSON API は `sample/json/main.c` の使用例が実装サンプル。`QS_MEMORY_CONTEXT` はサーバー起動時に確保済み（`g_temporary_memory`）。

---

## Tool: http_request

### エンドポイント

```
POST /api/agent/execute  (既存の汎用実行APIで呼び出し)
```

### JSON入力

```json
{
    "method": "GET",
    "url": "https://api.github.com/repos/owner/repo",
    "headers": {
        "Authorization": "Bearer xxx",
        "Accept": "application/vnd.github.v3+json"
    },
    "body": "{\"key\":\"value\"}",
    "timeout_ms": 30000,
    "max_body_bytes": 1048576
}
```

| フィールド | 必須 | デフォルト | 説明 |
|---|---|---|---|
| `method` | Yes | — | `GET`, `POST`, `PUT`, `DELETE` のみ許可 |
| `url` | Yes | — | 完全URL（`http://` または `https://` のみ） |
| `headers` | No | `{}` | キーバリューペアのオブジェクト |
| `body` | No | `""` | POST/PUTボディ（文字列） |
| `timeout_ms` | No | `30000` | タイムアウト（ミリ秒） |
| `max_body_bytes` | No | `1048576` (1MB) | 受信Body最大サイズ |

### JSON出力（成功時）

```json
{
    "ok": true,
    "status_code": 200,
    "headers": {
        "Content-Type": "application/json",
        "Content-Length": "1234"
    },
    "body": "...",
    "response_time_ms": 142
}
```

### JSON出力（エラー時）

```json
{"error":"host 'evil.com' is not in the allowed hosts whitelist"}
```

| エラーケース | HTTPステータス | 内容 |
|---|---|---|
| ホスト未許可 | 400 | ホワイトリスト違反 |
| URLスキーム不正 | 400 | `http://` / `https://` のみ許可 |
| タイムアウト超過 | 504 | 指定ミリ秒以内に応答なし |
| Bodyサイズ超 | 413 | `max_body_bytes` を超えるレスポンス |
| リダイレクト検知 | 403 | 自動リダイレクトは禁止（手動再送信が必要） |

### JSON入力パース

既存の `tool_json_extract_str`（文字列マッチ簡易パーサ）ではなく、本ツールでは `api_qs_json_decode_object()` を使用する：

```c
QS_MEMORY_CONTEXT mem;
api_qs_memory_alloc(&mem, 64 * 1024); /* 64KB — ツール呼び出しごとに確保/解放 */

QS_JSON_ELEMENT_OBJECT root;
api_qs_json_decode_object(&mem, &root, input_json);

const char* method = api_qs_object_get_string(&root, "method");
const char* url    = api_qs_object_get_string(&root, "url");
```

既存ツール（`file_read`, `grep_search`等）は文字列マッチベースのまま維持。簡易パーサは十分速く、依存も不要なため。

### URLホスト抽出ロジック

外部ライブラリ不使用。簡易パーサで実装：

```c
static int parse_url(const char* url, char* host, size_t host_sz,
                     int* port, char* path, size_t path_sz)
{
    // 1. "http://" または "https://" プレフィックスを検証・除去
    // 2. authority部分の終わりまで探索（"/", "?", "#" の前）
    // 3. ":" がある場合はポート番号をパース、なければデフォルト(80/443)
    // 4. host から "[" "]" を除去（IPv6は未対応）
}
```

### レスポンスJSON構築

```c
QS_MEMORY_CONTEXT mem;
api_qs_memory_alloc(&mem, 32 * 1024);

QS_JSON_ELEMENT_OBJECT resp_obj;
api_qs_object_create(&mem, &resp_obj);
api_qs_object_push_big_integer(&resp_obj, "ok", 1);
api_qs_object_push_big_integer(&resp_obj, "status_code", status_code);

/* headers — キーバリューをループで追加 */
QS_JSON_ELEMENT_ARRAY header_keys;
api_qs_array_create(&mem, &header_keys);
// ...

char* json_str = api_qs_json_encode_object(&resp_obj, 32 * 1024);
```

### Blocking Wrapper 設計

`QS_HTTP_CLIENT_CONTEXT` はイベント駆動なので、同期ラッパーで囲む：

```c
/* tool_http_request_internal.h (internal only) */
typedef struct {
    QS_HTTP_CLIENT_CONTEXT ctx;
    int         done;
    int         status_code;
    long        elapsed_ms;
} HTTP_SYNC_CTX;

int http_sync_request(HTTP_SYNC_CTX* sync, const char* method,
                      const char* host, int port, int is_ssl,
                      const char* path, const char* headers_json,
                      const char* body);
void http_sync_free(HTTP_SYNC_CTX* sync);
```

`http_sync_request()` 内のフロー：
1. `qs_ssl_module_http_client_connect()` で接続開始
2. リクエストバッファ構築（method + path + Hostヘッダ + 追加ヘッダ）
3. bodyがある場合は Content-Length ヘッダを追加
4. `update()` + `sleep()` ループで phase が `DISCONNECT` になるまで待機
5. タイムアウト検出時はループを早期終了

---

## Tool: url_whitelist_get

### JSON入力

```json
{}
```

### JSON出力

```json
{
    "ok": true,
    "hosts": [
        {"host": "api.github.com", "description": "GitHub REST API v3"},
        {"host": "localhost", "description": "Local development server"}
    ],
    "count": 2
}
```

---

## Tool: url_whitelist_info_get

### JSON入力

```json
{
    "host": "api.github.com",
    "endpoint_filter": "issues"   // optional — パスパターンに部分一致するエンドポイントのみ返す
}
```

| フィールド | 必須 | デフォルト | 説明 |
|---|---|---|---|
| `host` | Yes | — | API詳細を取得するホスト名 |
| `endpoint_filter` | No | `""` | パスパターンに部分一致するエンドポイントのみフィルタ（未指定時は全エントリ） |

### JSON出力（成功時）

```json
{
    "ok": true,
    "host": "api.github.com",
    "endpoints": [
        {
            "method": "GET",
            "path_pattern": "/repos/{owner}/{repo}/issues",
            "description": "List issues for a repository",
            "query_params": {
                "state": ["open", "closed", "all"],
                "per_page": "1-100 (default 30)"
            }
        }
    ],
    "count": 1
}
```

### JSON出力（エラー時）

```json
{"error":"no documentation found for host 'unknown.com'"}
```

---

## prompt.conf 追記部分

TOOLSセクションに以下を追加：

```markdown
- url_whitelist_get: List all hosts allowed for http_request with descriptions.
  Args: {}

- url_whitelist_info_get: Get detailed API documentation (endpoints, params) for a host.
  Args: {"host":"api.github.com","endpoint_filter":"issues"}

- http_request: Make HTTP requests to whitelisted hosts only.
  Args: {"method":"GET","url":"https://api.github.com/repos/owner/repo","headers":{},"body":""}
```

---

## Security Design

| 項目 | 対策 |
|---|---|
| ホスト制限 | ホワイトリストのみ許可、それ以外は即400エラー |
| URLスキーム | `http://` と `https://` のみ許可 |
| リダイレクト | `Location` ヘッダを検知したら即終了（自動リダイレクト禁止） |
| Bodyサイズ上限 | `max_body_bytes` デフォルト1MB、内部硬制限4MB（context.body_bufferに合わせ） |
| タイムアウト | デフォルト30秒。超過時はループを早期終了 |
| メソッド制限 | `GET`, `POST`, `PUT`, `DELETE` のみ許可 |
| パス検証 | `/` で始まることを要求（ドメイン注入防止） |

---

## Makefile 変更点

`sample/llm_agent/Makefile`:
- `tools/tool_http_request.c` を OBJGROUP に追加
- `tools/tool_url_whitelist_get.c` を OBJGROUP に追加
- リンク時に `-L ../../ssl/build -lqs_ssl_module -lssl -lcrypto` を確認（既存で通っているはず）

---

## agent_core.c 変更点

```c
#include "tools/tool_http_request.h"
#include "tools/tool_url_whitelist_get.h"

static TOOL_ENTRY g_tool_registry[] = {
    // existing tools...
    { "http_request",       tool_http_request_execute        },
    { "url_whitelist_get",  tool_url_whitelist_get_execute   },
    { "url_whitelist_info_get", tool_url_whitelist_info_get_execute },
    { NULL, NULL }
};
```

---

## Server Startup Changes (`main.c`)

`agent_set_workspace_root()` の後、LLM初期化の前に追加：

```c
if (-1 == agent_load_url_whitelist("./url_whitelist.json")) {
    printf("[Main] Failed to load URL whitelist\n");
    return -1;
}
if (-1 == agent_load_api_docs("./api_docs.json")) {
    printf("[Main] Warning: api_docs.json not loaded (optional)\n");
}
```

---

## Implementation Order

| 順序 | ファイル | 説明 |
|---|---|---|
| 1 | `agent_core.h` / `agent_core.c` | Whitelist管理関数を追加 |
| 2 | `url_whitelist.json` | 設定ファイル作成（テスト用ホスト3-5件） |
| 3 | `tools/tool_url_whitelist_get.h/c` | url_whitelist_get ツール実装 |
| 4 | `api_docs.json` | テスト用のAPIドキュメント作成 |
| 5 | `agent_core.c` (続き) | API docsロード関数を追加 |
| 6 | `tools/tool_url_whitelist_info_get.h/c` | url_whitelist_info_get ツール実装 |
| 7 | `tools/tool_http_request.h/c` | http_request ツール実装 |
| 8 | `main.c` | Whitelist/docsロード処理を追加 |
| 9 | `prompt.conf` | ツール説明を追記 |
| 10 | `Makefile` | 新ファイル登録 |
