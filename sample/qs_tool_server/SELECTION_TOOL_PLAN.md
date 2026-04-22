# 選択ツール 実装計画書

最終更新: 2026-04-22

## 1. 目的

`sample/qs_tool_server` に矩形選択を中心とした選択ツールを導入し、
描画ツールから編集ツールへ発展させる。

この機能の目的は次の通り。

1. 範囲単位でコピー、切り取り、貼り付けができるようにする
2. 範囲単位で移動、反転、回転を行えるようにする
3. 将来の複数チップ選択、一括編集、オートタイル支援の土台にする

---

## 2. 現状整理

現在の編集モデルは「ピクセルに直接書く」前提で構成されている。

主な前提:

1. `PixelCanvas` は1枚の `PixelData` を画面に表示し、クリック位置をピクセル座標へ変換する
2. `EditorScene._applyTool()` がアクティブツールに応じて `appData.pixelData` を直接更新する
3. `AppData.layerData` は free モードでは `_layerData`、tileset モードでは選択チップの `LayerData` を返す
4. 表示中の編集対象は常に「現在のアクティブレイヤー」である
5. まだ「選択範囲」という概念はデータモデルに存在しない

つまり、選択ツールの導入では次の3点が必要になる。

1. `AppData` に選択状態を持たせる
2. `PixelCanvas` に選択枠の描画を追加する
3. `EditorScene` に選択操作の入力フローを追加する

---

## 3. 実装方針

### 3.1 最初は矩形選択に絞る

最初の実装では自由形選択はやらず、矩形選択のみ対応する。

理由:

1. ピクセルアート用途では矩形選択の頻度が高い
2. コピー、移動、反転、回転への展開が容易
3. Undo / Redo と組み合わせやすい

### 3.2 選択範囲は `AppData.selection` として保持する

free / tileset モードの両方で使えるように、選択状態は `AppData` に持たせる。

基本形:

```javascript
this.selection = {
  active: false,
  x: 0,
  y: 0,
  w: 0,
  h: 0,
  mode: 'rect',
  floating: null,
};
```

### 3.3 「選択状態」と「浮動選択」は分ける

コピーや切り取り、貼り付け後の移動に備えて、
単なる選択枠と、移動可能な浮動データを分ける。

概念:

1. 通常選択
   - 元画像上の範囲を指すだけ
2. 浮動選択
   - 切り取りや貼り付けで生まれる一時レイヤー的な状態
   - 移動、確定、キャンセルが必要

---

## 4. 新規データモデル

### 4.1 `SelectionState`

```javascript
{
  active: boolean,
  x: number,
  y: number,
  w: number,
  h: number,
  mode: 'rect',
  floating: FloatingSelection | null,
}
```

意味:

1. `active`
   - 選択範囲が存在するか
2. `x, y, w, h`
   - 選択範囲の左上とサイズ
3. `floating`
   - 浮動選択中ならその内容を保持

### 4.2 `FloatingSelection`

```javascript
{
  pixelData: PixelData,
  srcX: number,
  srcY: number,
  dstX: number,
  dstY: number,
  width: number,
  height: number,
  cut: boolean,
}
```

意味:

1. `pixelData`
   - 選択領域のピクセル内容
2. `srcX, srcY`
   - 元位置
3. `dstX, dstY`
   - 現在の配置先
4. `cut`
   - 切り取り由来かどうか

### 4.3 `SelectionClipboard`

```javascript
{
  pixelData: PixelData,
  width: number,
  height: number,
}
```

これは `AppData` に `selectionClipboard` として持たせる。

---

## 5. `AppData` への追加項目

追加候補:

```javascript
this.selection = {
  active: false,
  x: 0,
  y: 0,
  w: 0,
  h: 0,
  mode: 'rect',
  floating: null,
};

this.selectionClipboard = null;
```

補助メソッド候補:

```javascript
clearSelection()
setSelectionRect(x, y, w, h)
hasSelection()
hasFloatingSelection()
```

---

## 6. ツール構成方針

### 6.1 `activeTool` に `selectRect` を追加する

現在のツール:

1. `pencil`
2. `eraser`
3. `fill`
4. `eyedropper`

追加後:

1. `pencil`
2. `eraser`
3. `fill`
4. `eyedropper`
5. `selectRect`

### 6.2 将来の拡張を見越して `tool/` 切り出しと相性の良い形にする

今すぐクラス分離しなくても、`EditorScene._applyTool()` に選択専用の分岐を作るだけではなく、
将来 `tool_select_rect.js` に移しやすい形で整理する。

---

## 7. `PixelCanvas` への変更

### 7.1 選択枠描画

`PixelCanvas.render()` に、編集対象描画後のオーバーレイとして選択枠描画を追加する。

必要な描画:

1. 選択範囲の破線矩形
2. 浮動選択の内容プレビュー
3. 浮動選択の配置先矩形

追加候補メソッド:

```javascript
renderSelection(ctx, appData)
drawSelectionRect(ctx, x, y, w, h)
drawFloatingSelection(ctx, floating)
```

### 7.2 マーチングアリ風アニメーション

初期実装では固定破線でもよいが、最終的にはマーチングアリを入れたい。

簡易方式:

1. `CanvasManager` のフレーム更新を利用
2. `lineDashOffset` をフレームごとにずらす

### 7.3 ヒットテスト補助

今後の移動操作のため、選択範囲内クリックかどうかを判断する補助が必要。

候補:

```javascript
isPointInSelection(px, py, appData)
```

---

## 8. `EditorScene` への組み込み方針

### 8.1 選択開始 / 更新 / 確定のフロー

`PixelCanvas.onPixelDown/onPixelMove/onPixelUp` を利用して矩形選択を作る。

状態候補:

```javascript
this._selectionDrag = null;
this._floatingDrag = null;
```

選択作成フロー:

1. `onPixelDown`
   - `selectRect` ツール時、開始座標を記録
2. `onPixelMove`
   - 現在位置に応じて `AppData.selection` を更新
3. `onPixelUp`
   - 範囲を正規化して確定

### 8.2 浮動選択の移動

貼り付け後や切り取り後に、ドラッグで配置先を変更できるようにする。

フロー:

1. 浮動選択が存在し、範囲内で `mousedown`
2. `dstX/dstY` をドラッグ追従
3. `mouseup` で位置確定
4. 確定操作で元レイヤーへ書き戻し

### 8.3 他ツールとの排他

選択ツール中は `pencil` などの描画を行わない。

ルール:

1. `activeTool === 'selectRect'` のとき `_applyTool()` は描画しない
2. 浮動選択中はツール変更時に確認なしで破棄しない
3. 新規選択開始時に既存の通常選択は置き換える

---

## 9. 実装対象機能の優先順位

### Phase 1: 矩形選択のみ

対象:

1. 矩形選択
2. 選択枠表示
3. 選択解除

この段階ではコピーや移動はまだ入れない。

### Phase 2: コピー / 切り取り / 貼り付け

対象:

1. 選択範囲コピー
2. 選択範囲切り取り
3. 選択範囲貼り付け
4. 浮動選択の表示

### Phase 3: 選択範囲移動

対象:

1. 浮動選択のドラッグ移動
2. 確定
3. キャンセル

### Phase 4: 選択範囲変形

対象:

1. 選択範囲単位の左右反転
2. 選択範囲単位の上下反転
3. 選択範囲単位の90度回転

### Phase 5: 将来拡張

対象:

1. 複数チップ選択
2. 一括編集
3. レイヤーまたぎ選択
4. 選択範囲の別レイヤー貼り付け

---

## 10. 新規クラス / ファイル構成案

```text
www/js/
  selection.js
  selection_clipboard.js
  tool/
    tool_select_rect.js   // 将来切り出し候補
```

最初は新規ファイルを増やしすぎず、次の最小構成でもよい。

1. `app_data.js` に selection 追加
2. `editor_scene.js` に選択ロジック追加
3. `pixel_canvas.js` に選択枠描画追加

ただし、将来の見通しを考えると `selection.js` は早めに切る価値がある。

---

## 11. コピー / 切り取り / 貼り付けの仕様

### 11.1 コピー

処理:

1. アクティブレイヤーから選択範囲を `PixelData` として切り出す
2. `AppData.selectionClipboard` に保存
3. 元画像は変更しない

### 11.2 切り取り

処理:

1. コピーと同じく選択範囲を切り出す
2. 元画像の範囲を透明でクリアする
3. `floating` を生成する
4. Undo / Redo の1コマンドとして扱う

### 11.3 貼り付け

処理:

1. `selectionClipboard` の内容から `floating` を生成
2. 初期位置は元位置、または現在の選択左上
3. ドラッグで移動可能
4. 確定でアクティブレイヤーへ書き込む

### 11.4 確定とキャンセル

確定:

1. `floating.pixelData` を現在位置へ転写
2. 通常選択へ戻すか、選択解除する

キャンセル:

1. `floating` を破棄
2. 切り取り由来なら undo で戻せる前提にする

---

## 12. `MenuBar` とショートカットへの反映

既存メニュー定数はすでにあるため、未実装項目を接続する。

対象:

1. `EDIT_CUT`
2. `EDIT_COPY`
3. `EDIT_PASTE`
4. `EDIT_SELECT_ALL`

追加候補ショートカット:

1. `M` または `S` → 矩形選択ツール
2. `Ctrl+C` → コピー
3. `Ctrl+X` → 切り取り
4. `Ctrl+V` → 貼り付け
5. `Ctrl+A` → 全選択
6. `Escape` → 選択解除 / 浮動選択キャンセル

注意点:

1. ダイアログ編集中はショートカットを横取りしない
2. レイヤー名編集中はテキスト入力優先

---

## 13. tileset モードでの扱い

最初の実装では、tileset モードでも「選択中チップ内のみ」の矩形選択とする。

理由:

1. 現在の `appData.layerData` は選択チップの `LayerData` を返す設計だから自然
2. `PixelCanvas` も常に1チップを表示している
3. 複数チップ選択は別フェーズで追加できる

この方針なら free モードと tileset モードで同じ選択ロジックを使える。

将来拡張として、チップパレット上での複数チップ選択は別機能として扱う。

---

## 14. Undo / Redo との関係

選択ツールは Undo / Redo と密接に関係する。

履歴対象:

1. 切り取り
2. 貼り付け確定
3. 選択範囲変形
4. 選択範囲移動確定

履歴対象外:

1. 選択枠を引くこと自体
2. コピーのみ
3. 浮動選択中の一時移動

つまり「画像内容を変える操作だけ履歴化」する。

---

## 15. 実装フェーズ詳細

### Step 1: 選択状態の追加

作業内容:

1. `AppData.selection` 追加
2. `AppData.selectionClipboard` 追加
3. `activeTool` に `selectRect` を追加

完了条件:

1. 状態だけ保持できる

### Step 2: 矩形選択の入力処理

作業内容:

1. `EditorScene` に選択ドラッグ処理追加
2. 選択開始、更新、確定の実装
3. 右ドラッグやパン操作との競合調整

完了条件:

1. 矩形選択が作れる

### Step 3: 選択枠描画

作業内容:

1. `PixelCanvas` に選択オーバーレイ描画追加
2. 通常選択矩形の表示
3. マーチングアリ風の見た目調整

完了条件:

1. 選択範囲が視覚的に確認できる

### Step 4: コピー / 切り取り / 貼り付け

作業内容:

1. 選択範囲の切り出し処理
2. クリップボード保存
3. 切り取り実装
4. 貼り付け時の浮動選択生成

完了条件:

1. `Ctrl+C / Ctrl+X / Ctrl+V` が動く

### Step 5: 浮動選択移動

作業内容:

1. 浮動選択ドラッグ移動
2. 確定
3. キャンセル

完了条件:

1. 貼り付けた内容を動かして置ける

### Step 6: 選択範囲変形

作業内容:

1. 選択範囲単位の反転
2. 選択範囲単位の回転
3. メニュー接続

完了条件:

1. 選択中だけ変形対象になる

---

## 16. テスト観点

### 16.1 手動確認項目

1. free モードで矩形選択が作れる
2. tileset モードで選択中チップ内の矩形選択が作れる
3. 選択範囲が正しい座標で描画される
4. コピー後に内容が壊れない
5. 切り取り後に元範囲が透明化される
6. 貼り付け後に浮動選択として表示される
7. 浮動選択をドラッグ移動できる
8. Escape で解除またはキャンセルできる
9. Select All が現在のレイヤー全面を選択する
10. Undo / Redo 実装後に画像変化だけが履歴対象になる

### 16.2 境界条件

1. 1x1 選択
2. 逆方向ドラッグ選択
3. キャンバス端までの選択
4. 透明だけの領域コピー
5. 貼り付け時に一部がキャンバス外へ出るケース

---

## 17. 想定リスク

### 17.1 浮動選択の状態管理が複雑になる

対策:

1. 通常選択と浮動選択を分離する
2. まずは「1個の浮動選択のみ対応」に限定する

### 17.2 描画ツールとの競合

対策:

1. `activeTool` で排他する
2. `space + drag` のパンを最優先する

### 17.3 Undo / Redo 未実装時の切り取りが危険

対策:

1. 切り取りは Undo / Redo と同時導入が望ましい
2. 先に導入する場合は確認なし破壊操作を避ける

---

## 18. 最初の実装範囲の提案

最初のPRでは次までに絞る。

対象:

1. `AppData.selection`
2. `selectRect` ツール追加
3. 矩形選択入力処理
4. 選択枠描画
5. `Escape` による選択解除
6. `Ctrl+A` による全選択

後回し:

1. コピー、切り取り、貼り付け
2. 浮動選択
3. 選択範囲変形
4. 複数チップ選択

理由:

1. まず選択の基礎モデルを壊さず導入できる
2. UI と入力の破綻を先に潰せる
3. Undo / Redo と接続しやすい形を先に固められる

---

## 19. 完了イメージ

選択ツール完了後の最低ラインは次の状態。

1. 矩形選択が自然に作れる
2. 選択中であることが視覚的にわかる
3. 範囲コピー、切り取り、貼り付けができる
4. 貼り付け後の位置調整ができる
5. free / tileset の両モードで同じ操作感になる

この機能が入ると、単なる描画から編集へ進めるようになり、
その後の一括編集や量産補助機能にもつながる。