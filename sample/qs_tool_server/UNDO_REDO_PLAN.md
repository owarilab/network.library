# Undo / Redo 実装計画書

最終更新: 2026-04-22

## 1. 目的

`sample/qs_tool_server` のドットエディタ / タイルセットエディタに
実用的な Undo / Redo を導入する。

この機能の主目的は次の3点。

1. 描画ミスを安全に取り消せるようにする
2. タイルセット編集時の破壊的操作を戻せるようにする
3. 今後の選択ツール、一括編集、オートタイル支援の基盤にする

---

## 2. 現状整理

現状の編集は `EditorScene` から直接データへ書き込んでいる。

主な更新経路:

1. ピクセル描画
   - `EditorScene._applyTool()` から `appData.pixelData.setPixel()` を直接呼ぶ
2. 塗りつぶし
   - `EditorScene._floodFill()` が `PixelData` を直接更新する
3. レイヤー操作
   - `LayerPanel` から `LayerData.addLayer()/removeLayer()/moveLayer()/toggleVisibility()` を直接呼ぶ
4. 画像変形
   - `PixelData.flipH()/flipV()/rotate90CW()/rotate90CCW()` を直接呼ぶ
5. タイルセット操作
   - `TilesetData.clearChip()/swapChips()/pasteChipLayerData()/addRow()/addColumn()/removeRow()/removeColumn()` を直接呼ぶ
6. 通過フラグ
   - `tilesetData.passFlags[row][col]` を直接トグルしている

このため、Undo / Redo を入れるには、
「直接更新」を「コマンド経由更新」に段階的に置き換える必要がある。

---

## 3. 基本方針

### 3.1 コマンドパターンを採用する

すべての編集操作をコマンド化し、履歴スタックへ積む。

各コマンドは最低限次のインターフェースを持つ。

```javascript
class CommandBase {
  execute(appData) {}
  undo(appData) {}
  redo(appData) { this.execute(appData); }
  getLabel() { return 'command'; }
}
```

### 3.2 履歴管理は `HistoryManager` に集約する

履歴の push / undo / redo は `HistoryManager` に集中させる。

責務:

1. Undo スタック管理
2. Redo スタック管理
3. 最大履歴数制限
4. 実行中フラグ管理
5. UI への状態通知

### 3.3 描画系は「1ストローク = 1コマンド」にまとめる

ペンシルや消しゴムは、mousemove ごとに1履歴にすると使いづらい。
mousedown から mouseup までを1コマンドとして扱う。

このため、描画系には「ストローク記録バッファ」が必要。

---

## 4. 実装対象の優先順位

### Phase 1: 最低限の Undo / Redo

最初に対応する操作:

1. ペンシル
2. 消しゴム
3. 塗りつぶし
4. 左右反転
5. 上下反転
6. 90度回転
7. 新規作成

この段階で、日常編集の大半は戻せる状態にする。

### Phase 2: レイヤー操作

次に対応する操作:

1. レイヤー追加
2. レイヤー削除
3. レイヤー順変更
4. レイヤー表示/非表示
5. レイヤー名変更
6. レイヤー不透明度変更

### Phase 3: タイルセット操作

次に対応する操作:

1. チップコピー / 貼り付け
2. チップクリア
3. チップ入れ替え
4. 行追加 / 列追加
5. 行削除 / 列削除
6. 通過フラグ変更
7. タイルセット新規作成 / インポート

### Phase 4: 将来拡張

将来の対象:

1. 選択範囲移動
2. 一括編集
3. オートタイル生成
4. 斜め見下ろし生成
5. パレット編集

---

## 5. 新規クラス構成

追加候補ディレクトリ:

```text
www/js/
  history_manager.js
  command/
    command_base.js
    pixel_stroke_command.js
    flood_fill_command.js
    transform_command.js
    new_file_command.js
    layer_command.js
    tileset_command.js
```

### 5.1 `HistoryManager`

```javascript
class HistoryManager {
  constructor(limit = 100)
  execute(command, appData)
  undo(appData)
  redo(appData)
  clear()
  canUndo()
  canRedo()
  getUndoLabel()
  getRedoLabel()
}
```

内部状態:

1. `_undoStack = []`
2. `_redoStack = []`
3. `_limit = 100`
4. `_isApplying = false`

### 5.2 `CommandBase`

すべてのコマンドの基底。

```javascript
class CommandBase {
  execute(appData) {}
  undo(appData) {}
  redo(appData) { this.execute(appData); }
  getLabel() { return 'command'; }
}
```

### 5.3 `PixelStrokeCommand`

ペンシル / 消しゴム用。

保持するもの:

1. 対象モード (`free` / `tileset`)
2. 対象チップ座標
3. 対象レイヤーインデックス
4. 変更ピクセル一覧

変更ピクセルの1件:

```javascript
{ x, y, before, after }
```

注意点:

1. 同じピクセルを1ストローク中に複数回通っても `before` は最初の1回だけ保持する
2. `after` は最後の色で上書きする

### 5.4 `FloodFillCommand`

塗りつぶし用。

内部的には `PixelStrokeCommand` と同じ変更集合でよい。
違いは変更集合の生成方法だけ。

### 5.5 `TransformCommand`

反転 / 回転用。

対象レイヤーごとのピクセル配列スナップショットを保存する。

保持候補:

```javascript
{
  mode,
  chip,
  layerSnapshotsBefore: [Uint32Array],
  layerSnapshotsAfter:  [Uint32Array],
  widthBefore,
  heightBefore,
  widthAfter,
  heightAfter,
  kind: 'flipH' | 'flipV' | 'rotateCW' | 'rotateCCW'
}
```

### 5.6 `NewFileCommand`

新規作成や新規タイルセット作成用。

保持候補:

1. `AppData` 全体の編集対象スナップショット before
2. after

これは少し重いが、作成頻度は低いため許容できる。

### 5.7 `LayerCommand` 系

種類:

1. `AddLayerCommand`
2. `RemoveLayerCommand`
3. `MoveLayerCommand`
4. `ToggleLayerVisibilityCommand`
5. `RenameLayerCommand`
6. `SetLayerOpacityCommand`

### 5.8 `TilesetCommand` 系

種類:

1. `PasteChipCommand`
2. `ClearChipCommand`
3. `SwapChipsCommand`
4. `ResizeTilesetCommand`
5. `SetPassFlagCommand`

---

## 6. データ保存戦略

Undo / Redo では「差分保存」と「スナップショット保存」を使い分ける。

### 6.1 差分保存を使う操作

対象:

1. ペンシル
2. 消しゴム
3. 塗りつぶし
4. 通過フラグ切替
5. レイヤー名変更
6. 不透明度変更

利点:

1. メモリ効率が良い
2. Undo / Redo が高速

### 6.2 スナップショット保存を使う操作

対象:

1. 反転
2. 回転
3. レイヤー削除
4. チップ貼り付け
5. チップ入れ替え
6. タイルセットの行列変更
7. 新規作成 / インポート

利点:

1. 実装が単純
2. 複雑な構造変更でも安全

### 6.3 スナップショット対象の最小単位

重すぎる `AppData` 全体保存は避け、基本は次の単位で保存する。

1. 1レイヤーの `PixelData`
2. 1チップの `LayerData`
3. タイルセット構造変更時のみ `TilesetData` 部分

---

## 7. `AppData` への追加項目

`AppData` に次を追加する。

```javascript
this.history = new HistoryManager(100);
```

加えて、履歴対象のコンテキストを取得しやすくするため、補助メソッドを追加するとよい。

候補:

```javascript
getActiveEditTargetContext() {
  return {
    mode: this.editMode,
    chip: this.editMode === 'tileset' ? { ...this.selectedChip } : null,
    layerIndex: this.layerData.activeIndex,
  };
}
```

---

## 8. `EditorScene` への組み込み方針

### 8.1 描画ストロークの開始 / 終了を管理する

`EditorScene` に編集中ストロークを持たせる。

追加候補:

```javascript
this._currentStroke = null;
```

フロー:

1. `onPixelDown`
   - `PixelStrokeBuilder` を開始
2. `onPixelMove`
   - 変更ピクセルを追加
3. `onPixelUp`
   - 変更があれば `PixelStrokeCommand` 化して `history.execute()`

### 8.2 `_applyTool()` は直接書き込みをやめる

現在はここで直接 `setPixel()` しているが、
Undo 導入後は「変更記録付き適用」へ変える。

方針:

1. `setPixelWithRecord(x, y, color, strokeBuilder)` を導入
2. 直接 `setPixel()` を呼ばず、必ず記録経由で更新する

### 8.3 `_floodFill()` は変更一覧を返す形にする

現在は内部で直接変更している。
これを次のどちらかへ変更する。

1. 差分一覧を返す
2. `strokeBuilder` に変更を記録する

おすすめは 2。

---

## 9. `LayerPanel` への組み込み方針

現在は `LayerPanel.onMouseDown()` から `LayerData` を直接更新している。

これを次のように変更する。

1. `appData.history.execute(new AddLayerCommand(...), appData)`
2. `appData.history.execute(new RemoveLayerCommand(...), appData)`
3. `appData.history.execute(new MoveLayerCommand(...), appData)`
4. `appData.history.execute(new ToggleLayerVisibilityCommand(...), appData)`

インライン編集によるレイヤー名変更も確定時にコマンド化する。

---

## 10. タイルセット操作への組み込み方針

`EditorScene` のメニュー処理内にあるタイルセット操作もコマンド化する。

対象:

1. `TILESET_PASTE_CHIP`
2. `TILESET_CLEAR_CHIP`
3. `TILESET_SWAP_CHIP`
4. `TILESET_ADD_ROW`
5. `TILESET_ADD_COL`
6. `TILESET_REMOVE_ROW`
7. `TILESET_REMOVE_COL`
8. チップ右クリックによる通過フラグ変更

特に `add/remove row/col` は選択チップ補正も undo 対象に含める必要がある。

---

## 11. メニューとショートカット

### 11.1 メニュー接続

`MenuConstants.EDIT_UNDO` / `MenuConstants.EDIT_REDO` を `HistoryManager` に接続する。

```javascript
if (id === MenuConstants.EDIT_UNDO) {
  appData.history.undo(appData);
  this._pixelCanvas.markDirty();
  return;
}
if (id === MenuConstants.EDIT_REDO) {
  appData.history.redo(appData);
  this._pixelCanvas.markDirty();
  return;
}
```

### 11.2 キーボードショートカット

対応候補:

1. `Ctrl+Z` → undo
2. `Ctrl+Shift+Z` → redo
3. `Ctrl+Y` → redo

注意点:

1. ダイアログ編集中はショートカットを横取りしない
2. LayerPanel の名前編集中はテキスト入力を優先する

---

## 12. 実装フェーズ詳細

### Step 1: 土台作成

作業内容:

1. `history_manager.js` 追加
2. `command_base.js` 追加
3. `AppData.history` 追加
4. `index.html` のスクリプト読み込み順に追加

完了条件:

1. 空のコマンドで `undo/redo` が呼べる

### Step 2: 描画ストローク対応

作業内容:

1. `PixelStrokeCommand` 追加
2. `EditorScene` にストローク開始/終了処理追加
3. ペンシルと消しゴムを履歴対応

完了条件:

1. 1回のドラッグ描画が1回で undo できる

### Step 3: 塗りつぶし対応

作業内容:

1. `FloodFillCommand` 追加
2. `_floodFill()` を記録可能な形へ変更

完了条件:

1. 塗りつぶし1回が1回で undo できる

### Step 4: 反転 / 回転 / 新規作成

作業内容:

1. `TransformCommand` 追加
2. `NewFileCommand` 追加
3. メニュー処理をコマンド経由へ変更

完了条件:

1. 反転 / 回転 / 新規作成の undo が可能

### Step 5: レイヤー操作

作業内容:

1. LayerCommand 群追加
2. `LayerPanel` を履歴対応

完了条件:

1. レイヤー追加 / 削除 / 並び替え / 可視切替が戻せる

### Step 6: タイルセット操作

作業内容:

1. TilesetCommand 群追加
2. 通過フラグトグルを履歴対応
3. 行列追加削除を履歴対応

完了条件:

1. タイルセット編集の主要操作がすべて戻せる

---

## 13. テスト観点

### 13.1 単体テスト相当で確認したいこと

1. `undo()` 後に before 状態へ戻る
2. `redo()` 後に after 状態へ戻る
3. 連続 `undo/redo` で破綻しない
4. 履歴上限を超えたとき古い履歴が捨てられる

### 13.2 手動確認項目

1. ペンシルを長く引いて1回で取り消せる
2. 消しゴムストロークが1回で戻せる
3. 塗りつぶしが戻せる
4. 反転 / 回転が戻せる
5. レイヤー追加 / 削除 / 並び順変更が戻せる
6. チップ貼り付け / 入れ替え / クリアが戻せる
7. 行追加 / 列追加 / 行削除 / 列削除が戻せる
8. 通過フラグ変更が戻せる
9. free モードと tileset モードで同じ履歴基盤が動く

---

## 14. 想定リスク

### 14.1 メモリ使用量

リスク:

1. 大きいキャンバス
2. レイヤー多数
3. タイルセット全体スナップショット

対策:

1. 差分保存を優先
2. スナップショット対象を最小化
3. 履歴数上限を設ける

### 14.2 既存コードの直接更新経路が残る

リスク:

1. 一部だけ undo 対応
2. 戻らない操作が混在

対策:

1. 直接更新箇所を grep で洗い出す
2. 「編集操作は必ず command 経由」というルールを決める

### 14.3 タイルセット構造変更時の参照ずれ

リスク:

1. 行列削除で `selectedChip` が範囲外になる
2. undo 時に復元漏れが出る

対策:

1. コマンドに選択位置も含めて保存
2. execute / undo で必ず補正する

---

## 15. 最初の実装範囲の提案

最初のPRでは範囲を絞る。

対象:

1. `HistoryManager`
2. `CommandBase`
3. `PixelStrokeCommand`
4. `FloodFillCommand`
5. `TransformCommand`
6. `EditorScene` の `EDIT_UNDO / EDIT_REDO`
7. `Ctrl+Z / Ctrl+Shift+Z / Ctrl+Y`

この段階では次を後回しにする。

1. レイヤー操作
2. タイルセット構造変更
3. 通過フラグ
4. パレット編集

理由:

1. まず日常編集の大部分を戻せるようにする
2. 既存コードへの影響範囲を抑える
3. コマンド基盤の妥当性を先に検証できる

---

## 16. 完了イメージ

Undo / Redo 完了後の最低ラインは次の状態。

1. ペンシル、消しゴム、塗りつぶしが自然に戻せる
2. 反転、回転、新規作成が戻せる
3. メニューとショートカットの両方で操作できる
4. free モード / tileset モードの両方で破綻しない
5. 今後の選択ツール、一括編集の基盤として再利用できる

この機能は単体でも価値が高いが、今後のすべての編集強化の基礎になる。