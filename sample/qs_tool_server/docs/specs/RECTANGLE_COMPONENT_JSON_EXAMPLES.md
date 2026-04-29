# Rectangle Component JSON 例集

最終更新: 2026-04-29

`PlayUnitEditorScene` の JSON 編集で `Rectangle` component を扱うための、
実装テンプレート集。

---

## 基本テンプレート

### 矩形

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

### 円

```json
{
  "shape": "circle",
  "width": 48,
  "height": 0,
  "fillColor": "#4ecdc4",
  "fillAlpha": 1,
  "strokeColor": "#000000",
  "strokeWidth": 1,
  "strokeAlpha": 1,
  "rotation": 0,
  "originX": 0.5,
  "originY": 0.5
}
```

### 多角形

```json
{
  "shape": "polygon",
  "width": 40,
  "height": 40,
  "sides": 6,
  "fillColor": "#ffd93d",
  "fillAlpha": 1,
  "strokeColor": "#333333",
  "strokeWidth": 1.5,
  "strokeAlpha": 1,
  "rotation": 0,
  "originX": 0.5,
  "originY": 0.5
}
```

---

## ゲーム用例

### 1. 敵キャラクターの当たり判定ビジュアライザー

```json
{
  "shape": "rectangle",
  "width": 32,
  "height": 32,
  "fillColor": "#ffffff",
  "fillAlpha": 0.1,
  "strokeColor": "#ff0000",
  "strokeWidth": 1,
  "strokeAlpha": 1,
  "rotation": 0,
  "originX": 0.5,
  "originY": 0.5
}
```

敵オブジェクトの当たり判定を赤い半透明矩形で可視化。
デバッグ表示として使用。

---

### 2. HP バー背景

```json
{
  "shape": "rectangle",
  "width": 120,
  "height": 12,
  "fillColor": "#333333",
  "fillAlpha": 1,
  "strokeColor": "#666666",
  "strokeWidth": 1,
  "strokeAlpha": 1,
  "rotation": 0,
  "originX": 0,
  "originY": 0.5
}
```

HPバーの背景。Text component でHP数値と組み合わせて使用。

---

### 3. HP バー（満量状態）

```json
{
  "shape": "rectangle",
  "width": 100,
  "height": 10,
  "fillColor": "#22c55e",
  "fillAlpha": 1,
  "strokeColor": "#16a34a",
  "strokeWidth": 1,
  "strokeAlpha": 1,
  "rotation": 0,
  "originX": 0,
  "originY": 0.5
}
```

HP が満量の場合の表示。色を #22c55e（緑）に設定。

---

### 4. HP バー（半分）

```json
{
  "shape": "rectangle",
  "width": 50,
  "height": 10,
  "fillColor": "#eab308",
  "fillAlpha": 1,
  "strokeColor": "#ca8a04",
  "strokeWidth": 1,
  "strokeAlpha": 1,
  "rotation": 0,
  "originX": 0,
  "originY": 0.5
}
```

HP が半分の場合の表示。色を黄色に設定。

---

### 5. HP バー（危機状態）

```json
{
  "shape": "rectangle",
  "width": 20,
  "height": 10,
  "fillColor": "#ef4444",
  "fillAlpha": 1,
  "strokeColor": "#dc2626",
  "strokeWidth": 1,
  "strokeAlpha": 1,
  "rotation": 0,
  "originX": 0,
  "originY": 0.5
}
```

HP が少ない場合の表示。色を赤に設定。

---

### 6. UI ボタン背景

```json
{
  "shape": "rectangle",
  "width": 160,
  "height": 48,
  "fillColor": "#3b82f6",
  "fillAlpha": 1,
  "strokeColor": "#1e40af",
  "strokeWidth": 2,
  "strokeAlpha": 1,
  "rotation": 0,
  "originX": 0,
  "originY": 0
}
```

UI ボタンの背景。Text component で「OK」などのラベルを重ねる。

---

### 7. ボタン（押下時）

```json
{
  "shape": "rectangle",
  "width": 160,
  "height": 48,
  "fillColor": "#1e40af",
  "fillAlpha": 1,
  "strokeColor": "#0c4a6e",
  "strokeWidth": 2,
  "strokeAlpha": 1,
  "rotation": 0,
  "originX": 0,
  "originY": 0
}
```

ボタンが押下状態の表現。色を濃くする。

---

### 8. プレイヤー移動範囲の可視化

```json
{
  "shape": "circle",
  "width": 160,
  "height": 0,
  "fillColor": "#ffffff",
  "fillAlpha": 0.05,
  "strokeColor": "#4ecdc4",
  "strokeWidth": 2,
  "strokeAlpha": 0.5,
  "rotation": 0,
  "originX": 0.5,
  "originY": 0.5
}
```

プレイヤーが移動可能な範囲を青い円で表示。
デバッグ表示。

---

### 9. 方向を示す矢印（Triangle）

```json
{
  "shape": "triangle",
  "width": 48,
  "height": 48,
  "fillColor": "#e74c3c",
  "fillAlpha": 1,
  "strokeColor": "#333333",
  "strokeWidth": 2,
  "strokeAlpha": 1,
  "rotation": 0,
  "originX": 0.5,
  "originY": 0.5
}
```

敵やプレイヤーの向き、方向矢印を表示。
`rotation` で向きを制御（0 = 上向き、90 = 右向き、etc）。

---

### 10. 敵の視野範囲

```json
{
  "shape": "polygon",
  "width": 120,
  "height": 120,
  "sides": 3,
  "fillColor": "#ffc300",
  "fillAlpha": 0.15,
  "strokeColor": "#ffc300",
  "strokeWidth": 1,
  "strokeAlpha": 0.3,
  "rotation": 0,
  "originX": 0.5,
  "originY": 0.5
}
```

敵の視野範囲を三角形で表示。
`rotation` で向きを制御。

---

### 11. 装飾的な星マーク

```json
{
  "shape": "star",
  "width": 40,
  "height": 40,
  "points": 5,
  "innerRadius": 0.4,
  "fillColor": "#fff74c",
  "fillAlpha": 1,
  "strokeColor": "#ffc300",
  "strokeWidth": 1,
  "strokeAlpha": 1,
  "rotation": 0,
  "originX": 0.5,
  "originY": 0.5
}
```

リザルト画面や重要アイテムの装飾。

---

### 12. ワープゾーンの可視化

```json
{
  "shape": "rectangle",
  "width": 80,
  "height": 80,
  "fillColor": "#9333ea",
  "fillAlpha": 0.1,
  "strokeColor": "#a855f7",
  "strokeWidth": 2,
  "strokeAlpha": 1,
  "rotation": 45,
  "originX": 0.5,
  "originY": 0.5
}
```

45度回転させた矩形でワープゾーンを表現。

---

### 13. タイルグリッド（デバッグ用）

```json
{
  "shape": "rectangle",
  "width": 32,
  "height": 32,
  "fillColor": "#ffffff",
  "fillAlpha": 0,
  "strokeColor": "#94a3b8",
  "strokeWidth": 0.5,
  "strokeAlpha": 0.3,
  "rotation": 0,
  "originX": 0,
  "originY": 0
}
```

マップチップのグリッドラインを示す。
`fillAlpha: 0` で透明背景。

---

### 14. 攻撃範囲表示

```json
{
  "shape": "circle",
  "width": 64,
  "height": 0,
  "fillColor": "#ef4444",
  "fillAlpha": 0.2,
  "strokeColor": "#dc2626",
  "strokeWidth": 2,
  "strokeAlpha": 0.8,
  "rotation": 0,
  "originX": 0.5,
  "originY": 0.5
}
```

キャラクターが攻撃可能な範囲を赤い円で表示。

---

### 15. スキル効果範囲（正方形）

```json
{
  "shape": "rectangle",
  "width": 96,
  "height": 96,
  "fillColor": "#4ecdc4",
  "fillAlpha": 0.2,
  "strokeColor": "#14b8a6",
  "strokeWidth": 2,
  "strokeAlpha": 1,
  "rotation": 45,
  "originX": 0.5,
  "originY": 0.5
}
```

スキルの効果範囲を回転した矩形で表現。

---

### 16. ミニマップ用フレーム

```json
{
  "shape": "rectangle",
  "width": 128,
  "height": 128,
  "fillColor": "#0f172a",
  "fillAlpha": 0.8,
  "strokeColor": "#475569",
  "strokeWidth": 2,
  "strokeAlpha": 1,
  "rotation": 0,
  "originX": 0,
  "originY": 0
}
```

ミニマップの背景フレーム。
UI/HUD 領域として Image component と組み合わせて使用。

---

## 形状サンプル（視覚確認用）

### 各 shape の基本形

| shape | 説明 | sides/options |
|-------|------|---------------|
| `rectangle` | 矩形 | width, height |
| `circle` | 円 | width（直径） |
| `polygon` | 正多角形 | sides パラメータで辺数指定 |
| `triangle` | 三角形 | sides = 3 相当 |
| `star` | 星型 | points, innerRadius |

### 多角形の辺数例

```json
{
  "shape": "polygon",
  "width": 48,
  "height": 48,
  "sides": 3,
  "fillColor": "#95e1d3",
  "strokeColor": "#333333",
  "strokeWidth": 1,
  "originX": 0.5,
  "originY": 0.5
}
// 三角形

{
  "shape": "polygon",
  "width": 48,
  "height": 48,
  "sides": 4,
  "fillColor": "#95e1d3",
  "strokeColor": "#333333",
  "strokeWidth": 1,
  "originX": 0.5,
  "originY": 0.5
}
// 正四角形

{
  "shape": "polygon",
  "width": 48,
  "height": 48,
  "sides": 6,
  "fillColor": "#95e1d3",
  "strokeColor": "#333333",
  "strokeWidth": 1,
  "originX": 0.5,
  "originY": 0.5
}
// 六角形

{
  "shape": "polygon",
  "width": 48,
  "height": 48,
  "sides": 8,
  "fillColor": "#95e1d3",
  "strokeColor": "#333333",
  "strokeWidth": 1,
  "originX": 0.5,
  "originY": 0.5
}
// 八角形
```

---

## 色の選択ガイド

### 推奨カラーパレット

| 用途 | 色 | RGB |
|------|---|----|
| 通常 | `#ffffff` | 白 |
| 背景 / 無効 | `#94a3b8` | グレー |
| 成功 / HP満量 | `#22c55e` | 緑 |
| 注意 / 半分 | `#eab308` | 黄色 |
| エラー / 危機 | `#ef4444` | 赤 |
| UI 要素 | `#3b82f6` | 青 |
| アクセント | `#4ecdc4` | 青緑 |
| 装飾 | `#ffc300` or `#fff74c` | 黄 |
| ダーク背景 | `#0f172a` | 濃紺 |

---
