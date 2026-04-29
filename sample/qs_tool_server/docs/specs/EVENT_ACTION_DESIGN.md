# EventAction コンポーネント 設計書

最終更新: 2026-04-29

---

## 概要

`EventAction` は、Trigger コンポーネントが発火した `eventId` を受け取り、
指定オブジェクトのプロパティ変更・表示切替・アニメーション・イベントチェーンなどを実行するコンポーネント。

同一オブジェクトに複数付与可能。
Trigger とは別オブジェクトに置くことも、同一オブジェクトに混在させることもできる。

---

## 既存システムとの関係

```
Trigger コンポーネント
    └─ triggerOn 条件成立 (click / overlap / pointerEnter など)
    └─ _fireTrigger() → _pendingEvents に eventId を登録
            ↓
PlayTestScene._processEventActions()
    └─ 全オブジェクトの EventAction を走査
    └─ listenTo === eventId のものを _executeAction() で実行
            ↓
アクション実行（setProperty / setEnabled / playTween / fireEvent / returnPlayUnit）
            ↓
PlayTestScene._updateTweens() (playTween の場合、毎フレーム値を補間)
```

---

## EventAction コンポーネント データ仕様

| フィールド | 型 | 必須 | 説明 |
|---|---|---|---|
| `listenTo` | string | ○ | 反応するイベント ID |
| `action` | string | ○ | `setProperty` / `setEnabled` / `playTween` / `fireEvent` / `returnPlayUnit` |
| `targetObjectId` | string | action 依存 | 操作対象オブジェクトの ID。`fireEvent` と `returnPlayUnit` では不要 |

### action = `setProperty` 追加フィールド

| フィールド | 型 | 説明 |
|---|---|---|
| `componentType` | string | 対象コンポーネント型 (例: `"Text"`, `"Rectangle"`, `"Transform"`) |
| `property` | string | 変更するプロパティ名 |
| `value` | string | JSON としてパース試行。失敗時は文字列そのまま |

### action = `setEnabled` 追加フィールド

| フィールド | 型 | 説明 |
|---|---|---|
| `enabled` | boolean | `false` で非表示・無効化。`true` で有効化 |

### action = `playTween` 追加フィールド

| フィールド | 型 | 既定値 | 説明 |
|---|---|---|---|
| `componentType` | string | — | 対象コンポーネント型 |
| `property` | string | — | アニメーションするプロパティ名 |
| `tweenDuration` | number | 500 | 所要時間 (ms) |
| `tweenFrom` | number | 0 | 開始値 |
| `tweenTo` | number | 1 | 終了値 |
| `tweenEasing` | string | `"linear"` | `linear` / `easeIn` / `easeOut` / `easeInOut` |

イージング式:
- `linear`: $t$
- `easeIn`: $t^2$
- `easeOut`: $1 - (1-t)^2$
- `easeInOut`: $t < 0.5$ → $2t^2$ / $t \ge 0.5$ → $1 - 2(1-t)^2$

同一 `targetObjectId + componentType + property` の組み合わせで上書きされる（後勝ち）。

### action = `fireEvent` 追加フィールド

| フィールド | 型 | 説明 |
|---|---|---|
| `eventId` | string | チェーン発火させるイベント ID |

循環参照ガード: 同一フレーム内で同一 eventId は最大1回のみ処理。
フレームをまたいだループは発生しない。

### action = `returnPlayUnit` 追加フィールド

追加フィールドなし。

Runtime global variable `system.fixed.returnPlayUnitId` を参照し、その PlayUnit が有効なら即時切替する。
切替成功時は `currentPlayUnitId` が切り替わり、`returnPlayUnitId` には切替前の PlayUnit ID が入るため、同じ action を再度実行すると往復移動にも使える。

---

## 実行順序

1. `_fireTrigger()` が呼ばれるたびに `_pendingEvents` に eventId を追加（重複排除）
2. `update(dt)` 内で `_processEventActions()` が実行される
3. `fireEvent` で追加された eventId も同一フレーム内で処理される（最大 64 パス）
4. `playTween` で登録された tween は `_updateTweens(dt)` が毎フレーム補間して反映
5. `returnPlayUnit` は実行フレーム内で即時に active PlayUnit を差し替える

---

## 制限事項（現バージョン）

- `playTween` はスカラー値（数値）のみ対応。色文字列などには使用不可
- `setProperty` の `value` は JSON パースを試みるが、複雑なオブジェクトより `string` / `number` / `boolean` を推奨
- `setEnabled` は `PlayTestScene` の描画ループが `objectData.enabled === false` をスキップすることで非表示を実現しているため、`PlayUnitRuntime.fromPlayUnit()` を経由して反映される
- `returnPlayUnit` は `system.fixed.returnPlayUnitId` が空または無効 ID の場合は何も切り替えず、エラーステータスのみ表示する
- EventAction 自体は `PlayUnitData` に保存・ロードされる通常コンポーネントなので、プロジェクト保存でそのまま永続化される
