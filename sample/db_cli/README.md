# qs_db_cli

`qs_datastore`（低レベル `qs_db_*` API）を使う mysql-cli 風のコマンドラインツール。
`qs_datastore` の各 API の使い方をそのまま示すサンプルである。

## ビルド

```sh
make
```

core（`core/libqs_core.a`）が古い場合は `core/src` で `make` してから再ビルドすること。

## 使い方

```sh
./qs_db_cli [base_dir]                 # 対話モード（REPL）
./qs_db_cli [base_dir] -e "<command>"  # ワンショットモード（複数 -e 可）
```

- `base_dir` は database ディレクトリ群の親ディレクトリ（既定 `./db_data`）。存在しない場合は自動作成される。
- ワンショットモードでは、失敗したコマンド以降は実行されず exit code 1 で終了する。
- 対話モードでは失敗しても続行する。
- 破壊的操作（`drop database` / `drop table`）は対話モードで `y/n` 確認がある（ワンショットモードでは省略）。

## コマンド一覧

| コマンド | 説明 | 対応 API |
|---|---|---|
| `create database <name>` | database 作成 | `qs_db_create_database` |
| `drop database <name>` | database 削除（use 中は拒否） | `qs_db_drop_database` |
| `show databases` | database 一覧 | `qs_db_show_database` |
| `use <name>` | database をオープン（前のものは close） | `qs_db_open_database` |
| `create table <name> [capacity <N>]` | table 作成（N: 1/8/16/32/64/128/256/512/1024、既定 1） | `qs_db_create_table` |
| `drop table <name>` | table 削除 | `qs_db_drop_table` |
| `show tables` | table 一覧（system/schema/index は除外） | `qs_db_show_table` |
| `schema define <table> <json>` | スキーマ定義 | `qs_db_schema_define` |
| `schema get <table>` | スキーマ取得 | `qs_db_schema_get` |
| `schema validate <table> <json>` | レコードのスキーマ検証（`OK (valid)` / `INVALID`） | `qs_db_schema_validate` |
| `insert <table> <id> <json>` | レコード挿入（重複 id はエラー） | `qs_db_record_insert` |
| `get <table> <id>` | レコード取得 | `qs_db_record_get` |
| `update <table> <id> <json>` | レコード更新（未存在 id はエラー） | `qs_db_record_update` |
| `delete <table> <id>` | レコード削除（未存在 id でも OK、冪等） | `qs_db_record_delete` |
| `list <table> [filter_json] [index]` | レコード一覧（FULL スキャン、`index` 指定で index 検索） | `qs_db_record_list` |
| `index add <table> <field> <value> <id>` | index 追加 | `qs_db_index_add` |
| `index remove <table> <field> <value> <id>` | index 削除 | `qs_db_index_remove` |
| `index rebuild <table> <fields_json>` | index 再構築（fields_json は `["field1","field2"]` 形式） | `qs_db_index_rebuild` |
| `next id <table> <prefix>` | id 採番（`prefix1`, `prefix2`, ...） | `qs_db_next_id` |
| `help` | コマンド一覧 | - |
| `exit` / `quit` | 終了 | - |

`schema define` は渡された文字列を JSON/schema 形式の検証なしで保存する。検証が必要な場合は `insert` / `update` の前に `schema validate` を明示的に実行すること。CRUD は schema 検証も index 更新も自動では行わない。現状の検証対象は `int32` / `int64` / `string` の型、`required`、string の `maxLength`。default の適用、未知・重複 field、配列の検証は行わない。

## 例

```sh
$ ./qs_db_cli ./mydata -e "create database app" -e "use app" \
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
$ ./qs_db_cli ./mydata
qs_db_cli (base_dir: ./mydata). Type 'help' for commands.
qsdb> use app
OK (database: app)
qsdb> list issue
[{"status":"open","id":"i1"},{"status":"closed","id":"i2"}]
qsdb> exit
```

## 一通りの操作例

以下は、database 作成から削除まで全コマンドを使う対話モードの例である。

```sh
$ ./qs_db_cli ./demo_data
qs_db_cli (base_dir: ./demo_data). Type 'help' for commands.

# --- database 作成・一覧・オープン ---
qsdb> create database mydb
OK
qsdb> create database other
OK
qsdb> show databases
mydb
other
qsdb> use mydb
OK (database: mydb)

# --- table 作成・一覧 ---
qsdb> create table issue
OK
qsdb> create table user capacity 8
OK
qsdb> show tables
issue
user

# --- schema 定義・取得・検証 ---
qsdb> schema define issue {"fields":[{"name":"id","type":"string","maxLength":64,"required":1},{"name":"title","type":"string","maxLength":256,"required":1},{"name":"status","type":"string","maxLength":32,"required":1},{"name":"priority","type":"int32","required":0}]}
OK
qsdb> schema get issue
{"fields":[{"name":"id","type":"string","maxLength":64,"required":1},{"name":"title","type":"string","maxLength":256,"required":1},{"name":"status","type":"string","maxLength":32,"required":1},{"name":"priority","type":"int32","required":0}]}
qsdb> schema validate issue {"id":"i1","title":"bug fix","status":"open","priority":1}
OK (valid)
qsdb> schema validate issue {"id":"i1","status":"open"}
INVALID

# --- レコード CRUD ---
qsdb> insert issue i1 {"id":"i1","title":"bug fix","status":"open","priority":1}
OK
qsdb> insert issue i2 {"id":"i2","title":"add feature","status":"open","priority":2}
OK
qsdb> insert issue i3 {"id":"i3","title":"docs update","status":"closed","priority":3}
OK
qsdb> get issue i1
{"id":"i1","title":"bug fix","status":"open","priority":1}
qsdb> update issue i1 {"id":"i1","title":"bug fix (urgent)","status":"open","priority":0}
OK
qsdb> get issue i1
{"id":"i1","title":"bug fix (urgent)","status":"open","priority":0}

# --- list（フルスキャン / フィルタ / 空結果） ---
# 注: list の結果は内部のハッシュ順でキーが並ぶ（get は保存順を保持する）
qsdb> list issue
[{"priority":0,"status":"open","id":"i1","title":"bug fix (urgent)"},{"priority":2,"status":"open","id":"i2","title":"add feature"},{"priority":3,"status":"closed","id":"i3","title":"docs update"}]
qsdb> list issue {"status":"open"}
[{"priority":0,"status":"open","id":"i1","title":"bug fix (urgent)"},{"priority":2,"status":"open","id":"i2","title":"add feature"}]
qsdb> list issue {"status":"nonexistent"}
[]

# --- index 構築・検索・手動更新 ---
qsdb> index rebuild issue ["status","priority"]
OK
qsdb> list issue {"status":"open"} index
[{"priority":0,"status":"open","id":"i1","title":"bug fix (urgent)"},{"priority":2,"status":"open","id":"i2","title":"add feature"}]
qsdb> index add issue status open i9
OK
qsdb> list issue {"status":"open"} index
[{"priority":0,"status":"open","id":"i1","title":"bug fix (urgent)"},{"priority":2,"status":"open","id":"i2","title":"add feature"}]
qsdb> index remove issue status open i9
OK

# --- id 採番 ---
qsdb> next id issue iss_
iss_1
qsdb> next id issue iss_
iss_2

# --- レコード削除 ---
qsdb> delete issue i3
OK
qsdb> list issue
[{"priority":0,"status":"open","id":"i1","title":"bug fix (urgent)"},{"priority":2,"status":"open","id":"i2","title":"add feature"}]

# --- table 削除（y/n 確認あり） ---
qsdb> drop table user
Drop table? [y/N]: y
OK
qsdb> show tables
issue

# --- database 削除（use 中は拒否） ---
qsdb> drop database mydb
ERROR: cannot drop database mydb while in use
qsdb> use other
OK (database: other)
qsdb> drop database mydb
Drop database? [y/N]: y
OK
qsdb> show databases
other

# --- 終了 ---
qsdb> exit
```

ワンショットモードで同じ操作を 1 行にまとめた例:

```sh
$ ./qs_db_cli ./demo_data \
    -e "create database mydb" \
    -e "use mydb" \
    -e "create table issue" \
    -e "schema define issue {\"fields\":[{\"name\":\"id\",\"type\":\"string\",\"maxLength\":64,\"required\":1},{\"name\":\"status\",\"type\":\"string\",\"maxLength\":32,\"required\":1}]}" \
    -e "insert issue i1 {\"id\":\"i1\",\"status\":\"open\"}" \
    -e "insert issue i2 {\"id\":\"i2\",\"status\":\"closed\"}" \
    -e "index rebuild issue [\"status\"]" \
    -e "list issue {\"status\":\"open\"} index" \
    -e "next id issue iss_"
OK
OK (database: mydb)
OK
OK
OK
OK
OK
[{"status":"open","id":"i1"}]
iss_1
```

## 注意点

- **index の管理は呼び出し側の責任**: `insert` / `update` / `delete` は index を自動更新しない。
  `list ... index` で正しい結果を得るには、CRUD 後に `index add` / `index remove` を実行するか、
  `index rebuild` で再構築する。index 未構築・更新漏れでは index 検索の結果が欠落する。
- **schema 検証も自動ではない**: `insert` / `update` はスキーマを検証しない。必要なら `schema validate` を先に実行する。
- **名前の制約**: database / table 名は ASCII の英数字・`_`・`-` のみ（1〜46 byte）。API 側が検証する。
- **`system` / `schema` / `index` は予約名**であり、table として create/drop できない。
- **容量**: table は固定容量（mmap ベース）で、容量不足時は自動拡張せずエラーになる。
  `capacity` は作成時のみ指定可能（`system.db` に保存され、以降は保存値が使われる）。
- **`delete` は冪等**: 存在しない id の削除も成功（OK）を返す。
- **`list` の結果は 64KB 以下**: 結果 JSON が終端 NUL を含め 64KB を超える場合はエラーになる。
- **JSON 引数の空白**: `{` または `[` で始まる引数は括弧のバランスで 1 トークンとして扱われるため、
  値に空白が含まれていても（例: `"title":"bug fix"`）そのまま渡せる。
  一方、`get` / `insert` の戻り値・レコード JSON は保存順のキー順で、`list` の結果は内部ハッシュ順のキー順になる。
- **排他制御**: 同一 database の同時 open は拒否される（プロセス内 registry + directory flock）。
  別プロセスから同一 DB を操作する場合はこのツールを 1 つだけ実行すること。
