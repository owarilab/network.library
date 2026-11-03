# Issue Tracker CLI サンプル

`core` の永続化 KVS（`api_qs_kvs_create_b*mb_persistence`）を使った、  
Issue Tracker 向け CLI サンプルです。

- user / label / issue / index をストア分割して運用
- issue の status / assignee / label インデックスを自動更新
- ストア再構築（compaction）コマンドを提供
- index の再構築（reindex）と整合性検証（verify）に対応

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
- `system_settings.json`（システム設定。現状は issue.statuses を定義）
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
`update` は差分更新なので、変更したい項目だけを渡せます。

### delete

```bash
./qs_issue_tracker_cli user delete --id u1
./qs_issue_tracker_cli label delete --id bug
```

### unset（1項目リセット/削除）

```bash
./qs_issue_tracker_cli user unset --id u1 --field name
./qs_issue_tracker_cli label unset --id bug --field name
```

- 指定フィールドに schema の `default` がある場合: その値へ戻します
- `default` がない場合: そのフィールドを削除します

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
`status` は `system_settings.json` の `issue.statuses` で定義された値のみ許可されます。

### get

```bash
./qs_issue_tracker_cli issue get --id iss1
```


./qs_issue_tracker_cli issue create --id iss2 --json '{"id":"iss1","title":"bug","status":"open","assigneeIds":["u1"],"labelIds":["bug"]}'


./qs_issue_tracker_cli issue update --id iss_000001 --json '{"id":"iss_000001","title":"bug2","status":"in_progress","assigneeIds":[],"labelIds":[]}'

### update

./qs_issue_tracker_cli issue update --id iss_000001 --json '{"id":"iss_000001","title":"bug2","status":"in_progress","assigneeIds":["u2"],"labelIds":["feature"]}'

```bash
./qs_issue_tracker_cli issue update --id iss1 --json '{"id":"iss1","title":"bug2","status":"in_progress","assigneeIds":["u2"],"labelIds":["feature"]}'
```

> `issue update` は **差分更新**です。  
> 送信した項目だけを更新し、未指定項目は既存値を維持します。
> また `--id` と JSON 内 `id` が不一致の場合はエラーになります。
> `assigneeIds` / `labelIds` に存在しないIDが含まれる場合もエラーになります。

※ schema でのデフォルト補完・未定義フィールド検証は create/update の両方で適用されます。

### delete

```bash
./qs_issue_tracker_cli issue delete --id iss1
```

### unset（1項目リセット/削除）

```bash
./qs_issue_tracker_cli issue unset --id iss1 --field description
./qs_issue_tracker_cli issue unset --id iss1 --field status
```

- `status` のように schema `default` がある項目は default 値へ戻ります（例: `open`）
- `unsettable=1` が schema に定義された項目のみ unset できます

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

## 7. index の再構築・検証

issue 本体を正として index を再生成します（既存の`index:`キーを削除してから作り直します）。

```bash
./qs_issue_tracker_cli reindex
```

index の欠落・余分な参照・不正な値を検査:

```bash
./qs_issue_tracker_cli verify
```

どちらのコマンドも `--base-dir` を指定できます。問題がある場合、`verify` は詳細と集計を出力し、終了コード `4` を返します。`reindex` は issue 本体を走査して index を作り直すため、実行中に他のプロセスから同じストアを書き換えないでください。処理途中で失敗すると index が部分的な状態になる可能性があります。その場合は、issue ストアを保全したうえで再度実行してください。

---

## 8. schema 取得

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
    {"name":"id","type":"string","maxLength":64,"required":0,"unsettable":0},
    {"name":"title","type":"string","maxLength":256,"default":"","required":1,"unsettable":1},
    {"name":"priority","type":"int32","default":0,"required":0,"unsettable":1},
    {"name":"updatedAt","type":"int64","default":0,"required":0,"unsettable":1}
  ]
}
```

各フィールド定義の意味:

- `name`: 項目名
- `type`: `string` / `int32` / `int64`
- `required`: `1` なら必須、`0` なら任意
- `default`: 未指定時に補完する値（型に一致している必要あり）
- `maxLength`: `type=string` のときのみ有効（最大文字数）
- `unsettable`: `1` なら `unset` 可能、`0` なら `unset` 不可

運用ルール:

- create/update 時に schema が適用されます。
- unset 時も schema を参照します。
- schema にない項目は create/update でエラーです（issue の `assigneeIds` / `labelIds` は create/update でのみ例外許可）。
- `required=1` かつ `default` がない項目を未指定で送るとエラーになります。
- `maxLength` 超過、型不一致はエラーになります。
- unset で `default` がある項目は default に戻り、`default` がない項目は削除されます。
- unset は `unsettable=1` の項目のみ実行できます。

---

## 9. system_settings.json の書き方（status定義）

issue の `status` は `system_settings.json` で管理します。

```json
{
  "issue": {
    "statuses": ["open", "in_progress", "closed", "review"]
  }
}
```

- `statuses` に含まれない文字列を issue の `status` に指定すると create/update でエラーになります。
- 起動時に `system_settings.json` を読み込みます。

---

## 10. オプション共通

多くのコマンドで以下を指定できます。

- `--base-dir <dir>`: ストア配置ディレクトリを指定

例:

```bash
./qs_issue_tracker_cli issue list --base-dir ./my_issue_data --status open
```

---

## 11. 補足

- 本サンプルは固定容量 KVS 前提です（容量不足時はエラー）。
- compaction は「新ファイル再構築→切替」方式です。
- `create` 時に `--id` 省略した場合は `system.db` のカウンタを使って自動採番します。

---

## 12. issue update の利用ケース（運用手順）

`issue update` は実運用で使うケースが多いため、代表ケースを手順化します。  
共通の基本フローは次の通りです。

1. 現在値取得（任意）: `issue get --id <id>`
2. 変更したい項目だけ JSON を作成
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

索引対象（`status` / `assigneeIds` / `labelIds`）を変えない場合は、  
その項目を省略しても既存値が維持されます。

```bash
./qs_issue_tracker_cli issue update --id iss1 --json '{"id":"iss1","title":"title updated","description":"desc updated"}'
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

- `issue update` は **差分更新**。未指定フィールドは既存値が保持される
- 更新後は必ず `issue list --status / --assignee / --label` で索引整合を確認すること
- 大量更新後は必要に応じて `compact --store issue` / `compact --store index` を実行すること
