# PlayUnit 設計書

最終更新: 2026-04-26

## 目的

`qs_tool_server` を、単なるドット絵 / タイルセット / マップ編集ツールから、
より実用的なゲーム制作基盤へ発展させるため、
ゲーム内で扱う構成単位を `PlayUnit` として定義する。

既存の `Scene` はすでにアプリケーション側の画面遷移に使っているため、
「編集対象となるゲーム側の単位」には別名を与える。
この役割を担う名称として `PlayUnit` を採用する。

今回の文書は実装計画の前段として、
まず責務、データ構造、既存システムとの関係を固定することを目的とする。

---

## 背景

- 現在の `Scene` は `TitleScene`、`ProjectTopScene`、`EditorScene`、`MapEditorScene` のような画面遷移単位で使われている
- 一方、ゲームエンジン寄りの拡張を考えると、マップ、設定、配置物、プレイヤー、イベントなどをまとめて扱うゲーム側の単位が必要になる
- その単位は必ずしも「インゲーム空間」だけとは限らず、会話専用領域、イベント進行専用構成、UI 主体のプレイ領域も含みうる
- このため `Stage` や `Level` よりも中立的な `PlayUnit` を採用する

---

## このフェーズで決めること

1. `Scene` と `PlayUnit` の責務を分離する
2. `PlayUnitData` の最小データ構造を定義する
3. `PlayUnit` 配下は `objects` の1本に統一する
4. `tilemaps`、`settings` を専用配列で持たず、object + component で抽象化する
5. 将来の `PlayUnitRuntime` へ接続しやすい形にする

---

## 今回は対象外

- `PlayUnit` エディタ UI の実装
- Runtime の更新ループ、描画ループの実装
- 物理演算、アニメーション、AI の本格設計
- スクリプト実行基盤の詳細
- PlayUnit と既存 map asset の完全統合実装

---

## 用語定義

| 用語 | 説明 |
|------|------|
| `Scene` | アプリケーションの画面遷移単位。Title / ProjectTop / Editor など |
| `PlayUnit` | ゲーム側で扱う編集単位。マップ、配置物、設定などを束ねる |
| `PlayObject` | `PlayUnit` 内に置かれるオブジェクト |
| `Component` | `PlayObject` に機能や意味を付与する部品 |
| `PlayUnitRuntime` | `PlayUnitData` を実行可能状態へ変換した runtime 表現 |

---

## 責務分離

## 1. Scene

`Scene` は今までどおり、アプリケーションの画面遷移単位として使う。

例:

- `TitleScene`
- `ProjectTopScene`
- `EditorScene`
- `MapEditorScene`
- 将来の `PlayUnitEditorScene`
- 将来の `PlayTestScene`

`Scene` は「どの画面を表示し、どの UI フローを進めるか」を担当し、
ゲームデータ本体の正本は持たない。

## 2. PlayUnit

`PlayUnit` はゲーム側の編集対象単位とする。

1つの `PlayUnit` は、次のような構成要素をまとめて持てる。

- タイルマップ
- NPC や敵などの配置物
- カメラや当たり判定に関する設定
- ワープ、トリガー、メッセージ開始点
- UI を伴う特殊なプレイ用構成

重要なのは、これらを top-level の別配列に分けず、
すべて `objects` の配列に統一して保持すること。

---

## 設計方針

## 1. objects を正本にする

`PlayUnitData` は `objects` のみを持つ。

## 2. 機能は component で表現する

`tilemaps`、`settings` を top-level property にせず、
`PlayObject` に付与された component で表す。

例:

- タイルマップ: `TilemapComponent`
- プレイ設定: `PlaySettingsComponent`

---

## データモデル

## 1. PlayUnitData

`PlayUnitData` は保存対象の最小単位であり、
`ProjectData` 配下に複数保持できる前提とする。

```js
{
  id: 'pu_town_01',
  name: 'Town',
  objects: [
    /* PlayObjectData */
  ]
}
```

### 最小プロパティ

- `id`: PlayUnit 識別子
- `name`: 表示名
- `objects`: `PlayObjectData[]`

### 将来拡張候補

- `version`
- `createdAt`
- `updatedAt`
- `rootObjectId`
- `metadata`

今回の合意段階では `id`, `name`, `objects` に絞る。

## 2. PlayObjectData

```js
{
  id: 'obj_player',
  name: 'Player',
  enabled: true,
  parentId: null,
  children: [],
  components: [
    /* ComponentData */
  ]
}
```

### 最小プロパティ

- `id`: object 識別子
- `name`: 表示名
- `enabled`: 有効 / 無効
- `parentId`: 親 object ID
- `children`: 子 object ID 配列
- `components`: `ComponentData[]`

## 3. ComponentData

```js
{
  type: 'Transform',
  enabled: true,
  data: {
    x: 10,
    y: 8,
    z: 0
  }
}
```

### 最小プロパティ

- `type`: コンポーネント種別
- `enabled`: 有効 / 無効
- `data`: 種別ごとの設定値

複雑な型階層は最初から持ち込まず、
シリアライズしやすい JSON 構造を優先する。

---

## 初期コンポーネント案

最初の PlayUnit 設計では、次の component 群を中核とする。

## 1. Transform

位置、回転、スケール、描画順などの基礎情報。

```js
{
  type: 'Transform',
  enabled: true,
  data: {
    x: 0,
    y: 0,
    z: 0,
    rotation: 0,
    scaleX: 1,
    scaleY: 1
  }
}
```

## 2. Tilemap

既存 map asset やその layer を参照するための component。

```js
{
  type: 'Tilemap',
  enabled: true,
  data: {
    mapAssetId: 'map_01',
    layerId: 'layer_ground'
  }
}
```

## 3. PlaySettings

その PlayUnit 全体にひもづく設定。

実際の camera 実体は `Camera` component を持つ `PlayObject` で表し、
`PlaySettings` は既定 camera の選択を保持する薄い設定 object とする。

```js
{
  type: 'PlaySettings',
  enabled: true,
  data: {
    defaultCameraObjectId: 'obj_camera'
  }
}
```

## 4. Camera

PlayUnit 内の視点を表す component。

複数 camera を置けるようにし、将来的な分割画面、観戦 camera、
多人数プレイ camera の基礎にする。

```js
{
  type: 'Camera',
  enabled: true,
  data: {
    zoom: 1,
    viewportX: 0,
    viewportY: 0,
    viewportWidth: 0,
    viewportHeight: 0,
    followTargetObjectId: '',
    followLerp: 1
  }
}
```

通常は `Transform` と同一 object に付与し、camera object 自身の位置を
`Transform` 側で持つ。

## 5. Image

`pixelDocument` をそのまま表示するための最小 component。

現在の `pixelDocument` は単体のドット絵アセットとして扱っているため、
この段階では `Sprite` よりも `Image` の方が責務が明確である。

`Sprite` は将来的に tileset / atlas / frame animation を扱う段階で、
別 component として導入する。

```js
{
  type: 'Image',
  enabled: true,
  data: {
    pixelDocumentId: 'px_icon_01',
    alpha: 1,
    width: 0,
    height: 0,
    keepAspect: true,
    originX: 0,
    originY: 0
  }
}
```

最小の runtime 確認では、`Transform` と `Image` を同一 object に付与し、
`Transform.data.x`, `Transform.data.y` を描画座標、
`Image.data.pixelDocumentId` を参照先 asset、
`Image.data.alpha` を透明度として扱う。

`width`, `height` は 0 のとき元の `pixelDocument` サイズを使い、
0 より大きいときは preview 上でそのサイズへ拡大縮小して描画する。

`keepAspect` が `true` のときは、`width` または `height` の片側だけを指定しても
元画像の縦横比を維持して補完する。

`originX`, `originY` は 0.0 から 1.0 の範囲で扱い、
`Transform` の座標を画像のどの基準点として使うかを表す。
たとえば `(0, 0)` は左上、`(0.5, 0.5)` は中央、`(1, 1)` は右下になる。

## 6. Text

テキスト表示用の最小 component。

```js
{
  type: 'Text',
  enabled: true,
  data: {
    text: 'Hello World',
    font: '24px sans-serif',
    color: '#ffffff'
  }
}
```

最小の runtime 確認では、`Transform` と `Text` を同一 object に付与し、
`Transform.data.x`, `Transform.data.y` を描画座標、
`Text.data.text`, `Text.data.font`, `Text.data.color` を描画設定として扱う。

## 7. Collider

当たり判定と判定形状を担当する component。

`Collider` 自体はイベントを持たず、
「どの形で判定するか」「物理衝突として扱うか、trigger 判定として扱うか」を表す。

また、`Collider` は world 上の object 同士の overlap 判定だけでなく、
マウスカーソルや将来の touch / pointer の hit test にも使う共通の判定面とする。

最初の方針では `shape: 'rect'` を前提とし、
同一 object 上の `Transform` を基準にローカル矩形を構成する。

```js
{
  type: 'Collider',
  enabled: true,
  data: {
    shape: 'rect',
    offsetX: 0,
    offsetY: 0,
    width: 1,
    height: 1,
    isTrigger: false
  }
}
```

### 設計方針

- `Collider` は判定形状の正本とする
- `Trigger` は判定形状を自前で持たず、同一 object 上の `Collider` を利用する
- `isTrigger: true` の `Collider` は物理衝突解決ではなく、イベント判定用の領域として使う
- `Collider` は object 同士の衝突判定と pointer hit test の両方に使う
- 最初の pointer hit test は `rect` 前提とし、cursor enter / move / leave / down / up / click の基盤にする
- 将来 `circle` や複数 collider を扱いたくなった場合も、まずは `Collider` の責務拡張で吸収する

### 将来の pointer 系用途

- ボタン hover / press / click
- 物をつかむ、ドラッグする
- オブジェクト選択や inspect
- UI ライクな hotspot 判定

これらは専用の `Button` component をいきなり作るのではなく、
まず `Collider` による hit test と `Trigger` のイベント発火を組み合わせて表現できるようにする。

## 8. Trigger

接触や操作で反応するイベント起点。

`Trigger` は判定そのものを担当せず、
同一 object 上の `Collider` の判定結果を受けてイベントを発火する。

このため、最小構成では `Transform + Collider + Trigger` を同一 object に付与し、
runtime 側で `Collider` の重なり判定や pointer hit test が成立したとき `Trigger.eventId` を発火対象とする。

```js
{
  type: 'Trigger',
  enabled: true,
  data: {
    eventId: 'ev_message_hello',
    triggerOn: 'overlap',
    once: false,
    targetObjectId: ''
  }
}
```

### 設計方針

- `Trigger` はイベント意味だけを持つ
- 最初の実装では `eventId`, `once`, `targetObjectId`, `triggerOn` を最小項目とする
- `targetObjectId` が空のときは、runtime の既定対象を使う
- `once: true` のときは、一度発火した後に同じ条件では再発火しない
- `triggerOn` は最初は `overlap` を基準にし、将来 `pointerEnter`, `pointerMove`, `pointerLeave`, `pointerDown`, `pointerUp`, `click`, `dragStart`, `dragMove`, `dragEnd` へ広げられる形にする

### pointer 発火の考え方

- `pointerEnter`: カーソルが collider 内に入った瞬間
- `pointerMove`: カーソルが collider 内を移動している間
- `pointerLeave`: カーソルが collider 外へ出た瞬間
- `pointerDown`: collider 内でマウスボタンを押した瞬間
- `pointerUp`: collider 内でマウスボタンを離した瞬間
- `click`: collider 内で down / up が成立したとき

これにより、将来的には `Button` や `GrabHandle` のような高水準 component を追加しなくても、
`Collider + Trigger` の組み合わせで基本的な UI / インタラクションを記述しやすくする。

### Collider + Trigger の協調

- `Transform`: object の基準位置
- `Collider`: 判定形状と pointer hit test 面
- `Trigger`: 発火条件とイベント識別子

この 3 つを同一 object 上に持たせることで、
「どこにあるか」「どの範囲で反応するか」「何を起こすか」を分離する。

例:

```js
{
  id: 'obj_exit_gate',
  name: 'ExitGate',
  components: [
    { type: 'Transform', enabled: true, data: { x: 12, y: 8, z: 0, rotation: 0, scaleX: 1, scaleY: 1 } },
    { type: 'Collider', enabled: true, data: { shape: 'rect', offsetX: 0, offsetY: 0, width: 2, height: 1, isTrigger: true } },
    { type: 'Trigger', enabled: true, data: { eventId: 'ev_stage_clear', triggerOn: 'overlap', once: true, targetObjectId: 'obj_player' } }
  ]
}
```

この例では、`obj_player` が `ExitGate` の trigger collider に入ると
`ev_stage_clear` が発火する想定になる。

ボタン的な例:

```js
{
  id: 'obj_start_button',
  name: 'StartButton',
  components: [
    { type: 'Transform', enabled: true, data: { x: 160, y: 120, z: 0, rotation: 0, scaleX: 1, scaleY: 1 } },
    { type: 'Collider', enabled: true, data: { shape: 'rect', offsetX: 0, offsetY: 0, width: 96, height: 32, isTrigger: true } },
    { type: 'Trigger', enabled: true, data: { eventId: 'ev_start_game', triggerOn: 'click', once: false, targetObjectId: '' } }
  ]
}
```

この例では、pointer が button collider 上で click したとき `ev_start_game` が発火する想定になる。

## 9. Controller

操作される object に直接付与する最小 component。

`Controller` は別 object を参照して操作するのではなく、
付与された object 自身を操作対象とする。

最初の実装では `Transform` と同一 object に付与し、
入力に応じてその object の `Transform.data.x`, `Transform.data.y` を更新する。

```js
{
  type: 'Controller',
  enabled: true,
  data: {
    inputMode: 'player1',
    moveSpeed: 120
  }
}
```

### 設計方針

- `Controller` は操作対象 object に直接付与する
- 初期仕様では `targetObjectId` は持たない
- 将来必要になれば、遠隔操作や憑依のための拡張項目として別途追加する
- `Transform` が存在しない object に付与しても移動処理は行わない

---

## 抽象化の対応表

これまで専用配列で持たせる案があった要素は、次のように object + component へ吸収する。

| 旧イメージ | 新しい表現 |
|-----------|------------|
| `tilemaps[]` | `Tilemap` component を持つ `PlayObject` |
| `settings` | `PlaySettings` component を持つ `PlayObject` |
| `camera` | `Camera` component を持つ `PlayObject` + `PlaySettings.defaultCameraObjectId` |
| `player control` | `Controller` component を持つ `PlayObject` |

この方式により、構造が一貫し、編集 UI も object inspector ベースで統一しやすくなる。

プレイヤーに関する object / component 構成は、現段階ではこの文書のスコープ外とし、別途設計する。

---

## 例: 最小 PlayUnitData

```js
{
  id: 'pu_town_01',
  name: 'Town',
  objects: [
    {
      id: 'obj_root',
      name: 'Root',
      enabled: true,
      parentId: null,
      children: ['obj_camera', 'obj_settings', 'obj_tilemap_ground'],
      components: []
    },
    {
      id: 'obj_camera',
      name: 'CameraObject',
      enabled: true,
      parentId: 'obj_root',
      children: [],
      components: [
        { type: 'Transform', enabled: true, data: { x: 0, y: 0, z: 0 } },
        { type: 'Camera', enabled: true, data: { zoom: 1, viewportX: 0, viewportY: 0, viewportWidth: 0, viewportHeight: 0, followTargetObjectId: '', followLerp: 1 } }
      ]
    },
    {
      id: 'obj_tilemap_ground',
      name: 'Ground Tilemap',
      enabled: true,
      parentId: 'obj_root',
      children: [],
      components: [
        { type: 'Transform', enabled: true, data: { x: 0, y: 0, z: 0 } },
        { type: 'Tilemap', enabled: true, data: { mapAssetId: 'map_01', layerId: 'layer_ground' } }
      ]
    },
    {
      id: 'obj_settings',
      name: 'PlaySettingsObject',
      enabled: true,
      parentId: 'obj_root',
      children: [],
      components: [
        { type: 'PlaySettings', enabled: true, data: { defaultCameraObjectId: 'obj_camera' } }
      ]
    },
    {
      id: 'obj_trigger_message_01',
      name: 'Message Trigger',
      enabled: true,
      parentId: 'obj_root',
      children: [],
      components: [
        { type: 'Transform', enabled: true, data: { x: 10, y: 8, z: 1 } },
        { type: 'Collider', enabled: true, data: { shape: 'rect', width: 1, height: 1, isTrigger: true } },
        { type: 'Trigger', enabled: true, data: { triggerType: 'overlap', actionType: 'message', actionValue: 'hello' } }
      ]
    }
  ]
}
```

### 新規作成時の初期構成

`PlayUnitData.createDefault(name)` では、最低限次の object 群を自動生成する。

- `Root`
- `CameraObject`
- `PlaySettingsObject`

`CameraObject` と `PlaySettingsObject` は `Root` の子として作成し、
`PlaySettingsObject` の `PlaySettings.data.defaultCameraObjectId` には
生成した `CameraObject` の ID を設定する。

---

## 既存システムとの接続方針

## 1. ProjectData への追加

将来的には `ProjectData.assets` に `playUnits` を追加する。

```js
assets: {
  pixelDocuments: [],
  tilesets: [],
  maps: [],
  playUnits: []
}
```

ただし今回の文書では、実装ではなく構造合意までを対象とする。

## 2. Map asset との関係

既存の `map` は PlayUnit に取り込まれて消えるのではなく、
当面は `Tilemap` component から参照されるアセットとして残す。

この方針の利点:

- 既存の `MapEditorScene` をすぐ壊さずに済む
- タイルマップ編集とゲーム構成編集を段階的に分離できる
- 将来、1つの PlayUnit に複数 map layer や複数 map asset を含めやすい

## 3. Scene との接続

`PlayUnit` 自体は Scene ではない。

想定される接続は次のとおり。

- `ProjectTopScene` で PlayUnit 一覧を開く
- `PlayUnitEditorScene` で object / component を編集する
- `PlayTestScene` で `PlayUnitRuntime` として再生する

---

## Runtime 側の見通し

`Runtime` という語は保存データ名ではなく、
`PlayUnitData` を実行状態に変換した側へ使う。

例:

- `PlayUnitRuntime`
- `PlayObjectRuntime`
- `ComponentRuntime`

役割:

- object 一覧から更新対象を抽出する
- `Trigger` などの動作を解決する
- タイルマップ参照とプレイヤー位置から描画対象を組み立てる

保存データと実行状態の責務を分けることで、
エディタと runtime の双方を整理しやすくする。

---

## 補足方針

## 1. `tilemaps` `settings` を専用 property で持たない

方針:

- object inspector ベースの統一編集がしにくい
- 新しい要素を足すたびに top-level schema が増える
- データ構造の一貫性が崩れやすい

## 2. `Runtime` は保存単位名ではなく実行側に使う

方針:

- runtime は通常、実行中インスタンスを指すため意味がぶれる
- 保存構造と実行構造の名前を分けたほうが保守しやすい

---

## 次の設計フェーズ候補

1. `PlayUnitData` / `PlayObjectData` / `ComponentData` の JavaScript クラス定義
2. `ProjectData` への `playUnits` 追加設計
3. `PlayUnitEditorScene` の UI 構成設計
4. `PlayUnitRuntime` の最小再生仕様設計
5. 既存 `MapEditorScene` と `TilemapComponent` の橋渡し設計

---