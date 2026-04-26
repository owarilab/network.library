# マップエディタシーン実装計画書

最終更新: 2026-04-26 (viewport minimum implementation updated)

## 目的

`ProjectTopScene` から入れる `MapEditorScene` を追加し、
既存のドット絵エディタと同じく `UIWindow` ベースの複数パネル構成で
マップ編集機能を段階的に拡張できる土台を作る。

今回のゴールは「完成版マップエディタ」ではなく、次を満たす雛形である。
以下のゴールは 2026-04-26 時点で実装済み。

- プロジェクトからマップアセットを開ける
- マップ用データを `ProjectData.assets.maps` に保存できる
- マップ表示領域、タイル選択領域、インスペクタをウィンドウとして分離する
- 既存の `tileset` アセットを流用してセルへ配置できる

---

## 実装状況サマリ

### 実装済み

- `ProjectTopScene` から `MapEditorScene` を開く導線
- map asset が無い場合の自動生成
- `AppData` 経由の `mapData` 保持と project 保存
- `MapViewWindow` / `MapTilesetWindow` / `MapInspectorWindow` の 3 ウィンドウ構成
- tileset からタイル選択 → 左クリック配置 / 右クリッククリア
- `G` キーと inspector ボタンからのグリッド表示切り替え
- `MapResizeDialog` による左上固定のマップサイズ変更
- viewport ベースの部分描画
- wheel による zoom、`Space + drag` / 中ボタンドラッグによる pan
- inspector からの zoom 状態表示と `Reset View`
- 最小メニュー UI (`MapEditorMenuBar`)
  - `保存`
  - `マップサイズ変更...`
  - `プロジェクトトップへ戻る`
  - `グリッド表示`

### 未実装

- レイヤーパネル追加
- スクロール / 部分表示 / ズームの仕上げ調整
- 複数タイルセット参照の整理
- Undo / Redo の map 編集対応
- オートタイル / collision layer などの拡張

---

## 既存実装から流用する点

### 1. シーン導線

- `TitleScene` → `ProjectTopScene` → 各エディタ、という遷移はすでに確立済み
- `ProjectTopScene` はアセット選択と scene 切り替えハブとして使う
- `ProjectSession.activeDocumentRef` を更新してから scene を切り替える流れを踏襲する

### 2. ウィンドウ UI 基盤

- `UIWindow` のドラッグ可能ウィンドウ基底をそのまま使う
- ドット絵エディタの `ToolBarWindow` / `LayerPanelWindow` / `ChipPaletteWindow` と同じく、
  コンテンツ描画とイベント処理をウィンドウ単位に分割する

### 3. タイル表示ロジック

- `TilesetData.compositeChip()` を使い、既存タイルセットのチップ画像をそのまま表示する
- `ChipPalette` / `TilePreview` が持つ offscreen canvas + nearest-neighbor 描画パターンを再利用する

### 4. 保存フロー

- `AppData.saveActiveProjectAssetState()` に `map` を追加し、
  `ProjectTopScene` からの browser 保存 / `.qsproj` export で同一経路を通す
- `ProjectSerializer` / `ProjectBrowserStorage` はすでに `mapData` JSON を扱えるため、
  追加で壊さずに接続する

---

## 今回の雛形の構成

`MapEditorScene` は次の 3 ウィンドウで始める。

1. `MapViewWindow`
   - マップ全体をグリッド表示する主編集領域
   - 左クリックで選択タイルを配置
   - 右クリックでセルをクリア

2. `MapTilesetWindow`
   - プロジェクト内タイルセットの切り替え
   - チップ選択
   - 将来のタイルカテゴリ分割や検索の受け皿

3. `MapInspectorWindow`
   - マップサイズ、タイルサイズ、カーソル位置、選択タイル表示
   - `Toggle Grid` / `Back` の最小操作

---

## マップデータの最小形

`ProjectData.assets.maps[].mapData` は当面、プレーンな JSON 構造に留める。

```js
{
  version: 1,
  width: 24,
  height: 18,
  tileWidth: 16,
  tileHeight: 16,
  tilesetId: 'ts_xxx' | null,
  selectedLayer: 0,
  selectedTileRef: {
    tilesetId: 'ts_xxx',
    col: 0,
    row: 0,
    index: 0,
  } | null,
  cursor: { x: 0, y: 0 },
  view: {
    showGrid: true,
    zoom: 2,
  },
  layers: [{
    id: 'layer_ground',
    name: 'Ground',
    visible: true,
    locked: false,
    tiles: [number, ...],
  }],
}
```

補足:

- `tiles` は一次元配列で、未配置は `-1`
- 初期段階は 1 レイヤー固定で十分
- 将来の複数レイヤー化でも `layers[]` 形式なら拡張しやすい
- scroll / zoom 対応フェーズでは `view` に `panX / panY / minZoom / maxZoom` を追加する

---

## 実装ステップ

### Phase 1: 導線と保存

状況: 実装済み

- `ProjectTopScene` の `Map Editor` ボタンを有効化
- map asset クリック時は `MapEditorScene` を開く
- アクティブ map が無ければデフォルトの map asset を自動生成する
- `AppData.saveActiveProjectAssetState()` で `map` を保存対象に含める

### Phase 2: UIWindow ベースの雛形

状況: 実装済み

- `MapEditorScene` を追加
- `MapViewWindow` / `MapTilesetWindow` / `MapInspectorWindow` を追加
- scene 側は入力の中継と状態変更だけを担当し、描画詳細は各 window に寄せる

### Phase 3: 最小編集

状況: 実装済み

- tileset から paint tile を選択
- map view 上で左クリック配置、右クリッククリア
- `G` キーとボタンでグリッド表示を切り替える

### Phase 4: マップサイズ変更

状況: 最小実装済み

- `MapEditorMenuBar` に `ファイル > マップサイズ変更...` を追加する
- メニュー項目から `MapResizeDialog` を開く流れを固定する
- ダイアログ確定後に `mapData.width / height / layers[].tiles / cursor` を一括更新する
- 既存タイルの保持方針として、拡大時は空セル `-1`、縮小時は範囲外セルを切り捨てる

### Phase 5: マップスクロール / 部分表示 / ズーム

状況: 最小実装済み

- `MapViewWindow` を「マップ全体 fit 描画」から「viewport 描画」へ切り替える
- `mapData.view.zoom` を実倍率として扱い、表示位置を保持する `panX / panY` を追加する
- 描画対象を全セルではなく可視セル範囲のみに絞る
- マウスホイールによるズーム、`Space + left drag` / `middle drag` によるパンを追加する
- `MapInspectorWindow` に現在倍率 / pan 状態表示と `Reset View` を追加する
- visible range と pan clamp を調整し、端の列欠けや空白が出にくいように補正する

---

## マップサイズ変更の拡張計画

### 目的

- 既存の map asset を作り直さずにセル数を変更できるようにする
- 初期段階では「矩形グリッドの拡大・縮小」に絞り、複雑な再配置は避ける

### UI 方針

- `DialogBase` を継承した `MapResizeDialog` を追加する
- 入力項目は最小構成で次を持つ
  - 幅 (cells)
  - 高さ (cells)
  - 基準位置 (`左上固定` のみで開始)
  - 適用 / キャンセル
- まずは `NewFileDialog` と同じく数値入力 + ボタンの構成を流用する
- 2026-04-26 時点ではこの最小 UI を実装済み

### データ更新方針

- 対象は `appData.mapData` のみとし、保存は既存の `saveActiveProjectAssetState()` に委譲する
- 各 layer の `tiles` は新しい `width * height` 長の配列を作る
- 旧データは左上原点でコピーする
- 拡大で増えた領域は `-1` を埋める
- 縮小で収まらない領域は破棄する
- `cursor` は新しい範囲内に clamp する
- `selectedLayer` はそのまま維持する
- 2026-04-26 時点では左上固定コピーのみ実装済み

### 実装ステップ案

1. `MenuConstants.FILE_MAP_RESIZE` を起点にメニュー選択を受ける
2. `MapResizeDialog` を追加し、`MapEditorScene` から表示できるようにする
3. `MapEditorScene` に `resizeMap(width, height)` を追加する
4. `resizeMap()` 内で layer 配列の再構築と cursor clamp を行う
5. 適用後は `projectSession.markDirty()` と status 更新を行う

上記 1-5 は 2026-04-26 時点で実装済み。

### 今回は見送るもの

- 基準位置の複数指定 (`中央`, `右下` など)
- タイルの自動再配置
- Undo / Redo との連携
- マップサイズ変更のプレビュー表示

---

## マップスクロール / 部分表示 / ズームの拡張計画

### 現在の実装範囲

- viewport ベースでの部分描画
- `mapData.view.zoom / panX / panY` の最小保持
- wheel zoom
- `Space + left drag` または `middle drag` での pan
- inspector での zoom / pan 表示と `Reset View`
- visible range の overscan と pan clamp による境界補正

### 今後の目的

- 大きいマップでも編集対象周辺を拡大表示しながら操作できるようにする
- 現在の `MapViewWindow` が持つ「常に全体を fit して描画する」前提を外し、将来の大規模マップ対応の土台を作る
- 描画負荷を可視範囲ベースへ寄せ、マップサイズ増加時のコストを抑える

### 基本方針

- 既存の [pixel_canvas.js](../../www/js/pixel_canvas.js) が持つ `zoom` / `pan` / `screenToPixel` の考え方を参照する
- ただしマップ編集は `PixelCanvas` を直接流用せず、`MapViewWindow` に viewport 責務を持たせる
- 画面座標とセル座標の相互変換を `MapViewWindow` 側へ集約し、scene は入力の中継と状態更新に留める
- 現在は wheel + drag の最小操作まで実装済みで、minimap やスクロールバーは引き続き見送る

### view モデル拡張案

`mapData.view` は Phase 5 で次の項目を追加する。

```js
view: {
  showGrid: true,
  zoom: 2,
  panX: 0,
  panY: 0,
  minZoom: 1,
  maxZoom: 8,
}
```

補足:

- `panX / panY` はセル単位ではなく screen px 基準で保持する
- ズーム中心をマウス位置へ寄せるため、screen 座標系での保持を前提にする
- 既存 project との互換では `panX / panY` 未定義時に `0` 扱いとする

### 描画・座標変換方針

- 現在の `_getGridRect()` 依存を縮小し、viewport 用の `screenToCell()` / `getVisibleCellRange()` を新設する
- visible range は `startCol / endCol / startRow / endRow` を viewport から算出し、`0..width-1`, `0..height-1` に clamp する
- タイル描画、hover 表示、cursor 表示、クリック判定のすべてを同じ viewport 変換に統一する
- マップが window より小さい場合は中央寄せを維持し、拡大時のみ pan の影響が見えるようにする

### 入力方針

- `wheel`: マウス位置基準で zoom in / out
- `Space + left drag` または `middle drag`: pan 開始
- `left click`: 通常のタイル配置
- `right click`: 通常のセルクリア
- `Escape` やメニュー操作中は pan / paint を抑止し、dialog > menu > viewport の優先順を維持する

### 残作業

1. view メニューへ `拡大` / `縮小` / `ビューをリセット` を追加する
2. hover / 配置セルのズレや端境界の挙動を実機確認ベースで仕上げる
3. open 時の viewport 初期化方針に合わせて view 状態を明示的に整理する
4. 必要であれば minimap やスクロールバーを追加する

### 検証観点

- 小さいマップでは従来どおり編集しやすいこと
- 大きいマップでは全体を常時縮小せず、局所拡大して編集できること
- zoom 後も hover と配置セルがずれないこと
- pan 中に `UIWindow` 自体のドラッグと競合しないこと
- map resize 後も viewport が不正な範囲へ飛ばないこと

---

## 今後の拡張候補

以下は引き続き未実装。

1. レイヤーパネル追加
2. マップスクロール / 部分表示 / ズームの仕上げ調整
3. 複数タイルセット参照の整理
4. Undo / Redo を map 編集にも拡張
5. オートタイルや collision layer の導入

---

## 注意点

- 既存の `EditorScene` は `PixelData` / `LayerData` の編集に特化しているため、
  マップ編集を直接混ぜず別 scene に分離する
- `mapData` は plain object のまま保存できるように保ち、
  先に serializer を複雑化させない
- まずは「開ける・置ける・保存できる」を固め、その後に window を増やす
- 現状はこの最小ゴールまでは到達しており、ここから先は拡張フェーズとして扱う