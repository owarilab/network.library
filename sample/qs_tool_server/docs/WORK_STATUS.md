# ドットエディタ 作業状況

最終更新: 2026-04-27 (25)

## 概要

`sample/qs_tool_server/www/` 以下に HTML5 Canvas ベースのドット絵エディタを実装中。
外部ライブラリなし（Vanilla JS）。ゲームエンジン風の Scene/SceneManager パターンを採用。

サーバーは `sample/http_server_simple` の C製HTTPサーバーを流用または
`qs_tool_server` のサーバー実装を使う。URL: `http://localhost:4444/`

---

## ファイル構成

```
docs/
├── WORK_STATUS.md                 # 現在の実装状況
└── specs/
  ├── BINARY_FORMAT_PLAN.md        # QTS バイナリ形式の実装計画
  ├── BROWSER_STORAGE_PLAN.md      # IndexedDB を使ったブラウザ保存の最小設計
  ├── FEATURE_ROADMAP.md           # 今後の機能ロードマップ
  ├── LAYER_OPERATIONS_PLAN.md     # レイヤー操作拡張の計画
  ├── MAP_CHIP_SPEC.md             # マップチップエディタ仕様書
  ├── PROJECT_SCENE_FLOW_PLAN.md   # タイトル/プロジェクトトップ/遷移基盤の計画
  ├── PLAY_UNIT_DESIGN.md          # PlayUnit / PlayObject / Component 設計書
  ├── PLAY_UNIT_IMPLEMENTATION_PLAN.md # PlayUnit 実装計画書
  ├── CAMERA_PLAY_SETTINGS_JSON_EXAMPLES.md # Camera / PlaySettings 用 JSON テンプレート集
  ├── COLLIDER_TRIGGER_JSON_EXAMPLES.md # Collider / Trigger 用 JSON テンプレート集
  ├── IMAGE_COMPONENT_JSON_EXAMPLES.md # Image component 用 JSON テンプレート集
  ├── TEXT_COMPONENT_JSON_EXAMPLES.md # Text component 用 JSON テンプレート集
  ├── SELECTION_TOOL_PLAN.md       # 選択ツール実装計画
  ├── SPEC_quarter_view_tile.md    # クォータービュータイル生成仕様書
  └── UNDO_REDO_PLAN.md            # Undo / Redo 実装計画

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
    ├── project/
    │   ├── project_data.js         # ProjectData: 保存対象のプロジェクト最小ルート
    │   ├── project_session.js      # ProjectSession: シーン間の一時編集状態
    │   └── project_browser_storage.js # ProjectBrowserStorage: IndexedDB 保存層
    ├── scene/
    │   ├── scene.js                # Scene: 基底クラス
    │   ├── title_scene.js          # TitleScene: 起動時の入口
    │   ├── project_top_scene.js    # ProjectTopScene: プロジェクト単位のハブ
    │   └── editor_scene.js         # EditorScene: ドット絵エディタシーン
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

関連仕様:

- [クォータービュータイル自動生成 仕様書](specs/SPEC_quarter_view_tile.md)
- [プロジェクト管理・シーン遷移基盤 計画書](specs/PROJECT_SCENE_FLOW_PLAN.md)
- [PlayUnit 設計書](specs/PLAY_UNIT_DESIGN.md)
- [PlayUnit 実装計画書](specs/PLAY_UNIT_IMPLEMENTATION_PLAN.md)
- [Camera / PlaySettings JSON 例集](specs/CAMERA_PLAY_SETTINGS_JSON_EXAMPLES.md)
- [Collider / Trigger JSON 例集](specs/COLLIDER_TRIGGER_JSON_EXAMPLES.md)
- [Image component JSON 例集](specs/IMAGE_COMPONENT_JSON_EXAMPLES.md)
- [Text component JSON 例集](specs/TEXT_COMPONENT_JSON_EXAMPLES.md)
- [ブラウザ保存（IndexedDB）最小設計書](specs/BROWSER_STORAGE_PLAN.md)
- [マップエディタシーン実装計画書](specs/MAP_EDITOR_SCENE_PLAN.md)

---

## 次フェーズ計画

次の実装フェーズでは、単一のドット絵エディタ構成から、
プロジェクトを起点に複数シーンへ遷移する構成へ移行する。

追加予定の中核要素:

- `TitleScene`: 起動直後の入口。新規プロジェクト作成、ロードを担当
- `ProjectTopScene`: プロジェクト単位のホーム。各派生シーンへの遷移ハブ
- `Project` 概念: ドット絵、タイルセット、将来のマップなどを束ねる管理単位
- シーン間コンテキスト受け渡し基盤: `Project` を中心にデータを引き継ぐ

詳細計画:

- [プロジェクト管理・シーン遷移基盤 計画書](specs/PROJECT_SCENE_FLOW_PLAN.md)

---

## 実装済み機能

### 基盤

- [x] `CanvasManager`: 全画面 canvas、RAF 60fps ループ（ドリフト補正付き）、resize 対応
- [x] `Scene` / `SceneManager`: シーン切り替え時ライフサイクル（onEnter/onLeave）、`input.clearAll()`
  - シーン名を `ProjectSession.currentScene` に反映する最小連携を追加
  - シーン切り替え直後に resize 処理を再実行して表示崩れを抑制
- [x] `Input`: keyboard/mouse/touch イベント一元管理、canvas 相対座標正規化、wheel/contextmenu 既定動作抑止
- [x] `AppData`: `LayerData` インスタンスを保持、`pixelData` getter でアクティブレイヤー返却、`createPixelData()` デリゲート
  - `currentProject` / `projectSession` / `sceneManager` を追加
  - `ProjectSession` への編集状態同期 API を追加
- [x] `TitleScene` / `ProjectTopScene` の最小導線
  - 起動時は `TitleScene` へ入り、`New Project` から `ProjectTopScene` へ遷移
  - `ProjectTopScene` から `EditorScene` を開くハブ導線を追加
  - `TitleScene` から `.qsproj` のロード、`ProjectTopScene` から `.qsproj` の保存を追加
  - `EditorScene` の `File > Exit` から `ProjectTopScene` へ戻る導線を追加
  - `ProjectTopScene` に既存 asset 一覧表示と選択オープン導線を追加
  - `TitleScene` に browser project 一覧表示・削除導線を追加
  - `ProjectTopScene` の保存導線を browser 保存 / `.qsproj` export に分離
- [x] `PlayUnit` / `PlayTest` の最小 runtime 導線
  - `PlayUnitData.createDefault()` で `Root` / `CameraObject` / `PlaySettingsObject` の初期生成を追加
  - `PlayTestScene` で `Transform + Text` / `Transform + Image` の最小 preview を追加
  - `Image` preview に `keepAspect` と `originX` / `originY` を追加
  - `PlaySettings.defaultCameraObjectId` による default camera 解決を追加
  - `Camera.followTargetObjectId` / `followLerp` による最小 follow preview を追加
  - `Controller` component による object 自己操作 preview を追加
  - `PlayUnitEditorScene` に `+CameraObject` / `+ImageObject` / `+Camera` / `+Controller` / `+Image` を追加
  - `+ImageObject` 作成時に既存 `pixelDocument` を prompt で選択できるようにした
  - `Trigger` component テンプレートを `triggerOn` 付きの新仕様へ更新
  - `PlayTestScene` に pointer ベースの `Collider + Trigger` 最小ログ表示を追加
  - `PlayTestScene` に `Controller + Collider` と `Trigger + Collider` の `overlap` 最小ログ表示を追加
  - default camera 表示と object ID のコピペ用 input を追加
  - camera の中央基準描画や screen-space 分離は仕様未確定のため保留
- [x] `MapEditorScene` の雛形追加
  - `ProjectTopScene` から map asset を開けるように変更
  - `UIWindow` ベースの `MapViewWindow` / `MapTilesetWindow` / `MapInspectorWindow` を追加
  - 既存 tileset を選んでマップセルへ配置できる最小編集を追加
  - `map` アセットの project 保存フローを `AppData.saveActiveProjectAssetState()` に接続
  - 最小メニュー UI を `MapEditorMenuBar` として分離し、`保存` / `戻る` / `グリッド表示` を追加
  - メニューバーはウィンドウ群より前面で描画するように調整
  - `MapResizeDialog` を追加し、`ファイル > マップサイズ変更...` からセル数の変更を可能にした
  - `MapViewWindow` に viewport ベースの部分描画を導入し、wheel zoom / pan / inspector reset を追加
  - visible range と pan clamp を調整し、大きいマップで端の欠けや空白が出にくいように補正
- [x] シーン切り替え直後の canvas 文字描画状態の補正
  - `MenuBar` 描画で `textAlign` / `textBaseline` を明示初期化
- [x] `TitleScene` の Browser Projects 一覧に `map` 件数表示を追加

### プロジェクト基盤

- [x] `ProjectData`:
  - `id / version / name / createdAt / updatedAt`
  - `assets.pixelDocuments / tilesets / maps`
  - `settings.defaultChipWidth / defaultChipHeight`
  - `addPixelDocument()` / `addTileset()` / `addMap()` / `getAssetByRef()`
- [x] `ProjectSession`:
  - `projectId / dirty / currentScene`
  - `activeDocumentRef`
  - `editorState` (`activeTool / foreColor / backColor / editMode / selectedChip`)
- [x] `ProjectSerializer`:
  - `.qsproj` JSON 形式で `ProjectData + ProjectSession` を保存/復元
  - 画像アセット本体は `qts-base64` 埋め込みで保持

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

#### 次フェーズ基盤

- [x] **タイトルシーン追加** (`js/scene/title_scene.js`)
  - 起動時の入口を `EditorScene` から分離
  - 新規プロジェクト作成導線を配置
  - ロード導線はプレースホルダ
- [x] **プロジェクトトップシーン追加** (`js/scene/project_top_scene.js`)
  - ドット絵エディタや今後の派生シーンへの遷移ハブ
  - `Map Editor` から `MapEditorScene` の雛形を開ける
  - 2 列ボタン配置とショートカット表示を追加し、`Open Map Editor` 導線を見つけやすく調整
- [x] **Project / ProjectSession 導入** (`js/project/`)
  - シーン間で共有する編集対象と、一時状態を分離して管理
  - `AppData` と最小同期を接続済み
- [x] **EditorScene のプロジェクト配下化（最小）**
  - 既存ドット絵エディタをプロジェクト起点の1機能として接続
  - `ProjectTopScene` 経由で `EditorScene` を開く構成へ変更
  - 詳細計画: [PROJECT_SCENE_FLOW_PLAN.md](specs/PROJECT_SCENE_FLOW_PLAN.md)
- [x] **Project ロード / 保存導線**
  - `TitleScene` の `Load Project` で `.qsproj` 読込
  - `ProjectTopScene` から browser 保存と `.qsproj` export を実行可能
  - `EditorScene` 入退場時に active asset を project へ同期
  - `ProjectTopScene` の asset 一覧から復元済み asset を選択して確認可能

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
- [x] **EditablePalette32** (`js/ui/editable_palette_32.js`)
  - 32色編集パレットとして tileset/workflow 向けに利用可能
  - 仕様書: [BINARY_FORMAT_PLAN.md](specs/BINARY_FORMAT_PLAN.md)

---

### 優先度: 中（快適な操作のため）

#### Undo / Redo

- [x] **コマンドパターン土台** (`js/command/`)
  - `CommandBase`: `execute() / undo()` インターフェース
  - `PixelStrokeCommand`: 描画操作1ストローク分のピクセル変更を保持
  - `FloodFillCommand`: 塗りつぶし操作
- [x] **HistoryManager** (`js/history_manager.js`)
  - `execute(command)` / `undo()` / `redo()` / `clear()` の最小実装
  - `canUndo()` / `canRedo()` / `getUndoLabel()` / `getRedoLabel()` を追加
  - `_isApplying` による履歴適用中ガード
- [x] `AppData.history` と `index.html` の読み込み順を接続
- [x] メニュー `EDIT_UNDO / EDIT_REDO` を `HistoryManager` に接続
- [x] `Ctrl+Z` / `Ctrl+Y`（または `Ctrl+Shift+Z`）のキーボードショートカット
- [x] **ペンシル / 消しゴムの Undo / Redo**
  - 1ストロークを 1 コマンドとして履歴化
  - free モード / tileset モードで同じ履歴基盤を利用
- [x] **塗りつぶしの Undo / Redo**
  - `_floodFill()` を変更集合返却へ変更
  - `FloodFillCommand` 経由で履歴化
- [x] ブラウザ上で `pencil / eraser / fill` の Undo / Redo 挙動を確認
- [x] **TransformCommand** (`js/command/transform_command.js`)
  - 左右反転 / 上下反転 / 回転の履歴化
  - 通常レイヤー変形のみ履歴対応、浮動選択変形は未対応
- [x] ブラウザ上で `flip / rotate` の Undo / Redo 挙動を確認
- [x] **NewFileCommand** (`js/command/new_file_command.js`)
  - 通常新規作成の履歴化
  - `AppData` の編集状態スナップショットで undo/redo を復元
- [x] ブラウザ上で `new file` の Undo / Redo 挙動を確認
- [x] **LayerPanel のコマンド経由化** (`js/command/layer_command.js` + `js/ui/layer_panel.js`)
  - `Add/Remove/Move/ToggleVisibility/ToggleLocked/Rename/Duplicate/MergeLayerDown` を追加
  - `LayerPanel` の直接更新を `appData.history.execute(...)` 経由へ置換
  - ブラウザ上で `add/remove/move/visible/lock/rename/duplicate/merge` の Undo / Redo 挙動を確認
- [x] **Layer opacity UI** (`js/ui/layer_panel.js` + `SetLayerOpacityCommand`)
  - LayerPanel 下部詳細欄に `Opacity: xx%` と横スライダーを追加
  - ドラッグ中は即時プレビューし、mouse up で 1 つの履歴へ統合
  - ブラウザ上で opacity ドラッグの preview と 1 履歴 undo/redo を確認
- [ ] **タイルセット操作の Undo / Redo 統合**
  - `clearChip / pasteChipLayerData / swapChips / addRow / addColumn / removeRow / removeColumn` は未コマンド化
  - 通過フラグ `passFlags[row][col]` も直接更新経路が残っている
  - 選択ツールと同様、既存履歴基盤へ寄せるのが次段階

#### ズーム改善

- [ ] ズーム中心をマウスカーソル位置に固定（現在は画面中央固定）
  - 公式: `panX -= (centerX - canvas.width/2) * (newScale - oldScale) / oldScale` 系の計算
- [ ] メニュー `VIEW_ZOOM_IN / VIEW_ZOOM_OUT` を `pixelCanvas.zoom()` に接続
- [ ] メニュー `VIEW_ZOOM_RESET` を `pixelCanvas.resetPan()` + スケール初期化に接続
  - ホイールズーム自体は実装済みだが、メニュー項目は未接続
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
  - 通常ドット絵は `1x1` タイルセットとして `.qts` 保存可能
  - `1x1` `.qts` 読込時は free mode の多レイヤー画像として復元
  - `SaveDialog` に free mode / tileset mode 別の QTS 説明文を追加
  - `EditorScene` で開いた `.qts` は current project の asset として追加・管理
  - 1バイトピクセル（ColorID + passable）で量子化保存
  - 仕様書: [BINARY_FORMAT_PLAN.md](specs/BINARY_FORMAT_PLAN.md)
- [ ] **通常ドット絵(JSON v1)のメタ情報拡張**

#### プロジェクト永続化

- [ ] **ブラウザ保存 (IndexedDB)**
  - `.qsproj` の外部 import/export とは別に browser 内へ project を保存
  - `IndexedDB` に `projects / project_sessions / assets` を分割保存
  - asset 本体は `qts-base64` 文字列ではなく `ArrayBuffer` ベースで保持
  - 最小設計: [BROWSER_STORAGE_PLAN.md](specs/BROWSER_STORAGE_PLAN.md)
  - `ProjectBrowserStorage` を追加し、save/load/list/delete の最小実装を接続
  - `TitleScene` から browser project 一覧表示・読込・削除を実行可能
  - `ProjectTopScene` から browser 保存、`.qsproj` export を実行可能
  - `ProjectTopScene` から `Save As (Browser)` で別 project として複製保存可能
  - browser 保存した project の asset / session 復元を確認済み
  - browser project の削除、`Save As` による複製保存を確認済み
  - 未対応: 一覧のスクロール/ページング、recent projects の整備

- [x] **新規作成 asset の project 追加**
  - `EditorScene` の `File > New` で作成した `pixelDocument` を current project に追加
  - `EditorScene` の `File > New Tileset` で作成した `tileset` を current project に追加
  - 新規作成前の active asset は project へ保存し、新規 asset を activeDocument に切替

#### タイルセットモード

- [x] `AppData` に `editMode` / `tilesetData` / `selectedChip` / `chipClipboard` / `palette` / `showPassFlags` を追加
- [x] **`TilesetData`** を実装
  - チップ単位 `LayerData` 管理（`chips[row][col]`）
  - 通過フラグ配列 `passFlags[row][col]`
  - `clearChip / copyChip / swapChips / cloneChipLayerData / pasteChipLayerData`
  - `addRow / addColumn / removeRow / removeColumn / resize`
  - 仕様書: [BINARY_FORMAT_PLAN.md](specs/BINARY_FORMAT_PLAN.md)
- [x] **`EditorScene` へのタイルセット統合**
  - 新規/読込: `FILE_NEW_TILESET` / `FILE_OPEN_TILESET`
  - 出力: `FILE_EXPORT_TILESET`(PNG) / `FILE_EXPORT_CHIP`(PNG)
  - チップ操作: `TILESET_COPY_CHIP / PASTE_CHIP / CLEAR_CHIP / SWAP_CHIP`
  - 構造変更: `TILESET_ADD_ROW/COL / REMOVE_ROW/COL`
  - 表示: `TILESET_TILE_PREVIEW` / `VIEW_PASS_FLAGS`
- [x] **タイルセット編集メニュー拡張**
  - `EDIT_ROTATE_CW` / `EDIT_ROTATE_CCW` は通常モード・タイルセットモード両対応
  - `EDIT_FLIP_H` / `EDIT_FLIP_V` は両モード対応済み

#### グリッド・表示設定

- [x] メニュー `VIEW_GRID` でグリッド線表示/非表示トグル
- [ ] グリッド色・透明度をカスタマイズする設定ダイアログ

#### 編集ショートカット

- [x] `Ctrl+C / Ctrl+X / Ctrl+V / Ctrl+A` を `EditorScene._onKeyDown` に接続
- [x] `Enter` で浮動選択を確定
- [x] `Escape` で選択解除 / 浮動選択キャンセル

---

### 優先度: 低（拡張機能）

#### タイルセット関連 UI

- [x] **ChipPalette / ChipPaletteWindow**
  - チップ一覧表示、選択、右クリック操作に対応
- [x] **TilePreview / TilePreviewWindow**
  - 選択チップの 3x3 / 5x5 繰り返し表示
- [x] **NewTilesetDialog / ImportTilesetDialog**
  - 新規タイルセット作成、画像からのタイルセット変換設定を実装
- [x] **QuarterViewTileDialog**
  - `Generate > Quarter View Tile` メニューから起動可能
  - 仕様書: [SPEC_quarter_view_tile.md](specs/SPEC_quarter_view_tile.md)

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
- [x] レイヤー不透明度スライダー UI
  - 下部詳細欄に `Opacity: xx%` 表示とスライダーを実装
  - ドラッグ中プレビュー、mouse up 時に 1 履歴へ統合
- [ ] 不透明度の視覚UI改善
  - 微調整用の UI 磨き込みや表示密度の改善は余地あり

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
