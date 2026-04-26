# カスタムバイナリフォーマット（.qts）実装計画

最終更新: 2026-03-22

---

## 1. 概要

現状の PNG / JSON(Base64) 保存に加え、**ゲーム組み込みを前提とした独自バイナリフォーマット**（`.qts`）を実装する。

### 設計方針

- 1ピクセル = 1バイト（メモリ効率重視）
- カラーパレット最大32色（色ID 0 = 透明予約、有効色ID 1〜31）
- パレットはファイル内にメタデータとして保存し、エディタ上で編集可能にする
- ゲーム用メタ情報（通過可能フラグ等）をピクセルビットに格納
- 多レイヤー対応（レイヤー単位で1バイト形式を保存）
- 既存の PNG / JSON 保存機能は維持し、`.qts` を追加形式として加える

補足:

- 通常ドット絵は `columns=1, rows=1` の QTS として保存してよい
- 読み込み時に `1x1` の場合は free mode の単体画像として復元してよい

---

## 2. ファイルフォーマット仕様（`.qts`）

### 2.0 クロスプラットフォーム規則（重要）

#### エンディアン

- **ファイル内の全マルチバイト整数はリトルエンディアン（LE）で格納する。**
- 単一バイトフィールド（uint8, 1バイトピクセル, パレット成分 R/G/B/A）は
  バイト順序に依存しないため、そのまま読み書きしてよい。
- マルチバイトが登場するフィールドは以下の 2 箇所のみ:
  - タイルセットメタ情報: `chipWidth / chipHeight / columns / rows`（各 uint16 LE）
  - チップブロック内レイヤーメタ: レイヤー名バイト長 `L`（uint16 LE）

#### 実装ガイドライン

**JavaScript（読み書き双方）**

`Uint16Array` を `ArrayBuffer` のビュー上に直接マップすると、
実行環境のネイティブエンディアンをそのまま使用してしまう。
現在のほぼ全デスクトップ/モバイル環境はリトルエンディアンだが、
**将来の移植性のために必ず `DataView` + 明示的な `littleEndian=true` フラグを使うこと。**

```javascript
// 書き込み例（オフセット pos に uint16 LE で値を書く）
const view = new DataView(buffer);
view.setUint16(pos, value, /*littleEndian=*/true);

// 読み込み例
const value = view.getUint16(pos, /*littleEndian=*/true);
```

**C言語（組み込み・サーバーサイド）**

```c
/* リトルエンディアンで uint16 を書き出す（エンディアン非依存） */
static void write_u16le(uint8_t *buf, uint16_t v) {
    buf[0] = (uint8_t)(v & 0xFF);
    buf[1] = (uint8_t)(v >> 8);
}

/* リトルエンディアンで uint16 を読み込む（エンディアン非依存） */
static uint16_t read_u16le(const uint8_t *buf) {
    return (uint16_t)(buf[0]) | ((uint16_t)(buf[1]) << 8);
}
```

> `memcpy` や `*(uint16_t*)buf` のキャストでの読み書きはビッグエンディアン環境で
> 誤動作するため**禁止**。必ず上記関数を経由すること。

#### 文字コード

- レイヤー名は **UTF-8** エンコードで格納する。
- 日本語・絵文字等のマルチバイト文字を含む場合、**バイト長 L は文字数ではなくバイト数**。
- JavaScript では `TextEncoder` / `TextDecoder` を使用:

```javascript
// エンコード: string → Uint8Array
const encoded = new TextEncoder().encode(layerName);
const L = encoded.byteLength;  // ← これを uint16 LE で書く

// デコード: Uint8Array → string
const layerName = new TextDecoder().decode(bytes.subarray(pos, pos + L));
```

- **NULL 終端は含めない**（バイト長 L で範囲を管理するため不要）。

#### バイトオーダー依存フィールド一覧

| フィールド              | バイト数 | 型      | エンディアン依存 |
|------------------------|---------|---------|----------------|
| chipWidth              | 2       | uint16  | **LE 必須**    |
| chipHeight             | 2       | uint16  | **LE 必須**    |
| columns                | 2       | uint16  | **LE 必須**    |
| rows                   | 2       | uint16  | **LE 必須**    |
| レイヤー名バイト長 L   | 2       | uint16  | **LE 必須**    |
| その他全フィールド     | 1 each  | uint8   | 非依存（1バイト）|

---

### 2.1 全体構造

```
[ファイルヘッダ]             8 バイト（固定）
[タイルセットメタ情報]       8 バイト（固定）
[パレットブロック]           129 バイト（固定）
[チップブロック×(cols×rows)]  可変長
  各チップ:
    [レイヤー数]             1 バイト
    [レイヤー×N]
      [レイヤーメタ]         可変長（名前長 + 名前 + 可視 + 不透明度）
      [ピクセルデータ]       chipW × chipH バイト
```

チップは行優先（row-major）順で格納する: `chips[0][0], chips[0][1], ..., chips[rows-1][cols-1]`

---

### 2.2 ファイルヘッダ（8 バイト）

| オフセット | バイト数 | 内容                                        |
|-----------|---------|---------------------------------------------|
| 0x00      | 4       | マジックナンバー `0x51 0x53 0x54 0x53`（"QSTS"）|
| 0x04      | 1       | メジャーバージョン（`0x01`）                |
| 0x05      | 1       | マイナーバージョン（`0x00`）                |
| 0x06      | 2       | 予約（`0x00 0x00`）                         |

---

### 2.3 タイルセットメタ情報（8 バイト）

全フィールドはリトルエンディアン（→ セクション 2.0 参照）。

| オフセット | バイト数 | 型            | 内容                 |
|-----------|---------|--------------|----------------------|
| +0        | 2       | uint16 **LE** | チップ幅 (chipWidth)  |
| +2        | 2       | uint16 **LE** | チップ高さ (chipHeight)|
| +4        | 2       | uint16 **LE** | 列数 (columns)       |
| +6        | 2       | uint16 **LE** | 行数 (rows)          |

---

### 2.4 パレットブロック（129 バイト・固定長）

| オフセット | バイト数 | 内容                                              |
|-----------|---------|---------------------------------------------------|
| +0        | 1       | 有効色数 N（1〜32。インデックス0の透明色も含む） |
| +1〜+128  | 32×4    | 各色 RGBA（各1バイト、順: R, G, B, A）            |

**注意事項:**
- インデックス0（透明色）は A=0 固定。R/G/B は将来拡張向けに書き込むが読み込み時は無視する。
- 未使用スロット（index >= N）は 0 埋め。
- ブロックサイズは常に 129 バイト固定（N の変更による構造ずれがない）。

---

### 2.5 チップブロック（可変長）

#### チップ先頭

| オフセット | バイト数 | 型     | 内容          |
|-----------|---------|-------|---------------|
| +0        | 1       | uint8 | レイヤー数 N  |

#### レイヤーメタ（レイヤーごと）

| オフセット | バイト数 | 型            | 内容                                        |
|-----------|---------|--------------|---------------------------------------------|
| +0        | 2       | uint16 **LE** | レイヤー名バイト長 L（**バイト数**、文字数ではない）|
| +2        | L       | UTF-8        | レイヤー名文字列（NULL 終端なし）           |
| +2+L      | 1       | uint8        | 可視フラグ（1=表示, 0=非表示）              |
| +3+L      | 1       | uint8        | 不透明度（0〜255）                          |

> L が 0 の場合はレイヤー名なし（+2 に名前バイトは存在しない）。

#### ピクセルデータ（レイヤーごと）

`chipWidth × chipHeight` バイト。各バイトの構造は次節参照。

---

## 3. 1バイトピクセル構造

```
 7   6   5   4   3   2   1   0
┌───┬───┬───┬───┬───┬───┬───┬───┐
│ P │ R │ R │ C │ C │ C │ C │ C │
└───┴───┴───┴───┴───┴───┴───┴───┘
  P: 通過フラグ（1ビット）
     1 = 通過可能, 0 = 通過不可
  R: 予約ビット（2ビット）
     Bit 6: 予約①（将来の当たり判定拡張用、現在は 0）
     Bit 5: 予約②（将来のイベントフラグ用、現在は 0）
  C: カラーID（5ビット、0〜31）
     0 = 透明（描画しない）
     1〜31 = パレットインデックス
```

### ビット操作定義

**C 言語:**
```c
/* カラーID取得 (0〜31) */
#define PIXEL_COLOR_ID(b)    ((b) & 0x1F)
/* 通過フラグ取得 (0 or 1) */
#define PIXEL_PASSABLE(b)    (((b) >> 7) & 0x01)
/* 予約ビット取得 (0〜3) */
#define PIXEL_RESERVED(b)    (((b) >> 5) & 0x03)
/* バイト生成 */
#define PIXEL_MAKE(colorId, passable, reserved) \
    (((passable) << 7) | ((reserved) << 5) | ((colorId) & 0x1F))
```

**JavaScript:**
```javascript
const PIXEL_COLOR_ID = (b) => b & 0x1F;
const PIXEL_PASSABLE = (b) => (b >> 7) & 0x01;
const PIXEL_RESERVED = (b) => (b >> 5) & 0x03;
const PIXEL_MAKE     = (colorId, passable = 0, reserved = 0) =>
    ((passable & 1) << 7) | ((reserved & 3) << 5) | (colorId & 0x1F);
```

---

## 4. カラーパレット拡張設計

### 4.1 新クラス: `EditablePalette32`

既存の `ColorPalette16`（固定16色）を置き換え、**編集可能な32色パレット**を実装する。

```javascript
class EditablePalette32 extends ColorPalette {
  constructor()              // 初期32色で初期化（インデックス0=透明固定）
  getColors()                // 現在の色配列（32要素）を返す
  getPaletteName()           // 'Palette (32)'
  setColor(index, color)     // インデックス指定で色を変更（index=0は拒否）
  getColor(index)            // 指定インデックスの色を返す
  resetToDefaults()          // 初期32色にリセット
  clone()                    // ディープコピーを返す
}
```

**初期32色の構成:**

| インデックス | 内容                            |
|------------|--------------------------------|
| 0          | 透明（固定・編集不可）           |
| 1          | 黒                              |
| 2          | 濃いグレー                      |
| 3          | 薄いグレー                      |
| 4          | 白                              |
| 5          | 暗赤                            |
| 6          | 赤                              |
| 7          | オレンジ                        |
| 8          | 黄                              |
| 9          | 暗緑                            |
| 10         | 黄緑                            |
| 11         | 暗シアン                        |
| 12         | シアン                          |
| 13         | 暗青                            |
| 14         | 青                              |
| 15         | 紫                              |
| 16         | マゼンタ                        |
| 17〜31      | 空き（初期値: 透明 `0x00000000`）|

### 4.2 AppData の変更

```javascript
class AppData {
  constructor() {
    // 新規追加
    this.palette = new EditablePalette32();
    // (既存フィールドはすべて維持)
  }
}
```

### 4.3 パレット UI の変更（`ColorPalette` 基底クラス拡張）

- セル上で**ダブルクリック** → `ColorPickerDialog` を開いてパレット色を編集
- インデックス0（透明）は `ColorPickerDialog` を開かず変更不可
- `ColorPalette` 基底クラスに `onCellDoubleClick` コールバックを追加:

```javascript
/**
 * @type {((index: number, currentColor: number, callback: (newColor: number) => void) => void)|null}
 */
this.onCellDoubleClick = null;
```

- `EditorScene` でダブルクリック → `ColorPickerDialog.showWithColor()` → `appData.palette.setColor()` の橋渡しを実装

---

## 5. 変換ロジック

### 5.1 ARGB32 → 1バイト（量子化）

```javascript
/**
 * 1ピクセル(ARGB32) を最近傍探索でパレットインデックスに変換し、1バイトを生成する。
 * @param {number}   argb32   0xAARRGGBB
 * @param {number[]} palette  EditablePalette32.getColors() の結果（32要素）
 * @param {number}   [passable=0]  通過フラグ
 * @returns {number} 0〜255 の1バイト
 */
static quantizePixel(argb32, palette, passable = 0) {
  // アルファ0(透明)はカラーID 0
  if (((argb32 >>> 24) & 0xFF) === 0) return PIXEL_MAKE(0, 0, 0);

  // RGB各成分を取り出してユークリッド距離最近傍を探す
  const r = (argb32 >>> 16) & 0xFF;
  const g = (argb32 >>>  8) & 0xFF;
  const b =  argb32         & 0xFF;

  let bestId   = 1;
  let bestDist = Infinity;
  for (let i = 1; i < palette.length; i++) {
    if (((palette[i] >>> 24) & 0xFF) === 0) continue; // 空きスロットはスキップ
    const pr = (palette[i] >>> 16) & 0xFF;
    const pg = (palette[i] >>>  8) & 0xFF;
    const pb =  palette[i]         & 0xFF;
    const d  = (r - pr) ** 2 + (g - pg) ** 2 + (b - pb) ** 2;
    if (d < bestDist) { bestDist = d; bestId = i; }
  }
  return PIXEL_MAKE(bestId, passable);
}
```

### 5.2 1バイト → ARGB32（展開）

```javascript
/**
 * 1バイトをARGB32に展開する。
 * @param {number}   byte     1バイトピクセル値
 * @param {number[]} palette  EditablePalette32.getColors() の結果
 * @returns {number} 0xAARRGGBB
 */
static expandPixel(byte, palette) {
  const colorId = PIXEL_COLOR_ID(byte);
  if (colorId === 0) return 0x00000000; // 透明
  const c = palette[colorId];
  if (!c || ((c >>> 24) & 0xFF) === 0) return 0x00000000;
  return (c & 0x00FFFFFF) | 0xFF000000; // A=255 固定
}
```

---

## 6. 実装タスク一覧

### Phase 1 — 編集可能パレット基盤

- [x] **`EditablePalette32` クラス新規作成** (`js/ui/editable_palette_32.js`)
  - `ColorPalette` 継承
  - 32色配列管理（インデックス0=透明固定）
  - `setColor(index, color)` / `getColor(index)` / `resetToDefaults()` / `clone()`

- [x] **`ColorPalette` 基底クラスに `onCellDoubleClick` コールバック追加**
  - `onMouseDown` でダブルクリック判定（最終クリック時刻との差分）
  - インデックス0はコールバックを呼ばない

- [x] **`AppData.palette` プロパティ追加**
  - 型は `EditablePalette32`
  - `ColorPaletteWindow` が `appData.palette` を参照するよう変更

- [x] **`EditorScene` でパレット編集フロー実装**
  - `palette.onCellDoubleClick` → `ColorPickerDialog.showWithColor()` 起動
  - ピッカー確定 → `appData.palette.setColor(index, color)` 呼び出し

- [x] **`index.html` に `editable_palette_32.js` の読み込みを追加**
  - `color_palette_16.js` の後に追加

### Phase 2 — バイナリフォーマット入出力

- [x] **`PixelDataConverter` に `.qts` エクスポートを追加**
  - `static exportTilesetAsQts(tilesetData, palette, filename)` 実装
  - `DataView` を使った書き込み（エンディアン安全）:
    ```javascript
    const view = new DataView(buffer);
    view.setUint16(offset, value, true); // true = little-endian
    ```
  - レイヤー名は `TextEncoder` でエンコードし、バイト長を uint16 LE で書く
  - `Uint8Array` でバッファ構築:
    1. ヘッダ書き込み（8バイト）
    2. メタ情報書き込み（8バイト、uint16 LE × 4）
    3. パレットブロック書き込み（129バイト、全て uint8 なので非依存）
    4. チップループ（行×列）: レイヤーメタ + 量子化ピクセルデータ
  - `new Blob([buffer], { type: 'application/octet-stream' })` でダウンロード

- [x] **`PixelDataConverter` に `.qts` インポートを追加**
  - `static importFromQts(file)` → `{ tilesetData: TilesetData, palette: EditablePalette32 }`
  - `DataView` を使った読み込み（エンディアン安全）:
    ```javascript
    const view = new DataView(buffer);
    const chipW = view.getUint16(offset, true); // true = little-endian
    ```
  - レイヤー名は `TextDecoder` でデコード
  - マジックナンバー "QSTS" 検証（不正なら `reject`）
  - メタ情報読み込み → `TilesetData` 構築
  - パレットブロック読み込み → `EditablePalette32` 構築
  - チップループ: ピクセル展開 → `LayerData` 構築

- [x] **量子化・展開ユーティリティ実装**
  - `static quantizePixel(argb32, paletteColors, passable)` — セクション5.1参照
  - `static expandPixel(byte, paletteColors)` — セクション5.2参照

- [x] **`SaveDialog` に `.qts` 形式選択肢追加**
  - ラジオボタンに `'qts'` を追加
  - ダイアログ高さを調整（ラジオが3択に増える）
  - コールバック型: `(filename: string, format: 'png' | 'json' | 'qts') => void`

- [x] **`EditorScene` の `FILE_SAVE` 処理で `.qts` 対応**
  - `format === 'qts'` の分岐を追加
  - `PixelDataConverter.exportTilesetAsQts(appData.tilesetData, appData.palette, filename)`
  - タイルセットモード以外で `.qts` を選んだ場合はアラートまたは無効化

- [x] **`EditorScene` のファイルインポート処理で `.qts` 対応**
  - `_fileInput.accept` に `.qts` を追加
  - `.qts` ファイル読み込み時は `importFromQts(file)` を呼び出し
  - 取得した `palette` を `appData.palette` に適用

- [x] **`importFromFile` に `.qts` 分岐追加**
  - 拡張子 `.qts` → `importFromQts(file)` へルーティング

### Phase 3 — 通過フラグ編集 UI（後回し・オプション）

- [x] チップパレット上で右クリック → コンテキストメニューに「通過フラグ: ON/OFF」表示
- [x] `TilesetData` にチップ単位の通過フラグ配列 `passFlags[row][col]` を追加
- [x] `.qts` 保存時にチップの通過フラグをピクセルの P ビットに反映
- [x] `.qts` 読み込み時に P ビットから通過フラグを復元し `passFlags` に格納

---

## 7. ファイル変更影響範囲

| ファイル                                    | 変更種別 | 内容                                                  |
|-------------------------------------------|---------|-------------------------------------------------------|
| `js/ui/editable_palette_32.js`             | **新規** | `EditablePalette32` クラス                            |
| `js/ui/color_palette.js`                   | **修正** | `onCellDoubleClick` コールバック・ダブルクリック判定追加 |
| `js/app_data.js`                           | **修正** | `palette: EditablePalette32` プロパティ追加           |
| `js/util/pixel_data_converter.js`          | **修正** | `.qts` エクスポート/インポート・量子化/展開ユーティリティ追加 |
| `js/ui/dialog/save_dialog.js`              | **修正** | `.qts` 形式選択肢追加、`format` 型拡張                 |
| `js/scene/editor_scene.js`                 | **修正** | パレット編集フロー・`.qts` 保存・インポート対応        |
| `index.html`                               | **修正** | `editable_palette_32.js` スクリプト読み込み追加       |

---

## 8. バイナリサイズ試算

チップサイズ 32×32、8列×8行（64チップ）、1レイヤーの場合:

```
ヘッダ:            8 バイト
メタ:              8 バイト
パレット:        129 バイト
チップデータ:
  64チップ × (1バイト[レイヤー数] + 6バイト[レイヤーメタ概算] + 32×32バイト[ピクセル])
  = 64 × 1031 ≒ 65,984 バイト
合計:          ≒ 66 KB
```

同等の ARGB32 PNG（無圧縮時）が `64 × 32 × 32 × 4 ≒ 262 KB` なのに対し、**約1/4のサイズ**を達成。

---

## 9. 設計上の注意事項

1. **量子化の不可逆性**: ARGB32 → 1バイト変換は近似。編集中は常に ARGB32 内部表現を維持し、エクスポート時のみ量子化する。`.qts` 読み込み後に再編集してもパレットにない中間色は使えない。

2. **既存フォーマットとの共存**: PNG/JSON 機能は一切変更しない。`.qts` はタイルセットモード専用フォーマットとして位置づける。

3. **通過フラグの初期値**: エクスポート時、チップ単位の通過フラグ未設定の場合は「全ピクセル通過可（P=1）」をデフォルトとする。

4. **レイヤー合成 vs レイヤー個別保存**: `.qts` ではレイヤーを個別に保存する（合成後ではなく）。読み込み時に各レイヤーの PixelData を展開し、LayerData に積み直す。

5. **マジックナンバー**: "QSTS"（0x51, 0x53, 0x54, 0x53）で誤ったファイルの読み込みを防ぐ。

6. **インデックス0（透明）の扱い**: パレット編集 UI でインデックス0のセルは `ColorPickerDialog` を開かない。常に透明色固定。

7. **エンディアン安全の実装**: 全てのマルチバイト整数（uint16）の読み書きはセクション 2.0 に記載した方法を厳守する。JavaScript では `DataView`、C では `read_u16le` / `write_u16le` 関数を必ず経由し、直接キャスト（`*(uint16_t*)ptr`）や `Uint16Array` のネイティブビューは使用しない。

8. **UTF-8 バイト長の扱い**: レイヤー名の uint16 フィールドは**文字数ではなくバイト数**を格納する（ASCII以外の文字は1文字が複数バイト）。L の最大値は 65535 バイトで、実用上は 255 バイト程度に収まる名前を推奨する。

9. **バッファサイズの事前計算**: エクスポート時はチップ数・レイヤー数・レイヤー名バイト長から総バイト数を事前に計算して `ArrayBuffer` を一括確保する。動的な `push` / 結合を避けることでパフォーマンスと安全性を確保する。
