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

### enabled の扱い

初期仕様では、object の `enabled` は runtime 上の有効判定に直接影響する。

- `enabled: false` の object は描画、更新、hit test、event 発火の対象外とする
- 親 object が `enabled: false` の場合、その子孫 object もまとめて無効とみなす
- component の `enabled` は object の有効状態とは別に評価し、個別 component だけを無効化できるようにする

runtime は、対象 object 自身が `enabled` であり、かつ祖先 object もすべて `enabled` である場合にのみ、
その object を有効とみなす。

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

## 5. UICanvas

UI 用 object tree の起点になる container component。

`UICanvas` は独自の `children` 配列を持たず、
通常の `PlayObject.children` / `parentId` による親子関係を正本とする。

これにより、UI 要素も world object と同じ object tree 上で管理でき、
将来の `Collider` / `Trigger` / `EventAction` などを object 単位で一貫して扱える。

```js
{
  type: 'UICanvas',
  enabled: true,
  data: {
    renderMode: 'screen',
    sortOrder: 100,
    referenceWidth: 640,
    referenceHeight: 360
  }
}
```

### 設計方針

- `UICanvas` は「この object 配下を UI 座標系として解釈する」ことを示す
- `UICanvas` 自身は描画内容を持たず、見た目は子 object の component で表現する
- UI 要素の所有関係は `PlayObject.children` / `parentId` に統一する
- 初期仕様では `renderMode: 'screen'` のみを扱う
- 描画順は `UICanvas.data.sortOrder` を canvas 単位の優先度として使い、その中の並びは object 側の `children` 順で解決する
- nested canvas は将来拡張候補とし、初期仕様では 1 つの canvas 配下を単純に走査する

### 最小プロパティ

- `renderMode`: 初期仕様では `'screen'` 固定
- `sortOrder`: UI canvas 同士の描画優先順位
- `referenceWidth`: 基準画面幅
- `referenceHeight`: 基準画面高さ

### 描画順と hit test の規則

初期仕様では、UI の sibling order は `PlayObject.children` の配列順をそのまま使う。

- 同一親を持つ child object は `children` 配列の先頭から順に描画する
- 同一親を持つ child object は `children` 配列の末尾ほど前面に出る
- pointer hit test は描画順と逆順で評価し、最も手前に見えている object を優先して拾う
- 初期仕様では UI child object に個別の `z` や `zIndex` は持たせない

この方針により、editor 上の object tree の並びと、runtime の描画結果、
pointer の当たり判定順を一致させやすくする。

### world 描画との分離

初期仕様では、world object の描画と UI canvas の描画を別パスとして扱う。

```text
1. world object を `Transform.data.z` で描画する
2. その後に UI canvas 群を `UICanvas.data.sortOrder` で描画する
3. 各 UI canvas 内は object tree の `children` 順で描画する
```

このとき、`UICanvas.data.sortOrder` は UI canvas 同士の比較にのみ使い、
world object の `Transform.data.z` とは比較しない。

これにより、screen-space UI が world object に埋もれず、
camera から独立した HUD や menu として安定して扱える。

### 入力フォーカスの規則

初期仕様では、UI の pointer 入力対象は常に 1 つの object に限定する。

- runtime は pointer 座標に対して、最前面から逆順に hit test を行う
- 最初に hit した object をその時点の active target とする
- `pointerEnter`, `pointerLeave`, `pointerDown`, `pointerUp`, `click` は active target を基準に判定する
- 同一フレームで複数 object に同種の pointer event を配送しない
- event bubbling や capture phase は初期仕様では導入しない

この方針により、button や menu などの基本 UI を、複雑なイベント伝播なしで
一貫して扱えるようにする。

### `click` 判定の規則

初期仕様では、`click` は `pointerDown` と `pointerUp` が同一 object 上で成立した場合にのみ発火する。

- runtime は `pointerDown` 時に、その object を `pressedTarget` として保持する
- `pointerUp` 時に hit した object が `pressedTarget` と同一である場合のみ `click` を発火する
- `pointerDown` 後に pointer が object 外へ出た場合、その object に対する `click` は成立しない
- `pointerDown` と `pointerUp` が異なる object 上で発生した場合、`click` は発火しない

この方針により、押下中のキャンセルや、drag 開始前の基本的な button 挙動を
自然に扱えるようにする。

### drag 系 event の規則

初期仕様では、`dragStart`, `dragMove`, `dragEnd` は `pressedTarget` を起点に判定する。

- runtime は `pointerDown` で確定した `pressedTarget` を drag 候補として保持する
- `pointerDown` 後に pointer の移動量が所定の閾値を超えた時点で `dragStart` を発火する
- `dragStart` 後は、pointer が object 外へ出ても同じ `pressedTarget` に対して `dragMove` を継続して発火する
- `pointerUp` 時には、その時点で drag 中だった `pressedTarget` に対して `dragEnd` を発火する
- drag が開始された後は、同じ入力系列に対する `click` は発火しない

この方針により、slider、drag handle、ドラッグ移動のような操作を、
pointer の現在位置とは独立した一貫した対象 object に対して扱えるようにする。

### drag 開始閾値

初期仕様では、drag 開始判定に使う閾値は UI 座標系上のピクセル単位で扱う。

- 閾値は `UICanvas` の screen-space 座標系における固定ピクセル値とする
- 初期の既定値は 4 から 6 px 程度を想定する
- 判定は `pointerDown` 時点の座標と現在座標の差分から行う
- 実装上は距離の二乗比較を使ってもよい

この方針により、button と drag を安定して切り分けつつ、
screen-space UI の実装を単純に保てる。

### pointer event payload

初期仕様では、すべての pointer 系 event は共通の基礎 payload を持つ。

```js
{
  eventId: 'ev_start_game',
  triggerOn: 'click',
  sourceObjectId: 'obj_start_button',
  targetObjectId: 'obj_start_button',
  pointerId: 'mouse0',
  x: 120,
  y: 48,
  deltaX: 3,
  deltaY: -1,
  button: 0
}
```

 - `eventId` はどの論理イベントが発火したかを表す
 - `triggerOn` はどの条件で発火したかを表す
- `pointerId` は入力系列を識別する文字列 ID とする
- `sourceObjectId` はこの event を発火した Trigger 所有 object を表す
- `targetObjectId` は event の処理対象 object を表す
- `x`, `y` は現在の UI 座標
- `deltaX`, `deltaY` は前回更新からの移動差分
- `button` は押下に使われたボタン識別子を表す

### event payload の object 参照

初期仕様では、event payload は `sourceObjectId` を共通項目として持つ。

また、`eventId` と `triggerOn` も全 event payload の共通項目とする。

- `eventId` は発火した論理イベント ID を表す
- `triggerOn` は発火条件の種別を表す
- `sourceObjectId` は Trigger の発火元 object を表す
- `targetObjectId` は event の処理対象 object を表す
- `sourceObjectId` と `targetObjectId` が同一であってもよい
- `targetObjectId` が明示指定されていても、`sourceObjectId` により発火元を失わない

この方針により、発火元 object と処理対象 object を分離して扱えるようにする。

### `button` の値域

初期仕様では、`button` は Web の一般的な mouse button 定義に合わせて次の数値に限定する。

- `0`: primary button
- `1`: middle button
- `2`: secondary button

初期仕様では、`click` は `button = 0` の入力に対してのみ正式対応とする。

- `button = 1`, `button = 2` に対する個別挙動は初期仕様では導入しない
- 将来必要になった場合に、context menu や追加の button 対応を別仕様として拡張する

`pointerDown`, `pointerUp`, `pointerMove`, `click` は、この共通 payload 形式を基礎として持つ。

- `pointerDown`, `pointerUp` では `deltaX`, `deltaY` を 0 としてよい
- `pointerMove` では現在座標と差分を更新する
- `click` でも同じキー集合を持つことで、event 利用側の解釈を揃えやすくする

### `pointerId` の定義

初期仕様では、`pointerId` は常に存在する入力系列 ID として扱う。

- 最初の実装では `mouse0` を既定値としてよい
- `pointerId` は数値ではなく文字列として扱う
- この ID は将来の touch や pen を含む複数入力系列へ拡張可能な識別子として定義する
- `pointerType` のような追加種別は、必要になるまで導入しない

### drag event payload

初期仕様では、`dragStart`, `dragMove`, `dragEnd` は共通 pointer payload に加えて
drag 開始点からの累積差分を持つ。

```js
{
  eventId: 'ev_slider_drag',
  triggerOn: 'dragMove',
  sourceObjectId: 'obj_slider_knob',
  targetObjectId: 'obj_slider_knob',
  pointerId: 'mouse0',
  x: 120,
  y: 48,
  deltaX: 3,
  deltaY: -1,
  button: 0,
  dragStartX: 100,
  dragStartY: 40,
  dragOffsetX: 20,
  dragOffsetY: 8
}
```

- `dragStartX`, `dragStartY` は `pointerDown` 時点の開始座標
- `dragOffsetX`, `dragOffsetY` は開始座標から現在座標までの累積差分
- `dragStart`, `dragMove`, `dragEnd` は同じ入力系列に対して同一形式の座標情報を持てるようにする
- drag 系 event は pointer 系 event の共通キーを引き継いだ上で追加情報を持つ

### 条件付き payload 項目

初期仕様では、`otherObjectId` は相手 object が意味を持つ event に対してのみ付与する。

- `overlap` 系 event では `otherObjectId` に接触相手 object の ID を入れてよい
- pointer 系 event では `otherObjectId` を共通必須項目にしない
- 相手 object が存在しない event では `otherObjectId` を省略してよい

例:

```js
{
  eventId: 'ev_stage_clear',
  triggerOn: 'overlap',
  sourceObjectId: 'obj_exit_gate',
  targetObjectId: 'obj_exit_gate',
  otherObjectId: 'obj_player'
}
```

### keyboard / gamepad focus の扱い

初期仕様では、keyboard / gamepad による UI focus 移動は対象外とする。

- `UICanvas` の入力仕様は pointer 入力のみを対象にする
- focusable object の選択規則、上下左右ナビゲーション規則、focus ring 描画は初期仕様では導入しない
- 将来必要になった時点で、`UISelectable` や `Navigation` のような別仕様として拡張する

この方針により、初期の UI 実装は pointer ベースの基礎挙動に集中し、
入力モデルの混在を避ける。

## 6. UITransform

UI object の位置、サイズ、基準点を表す component。

world object が `Transform` を使うのに対し、UI object は `UITransform` を使う。
これにより、world 座標と screen 座標を同じ component に混在させずに済む。

```js
{
  type: 'UITransform',
  enabled: true,
  data: {
    x: 0,
    y: 0,
    width: 0,
    height: 0,
    anchorX: 0,
    anchorY: 0,
    pivotX: 0,
    pivotY: 0
  }
}
```

### 設計方針

- `UITransform` は `UICanvas` 配下の object に付与する前提とする
- `x`, `y` は canvas 基準座標で扱う
- `width`, `height` は UI 要素のレイアウトサイズとして扱う
- `anchorX`, `anchorY` は親領域のどこを基準に配置するかを表す
- `pivotX`, `pivotY` は object 自身のどこを座標基準にするかを表す
- 初期仕様では回転や複雑な stretch は持ち込まず、最小限の固定配置を優先する

### 最小プロパティ

- `x`, `y`: canvas 基準の配置座標
- `width`, `height`: 要素サイズ
- `anchorX`, `anchorY`: 0.0 から 1.0 の範囲で扱う親基準点
- `pivotX`, `pivotY`: 0.0 から 1.0 の範囲で扱う自身の基準点

### 厳密な計算規則

`UITransform` は、親矩形上の anchor 点と、自身矩形上の pivot 点を使って
最終的な UI rect を確定する。

初期仕様では、UI 座標系は左上原点、右方向を `+x`、下方向を `+y` とする。

- `anchorX`, `anchorY` は親矩形上の基準点を表す
- `pivotX`, `pivotY` は自身矩形上の基準点を表す
- `x`, `y` は親 anchor 点から自身 pivot 点までのオフセットを表す
- `width`, `height` は自身矩形のサイズを表す

親矩形を `parentLeft`, `parentTop`, `parentWidth`, `parentHeight` としたとき、
anchor 点は次の式で求める。

```text
anchorPointX = parentLeft + parentWidth * anchorX
anchorPointY = parentTop + parentHeight * anchorY
```

自身矩形の左上座標は次の式で求める。

```text
left = anchorPointX + x - width * pivotX
top = anchorPointY + y - height * pivotY
```

最終矩形は次のとおりとする。

```text
right = left + width
bottom = top + height
```

この計算により、anchor と pivot の意味を固定する。

- `anchor=(0,0)`, `pivot=(0,0)` は左上基準配置
- `anchor=(0.5,0.5)`, `pivot=(0.5,0.5)` は中央基準配置
- `anchor=(1,1)`, `pivot=(1,1)` は右下基準配置

### 親矩形の決定規則

`UITransform` の計算に使う親矩形は、object tree の親から次の規則で決定する。

- 親 object が `UICanvas` を持つ場合、親矩形は `0, 0, referenceWidth, referenceHeight` とする
- 親 object が `UITransform` を持つ場合、親矩形はその object の確定済み UI rect とする
- 親 object が `UICanvas` も `UITransform` も持たない場合、その `UITransform` は無効構成とみなす

runtime は `UICanvas` 配下の object tree を親から子へ順に走査し、
親矩形が確定した後に子の rect を確定する。

### `UITransform` 不在時の扱い

`UICanvas` 配下の object が UI 要素として描画または hit test 対象になるには、
原則としてその object 自身が `UITransform` を持つ必要がある。

- `Image`, `Text`, `Collider`, `Trigger` を UI object として使う場合、その object は `UITransform` を持つべきとする
- `UICanvas` 配下にあっても `UITransform` を持たない object は、初期仕様では UI 描画対象および UI hit test 対象から除外する
- validator または editor は、`UICanvas` 配下で UI 系 component を持つのに `UITransform` がない object を警告またはエラーとして扱う
- runtime は暗黙の `UITransform` 補完を行わない

この方針により、UI object の配置基準を曖昧にせず、
構成ミスを早い段階で検出しやすくする。

### 初期仕様の制限

- `anchorX`, `anchorY`, `pivotX`, `pivotY` は 0.0 から 1.0 の範囲に制限する
- `width`, `height` は固定数値のみを扱い、式や stretch は導入しない
- 回転、scale、skew は `UITransform` では扱わない
- 子 object の配置基準は親の見た目ではなく、親 `UITransform` の layout rect を使う
- 描画と pointer hit test は同じ確定 rect を使う
- 親子間のクリッピングは初期仕様では行わない

### クリッピング方針

初期仕様では、親 `UITransform` の矩形から child object がはみ出していても、
その child object は通常どおり描画および hit test 対象とする。

- parent rect は child 配置の基準には使うが、描画や hit test の切り抜きには使わない
- clip rect や mask は初期仕様では導入しない
- スクロールビューやマスク UI が必要になった時点で、専用 component または別仕様で拡張する

将来、stretch や複雑なレスポンシブ配置が必要になった場合は、
`anchorMin` / `anchorMax` のような別仕様を追加して拡張する。

## 7. Image

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

`Image` は world object にも UI object にも付与できる。

- world object では `Transform` を描画基準に使う
- UI object では `UITransform` を描画基準に使う

## 8. Text

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

`Text` も `Image` と同様に、world object と UI object の両方で利用できる。

- world object では `Transform` を描画基準に使う
- UI object では `UITransform` を描画基準に使う

## 9. Collider

当たり判定と判定形状を担当する component。

`Collider` 自体はイベントを持たず、
「どの形で判定するか」「物理衝突として扱うか、trigger 判定として扱うか」を表す。

また、`Collider` は world 上の object 同士の overlap 判定だけでなく、
マウスカーソルや将来の touch / pointer の hit test にも使う共通の判定面とする。

最初の方針では `shape: 'rect'` を前提とし、
同一 object 上の `Transform` または `UITransform` を基準にローカル矩形を構成する。

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
- UI object 上では `UITransform` 基準の hit test 面としても使う
- 最初の pointer hit test は `rect` 前提とし、cursor enter / move / leave / down / up / click の基盤にする
- 将来 `circle` や複数 collider を扱いたくなった場合も、まずは `Collider` の責務拡張で吸収する

### `isTrigger` の意味

初期仕様では、`Collider.data.isTrigger` は world 側の衝突解決に対する意味だけを持つ。

- `isTrigger: true` は、その collider を物理衝突解決の対象ではなく判定専用領域として扱うことを表す
- `overlap` 系 trigger では、この判定専用領域の意味を利用できる
- pointer 系 trigger は `Collider` の存在を前提とするが、`isTrigger` の値には依存しない
- したがって、solid な world object でも同じ `Collider` を pointer hit test 面として利用できる

この方針により、`Collider` の形状責務、`isTrigger` の world 衝突責務、
`Trigger` の event 条件責務を分離する。

### 将来の pointer 系用途

- ボタン hover / press / click
- 物をつかむ、ドラッグする
- オブジェクト選択や inspect
- UI ライクな hotspot 判定

これらは専用の `Button` component をいきなり作るのではなく、
まず `Collider` による hit test と `Trigger` のイベント発火を組み合わせて表現できるようにする。

## 10. Trigger

接触や操作で反応するイベント起点。

`Trigger` は判定そのものを担当せず、
同一 object 上の `Collider` の判定結果を受けてイベントを発火する。

このため、最小構成では `Transform + Collider + Trigger` または
`UITransform + Collider + Trigger` を同一 object に付与し、runtime 側で
`Collider` の重なり判定や pointer hit test が成立したとき `Trigger.eventId` を発火対象とする。

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
- `triggerOn` は初期仕様では `overlap`, `pointerEnter`, `pointerMove`, `pointerLeave`, `pointerDown`, `pointerUp`, `click`, `dragStart`, `dragMove`, `dragEnd` を正式対応とする

### `targetObjectId` が空のときの既定対象

初期仕様では、`Trigger.data.targetObjectId` が空文字列のとき、既定対象はその `Trigger` を持つ object 自身とする。

- `targetObjectId` が空のときは、Trigger 所有 object を対象 object とみなす
- `targetObjectId` が指定されている場合のみ、明示された別 object を対象 object とする
- `overlap` 相手や pointer 情報のような実行時文脈は、必要に応じて event payload 側で扱う

この方針により、Trigger の所有元と処理対象の既定関係を単純に保ち、
trigger 種別ごとの暗黙解釈差を避ける。

### `once` の意味

初期仕様では、`once: true` は `triggerOn` の種類に関係なく同じ意味で扱う。

- `Trigger` が一度でも発火したら、その PlayUnit 実行中は再発火しない
- `pointerEnter`, `dragStart`, `click` など trigger 種別によって `once` の意味を変えない
- pointer が一度外へ出たことや、条件が一度解除されたことでは再装填しない
- 再発火させたい場合は、将来別仕様として resettable trigger や cooldown を導入する

この方針により、`once` の意味を単純な再発火禁止フラグとして保ち、
trigger 種別ごとの解釈差を避ける。

### `triggerOn` の pointer 系正式列挙

初期仕様で `Trigger.data.triggerOn` に指定できる pointer 系イベントは次のとおりとする。

- `pointerEnter`
- `pointerMove`
- `pointerLeave`
- `pointerDown`
- `pointerUp`
- `click`
- `dragStart`
- `dragMove`
- `dragEnd`

次のイベントは初期仕様では対象外とする。

- `doubleClick`
- `longPress`
- `wheel`
- `contextMenu`
- `focus`
- `blur`

これらは追加の状態管理や入力モデルを必要とするため、必要になった時点で別仕様として拡張する。

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
- `UITransform`: UI object の基準位置とサイズ
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

UI ボタン的な例:

```js
{
  parentId: 'obj_ui_root',
  id: 'obj_start_button',
  name: 'StartButton',
  children: ['obj_start_button_label'],
  components: [
    { type: 'UITransform', enabled: true, data: { x: 160, y: 120, width: 96, height: 32, anchorX: 0, anchorY: 0, pivotX: 0, pivotY: 0 } },
    { type: 'Collider', enabled: true, data: { shape: 'rect', offsetX: 0, offsetY: 0, width: 96, height: 32, isTrigger: true } },
    { type: 'Trigger', enabled: true, data: { eventId: 'ev_start_game', triggerOn: 'click', once: false, targetObjectId: '' } }
  ]
}
```

この例では、`UICanvas` 配下の button object に対して pointer が click したとき
`ev_start_game` が発火する想定になる。

## 11. Controller

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
| `ui canvas` | `UICanvas` component を持つ親 `PlayObject` + UI child object 群 |

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
      id: 'obj_ui_root',
      name: 'UIRoot',
      enabled: true,
      parentId: 'obj_root',
      children: ['obj_ui_score_text'],
      components: [
        { type: 'UICanvas', enabled: true, data: { renderMode: 'screen', sortOrder: 100, referenceWidth: 640, referenceHeight: 360 } }
      ]
    },
    {
      id: 'obj_ui_score_text',
      name: 'ScoreText',
      enabled: true,
      parentId: 'obj_ui_root',
      children: [],
      components: [
        { type: 'UITransform', enabled: true, data: { x: 12, y: 12, width: 120, height: 24, anchorX: 0, anchorY: 0, pivotX: 0, pivotY: 0 } },
        { type: 'Text', enabled: true, data: { text: 'Score: 0', font: '16px sans-serif', color: '#ffffff' } }
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

UI を使う PlayUnit では、必要に応じて `UICanvas` を持つ `UIRoot` object を
`Root` 配下に追加する。

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