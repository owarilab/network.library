# Issue Tracker CLI サンプル

`core` の永続化 KVS（`api_qs_kvs_create_b*mb_persistence`）を使った、  
Issue Tracker 向け CLI サンプルです。

- user / label / issue / index をストア分割して運用
- issue の status / assignee / label インデックスを自動更新
- ストア再構築（compaction）コマンドを提供

---

## 1. ビルド

```bash
cd sample/issue_tracker_cli
make
```

生成物:

- `qs_issue_tracker_cli`

---

## 2. 初期化

デフォルトは `./issue_data` 配下にストアファイルを作成します。

```bash
./qs_issue_tracker_cli init
```

任意ディレクトリ指定:

```bash
./qs_issue_tracker_cli init --base-dir ./my_issue_data
```

既存ストアを上書き再作成:

```bash
./qs_issue_tracker_cli init --base-dir ./my_issue_data --force
```

作成されるファイル:

- `user.db`
- `label.db`
- `issue.db`
- `issue_index.db`
- `system.db`（連番カウンタ等のシステムデータ）
- `schema_user.json`
- `schema_label.json`
- `schema_issue.json`

---

## 3. user / label 操作

### create

```bash
./qs_issue_tracker_cli user create --id u1 --json '{"id":"u1","name":"alice"}'
./qs_issue_tracker_cli label create --id bug --json '{"id":"bug","name":"Bug"}'
```

`--id` を省略すると自動採番します。

```bash
./qs_issue_tracker_cli user create --json '{"name":"alice"}'
./qs_issue_tracker_cli label create --json '{"name":"Bug"}'
```

### get

```bash
./qs_issue_tracker_cli user get --id u1
./qs_issue_tracker_cli label get --id bug
```

### list（全件）

```bash
./qs_issue_tracker_cli user list
./qs_issue_tracker_cli label list
```

### update

```bash
./qs_issue_tracker_cli user update --id u1 --json '{"id":"u1","name":"alice2"}'
./qs_issue_tracker_cli label update --id bug --json '{"id":"bug","name":"Bug Fix"}'
```

`--id` と JSON 内 `id` が不一致の場合はエラーになります。

### delete

```bash
./qs_issue_tracker_cli user delete --id u1
./qs_issue_tracker_cli label delete --id bug
```

---

## 4. issue 操作

### create

```bash
./qs_issue_tracker_cli issue create --id iss1 --json '{"id":"iss1","title":"bug","status":"open","assigneeIds":["u1"],"labelIds":["bug"]}'
```

`--id` を省略すると `iss_000001` のような連番IDを自動採番します。

```bash
./qs_issue_tracker_cli issue create --json '{"title":"bug","status":"open","assigneeIds":["u1"],"labelIds":["bug"]}'
```

シンプル版

```bash
./qs_issue_tracker_cli issue create --json '{"title":"bug","status":"open","assigneeIds":[],"labelIds":[]}'
./qs_issue_tracker_cli issue create --json '{"title":"ビルドに失敗する原因の調査","status":"open"}'
```

`assigneeIds` に指定した user、`labelIds` に指定した label が存在しない場合はエラーになります。
また create/update 時は schema ファイルに基づいて不足フィールドへデフォルト値を補完し、schema 未定義フィールドはエラーになります。

### get

```bash
./qs_issue_tracker_cli issue get --id iss1
```

### update

./qs_issue_tracker_cli issue update --id iss_000001 --json '{"id":"iss_000001","title":"bug2","status":"in_progress","assigneeIds":["u2"],"labelIds":["feature"]}'

```bash
./qs_issue_tracker_cli issue update --id iss1 --json '{"id":"iss1","title":"bug2","status":"in_progress","assigneeIds":["u2"],"labelIds":["feature"]}'
```

> `issue update` は「差分パッチ」ではなく、**issue JSON 全体の再保存**です。  
> 更新時は `status` / `assigneeIds` / `labelIds` を含めて送る運用を推奨します。
> また `--id` と JSON 内 `id` が不一致の場合はエラーになります。
> `assigneeIds` / `labelIds` に存在しないIDが含まれる場合もエラーになります。

※ schema でのデフォルト補完・未定義フィールド検証は create/update の両方で適用されます。

### delete

```bash
./qs_issue_tracker_cli issue delete --id iss1
```

---

## 5. issue 一覧・検索

### 全件

```bash
./qs_issue_tracker_cli issue list
```

### status 条件

```bash
./qs_issue_tracker_cli issue list --status open
```

### assignee 条件

```bash
./qs_issue_tracker_cli issue list --assignee u1
```

### label 条件

```bash
./qs_issue_tracker_cli issue list --label bug
```

---

## 6. compaction（再構築）

全ストア再構築:

```bash
./qs_issue_tracker_cli compact --store all
```

個別ストア再構築:

```bash
./qs_issue_tracker_cli compact --store issue
```

`--store` は以下を指定可能です。

- `user`
- `label`
- `issue`
- `index`
- `system`
- `all`

---

## 7. schema 取得

利用可能なスキーマ一覧:

```bash
./qs_issue_tracker_cli schema list
```

特定エンティティのスキーマ取得:

```bash
./qs_issue_tracker_cli schema get --entity user
./qs_issue_tracker_cli schema get --entity label
./qs_issue_tracker_cli schema get --entity issue
```

`--base-dir` を指定して別ディレクトリの schema を読むことも可能です。

```bash
./qs_issue_tracker_cli schema get --base-dir ./my_issue_data --entity issue
```

---

## 8. schema ファイルの書き方

schema は次の形式です。

```json
{
  "fields": [
    {"name":"id","type":"string","maxLength":64,"required":0},
    {"name":"title","type":"string","maxLength":256,"default":"","required":1},
    {"name":"priority","type":"int32","default":0,"required":0},
    {"name":"updatedAt","type":"int64","default":0,"required":0}
  ]
}
```

各フィールド定義の意味:

- `name`: 項目名
- `type`: `string` / `int32` / `int64`
- `required`: `1` なら必須、`0` なら任意
- `default`: 未指定時に補完する値（型に一致している必要あり）
- `maxLength`: `type=string` のときのみ有効（最大文字数）

運用ルール:

- create/update 時に schema が適用されます。
- schema にない項目はエラーです（ただし issue の `assigneeIds` / `labelIds` は例外で許可されています）。
- `required=1` かつ `default` がない項目を未指定で送るとエラーになります。
- `maxLength` 超過、型不一致はエラーになります。

---

## 9. オプション共通

多くのコマンドで以下を指定できます。

- `--base-dir <dir>`: ストア配置ディレクトリを指定

例:

```bash
./qs_issue_tracker_cli issue list --base-dir ./my_issue_data --status open
```

---

## 10. 補足

- 本サンプルは固定容量 KVS 前提です（容量不足時はエラー）。
- compaction は「新ファイル再構築→切替」方式です。
- `create` 時に `--id` 省略した場合は `system.db` のカウンタを使って自動採番します。

---

## 11. issue update の利用ケース（運用手順）

`issue update` は実運用で使うケースが多いため、代表ケースを手順化します。  
共通の基本フローは次の通りです。

1. 現在値取得: `issue get --id <id>`
2. JSON を編集（必要項目を維持したまま変更）
3. `issue update --id <id> --json '<edited-json>'`
4. 検索で確認（`issue list --status ...` など）

### Case A: ステータス遷移（Open -> In Progress -> Closed）

```bash
# 1) まず現在値を確認
./qs_issue_tracker_cli issue get --id iss1

# 2) open -> in_progress
./qs_issue_tracker_cli issue update --id iss1 --json '{"id":"iss1","title":"bug","status":"in_progress","assigneeIds":["u1"],"labelIds":["bug"]}'

# 3) in_progress -> closed
./qs_issue_tracker_cli issue update --id iss1 --json '{"id":"iss1","title":"bug","status":"closed","assigneeIds":["u1"],"labelIds":["bug"]}'
```

確認:

```bash
./qs_issue_tracker_cli issue list --status open
./qs_issue_tracker_cli issue list --status in_progress
./qs_issue_tracker_cli issue list --status closed
```

### Case B: 担当者変更（assignee 差し替え）

```bash
./qs_issue_tracker_cli issue update --id iss1 --json '{"id":"iss1","title":"bug","status":"open","assigneeIds":["u2"],"labelIds":["bug"]}'
```

確認:

```bash
./qs_issue_tracker_cli issue list --assignee u1
./qs_issue_tracker_cli issue list --assignee u2
```

### Case C: ラベル変更（label 差し替え）

```bash
./qs_issue_tracker_cli issue update --id iss1 --json '{"id":"iss1","title":"bug","status":"open","assigneeIds":["u1"],"labelIds":["feature"]}'
```

確認:

```bash
./qs_issue_tracker_cli issue list --label bug
./qs_issue_tracker_cli issue list --label feature
```

### Case D: 複合更新（status + assignee + label を同時変更）

```bash
./qs_issue_tracker_cli issue update --id iss1 --json '{"id":"iss1","title":"hotfix","status":"review","assigneeIds":["u3"],"labelIds":["urgent","backend"]}'
```

確認:

```bash
./qs_issue_tracker_cli issue list --status review
./qs_issue_tracker_cli issue list --assignee u3
./qs_issue_tracker_cli issue list --label urgent
```

### Case E: タイトル/本文だけ更新（索引対象を維持）

索引対象（`status` / `assigneeIds` / `labelIds`）を変えない場合でも、  
更新JSONにはそれらを含めるのが安全です。

```bash
./qs_issue_tracker_cli issue update --id iss1 --json '{"id":"iss1","title":"title updated","description":"desc updated","status":"open","assigneeIds":["u1"],"labelIds":["bug"]}'
```

### Case F: 誤更新のリカバリ

1. 更新前に `issue get` の結果をローカルに退避
2. 誤更新時は退避した JSON で `issue update` を再実行

```bash
# 例: バックアップしてから更新
./qs_issue_tracker_cli issue get --id iss1 > /tmp/iss1_backup.json
./qs_issue_tracker_cli issue update --id iss1 --json '<new-json>'

# 例: 戻す
./qs_issue_tracker_cli issue update --id iss1 --json "$(cat /tmp/iss1_backup.json)"
```

### 運用上の注意（update）

- `issue update` は **全体置換**なので、必要フィールドを落とさないこと
- 更新後は必ず `issue list --status / --assignee / --label` で索引整合を確認すること
- 大量更新後は必要に応じて `compact --store issue` / `compact --store index` を実行すること
