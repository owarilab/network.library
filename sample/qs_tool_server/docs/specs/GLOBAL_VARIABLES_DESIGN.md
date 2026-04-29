# Global Variables 設計書

最終更新: 2026-04-29

---

## 目的

`PlayUnit` 単位ではなく、Runtime 全体で共有される状態を扱うため、
プロジェクトに `Global Variables` という概念を追加する。

この仕組みは次の2つを同時に満たすことを目的とする。

- Runtime システム固有の状態や設定を、明示的なデータ領域として定義できること
- ゲーム固有の進行状態やフラグを、将来的なセーブデータ機能へ接続できること

今回の文書は設計整理のみを目的とし、実装方法や UI 詳細までは確定しない。

---

## 背景

現在の `.qsproj` は `project.assets.playUnits[]` を中心に構成されており、
PlayUnit 内の object / component は定義できるが、複数 PlayUnit をまたいで共有される状態の正規の置き場が存在しない。

このため、今後以下のような機能を追加したくなった際に、状態の所属先が曖昧になる。

- ゲーム全体の進行フラグ
- 所持金、スコア、クエスト状態
- Runtime の現在 PlayUnit ID
- 一度開放した UI 状態や永続アンロック情報
- 将来のセーブ / ロード対象データ

この問題を避けるため、Project レベルにグローバル変数領域を明示的に導入する。

---

## 今回決めること

1. Global Variables の責務を定義する
2. `system` / `user` の2分類を定義する
3. `fixed` / `persistent` の2階層を定義する
4. `.qsproj` に保存するのは「定義」と「初期値」であることを明確化する
5. 将来の Runtime 実体と SaveData 実体の責務分離を定義する

---

## 今回は対象外

- 実行時 API の実装
- エディタ UI の実装
- セーブデータ形式の最終確定
- EventAction や条件分岐からの参照 / 書き換え実装
- 型検証や式評価器の実装

---

## 用語定義

| 用語 | 説明 |
|---|---|
| `Global Variables` | Project レベルで管理される Runtime 全体共有の変数群 |
| `system` | Runtime システム側が意味を予約している変数群 |
| `user` | 制作者が自由に定義して使う変数群 |
| `fixed` | 起動のたびにプロジェクト定義の初期値へ初期化される領域 |
| `persistent` | 将来のセーブデータ対象となる永続状態の初期値を定義する領域 |
| `definition` | `.qsproj` に保存される変数の宣言情報と初期値 |
| `runtime state` | 起動後の実行中メモリ上で保持される現在値 |
| `save data` | 将来、`persistent` 現在値を保存 / 復元するための外部データ |

---

## 基本方針

## 1. グローバル変数は Project 配下に置く

`PlayUnit` 配下ではなく `project` 配下に置く。

理由:

- 複数 PlayUnit をまたいで共有したい
- 現在アクティブな PlayUnit と独立して保持したい
- 将来のセーブデータ対象を Project 全体の責務として扱いたい

## 2. 4区画で整理する

グローバル変数は次の4区画で管理する。

| 区画 | 用途 |
|---|---|
| `system.fixed` | Runtime 内部の一時的・起動時初期化前提のシステム変数 |
| `system.persistent` | Runtime システムが継続保持したい永続系システム変数 |
| `user.fixed` | 制作者が自由に使う、起動ごと初期化される変数 |
| `user.persistent` | 制作者が自由に使う、将来のセーブ対象となる変数 |

## 3. `.qsproj` は persistent の「現在値」を持たない

`persistent` 領域であっても、`.qsproj` に保存されるのは初期値である。

つまり:

- `.qsproj` はプロジェクト定義ファイル
- 実際の進行状況は save data 側で保持する
- save data が無い起動では `.qsproj` の初期値から runtime state を生成する

この分離により、プロジェクトファイルを編集してもプレイヤーの進行データと混ざらない。

## 4. system は予約領域、user は自由領域

`system` は Runtime が意味を理解するための予約済み名前空間とする。
制作者が値を初期設定することはありえるが、項目追加の自由度は `user` より低い。

`user` はゲームロジック用の自由変数領域とする。

初期導入では、`system` 領域を次の最小構成から始める。

- `system.fixed.startupPlayUnitId`
- `system.fixed.requestedPlayUnitId`
- `system.fixed.currentPlayUnitId`
- `system.fixed.returnPlayUnitId`
- `system.fixed.isPaused`
- `system.persistent.masterVolume`

このうち、PlayUnit 遷移や一時停止のような実行中状態は `fixed`、
設定として次回起動へ持ち越したい音量は `persistent` に置く。

---

## データ配置案

`.qsproj` の `project` 配下に `globalVariables` を追加する。

```json
{
  "format": "qsproj",
  "version": 1,
  "project": {
    "id": "proj_xxxxx",
    "version": 1,
    "name": "Project",
    "settings": {},
    "globalVariables": {
      "version": 1,
      "system": {
        "fixed": {},
        "persistent": {}
      },
      "user": {
        "fixed": {},
        "persistent": {}
      }
    },
    "assets": {
      "playUnits": []
    }
  }
}
```

`globalVariables` は project の top-level 構造として扱い、`assets` の一部にはしない。

理由:

- asset ではなく、Project の実行設定 / 状態定義に近い
- `playUnits` や `pixelDocuments` と性質が異なる
- Runtime 起動時に常に読み込むべき情報である

---

## 変数定義モデル

各変数は「ただの値」ではなく、最低限のメタ情報を持つ定義オブジェクトとして保持する。

```json
{
  "hp": {
    "type": "number",
    "initialValue": 100,
    "description": "プレイヤーの初期HP"
  }
}
```

### 最小フィールド

| フィールド | 型 | 必須 | 説明 |
|---|---|---|---|
| `type` | string | ○ | 値型。`string` / `number` / `boolean` / `json` を想定 |
| `initialValue` | any | ○ | 起動時の初期値。`persistent` でも `.qsproj` 上では初期値として扱う |
| `description` | string | 任意 | エディタ表示用の説明 |

### 将来拡張候補

| フィールド | 用途 |
|---|---|
| `readOnly` | 書き換え禁止の明示 |
| `min` / `max` | number の入力制約 |
| `enum` | 選択式文字列 |
| `tags` | 検索・分類 |
| `editor` | UI 表示ヒント |

初期段階では `type`, `initialValue`, `description` までに絞る。

---

## 型方針

## 1. 初期対応型

| 型 | 用途 |
|---|---|
| `string` | 名前、ID、状態名など |
| `number` | スコア、所持金、段階値など |
| `boolean` | 開放済みフラグ、ON/OFF 状態 |
| `json` | 配列やオブジェクトなどの複合値 |

## 2. `json` 型は escape hatch として扱う

複合値が必要なケースに備えて `json` 型は許可するが、
条件分岐や EventAction などと接続する際の扱いやすさを考えると、
基本は `string` / `number` / `boolean` を推奨する。

## 3. null の扱い

初期設計では専用型 `null` は作らず、必要なら `json` 型で `null` を持てるものとする。

---

## 4区画の責務

## 1. system.fixed

Runtime が内部的に使う一時状態や、起動ごとに初期化すべきシステム変数を置く。

初期導入時の項目:

- `startupPlayUnitId`
- `requestedPlayUnitId`
- `currentPlayUnitId`
- `returnPlayUnitId`
- `isPaused`

`startupPlayUnitId` は、Runtime 起動直後に最初にロードする PlayUnit を決めるための system 変数とする。
この値はプロジェクト制作者が `.qsproj` 側で設定する。

`requestedPlayUnitId` は、EventAction などが「次にこの PlayUnit へ切り替えたい」という要求を Runtime へ渡すための system 変数とする。
この値の更新だけでは切り替え完了を意味せず、Runtime が要求を受けて実際の遷移処理を行う。
要求を処理したあとは、自動的に空文字へ戻すワンショット要求値として扱う。
存在しない PlayUnit ID が入っていた場合は、その要求は無視し、同様に空文字へ戻す。

`currentPlayUnitId` は、実行中に現在アクティブな PlayUnit を保持するための system 変数とする。
これは要求値ではなく結果値であり、実際の切り替えが成功した時点で Runtime が更新する。

`returnPlayUnitId` は、一時的に別 PlayUnit を開いたあと戻る先を保持するための system 変数とする。

`isPaused` は、Runtime 全体が一時停止中かどうかを表す system 変数とする。

例:

- `startupPlayUnitId`
- `requestedPlayUnitId`
- `currentPlayUnitId`
- `returnPlayUnitId`
- `isPaused`

特徴:

- 起動ごとに `.qsproj` 初期値から作り直す
- save data 対象にしない
- 起動時の初期遷移先決定や、実行中の遷移状態保持に使う

補足:

- `startupPlayUnitId` は「最初に開く PlayUnit」の設定値
- `requestedPlayUnitId` は「次に開きたい PlayUnit」の要求値
- `currentPlayUnitId` は「今開いている PlayUnit」の結果値
- `returnPlayUnitId` は「一時遷移から戻る先」の実行時状態
- `isPaused` は Runtime 全体の更新停止状態
- `requestedPlayUnitId` を更新しても、それだけで切り替え完了とはみなさない
- `requestedPlayUnitId` は要求処理後に自動で空文字へ戻す
- 無効な PlayUnit ID が入っていた場合も、要求は無視して空文字へ戻す
- 実際の遷移成功後に `currentPlayUnitId` を Runtime が更新する
- `returnPlayUnitId` は保持用、`requestedPlayUnitId` はワンショット要求用として役割を分ける
- これらは用途が異なるため、近い概念でも分離して持つ

## 2. system.persistent

Runtime システム自身が継続保持したい情報を置く。

例:

- `masterVolume`
- `lastSelectedLanguage`
- `unlockedDebugFlags`

初期導入時に優先して持つ項目:

- `masterVolume`

特徴:

- `.qsproj` には初期値だけを定義する
- 実プレイ中の変更結果は save data 側へ保存される
- user ロジックでも参照はできるが、意味は Runtime 側が予約する

補足:

- `masterVolume` はユーザー設定として次回起動にも持ち越したい可能性が高いため、`fixed` ではなく `persistent` に置く

## 3. user.fixed

制作者が自由に使う、一時状態用の変数を置く。

例:

- `battleTemporaryScore`
- `openedDialogId`
- `currentWave`

特徴:

- 起動ごとに初期値へ戻る
- セーブ対象にしない
- セッション中だけ有効な状態の置き場として使う

## 4. user.persistent

制作者が自由に使う、将来のセーブデータ対象となる変数を置く。

例:

- `playerLevel`
- `gold`
- `quest_main_001_cleared`
- `openedTreasureCount`

特徴:

- `.qsproj` には初期値を定義する
- 実行中の変更値は save data へ書き出す対象となる
- 新規ゲーム開始時は `.qsproj` 初期値で始まる

---

## 初期化とライフサイクル

Runtime 起動時は次の順序で状態を構築する。

### save data が無い場合

1. `.qsproj` の `globalVariables` 定義を読む
2. `system.fixed` を `initialValue` で runtime state 化する
3. `system.persistent` を `initialValue` で runtime state 化する
4. `user.fixed` を `initialValue` で runtime state 化する
5. `user.persistent` を `initialValue` で runtime state 化する

### save data がある場合

1. `.qsproj` の `globalVariables` 定義を読む
2. `fixed` 系は常に `.qsproj` 初期値から runtime state 化する
3. `persistent` 系は save data の値で上書きする
4. save data に存在しない persistent 変数は `.qsproj` の初期値を使う

このルールにより、新しい persistent 変数をプロジェクトに追加した場合でも、既存セーブデータへ自然に後方互換で混在させやすい。

---

## `.qsproj` と save data の責務分離

## 1. `.qsproj`

持つもの:

- 変数の存在定義
- 型定義
- 説明
- 初期値

持たないもの:

- プレイヤーが遊んだ結果の現在値
- 実行中の一時 dirty state

## 2. runtime state

持つもの:

- 今この瞬間の現在値
- fixed / persistent の両方の現在値

## 3. save data

持つもの:

- `persistent` 区画の現在値
- 将来必要ならメタ情報（save slot, timestamp, play time など）

持たないもの:

- `fixed` 区画の現在値
- 変数定義そのもの

---

## 命名方針

## 1. system 変数

Runtime 予約領域であることを明確化するため、UI 上では system と user を分離表示する。

命名例:

- `requestedPlayUnitId`
- `currentPlayUnitId`
- `runtimeMode`
- `masterVolume`

## 2. user 変数

制作者の自由度を高く保つ。

推奨:

- 意味が明確な lowerCamelCase
- 真偽値は `is` / `has` / `can` で始める
- 進行フラグは `questMain001Cleared` のように用途を明示する

非推奨:

- `flag1`
- `tmp2`
- 意味の曖昧な短縮名

---

## JSON 例

```json
{
  "globalVariables": {
    "version": 1,
    "system": {
      "fixed": {
        "startupPlayUnitId": {
          "type": "string",
          "initialValue": "pu_title_main_ab12cd34",
          "description": "Runtime 起動時に最初に開く PlayUnit ID"
        },
        "requestedPlayUnitId": {
          "type": "string",
          "initialValue": "",
          "description": "Runtime に対する PlayUnit 切り替え要求先 ID。処理後は自動で空文字に戻る"
        },
        "currentPlayUnitId": {
          "type": "string",
          "initialValue": "pu_title_main_ab12cd34",
          "description": "Runtime 実行中に現在アクティブな PlayUnit ID。切り替え成功後に Runtime が更新する"
        },
        "returnPlayUnitId": {
          "type": "string",
          "initialValue": "",
          "description": "一時遷移後に戻る先の PlayUnit ID"
        },
        "isPaused": {
          "type": "boolean",
          "initialValue": false,
          "description": "Runtime 全体の一時停止状態"
        }
      },
      "persistent": {
        "masterVolume": {
          "type": "number",
          "initialValue": 0.8,
          "description": "全体音量"
        },
        "lastSelectedLanguage": {
          "type": "string",
          "initialValue": "ja",
          "description": "前回選択言語"
        }
      }
    },
    "user": {
      "fixed": {
        "currentWave": {
          "type": "number",
          "initialValue": 1,
          "description": "現在ウェーブ"
        },
        "battleTemporaryScore": {
          "type": "number",
          "initialValue": 0,
          "description": "戦闘中のみ使う一時スコア"
        }
      },
      "persistent": {
        "playerLevel": {
          "type": "number",
          "initialValue": 1,
          "description": "プレイヤーレベル"
        },
        "gold": {
          "type": "number",
          "initialValue": 0,
          "description": "所持金"
        },
        "questMain001Cleared": {
          "type": "boolean",
          "initialValue": false,
          "description": "メインクエスト001クリア済み"
        }
      }
    }
  }
}
```

---

## Runtime API の責務イメージ

実装は今回行わないが、Runtime 側では概念的に次のような責務が必要になる。

```js
globals.get('user.persistent.gold')
globals.set('user.persistent.gold', 120)
globals.set('system.fixed.requestedPlayUnitId', 'pu_town_01')
globals.get('system.fixed.currentPlayUnitId')
globals.get('system.fixed.returnPlayUnitId')
globals.resetFixed()
globals.exportPersistentState()
globals.importPersistentState(saveData)
```

重要なのは、Runtime が `.qsproj` 定義と save data を合成し、
最終的な現在値ストアを1か所で管理すること。

---

## EventAction / 条件分岐との接続方針

今後この設計を使う場合、少なくとも次の拡張と相性が良い。

- EventAction からグローバル変数を読む
- EventAction からグローバル変数へ代入する
- 条件分岐で `user.persistent.questMain001Cleared === true` のように判定する
- Runtime 起動時に `system.fixed.startupPlayUnitId` を参照して最初の PlayUnit を決定する
- EventAction などが `system.fixed.requestedPlayUnitId` に切り替え要求を書き込む
- Runtime が要求を受けて実際に PlayUnit を切り替える
- PlayUnit 切り替え成功時に `system.fixed.currentPlayUnitId` を更新する
- `system.fixed.requestedPlayUnitId` は処理後に空文字へ戻す
- 無効な PlayUnit ID の要求は無視し、`system.fixed.requestedPlayUnitId` は空文字へ戻す
- 一時メニューや会話用 PlayUnit へ遷移する際に `system.fixed.returnPlayUnitId` を退避先として使う
- Pause 制御で `system.fixed.isPaused` を参照する
- 音量設定 UI で `system.persistent.masterVolume` を更新する

この文書の時点では、あくまで「参照先 / 書き込み先となる正式な状態領域を定義する」ことが目的であり、
具体的なアクション種別や式仕様は別文書で設計する。

---

## バリデーション方針

将来の実装では、少なくとも以下を検証対象にする。

- `globalVariables.version` が対応バージョンであること
- 各区画が object であること
- 変数名が重複していないこと
- 各変数定義が `type` と `initialValue` を持つこと
- `type` と `initialValue` の整合が取れていること

追加で、system 予約名の衝突チェックも必要になる。

---

## 移行方針

既存プロジェクトとの互換性のため、`globalVariables` が存在しない場合は次のように扱う。

- `globalVariables` 未定義 = 4区画すべて空オブジェクト
- Runtime はエラーではなく空定義として扱う
- エディタ保存時に必要なら `globalVariables` を補完生成する

これにより、既存の `.qsproj` を壊さず段階導入できる。

---

## この設計の利点

- Project 全体状態の正規の置き場ができる
- `fixed` と `persistent` を明確に分離できる
- save data と project 定義の責務が混ざらない
- system と user の責務を分けられる
- PlayUnit 遷移状態とシステム設定の最小セットをデータで持てる
- 将来の EventAction、条件分岐、PlayUnit 遷移、セーブ / ロードの基盤になる

---

## VariablesEditorScene 方針

変数エディタは `ProjectTopScene` に埋め込まず、専用 `VariablesEditorScene` として実装する方針とする。

この命名にする理由は、今後 `project.globalVariables` だけでなく、
`PlayUnit` 単位のローカル変数定義を扱う可能性が高いためである。

初期実装では `VariablesEditorScene` の対象をグローバル変数に限定するが、
設計上は将来 `Project Variables` と `PlayUnit Local Variables` の両方を扱える拡張余地を持たせる。

理由:

- `system` / `user` と `fixed` / `persistent` の4区画を扱うため、一覧と編集フォームの両方に一定の画面面積が必要
- 今後、検索、絞り込み、型別 UI、バリデーション表示、予約 system 変数の read-only 制御などを追加する余地がある
- 将来 `PlayUnit` ローカル変数まで扱う場合、ProjectTop に機能を詰め込むより独立画面の方が構造を保ちやすい

### 画面遷移方針

- 遷移元は `ProjectTopScene` とする
- `ProjectTopScene` から `VariablesEditorScene` へ遷移できる導線を持つ
- プロトタイプ段階では `VariablesEditorScene` から `ProjectTopScene` へ戻る導線を備える

### プロトタイプ版の到達目標

初期実装では高機能化を狙わず、最低限の編集ができる UI までを到達目標とする。

ただしこの段階でも、Scene 名と画面構成は将来のローカル変数対応を見越して一般化しておく。
画面レイアウトは左ナビ構成を採用する。

プロトタイプ版で備える機能:

- 変数スコープとして少なくとも `Project Variables` を選択できる
- `Project Variables` 配下で `system.fixed` / `system.persistent` / `user.fixed` / `user.persistent` を区画ごとに一覧表示する
- 各変数について `name`, `type`, `initialValue`, `description` を確認できる
- 変数の追加ができる
- 変数の削除ができる
- `type`, `initialValue`, `description` の編集ができる
- 編集結果を `.qsproj` の `project.globalVariables` に保存できる

プロトタイプ版では後回しにするもの:

- `PlayUnit Local Variables` の実編集機能
- 高度な検索・フィルタ
- ドラッグによる並び替え
- `json` 型専用エディタ
- 変更履歴や差分プレビュー
- 実行中 Runtime state の監視 UI

### プロトタイプ版 UI の最小構成

専用 Scene の最小構成は、左ナビを持つ3ペイン寄りの構成を想定する。

- 上部: タイトル、戻るボタン、保存ボタン
- 左側: スコープ選択と区画切り替えを兼ねる左ナビ
- 中央: 選択中区画の変数一覧
- 右側または下部: 選択中変数の編集フォーム

左ナビの初期構成イメージ:

- `Project Variables`
- `Project Variables / system.fixed`
- `Project Variables / system.persistent`
- `Project Variables / user.fixed`
- `Project Variables / user.persistent`

将来の拡張では、同じ左ナビ配下に `PlayUnit Local Variables` と各 PlayUnit ノードを追加できる構成を想定する。

将来拡張時の想定スコープ例:

- `Project Variables`
- `PlayUnit Local Variables`

最低限必要な入力項目:

- 変数名
- 型選択 (`string` / `number` / `boolean` / `json`)
- 初期値
- 説明

### プロトタイプ段階の system 変数編集方針

プロトタイプ段階では `system` 変数も同じ専用 Scene 上で扱うが、
将来的に Runtime 管理項目は read-only または一部編集制限をかける前提で設計する。

初期段階では次のように扱うのが妥当とする。

- `startupPlayUnitId` と `masterVolume` は編集可能
- `requestedPlayUnitId`, `currentPlayUnitId`, `returnPlayUnitId`, `isPaused` は定義表示はしても、実行時状態そのものを編集する UI とは分離して考える

ここでの目的は、まず `.qsproj` 上の定義を安全に編集できる導線を作ることにある。

---

## 今後の課題

以下は現時点では確定せず、今後の拡張課題として後回しにする。

1. `system` 変数のうち、どこまでをユーザー編集可能にするか
2. `persistent` save data を「全量保存」にするか「差分保存」にするか
3. `json` 型を初期段階から許可するか、後で追加するか
4. プロトタイプ版で `system` 変数のうちどこまで編集可能にするか
5. EventAction で扱う参照パス記法を `user.persistent.gold` のような文字列パスに統一するか

本設計の初期実装は、これら5項目を未確定のままでも進められる前提とする。
必要になった時点で個別に設計を追加し、この文書または関連仕様へ反映する。