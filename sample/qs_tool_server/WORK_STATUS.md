# ドットエディタ 作業状況

最終更新: 2026-04-23 (11)

## 概要

`sample/qs_tool_server/www/` 以下に HTML5 Canvas ベースのドット絵エディタを実装中。
外部ライブラリなし（Vanilla JS）。ゲームエンジン風の Scene/SceneManager パターンを採用。

サーバーは `sample/http_server_simple` の C製HTTPサーバーを流用または
`qs_tool_server` のサーバー実装を使う。URL: `http://localhost:4444/`

---

## ファイル構成

```
www/
├── index.html                      # エントリーポイント・スクリプト読み込み順管理
├── css/
│   └── style.css                   # body/canvas 全画面スタイル
└── js/
    ├── pixel_data.js               # PixelData: Uint32Array ベースのピクセルデータ管理
    ├── layer_data.js               # LayerData: 複数 PixelData レイヤー管理・アルファ合成
    ├── app_data.js                 # AppData: シーン間共有データコンテナ (LayerData 統合済)
    ├── tileset_data.js             # TilesetData: チップ単位 LayerData 管理・通過フラグ
    ├── input.js                    # Input: キーボード/マウス/タッチイベント一元管理
    ├── pixel_canvas.js             # PixelCanvas: PixelData描画・ズーム・パン
    ├── canvas_manager.js           # CanvasManager: RAFループ・各マネージャ生成
    ├── scene_manager.js            # SceneManager: アクティブシーン管理・切り替え
    ├── scene/
    │   ├── scene.js                # Scene: 基底クラス
    │   └── editor_scene.js         # EditorScene: メインシーン（描画・メニュー操作）
    ├── ui/
        ├── menu_constants.js       # MenuConstants: メニューID定数
        ├── menu_bar.js             # MenuBar: メニューバー描画
        ├── dropdown_menu.js        # DropdownMenu: プルダウンリスト
        ├── color_palette.js        # ColorPalette: カラーパレット基底クラス (コンテンツのみ)
        ├── color_palette_16.js     # ColorPalette16: 固定16色パレット
        ├── editable_palette_32.js  # EditablePalette32: 編集可能32色パレット
        ├── color_palette_window.js # ColorPaletteWindow: UIWindow継承のパレットウィンドウ
        ├── chip_palette.js         # ChipPalette: タイルチップ一覧コンテンツ
        ├── chip_palette_window.js  # ChipPaletteWindow: UIWindow継承のチップパレット
        ├── tile_preview.js         # TilePreview: 選択チップのタイリング表示
        ├── tile_preview_window.js  # TilePreviewWindow: UIWindow継承のタイルプレビュー
        ├── tool_bar.js             # ToolBar: ツールボタンコンテンツクラス
        ├── tool_bar_window.js      # ToolBarWindow: UIWindow継承のツールバーウィンドウ
        ├── layer_panel.js          # LayerPanel: レイヤー一覧コンテンツクラス
        ├── layer_panel_window.js   # LayerPanelWindow: UIWindow継承のレイヤーパネル
        ├── ui_window.js            # UIWindow: ドラッグ可能ウィンドウ共通基底クラス
        └── dialog/
            ├── dialog_base.js      # DialogBase: モーダルダイアログ基底クラス
            ├── new_file_dialog.js   # NewFileDialog: 新規作成ダイアログ
            ├── new_tileset_dialog.js # NewTilesetDialog: タイルセット新規作成
            ├── import_tileset_dialog.js # ImportTilesetDialog: 画像→タイルセット変換設定
            ├── save_dialog.js       # SaveDialog: エクスポートダイアログ
            ├── color_picker_dialog.js # ColorPickerDialog: HSVカラーピッカーダイアログ
            └── quarter_view_tile_dialog.js # QuarterViewTileDialog: 斜め見下ろし生成
    └── util/
        ├── pixel_data_converter.js # PNG/JSON/QTS 変換
        └── quarter_view_tile_generator.js # 斜め見下ろしタイル生成
```

---

## 実装済み機能

### 基盤

- [x] `CanvasManager`: 全画面 canvas、RAF 60fps ループ（ドリフト補正付き）、resize 対応
- [x] `Scene` / `SceneManager`: シーン切り替え時ライフサイクル（onEnter/onLeave）、`input.clearAll()`
- [x] `Input`: keyboard/mouse/touch イベント一元管理、canvas 相対座標正規化、wheel/contextmenu 既定動作抑止
- [x] `AppData`: `LayerData` インスタンスを保持、`pixelData` getter でアクティブレイヤー返却、`createPixelData()` デリゲート

### データ層

- [x] `PixelData`:
  - `createPixelData(width, height, fillColor)` ファクトリメソッド（コンストラクタ初期化なし）
  - `getPixel(x, y)` / `setPixel(x, y, color)` / `fill(color)`
  - `static rgba(r, g, b, a)` / `static unpack(color)` / `static toCssColor(color)`
  - ピクセルフォーマット: `0xAARRGGBB`（`Uint32Array`）

### 描画

- [x] `PixelCanvas`:
  - `ImageData` でオフスクリーン canvas に書き込み → `drawImage` 拡大（`imageSmoothingEnabled=false`）
  - 透明ピクセル用市松模様背景（`CHECKER_LIGHT/DARK`、8px マス）
  - グリッド線（scale >= 4 で表示）
  - 枠線
  - 選択範囲オーバーレイ表示（半透明フィル + 破線枠）
  - 浮動選択プレビュー表示
  - dirty フラグで無駄なオフスクリーン再生成を抑止
  - `screenToPixel(sx, sy, pixelData)` 座標変換（範囲外 null）

### ズーム

- [x] ホイールでズーム（`EditorScene._onWheel` → `pixelCanvas.zoom()`）
- [x] ステップ配列 `[1,2,3,4,5,6,7,8,10,12,14,16,20,24,28,32,40,48,56,64]` による細かい倍率変化

### パン（スクロール）

- [x] スペースキー + 左ドラッグでパン移動
- [x] スペースキー押下中カーソル `grab`、ドラッグ中 `grabbing`
- [x] `panX / panY` で中央配置オフセットに加算
- [x] `startPan / movePan / endPan / resetPan` API

### UI

- [x] `MenuBar`: ファイル・編集・表示メニュー
- [x] `DropdownMenu`: ホバーハイライト・セパレータ・`pick()` でID返却
- [x] `MenuConstants`: 全メニューID定数
  - FILE: `NEW / OPEN / SAVE / EXIT`
  - EDIT: `UNDO / REDO / CUT / COPY / PASTE / SELECT_ALL`
  - VIEW: `GRID / ZOOM_IN / ZOOM_OUT / ZOOM_RESET`
  - TILESET/GENERATE 系IDを追加（`FILE_NEW_TILESET` など）
  - `SEPARATOR`
- [x] `DialogBase`: モーダルオーバーレイ、`renderBody() / onMouseDownBody() / onKeyDownBody()` オーバーライドポイント
- [x] `NewFileDialog`:
  - サイズプリセット: 16 / 32 / 48 / 64 / 128
  - 幅・高さスピナー（キーボード入力対応）
  - 背景色ラジオ（透明 / 白）
  - `onConfirm(width, height, bgColor)` コールバック

---

## TODO リスト

### 優先度: 高（基本描画機能の完成）

#### ツール系

- [x] **ツールバー UI** (`js/ui/tool_bar.js` + `tool_bar_window.js`)
  - `UIWindow` 継承のドラッグ可能ウィンドウ
  - pencil / eraser / fill / eyedropper / selectRect の 5 ツールに対応
  - アイコン: canvas 2D で描画した幾何グラフィック
  - ホバー時「ツール名」ツールチップ表示
  - `AppData.activeTool` でアクティブツールを管理
- [x] **アクティブツール管理** (`AppData.activeTool`)
  - `EditorScene._applyTool()` でツールごとにディスパッチ
- [x] **ペンシルツール** — `foreColor`/`backColor` で左/右クリック描画
- [x] **消しゴムツール** — `0x00000000`（透明）で塗りつぶし
- [x] **塗りつぶしツール** — `EditorScene._floodFill()` BFS フラッドフィル実装
- [x] **スポイトツール** — クリックしたピクセルの色を `foreColor`/`backColor` に取得
- [ ] **ツールクラス切り出し** (`js/tool/tool_pencil.js` など)
  - 現在 `EditorScene._applyTool()` 内にインライン実装済み; 将来分離する予定

#### カラーパレット

- [x] `AppData.foreColor` / `AppData.backColor` プロパティ追加（前景色・背景色）
  - 初期値: 前景=黒 `0xFF000000`、背景=白 `0xFFFFFFFF`
- [x] **UIWindow** (`js/ui/ui_window.js`) — ドラッグ可能ウィンドウ共通基底クラス
  - タイトルバードラッグによる自由移動
  - `getWindowTitle()` / `getContentSize()` / `renderContent()` オーバーライドポイント
  - `onContentMouseMove/Down/Up()` コンテンツイベント委譲
  - キャンバス内クランプ（メニューバー下から画面内に留まる）
  - `onMouseDown(e, appData)` → `boolean` (ウィンドウ外は false で素通し)
- [x] **ColorPalette** (`js/ui/color_palette.js`) リファクタ
  - ウィンドウ枠・背景描画を分離、`render(ctx, x, y, appData)` 形式に変更
  - `getContentSize()` → `{w, h}` を UIWindowと共有
- [x] **ColorPalette16** (`js/ui/color_palette_16.js`)
  - 固定16色（グレースケール4・暖色4・緑青緑4・青紫4）
  - `ColorPalette` を継承し `getColors()` / `getPaletteName()` を実装
- [x] **ColorPaletteWindow** (`js/ui/color_palette_window.js`)
  - `UIWindow` 継承、任意の `ColorPalette` インスタンスをコンテンツとして包む
  - `setPalette(palette)` で実行時パレット切り替え可能
- [x] **カラーピッカーダイアログ** (`js/ui/dialog/color_picker_dialog.js`)
  - HSV カラーホイール (SV 領域 + Hue バー)
  - RGB / A スピナー入力 + Hex (#RRGGBB) 入力
  - 旧色 / 新色プレビュー表示
  - `DialogBase` を継承して実装
  - パレットの FG/BG スウォッチクリックでダイアログを開く

---

### 優先度: 中（快適な操作のため）

#### Undo / Redo

- [ ] **コマンドパターン** (`js/command/`)
  - `CommandBase`: `execute() / undo()` インターフェース
  - `DrawCommand`: 描画操作1ストローク分のピクセル変更を保持
  - `FillCommand`: 塗りつぶし操作
  - `NewFileCommand`: 新規作成操作
- [ ] **HistoryManager** (`js/history_manager.js`)
  - `push(command)` → `command.execute()` して履歴スタックに積む
  - `undo()` / `redo()` の実装
  - 最大履歴数の制限（例: 100ステップ）
- [ ] メニュー `EDIT_UNDO / EDIT_REDO` を `HistoryManager` に接続
- [ ] `Ctrl+Z` / `Ctrl+Y`（または `Ctrl+Shift+Z`）のキーボードショートカット

#### ズーム改善

- [ ] ズーム中心をマウスカーソル位置に固定（現在は画面中央固定）
  - 公式: `panX -= (centerX - canvas.width/2) * (newScale - oldScale) / oldScale` 系の計算
- [ ] メニュー `VIEW_ZOOM_IN / VIEW_ZOOM_OUT` を `pixelCanvas.zoom()` に接続
- [ ] メニュー `VIEW_ZOOM_RESET` を `pixelCanvas.resetPan()` + スケール初期化に接続
- [ ] ステータスバーに現在の倍率を表示

#### ファイル入出力

- [x] **PNG エクスポート / JSON エクスポート** (`FILE_SAVE`)
  - `PixelDataConverter` (`js/util/pixel_data_converter.js`): PNG Blob変換・ JSONエンコード・`_downloadBlob`
  - `SaveDialog` (`js/ui/dialog/save_dialog.js`): ファイル名入力 + PNG/JSON形式選択 + Enterサポート
  - `EditorScene`: `FILE_SAVE` メニュー接続・ダイアログ統合済
  - JSON形式: `{ version, width, height, format: "ARGB32", data: base64 }`
- [x] **PNG インポート / JSON インポート** (`FILE_OPEN`)
  - `PixelDataConverter.importFromFile(file)` — 拡張子で PNG/JSON を判別してルーティング
  - `importFromPng(file)` — `Image` → オフスクリーン canvas → `getImageData` → RGBA→0xAARRGGBB
  - `importFromJson(file)` — `FileReader.readAsText` → Base64 decode → `Uint8Array` → `PixelData`
  - `EditorScene`: 隠し `<input type="file">` を DOM に生成、`FILE_OPEN` メニューで `click()` 呼び出し
- [x] **タイルセット JSON v2 保存/読込**
  - `PixelDataConverter.toTilesetJsonString()` / `_parseTilesetJson()` 実装
  - 形式: `{ version: 2, type: "tileset", chipWidth, chipHeight, columns, rows, chips[] }`
- [x] **QTS バイナリ保存/読込** (`.qts`)
  - `exportTilesetAsQts()` / `importFromQts()` 実装
  - 1バイトピクセル（ColorID + passable）で量子化保存
- [ ] **通常ドット絵(JSON v1)のメタ情報拡張**

#### グリッド・表示設定

- [x] メニュー `VIEW_GRID` でグリッド線表示/非表示トグル
- [ ] グリッド色・透明度をカスタマイズする設定ダイアログ

#### 編集ショートカット

- [x] `Ctrl+C / Ctrl+X / Ctrl+V / Ctrl+A` を `EditorScene._onKeyDown` に接続
- [x] `Enter` で浮動選択を確定
- [x] `Escape` で選択解除 / 浮動選択キャンセル

---

### 優先度: 低（拡張機能）

#### 選択ツールとクリップボード

- [x] **矩形選択ツール**
  - `AppData.selection` / `selectionClipboard` / `hasSelection()` / `hasFloatingSelection()` を追加
  - `selectRect` ツールを `ToolBar` / `AppData.activeTool` / `EditorScene._applyTool()` に統合
  - 選択範囲をダッシュ枠 + 半透明塗りで描画
- [x] **EDIT_CUT / COPY / PASTE** の実装
  - コピー: 選択範囲を `selectionClipboard` へ格納
  - カット: 選択範囲を透明化し、浮動選択として保持
  - ペースト: 浮動選択として生成し、ドラッグ移動後に `Enter` で確定
- [x] **EDIT_SELECT_ALL** の実装
- [x] **通常選択のドラッグ移動開始**
  - 選択範囲内ドラッグで通常選択を浮動選択へ持ち上げ、そのまま移動可能
- [x] **浮動選択の変形**
  - `EDIT_FLIP_H / EDIT_FLIP_V / EDIT_ROTATE_CW / EDIT_ROTATE_CCW` を浮動選択に優先適用
- [ ] **選択ツールクラス切り出し** (`js/tool/tool_select_rect.js`)
- [ ] **浮動選択描画の最適化**
  - 現状はオーバーレイ canvas を都度生成しているため、必要ならキャッシュ化する

#### 反転

- [x] **左右に反転** (`EDIT_FLIP_H`)
  - `PixelData.flipH()`: 各行の左右ピクセルを swap して水平反転
  - `EditorScene` でメニュー `編集 > 左右に反転` と接続
  - 通常モード（`layerData.layers` 全レイヤー対象）・タイルセットモード（選択チップ対象）両対応
- [x] **上下に反転** (`EDIT_FLIP_V`)
  - `PixelData.flipV()`: 上下行を swap して垂直反転
  - `EditorScene` でメニュー `編集 > 上下に反転` と接続
  - 通常モード・タイルセットモード両対応

#### 図形ツール

- [ ] **直線ツール** (`js/tool/tool_line.js` / Bresenham アルゴリズム)
- [ ] **矩形ツール** (`js/tool/tool_rect.js`、塗りつぶし/枠線モード)
- [ ] **楕円ツール** (`js/tool/tool_ellipse.js`、Midpoint Circle/Ellipse アルゴリズム)

#### レイヤー

- [x] **`LayerData`** クラス (`js/layer_data.js`): 複数 `PixelData` レイヤー管理・アルファ合成
  - `init(w, h, fill)` で1レイヤー構成で初期化
  - `addLayer()` / `removeLayer()` / `moveLayer()` レイヤー操作
  - `toggleVisibility()` / `setOpacity()` 表示制御
  - `renameLayer()` / `setLocked()` / `toggleLocked()` / `canEditLayer()` を追加
  - `duplicateLayer()` / `mergeLayerDown()` を追加
  - `locked` フィールドをレイヤーエントリへ追加
  - `composite()` ボトムアップ Porter-Duff "source over" アルファ合成、キャッシュ付き
  - `markCompositeDirty()` で合成キャッシュ無効化
- [x] **`LayerPanel`** UI (`js/ui/layer_panel.js`): レイヤー一覧コンテンツ
  - 最前面が上の行表示、アクティブレイヤーハイライト
  - 目アイコンで visibility トグル
  - 鍵アイコンで lock/unlock 切り替え
  - レイヤー名のインライン編集
  - 下部ボタン: [＋追加] [−削除] [↑上へ] [↓下へ] [D複製] [M結合]
  - `onChange` コールバックで外部に変更通知
- [x] **`LayerPanelWindow`** (`js/ui/layer_panel_window.js`): UIWindow 継承
  - 初回描画時に画面右寄せ配置
- [x] **`AppData`** 改修: `layerData` プロパティ追加、`pixelData` を getter/setter 化（後方互換）
- [x] **`EditorScene`** 統合: 合成画像描画、スポイトは合成結果から色取得、PNGエクスポートは合成結果
  - ロック中レイヤーへの描画、消去、塗りつぶし、切り取り、浮動選択確定を禁止
  - 反転、回転はロックされていないレイヤーにのみ適用
- [x] レイヤー名変更（インライン編集経由）
- [x] レイヤーロック
- [x] レイヤー複製
- [x] レイヤー結合（下レイヤーへ結合）
- [x] `locked` の JSON 保存/読込と tileset clone/paste 系への伝播
- [ ] レイヤー不透明度スライダー UI
- [ ] 不透明度の視覚UI改善（数値表示 / スライダー）

#### アニメーション

- [ ] **フレームパネル** UI: フレームのサムネイル一覧
- [ ] **`AppData.frames: PixelData[]`** + `AppData.activeFrameIndex`
- [ ] プレビュー再生（指定FPS で frames を切り替えて表示）
- [ ] アニメーション GIF エクスポート（外部ライブラリまたは純粋実装）

#### UI 改善

- [ ] キャンバスサイズ変更ダイアログ（既存ピクセルを維持したままキャンバス拡縮）
- [ ] 設定ダイアログ（チェッカー色、FPS 上限、ショートカット一覧）
- [ ] ステータスバー（現在座標・倍率・ファイル名表示）
- [ ] タッチ操作: ピンチイン/アウトでズーム、2本指スワイプでパン
- [ ] **ツールキーボードショートカット** (`EditorScene._onKeyDown` に追加)
  - `P` → pencil / `E` → eraser / `G` → fill / `I` → eyedropper
  - ショートカットキー一覧を設定ダイアログに表示

---

## 2026-03-22 追記（コード反映済み）

### タイルセットモード

- [x] `AppData` に `editMode` / `tilesetData` / `selectedChip` / `chipClipboard` / `palette` / `showPassFlags` を追加
- [x] `TilesetData` を実装
  - チップ単位 `LayerData` 管理（`chips[row][col]`）
  - 通過フラグ配列 `passFlags[row][col]`
  - `clearChip / copyChip / swapChips / cloneChipLayerData / pasteChipLayerData`
  - `addRow / addColumn / removeRow / removeColumn / resize`
- [x] `EditorScene` にタイルセットメニュー処理を統合
  - 新規/読込: `FILE_NEW_TILESET` / `FILE_OPEN_TILESET`
  - 出力: `FILE_EXPORT_TILESET`(PNG) / `FILE_EXPORT_CHIP`(PNG)
  - チップ操作: `TILESET_COPY_CHIP / PASTE_CHIP / CLEAR_CHIP / SWAP_CHIP`
  - 構造変更: `TILESET_ADD_ROW/COL / REMOVE_ROW/COL`
  - 表示: `TILESET_TILE_PREVIEW` / `VIEW_PASS_FLAGS`
- [x] 編集メニュー拡張
  - `EDIT_ROTATE_CW` / `EDIT_ROTATE_CCW`（通常モード・タイルセットモード両対応）
  - `EDIT_FLIP_H` / `EDIT_FLIP_V` は両モード対応済み

### UI 拡張

- [x] `EditablePalette32`（32色編集パレット）
- [x] `ChipPalette` / `ChipPaletteWindow`（チップ一覧、選択・右クリック）
- [x] `TilePreview` / `TilePreviewWindow`（選択チップの 3x3 / 5x5 繰り返し表示）
- [x] `NewTilesetDialog` / `ImportTilesetDialog` / `QuarterViewTileDialog`
- [x] `Generate > Quarter View Tile` メニュー接続

---

## 2026-04-22 追記（選択ツール実装進捗）

### 選択ツール Phase 1-4

- [x] Phase 1: 矩形選択
  - 矩形ドラッグで選択範囲を作成
  - `Ctrl+A` / `EDIT_SELECT_ALL` で全選択
  - `Escape` で選択解除
- [x] Phase 2: コピー / 切り取り / 貼り付け
  - `Ctrl+C / Ctrl+X / Ctrl+V` と編集メニューに接続
  - 選択内容は `selectionClipboard` に `PixelData` として保持
- [x] Phase 3: 浮動選択移動
  - カット / 貼り付け後は浮動選択として表示
  - `selectRect` ツールでドラッグ移動
  - `Enter` で確定、`Escape` で取消
  - 通常選択からのドラッグ開始でも浮動選択へ移行可能
- [x] Phase 4: 選択範囲変形
  - 浮動選択に対して左右反転 / 上下反転 / 90度回転を適用
  - 変形後は `selection.w/h` と `floating.width/height` を同期

### 未着手 / 残課題

- [ ] Undo / Redo と選択ツールの統合
- [ ] 選択操作のクラス分離 (`tool_select_rect.js` など)
- [ ] 浮動選択プレビューの描画キャッシュ化
- [ ] 複数チップ選択・一括編集への拡張

---

## 技術メモ

### ピクセルフォーマット

```
Uint32Array の各要素は 0xAARRGGBB 形式
 - A: アルファ (0=透明, 255=不透明)
 - R/G/B: 各チャンネル 0〜255
PixelData.rgba(r, g, b, a) でパック、PixelData.unpack(color) でアンパック
```

### スクリプト読み込み順（index.html）

```
pixel_data.js → pixel_data_converter.js → layer_data.js → app_data.js → input.js
→ menu_constants.js → dropdown_menu.js → menu_bar.js
→ ui_window.js
→ pixel_canvas.js
→ dialog_base.js → new_file_dialog.js → new_tileset_dialog.js → import_tileset_dialog.js
→ save_dialog.js → color_picker_dialog.js → quarter_view_tile_dialog.js
→ color_palette.js → color_palette_16.js → editable_palette_32.js → color_palette_window.js
→ chip_palette.js → chip_palette_window.js
→ tile_preview.js → tile_preview_window.js
→ tool_bar.js → tool_bar_window.js
→ layer_panel.js → layer_panel_window.js
→ tileset_data.js → quarter_view_tile_generator.js
→ scene.js → scene_manager.js → editor_scene.js
→ canvas_manager.js
→ インラインスクリプト（起動）
```

### 主要クラスの関係

```
CanvasManager
  ├── AppData          ... LayerData + TilesetData + 色/ツール/モード状態を保持
  │     ├── LayerData      ... 複数 PixelData レイヤー + アルファ合成
  │     └── TilesetData    ... チップ単位 LayerData + 通過フラグ
  ├── Input            ... DOM イベントを canvas 相対座標に変換して通知
  └── SceneManager
        └── EditorScene (active)
              ├── MenuBar
              │     └── DropdownMenu[]
              ├── PixelCanvas          ... offsetX/Y + panX/Y でキャンバス配置
              ├── NewFileDialog
              ├── NewTilesetDialog / ImportTilesetDialog
              ├── SaveDialog
              ├── QuarterViewTileDialog
              ├── ToolBarWindow        ... UIWindow 継承・ドラッグ移動可能
              │     └── ToolBar            ... ツールボタンコンテンツ
              ├── ColorPaletteWindow   ... UIWindow 継承・ドラッグ移動可能
              │     └── EditablePalette32 ... ColorPalette 継承・32色編集コンテンツ
              ├── ChipPaletteWindow    ... UIWindow 継承・チップ一覧
              │     └── ChipPalette       ... チップサムネイルコンテンツ
              ├── TilePreviewWindow    ... UIWindow 継承・タイル繰り返し表示
              │     └── TilePreview       ... タイリングプレビューコンテンツ
              └── LayerPanelWindow     ... UIWindow 継承・ドラッグ移動可能
                    └── LayerPanel        ... レイヤー一覧コンテンツ
```

### UIWindow 継承パターン

```
UIWindow  (js/ui/ui_window.js)
  ├── render(ctx, canvas, appData)          → タイトルバー+背景+renderContent()
  ├── onMouseDown/Move/Up(e, appData)       → ドラッグ or コンテンツ委譲
  └── [override]
      ├── getWindowTitle()  → string
      ├── getContentSize()  → {w, h}
      ├── renderContent(ctx, cx, cy, cw, ch, appData)
      ├── onContentMouseMove(e, appData)
      ├── onContentMouseDown(e, appData)    → boolean
      └── onContentMouseUp(e, appData)

ColorPaletteWindow extends UIWindow
  └── ColorPalette  (コンテンツクラス)
  ├── ColorPalette16 extends ColorPalette
  └── EditablePalette32 extends ColorPalette
```

---
