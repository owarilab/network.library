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
---

## 例13: グローバル変数を読む（readGlobalVariable）

**目的**: `user.fixed` / `user.persistent` の値を読み取り、表示や後続処理に使う

`EventAction.data`
```json
{
  "listenTo": "ev_show_score",
  "action": "readGlobalVariable",
  "variablePath": "user.persistent.score",
  "targetObjectId": "表示先オブジェクトのID",
  "componentType": "Text",
  "property": "text"
}
```

**補足**:
- `variablePath` は `system.fixed.*` / `system.persistent.*` / `user.fixed.*` / `user.persistent.*` 形式
- 取得結果は `targetObjectId` の `componentType.property` に反映できる
- `targetObjectId` を省略した場合は、runtime 側の一時結果として扱う実装もある

---

## 例14: グローバル変数に書き込む（setGlobalVariable）

**目的**: クリック時に `user.persistent.score` を更新し、別の `user.fixed` へコピーする

`EventAction.data`（リテラル代入）
```json
{
  "listenTo": "ev_add_score",
  "action": "setGlobalVariable",
  "variablePath": "user.persistent.score",
  "valueSource": "literal",
  "value": "11"
}
```

`EventAction.data`（別変数から代入）
```json
{
  "listenTo": "ev_copy_score",
  "action": "setGlobalVariable",
  "variablePath": "user.fixed.counter",
  "valueSource": "variable",
  "valueVariablePath": "user.persistent.score"
}
```

`EventAction.data`（別変数を加算：スコア加算など）
```json
{
  "listenTo": "ev_add_score",
  "action": "setGlobalVariable",
  "variablePath": "user.persistent.score",
  "valueSource": "variable",
  "valueVariablePath": "player.level",
  "op": "add"
}
```

`EventAction.data`（リテラル値で減算：HP減算など）
```json
{
  "listenTo": "ev_take_damage",
  "action": "setGlobalVariable",
  "variablePath": "user.persistent.hp",
  "valueSource": "literal",
  "value": 10,
  "op": "subtract"
}
```

**補足**:
- `valueSource` は `literal` / `variable`
- `literal` の `value` は JSON として解釈され、失敗した場合は文字列として扱われる
- `variable` の場合は `valueVariablePath` が必要
- `op` は `set`（デフォルト）/ `add` / `subtract` / `multiply` / `divide`
- 算術演算（add/subtract/multiply/divide）は変数の type が `number` のみ有効

## 例15: 順次実行（sequence）で会話を進める

窓をフェードインし、文を 1 つずつ出す。`typewriter` は（`waitComplete` により）全文が出るまで止まり、`waitEvent` は `msg.next` が発火するまで止まる。`msg.next` はクリックを受ける Conditional が発火する。最後の `waitEvent` に `timeout` を付けると、クリックが無くても 3 秒後に閉じる（自動送りの形）。

```json
{
  "type": "EventAction",
  "enabled": true,
  "data": {
    "listenTo": "msg.show",
    "action": "sequence",
    "onBusy": "ignore",
    "completeEventId": "msg.done",
    "steps": [
      { "action": "playTween", "targetObjectId": "obj_msg_box", "componentType": "NineSlice", "property": "alpha", "tweenFrom": 0, "tweenTo": 1, "tweenDuration": 300, "waitComplete": true },
      { "action": "typewriter", "targetObjectId": "obj_msg_body", "componentType": "Text", "valueSource": "literal", "value": "一行目です。", "charsPerSecond": 24, "waitComplete": true },
      { "action": "waitEvent", "eventId": "msg.next" },
      { "action": "typewriter", "targetObjectId": "obj_msg_body", "componentType": "Text", "valueSource": "literal", "value": "二行目です。", "charsPerSecond": 24, "waitComplete": true },
      { "action": "waitEvent", "eventId": "msg.next", "timeout": 3 },
      { "action": "setEnabled", "targetObjectId": "obj_msg_box", "enabled": false }
    ]
  }
}
```

## 例16: 扉で別マップへ移動する（warp、フェード付き）

前提:
- 行き先の PlayUnit（室内）に、同じ ID の操作対象（`obj_player`）と出現位置（`obj_spawn_from_field`）がある
- 両方の PlayUnit に、フェード用の黒い幕がある（UICanvas の子に全画面の Rectangle。`fillAlpha` は 0）
- 扉は overlap の Trigger（Collider の `isTrigger: true`）

### 扉（Field 側）

```json
{ "type": "Trigger", "enabled": true, "data": { "eventId": "door.house", "triggerOn": "overlap", "once": false, "targetObjectId": "" } }
```

### 扉を踏んだら幕を閉じてから移動（Field 側の UICanvas `obj_field_fade` が持つ）

```json
{
  "type": "EventAction",
  "enabled": true,
  "data": {
    "listenTo": "door.house",
    "action": "sequence",
    "targetObjectId": "",
    "onBusy": "ignore",
    "steps": [
      { "action": "playTween", "targetObjectId": "obj_field_fade_box", "componentType": "Rectangle", "property": "fillAlpha", "tweenFrom": 0, "tweenTo": 1, "tweenDuration": 250, "tweenEasing": "linear", "waitComplete": true },
      { "action": "warp", "playUnitId": "pu_character_walk_house", "spawnObjectId": "obj_spawn_from_field", "facing": "up" }
    ]
  }
}
```

### 到着したら幕を開ける（行き先の UICanvas `obj_house_fade` が持つ）

```json
{
  "type": "EventAction",
  "enabled": true,
  "data": {
    "listenTo": "system.playUnitEnter",
    "action": "playTween",
    "targetObjectId": "obj_house_fade_box",
    "componentType": "Rectangle",
    "property": "fillAlpha",
    "tweenFrom": 1,
    "tweenTo": 0,
    "tweenDuration": 250,
    "tweenEasing": "easeOut",
    "completeEventId": ""
  }
}
```

**補足**:
- 出口は同じ形で、`listenTo: "house.exit"`、`playUnitId: "pu_character_walk_field"`、`spawnObjectId: "obj_spawn_from_house"`、`facing: "down"` にする
- `warp` は sequence の最後に置く（後ろの step は旧 PlayUnit の状態で走る）
- 出現位置は通行可能なマスに置く。実際のデモは `tools/add_map_transition_to_demo.py` で作れる

---

## 例17: データベースの値を使う（HP の回復と罠のダメージ）

前提: 可変 DB `party`（行 `hero` に `hp` / `maxHp`）と、定数 DB `items`（行 `potion` に `heal`）、`hazards`（行 `spike` に `damage`）がある。デモは `tools/add_database_to_demo.py` で作れる。

### HP の表示（Text。`${…}` に DB のパスを書ける）

```json
{ "type": "Text", "enabled": true, "data": { "text": "${db.mutable.party.hero.label}  HP ${db.mutable.party.hero.hp} / ${db.mutable.party.hero.maxHp}" } }
```

### ポーション: HP が最大より少ないときだけ、定数 DB の回復量を足す

```json
{
  "type": "Conditional",
  "enabled": true,
  "data": {
    "listenTo": "hud.potion",
    "branches": [{
      "condition": { "type": "compare", "left": "${db.mutable.party.hero.hp}", "operator": "<", "right": "${db.mutable.party.hero.maxHp}" },
      "action": {
        "action": "sequence",
        "steps": [
          { "action": "setGlobalVariable", "variablePath": "db.mutable.party.hero.hp", "op": "add", "valueSource": "variable", "valueVariablePath": "db.const.items.potion.heal" },
          { "action": "fireEvent", "eventId": "party.clamp" }
        ]
      }
    }]
  }
}
```

### HP を 0 〜 最大 HP に収める

```json
{
  "type": "Conditional",
  "enabled": true,
  "data": {
    "listenTo": "party.clamp",
    "branches": [
      { "condition": { "type": "compare", "left": "${db.mutable.party.hero.hp}", "operator": ">", "right": "${db.mutable.party.hero.maxHp}" },
        "action": { "action": "setGlobalVariable", "variablePath": "db.mutable.party.hero.hp", "op": "set", "valueSource": "variable", "valueVariablePath": "db.mutable.party.hero.maxHp" } },
      { "condition": { "type": "compare", "left": "${db.mutable.party.hero.hp}", "operator": "<", "right": 0 },
        "action": { "action": "setGlobalVariable", "variablePath": "db.mutable.party.hero.hp", "op": "set", "valueSource": "literal", "value": 0 } }
    ]
  }
}
```

**補足**:
- `db.const` と `db.system` は読み取り専用。書き込むとエラーになる
- 可変 DB は Play Test を始めるたびに、定義の初期値へ戻る
- 存在確認は Conditional の `exists`（例: `db.const.items.potion`）

---

## 4. Conditional Component

複数の条件分岐を順序付きで評価し、最初にマッチした分岐のアクションを実行します（短絡評価）。

`Conditional.data`（基本的な if-else 分岐）
```json
{
  "listenTo": "ev_level_up",
  "branches": [
    {
      "condition": {
        "type": "compare",
        "left": "${user.persistent.level}",
        "operator": ">=",
        "right": 10
      },
      "action": {
        "action": "setGlobalVariable",
        "variablePath": "user.persistent.battleAction",
        "valueSource": "literal",
        "value": "ULTIMATE_SKILL",
        "op": "set"
      }
    },
    {
      "condition": {
        "type": "compare",
        "left": "${user.persistent.level}",
        "operator": ">=",
        "right": 5
      },
      "action": {
        "action": "setGlobalVariable",
        "variablePath": "user.persistent.battleAction",
        "valueSource": "literal",
        "value": "SPECIAL_SKILL",
        "op": "set"
      }
    }
  ],
  "defaultAction": {
    "action": "setGlobalVariable",
    "variablePath": "user.persistent.battleAction",
    "valueSource": "literal",
    "value": "NORMAL_ATTACK",
    "op": "set"
  }
}
```

`Conditional.data`（複数条件型：truthy + has）
```json
{
  "listenTo": "ev_item_use",
  "branches": [
    {
      "condition": {
        "type": "truthy",
        "left": "${user.persistent.hasPoison}"
      },
      "action": {
        "action": "setGlobalVariable",
        "variablePath": "user.persistent.itemUsed",
        "valueSource": "literal",
        "value": "POISON_APPLIED",
        "op": "set"
      }
    },
    {
      "condition": {
        "type": "has",
        "left": "${user.persistent.inventory}",
        "right": "antidote"
      },
      "action": {
        "action": "setGlobalVariable",
        "variablePath": "user.persistent.itemUsed",
        "valueSource": "literal",
        "value": "ANTIDOTE_USED",
        "op": "set"
      }
    }
  ],
  "defaultAction": {
    "action": "setGlobalVariable",
    "variablePath": "user.persistent.itemUsed",
    "valueSource": "literal",
    "value": "NO_SUITABLE_ITEM",
    "op": "set"
  }
}
```

**Conditional 仕様**:
- `listenTo`: イベント ID（EventAction と同じ）
- `branches`: 条件分岐の配列（順序が重要、最初のマッチのみ実行）
  - 各 branch は `condition` と `action` を含む
  - `condition`: 評価する条件オブジェクト
  - `action`: マッチ時に実行する EventAction 形式のアクション
- `defaultAction`: 全分岐がマッチしなかった場合に実行（オプション）
- **評価順序**: branches 配列の順序で評価。最初にマッチした分岐のアクションを実行して終了
- **No Match 時**: defaultAction が存在すれば実行、なければ何もしない

**Condition Types**:
| Type | left | operator/right | 説明 |
|------|------|---|---|
| compare | 値/変数 | `>` `>=` `<` `<=` `===` `!==` | 数値比較 |
| equals | 値/変数 | right: 文字列 | 文字列一致（大小文字区別） |
| truthy | 値/変数 | なし | JavaScript 的な真値判定 |
| has | オブジェクト/変数 | right: プロパティ名 | オブジェクトにプロパティが存在するか |
| exists | 変数パス文字列 | なし | グローバル変数が存在するか |

**例1: HP が低いと逃げる**
```json
{
  "condition": {
    "type": "compare",
    "left": "${user.persistent.hp}",
    "operator": "<",
    "right": 20
  },
  "action": { "action": "setGlobalVariable", "variablePath": "user.persistent.state", "value": "FLEE", "op": "set" }
}
```

**例2: 状態が「中毒」かどうか確認**
```json
{
  "condition": {
    "type": "equals",
    "left": "${user.persistent.status}",
    "right": "poison"
  },
  "action": { "action": "setGlobalVariable", "variablePath": "user.persistent.battleAction", "value": "USE_ANTIDOTE", "op": "set" }
}
```

**例3: シールドを持っているか確認**
```json
{
  "condition": {
    "type": "truthy",
    "left": "${user.persistent.hasShield}"
  },
  "action": { "action": "setGlobalVariable", "variablePath": "user.persistent.defense", "value": 5, "op": "add" }
}
```

