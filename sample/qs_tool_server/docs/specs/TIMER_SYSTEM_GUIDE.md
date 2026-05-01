# タイマーシステム実装ガイド

最終更新: 2026-05-01

## 目的

`qs_tool_server` のゲーム実行時（PlayTestScene）にタイマー機能を提供し、
時間ベースのイベント駆動を可能にする。

## システム構成

### 1. Global Variable `system.fixed.timer`

- **初期値**: `0`
- **型**: `number`（秒単位の浮動小数点数）
- **更新タイミング**: PlayTestScene.update() で毎フレーム加算
- **用途**: ゲーム開始からの経過時間トラッキング

### 2. Trigger タイプの拡張

#### triggerOn の新しい値

| 値 | 説明 | Collider 必要 | 発火タイミング |
|---|---|---|---|
| `onTimer` | 指定時間経過時 | 不要 | duration に達した時点で1回 |
| `onUpdate` | 毎フレーム | 不要 | 毎フレーム（条件判定毎回） |

#### Trigger.data の新しいプロパティ

```typescript
{
  eventId: string;           // 発火イベントID
  triggerOn: string;         // 'onTimer' | 'onUpdate' | ... 既存値
  duration?: number;         // onTimer 使用時の待機秒数（デフォルト: 1）
  once?: boolean;            // true なら1回発火後、同じ条件では再発火しない
  targetObjectId?: string;   // overlap 時の対象絞り込み用
}
```

## 使用パターン

### パターン1: ゲーム開始後一定時間で何かを起動

**用途**: 「3秒後に敵が出現」などのステージ進行

```json
{
  "id": "obj_spawn_timer",
  "name": "SpawnTimer",
  "enabled": true,
  "parentId": "",
  "children": [],
  "components": [
    {
      "type": "Trigger",
      "enabled": true,
      "data": {
        "eventId": "ev_enemy_spawn",
        "triggerOn": "onTimer",
        "duration": 3,
        "once": true
      }
    }
  ]
}
```

このトリガーは PlayUnit の任意の object に追加可能。Collider や Transform は不要。

### パターン2: 毎フレーム何かをチェック（Conditional と組み合わせ）

**用途**: 「毎フレーム経過時間をチェックして、5秒経ったら何か起きる」

```json
{
  "id": "obj_update_checker",
  "name": "UpdateChecker",
  "enabled": true,
  "components": [
    {
      "type": "Trigger",
      "enabled": true,
      "data": {
        "eventId": "ev_frame_check",
        "triggerOn": "onUpdate"
      }
    }
  ]
}
```

このトリガーが毎フレーム `ev_frame_check` を発火し、
Conditional component がそのイベントをリッスンして条件判定を実行。

#### Conditional の条件例（global variable 参照）

```json
{
  "type": "Conditional",
  "enabled": true,
  "data": {
    "listenTo": "ev_frame_check",
    "branches": [
      {
        "condition": {
          "type": "compare",
          "left": {
            "type": "globalVariable",
            "path": "system.fixed.timer"
          },
          "operator": ">=",
          "right": {
            "type": "literal",
            "value": 5
          }
        },
        "eventId": "ev_time_five_seconds"
      }
    ]
  }
}
```

### パターン3: 複数の時間経過マイルストーン

```json
{
  "type": "Conditional",
  "enabled": true,
  "data": {
    "listenTo": "ev_frame_check",
    "branches": [
      {
        "condition": {
          "type": "compare",
          "left": { "type": "globalVariable", "path": "system.fixed.timer" },
          "operator": "==",
          "right": { "type": "literal", "value": 3 }
        },
        "eventId": "ev_milestone_3s"
      },
      {
        "condition": {
          "type": "compare",
          "left": { "type": "globalVariable", "path": "system.fixed.timer" },
          "operator": "==",
          "right": { "type": "literal", "value": 10 }
        },
        "eventId": "ev_milestone_10s"
      }
    ]
  }
}
```

## Global Variable 参照方法

### PlayTestScene 内での直接参照

AppData の runtime global variables：

```javascript
const timer = appData.globalVariableState.system.fixed.timer;
console.log(`現在の経過時間: ${timer}秒`);
```

### Conditional の条件式での参照

```json
{
  "type": "globalVariable",
  "path": "system.fixed.timer"
}
```

## 実装内部

### timer 値の更新

PlayTestScene.update(dt) の冒頭で：

```javascript
if (this._appData?.globalVariableState?.system?.fixed && Number.isFinite(Number(dt))) {
  const deltaSeconds = Math.max(0, Number(dt) / 1000);
  this._appData.globalVariableState.system.fixed.timer += deltaSeconds;
}
```

- `dt` はミリ秒単位（RAF から取得）
- 秒に変換して累積加算

### onTimer トリガーの評価

`_processTimerTriggers(dt, playUnit)` で：

1. 各 object の Trigger コンポーネントを走査
2. `triggerOn === 'onTimer'` の場合、duration に到達したか確認
3. 到達したら `_fireTrigger()` を呼び出して eventId を pending へ追加
4. 時間をリセット（`once: true` でない場合）

### onUpdate トリガーの評価

`_processTimerTriggers(dt, playUnit)` で：

1. `triggerOn === 'onUpdate'` の場合、毎フレーム `_fireTrigger()` を呼び出す
2. eventId を pending へ追加

## よくある使用例

### 例1: ゲーム時間制限（最大60秒）

```json
{
  "type": "Trigger",
  "data": {
    "eventId": "ev_time_up",
    "triggerOn": "onTimer",
    "duration": 60,
    "once": true
  }
}
```

```json
{
  "type": "Conditional",
  "data": {
    "listenTo": "ev_time_up",
    "branches": [
      {
        "condition": {
          "type": "literal",
          "value": true
        },
        "eventId": "ev_game_over"
      }
    ]
  }
}
```

### 例2: 敵の定期的な射撃

```json
{
  "type": "Trigger",
  "data": {
    "eventId": "ev_shoot_cycle",
    "triggerOn": "onTimer",
    "duration": 1.5,
    "once": false
  }
}
```

1.5秒ごとに `ev_shoot_cycle` を発火（once: false で何度も）

### 例3: スコア表示の時間更新

```json
{
  "id": "obj_score_display",
  "components": [
    {
      "type": "Trigger",
      "data": {
        "eventId": "ev_update_score_display",
        "triggerOn": "onUpdate"
      }
    },
    {
      "type": "Text",
      "data": {
        "text": "Time: {timer_display}",
        "font": "16px sans-serif",
        "color": "#ffffff"
      }
    }
  ]
}
```

毎フレーム Text の内容を更新する EventAction で、timer 値を表示フォーマットへ。

## 設計の特徴

### 予約変数アプローチ

- `system.fixed.timer` はシステムで予約・管理
- ユーザーは直接触らず、Global Variable として参照のみ
- 将来的に `system.persistent.timer` による進行状態の永続化も可能

### 疎結合な時間駆動

- Trigger が時間イベントを発火
- Conditional / EventAction でそれを受け取って分岐・処理
- Component 間の依存度が低い

### Collider 不要

- `onTimer` / `onUpdate` はコライダー判定不要
- 空の object でも Trigger コンポーネントがあれば動作
- ステージ進行・UI更新・音声再生などを timer 発火で制御可能

## 制限事項

### 現状未対応

- timer の一時停止 / リセット（今後の拡張候補）
- 複数 timer の管理（必要なら user.fixed に独自 timer を定義可能）
- timer に紐づいた自動アニメーション（Animator component の実装待ち）

### パフォーマンス

- onUpdate は毎フレーム実行されるため、複雑な条件判定は avoid
- 大量の timer トリガーが同時実行される場合は、Conditional の条件を簡潔に

## 今後の拡張案

1. **Timer リセット / 一時停止**: EventAction の新しい action 追加
2. **複数 timer**: user.fixed に名前付き timer を追加可能
3. **Animator component**: timer ベースのアニメーション再生
4. **Timer イベント**: timer 達成時の自動イベントチェーン
