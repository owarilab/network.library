# PlayUnit 実装計画書

最終更新: 2026-04-27

## 目的

`PlayUnit` 構造を `qs_tool_server` に段階的に導入し、
保存、一覧表示、最小編集、将来の runtime 接続までを
破壊的変更を避けながら進められるようにする。

この文書は、[PLAY_UNIT_DESIGN.md](PLAY_UNIT_DESIGN.md) で合意した設計を
実際の作業へ落とし込むための phase 単位の実装計画書である。

---

## 前提

- `PlayUnit` は `Scene` とは別概念であり、ゲーム側の編集対象単位として扱う
- `PlayUnitData` は `id`, `name`, `objects` を持つ最小構造から始める
- `PlayObjectData` は `id`, `name`, `enabled`, `parentId`, `children`, `components` を持つ
- `tilemaps` や `settings` は top-level property ではなく component として表現する
- プレイヤーに関する構造はこの計画の直接対象外とし、別設計で扱う

---

## この計画で達成すること

1. `ProjectData` が `playUnits` を保持できるようにする
2. `PlayUnitData` / `PlayObjectData` / `ComponentData` の最小データ層を追加する
3. `.qsproj` 保存 / 復元に `playUnits` を含める
4. `ProjectTopScene` から `PlayUnit` を作成・選択できるようにする
5. `PlayUnitEditorScene` の雛形を作る
6. 後続フェーズで runtime や object 編集 UI を足せる状態にする

---

## 今回は対象外

- 本格的な object tree 編集 UI
- component ごとの専用 inspector
- runtime の実行ループ
- プレイヤー制御や敵 AI
- 物理演算
- スクリプトイベント
- PlayUnit と map asset の完全統合

---

## 実装状況サマリ

### 実装済み

- Phase 1 完了: `www/js/play_unit/play_unit_data.js` を追加し、`PlayUnitData` / `PlayObjectData` / `ComponentData` の最小データ層を実装済み
- Phase 2 完了: `ProjectData` と `ProjectSession` に `playUnits` と active document 対応を実装済み
- Phase 3 完了: `.qsproj` serializer と browser 保存層に `playUnits` の保存 / 復元を実装済み
- Phase 4 完了: `ProjectTopScene` から `PlayUnit` の作成、選択、オープン導線を実装済み
- Phase 5 完了: `PlayUnitEditorScene` の雛形を追加し、選択中 `PlayUnit` の最小表示を実装済み
- Phase 6 実装進行: object の追加 / 削除 / 改名、component の追加 / 削除、component parameter の最小 JSON 編集、object / component 一覧スクロールを実装済み
- Phase 6 実装進行: `Text` component の最小編集項目として `text`, `font`, `color`, `alpha`, `align`, `baseline`, `wrap`, `maxWidth`, `lineHeight`, `strokeColor`, `strokeWidth`, `backgroundColor`, `padding` を扱える状態まで拡張済み
- Phase 7 実装進行: `PlayUnitRuntime` と `PlayTestScene` を追加し、`Transform + Text` の最小 preview 描画を実装済み
- 周辺 UI 調整として、`Project Assets` と `Browser Projects` の一覧スクロールと表示調整を実装済み

### 未実装

- Phase 7: `Tilemap` component の参照解決
- Phase 7: `Trigger` component の最小評価
- Phase 7: `PlaySettings` の camera 反映など preview 範囲の拡張

---

## 想定追加ファイル

### データ層

- `www/js/play_unit/play_unit_data.js`
  - `PlayUnitData`
  - `PlayObjectData`
  - `ComponentData`

### シーン

- `www/js/scene/play_unit_editor_scene.js`
  - `PlayUnit` 編集用の最小シーン

### 将来追加候補

- `www/js/play_unit/play_unit_runtime.js`
- `www/js/ui/play_unit_object_tree_window.js`
- `www/js/ui/play_unit_inspector_window.js`

---

## 変更対象ファイル

### 保存 / プロジェクト基盤

- `www/js/project/project_data.js`
- `www/js/project/project_session.js`
- `www/js/app_data.js`

### シーン導線

- `www/js/scene/project_top_scene.js`
- `www/index.html`

### 必要に応じて変更

- `docs/WORK_STATUS.md`
- PlayUnit 関連の追加設計書

---

## フェーズ構成

## Phase 1: データ層の導入

### 目的

- `PlayUnit` を JavaScript 上で扱える最小構造を追加する
- 既存コードへ強く依存しない独立したデータ層を先に作る

### タスク

1. `www/js/play_unit/play_unit_data.js` を追加する
2. `PlayUnitData` の `createDefault(name)` を定義する
3. `PlayObjectData` の最小生成 API を定義する
4. `ComponentData` の最小生成 API を定義する
5. ID 採番の最小ルールを決める
6. `objects` 配列を正本とするユーティリティを用意する

### 完了条件

- `PlayUnitData` を 1 つ生成できる
- 空の `PlayObjectData` を追加できる
- `Tilemap` / `PlaySettings` / `Text` / `Trigger` component をデータとして保持できる

### このフェーズで作るもの

- `play_unit_data.js`

### このフェーズではやらないこと

- UI 接続
- 保存処理への組み込み

---

## Phase 2: ProjectData への統合

### 目的

- `Project` が `playUnits` を保持できるようにする
- project 内の他アセットと同様に扱えるようにする

### タスク

1. `ProjectData.assets` に `playUnits` を追加する
2. `addPlayUnit()` を追加する
3. `findPlayUnitById()` を追加する
4. `getAssetByRef()` に `playUnit` を追加するか、別取得 API を設けるか整理する
5. `ProjectSession.activeDocumentRef` で `playUnit` を扱えるようにする

### 完了条件

- project に `playUnits[]` を保持できる
- `playUnit` を ID で取得できる
- active asset として `playUnit` を選べる

### 変更ファイル

- `www/js/project/project_data.js`
- `www/js/project/project_session.js`

---

## Phase 3: 保存 / 復元の統合

### 目的

- `.qsproj` と browser 保存で `playUnits` を失わないようにする

### タスク

1. serializer に `playUnits` 出力を追加する
2. deserialize に `playUnits` 復元を追加する
3. browser 保存層で追加変更が必要か確認する
4. `AppData` から `playUnit` の保存対象連携を整理する

### 完了条件

- project を保存して再読み込みしても `playUnits` が残る
- `playUnits` の component 構造が JSON 上で壊れない

### 変更ファイル

- `www/js/project/project_session.js`
- `www/js/app_data.js`

### 検証

- 最小 `playUnit` を含む project を export / import する
- browser 保存後に再ロードして `playUnit` が残ることを確認する

---

## Phase 4: ProjectTopScene からの導線追加

### 目的

- ユーザーが `PlayUnit` を作成、選択、オープンできるようにする

### タスク

1. `ProjectTopScene` に `PlayUnit` セクションを追加する
2. 新規 `PlayUnit` 作成ボタンを追加する
3. `PlayUnit` 一覧表示を追加する
4. 選択した `PlayUnit` を active document に設定できるようにする
5. `PlayUnitEditorScene` への遷移導線を追加する

### 完了条件

- 新規 `PlayUnit` を 1 つ作れる
- project top 上で一覧表示される
- 選択して editor scene へ遷移できる

### 変更ファイル

- `www/js/scene/project_top_scene.js`

---

## Phase 5: PlayUnitEditorScene の雛形

### 目的

- `PlayUnit` を編集するための専用 scene を用意する
- 最初は本格編集ではなく、読み込みと基本表示を成立させる

### タスク

1. `play_unit_editor_scene.js` を追加する
2. active `PlayUnit` を読み込む
3. object 一覧の最小表示を行う
4. status 表示と `ProjectTopScene` へ戻る導線を追加する
5. 最小の保存導線をつなぐ

### 完了条件

- `PlayUnitEditorScene` が開く
- 選択中 `PlayUnit` の object 数や object 名を表示できる
- project top に戻れる

### このフェーズではやらないこと

- object tree 編集
- component inspector
- drag & drop

### 追加ファイル

- `www/js/scene/play_unit_editor_scene.js`

---

## Phase 6: 最小 object 編集

### 目的

- `PlayUnit` 内 object を増減できるようにする
- component ベース構造の編集を最小限始める

### タスク

1. object 追加ボタンを用意する
2. object 名変更を可能にする
3. object 削除を可能にする
4. `Transform` component の追加を可能にする
5. `Tilemap` / `PlaySettings` / `Text` / `Trigger` の追加を可能にする
6. component parameter の最小 JSON 編集を可能にする
7. `Text` component は runtime 確認用の表示系 parameter を先行して扱えるようにする

### 完了条件

- object を追加 / 削除 / リネームできる
- 最小 component を object に付与できる
- runtime 確認に必要な parameter を JSON で編集できる

### Phase 6 補足

- `Text` component の最小 runtime 確認対象は `text`, `font`, `color`, `alpha`, `align`, `baseline`, `wrap`, `maxWidth`, `lineHeight`, `strokeColor`, `strokeWidth`, `backgroundColor`, `padding` とする
- 専用 inspector はまだ作らず、当面は JSON 編集で値を調整する

### 依存

- Phase 5 完了

---

## Phase 7: Runtime 接続の下準備

### 目的

- `PlayUnitData` を実行用に変換する入口を用意する
- 後続の play test 実装へ進みやすくする

### タスク

1. `PlayUnitRuntime` の最小設計を別紙化する
2. `PlayUnitData` → `PlayUnitRuntime` 変換 API を作る
3. `Tilemap` component の参照解決方法を定義する
4. `Trigger` component の最小評価方法を定義する
5. `PlayTestScene` 接続用の stub を追加する
6. 最初の preview 対象を `Transform + Text` に限定して描画確認できるようにする

### 完了条件

- `PlayUnitRuntime` の雛形ができる
- 最小 `PlayUnit` を runtime へ渡せる

### Phase 7 補足

- 最初の preview は `Transform + Text` のみを対象とする
- `Text` preview では `align`, `baseline`, `wrap`, `maxWidth`, `lineHeight`, `strokeColor`, `strokeWidth`, `backgroundColor`, `padding`, `alpha` を扱う
- `Tilemap`, `Trigger`, camera 反映はこの後続タスクで広げる

### このフェーズではやらないこと

- プレイヤー制御
- 完全なゲームループ
- スクリプト実行

---

## 優先順位

最小の価値を早く出すため、推奨順は次のとおり。

1. Phase 1: データ層の導入
2. Phase 2: ProjectData への統合
3. Phase 3: 保存 / 復元の統合
4. Phase 4: ProjectTopScene からの導線追加
5. Phase 5: PlayUnitEditorScene の雛形
6. Phase 6: 最小 object 編集
7. Phase 7: Runtime 接続の下準備

---

## フェーズごとの検証観点

### Phase 1

- 空の `PlayUnitData` を生成できる
- object / component を JSON 化しても壊れない

### Phase 2

- project に `playUnits` を追加できる
- active document として `playUnit` を選べる

### Phase 3

- `.qsproj` 保存 / 読込で `playUnits` が維持される
- browser 保存後の再ロードで構造が残る

### Phase 4

- UI から `PlayUnit` を新規作成できる
- `ProjectTopScene` 一覧から開ける

### Phase 5

- `PlayUnitEditorScene` が表示される
- 選択中 `PlayUnit` の object が見える

### Phase 6

- object の追加 / 削除 / 改名ができる
- component の最小追加ができる
- component parameter を JSON で編集して `Text` の表示確認ができる

### Phase 7

- runtime 変換 API が最小 `PlayUnit` を受け取れる
- `Transform + Text` を PlayTest 上で preview 表示できる

---

## 実装上の注意点

1. 既存の `map` や `tileset` フローを壊さない
2. `PlayUnit` は既存資産の上位概念ではなく、まずは並列アセットとして導入する
3. serializer 変更は既存 project との後方互換を意識する
4. scene 導線追加は `ProjectTopScene` 中心で進め、タイトル側を広げすぎない
5. editor scene は最小表示から入り、いきなり複雑な inspector を作らない

---

## 関連文書

- [PLAY_UNIT_DESIGN.md](PLAY_UNIT_DESIGN.md)
- [PROJECT_SCENE_FLOW_PLAN.md](PROJECT_SCENE_FLOW_PLAN.md)
- [MAP_EDITOR_SCENE_PLAN.md](MAP_EDITOR_SCENE_PLAN.md)

---

## 結論

`PlayUnit` 導入は、いきなり editor や runtime を大きく作るのではなく、
次の順で進めるのが最も安全である。

1. データ構造を入れる
2. project 保存へ統合する
3. scene 導線を追加する
4. editor の雛形を置く
5. 最小編集を入れる
6. runtime 接続へ進む

この順序で進めれば、既存の `qs_tool_server` を壊さずに、
phase ごとに確認しながら PlayUnit 基盤を拡張できる。