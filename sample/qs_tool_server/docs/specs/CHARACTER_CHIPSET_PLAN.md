# キャラクターチップセット設計書（歩行アニメ + プレイヤー移動）

ステータス: 実装済み（フェーズ 1〜3 の完了条件を Playwright で確認。実装差分は[節 13](#13-実装時の差分)）
最終更新: 2026-10-12
関連: [JRPG_FEATURE_GAP_ANALYSIS.md](JRPG_FEATURE_GAP_ANALYSIS.md)（P0-1 の前段）、[MAP_CHIP_SPEC.md](MAP_CHIP_SPEC.md)、[BINARY_FORMAT_PLAN.md](BINARY_FORMAT_PLAN.md)

---

## 1. 目的とスコープ

### 目的

キャラクターの歩行アニメ用チップセットを作る機能を追加し、そのデータで**プレイヤー1体を表示・移動・歩行アニメさせる**ところまでを MVP とする。エディタは新規シーンを作らず、既存の EditorScene に同居させる。

### MVP に含むもの

- キャラクターチップセットの新規作成、編集、保存、読込（.qsproj とブラウザ保存）
- 1枚のシートに、4方向 × 3フレーム（立ち1 + 歩き2）を描く
- 左向きを右向きから反転コピーする補助
- エディタ内での歩行アニメのプレビュー
- PlayTest でプレイヤー1体を表示し、移動に応じて向きと歩行フレームを切り替える
- PlayTest 中の移動を playUnit データに書き込まない（[節 6.3](#63-playtestscene-との連携)）

### MVP に含まないもの

- NPC、壁による衝突、会話、戦闘、ワープ、タッチ操作、単体配布
- 半透明を含む絵柄への対応（[節 4.4](#44-qts-の制約キャラクター用途での注意)）
- Undo / Redo（キャラクターチップの編集操作）

---

## 2. 前提（既存コードの現状）

調査時点の確認結果。行番号は変わる可能性がある。

| 項目 | 現状 | 場所 |
|---|---|---|
| チップ格納 | `TilesetData(chipW, chipH, cols, rows)`。チップ単位で `LayerData` を持つ | [tileset_data.js](../www/js/tileset_data.js) L16 |
| チップ取得 | `getChipLayerData(col, row)` | 同 L60 |
| チップ操作 | clear / copy / paste / swap / add / remove。Undo なし | 同 L112 以降 |
| アセット追加 | `ProjectData.addTileset`、正規化 `_normalizeTileset`、ID は `_normalizeId(prefix)` | [project_data.js](../www/js/project/project_data.js) L260, L363, L408 |
| 参照解決 | `ProjectData.getAssetByRef(ref)` は type で switch | 同 L296 |
| .qsproj 直列化 | `ProjectSerializer` が assets を種別ごとに直列化・復元 | [project_session.js](../www/js/project/project_session.js) L192, L311 |
| ブラウザ保存 | 収集 `_collectProjectAssets`、Save As の ID 振り直し `_createSaveAsCopy`、読込は type で振り分け | [project_browser_storage.js](../www/js/project/project_browser_storage.js) L168, L191, L334-341 |
| タイルセット編集 | EditorScene の tileset モード。`AppData.layerData` の getter が選択チップの LayerData を返す | [editor_scene.js](../www/js/scene/editor_scene.js)、[MAP_CHIP_SPEC.md](MAP_CHIP_SPEC.md) |
| タイルセット一覧 | マップのタイル選択が `assets.tilesets` を全件表示 | [map_tileset_window.js](../www/js/ui/map_tileset_window.js) L26 |
| 左右反転 | `PixelData.flipH()` | [pixel_data.js](../www/js/pixel_data.js) L73 |
| プレイヤー移動 | `_applyControllerMovement` が入力から速度を計算し、`transform.data.x/y` を**playUnit データに直接書く** | [play_test_scene.js](../www/js/scene/play_test_scene.js) L257-292 |
| 実行時の状態変更 | `setProperty`、`playTween` も `component.data[...]` を**playUnit データに直接書く** | 同 L1497, L1518, L1537 |
| ランタイム構築 | `PlayUnitRuntime.fromPlayUnit(playUnit)` を毎フレーム呼び、playUnit データから作り直す | 同 L212、[play_unit_runtime.js](../www/js/play_unit/play_unit_runtime.js) L36 |
| 画像描画 | Image は全体描画のみ。毎フレーム全画素を `putImageData` する | 同（`_getPixelDocumentCanvas`） |

---

## 3. データ設計

### 3.1 向きとフレームの規約

- シートの**列**がフレーム、**行**が向き。固定サイズで、列数 3、行数 4。
- 列（フレーム）: `0 = 立ち`, `1 = 歩きA`, `2 = 歩きB`
- 行（向き）: `0 = 下`, `1 = 左`, `2 = 右`, `3 = 上`

定数として 1 か所に定義する（例: `www/js/character/character_chipset.js`）。

```js
CHARACTER_DIRECTIONS = ['down', 'left', 'right', 'up'];
CHARACTER_FRAME_COLUMNS = 3;   // 0: 立ち, 1: 歩きA, 2: 歩きB
```

### 3.2 データ型

既存の `TilesetData` を格納コンテナとして内包し、役割情報を持つ薄いラッパーを作る。

```js
// 構造のイメージ（実装時に確定）
{
  id: 'cc_xxx',                 // 接頭辞 'cc'
  type: 'characterChipset',
  name: 'Hero',
  chipWidth: 16,                // 既定 16（マップチップと同じ）
  chipHeight: 16,
  layout: {
    directions: ['down', 'left', 'right', 'up'],
    framesPerDirection: 3,
  },
  tilesetData: TilesetData,     // columns = 3, rows = 4 の固定
}
```

ラッパーの主な API:

- `getFrameLayerData(direction, frameIndex)`: 向きの行と、フレームの列から `LayerData` を返す（内部で `getChipLayerData(col, row)`）
- `mirrorDirection(fromDir, toDir)`: 行の3フレームを `flipH` でコピーする（左の作成補助に使う）

### 3.3 歩行サイクル

歩行はフレーム列の並びで表現する。

- 循環順: `[0, 1, 0, 2]`（立ち → 歩きA → 立ち → 歩きB）
- 停止中はフレーム `0`（立ち）に固定する

---

## 4. 保存形式

### 4.1 .qsproj

`assets.characterChipsets` を新設する。

```json
"characterChipsets": [
  {
    "id": "cc_xxx",
    "type": "characterChipset",
    "name": "Hero",
    "chipWidth": 16,
    "chipHeight": 16,
    "layout": { "directions": ["down","left","right","up"], "framesPerDirection": 3 },
    "storage": { "type": "embedded", "codec": "qts-base64", "data": "..." }
  }
]
```

- `storage` は既存タイルセットと同じ QTS 埋め込みを使う（`_serializeTileset` と同じ方式を流用）。
- `layout` は .qsproj 側のメタデータとして持つ。QTS 本体には入れない。

### 4.2 ブラウザ保存（IndexedDB）

- アセット種別 `characterChipset`、codec は既存の `qts-arraybuffer` を流用する。

### 4.3 外部書き出し

- PNG: 3列×4行のシートを書き出す（既存のタイルセット PNG 出力を流用）。16×16 の場合は 48×64 px になる。
- JSON と QTS 単体の出力は MVP では含めない。

### 4.4 QTS の制約（キャラクター用途での注意）

[BINARY_FORMAT_PLAN.md](BINARY_FORMAT_PLAN.md) と既存実装の調査による。

- 色は量子化される（パレット 31 色まで）
- 読み込み時に alpha が失われ、常に不透明になる
- `passFlags` はチップ内の全ピクセルに複製される。キャラクターには意味がないため、チップ単位の通過フラグ UI は出さない
- `locked` と `activeIndex` は保存されない

半透明のふちを使う絵柄は QTS では欠けるため、MVP では対象外とする（推奨どおり）。

---

## 5. エディタ（EditorScene への同居）

新規シーンは作らず、EditorScene に `characterChipset` の編集モードを追加する。既存の tileset モードと同じ編集経路を使い、差分だけを分岐させる。

### 5.1 入口と保存

- **新規作成**: EditorScene の File メニューに「新規キャラクターチップ」を追加する（既存の「新規タイルセット」と並べる）。ダイアログは既存の NewTilesetDialog を流用し、列・行を 3・4 に固定して表示する。チップサイズのプリセットは 8/16/32/48/64、既定 16。
- **開く**: `ProjectTopScene._openAsset` と `_restoreActiveProjectAsset` に `characterChipset` の分岐を追加し、`editMode = 'characterChipset'` で EditorScene に入る（tileset と同じ経路）。
- **保存**: `AppData.saveActiveProjectAssetState` に分岐を追加し、`characterChipset` の場合は格納先のコンテナに書き戻す。

### 5.2 既存の編集経路を流用する部分

- チップ選択と編集対象（`AppData.layerData` の getter が選択チップの LayerData を返す仕組み）
- 描画ツール、クリップボード（copy / paste / clear / swap）
- 履歴（描画は既存の HistoryManager 経由。チップ操作は Undo なし＝既存と同じ）

### 5.3 差分として分岐させる部分

`editMode` の判定を小さな関数に閉じ、次だけ変える。

| 項目 | tileset モード | characterChipset モード |
|---|---|---|
| グリッド | 自由な cols × rows | 3 × 4 固定。列見出し「立ち / 歩きA / 歩きB」、行見出し「下 / 左 / 右 / 上」を重ねる |
| 行・列の追加・削除 | 可能 | 不可（メニューから外す） |
| 通過フラグ | 編集・表示可能 | 不可（トグルと表示を無効化） |
| メニュー | 既存 | 「右向きから左を作成」「プレビュー表示」を追加 |

### 5.4 反転コピーの補助

- 「右向きから左を作成」: 行 `right` の3フレームを `flipH` で行 `left` にコピーする
- 既存の左を上書きするので、実行前に確認ダイアログを出す
- コピー後の個別修正は可能にする

### 5.5 プレビュー

- 新規の UIWindow（例: `www/js/ui/character_preview_window.js`）を、メニューのトグルで開閉する
- 中身: 向きの切替ボタン（4方向）、再生と停止（循環順 `[0,1,0,2]`、既定 6fps）
- 描画は、フレームの LayerData を既存のピクセル描画経路で描く

---

## 6. ランタイム

### 6.1 コンポーネント

PlayObject に新しいコンポーネント `Character` を追加する。

```js
{
  type: 'Character',
  enabled: true,
  data: {
    characterChipsetId: 'cc_xxx',
    scale: 1,
    origin: { x: 0.5, y: 1.0 },  // 足元中央を基準にする
  },
}
```

- プレイヤーは、`Controller`（`inputMode: 'player1'`）と `Character` を同じ PlayObject に持たせる
- 向きとフレームは `Character` の data に書かない（実行時の状態）

### 6.2 PlayerController（新規クラス）

配置: `www/js/play_unit/player_controller.js`

- 役割: 入力から移動量を計算し、向き・歩行フレームの状態を保持する
- 状態: `direction`、`frameIndex`（歩行サイクル内の位置）、`walkTimer`、`moving`
- API（案）:
  - `update(dt, inputVector, speed)` → 移動量を返す。内部で向きと歩行フレームを更新する
  - `getFrameLayerData(chipset)` → 現在の向きとフレームの `LayerData` を返す

向きの決め方:

- 移動していないときは、直前の向きを保つ
- 斜め移動では、直近に押したキーの方向を優先する

### 6.3 PlayTestScene との連携

#### 現状の問題

PlayTest の書き込みは、すべて playUnit データ（アセット実体）に対して行われている。

- `_applyControllerMovement` が `transform.data.x/y` を書く（[play_test_scene.js](../www/js/scene/play_test_scene.js) L289-290）
- `setProperty` が `component.data[property]` を書く（同 L1497）
- `playTween` が `component.data[property]` を書く（同 L1518, L1537）
- ランタイムは毎フレーム playUnit データから作り直すため（同 L212）、書いた値はそのまま次のフレームに反映される

結果として、PlayTest で動かした座標がアセットに残る。保存すると移動後の位置が保存される。

#### 採用案（提案）: セッション内の実行用コピー

1. PlayTestScene の `onEnter` で、実行対象の playUnit のディープコピーを作り、`this._runtimeUnits`（`Map<unitId, copy>`）に入れる。
2. 以降の書き込みはすべてコピーに対して行う。対象は `_applyControllerMovement`、`setProperty`、`playTween` の 3 箇所（上記の行番号）。`PlayUnitRuntime.fromPlayUnit` にもコピーを渡す。
3. `requestPlayUnit` で別の PlayUnit に切り替えたときは、その unit のコピーを作る。一度作ったコピーは PlayTest のセッション中は再利用する。これは現行と同じく、同じセッション内で戻ると前回の状態が残る挙動になる。
4. `onLeave` で Map を破棄する。アセットは PlayTest によって変更されない。

#### この案の利点

- 書き込み箇所の対象を差し替えるだけで、ランタイムの仕組みはほぼ変わらない
- Controller だけでなく、イベントによる `setProperty` や `playTween` も同じ仕組みで扱える。Controller だけを分けると、両者の状態がずれる
- PlayerController の状態（向き、フレーム）は、objectId をキーにした別の Map で持つ

#### 挙動の変化（要確認）

- PlayTest を終了すると、PlayUnit エディタで見える座標はプレイ前の値のままになる
- PlayTest の結果を保存しても、移動後の位置は残らない

この変化は H で決定済みです（ユーザー確認済み）。

### 6.4 描画

- `Character` の描画は、チップ1枚を `drawImage` で描く新しい経路にする
- チップごとに `LayerData` を合成した canvas をキャッシュし、フレームごとに `putImageData` しない
- 描画サイズは `chipWidth × scale`、`chipHeight × scale`。原点は `origin` で決める（既定は足元中央）

---

## 7. 実装フェーズ

| フェーズ | 内容 | 完了条件 |
|---|---|---|
| 1. データ層 | 型ラッパー、ProjectData の追加・正規化・参照解決、直列化（.qsproj）、ブラウザ保存、Save As の振り直し | 作成したチップセットが保存と読込の往復で残る |
| 2. エディタ同居 | EditorScene に editMode を追加、File メニューの新規作成、グリッド見出し、反転補助、プレビュー窓 | 12 フレームを描いて保存し、再読込できる |
| 3. ランタイム | Character コンポーネント、PlayerController、チップ描画経路とキャッシュ、PlayTest の実行用コピー（採用の場合） | プレイヤーが4方向に移動し、歩行アニメと立ち復帰が動く。アセットの座標が変わらない |
| 4. 検証 | [節 9](#9-検証計画) の項目を実施 | 検証項目がすべて通る |

各フェーズは、前のフェーズの完了後に進める。

---

## 8. 既存機能への影響

- **ProjectData**: `characterChipsets` 配列の追加、`addCharacterChipset`、正規化、`findCharacterChipsetById`、`getAssetByRef` の分岐。
- **ProjectSerializer**: 直列化と復元の追加。アセット型判定にも分岐を足す。
- **ブラウザ保存**: `_collectProjectAssets`、保存、読込（type による振り分け）、`_createSaveAsCopy` への振り直し行の追加。
- **Save As の参照**: `_reassignAssetIds` は、他アセットからの参照（例: Character の `characterChipsetId`）を更新しない。キャラクター参照を持つ PlayUnit が Save As 後に壊れるため、MVP では Save As 時に `characterChipsetId` を新しい ID に書き換える処理を追加する。
- **EditorScene**: 編集モードの分岐が増える（現在 1722 行）。分岐は editMode 判定の関数に集め、既存の tileset 処理には触れない。
- **ProjectTopScene**: 資産の件数表示（`characterChipsets` の件数を含める）と、開く処理の分岐。
- **PlayTestScene**: 節 6.3 の書き込み先の変更（採用の場合）。
- **マップのタイル選択**: `assets.tilesets` とは別配列にするため、影響はない。

---

## 9. 検証計画

- **保存の往復**: 新規作成 → 12フレーム描画 → ブラウザ保存 → 再読込 → 画素が一致する（QTS の量子化を考慮し、パレット内の色で比較する）
- **.qsproj の往復**: 書き出し → 読込 → 同じ内容になる
- **Save As**: 作成したキャラクターを参照する PlayUnit が、複製後も正しいチップセットを指す
- **反転補助**: 右向きの3フレームが、左向きに左右反転されている
- **PlayTest**
  - 4方向に移動でき、向きが切り替わる
  - 移動中は `[0,1,0,2]` の順でフレームが進み、停止すると立ちに戻る
  - 斜め移動で向きが期待どおりになる
  - 実行後、playUnit データの座標と `setProperty` / `playTween` の値が変わっていない（採用の場合）
- **画面確認**: Playwright でエディタとプレイテストのスクリーンショットを撮り、重なりや欠けがないことを確認する

---

## 10. 既知のリスクと注意点

- **QTS の非可逆性**: 半透明や細かな色の変化は保存時に欠ける。キャラクターの絵柄によっては JSON や PNG を正とする運用が必要になる。
- **毎フレームの画素転送**: 既存の `_getPixelDocumentCanvas` は毎フレーム全画素を書き込むため、キャラクターを複数体置くと負荷が上がる。チップ単位のキャッシュは必須。
- **Undo なし**: copy / paste / clear が履歴に載らない（MVP では対象外）。
- **資産の書き換え**: 既存の PlayTest は playUnit データに書き込む（[節 6.3](#63-playtestscene-との連携)）。H を採らない場合、プレイ後の保存で移動後の位置が残る。

---

## 11. 未決定事項

| 記号 | 項目 | 決定 |
|---|---|---|
| A | 向きの行順 | **決定: 下・左・右・上** |
| B | チップサイズの既定値 | **決定: 16×16**（マップチップと同じ） |
| C | 歩行サイクルの順序 | **決定: `[0,1,0,2]`** |
| D | 半透明の扱い（QTS の alpha 喪失） | **決定: MVP では対象外** |
| E | エディタの描画経路 | **決定: EditorScene に同居させ、既存の描画を流用する**（新規シーンは作らない） |
| F | Undo の対応 | **決定: MVP では対象外** |
| G | 斜め移動時の向き | **決定: 直近に押したキーの方向を優先** |
| H | プレイヤーの位置の扱い | **決定: セッション内の実行用コピーを使う**（[節 6.3](#63-playtestscene-との連携)）。PlayTest の結果はアセットに残らない |

---

## 12. 変更履歴

- 2026-10-12: 初版（設計案）
- 2026-10-12: v2。A〜G の決定を反映（B は 16×16、E は EditorScene 同居に変更）。H の具体案を節 6.3 に追加。
- 2026-10-12: 実装完了（フェーズ 1〜3）。節 13 に実装差分を追記。

---

## 13. 実装時の差分

設計からの変更点と、実装で判明した事項。

| 項目 | 設計 | 実装 |
|---|---|---|
| 編集モード | `editMode='characterChipset'` | `editMode` は `'tileset'` のまま。種別は `AppData.tilesetKind`（`'standard'` / `'characterChipset'`）。`layerData` getter や各コマンドが無改修で動くため |
| 種別の設定 | 各箇所で `editMode` を代入 | `EditorScene._enterTilesetMode(kind)` に集約。取り込み・新規作成・復元の全経路がこれを使う |
| 保存 | 種別ごとに分岐 | `saveActiveProjectAssetState` は `tileset` と `characterChipset` を同じ分岐で書き戻す（格納内容が同じため） |
| 実行用コピー | 切替と戻りの経路を個別に差し替え | `PlayTestScene._getActiveRuntimePlayUnit()` が `getActiveProjectAsset()` からコピーを引く。切替・戻りはアセットの取得結果で自動追従するため、経路ごとの変更は不要 |
| コピーの作り方 | `PlayUnitData.from` + `data` の深いコピー | 同じ（`ProjectData._cloneJsonLikeValue`）。JSON の往復は使わない（クラスのメソッドが消えるため） |
| 歩行の状態 | PlayTest の Map | `PlayerController`（`play_unit/player_controller.js`）を `${unitId}/${objectId}` で保持 |
| 通常タイルセットの表示 | — | 種別が `characterChipset` のときだけ、通過フラグの ✕ と「タイルプレビュー」窓を抑制する（`standard` は従来どおり） |
| 反転コピー | — | `CharacterChipset.mirrorDirection`。履歴には載せない（設計 F） |
| 新規作成ダイアログ | 既存の NewTilesetDialog を流用 | `NewCharacterChipsetDialog` を新設（チップサイズの選択と固定レイアウトの表示だけ） |
| 既定サイズ | 16×16 | 同じ（ダイアログの既定も 16） |

### 確認できた既存の制約

- `requestedPlayUnitId` などの予約変数は、変数エディタで定義されていないと設定できない（`setRuntimeGlobalVariable` が失敗する）。PlayUnit の切替を使うプロジェクトでは、事前に定義が必要。
- QTS の色量子化により、保存後の色は元の色と一致しない（例: `ff111111` → `ff202020`）。画素の比較はパレット内の色で行うこと。
- `Collider` は Transform を左上基準とする矩形のまま。`origin` を足元中央（既定）にすると、見た目と当たり判定がずれる（Collider の offset 対応は別タスク）。
