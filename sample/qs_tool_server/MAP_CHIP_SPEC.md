# マップチップエディタ 仕様書

最終更新: 2026-03-08 (v2: チップ独立レイヤー方式)

## 1. 概要

既存のドット絵エディタを拡張し、2Dゲーム用マップチップ（タイルセット）画像を作成・管理するツールに発展させる。

### 1.1 コンセプト

- **1枚のタイルセット画像**の中に複数のマップチップ（タイル）を格子状に配置して編集する
- 個々のチップをピクセル単位で編集する既存機能はそのまま活用
- チップの一覧・選択・並び替え・プレビューなどマップチップ特有のワークフローを追加
- 既存のドット絵エディタとしての機能（レイヤー・パレット・ツール）はすべて維持

### 1.2 用語定義

| 用語 | 説明 |
|------|------|
| **チップ (Chip/Tile)** | タイルセット内の1つのマス。通常 16×16 や 32×32 ピクセル |
| **タイルセット (Tileset)** | 複数のチップを格子状に並べた1枚の画像 |
| **チップサイズ** | 1チップの幅×高さ（px）。タイルセット全体で統一 |
| **グリッドID** | タイルセット内のチップ位置。左上を (0,0) とした列・行番号 |
| **タイルインデックス** | チップの通し番号。左上から右方向→下方向の順（0始まり） |

---

## 2. 現状の設計分析

### 2.1 活用できる既存機能

| 既存機能 | マップチップエディタでの活用 |
|----------|------------------------------|
| `PixelData` (Uint32Array) | 各チップのピクセルデータ保持 |
| `LayerData` (多レイヤー合成) | チップごとのレイヤー管理（下地/輪郭/ディテール等を独立管理） |
| `PixelCanvas` (ズーム・パン・グリッド) | チップ編集ビューの描画エンジン |
| `ToolBar` (pencil/eraser/fill/eyedropper) | ピクセル編集ツール群 |
| `ColorPalette16` + `ColorPickerDialog` | 色選択 |
| `UIWindow` (ドラッグ可能ウィンドウ) | 新規UIパネルの基盤 |
| `PixelDataConverter` (PNG/JSON入出力) | タイルセットのインポート・エクスポート |
| `MenuBar` + `DropdownMenu` | メニュー拡張 |
| `Scene` / `SceneManager` | エディタモード切り替え |

### 2.2 不足している機能（要新規実装）

- タイルセットのグリッド管理とチップ単位の操作
- チップ選択・ナビゲーション UI
- タイリングプレビュー
- チップ間のコピー・入れ替え操作
- タイルセット新規作成ダイアログ（チップサイズ・枚数指定）
- アニメーションタイル管理（将来）

---

## 3. データモデル

### 3.1 TilesetData クラス（新規）

タイルセット全体のメタ情報とチップアクセスを担う中核クラス。
各チップが独立した `LayerData` を持ち、チップごとに自由なレイヤー構成が可能。

```
TilesetData
  chipWidth    : number              // 1チップの幅 (px)  例: 16, 32
  chipHeight   : number              // 1チップの高さ (px) 例: 16, 32
  columns      : number              // 横方向のチップ数
  rows         : number              // 縦方向のチップ数
  chips        : LayerData[][]       // chips[row][col] = LayerData (チップごとのレイヤー)
```

**画像サイズの算出:**
```
imageWidth  = chipWidth  × columns
imageHeight = chipHeight × rows
```

**重要な設計判断:** 各チップが独立した `LayerData` を持つ。タイルセット全体を1枚の画像として扱うのではなく、チップ単位でレイヤー構成を管理する。

**理由:**
- チップごとに異なるレイヤー構成が可能（草チップは1レイヤー、建物チップは3レイヤーなど）
- `AppData.layerData` の getter を差し替えるだけで、既存の `PixelCanvas` / `LayerPanel` / ツール群が無改修で動作する
- チップ選択を切り替えると、レイヤーパネルの内容もそのチップのレイヤー構成に自動的に切り替わる
- Undo/Redo がチップ単位で軽量に管理できる
- PNGエクスポート時は `compositeAll()` で全チップを1枚に組み立てて出力する

### 3.1.1 既存コードとの互換メカニズム

既存コードはすべて `appData.layerData` 経由でデータにアクセスしている。
この getter がモードに応じて返すオブジェクトを切り替えることで、
下流のコード（PixelCanvas / LayerPanel / ツール群）は無改修で動作する。

```
[tileset モード]
  appData.layerData  →  tilesetData.chips[selectedRow][selectedCol]  (選択チップの LayerData)
  appData.pixelData  →  上記 LayerData のアクティブレイヤー

[free モード (従来)]
  appData.layerData  →  appData._layerData  (従来通り)
  appData.pixelData  →  上記 LayerData のアクティブレイヤー
```

### 3.2 TilesetData API

```javascript
// ---- コンストラクタ ----
constructor(chipW, chipH, cols, rows, fillColor = 0x00000000)
  // chips[row][col] に LayerData を生成・初期化

// ---- チップの LayerData アクセス ----
getChipLayerData(col, row)   → LayerData   // 指定チップの LayerData を返す

// ---- タイルセット全体画像の組み立て (エクスポート・チップパレット表示用) ----
compositeAll()               → PixelData   // 全チップの composite() を1枚に配置
compositeChip(col, row)      → PixelData   // 指定チップの composite() を返す (単純委譲)

// ---- チップ単位の操作 ----
clearChip(col, row)                         // チップの全レイヤーをクリア
copyChip(srcCol, srcRow, dstCol, dstRow)    // チップ間コピー (LayerData のディープコピー)
swapChips(col1, row1, col2, row2)           // LayerData の参照を入れ替え

// ---- タイルセット構造変更 ----
addColumn()                                 // 右端に1列追加 (新規 LayerData を生成)
addRow()                                    // 下端に1行追加
removeColumn()                              // 右端の1列を削除
removeRow()                                 // 下端の1行を削除
resize(newCols, newRows)                    // 列・行数を変更（既存チップ保持）
```

### 3.2.1 compositeAll() の実装方針

チップパレット表示とPNGエクスポートで使用する全体画像組み立てメソッド。

```javascript
compositeAll() {
  const fullPd = new PixelData();
  fullPd.createPixelData(this.chipWidth * this.columns, this.chipHeight * this.rows);
  for (let r = 0; r < this.rows; r++) {
    for (let c = 0; c < this.columns; c++) {
      const chipPd = this.chips[r][c].composite();
      const ox = c * this.chipWidth;
      const oy = r * this.chipHeight;
      for (let y = 0; y < this.chipHeight; y++) {
        for (let x = 0; x < this.chipWidth; x++) {
          fullPd.setPixel(ox + x, oy + y, chipPd.getPixel(x, y));
        }
      }
    }
  }
  return fullPd;
}
```

**最適化:** 変更フラグ `_compositeAllDirty` を持ち、チップの描画操作時のみ再生成する。各チップの `LayerData` はすでに内部にcompositeキャッシュを持つため、変更のないチップは高速にスキップされる。

### 3.3 AppData の拡張

```javascript
class AppData {
  constructor() {
    // --- 既存プロパティ ---
    this._layerData  = new LayerData();   // free モード用 (名前を _ 付きに変更)
    this.foreColor   = PixelData.rgba(0, 0, 0, 255);
    this.backColor   = PixelData.rgba(255, 255, 255, 255);
    this.activeTool  = 'pencil';

    // --- 新規プロパティ ---
    this.tilesetData  = null;             // TilesetData | null
    this.selectedChip = { col: 0, row: 0 };
    this.editMode     = 'free';           // 'free' | 'tileset'
  }

  /**
   * モードに応じた LayerData を返す。
   * tileset モード: 選択中チップの LayerData
   * free モード: 従来の _layerData
   * → これにより PixelCanvas / LayerPanel / ツール群は無改修で動作
   */
  get layerData() {
    if (this.editMode === 'tileset' && this.tilesetData) {
      return this.tilesetData.getChipLayerData(
        this.selectedChip.col, this.selectedChip.row
      );
    }
    return this._layerData;
  }

  /**
   * アクティブレイヤーの PixelData を返す (後方互換)。
   */
  get pixelData() {
    return this.layerData.getActiveLayer();
  }

  // set pixelData(pd) { ... }          // 既存のインポート互換用 setter は維持
  // createPixelData(w, h, fill) { ... } // 既存の初期化メソッドは _layerData に委譲
}
```

**ポイント:** `layerData` プロパティを getter 化するのが、この設計の中核。既存コードで `appData.layerData` を参照している箇所（EditorScene / LayerPanel / PixelCanvas等）はすべて、チップ選択を切り替えるだけで自動的に対象チップのレイヤーデータに切り替わる。

---

## 4. 画面構成

### 4.1 メインレイアウト

```
┌─────────────────────────────────────────────────────┐
│  メニューバー [ファイル] [編集] [表示] [タイルセット]  │
├──────────────────────────┬──────────────────────────┤
│                          │   チップパレット          │
│                          │   ┌──┬──┬──┬──┐          │
│     メイン編集領域        │   │00│01│02│03│          │
│  (PixelCanvas)           │   ├──┼──┼──┼──┤          │
│                          │   │04│05│06│07│          │
│  ・チップ単位グリッド表示 │   ├──┼──┼──┼──┤          │
│  ・選択チップ強調表示     │   │08│09│10│11│          │
│                          │   └──┴──┴──┴──┘          │
│                          │   [選択中チップ: (1,0)]   │
│                          ├──────────────────────────┤
│                          │   タイルプレビュー         │
│                          │   ┌────────────┐         │
│                          │   │ ■■■■■■     │         │
│                          │   │ ■■■■■■     │         │
│                          │   │ ■■■■■■     │         │
│                          │   └────────────┘         │
├──────────────────────────┤   [3×3 タイリング]        │
│                          ├──────────────────────────┤
│  ToolBar  ColorPalette   │   LayerPanel             │
│  Window   Window         │                          │
└──────────────────────────┴──────────────────────────┘
```

### 4.2 新規UIコンポーネント

#### 4.2.1 チップパレットウィンドウ (`ChipPaletteWindow`)

タイルセット内の全チップをサムネイル一覧として表示する UIWindow。

- タイルセット全体をスケール縮小して格子表示
- 選択中チップを枠線でハイライト
- クリックでチップ選択 → メイン編集領域がそのチップ位置にフォーカス
- 右クリックでコンテキストメニュー（コピー/ペースト/クリア/入れ替え）
- ドラッグ&ドロップでチップ位置の入れ替え

**実装:**
```
ChipPaletteWindow extends UIWindow
  └── ChipPalette (コンテンツクラス)
        tilesetData  : TilesetData
        selectedChip : { col, row }
        scrollY      : number        // 縦スクロール
```

#### 4.2.2 タイルプレビューウィンドウ (`TilePreviewWindow`)

選択中のチップがタイリング（繰り返し配置）されたときの見た目をプレビューする UIWindow。

- 3×3 または 5×5 のタイリングプレビュー
- 隣接するチップとの境界がつながるかを確認できる
- プレビューサイズの切り替え（3×3 / 5×5 / カスタム）

**実装:**
```
TilePreviewWindow extends UIWindow
  └── TilePreview (コンテンツクラス)
        repeatX : number  // 横方向繰り返し数
        repeatY : number  // 縦方向繰り返し数
```

#### 4.2.3 タイルセット新規作成ダイアログ (`NewTilesetDialog`)

タイルセットモードの新規作成時に表示するダイアログ。

- **チップサイズ選択:** 8×8 / 16×16 / 32×32 / 48×48 / 64×64（プリセット＋カスタム入力）
- **タイルセットサイズ:** 列数 × 行数（例: 8列 × 16行）
- **背景色:** 透明 / 白
- 最終画像サイズのプレビュー表示（例: 「256 × 512 px」）

---

## 5. メニュー構成の変更

### 5.1 ファイルメニュー拡張

```
ファイル
├── 新規作成               (既存: 自由キャンバス)
├── 新規タイルセット作成    ★ 新規
├── ─────────
├── 開く                    (既存)
├── タイルセットを開く      ★ 新規 (PNG読み込み時にチップサイズ指定)
├── ─────────
├── 保存                    (既存: PNG/JSON)
├── タイルセット書き出し    ★ 新規 (1枚のPNG画像)
├── 選択チップ書き出し      ★ 新規 (選択チップのみPNG)
└── ─────────
```

### 5.2 タイルセットメニュー（新規追加）

```
タイルセット
├── チップサイズ変更
├── ─────────
├── 行を追加
├── 列を追加
├── 行を削除
├── 列を削除
├── ─────────
├── チップをコピー
├── チップをペースト
├── チップをクリア
├── チップを入れ替え
├── ─────────
├── タイルプレビュー表示
└── チップパレット表示
```

---

## 6. チップ編集ワークフロー

### 6.1 基本的な操作フロー

```
1. [新規タイルセット作成] からチップサイズ・列行数を指定して開始
2. チップパレットから編集したいチップをクリック選択
3. メイン編集領域でズームイン → ピクセル単位で描画 (既存ツール使用)
4. タイルプレビューで繰り返し表示を確認
5. 別のチップを選択して繰り返す
6. [タイルセット書き出し] で1枚のPNG画像をエクスポート
```

### 6.2 既存PNGからのタイルセット読み込み

```
1. [タイルセットを開く] で既存PNGを選択
2. チップサイズ指定ダイアログが表示される
3. チップサイズを入力（画像サイズから列・行数を自動算出）
4. タイルセットモードで開かれ、チップ単位の編集が可能になる
```

### 6.3 メイン編集領域でのチップ操作

タイルセットモード時のメインキャンバスは**選択中チップの内容のみ**を表示する（1チップ = 1キャンバス）。
チップの切り替えはチップパレットウィンドウで行う。

- **表示対象:** `appData.layerData`（= 選択チップの LayerData）の `composite()` 結果
- **描画操作:** 既存ツール（pencil/eraser/fill/eyedropper）がそのまま動作。チップサイズのキャンバスに対する操作になるため、チップ境界制約は不要
- **レイヤーパネル:** 選択チップのレイヤー構成が自動表示される
- **チップ切り替え:** チップパレットで別チップをクリックすると `selectedChip` が更新され、メイン編集領域とレイヤーパネルが自動的に切り替わる

---

## 7. PixelCanvas の変更

チップ独立レイヤー方式では、メイン編集領域は常に**1つのチップの LayerData composite 結果**を表示する。そのため `PixelCanvas` 本体への変更は不要。

既存の動作:
- `EditorScene.render()` が `appData.layerData.composite()` を `PixelCanvas.render()` に渡す
- `appData.layerData` getter がモードに応じて選択チップの LayerData を返す
- → **PixelCanvas は何も変更せずにチップの編集ビューとして機能する**

### 7.1 チップパレットでの全体表示

タイルセット全体のグリッド表示はチップパレットウィンドウ (`ChipPaletteWindow`) が担当する。

- `TilesetData.compositeAll()` で生成した全体画像をサムネイル表示
- チップ境界のグリッド線はチップパレット内で描画
- 選択中チップのハイライト（枠線）もチップパレット内で描画

これにより PixelCanvas の責務はシンプルに保たれ、チップ固有のグリッド描画ロジックが不要になる。

---

## 8. ファイルフォーマット

### 8.1 タイルセットPNG出力（標準）

チップを格子状に配置した1枚のPNG画像。RPGツクール・Tiled・Unity などの汎用形式と互換。

```
出力画像サイズ: (chipWidth × columns) × (chipHeight × rows)
配置順: 左上 → 右 → 改行 → 左 → 右 …（ラスタスキャン順）
```

### 8.2 タイルセットJSON保存形式（プロジェクト保存）

チップごとのレイヤー情報を含む独自JSON形式。各チップが独立したレイヤー構成を保持できる。

```json
{
  "version": 2,
  "type": "tileset",
  "chipWidth": 32,
  "chipHeight": 32,
  "columns": 8,
  "rows": 4,
  "chips": [
    {
      "col": 0, "row": 0,
      "layers": [
        {
          "name": "下地",
          "visible": true,
          "opacity": 255,
          "data": "<base64 encoded Uint32Array>"
        },
        {
          "name": "ディテール",
          "visible": true,
          "opacity": 255,
          "data": "<base64>"
        }
      ]
    },
    {
      "col": 1, "row": 0,
      "layers": [
        {
          "name": "レイヤー 1",
          "visible": true,
          "opacity": 255,
          "data": "<base64>"
        }
      ]
    }
  ],
  "palette": ["#000000", "#FFFFFF", "..."]
}
```

**注:** レイヤーが1枚の空チップはデータを省略可能（読み込み時にデフォルト生成）。

### 8.3 個別チップPNG出力

選択中のチップの `composite()` 結果を `chipWidth × chipHeight` サイズのPNGとして書き出す。
レイヤー合成は各チップの `LayerData.composite()` がそのまま使える。

---

## 9. 実装フェーズ

### Phase 1: タイルセットデータモデルとグリッド表示（基盤）

**目標:** タイルセットとして画像を管理する基盤を構築する。

| # | 作業内容 | 対象ファイル | 進捗 |
|---|----------|-------------|------|
| 1-1 | `TilesetData` クラス新規作成 | `js/tileset_data.js` | ✅ 完了 |
| 1-2 | `AppData` に `tilesetData` / `editMode` / `selectedChip` 追加 | `js/app_data.js` | ✅ 完了 |
| 1-3 | `PixelCanvas` にチップグリッド描画を追加 | `js/pixel_canvas.js` | ※ v2設計で不要 |
| 1-4 | `PixelCanvas` に選択チップハイライト描画を追加 | `js/pixel_canvas.js` | ※ v2設計で不要 |
| 1-5 | `NewTilesetDialog` 新規作成 | `js/ui/dialog/new_tileset_dialog.js` | ✅ 完了 |
| 1-6 | メニューに「新規タイルセット作成」追加 | `js/ui/menu_constants.js`, `js/ui/menu_bar.js` | ✅ 完了 |
| 1-7 | `EditorScene` にタイルセットモードの初期化処理追加 | `js/scene/editor_scene.js` | ✅ 完了 |

### Phase 2: チップパレットと選択操作

**目標:** チップの一覧表示・選択・フォーカスができるようにする。

| # | 作業内容 | 対象ファイル | 進捗 |
|---|----------|-------------|------|
| 2-1 | `ChipPalette` コンテンツクラス新規作成 | `js/ui/chip_palette.js` | ✅ 完了 |
| 2-2 | `ChipPaletteWindow` 新規作成 | `js/ui/chip_palette_window.js` | ✅ 完了 |
| 2-3 | チップクリック選択の実装 | `js/scene/editor_scene.js` | ✅ 完了 |
| 2-4 | チップダブルクリックでフォーカスズーム | `js/scene/editor_scene.js` | ✅ 完了 |
| 2-5 | メインキャンバスでのチップ選択操作 | `js/scene/editor_scene.js` | ✅ 完了 |

### Phase 3: チップ単位操作

**目標:** チップのコピー・ペースト・入れ替え・クリアを実装する。

| # | 作業内容 | 対象ファイル | 進捗 |
|---|----------|-------------|------|
| 3-1 | `TilesetData` にチップ操作メソッド実装 | `js/tileset_data.js` | ✅ 完了 |
| 3-2 | チップクリップボード（AppData にバッファ追加） | `js/app_data.js` | ✅ 完了 |
| 3-3 | タイルセットメニュー追加 | `js/ui/menu_constants.js`, `js/ui/menu_bar.js` | ✅ 完了 |
| 3-4 | メニューコマンド接続 | `js/scene/editor_scene.js` | ✅ 完了 |
| 3-5 | チップ選択切り替え時のビューリセット処理 | `js/scene/editor_scene.js` | ✅ 完了 |

### Phase 4: タイルプレビュー

**目標:** 選択チップのタイリング表示を実装する。

| # | 作業内容 | 対象ファイル | 進捗 |
|---|----------|-------------|------|
| 4-1 | `TilePreview` コンテンツクラス新規作成 | `js/ui/tile_preview.js` | 未着手 |
| 4-2 | `TilePreviewWindow` 新規作成 | `js/ui/tile_preview_window.js` | 未着手 |
| 4-3 | プレビューサイズ切り替え | `js/ui/tile_preview.js` | 未着手 |

### Phase 5: インポート・エクスポート拡張

**目標:** タイルセット形式での入出力を実装する。

| # | 作業内容 | 対象ファイル | 進捗 |
|---|----------|-------------|------|
| 5-1 | タイルセットPNGエクスポート（合成→書き出し） | `js/util/pixel_data_converter.js` | 未着手 |
| 5-2 | 個別チップPNGエクスポート | `js/util/pixel_data_converter.js` | 未着手 |
| 5-3 | タイルセットJSON保存形式 v2 | `js/util/pixel_data_converter.js` | 未着手 |
| 5-4 | 既存PNGからのタイルセット読み込みダイアログ | `js/ui/dialog/import_tileset_dialog.js` | 未着手 |
| 5-5 | メニュー接続 | `js/scene/editor_scene.js` | 未着手 |

### Phase 6: 品質向上・追加機能（後回し可）

| # | 作業内容 | 進捗 |
|---|----------|------|
| 6-1 | Undo/Redo にチップ操作コマンドを追加 | 未着手 |
| 6-2 | チップパレットでのドラッグ&ドロップ入れ替え | 未着手 |
| 6-3 | タイルセットサイズ変更（行列追加・削除） | 未着手 |
| 6-4 | アニメーションタイル管理（フレーム列指定） | 未着手 |
| 6-5 | チップへのタグ・名前付け | 未着手 |
| 6-6 | 隣接チップを並べた9パッチプレビュー | 未着手 |

---

## 10. 既存コードへの影響範囲

### 10.1 変更が必要なファイル

| ファイル | 変更内容 |
|----------|----------|
| `js/app_data.js` | `layerData` を getter 化、`_layerData` / `tilesetData` / `editMode` / `selectedChip` 追加 |
| `js/ui/menu_constants.js` | タイルセットメニューID定数追加 |
| `js/ui/menu_bar.js` | タイルセットメニュー追加 |
| `js/scene/editor_scene.js` | タイルセットモード分岐・チップ選択・PNGエクスポート分岐・メニュー接続 |
| `js/util/pixel_data_converter.js` | タイルセットJSON v2 対応 |
| `www/index.html` | 新規スクリプトの読み込み追加 |

### 10.1.1 変更不要なファイル（チップ独立レイヤー方式の利点）

| ファイル | 理由 |
|----------|------|
| `js/pixel_canvas.js` | `composite()` 結果を受け取るだけ。チップグリッド描画はチップパレットが担当 |
| `js/layer_data.js` | そのまま各チップのレイヤー管理に再利用 |
| `js/pixel_data.js` | 変更なし |
| `js/ui/layer_panel.js` | `appData.layerData` を参照するだけ（getter が自動切り替え） |
| `js/ui/tool_bar.js` | 変更なし |
| `js/ui/color_palette*.js` | 変更なし |
| `js/ui/dialog/color_picker_dialog.js` | 変更なし |

### 10.2 新規作成ファイル

| ファイル | 内容 |
|----------|------|
| `js/tileset_data.js` | TilesetData クラス |
| `js/ui/chip_palette.js` | チップパレットコンテンツ |
| `js/ui/chip_palette_window.js` | チップパレットウィンドウ |
| `js/ui/tile_preview.js` | タイルプレビューコンテンツ |
| `js/ui/tile_preview_window.js` | タイルプレビューウィンドウ |
| `js/ui/dialog/new_tileset_dialog.js` | タイルセット新規作成ダイアログ |
| `js/ui/dialog/import_tileset_dialog.js` | タイルセットインポートダイアログ |

---

## 11. 設計方針のまとめ

1. **チップごとに独立した LayerData を持つ。** `TilesetData.chips[row][col]` が各チップの `LayerData` を保持し、チップごとに自由なレイヤー構成が可能。草チップは1レイヤー、建物チップは3レイヤーといった柔軟な使い方ができる。

2. **`AppData.layerData` getter による透過的切り替え。** tileset モード時は選択チップの `LayerData` を返し、free モード時は従来の `_layerData` を返す。これにより `PixelCanvas` / `LayerPanel` / 描画ツール群は無改修で動作する。

3. **`TilesetData` はチップ管理とタイルセット構造を担当。** チップサイズ・列行数の管理、チップ間コピー・入れ替え、`compositeAll()` による全体画像組み立てを提供する。ピクセル操作は各チップの `LayerData` に委譲する。

4. **自由編集モードとの共存。** `editMode` フラグで従来のドット絵エディタとして使うか、タイルセットモードで使うかを切り替える。モード切り替えは非破壊的。

5. **UIWindow パターンの踏襲。** 新しいウィンドウ（チップパレット・プレビュー）はすべて既存の `UIWindow` 基底クラスを継承して作成する。

6. **段階的実装。** Phase 1（データモデル＋チップパレット）だけで最低限の編集が可能になる構成にし、以降のフェーズで UI と操作性を段階的に向上させる。
