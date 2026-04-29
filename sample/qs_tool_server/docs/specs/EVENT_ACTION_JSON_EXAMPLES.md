# EventAction JSON 例集

最終更新: 2026-04-29

`+EventAction` ボタンで追加後、`edit` ボタンから JSON を貼り付けて使用する。
`listenTo` には Trigger の `eventId` と同じ文字列を指定する。

---

## 例1: ボタンクリックでテキストを書き換える

**目的**: 画面内のボタン（Trigger + Collider）をクリックしたら、別オブジェクトの Text を変更する

### ボタンオブジェクト

`Trigger.data`
```json
{
  "eventId": "ev_btn_click",
  "triggerOn": "click",
  "once": false,
  "targetObjectId": ""
}
```

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

### ラベルオブジェクト（EventAction を持つ）

`EventAction.data`
```json
{
  "listenTo": "ev_btn_click",
  "action": "setProperty",
  "targetObjectId": "ラベルオブジェクトのID",
  "componentType": "Text",
  "property": "text",
  "value": "クリックされました！"
}
```

---

## 例2: クリックでオブジェクトを表示 / 非表示にする

**目的**: ボタンクリックで別オブジェクトを消す（または表示する）

`EventAction.data`（非表示にする場合）
```json
{
  "listenTo": "ev_btn_hide",
  "action": "setEnabled",
  "targetObjectId": "対象オブジェクトのID",
  "enabled": false
}
```

`EventAction.data`（再表示する場合）
```json
{
  "listenTo": "ev_btn_show",
  "action": "setEnabled",
  "targetObjectId": "対象オブジェクトのID",
  "enabled": true
}
```

---

## 例3: アルファ値をフェードインさせる（playTween）

**目的**: イベント発火時に Rectangle の fillAlpha を 0 → 1 で 600ms かけてフェードイン

`EventAction.data`
```json
{
  "listenTo": "ev_fade_in",
  "action": "playTween",
  "targetObjectId": "フェード対象オブジェクトのID",
  "componentType": "Rectangle",
  "property": "fillAlpha",
  "tweenFrom": 0,
  "tweenTo": 1,
  "tweenDuration": 600,
  "tweenEasing": "easeOut"
}
```

**補足**:
- `tweenEasing` は `linear` / `easeIn` / `easeOut` / `easeInOut` から選択
- 同一プロパティに対して再度 playTween を発火すると上書きされる（後勝ち）

---

## 例4: 位置を左から右へスライドさせる（playTween × Transform.x）

**目的**: イベント発火でオブジェクトを x=−100 → x=200 へ 400ms で移動

`EventAction.data`
```json
{
  "listenTo": "ev_slide_in",
  "action": "playTween",
  "targetObjectId": "移動対象オブジェクトのID",
  "componentType": "Transform",
  "property": "x",
  "tweenFrom": -100,
  "tweenTo": 200,
  "tweenDuration": 400,
  "tweenEasing": "easeInOut"
}
```

---

## 例5: イベントをチェーンする（fireEvent）

**目的**: `ev_scene_start` を受けて `ev_show_hud` → `ev_play_bgm` をまとめて発火する

```
ev_scene_start
    → EventAction(fireEvent, eventId: "ev_show_hud")
    → EventAction(fireEvent, eventId: "ev_play_bgm")
```

`EventAction.data`（HUD 表示チェーン）
```json
{
  "listenTo": "ev_scene_start",
  "action": "fireEvent",
  "eventId": "ev_show_hud"
}
```

`EventAction.data`（BGM チェーン）
```json
{
  "listenTo": "ev_scene_start",
  "action": "fireEvent",
  "eventId": "ev_play_bgm"
}
```

チェーン先で実際にオブジェクトを操作する EventAction を別途用意する。

---

## 例6: ひとつ前の PlayUnit に戻る（returnPlayUnit）

**目的**: カットイン用 PlayUnit や一時 UI 用 PlayUnit から、元の PlayUnit に戻る

前提:
- Runtime 側で `system.fixed.returnPlayUnitId` に「戻り先の PlayUnit ID」が入っている
- これは `requestedPlayUnitId` による切替や起動時選択の結果として管理される

`EventAction.data`
```json
{
  "listenTo": "ev_close_overlay",
  "action": "returnPlayUnit"
}
```

`targetObjectId` は不要。
`returnPlayUnitId` が有効なら即時にその PlayUnit へ戻る。

---

## 例7: overlap で踏んだらメッセージ表示（RPG 風）

**目的**: Controller を持つキャラが特定ゾーンに重なったらメッセージウィンドウのテキストを書き換える

### ゾーンオブジェクト（Controller キャラに踏まれる側）

`Collider.data`
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

`Trigger.data`
```json
{
  "eventId": "ev_zone_entered",
  "triggerOn": "overlap",
  "once": true,
  "targetObjectId": ""
}
```

### メッセージウィンドウ（EventAction を持つ）

`EventAction.data`
```json
{
  "listenTo": "ev_zone_entered",
  "action": "setProperty",
  "targetObjectId": "メッセージTextオブジェクトのID",
  "componentType": "Text",
  "property": "text",
  "value": "ここは古い遺跡の入り口だ。"
}
```

`once: true` により、最初の1回だけ発火する。

---

## 例8: ホバーで色変え → 離れたら戻す

**目的**: ボタンにカーソルが乗ったら Rectangle の色を変え、離れたら元に戻す

### ホバー開始

`Trigger.data`（pointerEnter 用）
```json
{
  "eventId": "ev_btn_hover_enter",
  "triggerOn": "pointerEnter",
  "once": false,
  "targetObjectId": ""
}
```

`EventAction.data`
```json
{
  "listenTo": "ev_btn_hover_enter",
  "action": "setProperty",
  "targetObjectId": "ボタンRectangleオブジェクトのID",
  "componentType": "Rectangle",
  "property": "fillColor",
  "value": "#0ea5e9"
}
```

### ホバー終了

`Trigger.data`（pointerLeave 用）
```json
{
  "eventId": "ev_btn_hover_leave",
  "triggerOn": "pointerLeave",
  "once": false,
  "targetObjectId": ""
}
```

`EventAction.data`
```json
{
  "listenTo": "ev_btn_hover_leave",
  "action": "setProperty",
  "targetObjectId": "ボタンRectangleオブジェクトのID",
  "componentType": "Rectangle",
  "property": "fillColor",
  "value": "#1e293b"
}
```

**注意**: Trigger と EventAction は同じオブジェクトに置いても、別オブジェクトに分けても動作する。
ただし Trigger(pointerEnter) と Trigger(pointerLeave) を同一オブジェクトに共存させるには、
それぞれ `+Trigger` できないため、別の EventAction オブジェクトに分けることを推奨する。

---

## 例9: フェードアウト後にオブジェクトを非表示にする（chained tween + setEnabled）

**目的**: `ev_dismiss` 受信 → 0.5s でアルファ 0 → 非表示

```
ev_dismiss
    → playTween(alpha: 1→0, 500ms, easeIn)
    → fireEvent("ev_dismiss_done") ← ※現バージョンでは tween 完了後の自動発火は未実装
```

現バージョンでは tween 完了イベントの自動発火は未実装のため、
タイミングが許容できるシーンでは直接 `setEnabled: false` と `playTween` を同時に走らせる回避策が使える。

`EventAction.data`（フェードと同時に非表示）
```json
{
  "listenTo": "ev_dismiss",
  "action": "playTween",
  "targetObjectId": "対象オブジェクトのID",
  "componentType": "Rectangle",
  "property": "fillAlpha",
  "tweenFrom": 1,
  "tweenTo": 0,
  "tweenDuration": 500,
  "tweenEasing": "easeIn"
}
```

> **TODO**: tween 完了後に自動で次の eventId を発火する `onComplete` フィールドを将来追加予定。

---

## EventAction テンプレート（空欄付き）

`+EventAction` で追加した直後の初期 data。

```json
{
  "listenTo": "",
  "action": "setProperty",
  "targetObjectId": "",
  "componentType": "Text",
  "property": "text",
  "value": "",
  "enabled": true,
  "eventId": "",
  "tweenDuration": 500,
  "tweenFrom": 0,
  "tweenTo": 1,
  "tweenEasing": "linear"
}
```

`action` を変更する際は不要なフィールドを残しても動作に影響はないが、可読性のため不要フィールドは削除することを推奨。
