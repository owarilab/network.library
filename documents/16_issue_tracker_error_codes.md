# Issue Tracker CLI エラーコード表

`sample/issue_tracker_cli` の終了コードとエラーメッセージを、運用者が「何が起きたか」「どう対処するか」を判断できるようにまとめた表である。

> 本表は 2026-11-03 時点の `main.c` の出力文字列に基づく。メッセージ文字列は将来の修正で変わる可能性がある。

## 終了コード一覧

| コード | 意味 | 主な原因 |
|---|---|---|
| 0 | 正常終了 | — |
| 2 | 使用法エラー（EXIT_USAGE） | 引数不足・不正な値・スキーマ違反・参照整合性違反 |
| 3 | I/O エラー（EXIT_IO） | ディレクトリ作成失敗・ファイル読み書き失敗・一時メモリ確保失敗 |
| 4 | KVS エラー（EXIT_KVS） | ストア作成/オープン失敗・書き込み失敗・index 不整合 |
| 5 | 未検出（EXIT_NOT_FOUND） | 対象キーが存在しない |

## エラーメッセージ別表

### 終了コード 2（使用法エラー）

| エラーメッセージ | 原因 | 対処 |
|---|---|---|
| `error: --json is required for create` | create に `--json` 未指定 | `--json '{...}'` を追加 |
| `error: --json is required for update` | update に `--json` 未指定 | `--json '{...}'` を追加 |
| `error: --id is required` | get/update/delete/unset に `--id` 未指定 | `--id <id>` を追加 |
| `error: --field is required for unset` | unset に `--field` 未指定 | `--field <name>` を追加 |
| `error: --entity is required for schema get` | `schema get` に `--entity` 未指定 | `--entity user\|label\|issue` を追加 |
| `error: --id and json.id mismatch for create` | `--id` と JSON の `id` が不一致 | 両方を一致させる |
| `error: --id and json.id mismatch for update` | `--id` と JSON の `id` が不一致 | 両方を一致させる |
| `error: --id and json.id mismatch for issue create` | `--id` と JSON の `id` が不一致 | 両方を一致させる |
| `error: --id and json.id mismatch for issue update` | `--id` と JSON の `id` が不一致 | 両方を一致させる |
| `error: invalid json for create` | JSON デコード失敗 | JSON 構文を確認 |
| `error: invalid json for update` | JSON デコード失敗 | JSON 構文を確認 |
| `error: invalid json for issue create` | JSON デコード失敗 | JSON 構文を確認 |
| `error: invalid json for issue update` | JSON デコード失敗 | JSON 構文を確認 |
| `error: schema validation failed for create` | スキーマ未定義フィールド・型不一致・maxLength 超過 | スキーマ定義（`schema get`）と照合 |
| `error: schema validation failed for issue create` | 同上 | 同上 |
| `error: field not defined in schema: <key>` | スキーマにないフィールド | 不要フィールドを削除、またはスキーマに追加 |
| `error: schema type mismatch(int32): <name>` | int32 フィールドに非整数 | 整数値に変更 |
| `error: schema type mismatch(int64): <name>` | int64 フィールドに非整数 | 整数値に変更 |
| `error: schema type mismatch(string): <name>` | string フィールドに非文字列 | 文字列に変更 |
| `error: schema maxLength exceeded: <name>` | 文字列が maxLength 超過 | 文字列を短縮 |
| `error: required field missing: <name>` | 必須フィールド欠落 | フィールドを追加 |
| `error: id field cannot be unset` | `--field id` で unset 試行 | id は unset 不可 |
| `error: field not defined in schema for unset: <name>` | unset 対象がスキーマ未定義 | スキーマ定義を確認 |
| `error: field is not unsettable by schema: <name>` | スキーマで `unsettable: 0` | スキーマ定義を確認 |
| `error: field not found: <name>` | unset 対象フィールドがレコードに存在しない | `get` でレコード内容を確認 |
| `error: unsupported json field type: <key>` | 未対応 JSON 型 | 文字列/整数/配列/オブジェクトのいずれかに変更 |
| `error: issue status is not allowed: <status>` | `system_settings.json` の `issue.statuses` にない値 | 許可された status を使用 |
| `error: user id not found: <id>` | `assigneeIds` に存在しない user id | 有効な user id を使用 |
| `error: label id not found: <id>` | `labelIds` に存在しない label id | 有効な label id を使用 |
| `error: invalid --scan value: <value> (use index or full)` | `--scan` に不正値 | `index` または `full` を使用 |
| `error: invalid --store value: <value>` | `compact --store` に不正値 | `user\|label\|issue\|index\|system\|all` を使用 |
| `error: invalid --entity value: <value>` | `schema get --entity` に不正値 | `user\|label\|issue` を使用 |
| `error: unsupported action for <entity>: <action>` | 未対応の action | `create\|get\|update\|delete\|unset\|list` を使用 |
| `error: unsupported issue action: <action>` | 同上 | 同上 |
| `error: unsupported schema action: <action>` | 同上 | `list\|get` を使用 |
| `error: unknown command: <cmd>` | 未対応のコマンド | `help` で利用可能コマンドを確認 |

### 終了コード 3（I/O エラー）

| エラーメッセージ | 原因 | 対処 |
|---|---|---|
| `error: path exists but is not directory: <path>` | `--base-dir` がファイル | ディレクトリパスを指定 |
| `error: mkdir failed: <path> (<errno>)` | ディレクトリ作成失敗（権限等） | 権限を確認、または既存ディレクトリを指定 |
| `error: store file not found: <path>` | ストアファイル未作成 | `init --base-dir <dir>` を実行 |
| `error: failed to read system settings file: <path>` | `system_settings.json` 読み込み失敗 | `init --force` で再生成 |
| `error: invalid system settings json: <path>` | `system_settings.json` が不正 JSON | `init --force` で再生成 |
| `error: issue settings not found: <path>` | `system_settings.json` に `issue` キーなし | `init --force` で再生成 |
| `error: issue.statuses not found: <path>` | `issue.statuses` 配列なし | `init --force` で再生成 |
| `error: issue.statuses count is invalid: <n>` | statuses が 0 件または上限超過 | `init --force` で再生成 |
| `error: issue.statuses contains non-string or empty value` | statuses に空文字列 | `init --force` で再生成 |
| `error: failed to read schema file: <path>` | スキーマファイル読み込み失敗 | `init --force` で再生成 |
| `error: invalid schema json: <path>` | スキーマファイルが不正 JSON | `init --force` で再生成 |
| `error: schema fields not found: <path>` | スキーマに `fields` キーなし | `init --force` で再生成 |
| `error: schema field count over limit: <n>` | スキーマフィールドが 128 件超過 | スキーマを整理 |
| `error: unsupported schema type: <type>` | スキーマに未対応型 | `int32\|int64\|string` を使用 |
| `error: failed to write schema file: <path>` | スキーマファイル書き込み失敗 | 権限・ディスク容量を確認 |
| `error: failed to write system settings file: <path>` | 設定ファイル書き込み失敗 | 権限・ディスク容量を確認 |
| `error: failed to allocate temporary memory` | 一時メモリ確保失敗 | 使用メモリ量を確認、compaction を実行 |
| `error: failed to build path for <store>` | パス結合失敗（異常） | 報告 |

### 終了コード 4（KVS エラー）

| エラーメッセージ | 原因 | 対処 |
|---|---|---|
| `error: store already exists: <path>` | `init`（`--force` なし）で既存ストア | `init --force` で上書き、または既存データを使用 |
| `error: failed to remove existing store: <path> (<errno>)` | 既存ストア削除失敗 | 権限を確認 |
| `error: failed to create store(<name>): <path>` | ストアファイル作成失敗 | ディスク容量・権限を確認 |
| `error: failed to open store(<name>): <path>` | ストアファイルオープン失敗 | ファイル存在・権限を確認 |
| `error: key already exists: <key>` | create で既存キー | 別の id を使用、または `update` |
| `error: failed to set key: <key>` | KVS 書き込み失敗（容量不足等） | `compact --store <name>` を実行 |
| `error: key not found: <key>` | get/update/delete でキー未存在 | 正しい id を確認 |
| `error: failed to update key: <key>` | KVS 書き込み失敗 | `compact --store <name>` を実行 |
| `error: failed to delete key: <key>` | KVS 削除失敗 | 再実行 |
| `error: issue already exists: <key>` | issue create で既存 id | 別の id を使用、または `update` |
| `error: failed to create issue: <key>` | issue 書き込み失敗 | `compact --store issue` を実行 |
| `error: failed to update index on issue create` | index 更新失敗（create 後ロールバック） | `compact --store index` を実行し再作成 |
| `error: issue not found: <key>` | issue update/delete でキー未存在 | 正しい id を確認 |
| `error: failed to remove old index on issue update` | 旧 index 削除失敗 | `reindex` を実行 |
| `error: failed to update issue: <key>` | issue 書き込み失敗（旧 index にロールバック） | `compact --store issue` を実行 |
| `error: failed to add new index on issue update` | 新 index 追加失敗（旧値にロールバック） | `compact --store index` を実行 |
| `error: failed to remove issue index` | issue 削除時の index 削除失敗 | `reindex` を実行 |
| `error: failed to delete issue key` | issue キー削除失敗（index にロールバック） | 再実行 |
| `error: invalid index data for key: <key>` | index 値が不正 JSON | `reindex` を実行 |
| `error: failed to generate id for <entity>` | カウンター書き込み失敗 | `compact --store system` を実行 |
| `error: failed to generate issue id` | 同上 | 同上 |
| `error: failed to open source store: <path>` | compaction 時ソースオープン失敗 | ファイル存在・権限を確認 |
| `error: failed to create temp store: <path>` | compaction 時一時ストア作成失敗 | ディスク容量を確認 |
| `error: failed to copy store data: <name>` | compaction 時コピー失敗 | `verify` で破損確認、`reindex` を実行 |
| `error: failed to backup store: <path>` | compaction 時 rename 失敗 | 権限を確認 |
| `error: failed to switch compacted store: <path>` | compaction 時切替失敗（自動ロールバック） | 再実行 |
| `error: failed to create temp index store: <path>` | reindex 時一時ストア作成失敗 | ディスク容量を確認 |
| `error: failed to rebuild index for issue: <key>` | reindex 時 issue JSON 不正 | `verify` で破損 issue を特定 |
| `error: failed to backup index store: <path>` | reindex 時 rename 失敗 | 権限を確認 |
| `error: failed to switch index store: <path>` | reindex 時切替失敗（自動ロールバック） | 再実行 |
| `verify: error: failed to decode issue json: <id>` | verify 時 issue JSON 不正 | `reindex` を実行（破損 issue は再作成が必要） |
| `verify: error: failed to verify issue: <key>` | verify 時 issue 検証失敗 | `reindex` を実行 |
| `verify: error: failed to verify index against issues` | verify 時 index 検証失敗 | `reindex` を実行 |
| `verify: error: status key too long: <value>` | status 値が 1024 文字超過 | status 値を確認 |
| `verify: error: assignee key too long: <value>` | assignee id が 1024 文字超過 | assignee id を確認 |
| `verify: error: label key too long: <value>` | label id が 1024 文字超過 | label id を確認 |
| `verify: error: temp memory alloc failed for <key>` | verify 時一時メモリ確保失敗 | compaction を実行 |
| `verify: error: failed to create array for index keys` | verify 時配列作成失敗 | compaction を実行 |
| `verify: error: failed to list index keys` | verify 時 index キー列挙失敗 | `compact --store index` を実行 |
| `verify: error: failed to list issue keys` | verify 時 issue キー列挙失敗 | `compact --store issue` を実行 |
| `verify: error: failed to alloc temp memory for issue=<id> in index=<key>` | verify 時一時メモリ確保失敗 | compaction を実行 |

### 終了コード 5（未検出）

| エラーメッセージ | 原因 | 対処 |
|---|---|---|
| `error: key not found: <key>` | get/update/delete でキー未存在 | `list` で存在する id を確認 |

## 運用フロー

```
エラー発生
  │
  ├─ 終了コード 2 → 引数・スキーマ・参照整合性を確認（対処: 引数修正）
  │
  ├─ 終了コード 3 → ファイル・ディレクトリ・権限を確認（対処: init --force / 権限修正）
  │
  ├─ 終了コード 4 → KVS 層の問題
  │    ├─ 書き込み失敗 → compact --store <name>
  │    ├─ index 不整合 → verify → reindex
  │    └─ 破損 issue → verify で特定 → 再作成
  │
  └─ 終了コード 5 → 対象キーが存在しない（対処: list で確認）
```

## 関連コマンド

| コマンド | 用途 |
|---|---|
| `verify` | index と issue の整合性を検査。破損・欠落・stale を検出 |
| `reindex` | issue 本体から index を再構築（一時ストア切替方式、失敗時は元 index 保持） |
| `compact --store <name>` | ストアの compaction（新ファイル再構築→切替） |
| `init --force` | ストア・スキーマ・設定を再生成（既存データは上書き） |
