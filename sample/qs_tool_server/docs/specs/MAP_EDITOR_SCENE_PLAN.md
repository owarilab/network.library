# マップエディタシーン実装計画書

最終更新: 2026-04-26

## 目的

`ProjectTopScene` から入れる `MapEditorScene` を追加し、
既存のドット絵エディタと同じく `UIWindow` ベースの複数パネル構成で
マップ編集機能を段階的に拡張できる土台を作る。

今回のゴールは「完成版マップエディタ」ではなく、次を満たす雛形である。

- プロジェクトからマップアセットを開ける
- マップ用データを `ProjectData.assets.maps` に保存できる
- マップ表示領域、タイル選択領域、インスペクタをウィンドウとして分離する
- 既存の `tileset` アセットを流用してセルへ配置できる

---

## 既存実装から流用する点

### 1. シーン導線

- `TitleScene` → `ProjectTopScene` → 各エディタ、という遷移はすでに確立済み
- `ProjectTopScene` はアセット選択と scene 切り替えハブとして使う
- `ProjectSession.activeDocumentRef` を更新してから scene を切り替える流れを踏襲する

### 2. ウィンドウ UI 基盤

- `UIWindow` のドラッグ可能ウィンドウ基底をそのまま使う
- ドット絵エディタの `ToolBarWindow` / `LayerPanelWindow` / `ChipPaletteWindow` と同じく、
  コンテンツ描画とイベント処理をウィンドウ単位に分割する

### 3. タイル表示ロジック

- `TilesetData.compositeChip()` を使い、既存タイルセットのチップ画像をそのまま表示する
- `ChipPalette` / `TilePreview` が持つ offscreen canvas + nearest-neighbor 描画パターンを再利用する

### 4. 保存フロー

- `AppData.saveActiveProjectAssetState()` に `map` を追加し、
  `ProjectTopScene` からの browser 保存 / `.qsproj` export で同一経路を通す
- `ProjectSerializer` / `ProjectBrowserStorage` はすでに `mapData` JSON を扱えるため、
  追加で壊さずに接続する

---

## 今回の雛形の構成

`MapEditorScene` は次の 3 ウィンドウで始める。

1. `MapViewWindow`
   - マップ全体をグリッド表示する主編集領域
   - 左クリックで選択タイルを配置
   - 右クリックでセルをクリア

2. `MapTilesetWindow`
   - プロジェクト内タイルセットの切り替え
   - チップ選択
   - 将来のタイルカテゴリ分割や検索の受け皿

3. `MapInspectorWindow`
   - マップサイズ、タイルサイズ、カーソル位置、選択タイル表示
   - `Toggle Grid` / `Back` の最小操作

---

## マップデータの最小形

`ProjectData.assets.maps[].mapData` は当面、プレーンな JSON 構造に留める。

```js
{
  version: 1,
  width: 24,
  height: 18,
  tileWidth: 16,
  tileHeight: 16,
  tilesetId: 'ts_xxx' | null,
  selectedLayer: 0,
  selectedTileRef: {
    tilesetId: 'ts_xxx',
    col: 0,
    row: 0,
    index: 0,
  } | null,
  cursor: { x: 0, y: 0 },
  view: {
    showGrid: true,
    zoom: 2,
  },
  layers: [{
    id: 'layer_ground',
    name: 'Ground',
    visible: true,
    locked: false,
    tiles: [number, ...],
  }],
}
```

補足:

- `tiles` は一次元配列で、未配置は `-1`
- 初期段階は 1 レイヤー固定で十分
- 将来の複数レイヤー化でも `layers[]` 形式なら拡張しやすい

---

## 実装ステップ

### Phase 1: 導線と保存

- `ProjectTopScene` の `Map Editor` ボタンを有効化
- map asset クリック時は `MapEditorScene` を開く
- アクティブ map が無ければデフォルトの map asset を自動生成する
- `AppData.saveActiveProjectAssetState()` で `map` を保存対象に含める

### Phase 2: UIWindow ベースの雛形

- `MapEditorScene` を追加
- `MapViewWindow` / `MapTilesetWindow` / `MapInspectorWindow` を追加
- scene 側は入力の中継と状態変更だけを担当し、描画詳細は各 window に寄せる

### Phase 3: 最小編集

- tileset から paint tile を選択
- map view 上で左クリック配置、右クリッククリア
- `G` キーとボタンでグリッド表示を切り替える

---

## 今後の拡張候補

1. レイヤーパネル追加
2. マップサイズ変更ダイアログ
3. マップスクロール / 部分表示 / ズーム
4. 複数タイルセット参照の整理
5. Undo / Redo を map 編集にも拡張
6. オートタイルや collision layer の導入

---

## 注意点

- 既存の `EditorScene` は `PixelData` / `LayerData` の編集に特化しているため、
  マップ編集を直接混ぜず別 scene に分離する
- `mapData` は plain object のまま保存できるように保ち、
  先に serializer を複雑化させない
- まずは「開ける・置ける・保存できる」を固め、その後に window を増やす