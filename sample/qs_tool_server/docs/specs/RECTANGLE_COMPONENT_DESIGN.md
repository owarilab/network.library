# Rectangle Component 設計書

最終更新: 2026-04-29

## 目的

`PlayUnit` ゲーム構成内で、簡単な図形（矩形、円、多角形など）を配置できるようにするため、
汎用的な `Rectangle` component を追加する。

`Image` component と同じく、`Transform` component と組み合わせて使用され、
world-space 上に描画される。

---

## 概要

`Rectangle` component は、以下の特性を持つ。

- **Shape Types**: rectangle、circle、polygon など複数の図形をサポート
- **Color**: fill 色と stroke 色の独立制御
- **Size**: width、height で図形サイズを指定
- **Rotation**: 回転角度（多角形など方向を持つ図形向け）
- **Origin**: Image と同じく originX / originY で基準点をカスタマイズ可能
- **Alpha**: 透明度制御

---

## データ構造

### 基本テンプレート

```json
{
  "shape": "rectangle",
  "width": 64,
  "height": 32,
  "fillColor": "#ffffff",
  "fillAlpha": 1,
  "strokeColor": "#000000",
  "strokeWidth": 2,
  "strokeAlpha": 1,
  "rotation": 0,
  "originX": 0,
  "originY": 0
}
```

### パラメータ詳細

| パラメータ | 型 | デフォルト | 説明 |
|-----------|----|---------|----|
| `shape` | string | "rectangle" | 図形の種類。`rectangle`, `circle`, `polygon` など |
| `width` | number | 64 | 図形の幅。circle では直径として使用 |
| `height` | number | 32 | 図形の高さ。circle では無視される |
| `fillColor` | string | "#ffffff" | 塗りつぶし色。CSS色形式（#RRGGBB または rgba(...)） |
| `fillAlpha` | number | 1 | 塗りつぶし透明度。0.0 ～ 1.0 |
| `strokeColor` | string | "#000000" | 枠線色。CSS色形式 |
| `strokeWidth` | number | 2 | 枠線の太さ（ピクセル）。0 で無枠 |
| `strokeAlpha` | number | 1 | 枠線の透明度。0.0 ～ 1.0 |
| `rotation` | number | 0 | 回転角度（度数法）。0 ～ 360 |
| `originX` | number | 0 | 基準点X（0.0: 左、0.5: 中央、1.0: 右） |
| `originY` | number | 0 | 基準点Y（0.0: 上、0.5: 中央、1.0: 下） |

---

## Shape Types

### 1. rectangle

矩形を描画する。最も基本的な図形。

```json
{
  "shape": "rectangle",
  "width": 100,
  "height": 50,
  "fillColor": "#ff6b6b",
  "strokeColor": "#333333",
  "strokeWidth": 2,
  "originX": 0,
  "originY": 0
}
```

- `width` と `height` の両方を使用
- `originX`, `originY` で基準点を変更可能
- `rotation` で回転可能

### 2. circle

円を描画する。

```json
{
  "shape": "circle",
  "width": 48,
  "height": 0,
  "fillColor": "#4ecdc4",
  "strokeColor": "#333333",
  "strokeWidth": 1,
  "originX": 0.5,
  "originY": 0.5
}
```

- `width` が直径として使用される
- `height` は無視される
- `rotation` は適用されない（円は対称）
- `originX`, `originY` で中心位置を調整可能

### 3. polygon

正多角形を描画する。

```json
{
  "shape": "polygon",
  "width": 40,
  "height": 40,
  "sides": 6,
  "fillColor": "#ffd93d",
  "strokeColor": "#333333",
  "strokeWidth": 1.5,
  "rotation": 0,
  "originX": 0.5,
  "originY": 0.5
}
```

- `width` と `height` で外接円のサイズを指定
- 追加パラメータ `sides` で辺数を指定（3: 三角形、4: 正四角形、6: 六角形など）
- `rotation` で回転可能
- デフォルト `sides` は 4（正四角形と矩形の中間）

### 4. triangle

三角形の特化型。

```json
{
  "shape": "triangle",
  "width": 48,
  "height": 48,
  "fillColor": "#95e1d3",
  "strokeColor": "#333333",
  "strokeWidth": 1,
  "rotation": 0,
  "originX": 0.5,
  "originY": 0.5
}
```

- 正三角形として描画
- `rotation` で向き変更可能（矢印のような用途向け）
- `width`, `height` は外接円のサイズ

### 5. star

星型を描画する。

```json
{
  "shape": "star",
  "width": 50,
  "height": 50,
  "points": 5,
  "innerRadius": 0.4,
  "fillColor": "#fff74c",
  "strokeColor": "#333333",
  "strokeWidth": 1,
  "rotation": -90,
  "originX": 0.5,
  "originY": 0.5
}
```

- 追加パラメータ `points`: 星の先端数（デフォルト 5）
- 追加パラメータ `innerRadius`: 内側半径の比率 0.0 ～ 1.0（デフォルト 0.4）
- `rotation` で向き変更可能

---

## 使用例

### 例1: 敵の衝突判定用矩形

```json
{
  "shape": "rectangle",
  "width": 32,
  "height": 32,
  "fillColor": "#ffffff",
  "fillAlpha": 0.1,
  "strokeColor": "#ff6b6b",
  "strokeWidth": 1,
  "strokeAlpha": 1,
  "rotation": 0,
  "originX": 0.5,
  "originY": 0.5
}
```

デバッグ表示用の半透明矩形。敵オブジェクトの当たり判定を可視化。

### 例2: HP バーの背景

```json
{
  "shape": "rectangle",
  "width": 120,
  "height": 12,
  "fillColor": "#333333",
  "fillAlpha": 1,
  "strokeColor": "#666666",
  "strokeWidth": 1,
  "originX": 0,
  "originY": 0.5
}
```

### 例3: 装飾用な円形エフェクト

```json
{
  "shape": "circle",
  "width": 80,
  "height": 0,
  "fillColor": "#ffaf87",
  "fillAlpha": 0.3,
  "strokeColor": "#ff6b6b",
  "strokeWidth": 2,
  "strokeAlpha": 1,
  "originX": 0.5,
  "originY": 0.5
}
```

### 例4: UI ボタン背景

```json
{
  "shape": "rectangle",
  "width": 160,
  "height": 48,
  "fillColor": "#4ecdc4",
  "fillAlpha": 1,
  "strokeColor": "#2c8a84",
  "strokeWidth": 2,
  "originX": 0,
  "originY": 0
}
```

Text component と組み合わせてボタン表現。

### 例5: 矢印マーカー

```json
{
  "shape": "polygon",
  "width": 24,
  "height": 24,
  "sides": 3,
  "fillColor": "#ffd93d",
  "strokeColor": "#333333",
  "strokeWidth": 1,
  "rotation": 90,
  "originX": 0.5,
  "originY": 0.5
}
```

キャラクターの上に矢印を配置。`rotation` で向きを制御。

### 例6: 装飾用星

```json
{
  "shape": "star",
  "width": 40,
  "height": 40,
  "points": 5,
  "innerRadius": 0.4,
  "fillColor": "#fff74c",
  "strokeColor": "#ffc300",
  "strokeWidth": 1,
  "rotation": 0,
  "originX": 0.5,
  "originY": 0.5
}
```

---

## Image component との関係

| 項目 | Image | Rectangle |
|------|-------|-----------|
| 基本用途 | ドット絵画像を配置 | 図形を動的に描画 |
| 参照先 | pixelDocumentId | shape パラメータ |
| サイズ指定 | pixelDocumentId のサイズまたは width/height 指定 | width/height 指定 |
| 透明度 | alpha | fillAlpha / strokeAlpha 分離 |
| 回転 | 非対応 | rotation で対応 |
| 基準点 | originX / originY | originX / originY |
| Z-order | Transform.z | Transform.z |

---

## 実装計画

### Phase 1: Rectangle コンポーネント基本実装

- [ ] Rectangle component JSON テンプレート確定
- [ ] 基本 shape（rectangle, circle）の描画ロジック実装
- [ ] PlayUnitRuntime に rectangleEntries を追加
- [ ] PlayTestScene で Rectangle 描画

### Phase 2: 拡張 shape サポート

- [ ] polygon, triangle, star の描画ロジック追加
- [ ] rotation パラメータの適用

### Phase 3: UI 整備

- [ ] PlayUnitEditorScene での Rectangle component JSON 編集
- [ ] カラーピッカー連携で fillColor / strokeColor を選択可能にする（将来）

### Phase 4: デバッグ機能

- [ ] collider 可視化としての デバッグ矩形表示オプション
- [ ] grid overlay など補助機能

---

## 技術実装細部（参考）

### Canvas 描画実装パターン

```javascript
// 矩形
ctx.fillStyle = fillColorCss;
ctx.fillRect(x, y, width, height);
if (strokeWidth > 0) {
  ctx.strokeStyle = strokeColorCss;
  ctx.lineWidth = strokeWidth;
  ctx.strokeRect(x, y, width, height);
}

// 円
ctx.beginPath();
ctx.arc(x, y, radius, 0, Math.PI * 2);
ctx.fillStyle = fillColorCss;
ctx.fill();
if (strokeWidth > 0) {
  ctx.strokeStyle = strokeColorCss;
  ctx.lineWidth = strokeWidth;
  ctx.stroke();
}

// 多角形（回転対応）
ctx.save();
ctx.translate(x, y);
ctx.rotate((rotation * Math.PI) / 180);
ctx.beginPath();
for (let i = 0; i < sides; i++) {
  const angle = (i / sides) * Math.PI * 2;
  const px = Math.cos(angle) * radius;
  const py = Math.sin(angle) * radius;
  if (i === 0) ctx.moveTo(px, py);
  else ctx.lineTo(px, py);
}
ctx.closePath();
ctx.fillStyle = fillColorCss;
ctx.fill();
if (strokeWidth > 0) {
  ctx.strokeStyle = strokeColorCss;
  ctx.lineWidth = strokeWidth;
  ctx.stroke();
}
ctx.restore();
```

---

## 将来の拡張候補

- **Gradient fill**: グラデーション塗りつぶし
- **Line dash pattern**: 破線スタイル
- **Shadow**: ドロップシャドウ
- **Rounded corners**: 矩形の角丸
- **Bezier curves**: 曲線描画
- **Animation**: 図形のアニメーション property

---
