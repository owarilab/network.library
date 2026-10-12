# メッセージウィンドウ・タイプライター設計

- 状態: 実装済み（差分は末尾の「実装時の差分」を参照）
- 関連: [NINE_SLICE_DESIGN.md](NINE_SLICE_DESIGN.md)（ウィンドウの枠）、[EVENT_ACTION_DESIGN.md](EVENT_ACTION_DESIGN.md)、[JRPG_FEATURE_GAP_ANALYSIS.md](JRPG_FEATURE_GAP_ANALYSIS.md) の P0-2 / P0-3

## 1. 目的

メッセージ表示（タイプライター、全文表示、クリックで次へ）を、組込みの専用ウィンドウではなく PlayUnit の部品とイベントの組み合わせで作る。ウィンドウ、話者名、選択肢は同じ部品で作れるようにする。

タイプライターは「表示」の部品ではなく「イベントの実行」として扱う。イベントで指定されたテキストを一定速度で表示し、全文が出たら完了イベントを発火する。次へ進むのは次のトリガー（クリック）とする。

## 2. 流れ

```
(他のイベント) msg.show
  → EventAction: typewriter
       本文 Text に文字列をセットし、revealChars を 0 から増やす
       status = 'typing'
  → 全文表示で status = 'done'、completeEventId（例: msg.typed）を発火
  → プレイヤーがウィンドウをクリック（Trigger click、eventId: msg.click）
  → Conditional（listenTo: msg.click）
       status が 'typing' なら: typewriter（スキップ。全文表示して msg.typed を発火）
       それ以外（default）  : fireEvent msg.next
  → 次の処理（次の文の表示、または終了）
```

次の文を出す処理（`msg.next` を受けて文を選び、`msg.show` を発火する）は、シーケンス（P0-3 の wait と順次実行）の仕事とする。この設計では窓の動作だけを作る。

## 3. 決定事項（推奨）

### D1. 本文は Text の `text` を空のままにし、typewriter アクションが文字列を渡す

- PlayUnit に書く本文 Text の `text` は空（または必要に応じて `${...}` のテンプレート）。
- `typewriter` アクションが `valueSource` で文字列を受け取る。`literal`（`value`）と `variable`（`valueVariablePath`）を選べる。`setGlobalVariable` と同じ書き方に揃える。
- 文字列は `setProperty` を通さない。`setProperty` は値を JSON として解釈するため、`"123"` が数値になり Text が消える（既存の注意点）。

### D2. 表示文字数は Text の `revealChars` で持つ

- 既定値は `-1`（全文表示）。0 以上なら先頭から数えた文字数だけを表示する。
- `typewriter` が実行用コピーの Text に書き込み、`PlayUnitRuntime` が読んで描画に使う。アセットは変更しない。

### D3. 文字数の数え方

- 改行（`\n`）は 0 文字として数える。
- 折り返しで行末に残らなかった空白は数えない。
- 折り返し後の行を先頭から数える。折り返しの位置は表示中に変わらないので、文字が途中で行をまたいでも崩れない。

### D4. 完了後はクリックを待つ

- 全文表示後は次のトリガーを待つ（クリック）。自動送りは P0-3 の `waitEvent` の `timeout` で作れる（section 11）。
- クリックは UI の Trigger（`triggerOn: click`）で受ける。UI の当たり判定は、Trigger と Collider（`width` / `height` を 0 にすると UITransform の矩形を使う既存の挙動）で成立する。

### D5. 状態は変数で外に出す

- `typewriter` は `statusVariablePath` に `'typing'` / `'done'` を書く（省略可）。
- Conditional は既存の `equals` で読む。内部状態を読む専用の API は作らない。
- 変数は `user.fixed.msgStatus`（string）と `user.fixed.msgBody`（string）のように定義しておく。

### D6. 再実行の扱い

| 状態 | `typewriter` を実行したとき |
|---|---|
| `typing`（表示中） | 再開始しない。全文表示して `done` にし、`completeEventId` を発火する（スキップ） |
| `done` / 未開始 | 新しい文字列で開始する |

### D7. 速度

- `charsPerSecond`（既定 30）。文字ごとの間は、句読点の間を含めて MVP では持たない。

### D8. 対象の管理

- 同じ Text への typewriter は 1 件だけ持つ（キーは `${unitId}/${objectId}`）。
- PlayUnit を切り替えたら破棄する（PlayTest の状態として保持し、アセットには書かない）。

### D9. 空文字の Text を描画対象にする

現状は `text` が空だと描画エントリを作らない（[play_unit_runtime.js](../../www/js/play_unit/play_unit_runtime.js) L120-121、UI は L354-355 の `if (textValue)`）。空の Text も描画エントリを作るように変える。空文字の `backgroundColor` は描画しない（文字が無いときに 0 サイズの箱を出さない）。

### D10. 話者名と選択肢

- 話者名は別の Text にする。固定文字は `text` に書き、変数で変わるものは `${user.fixed.speaker}` のテンプレートで書く（既存の仕組み）。
- 選択肢は範囲外（同じ部品と Conditional で後日作る）。

## 4. 変更箇所

### 4.1 ランタイム

| 対象 | 変更 |
|---|---|
| [play_unit_runtime.js](../../www/js/play_unit/play_unit_runtime.js) | Text の `revealChars` を読む（ワールド L118 付近、UI L352 付近）。空文字でもエントリを作る |
| [play_test_scene.js](../../www/js/scene/play_test_scene.js) `_drawTextEntry` | 折り返し後の行に `revealChars` を適用する（`_buildTextLines` の後に、文字数で行を切る関数を入れる） |
| 同 `_executeAction` | `typewriter` を追加。targetObjectId の解決は既存の規則に従う |
| 同 `update` | `_updateTypewriters(dt, playUnit)` を追加。`_processEventActions` の後、`_updateTweens` と同じ位置に置く |
| 同 PlayUnit 切替 | `_typewriters` を破棄する |

### 4.2 `typewriter` アクションのデータ

```json
{
  "action": "typewriter",
  "targetObjectId": "obj_msg_body",
  "componentType": "Text",
  "valueSource": "variable",
  "valueVariablePath": "user.fixed.msgBody",
  "charsPerSecond": 30,
  "statusVariablePath": "user.fixed.msgStatus",
  "completeEventId": "msg.typed"
}
```

- `valueSource: "literal"` のときは `value` に文字列を書く。
- `componentType` は `Text` 固定とする（他のコンポーネントには使わない）。
- EventAction の `action` 一覧（EVENT_ACTION_DESIGN.md）に `typewriter` を追加する。

### 4.3 クリックで進む窓（Conditional）

```json
{
  "type": "Conditional",
  "enabled": true,
  "data": {
    "listenTo": "msg.click",
    "branches": [
      {
        "condition": { "type": "equals", "left": "${user.fixed.msgStatus}", "right": "typing" },
        "action": { "action": "typewriter", "targetObjectId": "obj_msg_body", "componentType": "Text",
                    "valueSource": "variable", "valueVariablePath": "user.fixed.msgBody",
                    "statusVariablePath": "user.fixed.msgStatus", "completeEventId": "msg.typed" }
      }
    ],
    "defaultAction": { "action": "fireEvent", "eventId": "msg.next" }
  }
}
```

- 条件は `ConditionalAction.evaluateCondition`（[conditional_action.js](../../www/js/component/conditional_action.js)）の `equals` を使う。`left` の `${...}` は変数として解決される。
- スキップ時は引数（文字列）を使わないが、同じ書式で書いておく。
- Conditional は分岐ごとに 1 アクションのみなので、この形で足りる。

### 4.4 窓の PlayUnit（構成例）

```
Root (UICanvas)
 ├─ MsgBox      UITransform（下端中央、幅 600 高さ 120）
 │              NineSlice（pixelDocumentId: px_frame、scale 2）
 │              Collider（width 0、height 0 → UITransform 矩形）
 │              Trigger（click、eventId: msg.click）
 │              Conditional（上の例）
 │   └─ MsgBody UITransform（内側 16px）
 │              Text（text: ""、wrap: true、revealChars: -1）
 └─ Speaker     UITransform（MsgBox の左上の外側）
                Text（text: "${user.fixed.speaker}"）
```

- 実行開始は他のイベントが `msg.show` を発火し、EventAction（listenTo: `msg.show`、action: `typewriter`）が受ける。サンプル PlayUnit では、`msg.show` の発火を開始時の `fireEvent` で代用する。

## 5. 範囲外（今回は作らない）

- wait / delay、順次実行（P0-3）。
- 自動送り、句読点の間、文字ごとの効果音。
- 選択肢 UI。
- キーボードでの送り（Trigger はクリックのみ）。
- ルビ、文字ごとの色や装飾。
- トゥイーン完了イベント。typewriter は独自に完了を持つので、この機能には不要。tween の完了イベントは P0-3 で別に必要になる。

## 6. なぜ playTween を使わないか

- `tweenDuration` は JSON に固定で書く。文字数に応じた長さを実行時に決められない。
- `tweenTo` も固定値のため、メッセージごとに文字数を変えられない。
- 「全文表示」「スキップ」の状態を tween では表せない。

専用の `typewriter` にすると、長さは実行時の文字数から決まり、スキップと状態変数を自然に扱える。

## 7. 実装フェーズ

1. Text の空文字と `revealChars`（描画のみ。固定値で確認する）。
2. `typewriter` アクション、実行時の進行、完了イベント、状態変数、PlayUnit 切替時の破棄。
3. 9 スライス（[NINE_SLICE_DESIGN.md](NINE_SLICE_DESIGN.md)）。窓の枠に使う。
4. サンプル: メッセージウィンドウ PlayUnit を追加する（`tools/` のスクリプトで .qsproj に追加し、9 スライスの枠も同じスクリプトで作る）。
5. ドキュメント（EVENT_ACTION_DESIGN.md、JSON 例、WORK_STATUS、QSPROJ の LLM 向け 2 文書）。

## 8. 検証（完了条件）

- `typewriter` 実行後、時間に応じて `revealChars` が増え、全文で `done` になり `msg.typed` が発火する。
- 表示中にクリックすると全文表示になり、`msg.next` は発火しない。全文表示後のクリックで `msg.next` が発火する。
- 折り返しで行数が変わる文でも、表示文字数が崩れない（スクリーンショット）。
- 改行を含む文で、改行が文字数に入らない。
- 変数 `user.fixed.msgBody` の変更が次の `typewriter` 実行で反映される。
- PlayUnit 切替で進行中の typewriter が止まり、戻ると再開しない。
- アセットの Text の `text` と `revealChars` は変わらない（実行用コピーのみ変わる）。
- 空文字の Text が描画エントリになり、既存の Text（文字あり）の描画結果が変わらない。

## 9. 未決事項（推奨で進める）

- 折り返し後の文字数の数え方（D3）は、実装時にデモで確認する。難しければ「折り返し前の文字数で数える」に切り替える。
- 送りの入力はクリックのみ。キー入力を足すかは後で決める。
- 既存ドキュメントの不整合: [EVENT_SYSTEM_ANALYSIS_REPORT.md](EVENT_SYSTEM_ANALYSIS_REPORT.md) の `tweenDuration` の例は秒として書かれているが、実装はミリ秒で扱う。本件とは別に直す。

## 10. 実装時の差分

- **完了イベント**: `completeEventId` は全文表示のフレームで `_pendingEvents` に入り、次のフレームで処理される。
- **文字数**: 改行を除き、折り返し後の行を先頭から数える。行数は変えないので、背景の箱の高さは全文のときと同じ。
- **PlayUnit 切替**: 進行中の表示は止まり、切替先に戻っても再開しない。Text の `revealChars` は全文（-1）に戻す。
- **空の Text**: 描画エントリになる。空の文字列では背景も描かない。
- **総文字数**: 改行を除いた文字数（コードポイント単位）。折り返しで削られた空白の分だけ、画面上の全文表示より完了が数フレーム遅れる。
- **デモ**: `tools/add_message_window_to_demo.py`。起動 0.5 秒後に `msg.show` を発火し、`MessageCanvas` の `typewriter` が本文を表示する。クリックは `msg.click` を発火し、`Conditional` が「表示中ならスキップ、そうでなければ `msg.next`」に分ける。`msg.next` で `MessageBox` を無効にして閉じる。
- **検証**: Playwright で、空の Text の描画、24 文字/秒の進行（0.5 秒後に開始、約 1.5 秒で完了）、完了後の再開始、表示中の再実行で全文表示になり `done` になる、部分表示の描画、デモの窓の表示・スキップ・閉じる（実際のマウス入力経路で確認）。


## 11. P0-3 での変更（順次実行）

- **文の進行**: 次の文を出す処理は、EventAction の `sequence` が担当する。`typewriter`（`waitComplete`）で文を出し、`waitEvent`（`msg.next`）でクリックを待つ。`msg.show` で始まる 1 本の sequence に文を並べる。
- **入力の解釈**: クリックは引き続き `msg.click` の Conditional が受け持つ。表示中ならスキップ（typewriter の再実行）、表示後なら `msg.next` を発火する。進行と入力の解釈を分けたので、Conditional の「単一アクション」の制限に当たらない。
- **フェードイン**: 窓の枠（NineSlice の alpha）を `playTween` で 0 → 1 にし、`waitComplete` で終わってから本文を出す。
- **自動送り**: 最後の `waitEvent` に `timeout`（秒）を付けると、クリックが無くても時間で進む。デモはクリック送りのまま。
- **デモ**: `tools/add_message_sequence_to_demo.py` は、P0-2 で `msg.show` / `msg.next` に付けた EventAction を外し、sequence に置き換える。枠の初期 alpha は 0 にする。再実行しても結果は変わる。前提として `add_message_window_to_demo.py` の実行が必要。
- **切替**: PlayUnit を切り替えると sequence は止まり、戻っても再開しない。窓は残り、`typewriter` の状態変数は `done` になる。
- **検証**: 実行用コピーで `scene.update` を直接回し、sequence の各 step（`wait`、`waitEvent`、`waitComplete`、`onBusy`、完了イベント、失敗時の中断、Conditional からの実行）と PlayUnit 切替を確認した。デモは実際の入力経路（キャンバスへの mousedown / mouseup）で、フェード → 1 行目 → スキップ → 2 行目 → 待ち中のクリックは捨てられる → 3 行目 → 閉じる、の順に確認した。
