# Core ライブラリ開発ガイド

コアライブラリ（`core/`）の開発ガイドを 1 ファイルにまとめたドキュメントです。

## 目次

- [1. コアライブラリ概要](#1-コアライブラリ概要)
- [2. ビルドとリンク](#2-ビルドとリンク)
- [3. メモリ管理](#3-メモリ管理)
- [4. JSON API](#4-json-api)
- [5. CSV API](#5-csv-api)
- [6. ソケット低レベル API](#6-ソケット低レベル-api)
- [7. 高レベル API（サーバ/クライアント）](#7-高レベル-apiサーバクライアント)
- [8. KVS（Key-Value Store）](#8-kvskey-value-store)
- [9. スクリプト実行](#9-スクリプト実行)
- [10. 付随ユーティリティ](#10-付随ユーティリティ)
- [11. サンプル・テスト対応表](#11-サンプルテスト対応表)
- [12. Router(ルームサーバー) 機能まとめ](#12-routerルームサーバー-機能まとめ)
- [13. DB API（datastore）](#13-db-apidatastore)
- [14. ランキング API](#14-ランキング-api)
- [15. WebSocket（クライアント・バイナリ）](#15-websocketクライアントバイナリ)

---

## 1. コアライブラリ概要

### ディレクトリ構成
- core/header: 公開ヘッダ群
- core/src: 実装とビルド用 Makefile
- sample: 利用例

### 主要ヘッダと役割

公開ヘッダは `core/header/` に 26 個あります。

**高レベル（推奨）**

- [core/header/qs_api.h](../../core/header/qs_api.h)
  - 高レベル API（サーバ/クライアント、HTTP/WebSocket、JSON/CSV/KVS/DB/ランキング/Script 等）

**低レベル（ソケット・メモリ・シリアライズ）**

- [core/header/qs_core.h](../../core/header/qs_core.h)
  - OS/ビルド定義、基本マクロ、型・サイズ定義
- [core/header/qs_memory_allocator.h](../../core/header/qs_memory_allocator.h)
  - メモリプール管理
- [core/header/qs_socket.h](../../core/header/qs_socket.h)
  - TCP/UDP の低レベルソケット API
- [core/header/qs_json.h](../../core/header/qs_json.h)
  - JSON パーサ/エンコーダ（低レベル）
- [core/header/qs_csv.h](../../core/header/qs_csv.h)
  - CSV パーサ
- [core/header/qs_string.h](../../core/header/qs_string.h)
  - 文字列ユーティリティ

**データ構造・ユーティリティ**

- [core/header/qs_datastore.h](../../core/header/qs_datastore.h)
  - 汎用永続 DB（低レベル `qs_db_*`。高レベルは 13 章）
- [core/header/qs_ranking.h](../../core/header/qs_ranking.h)
  - インメモリランキング（低レベル。高レベルは 14 章）
- [core/header/qs_hash.h](../../core/header/qs_hash.h)
  - ハッシュテーブル
- [core/header/qs_queue.h](../../core/header/qs_queue.h)
  - キュー（FIFO）
- [core/header/qs_chain_array.h](../../core/header/qs_chain_array.h)
  - 可変長配列（チェーン）
- [core/header/qs_io.h](../../core/header/qs_io.h)
  - ファイル I/O ユーティリティ（開閉・読込・書込・ディレクトリ操作）
- [core/header/qs_system.h](../../core/header/qs_system.h)
  - システムユーティリティ（スリープ・シグナル等）
- [core/header/qs_random.h](../../core/header/qs_random.h)
  - 乱数
- [core/header/qs_base64.h](../../core/header/qs_base64.h)
  - Base64 符号化/復号
- [core/header/qs_sha1.h](../../core/header/qs_sha1.h)
  - SHA1 ハッシュ

**スクリプト関連（script VM）**

- [core/header/qs_script.h](../../core/header/qs_script.h)
  - スクリプトインタプリタ（高レベル API は `qs_api.h`）
- [core/header/qs_token_analyzer.h](../../core/header/qs_token_analyzer.h)
  - トークナイザ
- [core/header/qs_variable.h](../../core/header/qs_variable.h)
  - 変数管理
- [core/header/qs_type.h](../../core/header/qs_type.h)
  - 要素型定義
- [core/header/qs_node.h](../../core/header/qs_node.h)
  - AST ノード
- [core/header/qs_array.h](../../core/header/qs_array.h)
  - 配列（スクリプト用）

**内部（プロトコル・ルーティング・ログ）**

- [core/header/qs_protocol.h](../../core/header/qs_protocol.h)
  - HTTP/WebSocket プロトコル処理（内部実装。静的ファイル配信もここに実装）
- [core/header/qs_packet_route.h](../../core/header/qs_packet_route.h)
  - パケットルーティング（内部実装）
- [core/header/qs_logger.h](../../core/header/qs_logger.h)
  - ロガー（サーバ側 API は `qs_api.h`）

### 推奨の使い分け
- 低レベルで制御したい: qs_socket + qs_memory_allocator
- 早く動かしたい: qs_api
- JSON/CSV を軽量に扱う: qs_api の JSON/CSV ラッパ

### 参照先サンプル
- TCP/UDP サーバ: [sample/server/main.c](../../sample/server/main.c)
- TCP/UDP クライアント: [sample/client/main.c](../../sample/client/main.c)
- JSON: [sample/json/main.c](../../sample/json/main.c)
- CSV: [test/csv_lowlevel_test/main.c](../../test/csv_lowlevel_test/main.c)
- HTTP/WS サーバ: [sample/http_server_simple/main.c](../../sample/http_server_simple/main.c)
- KVS: [test/kvs_create_test/main.c](../../test/kvs_create_test/main.c)
- KVS 永続化: [sample/persistence_cache/main.c](../../sample/persistence_cache/main.c)
- Script: [sample/script/main.c](../../sample/script/main.c)
- Base64/SHA1: [sample/encode/main.c](../../sample/encode/main.c)

---

## 2. ビルドとリンク

### コアライブラリのビルド
- Makefile は [core/src/Makefile](../../core/src/Makefile)
- 生成物: core/libqs_core.a

基本手順:
1. core/src へ移動
2. make を実行

### サンプルのビルド
各サンプルの Makefile は core/libqs_core.a をリンクします。
例:
- [sample/server/Makefile](../../sample/server/Makefile)
- [sample/http_server_simple/Makefile](../../sample/http_server_simple/Makefile)

基本フロー:
1. core を先にビルド
2. sample/XXX で make

### コンパイル時の include
- ヘッダは core/header を参照

### OS 定義
- Linux では -D_LINUX を指定（サンプル Makefile 参照）

### 依存
- pthread を使用（-pthread）

---

## 3. メモリ管理

### 2系統
- 低レベル: [core/header/qs_memory_allocator.h](../../core/header/qs_memory_allocator.h)
- 高レベル: [core/header/qs_api.h](../../core/header/qs_api.h) の `api_qs_memory_*`

### 高レベル API（推奨）
典型フロー:
1. `api_qs_init()`
2. `api_qs_memory_alloc()`
3. 使い終わったら `api_qs_memory_clean()` or `api_qs_memory_free()`

unitバイナリ操作（高レベル）:
- `api_qs_memory_block_used()`
- `api_qs_memory_block_remaining()`
- `api_qs_memory_block_append()`
- `api_qs_memory_block_write()`
- `api_qs_memory_block_read()`
- `api_qs_memory_block_clear()`

ブロック操作・監視（高レベル）:

| 関数 | 説明 |
|---|---|
| `api_qs_memory_create_block(context, size)` | `size` byte のブロックを確保し memid（`int32_t`）を返す（失敗 -1） |
| `api_qs_memory_free_block(context, &memid)` | ブロックを解放し `memid` を -1 にリセット |
| `api_qs_memory_get_pointer(context, memid)` | ブロック先頭のポインタ（`void*`） |
| `api_qs_memory_get_offset_pointer(context, memid, size, offset)` | `offset` 番目の要素のポインタ（`size` = 1 要素の byte 数。配列アクセス用） |
| `api_qs_memory_get_size(context, memid)` | ブロックのサイズ（byte） |
| `api_qs_memory_info(context)` | メモリプール状態を標準出力に出力（デバッグ用） |
| `api_qs_memory_available_size(context)` | プールの空きサイズ（byte） |

memid を使う API（13 章の DB、14 章のランキング、低レベルデータ構造等）は、
`create_block` で確保 → `get_pointer` / `get_offset_pointer` でアクセス → `free_block` で解放、が基本形です。

参考: [sample/json/main.c](../../sample/json/main.c), [sample/script/main.c](../../sample/script/main.c)

### 低レベルメモリプール
- `qs_initialize_memory_f64()` で初期化
- `qs_memory_clean()` / `qs_free()` で解放
- unitバイナリ操作:
  - `qs_memory_unit_data()`
  - `qs_memory_unit_capacity()`
  - `qs_memory_unit_used()`
  - `qs_memory_unit_remaining()`
  - `qs_memory_unit_reset()`
  - `qs_memory_unit_append()`
  - `qs_memory_unit_write()`
  - `qs_memory_unit_read()`
  - `qs_memory_unit_clear()`

参考: [test/csv_lowlevel_test/main.c](../../test/csv_lowlevel_test/main.c)

### 注意点
- JSON/CSV/KVS/Script は内部でメモリプールを参照するため、利用中に `api_qs_memory_clean()` を挟む場合は寿命に注意。
- 一時領域と長期領域を分けると管理しやすい（例: KVS用メモリと一時メモリを分離）。

---

## 4. JSON API

### 高レベル API
ヘッダ: [core/header/qs_api.h](../../core/header/qs_api.h)

代表的な関数:
- 作成: `api_qs_object_create`, `api_qs_array_create`
- 追加: `api_qs_object_push_*`, `api_qs_array_push_*`
- エンコード: `api_qs_json_encode_object`, `api_qs_json_encode_array`
- デコード: `api_qs_json_decode_object`
- 参照: `api_qs_object_get_*`, `api_qs_array_get_*`, `api_qs_object_get_keys`

参考: [sample/json/main.c](../../sample/json/main.c)

### 低レベル API
ヘッダ: [core/header/qs_json.h](../../core/header/qs_json.h)

- メモリプール (`QS_MEMORY_POOL`) を使って JSON をパース/エンコード
- 高速・軽量だが扱いはやや低レベル

### よくある使い方
- レスポンス用 JSON 生成
- KVS のキー列挙を JSON 配列化
- HTTP POST の JSON パース

### その他の関数

| 関数 | 説明 |
|---|---|
| `api_qs_object_exist(object, name)` | キー存在確認（1 = 存在 / 0 = 存在しない） |
| `api_qs_json_escape(src, dst, dst_size)` | JSON 文字列エスケープ（`"` / `\` / `\n` / `\r` / `\t` / 制御文字は `\u00XX`）。成功 0 / 失敗 -1 |
| `api_qs_http_response_raw_json(params, temporary_memory, json)` | 文字列化した JSON をそのままレスポンス本体として送信（Content-Type: application/json）。`QS_JSON_ELEMENT_OBJECT` を通さない既製 JSON の返却に使う |

### 64 ビット整数系

`int32_t` を超える値（ID・時刻等）は `*_big_integer` / `*_unsigned_big_integer` 系を使う:

- オブジェクト:
  - 追加: `api_qs_object_push_big_integer(object, name, int64)` / `api_qs_object_push_unsigned_big_integer(object, name, uint64)`
  - 取得: `api_qs_object_get_big_integer`（ポインタ）/ `api_qs_object_get_big_integer_val`（値）/ 非符号版も同型
- 配列:
  - 追加: `api_qs_array_push_big_integer(array, int64)` / `api_qs_array_push_unsigned_big_integer(array, uint64)`
  - 取得: `api_qs_array_get_big_integer(array, offset)` / `api_qs_array_get_unsigned_big_integer(array, offset)`（ポインタ）

HTTP 連携例: [sample/http_server_simple/main.c](../../sample/http_server_simple/main.c)

---

## 5. CSV API

ヘッダ: [core/header/qs_csv.h](../../core/header/qs_csv.h)

### 主要関数
- 読み込み: `qs_csv_file_load` / `qs_csv_parse`
- 行数/列数: `qs_csv_get_line_length`, `qs_csv_get_row_length`
- 取得: `qs_csv_get_row`
- 書き出し: `qs_csv_build_csv`
- 追加: `qs_csv_add_row`, `qs_csv_add_line`

### 使い方の流れ
1. メモリプールを初期化（`qs_initialize_memory_f64`）
2. CSV を読み込み or 文字列からパース
3. 必要に応じて行/列を追加
4. `qs_csv_build_csv` で CSV 文字列を生成

参考: [test/csv_lowlevel_test/main.c](../../test/csv_lowlevel_test/main.c)

---

## 6. ソケット低レベル API

ヘッダ: [core/header/qs_socket.h](../../core/header/qs_socket.h)

### 基本概念
- `QS_SOCKET_OPTION` にソケット設定とコールバックを集約
- TCP/UDP のサーバ/クライアントを同じ API で扱う
- `qs_server_update` / `qs_client_update` をポーリングして進行

### 最小フロー（サーバ）
1. `qs_create_tcp_server` or `qs_create_udp_server`
2. `set_on_connect_event`, `set_on_payload_recv_event`, `set_on_close_event`
3. `qs_socket` で開始
4. ループで `qs_server_update`

参考: [sample/server/main.c](../../sample/server/main.c)

### 最小フロー（クライアント）
1. `qs_create_tcp_client` or `qs_create_udp_client`
2. `set_on_payload_recv_event`
3. `qs_socket` で開始
4. ループで `qs_client_update`

参考: [sample/client/main.c](../../sample/client/main.c)

### 送信
- サーバから: `qs_send_broadcast`, `qs_send_one`, `qs_send_message`
- クライアントから: `qs_client_send_message`, `qs_client_send`

### Plain モード
- `qs_create_tcp_server_plain` / `qs_create_tcp_client_plain`
- `set_on_plain_recv_event` を利用

### 注意点
- `qs_free_socket` と `qs_free(option->memory_pool)` の組み合わせで解放
- 送受信バッファサイズは `qs_set_recv_buffer` などで調整可能

---

## 7. 高レベル API（サーバ/クライアント）

ヘッダ: [core/header/qs_api.h](../../core/header/qs_api.h)

### サーバ初期化
- `api_qs_server_init(&context, port, max_connection, server_type)`
- `server_type` は `QS_SERVER_TYPE_PLAIN` / `QS_SERVER_TYPE_SIMPLE` / `QS_SERVER_TYPE_HTTP`

### サーバオプション
- `api_qs_set_scheduler(context, mode)` — サーバ内部の system update（タイムアウト・ルーム TTL 等）の更新間隔を設定。
  `QS_SCHEDULER_MODE_HIGH`(1。既定・最速応答) / `QS_SCHEDULER_MODE_MIDDLE`(2) / `QS_SCHEDULER_MODE_LOW`(3。負荷最小)
- `api_qs_set_server_session_timeout(context, timeout_sec)` — データ受信がなく接続経過年数が
  `timeout_sec` を超えた接続を解放する（既定 5 秒。0 で無効）

### イベント登録
- `api_qs_set_on_connect_event`
- `api_qs_set_on_plain_event`
- `api_qs_set_on_simple_event`
- `api_qs_set_on_http_event`
- `api_qs_set_on_websocket_event`
- `api_qs_set_on_close_event`

### サーバループ
- `api_qs_update(context)`
- `api_qs_sleep(context)`

参考: [sample/http_server_simple/main.c](../../sample/http_server_simple/main.c)

### 接続管理

イベントコールバック内で接続を識別・管理する API。いずれも `QS_EVENT_PARAMETER` を第 1 引数に取る:

| 関数 | 説明 |
|---|---|
| `api_qs_get_server_context(params)` | イベントの所有するサーバ context |
| `api_qs_get_connection_id(params)` | 接続 ID（文字列） |
| `api_qs_get_connection_offset(params)` | 接続スロットの offset（`uint32_t`。指定接続送信に使う） |
| `api_qs_get_connection_create_time(params)` | 接続作成時刻（`time_t`） |
| `api_qs_get_connection_data(params)` | 接続に紐付けたユーザデータ（`uint8_t*`） |
| `api_qs_get_connection_data_size(params)` | ユーザデータのサイズ |
| `api_qs_set_connection_data(params, data, data_size)` | ユーザデータを接続に紐付ける（成功 0 / 失敗 -1） |
| `api_qs_send_response_by_connection_offset(context, offset, response)` | 指定 offset の接続へ plain レスポンスを送信（void。範囲外・無効接続は no-op） |

**接続データのパターン**: バイト列を自前構造体にキャストして per-connection 状態として保持する。
`api_qs_set_connection_data` は **`api_qs_server_create_router`（12 章）を先に実行したサーバのみ** 使用可能。

**イベントパラメータ体系**（サーバ側コールバック共通）:

- `QS_EVENT_PARAMETER` の `parameter_type` は 2 値:
  - `QS_EVENT_PARAMETER_TYPE_RECV` — 受信イベント（`params->params` は `QS_RECV_INFO*`。接続情報は `recv_info->tinfo`）
  - `QS_EVENT_PARAMETER_TYPE_CONNECTION` — 接続イベント（`params->params` は `QS_SERVER_CONNECTION_INFO*`）
- 接続系 API・WS 送信 API・HTTP 応答 API はいずれもこの `QS_EVENT_PARAMETER` を受け取る

### HTTP/WS の取得/応答
- 取得: `api_qs_get_http_method`, `api_qs_get_http_path`, `api_qs_get_http_post_body`
- パラメータ:
  - `api_qs_get_http_get_parameter(params, name)` — GET クエリ（URL デコード済み。存在しない名前は NULL）
  - `api_qs_get_http_post_parameter(params, name)` — POST フォーム
  - `api_qs_get_http_cookie_parameter(params, name)` — Cookie
  - `api_qs_get_http_post_json_object(params, &object)` — POST 本体を JSON として `QS_JSON_ELEMENT_OBJECT` へ（4 章の参照 API で読み出し）
- Plain ペイロード（`QS_SERVER_TYPE_PLAIN` の `on_plain_event` コールバック内）:
  - `api_qs_get_plain_payload_type(params)` — 対向が設定した payload 型
  - `api_qs_get_plain_payload(params)` — ペイロード本体（`uint8_t*`）
  - `api_qs_get_plain_payload_length(params)` — ペイロードサイズ
- 応答: `api_qs_send_response`, `api_qs_http_response_json`
- WS: `api_qs_get_ws_message`, `api_qs_send_ws_message`

### ルータ/ログ
- ルータ: `api_qs_server_create_router`, `api_qs_room_create`, `api_qs_room_join`
- サーババウンド ロガー: `api_qs_server_create_logger_access` / `api_qs_server_create_logger_debug` /
  `api_qs_server_create_logger_error`（サーバ起動時の接続・リクエストログ等の出力先を設定）
- 単体ロガー（サーバーなしで使用可）: `api_qs_logger_create(&memory, &logger, QS_LOG_LEVEL_INFO)` → `api_qs_logger_open(&logger, path)` → `api_qs_logger_info(&logger, "fmt %d", v)` / `API_QS_LOG_ERROR(&logger, ...)`（`__FILE__`・`__LINE__` 付き） → `api_qs_logger_free(&logger)`
- レベル: `api_qs_logger_set_level`, `api_qs_logger_get_level`（公開するのはレベルのみ）
- 書き込み関数: `api_qs_logger_debug` / `api_qs_logger_info` / `api_qs_logger_warn` / `api_qs_logger_error`
  / `api_qs_logger_close`（フラッシュしてクローズ）。いずれも `printf` 形式
- マクロ: `API_QS_LOG_DEBUG/INFO/WARN/ERROR(logger, fmt, ...)`（`__FILE__`・`__LINE__` を自動付与）
- レベル定数（閾値）: `QS_LOG_LEVEL_DEBUG`(0) < `QS_LOG_LEVEL_INFO` < `QS_LOG_LEVEL_WARN` <
  `QS_LOG_LEVEL_ERROR` < `QS_LOG_LEVEL_NONE`（NONE は全出力を抑制）

### クライアント
- `api_qs_client_init(&context, host, port, server_type)`
- イベント: `api_qs_set_client_on_connect_event`, `api_qs_set_client_on_plain_event`, `api_qs_set_client_on_simple_event`, `api_qs_set_client_on_close_event`
- 送信: `api_qs_client_send`, `api_qs_client_send_message`
- ループ: `api_qs_client_update`, `api_qs_client_sleep`
- リファレンス: `api_qs_client_get_context(params)`（コールバック内で context 取得）,
  `api_qs_client_get_socket(context)`（socket fd）
- 低レベルフック（生送受信の差し替え・計測用）:
  `api_qs_client_set_lowlevel_send_hook` / `api_qs_client_set_lowlevel_recv_hook`、
  サーバ側も `api_qs_server_set_lowlevel_send_hook` / `api_qs_server_set_lowlevel_recv_hook`
- 解放: `api_qs_client_free(context)` — 接続・メモリを含む全リソースを解放（ライフサイクルの最後に必ず呼ぶ）
- WebSocket クライアントは 15 章を参照

参考: [sample/client_simple/main.c](../../sample/client_simple/main.c)

---

## 8. KVS（Key-Value Store）

ヘッダ: [core/header/qs_api.h](../../core/header/qs_api.h)

### 作成
- 直接作成（サーバ外でも使用可）: `api_qs_kvs_create_b1mb` 〜 `api_qs_kvs_create_b1024mb`（9 サイズ: 1/8/16/32/64/128/256/512/1024MB）
- サーバ KVS: `api_qs_server_create_kvs`（下記）

### サーバ KVS
- `api_qs_server_create_kvs(context, type)` — サーバ context に専用メモリプールで KVS を作成。
  `type` は `QS_KVS_MEMORY_TYPE_B1MB`(既定) / `B128MB` / `B256MB` / `B512MB` / `B1024MB` の 5 サイズ
- `api_qs_server_get_kvs(context, &kvs)` — 基本操作用の `QS_KVS_CONTEXT` を取得（未作成は -1 を返す）
- `api_qs_kvs_memory_info(context)` — サーバ KVS のメモリ状態を出力
- サーバ KVS が存在すると組み込み HTTP API `/api/v1/mem/*` が有効になる（12 章）

### 基本操作
- 追加/更新: `api_qs_kvs_set`
- 取得: `api_qs_kvs_get`
- 削除: `api_qs_kvs_delete`
- キー数: `api_qs_kvs_key_length`
- サイズ: `api_qs_kvs_get_buffer_size`
- キー列挙: `api_qs_kvs_keys`, `api_qs_kvs_sorted_keys`

### 永続化
- `api_qs_kvs_create_b1mb_persistence` 〜 `api_qs_kvs_create_b1024mb_persistence`（9 サイズ。ファイルパス指定）
- 終了時: `api_qs_persistence_kvs_memory_free`

参考:
- メモリ KVS: [test/kvs_create_test/main.c](../../test/kvs_create_test/main.c)
- 永続化 KVS: [sample/persistence_cache/main.c](../../sample/persistence_cache/main.c)

### 典型パターン
- 大量登録時は一時メモリを別に確保し、`api_qs_memory_clean` を繰り返す
- JSON でキー一覧を返すなどの用途に向く

---

## 9. スクリプト実行

ヘッダ: [core/header/qs_api.h](../../core/header/qs_api.h)

`qs_script` は、C言語で記述された軽量なインタープリタ型スクリプトエンジンです。本ライブラリ内で HTTP / API 設定ファイルの読み込みやバッチ処理を実行するために使用できます。

### 基本手順
1. `api_qs_memory_alloc`
2. `api_qs_script_read_file`
3. `api_qs_script_run`
4. `api_qs_script_get_parameter`

参考: [sample/script/main.c](../../sample/script/main.c)

### 引数の受け渡し
- `api_qs_script_set_argv_object`
- `api_qs_script_set_argv_string`
- `api_qs_script_set_argv_integer`

### 典型用途
- 設定ファイルの読み込み（HTTP サーバ設定など）
- バッチ処理の実行

---

### qs_script 構文仕様

インタープリタの文法や機能の詳細は、[core/src/qs_script.c](../../core/src/qs_script.c)、[core/header/qs_token_analyzer.h](../../core/header/qs_token_analyzer.h)、[core/header/qs_type.h](../../core/header/qs_type.h) および [sample/script/sample/sample.qscript](../../sample/script/sample/sample.qscript) などの実装に基づいています。

#### 1. サポートされているデータ型
`qs_script` では、以下のデータ型が動的に管理されます。

- **数値 (Numeric)**: 内部的に `int32_t` なので実質32ビット整数。
- **浮動小数点数 (Float)**: パース上の定義はあるが基本演算時のキャスト型に制約あり。
- **文字列 (String)**: `"..."` で囲まれたテキスト。
- **配列 (Array)**: `[elem1, elem2, ...]` のように記述する0始まりのシーケンス。
- **ハッシュ (Hash)**: `[ "key1" : value1, "key2" : value2 ]` のように記述する連想配列。
- **バイナリ (Binary)**: `file_get(file_path, "b")` 等で返されるバイナリリテラル。

#### 2. コメント
- 単一行コメント: `//` から行末まで。
- 複数行コメント: `/*` から `*/` まで。

#### 3. 変数
- 変数は明示的な定義キーワード（例:`var`, `let`）を持たず、代入式により自動的に定義されます。
- スコープは定義位置に応じたグローバルおよびローカルスコープを形成します。
- 配列やハッシュの多次元またはネスト形式によるアクセス（例: `config["int_array"][2]`）に対応。

#### 4. 演算子
`qs_script` は Cライクな豊富な演算子を処理できます。

| 分類 | 演算子 | 説明 |
| --- | --- | --- |
| 代入 | `=`, `+=`, `-=`, `*=`, `/=`, `%=`, `^=`, `~=`, `\|=`, `&=` | 単純および複合代入演算 |
| 単項 | `-` | 単項マイナス（負の数定義） |
| 算術 | `+`, `-`, `*`, `/`, `%` | 数値は加減乗除・剰余。オペランドのいずれかが文字列、バイナリの場合、`+` は結合を意味します |
| 比較 | `==`, `>`, `<`, `>=`, `<=` | 数値同士の各種比較、または文字列同士の `==`/`!=`（一致・不一致）判定 |
| 論理 | `&&`, `\|\|` | 論理AND、論理OR |
| 後置 | `++`, `--` | 変数値に対する後置インクリメント・デクリメント |

*(重要：パースロジックの不具合により、不一致 `!=` 演算子はスクリプト内のコードに記述できません。実質 `==` での反転や条件整理による代替が必要です。)*

#### 5. 制御構造

##### 条件分岐: `if` / `elseif` (または `else if`) / `else`
```qscript
if (x > 10) {
    echo("greater than 10\n");
} else if (x == 10) {
    echo("equals 10\n");
} else {
    echo("less than 10\n");
}
```
※ `else if` のようにスペースを空けて2単語で記述された場合、パーサーが内部で `"elseif"` として自動変換して正しく処理します。

##### ループ分岐: `while`
```qscript
idx = 0;
while (idx < 5) {
    echo("idx = " + idx + "\n");
    idx++;
}
```
※ トークン予約語として `loop` が内部で定義されていますが、構文解析器および実行器上には実装されていないため、ループ制御としては `while` を用います。

#### 6. ユーザー定義関数
`def` キーワードを用いて関数を定義し、`return` により戻り値を返せます。

```qscript
def add(a, b) {
    return a + b;
}

result = add(10, 20);
echo(result + "\n");
```
※ アロー関数 `=>` も字句解析レベルで予約されているものの、現実装では実質的な無名関数コード定義として動作しないため、関数はすべて通常の `def` 定義を用います。

---

### 組み込みシステム関数 (Built-In Functions)

スクリプトエンジンにデフォルトで組み込まれており、追加手続きなしで直接呼び出せるシステム関数一覧です。[core/src/qs_api.c](../../core/src/qs_api.c) にてグローバルに登録されています。

| 関数名・引数 | 戻り値の型 | 説明 |
| --- | --- | --- |
| `echo(value)` | なし | コンソールへ `value`（数値、文字列、配列、ハッシュ）を出力。配列やハッシュの場合は内部データ構造をダンプします。 |
| `count(value)` | 数値 | 引数の長さを取得。文字列はバイト長、配列は要素数、ハッシュはキーエントリ数を返します。 |
| `file_exist(file_name)` | 数値 | 指定のファイルが存在すれば `1`、そうでなければ `0` を返します。 (引数は文字列) |
| `file_size(file_name)` | 数値 | 指定のファイルのバイトサイズを返します。エラー時は `0`。 (引数は文字列) |
| `file_extension(file_name)`| 文字列 | 指定のファイルの拡張子を取得します。 (引数は文字列) |
| `file_get(file_name[, mode])` | 文字列/バイナリ | 指定のファイル内容を全ロード。第二引数に `"b"` を渡した場合はバイナリデータとして強制的（バイト数ジャスト）に読み込みます。 |
| `file_put(file_name, data)`| 文字列 | `data` をファイル `file_name` に上書き保存。成功時は `"1"` 、失敗時は `"0"` を返します。 |
| `file_add(file_name, data)`| 文字列 | `data` をファイル `file_name` に追記保存。成功時は `"1"` 、失敗時は `"0"` を返します。 |
| `json_encode(value)` | 文字列 | 渡された配列やハッシュを JSON 文字列にエンコードして返します。 |
| `json_decode(json_str)` | 各種 | 指定された JSON 文字列をパースし、配列やハッシュ、プリミティブ型にデコードして返します。 |
| `gmtime()` | 文字列 | 現在の UTC 時間を示す文字列を取得します。 |
| `rand()` | 数値 | 32ビットの非負のランダム整数を生成します。 |
| `csv_read(file_path)` | 配列(2次元) | CSVファイルをロードして配列の配列（2次元配列形式）として返します。 |
| `csv_build(array[, bufsize])` | 文字列 | 2次元配列を CSV 形式のテキスト文字列にシリアライズします。任意でバッファサイズ `bufsize` を指定可能。 |
| `csv_write(file_path, array[, bufsize])` | 文字列 | 2次元配列を指定の `file_path` へ直接 CSV ファイルとして書き出します。成功時は `"1"`、失敗時は `"0"`。 |

---

## 10. 付随ユーティリティ

### Base64 / SHA1
ヘッダ: [core/header/qs_api.h](../../core/header/qs_api.h)
- `api_qs_base64_encode`
- `api_qs_base64_decode`
- `api_qs_sha1_encode`

参考: [sample/encode/main.c](../../sample/encode/main.c)

### ランダム
- `api_qs_rand`
- `api_qs_uniqid`

### 文字列
ヘッダ: [core/header/qs_string.h](../../core/header/qs_string.h)
- URL エンコード/デコード: `qs_urlencode`, `qs_urldecode`
- 文字列連結/コピー: `qs_strlcat`, `qs_strcopy`
- 時刻文字列: `qs_utc_time`

### ハッシュ
- [core/header/qs_hash.h](../../core/header/qs_hash.h) に低レベル API

### キュー / チェーン配列（データ構造）
- キュー: [core/header/qs_queue.h](../../core/header/qs_queue.h)
  - `qs_create_queue` / `qs_enqueue` / `qs_safe_enqueue`（満杯時エラー返却）/ `qs_dequeue` /
    `qs_read_queue`（チャンク読み出し）/ `qs_get_queue_length`
- チェーン配列: [core/header/qs_chain_array.h](../../core/header/qs_chain_array.h)
  - `qs_create_chain_array` / `qs_add_chain(_i)` / `qs_get_chain(_i)` / `qs_remove_chain` /
    `qs_resize_chain_array` / `qs_get_chain_length` / `qs_get_chain_size`

### ファイル I/O
- [core/header/qs_io.h](../../core/header/qs_io.h)
  - 開閉: `qs_finit` / `qs_fopen` / `qs_fclose` / `qs_fget_info`
  - 読み込み: `qs_fread` / `qs_fread_range`（位置指定）/ `qs_fread_bin(_range)`
  - 書き込み: `qs_fwrite` / `qs_fwrite_a`（追加）/ `qs_fwrite_bin(_a)` / `qs_fout(_line)` / `qs_fputchar(_line)`
  - ファイル・ディレクトリ操作: `qs_mkdir` / `qs_rmdir` / `qs_unlink` / `qs_frename` / `qs_chmod_char` / `qs_ls` / `qs_lstate`

### システム
- [core/header/qs_system.h](../../core/header/qs_system.h)
  - スリープ: `qs_sleep`
  - スケジューラ: `qs_initialize_scheduler`（`_high_speed` / `_middle_speed` / `_low_speed` 版）/ `qs_update_scheduler`
    （高レベルの `api_qs_set_scheduler` はこれらをラップ、7 章）
  - シグナル: `qs_set_defaultsignal` / `qs_set_sigaction` / 既定ハンドラ群
  - その他: `qs_daemonize`（デーモン化）、`qs_getopt`（引数解析）、`qs_localtime`、
    `gnt_get_argc` / `gnt_get_argv`（argv アクセス）

### 乱数（低レベル）
- [core/header/qs_random.h](../../core/header/qs_random.h)
  - `qs_srand_32/64`（シード設定）/ `qs_rand_32/64` / `qs_uniqid_r32/r64`（reentrant 版）

---

## 11. サンプル・テスト対応表

サンプル（`sample/`）とテスト（`test/`）の一覧は別ファイルに分離されています。

- [samples_tests.md](samples_tests.md) — サンプル一覧（ネットワーク / データ処理 / KVS / DB / スクリプト / アプリケーション例 / Unity）とテスト一覧・実行手順
- 最小の入口サンプルだけが必要な場合は、1 章の「参照先サンプル」を参照


---

## 12. Router(ルームサーバー) 機能まとめ

本セクションは `qs_api` の router 機能（ルームサーバー）と、`sample/http_server_simple` の WebSocket + Web API を使ったルームチャット動作を、実装ベースで整理したものです。

### 1. router(ルーム)とは

router は「ルームID(=route key) → 接続(connection)の集合」を管理する仕組みです。

- ルームは `qs_packet_route` によって管理され、**ルームID**（ランダム文字列）で参照します。
- WebSocket 接続ごとに **connection_id**（ランダム文字列）が割り当てられ、HTTP API で join すると `connection_id` がルームに紐づきます。
- WebSocket で `api_qs_send_ws_message()` を呼ぶと、送信者がルームに参加済みなら **同一ルームへブロードキャスト**、未参加なら **echo** になります。

### 2. 有効化方法（サーバ側）

HTTP/WebSocket サーバで router を使うには以下を行います。

1. サーバ初期化
2. router 生成
3. WebSocket/HTTP のイベント処理を設定（任意）

代表例は `sample/http_server_simple/main.c` です。

- `api_qs_server_init(..., QS_SERVER_TYPE_HTTP)`
- `api_qs_server_create_router(context)`
- `api_qs_set_on_websocket_event(context, on_ws_event)`
- `api_qs_set_on_http_event(context, on_http_event)`

※ `api_qs_exec_http()` 側に **組み込み Web API** があり、ステータスが 404 のときだけ `on_http_event` にフォールバックします。

### 3. Web API（組み込み /api/v1/room/*）

これらは `QS_SERVER_TYPE_HTTP` のサーバで自動的に有効になります（router を作っていることが前提）。

#### 3.1 ルーム作成

- `POST /api/v1/room/create`
- `Content-Type: application/x-www-form-urlencoded`
- パラメータ
  - `name` : ルーム名（最大 32 文字）

レスポンスは JSON（ルーム情報）です。

#### 3.2 ルーム一覧

- `GET /api/v1/room/list` または `POST /api/v1/room/list`

レスポンス例（概形）:

- `list`: ルーム配列
  - `id`: ルームID
  - `max`: 収容上限（デフォルト 10）
  - `connection`: 接続数
  - `created` / `updated`: epoch 秒
  - `owner_id`: オーナーの connection_id（未設定は空文字列）
  - `data`: ルームデータ（文字列）

`data` には `room/create` 時に格納された JSON 文字列（例: `{ "name": "test_room" }`）が入ります。

#### 3.3 ルーム参加

- `POST /api/v1/room/join`
- `Content-Type: application/x-www-form-urlencoded`
- パラメータ
  - `room_id`: 参加先ルームID
  - `connection_id`: WebSocket ハンドシェイク後に割り当てられた connection_id

成功時は JSON（ルーム情報）を返し、同時に WebSocket で join 通知が配信されます。

#### 3.4 KVS API（組み込み /api/v1/mem/*）

`api_qs_server_create_kvs`（8 章）でサーバ KVS を作成したサーバで自動的に有効になります。
すべて `POST`（form パラメータ）。

| エンドポイント | パラメータ | 説明 |
|---|---|---|
| `/api/v1/mem/set` | `k`, `v` | KVS に set（レスポンス: `k` / `v` / `result`） |
| `/api/v1/mem/get` | `k` | KVS から get（レスポンス: `k` / `v`。存在しない値は空文字列） |
| `/api/v1/mem/delete` | `k` | KVS から削除（レスポンス: `k`） |
| `/api/v1/mem/keys` | なし | キー一覧（レスポンス: `len` / `keys` 配列） |

```sh
curl -X POST http://localhost:8080/api/v1/mem/set -d 'k=memk1&v=memvalue1'
curl -X POST http://localhost:8080/api/v1/mem/get -d 'k=memk1'
```

### 4. WebSocket メッセージ形式（qs_make_ws_message_simple）

router が有効な場合、`api_qs_send_ws_message()` は WebSocket フレームに以下 JSON を載せて送ります。

```json
{ "id": "<sender_connection_id>", "type": "message|join|leave", "message": "<string>" }
```

- `id`: 送信者の connection_id
- `type`:
  - `message`: クライアントが送ったテキスト（またはサーバが指定した文字列）
  - `join`: join 発生
  - `leave`: close/leave 発生
- `message`:
  - `message` の場合: 本文文字列
  - `join/leave` の場合: **ルーム情報 JSON をさらに文字列として埋め込んだもの**

### 5. ブロードキャスト条件（room 未参加時は echo）

- 送信者がルームに join 済み: ルーム内の全 WebSocket 接続へ送信
- 送信者が未 join: 送信者へ echo

この挙動により、サンプルでは「接続直後は自分に返る → join 後はルームへ飛ぶ」という流れになります。

### 6. join/leave 時の動作

#### 6.1 join

`/api/v1/room/join` 成功時に、サーバは同一ルーム内の接続へ `type=join` を送ります。

また、ルームにオーナーが存在しない場合、最初に join した接続が `owner_id` になります。

#### 6.2 leave（切断）

WebSocket 切断時に、サーバは以下を行います。

- ルームから接続を削除
- オーナーだった場合は別接続へオーナーを移譲
- ルーム内へ `type=leave` を通知

### 7. ルームの寿命（TTL）

- デフォルト TTL は `QS_PACKET_ROUTE_LIFE_TIME_SEC_DEFAULT`（5 分）
- ルームが空で、かつ TTL 経過した場合に削除されます
- `api_qs_update()` 内で約 60 秒ごとに `qs_update_packet_route()` が呼ばれて掃除されます

### 8. サンプル: http_server_simple ルームチャット

対象: `sample/http_server_simple`

#### 8.1 起動

- ビルド: `make`
- 実行: `./qs_http_server`
- ポート: `server.conf` の `server_port`（デフォルトは 4444）

#### 8.2 クライアント

`sample/http_server_simple/www/wsock.html` は以下の流れで動きます。

1. `ws://localhost:<port>` に WebSocket 接続
2. 接続直後にランダム文字列（uuid）を 1 回送る
3. サーバからの echo を受け取り、`json.id` を **自分の connection_id** として採用
4. `GET /api/v1/room/list` を叩き、
   - ルームがあれば最初のルームに join
   - なければ `POST /api/v1/room/create` で `test_room` を作って join
5. join/leave を監視し、他ユーザ join 時にメッセージ送信（デモ用）

ブラウザの別タブで `wsock.html` を複数開くと、同じルームに join し、WebSocket 送信がルーム内にブロードキャストされます。

### 9. 静的ファイル配信

`QS_SERVER_TYPE_HTTP` のサーバは、`./www`（CWD 基準）を document root として静的ファイルを配信します。
`http_request_common`（[core/src/qs_protocol.c](../../core/src/qs_protocol.c)）が担当し、
リクエストはまず静的ファイルとして `./www` を検索され、ファイルが見つからなかった（404）場合に
組み込み `/api/v1/*` ルート → `api_qs_set_on_http_event` のユーザ callback の順で処理されます。

- `/` は `www/index.html` を返す（index.html が既定のドキュメント）
- 見つからないファイルは 404
- 拡張子に応じた Content-Type:
  - `html` → `text/html` / `css` → `text/css; charset=UTF-8` / `js` → `text/javascript; charset=UTF-8`
  - `json` → `application/json; charset=UTF-8` / `png` / `jpg` / `ico` → 画像系
  - `unityweb` → `application/octet-stream` など
- キャッシュ制御（html / css / js / json）:
  - レスポンスに `Cache-Control: max-age=30` と `Last-Modified` を付与
  - リクエストに `Cache-Control: max-age=0` かつ `If-Modified-Since` が付いていれば **304 Not Modified** を返す

参考:
- [sample/http_server_simple/www/](../../sample/http_server_simple/www/)（`index.html`・`css/`・`js/`・WebSocket デモ `wsock.html` 等）
- [test/static_file_test/](../../test/static_file_test/)（Content-Type・キャッシュ制御・304 の検証）

関連 API（ヘッダ）:

- `api_qs_server_create_router()`
- `api_qs_room_create()` / `api_qs_room_list()` / `api_qs_room_join()` / `api_qs_room_leave()`
- `api_qs_send_ws_message()`

---

## 13. DB API（datastore）

ヘッダ: [core/header/qs_api.h](../../core/header/qs_api.h)（高レベル `api_qs_db_*`）/ [core/header/qs_datastore.h](../../core/header/qs_datastore.h)（低レベル `qs_db_*`）

永続化 KVS（mmap ベース固定容量ストア）を基盤とした汎用の永続 DB です。
スキーマ・インデックス付きでレコード CRUD ができ、`QS_SERVER_CONTEXT` に依存しないため CLI・サーバの両方で使用できます。

### ストレージ構成

- 1 database = 1 ディレクトリ（`base_dir/database`）
- 1 table = 1 `.db` ファイル
- `schema.db`: テーブル名 → スキーマ JSON
- `index.db`: `index:{table}:{field}:{value}` → `[id,...]`
- `system.db`: id 採番カウンタ等のメタ情報
- テーブル容量: `QS_DB_CAPACITY_B1MB` 〜 `QS_DB_CAPACITY_B1024MB`（9 サイズ、既定 1MB。作成時に system.db へ保存され、以降の操作は保存値が使われる）
- `system` / `schema` / `index` は予約名（table の create/drop 不可）
- database/table 名は ASCII の英数字・`_`・`-` のみ（1〜45 byte）

### 高レベル API（推奨）

典型フロー:
1. `api_qs_db_init(&db)`（内部で 16MiB の固定メモリプールを確保）
2. スクラッチメモリ: `api_qs_memory_alloc(&scratch, size)` で 1 回確保し、
   結果を格納する関数（`show` 系 / `schema_validate` / `record_list` / `index_rebuild`）の
   直前に `api_qs_memory_clean(&scratch)` でリセット
3. `api_qs_db_open_database(&db, base_dir, database)`（同一 directory の open/drop は advisory flock 等で排他）
4. 各操作（下記）
5. 終了時: `api_qs_db_close_database` → `api_qs_db_free` → `api_qs_memory_free` の順で解放

参考: [sample/db_api_cli/main.c](../../sample/db_api_cli/main.c)（mysql-cli 風 REPL。コマンド一覧・例は [sample/db_api_cli/README.md](../../sample/db_api_cli/README.md)）

### Database 操作

- 作成: `api_qs_db_create_database(base_dir, database)`
- 削除: `api_qs_db_drop_database(base_dir, database)`
- 一覧: `api_qs_db_show_database(memory, array, base_dir)`（`QS_JSON_ELEMENT_ARRAY` へ）
- オープン: `api_qs_db_open_database(context, base_dir, database)`（使用中 context の再 open は拒否）
- クローズ: `api_qs_db_close_database(context)`（公開 table handle が残ると 1 を返して保留。全 handle を閉じてから再呼び出し）

### Table 操作

- 作成: `api_qs_db_create_table(context, table, capacity_mb)`
- 削除: `api_qs_db_drop_table(context, table)`（open 中の handle がある場合は先に close。削除は非原子的）
- 一覧: `api_qs_db_show_table(memory, context, array)`（system/schema/index は除外）
- 名前付き handle: `api_qs_db_table_open` / `api_qs_db_table_close`（最大 64 個。
  record/list/index 操作はテーブルを内部で自前オープンするため、**操作前に handle を閉じる**こと）

### スキーマ

- 定義: `api_qs_db_schema_define(context, table, schema_json)`（JSON を解釈せずそのまま保存。形式確認は validate 時）
- 取得: `api_qs_db_schema_get(context, table, out_buf, out_size)`
- 検証: `api_qs_db_schema_validate(memory, context, table, record_json)`
- 保証される型: `int32` / `int64` / `string`、`required`、string `maxLength`（未知の field・重複 field・配列は検証/適用しない）

```json
{
  "fields": [
    {"name": "id", "type": "string", "maxLength": 64, "required": 1, "is_index": 1},
    {"name": "age", "type": "int32", "required": 0, "is_index": 1},
    {"name": "status", "type": "string", "maxLength": 32, "required": 1, "default": "open"}
  ]
}
```

**高レベル API のみ**: スキーマ定義済みの table では `record_insert` / `record_update` が
**先に** `default` 定義の欠落 field に値を補完し、**その後に**補完済みレコードへ型・`required`・`maxLength` を
自動検証して保存する（入力に明示された値は default で上書きしない）。
検証は補完後に行われるため、`default` を持つ field は `required: 1` でも入力省略で検証に落ちない。
補完された field 名がスキーマ内に重複している場合は初回のみ適用される。
スキーマ未定義の table は検証・補完なしで入力 JSON がそのまま保存される。

### レコード CRUD

- 挿入: `api_qs_db_record_insert(context, table, id, record_json)`（重複 id はエラー）
- 取得: `api_qs_db_record_get(context, table, id, out_buf, out_size)`
- 更新: `api_qs_db_record_update(context, table, id, record_json)`（未存在 id はエラー）
- 削除: `api_qs_db_record_delete(context, table, id)`（未存在でも OK・冪等）
- 一覧: `api_qs_db_record_list(memory, context, table, filter_json, scan_mode)` → JSON 配列文字列（scratch メモリ内を指す）
  - `scan_mode`: `QS_DB_SCAN_FULL`（全レコード走査）/ `QS_DB_SCAN_INDEX`（index 検索）
  - フィルタ値は**常に文字列**（例: `{"status":"open","age":"30"}`）
  - 0 件は `"[]"`、エラーは NULL。結果は終端 NUL 含め 64KB 以下
  - INDEX では index が最新であることが前提。未構築・更新漏れでは結果が欠落し得る
    （index 不在による空結果と該当なしは区別されない）

### インデックス

- 追加: `api_qs_db_index_add(context, table, field, value, record_id)`
- 削除: `api_qs_db_index_remove(context, table, field, value, record_id)`
- 再構築: `api_qs_db_index_rebuild(memory, context, table, indexed_fields_json)`
  （`["field1","field2"]` 形式。失敗時は部分的な index が残り得るため、そのまま検索せず再構築する）
- キー形式: `index:{table}:{field}:{value}`。キー全体が **64 文字未満**（63 文字まで。
  `QS_DB_MAX_KEY` = `QS_DB_INDEX_KEY_MAX` = 64）のみ受理。table / field / value / record_id も
  個別に同じ制約（空でない・64 文字未満・制御文字・`/`・`\` 不可）に従う

#### 自動 index 更新

- 有効化: `api_qs_db_set_auto_index_update(context, enable)`（**デフォルトは無効**）
- 対象: スキーマで `"is_index": 1` の field（`string` / `int32` / `int64` のみ。他はスキップ）
- **書き込み成功後に**フィールド単位で同期:
  - insert → `index_add`
  - update → 旧レコードと比較し、変更分のみ `index_remove`（旧値）+ `index_add`（新値）
  - delete → 旧レコードの各フィールド `index_remove`
- 同期対象は **default 補完後の保存 JSON**。`default` で補完された値も自動で index に載る
- update/delete の差分計算は書き込み前に旧レコードを 1MiB バッファで取得して行う。
  バッファ超過等で旧レコードが取得できない場合、update は旧値なしで同期するため旧値の
  index エントリは除去されず、delete は同期自体を行わない（いずれもエラーにはならない。
  誤った index エントリが残っても検索は実レコードで再照合されるため結果は誤らない）
- 整数は正規 10 進文字列で index されるため、整数フィールドの検索も文字列フィルタ
  （`{"age":"30"}`）で INDEX/FULL どちらのモードでも一致する
- **エラーセマンティクス**: レコード書き込み成功後に index 同期が失敗した場合は
  関数が **-1 を返す**（レコードは書き込まれたまま残る）。`api_qs_db_index_rebuild` で復旧する。
  古い index エントリが残っても検索結果は誤らない（`record_list` は候補を実レコードで再照合するため）。
  影響は容量・スキャン効率のみ
- 既存データに後から有効化した場合、または `is_index` を追加/削除した場合、
  まず `api_qs_db_index_rebuild` で初期同期する

### id 採番

- `api_qs_db_next_id(context, table, prefix, out_id, out_size)`
  - system.db に `counter:{table}` を保持し、`prefix` + 連番（`prefix1`, `prefix2`, ...）を生成
  - 出力は 63 byte 以下。失敗時はカウンタを進めない

### 低レベル API（qs_datastore.h）

`api_qs_*` を使わず直接使用する場合。context は `QS_DB_CONTEXT`（自前保持）、
スクラッチメモリは `QS_MEMORY_POOL*`（`qs_initialize_memory_f64`）を第 1 引数に渡す規約。

- database: `qs_db_create/drop/show/open/close_database`
- table: `qs_db_create/drop/show_table`、`qs_db_open_table`（`QS_DB_STORE` 出力）/ `qs_db_close_table`
- schema: `qs_db_schema_define/get/validate`
- record: `qs_db_record_insert/update/delete/get/list`
- index: `qs_db_index_add/remove/rebuild`
- system: `qs_db_next_id`

高レベルとの主な差分:
- insert/update に**スキーマ自動検証・default 補完がない**
- 自動 index 更新機能がない（index の構築・CRUD 後の更新はすべて呼び出し側の責任）

参考: [sample/db_cli/main.c](../../sample/db_cli/main.c)（db_api_cli と同じコマンド文法・出力）

### テスト

- [test/datastore_test/](../../test/datastore_test/) — 低レベル API 検証
- [test/datastore_api_test/](../../test/datastore_api_test/) — 高レベル API 検証（自動 index 更新含む）

---

## 14. ランキング API

ヘッダ: [core/header/qs_api.h](../../core/header/qs_api.h)（高レベル `api_qs_ranking_*`）/ [core/header/qs_ranking.h](../../core/header/qs_ranking.h)（低レベル）

最大 `size` 人までユーザを登録し、スコア（`uint32_t`）で順位を管理するインメモリランキングです。
永続化機能はありません（終了時は消失。永続化が必要な場合は KVS / DB と組み合わせる）。

### 典型フロー

1. `api_qs_memory_alloc(&memory, ...)`（`api_qs_ranking_get` の結果用 scratch `dest_memory` も確保）
2. `api_qs_ranking_create(&memory, &ranking, size, key_size, get_max, refresh_size)`
3. `api_qs_ranking_entry(&ranking, id)` — ユーザ登録（成功 0 / 失敗 -1）
4. `api_qs_ranking_set_value(&ranking, id, value)` — スコア設定
   / `api_qs_ranking_add_value(&ranking, id, value)` — スコア加算
5. `api_qs_ranking_sort(&ranking)` — 値の降順でソート
6. `api_qs_ranking_get(&ranking, offset, length, &dest_memory)` — 結果 JSON 配列文字列（`dest_memory` 内を指す）
7. 終了時: `api_qs_memory_free`

`api_qs_ranking_get` の戻り値（各要素: `id` / `value` / `ranking`）:

```json
[ { "id": "player4", "value": 320, "ranking": 1 }, { "id": "player2", "value": 250, "ranking": 2 } ]
```

### 作成パラメータ

| パラメータ | 説明 |
|---|---|
| `size` | 登録可能な最大ユーザ数 |
| `key_size` | id の長さ（byte） |
| `get_max` | 1 回の `api_qs_ranking_get` で取得できる最大件数（`length` が超過した場合はこれでクランプされる） |
| `refresh_size` | 容量超過時に全ソート後に追い出す（evict）最低値エントリ数 |

### 注意点

- ソート順は **value 降順**（得点が高いほど ranking 1）
- 登録数が `size` に達すると全ソートが実行され、最低値の `refresh_size` 件が追い出される
  （追い出された id は以降の get に現れない）
- スコア型は `uint32_t`（`api_qs_ranking_add_value` は加算のみで、上書きは `set_value` を使う）
- `api_qs_ranking_get` の結果文字列は `dest_memory` 内を指すため、`dest_memory` の解放・再利用後は無効になる
- 使用例: [sample/ranking/main.c](../../sample/ranking/main.c)

### 低レベル API（qs_ranking.h）

`api_qs_*` を使わず直接使用する場合。datastore 低レベルと同様 `QS_MEMORY_POOL*` を第 1 引数に取る規約。

- `qs_create_ranking(_ppool, size, key_size, get_max, refresh_size)` → memory unit id（失敗 -1）
- `qs_entry_ranking(_ppool, ranking_munit, id)`
- `qs_set_ranking_value` / `qs_add_ranking_value`
- `qs_ranking_sort_all(_ppool, ranking_munit)` / `qs_ranking_sort_all_fast`
- `qs_get_ranking(_ppool, ranking_munit, offset, length, dest_memory)` → JSON 文字列

### テスト

- [test/ranking_ext_test/](../../test/ranking_ext_test/) — ランキング機能の検証

---

## 15. WebSocket（クライアント・バイナリ）

ヘッダ: [core/header/qs_api.h](../../core/header/qs_api.h)

### WebSocket クライアント（`api_qs_websocket_client_*`）

`ws://` サーバに接続する plain WebSocket クライアントです。`api_qs_client_init`（7 章）と同じクライアント基盤上で動きます。WSS 非対応（「現在の制約」参照）。

#### 典型フロー

1. `api_qs_websocket_client_init(&context, host, port, path)`
   （nonblocking TCP 接続 → HTTP Upgrade リクエスト送信 → `Sec-WebSocket-Accept` の検証）
2. `api_qs_websocket_client_set_max_message_size(context, max)`（省略可。既定 65535）
3. コールバック登録:
   - `api_qs_set_client_on_connect_event(context, on_connect)` — handshake 完了時
   - `api_qs_set_websocket_client_on_event(context, on_event)` — 受信フレームごとに
   - `api_qs_set_client_on_close_event(context, on_close)` — 接続クローズ時
4. メインループ: `api_qs_client_update(context)`（イベント処理）+ `api_qs_client_sleep(context)`（スリープ）
5. 送信: `api_qs_websocket_client_send(context, is_binary, payload, len)`
   （`is_binary`: 0 = text / 0 以外 = binary）
6. クローズ: `api_qs_websocket_client_close(context)`
   / `api_qs_websocket_client_close_with_reason(context, code, reason)`
   （呼び出し後は `get_state` が `CLOSING` を返し、追加の送信は拒否（非ゼロ返却）される）
7. 解放: `api_qs_client_free(context)`（ソケットを含む全リソースを解放）

参考: [sample/websocket_client/main.c](../../sample/websocket_client/main.c)
（`sample/http_server_simple` に対する room create / join / message / leave の統合テスト）

#### イベントコールバック

- `on_connect(QS_EVENT_PARAMETER)`: `parameter_type` は `QS_EVENT_PARAMETER_TYPE_CONNECTION`。handshake 完了時に 1 回。
- `on_event(QS_EVENT_PARAMETER)`: `parameter_type` は `QS_EVENT_PARAMETER_TYPE_RECV`。
  受信フレーム（text / binary / close / pong 等）ごとに呼ばれ、opcode で種別を区別する。

| 関数 | 説明 |
|---|---|
| `api_qs_get_websocket_payload(params)` | フレームペイロード（`uint8_t*`。**コールバック内でのみ有効**） |
| `api_qs_get_websocket_payload_length(params)` | ペイロードサイズ |
| `api_qs_get_websocket_opcode(params)` | opcode（RFC 6455: 0x1 text / 0x2 binary / 0x8 close / 0x9 ping / 0xA pong） |

#### 状態・エラー API

- `api_qs_websocket_client_get_state(context)` → `QS_WEBSOCKET_STATE`:
  `CONNECTING`(0) / `HANDSHAKING` / `OPEN` / `CLOSING` / `CLOSED` / `ERROR`
- `api_qs_websocket_client_get_error(context)` → `QS_WEBSOCKET_ERROR`:
  `NONE`(0) / `HANDSHAKE_SEND` / `HANDSHAKE_RESPONSE` / `HANDSHAKE_ACCEPT` /
  `RECEIVE_BUFFER` / `FRAME` / `SEND`
- `api_qs_websocket_client_get_close_code(context)` → close status code（`uint16_t`）
- `api_qs_websocket_client_get_close_reason(context)` → close reason 文字列
- メッセージサイズ上限: `api_qs_websocket_client_set_max_message_size` /
  `api_qs_websocket_client_get_max_message_size`
  （既定 `QS_WEBSOCKET_MAX_MESSAGE_SIZE` = 65535）

#### RFC 6455 対応範囲（クライアント）

- text / binary / ping / pong / close
- fragmented message（FIN=0）と continuation frame（opcode=0）の再構成
- 64-bit payload length（127）
- control frame の検証（FIN=1 かつ 125 bytes 以下）。不正 frame には close code `1002` を返す
- close status code / reason の解析と利用側への公開

#### 現在の制約

現時点（2026-11-10）で未実装の項目です。

- **送信キューなし**: 送信は `qs_send_all()` を直接呼び出すため、
  大きなメッセージの送信は完了まで（busy-wait 形式で）呼び出し側をブロックする。
  大きなフレームを頻繁に送る利用場面では応答性に影響し得る
- **callback の役割分離なし**: `on_event` 1 つに全フレームが渡るため opcode で判別
- **handshake の堅牢化未実施**: handshake header の検証・timeout が最小限
- **WSS 非対応**: `ws://` のみ。TLS は transport 層分離後の後続項目

### サーバ側: バイナリ・指定接続への送信

| 関数 | 説明 |
|---|---|
| `api_qs_send_ws_message(params, message)` | ルーム全員への text 送信（12 章。room 未参加時は送信者へ echo） |
| `api_qs_send_ws_message_plane(params, message)` | 送信元 connection（params の持有元）への text 送信 |
| `api_qs_send_ws_binary(params, data, size)` | 送信元 connection への binary 送信 |
| `api_qs_send_ws_text_by_connection_offset(context, offset, message)` | 指定 connection（offset は `api_qs_get_connection_offset`）への text 送信 |
| `api_qs_send_ws_binary_by_connection_offset(context, offset, data, size)` | 指定 connection への binary 送信（void 返却。WS 接続以外・範囲外 offset は no-op） |

サーバ側のフレームアクセス子（`on_message` コールバック内）:

| 関数 | 説明 |
|---|---|
| `api_qs_get_ws_message(params)` | text ペイロード（`char*`） |
| `api_qs_get_ws_message_size(params)` | ペイロードサイズ（`ssize_t`） |
| `api_qs_get_ws_opcode(params)` | opcode |

### テスト

- [test/websocket_protocol_test/](../../test/websocket_protocol_test/) — RFC 6455 frame 検証（クライアント側）
- [test/ws_binary_test/](../../test/ws_binary_test/) — サーバ側 WS binary 送受信
