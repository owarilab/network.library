# サンプル・テスト対応表

コアライブラリのサンプル（`sample/`）とテスト（`test/`）の一覧です。
本ファイルは [core_development_guide.md](core_development_guide.md) 11 章の本体で、
ガイド側は参照のみになっています。

## サンプル（sample/）

### ネットワーク
- TCP/UDP サーバ: [sample/server/main.c](../../sample/server/main.c)
- TCP/UDP クライアント: [sample/client/main.c](../../sample/client/main.c)
- 簡易クライアント: [sample/client_simple/main.c](../../sample/client_simple/main.c)
- HTTP サーバ（簡易）: [sample/http_server_simple/main.c](../../sample/http_server_simple/main.c)
- HTTP クライアント: [sample/http_client/main.c](../../sample/http_client/main.c)
- Plain クライアント（ペイロード送信）: [sample/client_plain/main.c](../../sample/client_plain/main.c)
- 受信出力クライアント: [sample/client_printer/main.c](../../sample/client_printer/main.c)
- WebSocket クライアント: [sample/websocket_client/main.c](../../sample/websocket_client/main.c)（15 章）
- HTTP ベンチマーク: [sample/http_bench/main.c](../../sample/http_bench/main.c)

### データ処理
- JSON: [sample/json/main.c](../../sample/json/main.c)
- Base64/SHA1: [sample/encode/main.c](../../sample/encode/main.c)
- ハッシュ: [sample/hash/main.c](../../sample/hash/main.c)
- 配列（スクリプト用）: [sample/array/main.c](../../sample/array/main.c)
- ランキング: [sample/ranking/main.c](../../sample/ranking/main.c)（14 章）
- ロガー: [sample/logger/main.c](../../sample/logger/main.c)（7 章）

### KVS
- 永続化 KVS: [sample/persistence_cache/main.c](../../sample/persistence_cache/main.c)
- Issue Tracker CLI 初期化: [sample/issue_tracker_cli/main.c](../../sample/issue_tracker_cli/main.c)
  - user/label/issue の CRUD・index 検索（`--scan index|full`）・issue コメント（`issue comment *`、`issue get --with-comments`）・verify/reindex/compaction
  - 運用向けエラーコード表: [documents/16_issue_tracker_error_codes.md](../16_issue_tracker_error_codes.md)

### DB（datastore）
- DB CLI（低レベル `qs_db_*`）: [sample/db_cli/main.c](../../sample/db_cli/main.c)（13 章）
- DB API CLI（高レベル `api_qs_db_*`）: [sample/db_api_cli/main.c](../../sample/db_api_cli/main.c)（13 章。コマンド一覧は [README.md](../../sample/db_api_cli/README.md)）

### スクリプト
- Script 実行: [sample/script/main.c](../../sample/script/main.c)

### アプリケーション例（LLM / STT / ツールサーバ）
- LLM エージェント: [sample/llm_agent/README.md](../../sample/llm_agent/README.md)（HTTP streaming + ツール呼び出し）
- LLM HTTP ストリーム: [sample/llm_http_stream/README.md](../../sample/llm_http_stream/README.md)
- ツールサーバ: [sample/qs_tool_server/README.md](../../sample/qs_tool_server/README.md)
- STT サーバ: [sample/stt_server/](../../sample/stt_server/) / [stt_server_v2](../../sample/stt_server_v2/) / [stt_server_v3](../../sample/stt_server_v3/)（音声認識の実装例）

### Unity
- ビルド手順は [documents/README.md](../README.md) を参照

## テスト（test/）

各テストは `make && ./<program>` で実行でき、pass/fail 集計を出力する（fail 0 で exit code 0）。
一括で全テストをビルド・実行するには `test/run_all.sh` を、全テストを clean するには `test/clean_all.sh` を実行する。

| テスト | 対象API | 検証内容 |
|---|---|---|
| [test/string_test/](../../test/string_test/) | `qs_string` 全21関数 | 文字列変換・検索・URLエンコード/デコード・エスケープ等 |
| [test/sha1_test/](../../test/sha1_test/) | `qs_sha1` / `api_qs_sha1_encode` | FIPS 180-1 ベクトル・ブロック境界・バイナリデータ |
| [test/http_param_test/](../../test/http_param_test/) | `api_qs_get_http_get_parameter` / `post_parameter` / `cookie_parameter` / `api_qs_get_http_post_json_object` / `api_qs_http_response_json` | GETクエリ（URLデコード）・POSTフォーム・POST JSON（ネスト）・Cookie |
| [test/room_test/](../../test/room_test/) | ルーム組み込みAPI（`/api/v1/room/*`）+ `api_qs_server_create_router` + WSクライアントAPI | room create/list/join/leave + WSブロードキャスト |
| [test/random_test/](../../test/random_test/) | `api_qs_rand` / `api_qs_uniqid` / `qs_rand_32/64` / `qs_srand_32/64` / `qs_uniqid_r32/r64` | 値の変化・再現性・uniqid の一意性 |
| [test/chain_array_test/](../../test/chain_array_test/) | `qs_create_chain_array` 等（低レベル版） | 追加・イテレーション・削除・resize |
| [test/io_test/](../../test/io_test/) | `qs_fread_range` 等（低レベル版） | ファイル読み書き・範囲読み込み |
| [test/ws_binary_test/](../../test/ws_binary_test/) | `api_qs_get_ws_opcode` / `api_qs_get_ws_message(_size)` / `api_qs_send_ws_binary` / `api_qs_websocket_client_send` | テキスト（opcode 1）・バイナリ（opcode 2・NULバイト含む）往復 |
| [test/packet_route_test/](../../test/packet_route_test/) | `qs_add_packet_route_connection` / `qs_get_route_info(s)` 等 | 接続管理・ルーター情報取得 |
| [test/json_ext_test/](../../test/json_ext_test/) | JSON 拡張API | 複雑なJSON構造のエンコード/デコード |
| [test/csv_api_test/](../../test/csv_api_test/) | `api_qs_csv_read_file` / `api_qs_csv_parse` / `api_qs_csv_get_row` 等 | ファイル読み込み・引用符フィールド・日本語マルチバイト |
| [test/script_argv_test/](../../test/script_argv_test/) | Script 引数API | スクリプト実行時の引数処理 |
| [test/ranking_ext_test/](../../test/ranking_ext_test/) | ランキング拡張API | ランキング登録・取得・更新 |
| [test/datastore_test/](../../test/datastore_test/) | `qs_db_*`（低レベル datastore） | database/table 作成・削除・スキーマ・レコード CRUD・index・next_id |
| [test/datastore_api_test/](../../test/datastore_api_test/) | `api_qs_db_*`（高レベル） | スキーマ検証・default 補完・レコード CRUD・**自動 index 更新**（insert/update/delete での is_index 同期）・rebuild 復旧 |
| [test/hash_collision_delete_test/](../../test/hash_collision_delete_test/) | `qs_hash`（低レベル） | ハッシュ衝突・削除 |
| [test/json_decode_test/](../../test/json_decode_test/) | JSON デコーダ（低レベル） | decode 検証（エッジケース含む） |
| [test/kvs_ext_test/](../../test/kvs_ext_test/) | KVS 拡張API（`api_qs_kvs_keys` 等） | キー列挙・ソートキー |
| [test/kvs_create_test/](../../test/kvs_create_test/) | `api_qs_kvs_create_b*mb`（全9サイズ） | 作成・set/get/delete・key_length・sorted_keys 件数 |
| [test/kvs_stress_test/](../../test/kvs_stress_test/) | `api_qs_kvs_create_b1mb`（100万件投入） | LRU 退避後の key_length・sorted_keys 一致・get ラウンドトリップ |
| [test/csv_lowlevel_test/](../../test/csv_lowlevel_test/) | `qs_csv_*`（低レベル版） | file_load/parse/build_csv(_memid)/init/add_row/add_line・エッジケース |
| [test/queue_test/](../../test/queue_test/) | `qs_create_queue` / `qs_enqueue` / `qs_safe_enqueue` / `qs_dequeue` / `qs_read_queue` | 空キュー・チャンク読み込み・満杯上書き・複数キュー |
| [test/static_file_test/](../../test/static_file_test/) | 静的ファイル配信（`http_request_common`） | Content-Type・キャッシュ制御・304 Not Modified |
| [test/base64_test/](../../test/base64_test/) | `qs_base64` 系 | Base64 エンコード/デコード（Python で検証） |
| [test/websocket_protocol_test/](../../test/websocket_protocol_test/) | WebSocket フレーム処理 | フレーム構築・マスク・opcode |
| [test/memory_allocator_test/](../../test/memory_allocator_test/) | メモリアロケーター | 確保・解放・クリーン挙動 |
| [test/issue_tracker_cli_test/](../../test/issue_tracker_cli_test/) | Issue Tracker CLI (`sample/issue_tracker_cli`) | init/force・CRUD・index検索・複合AND検索・comment CRUD・カスケード削除・verify/reindex・compaction・断片化 |
