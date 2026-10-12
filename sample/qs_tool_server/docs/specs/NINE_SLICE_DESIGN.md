# 9スライス設計

- 状態: 実装済み（差分は末尾の「実装時の差分」を参照）
- 関連: [MESSAGE_WINDOW_DESIGN.md](MESSAGE_WINDOW_DESIGN.md)（メッセージウィンドウの枠として使う）、[PLAY_UNIT_DESIGN.md](PLAY_UNIT_DESIGN.md)（UICanvas / UITransform）

## 1. 目的

小さな画像から、任意サイズの枠を作る。四隅は原寸のまま、上下左右の辺は一方向に、中央は両方向に伸ばす。メッセージウィンドウ、選択肢の背景、ボタン、HP バーの枠などに同じ仕組みを使う。

## 2. 決定事項（推奨）

### D1. 9 スライスの定義は pixelDocument の任意項目にする

独立したアセット種別（`nineSlice`）は作らない。

- 理由: 保存・読込（.qsproj / ブラウザ保存）、Save As、プロジェクト一覧、EditorScene の開閉がそのまま使える。9 スライス用の画像は通常の画像としても使える。
- データ: `pixelDocument.nineSlice = { left, top, right, bottom }`。単位はソース画素。未設定なら通常の画像として扱う。
- 正規化: 各値は 0 以上の整数。`left + right <= width`、`top + bottom <= height` を満たさないものは無効（`nineSlice` を持たない扱い）。
- 対象箇所: [project_data.js](../../www/js/project/project_data.js) `_normalizePixelDocument`（L371）、[project_session.js](../../www/js/project/project_session.js) の pixelDocument の serialize / deserialize、[project_browser_storage.js](../../www/js/project/project_browser_storage.js) の pixelDocument の保存・読込分岐。

### D2. 作成画面は EditorScene の pixelDocument 編集に 9 スライスガイドを重ねる

新しい editMode は作らない（`editMode` は `'free'` のまま）。

- 表示メニューに「9スライスガイド」のトグルを置く（表示状態のみ。履歴には載せない）。
- ガイド線を 4 本（左・上・右・下）描き、マウスでドラッグして位置を変える。ドラッグ中は数値を表示する。
- 数値で設定するダイアログ「9スライス設定」を用意する（L / T / R / B の 4 数値）。
- プレビュー窓を用意する。指定サイズ（既定は 3 倍相当の 96×64）に伸ばした結果を表示し、サイズはダイアログで変えられる。
- insets の保存は既存の pixelDocument 保存の経路に乗せる。`AppData` に現在の insets を持ち、`saveActiveProjectAssetState` で `pixelDocument.nineSlice` へ書き戻す。
- 既存の pixelDocument と違い、insets の変更は Undo に載せない（MVP）。

### D3. 描画は共通モジュールに 1 本化する

`js/nine_slice/nine_slice_renderer.js`（新規、`index.html` で `play_test_scene.js` より前）に `NineSliceRenderer` を作る。EditorScene のプレビューと PlayTest の両方が使う。

```
NineSliceRenderer.draw(ctx, sourceCanvas, insets, dest, scale)
  dest  = { x, y, w, h }   描画先（世界座標または UI の矩形）
  scale = 整数倍率（ドット絵の見た目を揃える。角と辺の厚みに掛ける）
```

- 角は `inset * scale` の原寸。`dest` が小さく角の合計が `w` / `h` を超えるときは、左右（上下）の角を比率で縮める。
- 辺と中央は `drawImage` の 9 引数版でソース矩形を伸ばす。
- `imageSmoothingEnabled = false`。
- `insets` が無い場合は全体を `dest` に伸ばす（既存の Image と同じ扱い）。

### D4. PlayUnit コンポーネント `NineSlice` を追加する

```json
{
  "type": "NineSlice",
  "enabled": true,
  "data": { "pixelDocumentId": "px_frame", "scale": 1, "width": 0, "height": 0, "originX": 0, "originY": 0 }
}
```

- ワールドのオブジェクト: `Transform` の x / y を基準にし、`width` / `height` が 0 なら「画像の原寸 × scale」を使う。
- UICanvas 配下のオブジェクト: `UITransform` の矩形を使う。`width` / `height` は無視する（UI のサイズは UITransform に一本化する）。
- 既存の `Image` コンポーネントは変更しない。insets を `Image` で使いたいときも、`NineSlice` を使う（役割を分けて重複を避ける）。
- 対象箇所: [play_unit_runtime.js](../../www/js/play_unit/play_unit_runtime.js) の `fromPlayUnit`（ワールド）と `_appendUIRenderableEntries`（UI、kind は `nineSlice`）。描画は [play_test_scene.js](../../www/js/scene/play_test_scene.js) の `render` の kind 分岐と `_drawUIEntries`。
- PlayUnit エディタ: `+NineSlice` ボタン（pixelDocument 選択は既存の `_promptPixelDocumentSelection` を流用）。`_createComponentTemplate` に `NineSlice` の既定値を追加。

### D5. Save As 時の参照を書き換える

`NineSlice.pixelDocumentId` は Save As で新しい `px` ID へ書き換える。`_rewriteCharacterChipsetRefs` と同じ形の関数を作る。

- 補足: `Image` の `pixelDocumentId` と Tilemap の `mapAssetId` は、今も Save As の書き換え対象外である（既存の欠陥）。今回は範囲に含めず、別件として残す。

## 3. 変更箇所（一覧）

| 領域 | 変更 |
|---|---|
| データ | `_normalizePixelDocument` に `nineSlice` の正規化を追加。serialize / deserialize、ブラウザ保存の pixelDocument 分岐に `nineSlice` を通す |
| 描画 | 新規 `nine_slice_renderer.js`。`play_test_scene.js` の描画分岐に `nineSlice` を追加 |
| ランタイム | `play_unit_runtime.js` に `NineSlice` の解析（ワールド / UI）を追加 |
| エディタ（PlayUnit） | `+NineSlice` ボタンと既定テンプレート |
| エディタ（ピクセル） | 9 スライスガイド、ガイドのドラッグ、9 スライス設定ダイアログ、プレビュー窓、`saveActiveProjectAssetState` への書き戻し |
| 保存 | `_createSaveAsCopy` で `NineSlice.pixelDocumentId` を書き換え |

## 4. 実装フェーズ

1. データ層: `nineSlice` の正規化、.qsproj とブラウザ保存の往復。
2. 描画: `NineSliceRenderer`、PlayTest のワールドと UI の両方。PlayUnit エディタの `+NineSlice`。
3. EditorScene: ガイド表示とドラッグ、設定ダイアログ、プレビュー窓、保存への反映。
4. Save As の参照書き換え、検証、ドキュメント。

## 5. 検証（完了条件）

- 不正な insets（負値、非整数、合計超過）が無効になる。有効な値は .qsproj とブラウザ保存で残る。
- 3×3 の既知パターン（領域ごとに色を変えた画像）で、9 領域の描画位置を `getImageData` で確認する。
- UICanvas 配下で、UITransform の矩形いっぱいに枠が伸びる。角が縮むサイズでも崩れない。
- EditorScene で insets を変えて保存し、再読込で同じ位置にガイドが出る。
- Save As 後、PlayUnit の `NineSlice` が新しい `px` ID を参照する。
- PlayTest で PlayUnit 切替をしてもアセットの insets は変わらない。

## 6. 範囲外（今回は作らない）

- 9 スライス以外の伸縮方式（3 スライス、タイル繰り返しなど）。
- 角の縮小時の見た目の自動調整（プレビューで確認する）。
- insets の Undo。
- 9 スライス専用の独立アセット一覧。

## 7. 実装時の差分

- **境界の保持**: 編集中の境界は AppData に複製せず、アセット本体（`asset.nineSlice`）を正とする。`AppData.getActiveNineSlice()` / `setActiveNineSlice()` で読み書きし、変更時に `touch()` と `markDirty()` を呼ぶ。寸法が変わって収まらない境界は、保存時と読込時に無効（null）になる。
- **メニュー**: 表示メニューに「9スライスガイド表示」（チェック付き。`DropdownMenu` の `isChecked` で判定）と「9スライス設定…」を置いた。編集メニューには置いていない。
- **NineSlice のデータ**: `alpha` を追加した。`scale` は Tilemap と同じ整数 1〜8（`PlayUnitRuntime._normalizeScale`）。
- **プレビュー**: 「9-slice Preview」窓は固定の表示領域（256×160）で、描画先の幅・高さを 8 刻みで変える（W 16〜256、H 16〜160）。
- **ガイドの初期値**: 境界の無い画像でガイドを表示すると、短い辺の 1/4 を四辺に入れて保存する。
- **既存の制約（変えていない）**: UICanvas 配下のオブジェクトに Transform があると、ワールド描画の対象にもなる。UI の部品には Transform を付けない。
- **デモ**: `tools/add_message_window_to_demo.py` が枠の pixelDocument を作る（[MESSAGE_WINDOW_DESIGN.md](MESSAGE_WINDOW_DESIGN.md)）。

