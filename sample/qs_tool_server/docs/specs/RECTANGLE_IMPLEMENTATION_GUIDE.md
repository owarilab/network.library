# Rectangle Component 実装テンプレート - Phase 1 動作確認

## 動作確認用 PlayUnit JSON テンプレート

PlayUnitEditorScene で以下のテンプレートを使用して、Object/Component を JSON 編集で追加してテストできます。

---

## テンプレート 1: シンプルな矩形

新規 PlayUnit を作成後、以下の Object を JSON で追加します。

```json
{
  "id": "obj_rect1",
  "name": "RedRectangle",
  "enabled": true,
  "parentId": "obj_root",
  "children": [],
  "components": [
    {
      "type": "Transform",
      "enabled": true,
      "data": {
        "x": 100,
        "y": 100,
        "z": 0,
        "rotation": 0,
        "scaleX": 1,
        "scaleY": 1
      }
    },
    {
      "type": "Rectangle",
      "enabled": true,
      "data": {
        "shape": "rectangle",
        "width": 128,
        "height": 64,
        "fillColor": "#ff6b6b",
        "fillAlpha": 1,
        "strokeColor": "#333333",
        "strokeWidth": 2,
        "strokeAlpha": 1,
        "rotation": 0,
        "originX": 0,
        "originY": 0
      }
    }
  ]
}
```

**期待される動作**:
- プレビュー上に赤い矩形が表示される
- 左上が (100, 100) に配置される

---

## テンプレート 2: 円形

```json
{
  "id": "obj_circle1",
  "name": "BlueCircle",
  "enabled": true,
  "parentId": "obj_root",
  "children": [],
  "components": [
    {
      "type": "Transform",
      "enabled": true,
      "data": {
        "x": 300,
        "y": 150,
        "z": 0,
        "rotation": 0,
        "scaleX": 1,
        "scaleY": 1
      }
    },
    {
      "type": "Rectangle",
      "enabled": true,
      "data": {
        "shape": "circle",
        "width": 80,
        "height": 0,
        "fillColor": "#4ecdc4",
        "fillAlpha": 1,
        "strokeColor": "#333333",
        "strokeWidth": 1,
        "strokeAlpha": 1,
        "rotation": 0,
        "originX": 0.5,
        "originY": 0.5
      }
    }
  ]
}
```

**期待される動作**:
- プレビュー上に青緑色の円が表示される
- 中央が (300, 150) に配置される（originX: 0.5, originY: 0.5）

---

## テンプレート 3: 多角形（六角形）

```json
{
  "id": "obj_hex1",
  "name": "YellowHexagon",
  "enabled": true,
  "parentId": "obj_root",
  "children": [],
  "components": [
    {
      "type": "Transform",
      "enabled": true,
      "data": {
        "x": 500,
        "y": 150,
        "z": 0,
        "rotation": 0,
        "scaleX": 1,
        "scaleY": 1
      }
    },
    {
      "type": "Rectangle",
      "enabled": true,
      "data": {
        "shape": "polygon",
        "width": 60,
        "height": 60,
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
    }
  ]
}
```

**期待される動作**:
- プレビュー上に黄色い六角形が表示される

---

## テンプレート 4: 半透明矩形（デバッグ表示用）

```json
{
  "id": "obj_debug_rect",
  "name": "DebugBounds",
  "enabled": true,
  "parentId": "obj_root",
  "children": [],
  "components": [
    {
      "type": "Transform",
      "enabled": true,
      "data": {
        "x": 150,
        "y": 250,
        "z": 0,
        "rotation": 0,
        "scaleX": 1,
        "scaleY": 1
      }
    },
    {
      "type": "Rectangle",
      "enabled": true,
      "data": {
        "shape": "rectangle",
        "width": 96,
        "height": 96,
        "fillColor": "#ffffff",
        "fillAlpha": 0.15,
        "strokeColor": "#ff0000",
        "strokeWidth": 2,
        "strokeAlpha": 1,
        "rotation": 0,
        "originX": 0.5,
        "originY": 0.5
      }
    }
  ]
}
```

**期待される動作**:
- 半透明の赤い矩形で、中央が (150, 250) に配置される

---

## テンプレート 5: 回転矩形

```json
{
  "id": "obj_rotated",
  "name": "RotatedRectangle",
  "enabled": true,
  "parentId": "obj_root",
  "children": [],
  "components": [
    {
      "type": "Transform",
      "enabled": true,
      "data": {
        "x": 350,
        "y": 300,
        "z": 0,
        "rotation": 0,
        "scaleX": 1,
        "scaleY": 1
      }
    },
    {
      "type": "Rectangle",
      "enabled": true,
      "data": {
        "shape": "rectangle",
        "width": 100,
        "height": 50,
        "fillColor": "#95e1d3",
        "fillAlpha": 1,
        "strokeColor": "#333333",
        "strokeWidth": 1,
        "strokeAlpha": 1,
        "rotation": 45,
        "originX": 0.5,
        "originY": 0.5
      }
    }
  ]
}
```

**期待される動作**:
- 45度回転した矩形が (350, 300) に配置される

---

## 動作確認手順

1. **ProjectTopScene** で新規 PlayUnit を作成
2. **PlayUnitEditorScene** で上記テンプレートの Object を JSON 編集で追加
   - `+ Add Object` → Component Type 選択で `Rectangle`
   - または直接 `{ "id": "...", ... }` JSON を貼り付け
3. **PlayTestScene** で `[Play Test]` ボタンをクリック
4. プレビュー領域で図形が正しく表示されているか確認

---

## 確認事項

### ✅ 実装済み（Phase 1）

- [x] PlayUnitRuntime に rectangleEntries を追加
- [x] PlayTestScene で rectangle/circle/polygon 描画
- [x] fillColor, fillAlpha, strokeColor, strokeWidth, strokeAlpha の適用
- [x] originX, originY による基準点制御
- [x] rotation による回転

### ❌ Phase 2 以降

- [ ] triangle, star shape の実装
- [ ] Polygon の sides パラメータ詳細調整
- [ ] PlayUnitEditorScene の Rectangle component JSON 編集 UI 洗練化

---

## トラブルシューティング

### 図形が表示されない

- Transform component が有効になっているか確認
- Rectangle component の data が正しい JSON 形式か確認
- shape パラメータが `rectangle`, `circle`, `polygon` のいずれかか確認

### 図形の位置がおかしい

- originX, originY の値（0.0 ～ 1.0）を確認
- Transform.data.x, Transform.data.y の値を確認
- Camera 位置と zoom レベルを確認

### 色が表示されない

- fillColor / strokeColor が CSS 色形式（#RRGGBB など）か確認
- fillAlpha / strokeAlpha が 0 ～ 1 の範囲か確認

---
