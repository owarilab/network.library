# Image component JSON 例集

最終更新: 2026-04-27

`PlayUnitEditorScene` の JSON 編集で `Image` component を扱うための、
最小テンプレート集。

現状の `Image` は `pixelDocument` をそのまま描画する用途に絞っている。
`Sprite` のような atlas / frame 切り出しはまだ扱わない。

---

## 基本テンプレート

```json
{
  "pixelDocumentId": "px_your_image",
  "alpha": 1,
  "width": 0,
  "height": 0,
  "keepAspect": true,
  "originX": 0,
  "originY": 0
}
```

- `pixelDocumentId`: 描画したい `pixelDocument` の ID
- `alpha`: 0.0 から 1.0
- `width`, `height`: 0 なら元画像サイズ
- `keepAspect`: `true` なら片側サイズ指定時に縦横比を維持
- `originX`, `originY`: 0.0 から 1.0。`Transform` 座標をどの基準点として使うか

---

## 1. 元サイズで表示

```json
{
  "pixelDocumentId": "px_logo",
  "alpha": 1,
  "width": 0,
  "height": 0,
  "keepAspect": true,
  "originX": 0,
  "originY": 0
}
```

`Transform.data.x`, `Transform.data.y` を左上座標として、
`pixelDocument` の元サイズで描画する。

---

## 2. 半透明で重ねる

```json
{
  "pixelDocumentId": "px_shadow",
  "alpha": 0.5,
  "width": 0,
  "height": 0,
  "keepAspect": true,
  "originX": 0,
  "originY": 0
}
```

背景装飾やフェードしたアイコン確認用。

---

## 3. 拡大表示

```json
{
  "pixelDocumentId": "px_face_icon",
  "alpha": 1,
  "width": 96,
  "height": 96,
  "keepAspect": true,
  "originX": 0,
  "originY": 0
}
```

小さい `pixelDocument` を preview 上で見やすく拡大したいときに使う。

---

## 4. 横長バナー

```json
{
  "pixelDocumentId": "px_title_banner",
  "alpha": 1,
  "width": 240,
  "height": 64,
  "keepAspect": false,
  "originX": 0,
  "originY": 0
}
```

タイトル画面や UI バーの仮置き確認向け。

---

## 5. HUD アイコン

```json
{
  "pixelDocumentId": "px_hp_icon",
  "alpha": 1,
  "width": 32,
  "height": 32,
  "keepAspect": true,
  "originX": 0,
  "originY": 0
}
```

現状は world-space 描画なので、完全な HUD / screen-space 仕様ではない。
ただし UI パーツ自体の見た目確認には使える。

---

## 6. 中央基準で配置

```json
{
  "pixelDocumentId": "px_actor",
  "alpha": 1,
  "width": 48,
  "height": 0,
  "keepAspect": true,
  "originX": 0.5,
  "originY": 0.5
}
```

`Transform.data.x`, `Transform.data.y` を画像中央として使う。
キャラクターやマーカーの配置確認に向く。

---

## 7. 下端中央を足元に合わせる

```json
{
  "pixelDocumentId": "px_npc",
  "alpha": 1,
  "width": 0,
  "height": 0,
  "keepAspect": true,
  "originX": 0.5,
  "originY": 1
}
```

立ち絵やキャラ画像の足元を `Transform` 座標に合わせたいときの基本形。

---

## 補足

- `Image` を表示するには同じ object に `Transform` が必要
- `pixelDocumentId` が空、または参照先が見つからない場合は描画されない
- `keepAspect: true` なら `width`, `height` のどちらか片方だけでも縦横比を維持して補完される
- `keepAspect: false` のときは `width`, `height` を独立して扱う
- 将来的に atlas / frame 単位の描画が必要になったら、それは `Sprite` として別仕様に分ける