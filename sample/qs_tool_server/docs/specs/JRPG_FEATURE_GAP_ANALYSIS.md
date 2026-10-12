# JRPG 制作に向けた機能ギャップ分析

最終更新: 2026-10-12
対象: `sample/qs_tool_server`（`www/` 以下の Web アプリ、`main.c` の C サーバー）

## 目的

このツールで 2D の JRPG 風ゲームを制作できるようにするため、現状の機能を洗い出し、不足している機能を優先度付きで整理する。
実装計画ではなく、計画を立てるための事実整理である。

## 調査方法と注意

- コードとサンプル（`assets/*.qsproj`）を読んで判定した。実行による検証は一部のみ。
- 判定基準:
  - **あり**: コードとサンプルで動作を確認できる
  - **部分的**: 変数・イベントの組み合わせで手組みできる、または一部だけ実装されている
  - **なし**: 機能もサンプルもない
- 行番号は調査時点のものであり、変更があるとずれる。
- 既存ドキュメントと実装が食い違う箇所は [セクション 9](#9-既存ドキュメントとの食い違い) にまとめた。

---

## 1. 全体像

- ブラウザ上の Canvas ベースのドット絵・タイルセット・マップ・PlayUnit（ゲームシーン）編集ツール。Vanilla JS で外部ライブラリは使っていない。
- C サーバー（`main.c`）は `www/` の配信と、`POST /api/json_test`・WebSocket の echo だけを持つ。ゲームのビルドや配布パッケージを作る機能はない。
- 起動時は `TitleScene` に入る（`www/index.html`）。
- 「ゲーム」は `PlayUnit`（オブジェクトツリーとコンポーネントの集まり）として定義し、同じ Web アプリ内の `PlayTestScene` で動かす。

---

## 2. 既存機能の概要

### 2.1 ゲームのデータ構造

- `ProjectData`（`www/js/project/project_data.js`）
  - `assets: { pixelDocuments, tilesets, maps, playUnits }`
  - `settings: { defaultChipWidth: 16, defaultChipHeight: 16 }`
  - `globalVariables`
- `PlayUnitData`（`www/js/play_unit/play_unit_data.js`）: `{ id, name, objects[] }`。既定で `Root`、`CameraObject`、`PlaySettingsObject` を持つ。
- `PlayObjectData`: `{ id, name, enabled, parentId, children, components[] }`
- `ComponentData`: `{ type, enabled, data }`。`data` は自由形式の JSON。

### 2.2 コンポーネント一覧

| 種別 | 内容 | 実行時の対応 |
|---|---|---|
| Transform | x, y, z, rotation, scale | 対応 |
| Collider | rect、isTrigger | AABB の判定のみ |
| Camera | zoom, viewport, followTargetObjectId, followLerp | 対応（範囲制限なし） |
| PlaySettings | defaultCameraObjectId | 対応 |
| Controller | 移動（inputMode は `player1` のみ、moveSpeed 既定 120） | 対応（壁の押し戻しなし） |
| Image | pixelDocument 参照、keepAspect、origin | 対応 |
| Rectangle | rectangle / circle / polygon / triangle / star | 対応 |
| Text | `${var}` テンプレート展開、色、フォント、折り返し | 対応 |
| Trigger | triggerOn、eventId、once、duration、targetObjectId | 対応 |
| EventAction | listenTo と action | 対応 |
| Tilemap | mapAssetId、layerId | **エディタのみ。実行時に描画されない** |
| Conditional | 条件分岐 | 対応（エディタに追加ボタンなし） |
| UICanvas / UITransform | screen-space の UI | 対応（エディタに追加ボタンなし。生成ツール経由） |

### 2.3 マップとタイルセット

- マップ（`createMapData`、`www/js/app_data.js`）
  - `{ version, width(既定 24), height(既定 18), tileWidth, tileHeight, tilesetId, layers[{id,name,visible,locked,tiles[]}], selectedLayer, selectedTileRef, view }`
  - `tiles` は各セルのタイル index（-1 は空）。
- マップエディタ（`www/js/scene/map_editor_scene.js`）: セルの配置と消去、マップサイズ変更、タイルセット選択、ズーム、パン、グリッド表示。
- タイルセット（`www/js/tileset_data.js`）
  - チップの cols × rows 格納。チップごとに多レイヤーのピクセルを持つ。
  - 通過フラグ `passFlags[r][c]`（既定 true）。エディタでは編集できる。
  - 実行時（PlayTest や Runtime）では使われていない。

### 2.4 グローバル変数

- 4 区画: `system.fixed` / `system.persistent` / `user.fixed` / `user.persistent`
- 型: `string` / `number` / `boolean` / `json`
- 実行時 API: get / set / has / delete / list / reset（`AppData.globalVariableState`）。`set` は定義済みの変数だけを更新し、型を強制する。
- 予約・慣例名
  - `system.fixed`: `startupPlayUnitId`、`requestedPlayUnitId`、`currentPlayUnitId`、`returnPlayUnitId`、`isPaused`、`timer`
  - `system.persistent`: `masterVolume`
  - `isPaused` と `masterVolume` は JS 側で参照されていない。
- `timer` は PlayTest の update で自動加算される。`persistent` は将来のセーブ対象として設計されているが、セーブ機能はまだない。

### 2.5 イベント・スクリプト

**Trigger**（発火源。`www/js/scene/play_test_scene.js`）

- マウス・ポインタ: click、pointerEnter / Move / Leave / Down / Up、dragStart / Move / End
- overlap: Controller の Collider と isTrigger の rect Trigger が**進入したときだけ**発火する。退出イベントはない。
- onTimer: `duration` 秒後に発火。`once` で 1 回限り。
- onUpdate: 毎フレーム発火。

**EventAction**（`_executeAction`）

- setProperty: 任意コンポーネントの任意プロパティを上書きする。
- setEnabled: オブジェクトの有効・無効を切り替える。
- playTween: number プロパティを from から to へ補間する。easing は linear / easeIn / easeOut / easeInOut。
- fireEvent: 別イベントを連鎖発火する。1 フレームあたり 64 パスまでのガードがある。
- setGlobalVariable: set / add / subtract / multiply / divide。number 型のみ算術を行う。
- requestPlayUnit: `requestedPlayUnitId` を設定して PlayUnit を切り替える。
- returnPlayUnit: 直前の PlayUnit へ戻る。戻り先は 1 段だけ保持し、スタックではない。

**Conditional**（`www/js/component/conditional_action.js`）

- 条件型: compare（数値比較）、equals（文字列一致）、truthy、has（キーの存在）、exists（変数の存在）
- 左右の値は `${var.path}` テンプレートで解決する。
- 実行は「最初に一致した branch の単一アクション」。どれも一致しなければ `defaultAction`。
- `www/js/component/conditional_action_test.js`（31 ケース）は `index.html` から読み込まれておらず、ブラウザでは実行されない。

### 2.6 プレイテストの実行環境（`PlayTestScene`）

- 起動: `activateStartupPlayUnit` → `PlayUnitRuntime.fromPlayUnit`（`www/js/play_unit/play_unit_runtime.js`）
- ループ: `CanvasManager` の requestAnimationFrame（60fps）で `update(dt)` と `render` を呼ぶ。
- 入力: keydown、keyup、mouse 系を購読する。touch は購読していない。
- 移動: Controller のみ。WASD・矢印キー。座標を直接加算するだけで、コライダによる通行不可はない。
- 衝突判定: AABB のみ。用途はトリガーの重なりとポインタの hit test。
- カメラ: follow と lerp による追従のみ。範囲制限、中央基準、offset、screen-space の分離はない。
- UI: UICanvas（sortOrder）と UITransform（anchor、pivot）で screen-space UI を描画し、UI を優先してヒットテストする。
- デバッグ: グローバル変数パネルを描画する。

### 2.7 エディタ一覧

| シーン | ファイル | 主な機能 |
|---|---|---|
| TitleScene | `www/js/scene/title_scene.js` | New、Continue、Load、ブラウザ保存プロジェクトの一覧と削除 |
| ProjectTopScene | `www/js/scene/project_top_scene.js` | 各シーンへのハブ。保存（ブラウザ・Save As・.qsproj 出力） |
| EditorScene | `www/js/scene/editor_scene.js` | ドット絵とタイルセット。ピクセル描画、レイヤー、選択、Undo / Redo、通過フラグ、PNG・JSON・QTS の入出力 |
| MapEditorScene | `www/js/scene/map_editor_scene.js` | マップのセル配置、サイズ変更、ズーム、パン、グリッド |
| PlayUnitEditorScene | `www/js/scene/play_unit_editor_scene.js` | オブジェクトツリー、コンポーネントの追加と削除。data は `window.prompt` の JSON で編集する。ビジュアルな inspector はない |
| VariablesEditorScene | `www/js/scene/variables_editor_scene.js` | 4 区画の変数定義の追加、削除、型、初期値。保存時に検証する |

### 2.8 出力とファイル形式

- `.qsproj`: 平文 JSON（`format: "qsproj"`、`version: 1`）。画像は base64 で埋め込む。暗号化や難読化はない。
- ブラウザ保存: IndexedDB（`qs_tool_server_db`、stores は projects / sessions / assets）。
- 画像・タイルセット: PNG、JSON、QTS バイナリ（`www/js/util/pixel_data_converter.js`）。
- ゲームとしての単体配布、Runtime の分離は**ない**。PlayUnit やマップ単体の書き出しもない。

### 2.9 サンプル（`assets/*.qsproj`、14 件）

| ファイル | 内容 |
|---|---|
| simple_title.qsproj | タイトル画面の静的表示 |
| rpg_demo.qsproj | RPG 風 HUD（HP・MP バー、静的テキスト） |
| rpg_hud_world_button_demo.qsproj | Controller 移動、カメラ追従、UICanvas HUD、ワールドボタン |
| ui_canvas_demo.qsproj | UICanvas とボタンの click でテキストを変更 |
| ui_pointer_events_demo.qsproj | pointer 系、drag 系、click の確認 |
| click_event_demo.qsproj | click による変更、表示切替、playTween |
| arithmetic_demo.qsproj | setGlobalVariable の四則演算 |
| phase4_set_global_variable_check.qsproj | fixed と persistent への代入検証 |
| timer_sample_basic.qsproj | onTimer、onUpdate、時間による分岐 |
| conditional_minimal.qsproj | Conditional の最小例 |
| conditional_error_status_test.qsproj | 存在しない変数参照時のエラー確認 |
| test_conditional_demo.qsproj | 条件テストと敵 AI の分岐 |
| conditional_battle_event_demo.qsproj | **唯一の戦闘サンプル**。攻撃、ポーション、敵ターン、撃破判定、戦闘ログ。ロジックのみで、ターン制 UI や演出はない |
| return_playunit_demo.qsproj | 2 PlayUnit の往復（画面遷移の例） |

---

## 3. JRPG に必要な要素の判定

| 要素 | 判定 | 根拠・補足 |
|---|---|---|
| メッセージウィンドウ / 会話 | 部分的 | Text と Rectangle で手組みできる。送り、タイプライター、話者欄はない |
| 選択肢 | 部分的 | click Trigger 付きの Rectangle でボタンを作れる。選択 UI の組込みはない |
| 戦闘システム | 部分的 | サンプルのロジックのみ。ターン管理、戦闘画面、ダメージ式、演出待ちはない |
| ステータス・レベルアップ | 部分的 | 変数と四則演算と Conditional で表現できる。成長テーブルや式はない |
| アイテム・装備・所持品 | 部分的 | json 型で格納し、has で確認できる。追加・削除の操作はない |
| パーティ管理 | なし | 概念もアクションもない |
| メニュー画面 | 部分的 | PlayUnit の切替で画面遷移はできる。カーソル操作などの UI はない |
| セーブ / ロード（ゲーム内） | なし | プロジェクト保存は別機能としてある |
| フラグ管理 | あり | boolean 変数、set、truthy と equals |
| マップ遷移 | 部分的 | PlayUnit を別マップとして切り替え、出現位置と向きを指定できる（warp、P0-4）。扉は overlap と sequence で組める。マップ間で持ち回るオブジェクトはない（プレイヤーは各マップに複製する） |
| タイルマップ編集 | あり | MapEditorScene。ただし単一レイヤー実質、タイルセットは 1 つ前提 |
| タイルセットの通過フラグ | 部分的 | 編集はできるが、実行時に使われない |
| NPC 配置 | 部分的 | Image や Rectangle と Transform で配置できる。タイルマップ上の配置物の概念はない |
| NPC の AI 移動 | なし | 組込み AI はなく、経路探索や乱数もない |
| プレイヤー移動 | 部分的 | Controller で移動できる。通行不可はない |
| 当たり判定 | 部分的 | AABB の進入トリガーのみ。退出イベントや押し戻しはない |
| BGM / SE | なし | 音声の処理がない。masterVolume は予約名のみ |
| ターンベース演出 | 部分的 | playTween は複数同時に使える。sequence（wait / waitEvent / waitComplete）で順次演出を組める（P0-3）。並列や繰り返しはない |
| タイマー | あり | system.fixed.timer、onTimer、onUpdate |
| 乱数 | なし | 実装がない |
| 式評価 | なし | 四則演算は setGlobalVariable の op のみ |
| 配列・辞書 | 部分的 | json 型で格納のみ。操作アクションはない |
| イベント連鎖 | あり | fireEvent（1 フレーム 64 パスまで） |
| 条件分岐 | あり | Conditional（5 種類）。各 branch は単一アクション |
| 入力（キーボード・マウス・ドラッグ） | あり | PlayTest で処理する |
| タッチ入力（PlayTest） | なし | touch を購読していない |
| 固定 UI（HUD） | あり | UICanvas と UITransform |
| スプライトのフレームアニメ | なし | 未実装 |
| ゲームの単体配布 | なし | Runtime の分離がない |
| 暗号化・難読化 | なし | .qsproj は平文 |

---

## 4. 不足している機能（優先度順）

### P0: ゲームとして最低限成立させるために必要

1. **フィールドの歩行と通行判定**
   - 実行時にタイルマップが描画されない（Tilemap は PlayTest で未対応）。
   - タイルセットの通過フラグが実行時に使われていない。
   - コライダによる押し戻しがない。
2. **メッセージウィンドウ**
   - 送り、タイプライター表示、話者名、選択肢 UI が組込みでない。
3. **イベントの順次実行**
   - wait / delay がない。
   - Conditional の各 branch は単一アクションしか実行できない。
   - トゥイーン完了のイベントがない。
   - 会話や戦闘演出は順番に実行することが前提なので、これがないと組みにくい。
4. **マップ間の遷移（ワープ）**
   - PlayUnit の切替はあるが、マップ内の位置指定の移動と、切替時のプレイヤー位置の引き継ぎがない。
5. **ゲーム内のセーブとロード**
   - `persistent` 変数の永続化がない。今あるのはプロジェクト保存（IndexedDB と .qsproj）で、これはゲームのセーブとは別物。

### P1: JRPG らしさに必要

6. **戦闘システム**: ターン管理、戦闘画面、コマンド選択、ダメージ式、敵グループ、勝敗と逃走の判定。部品として使える形にする。
7. **データベース（キャラ・敵・アイテム・スキル）**: テーブル形式で定義して ID で参照する仕組みと、そのエディタ。現状は変数と json で手組みしている。
8. **式評価・乱数・配列と辞書の操作**: ダメージ式、確率判定、インベントリの追加・削除・検索。
9. **メニュー画面**: カーソル移動、項目選択、ステータス・アイテム・装備の各画面。
10. **パーティ管理**: 構成の概念と操作。
11. **レベルと成長テーブル**: 経験値とレベルアップの計算をデータとして定義する。

### P2: 演出と制作効率

12. **BGM / SE**: 音声の再生機構がない。masterVolume と接続する。
13. **スプライトのフレームアニメ**: 歩行アニメなど。
14. **NPC の AI 移動**: ランダム歩行や経路移動。乱数が前提。
15. **当たり判定の退出イベント**
16. **タッチ入力**: PlayTest が touch を購読していないため、スマートフォンでは操作できない。
17. **Conditional と UICanvas の追加 UI**: PlayUnit エディタに追加ボタンがなく、JSON 直接編集か生成スクリプト経由になっている。
18. **ゲームの単体配布（Runtime の分離）**: 暗号化は要件次第。

---

## 5. 推奨する進め方（縦切りの最小デモ）

いきなり全部を作るのは難しいので、次の縦切りを先に完成させる。

- 1 マップ（タイルマップの実行時描画と通行判定）
- NPC との会話（メッセージウィンドウと選択肢）
- 戦闘 1 回（ターン制。ダメージ式は P1 の式評価の範囲で）
- セーブとロード

この縦切りに必要な項目は、P0 の 1〜3 と 5、P1 の 6 と 8 である。
P0 の 3（順次実行の基盤）は後続の多くの前提になるため、最初に着手するのが効率的である。

---

## 6. 未決定事項

- 目標の範囲をどこに置くか。
  - (a) 最小デモ（上記の縦切り）を先に作る
  - (b) ターン制 RPG として完成させる（P0 と P1 の全部）
  - (c) まずエンジン基盤（順次実行とタイルマップの実行時描画）を固める
- 単体配布（P2-18）を目標に含めるか。

---

## 7. 関連ドキュメント

- [FEATURE_ROADMAP.md](FEATURE_ROADMAP.md): 既存の機能案（ゲームへの持ち込みは未実装とされている）
- [EVENT_ACTION_DESIGN.md](EVENT_ACTION_DESIGN.md)、[CONDITIONAL_COMPONENT_DESIGN.md](CONDITIONAL_COMPONENT_DESIGN.md): イベント系の設計
- [PLAY_UNIT_DESIGN.md](PLAY_UNIT_DESIGN.md): PlayUnit の設計（PlayUnit と map asset の完全統合は対象外とされている）
- [GLOBAL_VARIABLES_DESIGN.md](GLOBAL_VARIABLES_DESIGN.md): 変数の設計（persistent はセーブ対象として予定）
- [../WORK_STATUS.md](../WORK_STATUS.md): 実装状況

---

## 8. 参考: 既存 TODO の状況

[WORK_STATUS.md](../WORK_STATUS.md) の未完了項目のうち、本分析に関係するもの。

- Tilemap コンポーネントの参照解決と PlayTest での描画
- Trigger の event 実行本体（EventAction は実装済みのため、記載が古い可能性がある）
- Camera の詳細（中央基準、offset、screen-space の分離）
- アニメーション（フレームパネル、フレーム配列、プレビュー、GIF）
- 図形ツールの直線、矩形、楕円

---

## 9. 既存ドキュメントとの食い違い

- `EVENT_SYSTEM_ANALYSIS_REPORT.md` は「Conditional が最優先で未実装」と書いているが、現在の実装では完了している。
- `WORK_STATUS.md` の IndexedDB 項目は未チェックだが、実装と確認済みのサブ項目がある。
- `WORK_STATUS.md` の Trigger event 実行本体は未チェックだが、EventAction と Conditional は実装済み。
- `WORK_STATUS.md` で挙げられている `conditional_simple_test.qsproj` は `assets/` に存在しない。
- `CLAUDE.md` の「`www/index.html` を開く」は、サーバー経由で配信する前提と実態が合わない可能性がある。

---

## 10. 進捗

- **P0-1（フィールドの歩行と通行判定）**:
  - 完了: キャラクターチップセット（歩行アニメ用の 4 方向 × 3 フレーム）の作成・保存と、プレイヤー 1 体の移動・歩行アニメ（詳細は [CHARACTER_CHIPSET_PLAN.md](CHARACTER_CHIPSET_PLAN.md)）。
  - 完了: タイルマップの実行時描画（倍率 1〜8）、通過フラグによる通行判定、マップ外への移動の禁止。
  - 残り: コライダ同士の一般的な押し戻し、NPC・オブジェクトとの衝突、プレイヤーより手前に描くレイヤー。
  - 既知の制約: Collider は Transform の左上基準のため、足元中央の origin と併用するには offset が必要（デモは offset で合わせている）。
- **P0-2（メッセージウィンドウ）**: 9 スライスの枠（pixelDocument の境界、PlayTest と EditorScene）、表示文字数（Text の `revealChars`）、タイプライター（EventAction の `typewriter`、完了イベント、スキップ）、クリックで送るデモを実装した。詳細は [MESSAGE_WINDOW_DESIGN.md](MESSAGE_WINDOW_DESIGN.md) と [NINE_SLICE_DESIGN.md](NINE_SLICE_DESIGN.md)。
  - 残り: 選択肢 UI、話者名の枠組み（部品の組み合わせで作れる）。自動送りは P0-3 の `waitEvent` の `timeout` で作れるようになった。
- **P0-3（イベントの順次実行）**:
  - 完了: EventAction の `sequence`（`steps` を順に実行）。step の `wait`（秒）、`waitEvent`（イベント待ち。`timeout` で時間切れでも進む）、`waitComplete`（typewriter、playTween、入れ子の sequence の完了を待つ）、`playTween` と `sequence` の `completeEventId`、`onBusy`（ignore / restart）。Conditional の branch からも sequence を実行できる。詳細は [EVENT_ACTION_DESIGN.md](EVENT_ACTION_DESIGN.md)。
  - 完了: P0-2 のメッセージウィンドウを sequence で進める形に置き換えた（[MESSAGE_WINDOW_DESIGN.md](MESSAGE_WINDOW_DESIGN.md) の 11）。
  - 残り: 繰り返し、並列、sequence 内の分岐、途中の中断、専用の編集 UI（現状は JSON 編集のみ）。
  - 既知の制約: PlayUnit を切り替えると、進行中の sequence は再開しない（窓などの表示は残る）。
- **P0-4（マップ間の遷移）**:
  - 完了: EventAction の `warp`（行き先の PlayUnit、出現位置の `spawnObjectId` または `spawnX` / `spawnY`、`facing`）。同じ PlayUnit 内の移動にも使える。PlayUnit に入るたびに `system.playUnitEnter` を送る（到着演出用）。PlayUnit 切替時の後始末（タイマー、重なり、保留イベント、進行中の sequence・tween・typewriter）を requestPlayUnit、returnPlayUnit、warp で共通化した。カメラは切替で瞬時に追従する。デモは扉で室内へ移り、フェードで切り替える（[EVENT_ACTION_DESIGN.md](EVENT_ACTION_DESIGN.md) の warp と PlayUnit 切替時の後始末）。
  - 残り: プレイヤーの持ち回り（共有オブジェクト）、遷移中の入力ロック、マップ名の表示、warp 専用の編集フォーム（JSON 編集のみ）。
  - 既知の制約: 出現位置を通行不可のマスに置くと身動きが取れなくなる（自動補正はない）。窓が開いたまま扉を踏むと、窓は残る（P0-3 と同じ）。
