# Collider / Trigger JSON 例集

最終更新: 2026-04-27

`PlayUnitEditorScene` の JSON 編集で `Collider` component と `Trigger` component を扱うための、
最小テンプレート集。

現状の `PlayTestScene` では、次の 2 系統が確認対象になっている。

- pointer 系: `pointerEnter`, `pointerMove`, `pointerLeave`, `pointerDown`, `pointerUp`, `click`
- overlap 系: `Controller + Collider` を持つ object と、`Trigger + Collider` を持つ object の矩形重なり

この文書の例は、基本的に `Collider.data` または `Trigger.data` にそのまま入れる想定で記述する。

---

## Collider 基本テンプレート

```json
{
  "shape": "rect",
  "offsetX": 0,
  "offsetY": 0,
  "width": 1,
  "height": 1,
  "isTrigger": false
}
```

- `shape`: 現状は `"rect"` のみ
- `offsetX`, `offsetY`: `Transform` からのローカルオフセット
- `width`, `height`: 判定矩形サイズ
- `isTrigger`: `true` のとき trigger 判定用として扱う

---

## Trigger 基本テンプレート

```json
{
  "eventId": "ev_test",
  "triggerOn": "click",
  "once": false,
  "targetObjectId": ""
}
```

- `eventId`: 発火時に識別子として使う文字列
- `triggerOn`: `pointerEnter`, `pointerMove`, `pointerLeave`, `pointerDown`, `pointerUp`, `click`, `overlap`
- `once`: `true` のとき一度発火したら同じ条件では再発火しない
- `targetObjectId`: `overlap` 時に対象を特定 object に絞りたい場合に使う

---

## 例1: 最小の click ボタン

用途: pointer click が反応するか確認したいとき

`Collider.data`

```json
{
  "shape": "rect",
  "offsetX": 0,
  "offsetY": 0,
  "width": 120,
  "height": 40,
  "isTrigger": true
}
```

`Trigger.data`

```json
{
  "eventId": "ev_button_click",
  "triggerOn": "click",
  "once": false,
  "targetObjectId": ""
}
```

`Transform.data`

```json
{
  "x": 40,
  "y": 40,
  "z": 0,
  "rotation": 0,
  "scaleX": 1,
  "scaleY": 1
}
```

---

## 例2: hover 開始だけ反応する領域

用途: マウスカーソルが入った瞬間だけ反応するか確認したいとき

`Collider.data`

```json
{
  "shape": "rect",
  "offsetX": 0,
  "offsetY": 0,
  "width": 96,
  "height": 32,
  "isTrigger": true
}
```

`Trigger.data`

```json
{
  "eventId": "ev_hover_start",
  "triggerOn": "pointerEnter",
  "once": false,
  "targetObjectId": ""
}
```

---

## 例3: 一度だけ反応する click

用途: 初回だけ押せるボタン、チュートリアル確認用

`Trigger.data`

```json
{
  "eventId": "ev_open_once",
  "triggerOn": "click",
  "once": true,
  "targetObjectId": ""
}
```

---

## 例4: overlap 用のイベント領域

用途: プレイヤーが領域に入ったら発火するか確認したいとき

この例では、イベント object 側に `Transform + Collider + Trigger`、
プレイヤー側に `Transform + Controller + Collider` が必要。

イベント object の `Collider.data`

```json
{
  "shape": "rect",
  "offsetX": 0,
  "offsetY": 0,
  "width": 64,
  "height": 64,
  "isTrigger": true
}
```

イベント object の `Trigger.data`

```json
{
  "eventId": "ev_overlap_test",
  "triggerOn": "overlap",
  "once": false,
  "targetObjectId": ""
}
```

プレイヤー object の `Collider.data`

```json
{
  "shape": "rect",
  "offsetX": 0,
  "offsetY": 0,
  "width": 16,
  "height": 16,
  "isTrigger": false
}
```

プレイヤー object の `Controller.data`

```json
{
  "inputMode": "player1",
  "moveSpeed": 120
}
```

---

## 例5: 特定 object だけで overlap 発火

用途: プレイヤーだけ反応し、他の controller object では反応させたくないとき

`Trigger.data`

```json
{
  "eventId": "ev_player_only",
  "triggerOn": "overlap",
  "once": false,
  "targetObjectId": "obj_player"
}
```

---

## 例6: Trigger 矩形を少し前へずらす

用途: object の足元や前方に当たり判定を置きたいとき

`Collider.data`

```json
{
  "shape": "rect",
  "offsetX": 12,
  "offsetY": 0,
  "width": 24,
  "height": 24,
  "isTrigger": true
}
```

`Transform.data.x`, `Transform.data.y` の位置そのものではなく、
少しずれた位置に判定領域を置ける。

---

## 例7: UI ボタン風の押下領域

用途: `Image` や `Text` と重ねて UI ライクに使うとき

`Transform.data`

```json
{
  "x": 160,
  "y": 120,
  "z": 0,
  "rotation": 0,
  "scaleX": 1,
  "scaleY": 1
}
```

`Collider.data`

```json
{
  "shape": "rect",
  "offsetX": 0,
  "offsetY": 0,
  "width": 96,
  "height": 32,
  "isTrigger": true
}
```

`Trigger.data`

```json
{
  "eventId": "ev_start_game",
  "triggerOn": "click",
  "once": false,
  "targetObjectId": ""
}
```

---

## 補足

- `Trigger` を動かすには、同じ object に `Transform` と `Collider` が必要
- pointer 系を確認する場合は `Collider.data.isTrigger` を `true` にする
- `overlap` を確認する場合は、相手側にも `Controller + Collider` が必要
- 現状の `PlayTestScene` では `shape: "rect"` のみ対応
- `overlap` は重なり開始時に 1 回発火し、離れてから再度入ると再発火する
- `once: true` なら、同じ trigger 条件で 1 回だけ発火する