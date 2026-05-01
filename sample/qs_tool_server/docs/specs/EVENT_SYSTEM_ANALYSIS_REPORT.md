# Event System Analysis Report

**最終更新**: 2026-05-01  
**対象バージョン**: qs_tool_server current  
**目的**: 現在のイベントシステムの実装状況を整理し、ゲーム開発に必要な機能の優先順位を決定する

---

## Executive Summary

qs_tool_server のイベントシステムは、基本的なグローバル変数 API と EventAction フレームワークが実装済み（Phase 1-5 完了）。
ただし、実用的なゲーム開発には**条件分岐・遅延実行・シーケンシング**などが不足している。
次フェーズ（Phase 6-8）では、これらの機能を段階的に追加することで、RPG/Puzzle ゲームが作成可能になる。

---

## 1. EventAction システムの実装状況

### 1.1 現在実装されているアクションタイプ

| アクション名 | 説明 | 実装状態 | 必須フィールド |
|---|---|---|---|
| **setProperty** | オブジェクトのコンポーネントプロパティを変更 | ✅ 実装済み | `targetObjectId`, `componentType`, `property`, `value` |
| **setEnabled** | オブジェクトの有効/無効を切替 | ✅ 実装済み | `targetObjectId`, `enabled` |
| **playTween** | スカラー値をアニメーション | ✅ 実装済み | `targetObjectId`, `componentType`, `property`, `tweenFrom`, `tweenTo`, `tweenDuration` |
| **fireEvent** | 別のイベントをチェーン発火 | ✅ 実装済み | `eventId` |
| **setGlobalVariable** | グローバル変数を読み書き（四則演算対応） | ✅ 実装済み | `variablePath`, `op`, `valueSource` |
| **requestPlayUnit** | PlayUnitの切替をリクエスト | ✅ 実装済み | `playUnitId` |
| **returnPlayUnit** | 前の PlayUnit に戻す | ✅ 実装済み | なし |

### 1.2 EventAction の構造

**ファイル参照**: [play_test_scene.js](../../www/js/scene/play_test_scene.js#L911)

```javascript
// EventAction.data の共通フィールド
{
  "listenTo": "event_id",        // 反応するイベントID（Triggerの eventId と同じ）
  "action": "setProperty",       // アクション種別
  "targetObjectId": "obj_id",    // 対象オブジェクトID
  // 以降は action によって異なる
}
```

### 1.3 setGlobalVariable アクションの詳細実装

**ファイル参照**: [play_test_scene.js](../../www/js/scene/play_test_scene.js#L934-L1024)

```javascript
{
  "listenTo": "ev_increment_score",
  "action": "setGlobalVariable",
  "variablePath": "user.persistent.score",    // パス形式: scope.tier.name
  "op": "add",                                 // set|add|subtract|multiply|divide
  "valueSource": "literal",                    // literal|variable
  "value": 10,                                 // リテラル値
  "valueVariablePath": "user.fixed.bonus"     // source が variable の場合のパス
}
```

**サポートされる演算子**:
- `set` - 値を上書き
- `add` - 加算
- `subtract` - 減算
- `multiply` - 乗算
- `divide` - 除算（0除算チェック有り）

---

## 2. グローバル変数 API

### 2.1 AppData 実装済みメソッド

**ファイル参照**: [app_data.js](../../www/js/app_data.js#L177-L310)

| メソッド | 戻り値 | 説明 |
|---|---|---|
| `getRuntimeGlobalVariables()` | `{ system, user }` | 全グローバル変数の runtime state を取得 |
| `getRuntimeGlobalVariable(path)` | `any` | パス形式の変数値を取得（クローン返却） |
| `setRuntimeGlobalVariable(path, value)` | `boolean` | 変数値を更新（型強制） |
| `hasRuntimeGlobalVariable(path)` | `boolean` | 変数の存在確認 |
| `deleteRuntimeGlobalVariable(path)` | `boolean` | runtime state から削除 |
| `listRuntimeGlobalVariables(scopePath)` | `Array<{name, value}>` | bucket 内の全変数を列挙 |
| `resetRuntimeGlobalVariables()` | `{ system, user }` | 定義から state を再初期化 |
| `resolveRuntimeGlobalVariablePath(path)` | `{ scope, tier, name } \| null` | パス解析 |
| `getLastRuntimeGlobalVariableError()` | `string` | 直近のエラーメッセージ |

### 2.2 変数パス形式

```
scope.tier.name
  ├─ scope: 'system' | 'user'
  ├─ tier: 'fixed' | 'persistent'
  └─ name: 変数名（英数字アンダースコア）
```

**使用例**:
- `user.persistent.score` - ユーザー定義の永続的なスコア変数
- `system.fixed.currentPlayUnitId` - システムの固定的な現在PlayUnit ID
- `user.fixed.playerName` - ユーザー定義の固定的なプレイヤー名

### 2.3 値の型変換ルール

**ファイル参照**: [project_data.js](../../www/js/project/project_data.js#L115-L160)

| 型 | 受け入れ | 変換例 |
|---|---|---|
| `string` | string, number, boolean | `10` → `"10"`, `true` → `"true"` |
| `number` | number, 数値文字列 | `"42"` → `42`, `"invalid"` → エラー |
| `boolean` | boolean, 0/1, "true"/"false" | `"true"` → `true`, `1` → `true` |
| `json` | JSON化可能な全型 | object, array, primitives |

### 2.4 バリデーション

**ファイル参照**: [project_data.js](../../www/js/project/project_data.js#L160-L192)

関数 `validateGlobalVariables()` による検証内容:
- 変数名の非空性チェック
- 型サポート確認（string/number/boolean/json のみ）
- JSON 型の serializable 検証

---

## 3. トリガーシステム

### 3.1 Trigger コンポーネント構造

| フィールド | 型 | 説明 |
|---|---|---|
| `eventId` | string | 発火時に emit するイベントID |
| `triggerOn` | string | トリガー条件（下記参照） |
| `targetObjectId` | string | overlap時の対象オブジェクト（空=任意） |
| `once` | boolean | true=一度だけ発火、false=毎回 |

### 3.2 サポートされるトリガーイベント

**ファイル参照**: [play_test_scene.js](../../www/js/scene/play_test_scene.js#L270-L900)

| イベント | 条件 | コンポーネント要件 |
|---|---|---|
| **overlap** | Controller と Trigger 領域が重なった時 | Transform + Collider + Trigger |
| **click** | ポインタが Trigger 領域内でクリック | Transform + Collider + Trigger |
| **pointerEnter** | ポインタが Trigger 領域に進入 | Transform + Collider + Trigger |
| **pointerLeave** | ポインタが Trigger 領域から脱出 | Transform + Collider + Trigger |
| **pointerDown** | Trigger 領域でマウス押下 | Transform + Collider + Trigger |
| **pointerUp** | Trigger 領域でマウス解放 | Transform + Collider + Trigger |

### 3.3 イベント伝播フロー

```
Trigger.triggerOn 条件成立
  ↓
_fireTrigger(hit, eventType)
  ↓
_pendingEvents Set に eventId を追加
  ↓
update(dt) 内で _processEventActions()
  ↓
EventAction.listenTo === eventId を探して _executeAction() 実行
```

---

## 4. テンプレート変数機能

### 4.1 Text コンポーネントでのテンプレート展開

**実装**: [play_unit_runtime.js](../../www/js/play_unit/play_unit_runtime.js#L290-L320)

Text コンポーネントで `${variable_path}` 構文を使用すると、毎フレーム値が動的に展開される。

**例**:
```javascript
{
  "type": "Text",
  "data": {
    "text": "Score: ${user.persistent.score}\nHP: ${user.persistent.hp}/100"
  }
}
```

**フォーマット**: `${scope.tier.name}`

**動作**:
- 毎フレーム `PlayUnitRuntime.fromPlayUnit()` でテンプレート展開
- 変数が見つからない場合は `${...}` がそのまま表示される
- 複数の変数を1つのテキストに含められる

---

## 5. 現状の不足点と制限事項

### 5.1 実装完了（Phase 1-5）

✅ グローバル変数 runtime API  
✅ EventAction の setGlobalVariable  
✅ 値の型強制と検証  
✅ 変数パス解決メカニズム  
✅ テンプレート変数による動的テキスト表示  

### 5.2 未着手（Phase 6-7）

❌ **保存・復除・互換性確認**
- 既存 `.qsproj` のロード確認未実施
- globalVariables 未定義プロジェクトの動作確認未実施
- 保存形式の互換性検証未実施

❌ **動作確認と最終調整**
- user.fixed / user.persistent の動作確認未実施
- EventAction からの read/write の実機確認未実施
- エラー時の UI 表示調整が未実施

### 5.3 既知の制限事項

| 項目 | 制限 |
|---|---|
| **playTween** | スカラー値（数値）のみ対応。色や複雑なオブジェクトに未対応 |
| **setProperty** | 値は JSON パースを試みるが、複雑オブジェクトより string/number/boolean 推奨 |
| **setEnabled** | 描画ループで `objectData.enabled === false` をスキップするため、runtime 反映に `PlayUnitRuntime.fromPlayUnit()` が必要 |
| **fireEvent** | 同一フレーム内の循環参照ガード: 同一 eventId は最大1回のみ処理。最大64パスまで |
| **returnPlayUnit** | `system.fixed.returnPlayUnitId` が無効な場合は何も切り替えず、エラーステータスのみ表示 |
| **グローバル変数** | project 定義に存在する変数のみ操作可能。undefined bucket への代入は拒否 |

---

## 6. ゲーム開発に必要だが未実装の機能

| 機能 | 説明 | 対応案 |
|---|---|---|
| **条件分岐（if/else）** | EventAction に分岐ロジックがない。必須。 | EventAction 拡張提案あり（Phase A） |
| **遅延実行（delay）** | アニメーション終了後の実行など必要 | EventAction に delay アクション追加 |
| **シーケンシング** | 複数アクションの順序付け実行 | EventAction 配列のサポート |
| **ローカル変数** | PlayUnit スコープの一時変数が必要 | 別レイヤーで実装予定 |
| **条件演算子** | if/else の条件として `score >= 100` のような評価 | 式評価エンジンの構築 |
| **ランダム値** | ゲーム性向上に必須 | BuiltIn function の提供 |
| **system 変数の汎用化** | user と同じ API で統一 | Phase 後期で統一予定 |

---

## 7. 優先度別実装提案

### 7.1 🔴 高優先度（即座に実装すべき）

| 機能 | 理由 | 実装難度 | ゲーム規模での影響度 |
|------|------|--------|-------|
| **条件分岐（if/else）** | 現在の EventAction は単純な実行のみ。ゲームロジックには分岐が必須。RPG/Puzzle の基盤。 | 中 | 非常に高 |
| **数値比較オペレータ** | `score >= 100` のような条件評価がないと分岐が意味ない。ボスHP判定、ゲームオーバー判定など。 | 低 | 高 |
| **delay / wait** | アニメーション遅延、シーケンシャル実行が必要。戦闘演出などで即座に必要。 | 中 | 高 |
| **ローカル変数スコープ** | PlayUnit ごと、あるいはインスタンスごとの状態が必要。敵HP、スコア加算中フラグなど。 | 高 | 高 |
| **EventAction のチェーン/順序保証** | 複数アクションを順序付けて実行できない。現在は並列実行のみで不十分。 | 中 | 高 |

### 7.2 🟡 中優先度（ゲーム規模で必要）

| 機能 | 理由 | 実装難度 | ゲーム規模での影響度 |
|------|------|--------|-------|
| **ランダム値生成** | ゲーム性に必須。敵行動、ドロップアイテム、ダイス判定など。 | 低 | 中 |
| **三項演算子 / 式評価** | `max(hp, 0)` 的な簡単な計算が必要。スコア計算など。 | 中 | 中 |
| **array / dictionary 型** | インベントリ、敵リスト、マップデータなど複雑データが必須。 | 高 | 中 |
| **イベント循環参照ガード強化** | 現在は64パスまで。より深いチェーン対応。 | 低 | 低 |

### 7.3 🟢 低優先度（拡張の余地）

| 機能 | 理由 | 実装難度 | ゲーム規模での影響度 |
|------|------|--------|-------|
| **system 変数の汎用化** | user と同じ API で統一する。一貫性向上。 | 低 | 低 |
| **persistent 差分管理** | 保存効率化。大規模プロジェクト向け。 | 中 | 低 |
| **variable UI フォーム** | JSON 直編集より使いやすく。開発体験向上。 | 高 | 低 |

---

## 8. 実装パス提案

### 8.1 Phase A: 条件分岐（優先実装）

**目標**: EventAction とは別の **Conditional** コンポーネントで、複数条件をサポートする分岐ロジックを実現

**詳細設計**: [CONDITIONAL_COMPONENT_DESIGN.md](CONDITIONAL_COMPONENT_DESIGN.md)

**特徴**:
- EventAction から独立した新規コンポーネント型
- 複数条件を配列で順に評価（if-elseif-else パターン）
- 最初のマッチした条件のアクションを実行（short-circuit）
- 5 種類の条件タイプ：compare / equals / truthy / has / exists

**JSON フォーマット**:
```javascript
{
  "type": "Conditional",
  "enabled": true,
  "data": {
    "listenTo": "ev_check_state",
    "branches": [
      {
        "condition": {
          "type": "compare",
          "left": "${user.persistent.hp}",
          "operator": ">=",
          "right": 100
        },
        "action": { ... }
      },
      {
        "condition": {
          "type": "equals",
          "left": "${user.persistent.state}",
          "right": "defending"
        },
        "action": { ... }
      }
    ],
    "defaultAction": { ... }  // 全条件 false の場合（optional）
  }
}
```

**実装例**: 敵 AI
```javascript
{
  "type": "Conditional",
  "data": {
    "listenTo": "ev_enemy_turn",
    "branches": [
      {
        "condition": { "type": "compare", "left": "${user.persistent.enemyHp}", "operator": "<=", "right": 30 },
        "action": { "action": "fireEvent", "eventId": "ev_enemy_heal" }
      },
      {
        "condition": { "type": "compare", "left": "${user.persistent.playerHp}", "operator": "<", "right": 40 },
        "action": { "action": "fireEvent", "eventId": "ev_enemy_critical_attack" }
      }
    ],
    "defaultAction": {
      "action": "fireEvent",
      "eventId": "ev_enemy_normal_attack"
    }
  }
}
```

**実装ロケーション**: 
- `www/js/component/conditional_action.js`（新規）
- `www/js/scene/play_test_scene.js`（修正）

### 8.2 Phase B: delay + ローカル変数

**目標**: シーンフローとアニメーション遅延の実装

```javascript
{
  "action": "sequence",
  "actions": [
    {
      "action": "playTween",
      "targetObjectId": "obj_enemy",
      "componentType": "Transform",
      "property": "x",
      "tweenFrom": 100,
      "tweenTo": 200,
      "tweenDuration": 0.5
    },
    {
      "action": "delay",
      "duration": 0.5
    },
    {
      "action": "setGlobalVariable",
      "variablePath": "user.persistent.playerHp",
      "valueSource": "variable",
      "valueVariablePath": "user.persistent.damage",
      "op": "subtract"
    }
  ]
}
```

**実装ロケーション**: `play_test_scene.js` に sequence action 追加

### 8.3 Phase C: 式評価エンジン

**目標**: 簡単な計算式の評価（スコア計算、ダメージ計算など）

```javascript
{
  "action": "setGlobalVariable",
  "variablePath": "user.persistent.finalDamage",
  "valueSource": "expression",
  "expression": "${user.persistent.baseDamage} * (1 + ${user.persistent.level} * 0.1)"
}
```

**実装ロケーション**: 新規ファイル `expression_evaluator.js` を作成

---

## 9. 各フェーズの完了条件

### Phase A 完了条件
- [ ] `if` アクション型の実装と検証
- [ ] 比較演算子（>=, <=, ===, !==, >, <）のサポート
- [ ] テンプレート変数が condition でも機能することを確認
- [ ] テストケース（算術演算デモなど）の動作確認
- [ ] ドキュメント更新

### Phase B 完了条件
- [ ] `sequence` アクション型の実装
- [ ] `delay` アクション型の実装
- [ ] アクション実行順序の保証確認
- [ ] tweenAnimation との組み合わせテスト
- [ ] ドキュメント更新

### Phase C 完了条件
- [ ] 式評価エンジン（calculator）の実装
- [ ] 算術演算、関数呼び出しのサポート
- [ ] テンプレート変数の展開確認
- [ ] パフォーマンステスト（毎フレーム評価の負荷確認）
- [ ] ドキュメント更新

---

## 10. デバッグ・パフォーマンス上の考慮点

### 10.1 条件分岐のデバッグ

- [ ] EventAction 実行ログに条件評価結果を記録
- [ ] Visual Studio Code debugger との統合
- [ ] PlayTest UI でアクティブな eventId の表示

### 10.2 パフォーマンス

- [ ] 複雑な条件評価は毎フレーム重くなる可能性
  → 初期段階は「簡潔で読みやすい条件」に制限することを推奨
- [ ] 式評価は AST キャッシュやメモ化を検討
- [ ] トリガー検出のO(n)コストは無視できるレベル（数十〜数百個のオブジェクト想定）

### 10.3 後方互換性

- [ ] 既存の単純な EventAction（action が "setProperty" など）は変わらずに動作すること
- [ ] 既存 `.qsproj` ファイルが変更なく動作確認

---

## 11. 結論と次ステップ

### 現状評価

- **実装済みの基盤は堅牢**: グローバル変数 API、基本的な EventAction フレームワークが揃っている
- **テンプレート変数で動的テキスト展開も可能**: UI の自動更新が実現している
- **不足点は明確**: 条件分岐とシーケンシング

### 推奨される次ステップ

1. **Phase A（条件分岐）を優先実装** → RPG の基本的な分岐ロジックが可能に
2. **テストプロジェクト作成** （例: 簡単な RPG バトルシステム）
3. **Phase B, C の段階的実装** に基づき、ゲーム規模に応じて追加

### 目標達成時期の目安

| フェーズ | 難度 | 見積期間 | 到達地点 |
|---------|------|--------|---------|
| A | 中 | 2-3 days | 簡単な RPG バトル（turn-based） |
| B | 中 | 3-4 days | アニメーション付き戦闘演出 |
| C | 高 | 5-7 days | スコア計算、敵AI の簡単な判定 |

これらを通じて、**実用的なゲーム開発環境**へと段階的に進化させることが可能です。

---

## 付録: リファレンス

### ファイル一覧
- [play_test_scene.js](../../www/js/scene/play_test_scene.js) - EventAction 実行ロジック
- [app_data.js](../../www/js/app_data.js) - グローバル変数 API
- [play_unit_runtime.js](../../www/js/play_unit/play_unit_runtime.js) - テンプレート変数展開
- [project_data.js](../../www/js/project/project_data.js) - グローバル変数定義・バリデーション

### 関連ドキュメント
- [GLOBAL_VARIABLES_USER_API_EVENTACTION_PLAN.md](GLOBAL_VARIABLES_USER_API_EVENTACTION_PLAN.md) - 実装計画
- [EVENT_ACTION_JSON_EXAMPLES.md](EVENT_ACTION_JSON_EXAMPLES.md) - JSON 例集
- [QSPROJ_LLM_EXAMPLES_COMPACT.md](QSPROJ_LLM_EXAMPLES_COMPACT.md) - qsproj フォーマット例

