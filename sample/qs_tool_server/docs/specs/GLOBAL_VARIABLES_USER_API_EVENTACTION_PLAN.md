# user.fixed / user.persistent 汎用API と EventAction 連携 実装計画書

最終更新: 2026-04-30

## 目的

`sample/qs_tool_server` において、`user.fixed` / `user.persistent` を安全に読み書きできる汎用 API を整備し、
EventAction からグローバル変数の参照・代入を行えるようにする。

この計画書は、実装を小さな phase に分割し、各 phase の完了条件を明確にして進捗確認しやすくすることを目的とする。

---

## 対象範囲

- `user.fixed` / `user.persistent` の runtime 操作 API
- 変数パス解決とバリデーション
- EventAction からの変数参照アクション
- EventAction からの変数代入アクション
- PlayTest 時の挙動確認
- 保存/復元との整合確認

---

## 前提

- `project.globalVariables` は既に導入済み
- `system.fixed` 系の一部 runtime 参照は既に存在する
- `VariablesEditorScene` で `user.fixed` / `user.persistent` の定義編集はできる
- ただし、`user` 変数をゲーム実行ロジックから汎用的に扱う層はまだ薄い

---

## 実装方針

1. まず `AppData` に安全な variable API を追加する
2. 次に EventAction から使える「読み取り」「書き込み」「更新」アクションを定義する
3. 既存の `system.fixed` 系に影響を与えないよう、`user` 系を先に汎用化する
4. その後、必要に応じて `system` 系にも同じ API を流用できる形に整理する

---

## Phase 0: 現状整理

状態: 完了

### 目的

現在の `globalVariables` 関連コードを棚卸しし、どこまでが既存で、どこからが不足かを明確にする。

### 作業項目

1. `ProjectData` の `globalVariables` 正規化ロジックを確認する
2. `AppData` の runtime state 生成ロジックを確認する
3. `VariablesEditorScene` の編集対象と保存フローを確認する
4. `PlayTestScene` の EventAction 実行ロジックを確認する
5. 既存の `system.fixed` 専用処理を洗い出す

### 現状確認メモ

- `ProjectData` は `system` / `user` を含む `globalVariables` を正規化している
- `VariablesEditorScene` は `system.fixed` / `system.persistent` / `user.fixed` / `user.persistent` を編集対象にしている
- `AppData` は runtime state を生成し、`system.fixed.*` の専用参照と書き換えを持っている
- `PlayTestScene` は `requestPlayUnit` / `returnPlayUnit` などで `system.fixed` を直接利用している
- `user.fixed` / `user.persistent` を EventAction から操作する汎用経路はまだ実装されていない

### 洗い出し済みの関連箇所

- `www/js/project/project_data.js`
- `www/js/app_data.js`
- `www/js/scene/variables_editor_scene.js`
- `www/js/scene/play_test_scene.js`
- `www/js/project/project_session.js`
- `docs/specs/GLOBAL_VARIABLES_IMPLEMENTATION_PLAN.md`
- `docs/specs/EVENT_ACTION_DESIGN.md`

### 完了条件

- `user` 変数の編集・保存・runtime 参照の入口が一覧化されている
- 既存の `system.fixed` 専用コードと共存できることが確認されている

### Phase 0 の完了判定

- `globalVariables` の保存/復元/編集/実行の責務分担が把握できている
- 次 phase で実装すべき API の候補が明確になっている
- `user` 変数が「編集は可能だが runtime API が不足」という状態だと明文化されている

---

## Phase 1: 汎用 API の基盤整備

状態: 完了

### 目的

`user.fixed` / `user.persistent` を安全に読み書きするための、共通の runtime API を `AppData` 側に追加する。

### 作業項目

1. 変数パス形式 `scope.tier.name` の解決を共通化する
2. 変数の存在確認 API を追加する
3. 変数の取得 API を追加する
4. 変数の設定 API を追加する
5. 変数の削除 API を追加する
6. 値のクローン/代入ルールを整理する
7. 既存の `system.fixed` 処理との互換性を確認する

### 実装候補 API

- `getRuntimeGlobalVariable(path)`
- `setRuntimeGlobalVariable(path, value)`
- `hasRuntimeGlobalVariable(path)`
- `deleteRuntimeGlobalVariable(path)`
- `resolveRuntimeGlobalVariablePath(path)`

### 安全性要件

- 不正なパスは拒否する
- 未定義 bucket への代入は拒否する
- JSON 化できない値は保存対象にしない
- runtime state と project definition の境界を壊さない

### 完了条件

- `user.fixed` / `user.persistent` の値をコードから安全に読み書きできる
- 不正入力時に壊れない
- 既存の `system.fixed` 処理を壊さない
- `AppData` に安全な汎用APIが追加されている

### 完了メモ

- `resolveRuntimeGlobalVariablePath(path)` を追加した
- `hasRuntimeGlobalVariable(path)` を追加した
- `getRuntimeGlobalVariable(path)` は clone を返すようにした
- `setRuntimeGlobalVariable(path, value)` は定義済み変数のみ更新し、clone して保持するようにした
- `deleteRuntimeGlobalVariable(path)` を追加した
- `listRuntimeGlobalVariables(scopePath)` を追加した

---

## Phase 2: 値モデルとバリデーション整備

状態: 完了

### 目的

EventAction や UI から扱う値の型・変換ルールを確定し、変数操作時の事故を減らす。

### 作業項目

1. 変数定義の `type` と `initialValue` のルールを整理する
2. runtime 代入時の型変換ルールを決める
3. 文字列入力から数値/boolean/object/array を扱う変換ルールを定義する
4. deep clone が必要な型を決める
5. 参照時に返す値をコピーにするか参照にするかを決める
6. 変数名の妥当性チェックを再利用可能にする
7. `user.fixed` と `user.persistent` の差異をドキュメント化する

### `user.fixed` / `user.persistent` の扱い差

- `user.fixed` は、編集中に明示的に変えた値を runtime へ反映する短期スコープとして扱う
- `user.persistent` は、保存対象として残す前提の長期スコープとして扱う
- どちらも runtime API の観点では同じ `scopePath` 形式でアクセスするが、保存・復元の意図が異なる
- いずれも `AppData` からは clone した値を返し、直接参照を書き換えない
- EventAction からの参照/代入では、どちらも同じ型変換ルールを使う
- `system.*` は内部制御用、`user.*` はアプリ定義用として扱う

### 完了メモ

- `ProjectData.normalizeGlobalVariableInitialValue(value, type)` を追加し、初期値の型変換ルールを共通化した
- `ProjectData.validateGlobalVariables(source)` を追加し、保存前の定義検証を共通化した
- `ProjectData._normalizeVariableBucket(bucket)` を強化し、不正な定義を保存前に落とすようにした
- `VariablesEditorScene._validateProjectGlobalVariables()` を共通検証へ差し替えた
- `user.fixed` / `user.persistent` の意図差を短期スコープ / 長期スコープとして明文化した

### 完了条件

- 代入時の型変換方針がコード上で一貫している
- 変数操作で runtime state が壊れない
- 参照/コピーの扱いが明文化されている

---

## Phase 3: EventAction の変数参照アクション追加

状態: 完了

### 目的

EventAction から runtime の global variable を読み出せるようにする。

### 作業項目

1. 変数参照用の action 名を決める
2. 参照結果の格納先を決める
3. 参照失敗時の挙動を決める
4. `PlayTestScene` の `_executeAction()` に分岐を追加する
5. 参照値の表示用ログを整備する
6. 必要なら他アクションから参照結果を再利用できるようにする

### Action 候補

- `getGlobalVariable`
- `copyGlobalVariable`
- `readGlobalVariable`
- `resolveGlobalVariable`

### 参照先候補

- 別の global variable へ代入
- `EventAction` の内部一時結果
- 対象オブジェクトの component property
- ステータスログ表示

### 完了条件

- EventAction から `user.fixed` / `user.persistent` を読む action が動作する
- 存在しない変数を読んでも壊れない
- 結果確認が PlayTest でできる

### 進捗メモ

- `readGlobalVariable` を `PlayTestScene` に追加し、runtime global variable を読み取れるようにした
- 読み取り結果は `PlayTestScene._lastEventActionValue` に保持し、必要なら対象 component の property にも書き込めるようにした
- 未定義変数はエラー扱いにして、壊れずに止まるようにした

### 完了メモ

- EventAction から runtime global variable を参照する最小経路ができた
- 参照結果は一時保持または component property への反映に使える
- 存在しない変数参照はエラーで停止するため、PlayTest 中に壊れにくい

---

## Phase 4: EventAction の変数代入アクション追加

状態: 完了

### 目的

EventAction から runtime の global variable を書き換えられるようにする。

### 作業項目

1. 代入用 action 名を決める
2. 値のソースを整理する
3. 文字列リテラルと JSON リテラルの扱いを決める
4. `setGlobalVariable` 系の分岐を追加する
5. `increment` / `decrement` のような更新系を必要に応じて定義する
6. `toggle` のような boolean 向け更新を必要に応じて定義する
7. 書き換え後の dirty 状態更新を確認する

### 進捗メモ

- `setGlobalVariable` の最小実装を追加した
- `valueSource=literal` と `valueSource=variable` を扱えるようにした
- `EventAction` テンプレートに代入用の入力項目を追加した

### Action 候補

- `setGlobalVariable`
- `addGlobalVariable`
- `toggleGlobalVariable`
- `appendGlobalVariable`

### 完了条件

- EventAction から `user.fixed` / `user.persistent` に代入できる
- 型不一致や不正パスで壊れない
- PlayTest で値が変化することを確認できる

---

## Phase 5: EventAction JSON 例と UI の整備

状態: 進行中

### 目的

実装したアクションを、既存の JSON 例と編集 UI で使いやすくする。

### 作業項目

1. `EventAction` のテンプレート項目を確認する
2. `setGlobalVariable` / `readGlobalVariable` の編集項目を揃える
3. `valueSource` / `valueVariablePath` の入力が壊れないことを確認する
4. Phase 4 の確認用 `.qsproj` と整合するテンプレートを用意する
5. 必要なら UI で使う説明文や初期値を調整する

### 完了条件

- `EventAction` の JSON テンプレートから変数参照・代入を編集できる
- Phase 4 の確認用 `.qsproj` をそのまま編集・再利用できる
- 将来の専用フォーム化に向けて必要項目が漏れていない

---

## Phase 6: 保存・復元・互換性確認

状態: 未着手

### 目的

変数操作機能を追加しても、既存 `.qsproj` や既存プロジェクトを壊さないことを確認する。

### 作業項目

1. 既存 `.qsproj` のロード確認
2. `globalVariables` 未定義プロジェクトのロード確認
3. `user` 変数あり/なしの保存確認
4. browser storage 保存確認
5. export/import 後の round-trip 確認
6. 既存 `system.fixed` 系の動作確認

### 完了条件

- 旧プロジェクトが壊れない
- 新しい変数操作を含むプロジェクトが保存・復元できる
- 保存形式の互換性が維持されている

---

## Phase 7: 動作確認と最終調整

状態: 未着手

### 目的

実装全体を通して、PlayTest と Variables Editor の両方で期待どおり動くことを確認する。

### 作業項目

1. `user.fixed` の変更が即時反映されるか確認する
2. `user.persistent` の変更が保存に残るか確認する
3. EventAction からの read / write を確認する
4. 無効な変数名・パス・型のエラー表示を確認する
5. UI 文言を整える
6. 必要なら `WORK_STATUS.md` を更新する

### 完了条件

- 主要フローが一通り確認できる
- エラー時の挙動が破綻しない
- 次の拡張（system 変数の汎用化、ローカル変数、条件分岐など）へ繋げられる

---

## 進捗管理の見方

この計画は各 phase 単位で進捗を更新する。

- `未着手`: まだ作業していない
- `進行中`: 実装中
- `完了`: 完了条件を満たした

更新時は、各 phase の完了条件が実際に満たされているかを確認してから状態を更新する。

---

## 将来拡張の候補

- `system` 変数も同じ汎用 API に統合する
- `EventAction` に条件分岐を追加する
- `user` 変数の型ごとの専用 UI を追加する
- `persistent` を保存差分管理に対応させる
- PlayUnit ローカル変数を別スコープとして追加する
