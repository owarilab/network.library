# クォータービュータイル自動生成 — 機能仕様書

最終更新: 2026-03-08

---

## 1. 機能概要

クォータービュー（2:1 斜め見下ろし）ゲームで使うタイルの雛形を自動生成する機能。
「生成」メニューからダイアログを開き、パラメータを指定するとキャンバスに
タイル輪郭・ガイド色を塗り込んだ `PixelData` を作成する。

---

## 2. クォータービューのタイル形状

### 2.1 座標定義

キャンバスサイズ `W × H`、上端を y=0 として以下を定義する。

```
H_diamond = W / 2       （ダイアモンド高さ; 2:1 比率）
H_wall    = 設定値      （壁面高さ; 箱タイルのみ）
H         = H_diamond + H_wall
```

#### 地面タイル (type = ground)

```
         (W/2, 0)
        ／       ＼
(0, W/4)           (W-1, W/4)
        ＼       ／
         (W/2, W/2-1)
```

- キャンバスサイズ: `W × (W/2)`
- ダイアモンド 1 面のみ描画

#### 箱タイル (type = box)

```
            (W/2, 0)            ← 上頂点
           ／       ＼
   (0, W/4)           (W-1, W/4)     ← 左/右頂点
           ＼       ／
            (W/2, W/2-1)        ← 下頂点（上面底辺の中点）
           ／       ＼
  (0, W/4+H_wall-1)   (W-1, W/4+H_wall-1)  ← 壁面下端
           ＼       ／
            (W/2, W/2+H_wall-1) ← 全体の最下点
```

- キャンバスサイズ: `W × (W/2 + H_wall)`
- 上面（ダイアモンド）+ 左壁面（平行四辺形）+ 右壁面（平行四辺形）を描画

### 2.2 各頂点の具体値（W=32 の例）

| タイル | H_wall | キャンバス | 上面底点 | 左壁面下 | 右壁面下 |
|--------|--------|-----------|----------|----------|----------|
| ground | —      | 32×16     | (16,15)  | —        | —        |
| box    | 8      | 32×24     | (16,15)  | (0,23)   | (31,23)  |
| box    | 16     | 32×32     | (16,15)  | (0,31)   | (31,31)  |

### 2.3 各面の座標リスト

以下では `cx = W/2`, `ry = W/4`（ラテラル頂点のY）, `by = 2*ry - 1`（底辺Y） と表記。

| 面          | 頂点（時計回り）                                              |
|-------------|---------------------------------------------------------------|
| 上面        | `(cx,0)` → `(W-1,ry)` → `(cx, by)` → `(0,ry)`               |
| 左壁面      | `(0,ry)` → `(cx,by)` → `(cx,by+H_wall)` → `(0,ry+H_wall)`   |
| 右壁面      | `(cx,by)` → `(W-1,ry)` → `(W-1,ry+H_wall)` → `(cx,by+H_wall)` |

---

## 3. ダイアログ仕様

### 3.1 クラス名・ファイル

| 項目 | 内容 |
|------|------|
| クラス | `QuarterViewTileDialog` |
| ファイル | `js/ui/dialog/quarter_view_tile_dialog.js` |
| 継承元 | `DialogBase` |
| ダイアログサイズ | 約 440 × 400 px |

### 3.2 UI 項目

#### (A) タイプ選択（ラジオ）

| ラベル | 値 | 説明 |
|--------|----|------|
| 地面タイル | `ground` | ダイアモンド面のみ |
| 箱タイル   | `box`    | ダイアモンド + 左右壁面（デフォルト） |

#### (B) サイズプリセット（ボタン群）

タイプ `ground` のとき:

| ラベル | W | H |
|--------|---|---|
| 16×8   | 16 | 8  |
| 32×16  | 32 | 16 |
| 48×24  | 48 | 24 |
| 64×32  | 64 | 32 |

タイプ `box` のとき:

| ラベル  | W  | H_wall | H  |
|---------|----|--------|----|
| 16×16   | 16 | 8      | 16 |
| 32×24   | 32 | 8      | 24 |
| 32×32   | 32 | 16     | 32 |
| 64×48   | 64 | 16     | 48 |
| 64×64   | 64 | 32     | 64 |

#### (C) カスタムサイズ

- 幅スピナー（4の倍数のみ、4〜512、ステップ 4）
- 壁の高さスピナー（boxタイプのみ、1〜256）
- 高さは自動計算して表示（読み取り専用）

#### (D) レイヤー分け（チェックボックス）

| ラベル | 説明 |
|--------|------|
| レイヤーを分ける | OFF=1レイヤーに全描画 / ON=面ごとに別レイヤー（box は3層） |

レイヤー名（ON 時）:
- 地面: `上面`
- 箱: `右面` / `左面` / `上面`（layers[0]→[2] の順; [0]=最背面, [2]=最前面）

> LayerData は `layers[0]` が最背面のため、描画奥行き順に右面→左面→上面の順で格納する。

#### (E) 色設定

| スウォッチ   | 初期色              | 説明 |
|-------------|---------------------|------|
| 上面         | `0xFFC8C8C8`（明るいグレー） | 上面塗りつぶし色 |
| 左面         | `0xFF808080`（中グレー）     | 左壁面塗りつぶし色 |
| 右面         | `0xFF606060`（暗いグレー）   | 右壁面塗りつぶし色 |
| 輪郭         | `0xFF000000`（黒）           | 輪郭線色 |

各スウォッチクリックで `ColorPickerDialog` を開いて色変更可能。

#### (F) 透明化オプション（チェックボックス）

| ラベル | 説明 |
|--------|------|
| 上面を透明にする | 上面の塗りつぶしを 0x00000000 にする（輪郭は残る） |
| 左面を透明にする | 左壁面の塗りつぶしを透明にする |
| 右面を透明にする | 右壁面の塗りつぶしを透明にする |

#### (G) 適用先（ラジオ）

| ラベル | 値 | 説明 |
|--------|----|------|
| 新規作成 | `new` | 既存データを破棄して新規 `LayerData` として生成（デフォルト） |
| 現在のキャンバスに上書き | `overwrite` | 既存 `layerData` にそのまま描画（サイズ変更なし） |

#### (H) ボタン

- キャンセル（Escape, `onCancel` コールバック）
- 生成（Enter, `onConfirm(params)` コールバック）

#### (I) プレビュー

- ダイアログ右側 or 中央下部に 96×96 px 程度のプレビュー領域
- 生成されるタイルをリアルタイムで描画（設定変更のたびに更新）

---

## 4. ジェネレーター仕様

### 4.1 クラス名・ファイル

| 項目 | 内容 |
|------|------|
| クラス | `QuarterViewTileGenerator` |
| ファイル | `js/util/quarter_view_tile_generator.js` |
| 形式 | 静的メソッドのみの純粋ユーティリティクラス |

### 4.2 主要静的メソッド

```js
/**
 * パラメータから PixelData 群を生成する。
 * target='new' 時: 新規 LayerData を作成して返す。
 * target='overwrite' 時: 渡された既存 LayerData のアクティブレイヤーに描画し null を返す。
 * @param {QuarterViewTileParams} params
 * @param {LayerData|null} existingLayerData - overwrite 時に渡す既存データ
 * @returns {LayerData|null}
 */
static generate(params, existingLayerData = null)

/**
 * PixelData に単一面（多角形）の塗りつぶしと輪郭線を描画する。
 * separateLayers=false（1レイヤー描画）時に、面ごとの描画をまとめて行う
 * ユーティリティメソッド。内部で _fillPolygon → _drawLine（輪郭）の順に呼ぶ。
 * @param {PixelData} pd  - 描画先
 * @param {Array<{x:number,y:number}>} vertices - 頂点配列（時計回り）
 * @param {number} fillColor   - 0xAARRGGBB, 0=塗りつぶしなし
 * @param {number} outlineColor - 0xAARRGGBB
 */
static _drawPolygon(pd, vertices, fillColor, outlineColor)

/**
 * Bresenham 直線アルゴリズムで線を描画する。
 * @param {PixelData} pd
 * @param {number} x0 @param {number} y0
 * @param {number} x1 @param {number} y1
 * @param {number} color
 */
static _drawLine(pd, x0, y0, x1, y1, color)

/**
 * スキャンライン法でポリゴン内部を塗りつぶす。
 * @param {PixelData} pd
 * @param {Array<{x:number,y:number}>} vertices
 * @param {number} color
 */
static _fillPolygon(pd, vertices, color)
```

### 4.3 パラメータ型 `QuarterViewTileParams`

```js
{
  type:            'ground' | 'box',  // タイルタイプ
  width:           number,            // キャンバス幅（4の倍数）
  wallHeight:      number,            // 壁の高さ（boxのみ使用、groundは0固定）
  separateLayers:  boolean,           // true=面ごとにレイヤー分け
  topColor:        number,            // 上面塗りつぶし 0xAARRGGBB
  leftColor:       number,            // 左面塗りつぶし
  rightColor:      number,            // 右面塗りつぶし
  outlineColor:    number,            // 輪郭線色
  target:          'new' | 'overwrite', // 適用先（デフォルト: 'new'）
}
```

### 4.4 生成ロジック

#### 頂点計算

```
W  = params.width           (4の倍数)
cx = W / 2                  (整数)
ry = W / 4                  (整数; ラテラル頂点のY座標)
by = 2 * ry - 1             (底辺Y; キャンバス内に収まる最大Y)
H_w = params.wallHeight

上面頂点: [ (cx,0), (W-1,ry), (cx,by), (0,ry) ]

左面頂点: [ (0,ry), (cx,by), (cx,by+H_w), (0,ry+H_w) ]

右面頂点: [ (cx,by), (W-1,ry), (W-1,ry+H_w), (cx,by+H_w) ]
```

#### 描画順

**`separateLayers = false`（1レイヤー）時:**

1. 塗りつぶし: 上面 → 左面 → 右面 の順に `_fillPolygon` で全ピクセルを塗る
2. 輪郭線: 全面の辺を `_drawLine` で一括描画（塗りつぶしの上に重ね書き）

> 輪郭線は全面の塗りつぶし完了後にまとめて描くことで、常に最前面に出る。
> この場合 `_drawPolygon` は使わず `_fillPolygon` + `_drawLine` を直接呼ぶ。

**`separateLayers = true`（レイヤー分け）時:**

各面ごとに専用レイヤーの PixelData 上で `_drawPolygon` を呼ぶ。
`_drawPolygon` 内で `_fillPolygon` → 輪郭 `_drawLine` の順に実行するため、
面同士の塗りが干渉しない。

#### `_fillPolygon` スキャンライン法

```
1. 頂点から y の最小値 y_min と最大値 y_max を求める
2. y を y_min から y_max まで 1 ずつ増やしながら:
   a. 全辺と y=Y の交点 x 座標リストを収集
   b. x リストをソートして左右のペアで setPixel
```

#### `_drawLine` Bresenham アルゴリズム

- 標準的な整数演算による 1px 太さの直線描画
- 始点・終点ともに描画する（両端込み）

---

## 5. メニュー追加仕様

### 5.1 新規メニュー「生成」

既存メニュー（ファイル / 編集 / 表示 / タイルセット）に加え、
「**生成**」メニューを末尾に追加する。
今後の自動生成系機能（グラデーション、ノイズなど）はここに集約する。

### 5.2 追加するメニュー項目

```
生成
 └─ クォータービュータイル...  [GENERATE_QUARTER_VIEW_TILE]
```

### 5.3 `MenuConstants` への追加

```js
// ---- 生成 ----
static GENERATE_QUARTER_VIEW_TILE = 'generate.quarter_view_tile';
```

---

## 6. 変更・追加ファイル一覧

| ファイル | 変更種別 | 内容 |
|----------|----------|------|
| `js/ui/menu_constants.js` | 修正 | `GENERATE_QUARTER_VIEW_TILE` 定数を追加 |
| `js/ui/menu_bar.js` | 修正 | 「生成」メニュー追加、`GENERATE_QUARTER_VIEW_TILE` 追加 |
| `js/ui/dialog/quarter_view_tile_dialog.js` | **新規** | `QuarterViewTileDialog` クラス実装 |
| `js/util/quarter_view_tile_generator.js` | **新規** | `QuarterViewTileGenerator` クラス実装 |
| `js/scene/editor_scene.js` | 修正 | `GENERATE_QUARTER_VIEW_TILE` ハンドラ追加、ダイアログ初期化追加 |
| `www/index.html` | 修正 | 上記 2 ファイルの `<script>` タグを追加 |

### index.html 読み込み順の追記位置

```
...
dialog_base.js → new_file_dialog.js → save_dialog.js → color_picker_dialog.js
→ quarter_view_tile_dialog.js          ← ここに追加
...
→ pixel_data_converter.js
→ quarter_view_tile_generator.js       ← ここに追加
```

---

## 7. EditorScene 統合

### 7.1 初期化（コンストラクタ or `onEnter`）

```js
this._quarterViewTileDialog = new QuarterViewTileDialog(
  (params) => this._onQuarterViewTileConfirm(params),
  () => {}  // onCancel（特別な処理なし）
);
```

### 7.2 メニューハンドラ

```js
if (id === MenuConstants.GENERATE_QUARTER_VIEW_TILE) {
  this._quarterViewTileDialog.show();
  return;
}
```

### 7.3 確認コールバック

```js
_onQuarterViewTileConfirm(params) {
  if (params.target === 'overwrite') {
    // 既存キャンバスのアクティブレイヤーにそのまま描画（サイズ変更なし）
    QuarterViewTileGenerator.generate(params, this._appData.layerData);
    this._appData.layerData.markCompositeDirty();
    this._pixelCanvas.markDirty();
  } else {
    // 新規作成: generate が返す LayerData で _layerData を差し替える
    const ld = QuarterViewTileGenerator.generate(params);
    this._appData._layerData = ld;
    this._pixelCanvas.markDirty();
    this._pixelCanvas.resetView();  // パン＋ズームを初期化
  }
}
```

> `appData.layerData` は getter のみ（setter 未定義）のため、
> `new` モードでは内部プロパティ `_layerData` を直接差し替える。

---

## 8. 動作シナリオ

### 8.1 新規作成モード

```
1. ユーザーが「生成 > クォータービュータイル...」を選択
2. QuarterViewTileDialog がモーダル表示される
3. ユーザーが「箱タイル / 32×32 / レイヤーを分ける ON / 新規作成」を設定
4. プレビュー欄にリアルタイムでタイル形状が描画される
5. 「生成」ボタンを押す（または Enter）
6. QuarterViewTileGenerator.generate(params) → 新規 LayerData を返す
7. appData._layerData が置き換えられ、PixelCanvas がリフレッシュされる
8. エディタに 3 レイヤー（右面/左面/上面; 上面が最前面）のガイド入りタイルが表示される
9. ユーザーは各レイヤーを選択して上から描き込む
```

### 8.2 上書きモード

```
1. 既にキャンバスが開かれている状態で「生成 > クォータービュータイル...」を選択
2. ユーザーが「現在のキャンバスに上書き」を選択
3. 「生成」ボタンを押す
4. generate(params, existingLayerData) がアクティブレイヤーに直接描画
5. キャンバスサイズは変更されず、既存の絵の上にタイル輪郭が重ね書きされる
```

---

## 9. 将来の拡張候補

| 機能 | 概要 |
|------|------|
| 斜め壁タイル | 左または右だけ壁がある半箱タイル |
| 坂タイル | 上面が傾斜した地形タイル |
| 生成プリセット保存 | 最後の設定を localStorage に保存して次回に復元 |
| 別途「生成」メニュー | ノイズ塗りつぶし・グラデーション生成なども同メニューに追加 |
