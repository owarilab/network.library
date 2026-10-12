# データベース設計書

JRPG P1-7「データベース」の土台。キャラ・敵・アイテム・スキルなどの表のデータを、ID（行の key）で引ける形で持つ。
関連: [GLOBAL_VARIABLES_DESIGN.md](GLOBAL_VARIABLES_DESIGN.md)（変数との分担）、[EVENT_ACTION_DESIGN.md](EVENT_ACTION_DESIGN.md)（パスの使い方）、[JRPG_FEATURE_GAP_ANALYSIS.md](JRPG_FEATURE_GAP_ANALYSIS.md)

## 1. 3 種類の DB

| 種類 | 定義する人 | 実行時 | 保存 | 用途 |
|---|---|---|---|---|
| `system` | アプリ（エンジン） | 読み取り専用（凍結） | プロジェクトに入れない | 方向、イージング名、変数の型など、エンジンの列挙・定数 |
| `const` | ユーザー | 読み取り専用 | `project.database` | アイテム、敵、スキルなど、ゲーム中に変わらないデータ |
| `mutable` | ユーザー | **セルの値を変更できる** | `project.database`（初期値） | パーティの HP、所持数、フラグ表など、進行で変わる状態（将来のセーブ対象） |

- 実行時の `mutable` は定義の**コピー**で管理する（`AppData.databaseState`）。定義（アセット）の値は書き換わらない。PlayTest を始めるたびに定義から作り直す。
- 実行中に変えられるのは**セルの値だけ**。行と列の追加・削除はしない（件数が固定なので、セーブデータは「テーブル → 行 key → 列名 → 値」の差分で済み、定義の変更に強い）。数量は列で持つ（例: インベントリは行を固定して `count` 列）。

## 2. データ形式（`project.database`）

`globalVariables` と同じくプロジェクトのトップレベルに置く（アセットではなく、PlayUnit の部品でもない共有データ）。

```json
"database": {
  "version": 1,
  "tables": [
    { "id": "tbl_items", "name": "items", "label": "アイテム", "kind": "const",
      "columns": [
        { "id": "col_items_label", "name": "label", "label": "名前", "type": "string", "default": "" },
        { "id": "col_items_heal",  "name": "heal",  "label": "回復量", "type": "integer", "default": 0 },
        { "id": "col_items_kind",  "name": "kind",  "label": "", "type": "enum", "options": ["heal", "attack", "key"], "default": "heal" }
      ],
      "rows": [ { "key": "potion", "cells": { "label": "ポーション", "heal": 30, "kind": "heal" } } ] }
  ]
}
```

- `kind`: `const` / `mutable`。
- 識別子（テーブル名 `name`、列名 `name`、行の `key`）は `[A-Za-z_][A-Za-z0-9_]*`、64 文字まで。パスの区切りが `.` のため、ドットを含められない。表示名は `label`（テーブル・列）で別に持つ。
- テーブルと列は内部 `id`（`tbl_…` / `col_…`）を持つ。`ref` 列は参照先を**テーブルの ID**（`refTableId`）で持つので、テーブル名を変えても参照は切れない。行は `key` で引く（数値の連番 ID は使わない）。
- 上限（検証で拒否）: テーブル 128、列 64 / テーブル、行 5000 / テーブル。

### 列の型

| `type` | 値 | 追加の設定 | 備考 |
|---|---|---|---|
| `string` | 文字列 | — | 1 行 |
| `text` | 文字列 | — | 複数行（Shift+Enter で改行） |
| `number` | 有限の数値 | — | 小数可。数値の文字列は数値へ変換 |
| `integer` | 整数 | — | 小数は拒否（四則の結果が小数になる書き込みも拒否） |
| `boolean` | true / false | — | `"true"` `"false"` `1` `0` も受ける |
| `enum` | 選択肢のいずれか | `options`（1 つ以上） | 選択肢にない値は拒否 |
| `ref` | 別テーブルの行 `key`（空は「なし」） | `refTableId` | 参照先に行がなければ拒否 |
| `asset` | アセットの ID（空は「なし」） | `assetType`（map / playUnit / pixelDocument / tileset / characterChipset） | Save As で新しい ID に書き換わる |
| `json` | JSON で表せる値（null 可） | — | リストや辞書は v1 では json で持つ |

## 3. 実行時のパス

```
db.<kind>.<table>.<row>.<column>      例: db.mutable.party.hero.hp
db.<kind>.<table>.<row>               行（列名 → 値のオブジェクト）
db.<kind>.<table>                     表（行 key → 行のオブジェクト）
```

- `kind` は `system` / `const` / `mutable`。変数のスコープ（`system` / `user`）と衝突しない（`db.` で始まれば DB）。
- **新しいアクションは増やさない**。変数のパスを受けるすべての場所が、そのまま DB を扱える。
  - `setGlobalVariable` の `variablePath`（`set` と四則）と `valueVariablePath`
  - `typewriter` の `valueVariablePath`
  - Text の `${db.const.items.potion.heal}`（見つからなければ `${…}` のまま）
  - Conditional の `${db.mutable.party.hero.hp}`、`exists`（表・行・セルの存在確認）
- 読めるのは 3 種類すべて。**書けるのは `db.mutable` のセルだけ**（5 部分のパス）。`const` / `system` への書き込み、行・表への書き込み、存在しないセル、型に合わない値、`enum` の選択肢外、存在しない `ref` の行は失敗し、理由がステータスに出る（アクションなら sequence を中断）。
- 四則（`add` など）は、型が `number` / `integer` の列でだけ使える。`integer` 列への結果が小数になる（`divide` など）と失敗して、値は変わらない。
- 値は読み取るたびにコピーを返す（参照を共有しない）。
- システム DB の表: `direction`（down/left/right/up と dx/dy）、`easing`、`variableType`、`columnType`、`assetType`。内容を増やすのはアプリの更新で行う。

## 4. 保存と互換性

- `.qsproj` は `project.database` を持つ（`version` は上げない）。`database` が無い古いファイルは空の DB として読む。古いアプリで開くと `database` は読み捨てられる。
- IndexedDB のプロジェクトレコードにも `database` を保存する。**同時に `globalVariables` も保存するように直した**（以前は IndexedDB に保存されず、`.qsproj` の書き出しだけが運んでいた）。
- Save As: `asset` 列のセルと既定値を、振り直したアセット ID に書き換える。

## 5. セーブへの準備（まだ作らない）

`AppData.exportMutableDatabaseState()` / `importMutableDatabaseState(savedState)` を用意した。状態の形は `{ テーブル名: { 行 key: { 列名: 値 } } }`。読み込み時は、定義に無いテーブル・行・列を無視し、状態に無いセルや型に合わない値は定義の初期値のままにする。保存先とタイミングは P2-19（ゲーム内のセーブとロード）で決める。

## 6. Database Editor（`DatabaseEditorScene`）

`ProjectTopScene` の「Open Database Editor」（キー `B`）から開く。

- 画面: 左にテーブルの一覧（CONST / MUTABLE / SYSTEM）、中央にグリッド、右に Inspector。幅が 1040 px 未満では Inspector を隠す。
- グリッド: 行番号と `key` 列を固定。縦横のスクロールとスクロールバー、仮想化（見える範囲だけ描く。5000 行でも 1 フレーム 1 ms 未満）、列幅のドラッグ、列ヘッダに型のバッジ。
- 選択: セル、範囲（ドラッグ、Shift+クリック、Shift+矢印）、行・列・全体（ヘッダのクリック）。
- 編集: Enter / F2 / ダブルクリック / 文字入力で開始。DOM の入力欄を重ねるので日本語入力（IME）が使える（変換中の Enter は確定しない）。Enter で確定して下へ、Tab / Shift+Tab で右・左へ、Esc で取り消し、検証に失敗したら入力欄を開いたまま理由を出す。boolean はチェックボックス（クリック / Space）、enum / ref / asset はポップアップ（ref と asset は文字で絞り込める）、`text` と `json` は複数行。
- 操作: 行の追加・挿入・複製・削除・移動（Alt+↑↓）、列の追加・削除・移動（Alt+←→）、Delete で既定値へ戻す、右クリックのメニュー。
- クリップボード: Ctrl+C / X / V を TSV で（Excel などと相互にやり取りできる）。範囲に貼り付け、1 つの値は選択範囲全体へ、足りない行は追加する（検索中は追加しない）。型に合わないセルは取り込まず、件数を知らせる。
- 検索: Ctrl+F。キーとセルの文字で行を絞り込む（保存順は変えない）。
- Undo / Redo: このシーン専用の履歴（Ctrl+Z / Ctrl+Y / Ctrl+Shift+Z）。セルの変更は差分（`DatabaseSetCellsCommand`）、行・列・テーブルの変更はテーブル単位のスナップショット（`DatabaseTablesCommand`）。
- Inspector: テーブル（名前、ラベル、種類、削除）、選択中の列（名前、ラベル、型、既定値、enum の選択肢、ref の参照先、asset の種類、移動、削除）、選択中の行（key）、選択中のセルのパス（クリックでコピー）、問題の一覧、PlayUnit からの使用箇所。
- 検証: key / 名前の形式と重複、ref の行が無い、enum の選択肢外、型違い、asset が無い、を赤く示す（テーブルの一覧にも印が付く）。問題があると保存できない。
- 型の変更: 既存のセルを新しい型へ変換し、変換できないセルは既定値に戻して件数を知らせる（Undo で戻せる）。
- 行の key を変えると、他テーブルの `ref` セルと既定値も新しい key に直る。
- 保存: Ctrl+S（`Save` ボタン）。

### 制限事項

- 行と列の並べ替えはドラッグではなく、Alt+矢印キーと右クリックのメニュー。表示だけの並べ替え（ソート）は無い（検索による絞り込みだけ）。
- IME の最初の 1 打は取れない。日本語は Enter / F2 / ダブルクリックで編集を始めてから入力する（ASCII の文字は、押すだけで編集が始まる）。
- テーブル名・列名・行の key を変えても、PlayUnit の EventAction / Conditional / Text にある `db.…` のパスは自動では書き換わらない（Inspector の「使用箇所」で確認する。変更時に件数を警告する）。
- 実行中に行・列は増やせない（セルの値だけ）。`list` / 辞書の専用セルは無く、`json` で持つ。CSV ファイルの入出力は無い（TSV のコピー&ペーストで代用）。

## 7. 実装箇所

| 役割 | ファイル |
|---|---|
| データ層（正規化、検証、型変換、パス、実行用状態） | `www/js/database/database_data.js` |
| システム DB | `www/js/database/system_database.js` |
| 編集操作（行・列・テーブル、型変更、TSV） | `www/js/database/database_ops.js` |
| Undo コマンド | `www/js/command/database_command.js` |
| 保存 | `www/js/project/project_session.js`（.qsproj）、`project_browser_storage.js`（IndexedDB、Save As） |
| 実行時のパス解決 | `www/js/app_data.js`（`getRuntimeGlobalVariable` などが `db.` を振り分ける）、`getRuntimeVariableType` |
| 表の部品 | `www/js/ui/grid/`（`GridTheme`、`GridView`、`CellEditor`、`PopupList`） |
| エディタ | `www/js/scene/database_editor_scene.js`（画面と入力）、`database_editor_actions.js`（編集）、`database_editor_inspector.js`（Inspector） |
| デモ | `tools/add_database_to_demo.py`（`character_walk_demo.qsproj` に items / hazards / party、HUD、ポーションのボタン、罠を足す） |
