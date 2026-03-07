# ドットエディタ 作業状況

最終更新: 2026-03-07 (8)

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
    ├── input.js                    # Input: キーボード/マウス/タッチイベント一元管理
    ├── pixel_canvas.js             # PixelCanvas: PixelData描画・ズーム・パン
    ├── canvas_manager.js           # CanvasManager: RAFループ・各マネージャ生成
    ├── scene_manager.js            # SceneManager: アクティブシーン管理・切り替え
    ├── scene/
    │   ├── scene.js                # Scene: 基底クラス
    │   └── editor_scene.js         # EditorScene: メインシーン（描画・メニュー操作）
    └── ui/
        ├── menu_constants.js       # MenuConstants: メニューID定数
        ├── menu_bar.js             # MenuBar: メニューバー描画
        ├── dropdown_menu.js        # DropdownMenu: プルダウンリスト
        ├── color_palette.js        # ColorPalette: カラーパレット基底クラス (コンテンツのみ)
        ├── color_palette_16.js     # ColorPalette16: 固定16色パレット
        ├── color_palette_window.js # ColorPaletteWindow: UIWindow継承のパレットウィンドウ
        ├── tool_bar.js             # ToolBar: ツールボタンコンテンツクラス
        ├── tool_bar_window.js      # ToolBarWindow: UIWindow継承のツールバーウィンドウ
        ├── layer_panel.js          # LayerPanel: レイヤー一覧コンテンツクラス
        ├── layer_panel_window.js   # LayerPanelWindow: UIWindow継承のレイヤーパネル
        ├── ui_window.js            # UIWindow: ドラッグ可能ウィンドウ共通基底クラス
        └── dialog/
            ├── dialog_base.js      # DialogBase: モーダルダイアログ基底クラス
            ├── new_file_dialog.js   # NewFileDialog: 新規作成ダイアログ
            ├── save_dialog.js       # SaveDialog: エクスポートダイアログ
            └── color_picker_dialog.js # ColorPickerDialog: HSVカラーピッカーダイアログ
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
  - FILE: `NEW / OPEN / SAVE / SAVE_AS / EXIT`
  - EDIT: `UNDO / REDO / CUT / COPY / PASTE / SELECT_ALL`
  - VIEW: `GRID / ZOOM_IN / ZOOM_OUT / ZOOM_RESET`
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
  - pencil / eraser / fill / eyedropper の 4 ツールに対応
  - アイコン: canvas 2D で描画した幾何グラフィック
  - ホバー時「ツール名」ツールチップ表示
  - `AppData.activeTool` でアクティブツールを管理
- [x] **アクティブツール管理** (`AppData.activeTool`)
  - `EditorScene._applyTool()` でツールごとにディスパッチ
- [x] **ペンシルツール** — `foreColor`/`backColor` で左/右クリック描画
- [x] **消しゴムツール** — `0x00000000`（透明）で塃塩つぶし
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
- [ ] **JSON 形式の独自保存フォーマット**（メタ情報付き）

#### グリッド・表示設定

- [x] メニュー `VIEW_GRID` でグリッド線表示/非表示トグル
- [ ] グリッド色・透明度をカスタマイズする設定ダイアログ

---

### 優先度: 低（拡張機能）

#### 選択ツールとクリップボード

- [ ] **矩形選択ツール** (`js/tool/tool_select_rect.js`)
  - 選択範囲をダッシュ枠で描画（マーチングアンツアニメーション）
  - `AppData.selection` で選択状態を保持
- [ ] **EDIT_CUT / COPY / PASTE** の実装
  - カット: 選択範囲をクリップボードバッファへ、元ピクセルを透明に
  - コピー: 選択範囲をバッファへ
  - ペースト: バッファをドラッグ配置
- [ ] **EDIT_SELECT_ALL** の実装

#### 図形ツール

- [ ] **直線ツール** (`js/tool/tool_line.js` / Bresenham アルゴリズム)
- [ ] **矩形ツール** (`js/tool/tool_rect.js`、塗りつぶし/枠線モード)
- [ ] **楕円ツール** (`js/tool/tool_ellipse.js`、Midpoint Circle/Ellipse アルゴリズム)

#### レイヤー

- [x] **`LayerData`** クラス (`js/layer_data.js`): 複数 `PixelData` レイヤー管理・アルファ合成
  - `init(w, h, fill)` で1レイヤー構成で初期化
  - `addLayer()` / `removeLayer()` / `moveLayer()` レイヤー操作
  - `toggleVisibility()` / `setOpacity()` 表示制御
  - `composite()` ボトムアップ Porter-Duff "source over" アルファ合成、キャッシュ付き
  - `markCompositeDirty()` で合成キャッシュ無効化
- [x] **`LayerPanel`** UI (`js/ui/layer_panel.js`): レイヤー一覧コンテンツ
  - 最前面が上の行表示、アクティブレイヤーハイライト
  - 目アイコンで visibility トグル
  - 下部ボタン: [＋追加] [−削除] [↑上へ] [↓下へ]
  - `onChange` コールバックで外部に変更通知
- [x] **`LayerPanelWindow`** (`js/ui/layer_panel_window.js`): UIWindow 継承
  - 初回描画時に画面右寄せ配置
- [x] **`AppData`** 改修: `layerData` プロパティ追加、`pixelData` を getter/setter 化（後方互換）
- [x] **`EditorScene`** 統合: 合成画像描画、スポイトは合成結果から色取得、PNGエクスポートは合成結果
- [ ] レイヤー不透明度スライダー UI
- [ ] レイヤー名変更ダイアログ

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
→ dialog_base.js → new_file_dialog.js → save_dialog.js → color_picker_dialog.js
→ color_palette.js → color_palette_16.js → color_palette_window.js
→ tool_bar.js → tool_bar_window.js
→ layer_panel.js → layer_panel_window.js
→ scene.js → scene_manager.js → editor_scene.js
→ canvas_manager.js
→ インラインスクリプト（起動）
```

### 主要クラスの関係

```
CanvasManager
  ├── AppData          ... LayerData + foreColor + backColor + activeTool を保持
  │     └── LayerData      ... 複数 PixelData レイヤー + アルファ合成
  ├── Input            ... DOM イベントを canvas 相対座標に変換して通知
  └── SceneManager
        └── EditorScene (active)
              ├── MenuBar
              │     └── DropdownMenu[]
              ├── PixelCanvas          ... offsetX/Y + panX/Y でキャンバス配置
              ├── NewFileDialog
              ├── SaveDialog
              ├── ToolBarWindow        ... UIWindow 継承・ドラッグ移動可能
              │     └── ToolBar            ... ツールボタンコンテンツ
              ├── ColorPaletteWindow   ... UIWindow 継承・ドラッグ移動可能
              │     └── ColorPalette16 ... ColorPalette 継承・16色コンテンツ
              └── LayerPanelWindow     ... UIWindow 継承・ドラッグ移動可能
                    └── LayerPanel     ... レイヤー一覧コンテンツ
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
        └── ColorPalette16 extends ColorPalette
```

---

## 次のセッションで最初に着手すべき作業

1. **Undo/Redo (コマンドパターン)** —— `js/command/` 下に `CommandBase` / `DrawCommand` / `FillCommand` を実装、`HistoryManager` を作成
2. ~~**グリッド表示トグル** —— メニュー `VIEW_GRID` を `PixelCanvas` に接続~~ ✓ 完了
3. ~~**ピクセルキャンバスのリセット** — インポート後にキャンバス中央配置へ引こむ（パン・ズームリセット）~~ ✓ 完了

**優先度:低 — ツールID キーボードショートカット:**
- `P` → pencil、`E` → eraser、`G` → fill、`I` → eyedropper
- `EditorScene` の `_onKeyDown` に追加するだけで実現可能
