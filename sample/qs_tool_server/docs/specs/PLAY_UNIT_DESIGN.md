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

その PlayUnit にひもづく設定。

```js
{
  type: 'PlaySettings',
  enabled: true,
  data: {}
}
```

## 4. Sprite

スプライト表示用の見た目情報。

```js
{
  type: 'Sprite',
  enabled: true,
  data: {
    tilesetId: 'ts_player',
    tileIndex: 0
  }
}
```

## 5. Collider

当たり判定または判定用サイズ。

```js
{
  type: 'Collider',
  enabled: true,
  data: {
    shape: 'rect',
    width: 1,
    height: 1,
    isTrigger: false
  }
}
```

## 6. Trigger

接触や操作で反応するイベント起点。

```js
{
  type: 'Trigger',
  enabled: true,
  data: {
    triggerType: 'overlap',
    actionType: 'message',
    actionValue: 'hello'
  }
}
```

---

## 抽象化の対応表

これまで専用配列で持たせる案があった要素は、次のように object + component へ吸収する。

| 旧イメージ | 新しい表現 |
|-----------|------------|
| `tilemaps[]` | `Tilemap` component を持つ `PlayObject` |
| `settings` | `PlaySettings` component を持つ `PlayObject` |

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
      children: ['obj_tilemap_ground', 'obj_settings', 'obj_player'],
      components: []
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
      name: 'Play Settings',
      enabled: true,
      parentId: 'obj_root',
      children: [],
      components: [
        { type: 'PlaySettings', enabled: true, data: {} }
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