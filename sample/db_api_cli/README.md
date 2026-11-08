# qs_db_api_cli

datastore **高レベル API**（`api_qs_db_*` / `QS_DB_API_CONTEXT`）を使う
mysql-cli 風のコマンドラインツール。
低レベル版 `sample/db_cli`（`qs_db_*` API）と**同じコマンド文法・同じ出力**を提供し、
高レベル API のライフサイクルの正しい使い方を示す。

## ビルド

```sh
make
```

core（`core/libqs_core.a`）が古い場合は `core/src` で `make` してから再ビルドすること。

## 使い方

```sh
./qs_db_api_cli [base_dir]                 # 対話モード（REPL）
./qs_db_api_cli [base_dir] -e "<command>"  # ワンショットモード（複数 -e 可）
```

- `base_dir` は database ディレクトリ群の親ディレクトリ（既定 `./db_data`）。存在しない場合は自動作成される。
- ワンショットモードでは、失敗したコマンド以降は実行されず exit code 1 で終了する。
- 対話モードでは失敗しても続行する。
- 破壊的操作（`drop database` / `drop table`）は対話モードで `y/n` 確認がある（ワンショットモードでは省略）。

## コマンド一覧

| コマンド | 説明 | 対応 API |
|---|---|---|
| `create database <name>` | database 作成 | `api_qs_db_create_database` |
| `drop database <name>` | database 削除（use 中は拒否） | `api_qs_db_drop_database` |
| `show databases` | database 一覧 | `api_qs_db_show_database` |
| `use <name>` | database をオープン（前のものは close） | `api_qs_db_open_database` |
| `create table <name> [capacity <N>]` | table 作成（N: 1/8/16/32/64/128/256/512/1024、既定 1） | `api_qs_db_create_table` |
| `drop table <name>` | table 削除 | `api_qs_db_drop_table` |
| `show tables` | table 一覧（system/schema/index は除外） | `api_qs_db_show_table` |
| `schema define <table> <json>` | スキーマ定義 | `api_qs_db_schema_define` |
| `schema get <table>` | スキーマ取得 | `api_qs_db_schema_get` |
| `schema validate <table> <json>` | レコードのスキーマ検証（`OK (valid)` / `INVALID`） | `api_qs_db_schema_validate` |
| `insert <table> <id> <json>` | レコード挿入（重複 id はエラー） | `api_qs_db_record_insert` |
| `get <table> <id>` | レコード取得 | `api_qs_db_record_get` |
| `update <table> <id> <json>` | レコード更新（未存在 id はエラー） | `api_qs_db_record_update` |
| `delete <table> <id>` | レコード削除（未存在 id でも OK、冪等） | `api_qs_db_record_delete` |
| `list <table> [filter_json] [index]` | レコード一覧（FULL スキャン、`index` 指定で index 検索） | `api_qs_db_record_list` |
| `index add <table> <field> <value> <id>` | index 追加 | `api_qs_db_index_add` |
| `index remove <table> <field> <value> <id>` | index 削除 | `api_qs_db_index_remove` |
| `index rebuild <table> <fields_json>` | index 再構築（fields_json は `["field1","field2"]` 形式） | `api_qs_db_index_rebuild` |
| `next id <table> <prefix>` | id 採番（`prefix1`, `prefix2`, ...） | `api_qs_db_next_id` |
| `help` | コマンド一覧 | - |
| `exit` / `quit` | 終了 | - |

`schema define` は渡された文字列を JSON/schema 形式の検証なしで保存する。高レベル API 版では schema が定義されている場合、`insert` / `update` 時に対応する型・`required`・string `maxLength` を自動検証し、省略された field に `default` が定義されていれば値を補完して保存する。入力に明示された値は default で上書きしない。default は field の型に合う `int32` / `int64` 数値または string にすること。schema 未定義の table は従来どおり検証せずに保存する。`schema validate` は明示的な事前確認にも引き続き使える。index 更新は自動では行わない。

## 低レベル版 `qs_db_cli` との差分

コマンド文法・出力は同一。内部実装の違いは以下の通り。

| 項目 | 低レベル版 `qs_db_cli` | 高レベル版 `qs_db_api_cli` |
|---|---|---|
| 使う API | `qs_db_*`（`qs_datastore.h`） | `api_qs_db_*`（`qs_api.h`） |
| insert/update | schema 検証・default 補完なし | schema があれば検証し、欠落 default を補完 |
| context | `QS_DB_CONTEXT`（自前保持） | `QS_DB_API_CONTEXT`（`api_qs_db_init`/`free`） |
| スクラッチメモリ | `QS_MEMORY_POOL*`（`qs_initialize_memory_f64`） | `QS_MEMORY_CONTEXT`（`api_qs_memory_alloc`/`clean`/`free`） |
| 起動処理 | なし | `api_qs_db_init` |
| 終了処理 | `qs_db_close_database` | `api_qs_db_close_database` + `api_qs_db_free` + `api_qs_memory_free` |
| 結果配列の読み取り | `qs_array_length` / `qs_array_get` | `api_qs_array_get_length` / `api_qs_array_get_string` |

### ライフサイクルのポイント

- **起動時**: `api_qs_db_init(&db)` で `QS_DB_API_CONTEXT` を初期化（内部で 16MiB の固定メモリプールを確保）
- **スクラッチメモリ**: `api_qs_memory_alloc(&scratch, size)` で 1 回確保し、
  結果を格納するコマンド（`show` / `schema validate` / `list` / `index rebuild`）の
  直前に `api_qs_memory_clean(&scratch)` でリセットする
- **終了時**: `api_qs_db_close_database` → `api_qs_db_free` → `api_qs_memory_free` の順で解放

## 例

```sh
$ ./qs_db_api_cli ./mydata -e "create database app" -e "use app" \
    -e "create table issue" \
    -e "schema define issue {\"fields\":[{\"name\":\"id\",\"type\":\"string\",\"maxLength\":64,\"required\":1},{\"name\":\"status\",\"type\":\"string\",\"maxLength\":32,\"required\":1}]}" \
    -e "insert issue i1 {\"id\":\"i1\",\"status\":\"open\"}" \
    -e "insert issue i2 {\"id\":\"i2\",\"status\":\"closed\"}" \
    -e "index rebuild issue [\"status\"]" \
    -e "list issue {\"status\":\"open\"} index"
OK
OK (database: app)
OK
OK
OK
OK
OK
[{"status":"open","id":"i1"}]
```

対話モード:

```sh
$ ./qs_db_api_cli ./mydata
qs_db_api_cli (base_dir: ./mydata). Type 'help' for commands.
qsdb> use app
OK (database: app)
qsdb> list issue
[{"status":"open","id":"i1"},{"status":"closed","id":"i2"}]
qsdb> exit
```
