# Global Variables / VariablesEditorScene 実装計画書

最終更新: 2026-04-29

## 現在の進捗サマリ

| Phase | 状態 | 進捗概要 |
|---|---|---|
| Phase 1 | 完了 | `project.globalVariables` の導入、正規化、既存 `.qsproj` 互換を実装済み |
| Phase 2 | 完了 | Runtime current state の生成と `AppData` API を実装済み |
| Phase 3 | 完了 | `ProjectTopScene` から `VariablesEditorScene` への導線を実装済み |
| Phase 4 | 完了 | 左ナビ型の Variables Editor と基本編集 UI を実装済み |
| Phase 5 | 完了 | 保存ボタン、`Ctrl/Cmd+S`、保存前バリデーションを実装済み |
| Phase 6 | 完了 | `startup/requested/current/returnPlayUnitId` の接続、`requestPlayUnit` / `returnPlayUnit`、2-way サンプルを実装済み |
| 次フェーズ | 未着手 | `isPaused`、`masterVolume`、切替時の状態整理、変数参照/代入系 EventAction |

### 直近で追加済みのもの

1. `startupPlayUnitId` による起動時 PlayUnit 解決
2. `requestedPlayUnitId` の one-shot 消費と無効 ID の無視
3. `returnPlayUnitId` を使う `returnPlayUnit` EventAction
4. `requestedPlayUnitId` に値を書く `requestPlayUnit` EventAction
5. `MainRoom -> OverlayMenu -> MainRoom` を確認できる 2-way サンプル `return_playunit_demo.qsproj`

## 目的

`sample/qs_tool_server` に、Project 全体で共有される `Global Variables` 機能と、
その定義を編集する `VariablesEditorScene` を段階的に導入する。

今回の計画書は、すでに整理済みの設計書
`GLOBAL_VARIABLES_DESIGN.md` を実装へ落とすためのフェーズ分割を定義する。

主目的は次の4点。

1. `.qsproj` に `project.globalVariables` を安全に導入する
2. Runtime でグローバル変数定義を読み取り、利用可能な状態にする
3. `ProjectTopScene` から入れる `VariablesEditorScene` をプロトタイプ実装する
4. 将来の EventAction / PlayUnit 遷移 / SaveData へ接続できる土台を作る

---

## 背景

- 現在の `qs_tool_server` では `PlayUnit` や各 component の定義はあるが、Project 全体共有状態の正式な置き場はない
- 今後、PlayUnit 切り替え、進行フラグ、永続データ、ローカル変数などの概念を拡張したい
- その前段として、まずは `project.globalVariables` と `VariablesEditorScene` の最小実装を入れる必要がある
- ただし一度に全部実装すると、Runtime、保存形式、UI、EventAction 連携が同時に絡むため、phase ごとの導入が望ましい

---

## この計画で達成すること

1. `project.globalVariables` を既存 `.qsproj` 互換を保ちながら導入する
2. `system.fixed / system.persistent / user.fixed / user.persistent` の4区画を扱えるようにする
3. `ProjectTopScene -> VariablesEditorScene` の遷移導線を追加する
4. 左ナビ構成の `VariablesEditorScene` で、プロジェクト変数の最低限の追加 / 編集 / 削除 / 保存を可能にする
5. Runtime に `startupPlayUnitId`, `requestedPlayUnitId`, `currentPlayUnitId`, `returnPlayUnitId`, `isPaused`, `masterVolume` を扱う最小土台を入れる
6. 将来の PlayUnit ローカル変数対応に備え、Scene 名と UI 構造を一般化する

---

## 今回は対象外

- SaveData 実装本体
- `persistent` の全量保存 / 差分保存の最終確定
- `json` 型専用エディタ
- EventAction からのグローバル変数参照 / 書き換え実装
- PlayUnit ローカル変数の実装本体
- Runtime 実行中の変数監視 UI
- 高機能検索、差分プレビュー、履歴ブラウザ

---

## 全体フェーズ

```text
Phase 1: データモデル導入
  ↓
Phase 2: Runtime 読み込み基盤
  ↓
Phase 3: Scene 遷移導線
  ↓
Phase 4: VariablesEditorScene プロトタイプ UI
  ↓
Phase 5: 保存・検証の安定化
  ↓
Phase 6: PlayUnit 切り替えとの最小接続
```

現状では Phase 1 から Phase 6 までの最小実装は完了している。
今後は「system 変数の実挙動拡張」と「EventAction からの変数参照/代入」を後続計画として切り出す段階に入っている。

依存関係として、UI より先にデータモデルと Runtime 側の受け皿を用意する。
一方、EventAction や SaveData 連携は後段に送る。

---

## Phase 1: データモデル導入

状態: 完了

### 目的

`.qsproj` に `project.globalVariables` を追加できる状態を作る。

### 実装対象

1. `ProjectData` 相当の読み書き経路へ `globalVariables` を追加する
2. `globalVariables` 未定義の既存プロジェクトを空定義として扱う
3. 4区画の最小 shape を扱えるようにする
4. 必要なら default factory を追加する

### 想定変更箇所

- project load/save 周辺
- `AppData` または project モデル管理コード
- `.qsproj` テンプレート生成経路

### 完了条件

1. `globalVariables` が無い既存 `.qsproj` を壊さず開ける
2. 新規 project で空の `globalVariables` を持てる
3. 保存後も 4区画構造が維持される

### 実装メモ

1. `ProjectData.createDefaultGlobalVariables()` と `ProjectData.normalizeGlobalVariables()` を追加済み
2. serializer / deserializer が `globalVariables` を round-trip できるようになっている
3. `minimal_template.qsproj` に空の4区画構造を追加済み

### このフェーズではやらないこと

- UI 実装
- Runtime での意味解釈

---

## Phase 2: Runtime 読み込み基盤

状態: 完了

### 目的

Project 定義の `globalVariables` を Runtime で参照可能な形へ正規化する。

### 実装対象

1. Runtime 起動時に `project.globalVariables` を読み込む
2. `system.fixed / system.persistent / user.fixed / user.persistent` を current state として保持する
3. `globalVariables` 未定義なら空状態を生成する
4. `startupPlayUnitId`, `requestedPlayUnitId`, `currentPlayUnitId`, `returnPlayUnitId`, `isPaused`, `masterVolume` の最小初期値を扱えるようにする

### 想定変更箇所

- `AppData`
- PlayTest 起動前後の初期化経路
- project open / new project 後の session 初期化経路

### 完了条件

1. Runtime から `project.globalVariables` 相当の current state を取得できる
2. `requestedPlayUnitId` はまだ未使用でも state 上に存在できる
3. `startupPlayUnitId` が無い場合でもエラーで落ちない

### 実装メモ

1. `AppData.globalVariableState` を追加済み
2. `getRuntimeGlobalVariable()` / `setRuntimeGlobalVariable()` / `resetRuntimeGlobalVariables()` を追加済み
3. Runtime state は project 定義とは別領域として初期化される

### このフェーズではやらないこと

- EventAction 連携
- SaveData 連携
- 編集 UI

---

## Phase 3: Scene 遷移導線

状態: 完了

### 目的

`ProjectTopScene` から `VariablesEditorScene` へ遷移できる導線を入れる。

### 実装対象

1. `VariablesEditorScene` クラスの追加
2. `ProjectTopScene` に Variables Editor 入口を追加
3. `VariablesEditorScene` から `ProjectTopScene` へ戻る導線を追加

### 想定変更箇所

- scene 登録 / script 読み込み順
- `project_top_scene.js`
- scene factory / routing 相当コード

### 完了条件

1. `ProjectTopScene` から遷移できる
2. 空画面またはプレースホルダでも `VariablesEditorScene` が表示される
3. 戻る導線が機能する

### 実装メモ

1. `VariablesEditorScene` を追加済み
2. `ProjectTopScene` にボタンとショートカットを追加済み
3. `index.html` の script 読み込み順に反映済み

### このフェーズではやらないこと

- 本格的な編集 UI
- 保存処理

---

## Phase 4: VariablesEditorScene プロトタイプ UI

状態: 完了

### 目的

左ナビ構成の最低限編集可能な UI を実装する。

### 実装対象

1. 左ナビに `Project Variables` と4区画ノードを表示する
2. 中央ペインに選択区画の変数一覧を表示する
3. 右側または下部に編集フォームを表示する
4. 変数の追加 / 削除 / 選択を実装する
5. `name`, `type`, `initialValue`, `description` を編集できるようにする

### UI の最低仕様

#### 左ナビ

- `Project Variables`
- `Project Variables / system.fixed`
- `Project Variables / system.persistent`
- `Project Variables / user.fixed`
- `Project Variables / user.persistent`

#### 一覧

- 変数名
- 型
- 初期値の要約表示

#### 編集フォーム

- 変数名入力
- 型選択
- 初期値入力
- 説明入力
- 追加 / 削除 / 保存

### system 変数の初期方針

プロトタイプでは、`system` 変数も同じ UI 上に表示する。
ただし Runtime 状態値の直接編集と project 定義の編集は混同しない。

少なくとも次は編集対象にしてよい。

- `startupPlayUnitId`
- `masterVolume`

その他の system 項目は、初期版では定義表示中心でもよい。

### 完了条件

1. 4区画のいずれかに変数を追加できる
2. 既存変数の `name/type/initialValue/description` を編集できる
3. 削除操作ができる
4. UI 上で現在の project 定義を視認できる

### 実装メモ

1. 左ナビ、一覧、詳細編集ペインを実装済み
2. `name / type / initialValue / description` の編集を prompt ベースで実装済み
3. 編集時に project dirty と Runtime reset が入るようになっている

### このフェーズではやらないこと

- 高度な検索
- 並び替え
- `json` 専用エディタ
- PlayUnit ローカル変数 UI

---

## Phase 5: 保存・検証の安定化

状態: 完了

### 目的

編集結果を `.qsproj` に安全に保存し、壊れたデータを入りにくくする。

### 実装対象

1. `VariablesEditorScene` から project 保存へ接続する
2. 最低限のバリデーションを入れる
3. 既存の `globalVariables` 無し project に対する補完生成を安定化する

### 最低限の検証項目

1. 変数名が空でない
2. 同一区画内で重複名がない
3. `type` が許可値内である
4. `initialValue` が `type` と極端に矛盾しない

### 完了条件

1. 編集内容を保存して再読み込みしても壊れない
2. バリデーションエラー時に不正保存を防げる
3. 既存 project に後付けで `globalVariables` を追加できる

### 実装メモ

1. `VariablesEditorScene` から Browser save を直接呼べる
2. `Save to Browser` ボタンと `Ctrl/Cmd+S` を実装済み
3. bucket / type / initialValue の最低限バリデーションを保存前に実行する

### このフェーズではやらないこと

- すべての edge case を網羅した厳密 validator
- SaveData 連動

---

## Phase 6: PlayUnit 切り替えとの最小接続

状態: 完了

### 目的

Global Variables の存在意義を早期に確認するため、PlayUnit 切り替え関連 system 変数と Runtime の最小接続を入れる。

### 実装対象

1. Runtime 起動時に `startupPlayUnitId` を参照して最初の PlayUnit を決定する
2. 実行中の現在値として `currentPlayUnitId` を更新する
3. `requestedPlayUnitId` が設定されたら切り替え要求として扱う土台を入れる
4. 無効 ID の場合は無視して空文字へ戻す
5. 処理後に `requestedPlayUnitId` を空文字へ戻す
6. `returnPlayUnitId` を使って直前相当の PlayUnit へ戻る導線を入れる
7. EventAction から往路・復路を叩けるようにする

### 想定変更箇所

- `PlayTestScene`
- play unit 切り替え責務を持つ runtime 管理コード

### 完了条件

1. `startupPlayUnitId` で初期 PlayUnit を決められる
2. `currentPlayUnitId` が Runtime 側で追従更新される
3. `requestedPlayUnitId` はワンショット要求として処理後クリアされる
4. `returnPlayUnitId` により復路切り替えができる
5. `requestPlayUnit` / `returnPlayUnit` を使うサンプルで往復確認できる

### このフェーズではやらないこと

- 複雑な戻る履歴管理
- SaveData と組み合わせた復元

### 実装メモ

1. `activateStartupPlayUnit()` / `consumeRequestedRuntimePlayUnitSwitch()` / `activateReturnRuntimePlayUnit()` を追加済み
2. `requestPlayUnit` / `returnPlayUnit` EventAction を追加済み
3. `return_playunit_demo.qsproj` で 2-way デモを確認できる

---

## フェーズ優先順位

最短で価値を出す順序は次のとおり。

1. Phase 1: データモデル導入
2. Phase 3: Scene 遷移導線
3. Phase 4: VariablesEditorScene プロトタイプ UI
4. Phase 5: 保存・検証の安定化
5. Phase 2: Runtime 読み込み基盤
6. Phase 6: PlayUnit 切り替えとの最小接続

ただし、実装依存としては Phase 2 が Phase 6 の前提になるため、
コード着手順では `1 -> 2 -> 3 -> 4 -> 5 -> 6` を推奨する。

UI を早く見たい場合は、Phase 2 を最低限の stub に絞って先に 3 と 4 を進めてもよい。

---

## リスクと注意点

### 1. project 定義と runtime state の混同

`VariablesEditorScene` が編集するのは `.qsproj` の定義であり、
実行中 current state そのものではない。

この境界を曖昧にすると、system 変数の扱いが破綻しやすい。

### 2. `requestedPlayUnitId` を UI から誤って常用しないこと

`requestedPlayUnitId` はワンショット要求値であり、保存設定の中心にすべきではない。
UI 上での見せ方は、将来的に制限する可能性がある。

### 3. `json` 型を急いで広げないこと

初期版から複雑な `json` 編集を入れると、UI も validator も一気に重くなる。

### 4. PlayUnit ローカル変数を前倒ししないこと

`VariablesEditorScene` は将来拡張を見越すが、初回実装で local variables まで同時に入れると範囲が広がりすぎる。

---

## 各フェーズの検証観点

### Phase 1

状態: 完了

1. `globalVariables` 無し project が開ける
2. 保存後の `.qsproj` JSON が壊れない

### Phase 2

状態: 完了

1. Runtime 側で4区画にアクセスできる
2. system 初期値が読める

### Phase 3

状態: 完了

1. `ProjectTopScene` から `VariablesEditorScene` へ遷移できる
2. 戻る導線が機能する

### Phase 4

状態: 完了

1. 左ナビから区画切り替えできる
2. 追加 / 編集 / 削除が UI 上で反映される

### Phase 5

状態: 完了

1. 保存後に再読み込みしても編集内容が残る
2. 不正入力時に保存を止められる

### Phase 6

状態: 完了

1. `startupPlayUnitId` が初期起動に効く
2. `requestedPlayUnitId` が処理後に空へ戻る
3. 無効 ID は無視される
4. `returnPlayUnitId` で復路切り替えできる
5. `requestPlayUnit` と `returnPlayUnit` を使う 2-way サンプルがある

---

## この計画の出口

この計画が完了した時点で、次の状態を目指す。

1. Project には `globalVariables` 定義領域が存在する
2. `VariablesEditorScene` でグローバル変数を最低限編集できる
3. Runtime は主要 system 変数を理解できる
4. PlayUnit 切り替えや将来の EventAction 拡張へ接続できる足場が整う

この段階で初めて、次フェーズとして以下へ進みやすくなる。

1. EventAction による変数参照 / 代入
2. SaveData 連携
3. PlayUnit ローカル変数の追加
4. 条件分岐コンポーネントの設計

### 次に着手しやすい候補

1. `isPaused` を PlayTest 実行ループへ接続する
2. `masterVolume` の Runtime 反映土台を入れる
3. PlayUnit 切り替え時の camera / overlap / tween の状態整理を行う