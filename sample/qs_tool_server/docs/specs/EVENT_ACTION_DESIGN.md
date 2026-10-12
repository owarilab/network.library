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
| `action` | string | ○ | `setProperty` / `setEnabled` / `playTween` / `fireEvent` / `returnPlayUnit` / `setGlobalVariable` / `typewriter` / `sequence` |
| `targetObjectId` | string | action 依存 | 操作対象オブジェクトの ID。`fireEvent`、`returnPlayUnit`、`setGlobalVariable`、`sequence` では不要 |

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
| `completeEventId` | string | — | 任意。自然終了したときに一度だけ発火する event ID（次のフレームで処理）。同じキーの tween に置き換えられた場合は発火しない |

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

### action = `typewriter` 追加フィールド

文字列を一定速度で Text に表示する。全文表示後に完了イベントを発火する。詳細は [MESSAGE_WINDOW_DESIGN.md](MESSAGE_WINDOW_DESIGN.md)。

| フィールド | 型 | 説明 |
|---|---|---|
| `targetObjectId` | string | 表示する Text を持つオブジェクト（必須） |
| `componentType` | string | `Text` 固定 |
| `valueSource` | string | `literal`（`value` の文字列）または `variable`（`valueVariablePath` の値） |
| `value` | string | `valueSource: "literal"` のときの文字列（JSON として解釈しない） |
| `valueVariablePath` | string | `valueSource: "variable"` のときのグローバル変数パス |
| `charsPerSecond` | number | 表示速度（既定 30）。0 以下は既定値 |
| `statusVariablePath` | string | 任意。`typing` / `done` を書く string 変数 |
| `completeEventId` | string | 任意。全文表示時に一度だけ発火する event ID（次のフレームで処理） |

- 表示中に再実行すると、再開始せず全文表示にして `done` にする（スキップ）。
- `done` の後の再実行は、新しい文字列で始める。
- PlayUnit を切り替えると進行中の表示は止まり、Text の表示文字数は全文に戻る。
- 変更は実行用コピーの Text（`text` と `revealChars`）にだけ書く。

### action = `sequence` 追加フィールド

`steps` の各アクションを**順番に**実行する。`wait` / `waitEvent` や `waitComplete` で止まりながら進む。
アクションを置ける場所（EventAction の `listenTo`、Conditional の `branches[].action` と `defaultAction`、`sequence` の入れ子）なら、どこにでも書ける。

| フィールド | 型 | 既定値 | 説明 |
|---|---|---|---|
| `steps` | array | — | 実行するアクションの配列（必須。無いとエラーになる）。各要素は EventAction のデータと同じ形 |
| `onBusy` | string | `"ignore"` | 実行中に再度発火したときの扱い。`ignore`: 新しい実行を捨てる。`restart`: 今の実行を止めて最初からやり直す |
| `completeEventId` | string | — | 任意。最後の step の後に一度だけ発火する event ID（次のフレームで処理） |

step に使える追加のアクション:

| step の `action` | フィールド | 動作 |
|---|---|---|
| `wait` | `duration`（秒、既定 1） | `duration` 秒だけ止まる。0 以下は止まらない |
| `waitEvent` | `eventId`、`timeout`（秒、任意） | `eventId` が発火するまで止まる。`timeout` を付けると時間切れでも進む（自動送りに使う） |

step のその他のアクション（`setGlobalVariable`、`setEnabled`、`fireEvent` など）は即時に実行して次へ進む。
`typewriter` と `playTween` は `"waitComplete": true` を付けると、その処理が終わるまで止まる。
`sequence` の step も `"waitComplete": true` で、入れ子の sequence が終わるまで止まる。

- `waitEvent` は、待ち始めた**後**に処理されたイベントだけで解ける。待ち始める前に発火したイベントは受けない。
- 同じフレームの中でも同じ規則に従う。リスナーの実行中に始まった待ちは、そのリスナーが処理しているイベント自身では解けず、次に処理されたイベントで解ける。
- 同じ `sequence` が実行中に `onBusy: "ignore"` で再発火したときは、何もせず `true` を返す（Conditional からは「実行された」と見える）。
- step が失敗（対象が無い、変数が無い、など）すると、その sequence は**中断**される。後続の step と `completeEventId` は実行されず、ステータスに step 番号が出る。
- `wait` と `waitEvent` は sequence の step の中でだけ有効。単体の EventAction に書くとエラーになる。
- 時間の単位の対応: `wait` と `waitEvent.timeout` は**秒**（`Trigger.onTimer` と同じ）。`playTween` の `tweenDuration` と `typewriter` の `charsPerSecond` は、それぞれ**ミリ秒**と**文字毎秒**。
- PlayUnit を切り替えると、進行中の sequence は止まり、再開しない（窓などの表示は残る）。

### action = `returnPlayUnit` 追加フィールド

追加フィールドなし。

Runtime global variable `system.fixed.returnPlayUnitId` を参照し、その PlayUnit が有効なら即時切替する。
切替成功時は `currentPlayUnitId` が切り替わり、`returnPlayUnitId` には切替前の PlayUnit ID が入るため、同じ action を再度実行すると往復移動にも使える。

---

## 実行順序

1. `_fireTrigger()` が呼ばれるたびに `_pendingEvents` に eventId を追加（重複排除）
2. `update(dt)` 内で `_processEventActions()` が実行される（イベントごとに、待ち（waitEvent）へ処理済みを通知してから、リスナーを実行する）
3. `fireEvent` で追加された eventId も同一フレーム内で処理される（最大 64 パス）
4. `playTween` で登録された tween は `_updateTweens(dt)` が毎フレーム補間して反映。完了時に `completeEventId` を追加する
5. `_updateTypewriters(dt)` が表示文字数を進める。全文に達したら完了イベントを追加する
6. `_updateSequences(dt)` が待ちの条件（時間、イベント、typewriter・tween・入れ子の完了）を評価し、解けた sequence を次の step へ進める
7. `returnPlayUnit` は実行フレーム内で即時に active PlayUnit を差し替え、進行中の sequence・tween・typewriter を止める
8. 完了イベント（tween・typewriter・sequence の `completeEventId`）は、追加されたフレームの次に処理される

---

## 制限事項（現バージョン）

- `playTween` はスカラー値（数値）のみ対応。色文字列などには使用不可
- `setProperty` の `value` は JSON パースを試みるが、複雑なオブジェクトより `string` / `number` / `boolean` を推奨
- `setEnabled` は `PlayTestScene` の描画ループが `objectData.enabled === false` をスキップすることで非表示を実現しているため、`PlayUnitRuntime.fromPlayUnit()` を経由して反映される
- `returnPlayUnit` は `system.fixed.returnPlayUnitId` が空または無効 ID の場合は何も切り替えず、エラーステータスのみ表示する
- EventAction 自体は `PlayUnitData` に保存・ロードされる通常コンポーネントなので、プロジェクト保存でそのまま永続化される
- sequence には繰り返し・並列（parallel）・途中の中断（stopSequence）・sequence 内の分岐はない。分岐は Conditional に任せ、`fireEvent` で渡す
- sequence の step を JSON で手書きする必要がある（専用の編集 UI はない）。`steps` が無い sequence は保存時にエラーになる
- `wait` の長さは、開始したフレームの分だけ最大 1 フレーム短くなることがある
- 同じフレームで同じ eventId を複数回発火しても、1 回にまとめて処理される
