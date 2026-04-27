# Camera / PlaySettings JSON 例集

最終更新: 2026-04-27

## 目的

`PlayUnitEditorScene` では現在 `Camera` component と `PlaySettings` component の parameter を JSON で直接編集するため、
そのまま貼り付けて試せる例をまとめる。

この文書の例は、基本的に `Camera.data` または `PlaySettings.data` にそのまま入れる想定で記述する。

---

## 前提

- camera 実体は `Camera` component を持つ `PlayObject` で表現する
- 通常は同じ object に `Transform` も付ける
- `Transform.data.x`, `Transform.data.y` が camera の位置になる
- `PlaySettings.data.defaultCameraObjectId` で既定 camera object を選ぶ
- 現在の `PlayTestScene` で実際に反映されるのは `defaultCameraObjectId` と `Camera.data.zoom`、および camera object 側の `Transform.x`, `Transform.y`
- `followTargetObjectId`, `followLerp`, `viewportX`, `viewportY`, `viewportWidth`, `viewportHeight` は将来拡張を見据えた項目であり、現時点では主にテンプレート用途

---

## Camera 基本テンプレート

```json
{
  "zoom": 1,
  "viewportX": 0,
  "viewportY": 0,
  "viewportWidth": 0,
  "viewportHeight": 0,
  "followTargetObjectId": "",
  "followLerp": 1
}
```

---

## PlaySettings 基本テンプレート

```json
{
  "defaultCameraObjectId": "obj_camera"
}
```

---

## 例1: 最小の既定 camera

用途: まず camera が解決されるか確認したいとき

`Camera.data`

```json
{
  "zoom": 1,
  "viewportX": 0,
  "viewportY": 0,
  "viewportWidth": 0,
  "viewportHeight": 0,
  "followTargetObjectId": "",
  "followLerp": 1
}
```

`PlaySettings.data`

```json
{
  "defaultCameraObjectId": "obj_camera"
}
```

---

## 例2: 少し寄った preview

用途: Text を拡大して見たいとき

`Camera.data`

```json
{
  "zoom": 1.5,
  "viewportX": 0,
  "viewportY": 0,
  "viewportWidth": 0,
  "viewportHeight": 0,
  "followTargetObjectId": "",
  "followLerp": 1
}
```

---

## 例3: 引きの preview

用途: 画面内により広い範囲を見たいとき

`Camera.data`

```json
{
  "zoom": 0.75,
  "viewportX": 0,
  "viewportY": 0,
  "viewportWidth": 0,
  "viewportHeight": 0,
  "followTargetObjectId": "",
  "followLerp": 1
}
```

---

## 例4: 右下へずらした camera

用途: ワールド座標の原点から少し離れた場所を見たいとき

この場合は `Camera.data` ではなく camera object 側の `Transform.data` を調整する。

`Transform.data`

```json
{
  "x": 120,
  "y": 80,
  "z": 0,
  "rotation": 0,
  "scaleX": 1,
  "scaleY": 1
}
```

`Camera.data`

```json
{
  "zoom": 1,
  "viewportX": 0,
  "viewportY": 0,
  "viewportWidth": 0,
  "viewportHeight": 0,
  "followTargetObjectId": "",
  "followLerp": 1
}
```

---

## 例5: 大きく寄った HUD 確認用 camera

用途: 少数の Text を大きく確認したいとき

`Transform.data`

```json
{
  "x": 40,
  "y": 20,
  "z": 0,
  "rotation": 0,
  "scaleX": 1,
  "scaleY": 1
}
```

`Camera.data`

```json
{
  "zoom": 2,
  "viewportX": 0,
  "viewportY": 0,
  "viewportWidth": 0,
  "viewportHeight": 0,
  "followTargetObjectId": "",
  "followLerp": 1
}
```

---

## 例6: 別 camera へ切り替える PlaySettings

用途: 複数 camera object を置いて、既定 camera を切り替えたいとき

`PlaySettings.data`

```json
{
  "defaultCameraObjectId": "obj_camera_battle"
}
```

---

## 例7: follow 予定の camera

用途: 将来プレイヤー追従や注目対象追従へ拡張したいときの下書き

現時点では `followTargetObjectId` と `followLerp` は preview 未対応だが、
後続実装に向けた保存データ例として使える。

`Camera.data`

```json
{
  "zoom": 1,
  "viewportX": 0,
  "viewportY": 0,
  "viewportWidth": 0,
  "viewportHeight": 0,
  "followTargetObjectId": "obj_player",
  "followLerp": 0.2
}
```

---

## 例8: split screen 予定の camera

用途: 将来の多人数プレイや観戦 camera を見据えた下書き

現時点では viewport 系は preview 未対応だが、複数 camera を置く前提の保存例として使える。

左半分用 `Camera.data`

```json
{
  "zoom": 1,
  "viewportX": 0,
  "viewportY": 0,
  "viewportWidth": 640,
  "viewportHeight": 720,
  "followTargetObjectId": "obj_player_1",
  "followLerp": 1
}
```

右半分用 `Camera.data`

```json
{
  "zoom": 1,
  "viewportX": 640,
  "viewportY": 0,
  "viewportWidth": 640,
  "viewportHeight": 720,
  "followTargetObjectId": "obj_player_2",
  "followLerp": 1
}
```

---

## よく使う組み合わせ

### まず preview を確認したい

- `defaultCameraObjectId` を実在する camera object ID にする
- `zoom: 1`
- camera object の `Transform.x`, `Transform.y` を `0, 0` にする

### Text を大きく見たい

- `zoom` を `1.5` から `2` にする
- camera object の `Transform` は小さめの値に保つ

### 離れた位置を見たい

- camera object の `Transform.x`, `Transform.y` を動かす
- `zoom` は `1` のままにする

### 複数 camera を切り替えたい

- camera object を複数用意する
- `PlaySettings.data.defaultCameraObjectId` を切り替える

---

## 最小確認用の組み合わせ例

camera object 側の `Transform.data`

```json
{
  "x": 0,
  "y": 0,
  "z": 0,
  "rotation": 0,
  "scaleX": 1,
  "scaleY": 1
}
```

camera object 側の `Camera.data`

```json
{
  "zoom": 1,
  "viewportX": 0,
  "viewportY": 0,
  "viewportWidth": 0,
  "viewportHeight": 0,
  "followTargetObjectId": "",
  "followLerp": 1
}
```

`PlaySettings.data`

```json
{
  "defaultCameraObjectId": "obj_camera"
}
```

---

## 補足

- 現在の `PlayTestScene` では `defaultCameraObjectId`, camera object の `Transform.x`, `Transform.y`, `Camera.data.zoom` が実際に使われる
- `viewportX`, `viewportY`, `viewportWidth`, `viewportHeight` は将来の複数 camera / 分割画面向けの項目であり、まだ preview には反映されない
- `followTargetObjectId`, `followLerp` も現在は保存データ上の準備段階である
- `PlaySettings` は既定 camera の選択を持つ薄い設定 object として使う