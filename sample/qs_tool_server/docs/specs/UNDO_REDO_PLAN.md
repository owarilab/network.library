# Undo / Redo 実装計画書

最終更新: 2026-04-24

## 1. 目的

`sample/qs_tool_server` のドットエディタ / タイルセットエディタに
実用的な Undo / Redo を導入する。

この機能の主目的は次の3点。

1. 描画ミスを安全に取り消せるようにする
2. タイルセット編集時の破壊的操作を戻せるようにする
3. 今後の選択ツール、一括編集、オートタイル支援の基盤にする

---

## 2. 現状整理

現状は、Phase 1 の主要機能に加えて、Phase 2 の LayerPanel 経路を実装済みである。

実装済み:

1. `HistoryManager` / `CommandBase`
2. `AppData.history` と履歴関連スクリプトの読み込み順
3. メニュー `EDIT_UNDO / EDIT_REDO`
4. `Ctrl+Z` / `Ctrl+Shift+Z` / `Ctrl+Y`
5. ペンシル / 消しゴムの 1 ストローク履歴化
6. 塗りつぶしの履歴化
7. 通常レイヤーへの反転 / 回転の履歴化
8. 通常新規作成の履歴化

確認済み:

1. `pencil / eraser / fill` の Undo / Redo
2. `flip / rotate` の Undo / Redo
3. `new file` の Undo / Redo
4. `LayerPanel` 経由の `add/remove/move/visible/lock/rename/duplicate/merge` の Undo / Redo
5. `LayerPanel` の opacity スライダー変更の Undo / Redo

未対応:

1. タイルセット新規作成
2. タイルセット操作
3. 通過フラグ
4. 浮動選択系の履歴統合

まだ直接更新が残っている経路:

1. `TilesetData.clearChip()/swapChips()/pasteChipLayerData()/addRow()/addColumn()/removeRow()/removeColumn()`
2. `tilesetData.passFlags[row][col]` の直接トグル
3. 浮動選択の持ち上げ / 移動 / 確定 / キャンセル / 変形

したがって、以後の作業は「残っている直接更新経路をコマンド経由へ置き換える」ことに集中すればよい。

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
5. レイヤーロック切替
6. レイヤー名変更
7. レイヤー不透明度変更
8. レイヤー複製
9. レイヤー結合

確認状況:

1. LayerPanel 経由の各操作と opacity スライダーの Undo / Redo は確認済み
2. Phase 2 の実装対象は完了

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

1. 浮動選択の履歴統合
2. 選択範囲移動
3. 一括編集
4. オートタイル生成
5. 斜め見下ろし生成
6. パレット編集

補足:

1. 矩形選択、コピー、切り取り、貼り付け、浮動選択の移動と変形はすでに実装済み
2. ただし Undo / Redo の観点ではまだ未整理で、直接更新経路が残っている
3. 初回実装ではこの範囲を後回しにする方が安全

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
5. `ToggleLayerLockedCommand`
6. `RenameLayerCommand`
7. `SetLayerOpacityCommand`
8. `DuplicateLayerCommand`
9. `MergeLayerDownCommand`

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

注意:

1. 現在の `EditorScene` には選択ツール、浮動選択の確定、変形処理も入っている
2. 初回導入ではまず通常描画系のみを履歴対応し、浮動選択は現状維持にする
3. 浮動選択を同時に扱うと、描画コマンドと選択コマンドの境界が曖昧になりやすい

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
5. `appData.history.execute(new ToggleLayerLockedCommand(...), appData)`
6. `appData.history.execute(new DuplicateLayerCommand(...), appData)`
7. `appData.history.execute(new MergeLayerDownCommand(...), appData)`

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

現状:

1. `Ctrl+A / Ctrl+C / Ctrl+X / Ctrl+V` は実装済み
2. Undo / Redo 系のみ未実装

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
2. 浮動選択への反転 / 回転はこの段階では対象外でもよい

### Phase 1 の具体的な実装順

Step 1〜4 を、そのまま作業へ落とせる粒度に分解すると次の順になる。

#### Task 1: 履歴基盤ファイルを追加する

状態:

1. 完了

対象ファイル:

1. `www/js/history_manager.js`
2. `www/js/command/command_base.js`

作業内容:

1. `HistoryManager.execute()/undo()/redo()/clear()` の最小実装を作る
2. `canUndo()/canRedo()/getUndoLabel()/getRedoLabel()` を入れる
3. `_isApplying` を持たせ、undo/redo 中の二重 push を防ぐ土台を作る

完了条件:

1. ダミーコマンドで `execute → undo → redo` が成立する
2. まだ実編集コマンドがなくても API が固定できる

#### Task 2: `AppData` と読み込み順へ履歴基盤を接続する

状態:

1. 完了

対象ファイル:

1. `www/js/app_data.js`
2. `www/index.html`

作業内容:

1. `AppData` に `history = new HistoryManager(100)` を追加する
2. 必要ならアクティブ対象取得ヘルパーを追加する
3. `index.html` に履歴関連スクリプトを依存順で追加する

完了条件:

1. `EditorScene` から `appData.history` を参照できる
2. ブラウザ起動時に読み込みエラーが出ない

#### Task 3: Undo / Redo の配線だけ先に通す

状態:

1. 完了
2. 空履歴での基本挙動は確認済み

対象ファイル:

1. `www/js/scene/editor_scene.js`

作業内容:

1. メニュー `EDIT_UNDO / EDIT_REDO` を `appData.history` に接続する
2. `Ctrl+Z / Ctrl+Shift+Z / Ctrl+Y` を追加する
3. 実コマンド未導入でも空履歴で安全に動くようにする

完了条件:

1. UI 経路は先に固定できる
2. 空履歴で押しても壊れない

#### Task 4: 描画ストロークの記録器を入れる

状態:

1. 完了
2. ペンシル / 消しゴムの 1 ストローク 1 Undo は実装済み

対象ファイル:

1. `www/js/command/pixel_stroke_command.js`
2. `www/js/scene/editor_scene.js`

作業内容:

1. 1ストローク中の変更ピクセルを集める builder 相当の内部構造を決める
2. 同一ピクセルの `before` は最初の1回だけ保持する
3. `onPixelDown/onPixelMove/onPixelUp` の流れでコマンド確定する
4. まずペンシルと消しゴムだけを履歴対応する

完了条件:

1. 長いドラッグ描画が1回の undo で戻る
2. free / tileset の通常描画で同じ処理が使える

#### Task 5: 塗りつぶしを差分記録へ寄せる

状態:

1. 完了
2. pencil / eraser / fill の Undo / Redo はブラウザ上で挙動確認済み

対象ファイル:

1. `www/js/command/flood_fill_command.js`
2. `www/js/scene/editor_scene.js`

作業内容:

1. `_floodFill()` を「直接変更だけする関数」から「変更集合を記録できる関数」へ変える
2. 実装は `PixelStrokeCommand` と変更集合を共有できる形に寄せる
3. fill の mousedown 1回を 1 コマンドにする

完了条件:

1. 塗りつぶし1回が1回で戻る
2. 既存の塗りつぶし結果と視覚的な挙動が変わらない

#### Task 6: 変形操作をスナップショットで包む

状態:

1. 完了
2. 通常レイヤー変形の Undo / Redo はブラウザ上で挙動確認済み

対象ファイル:

1. `www/js/command/transform_command.js`
2. `www/js/scene/editor_scene.js`

作業内容:

1. 左右反転、上下反転、時計回り回転、反時計回り回転を `TransformCommand` 化する
2. 初回は通常レイヤー変形のみ対象とし、浮動選択変形は対象外のままにする
3. 変形前後の対象レイヤースナップショットを保存する

完了条件:

1. 変形系メニューが undo/redo 可能になる
2. 対象外の浮動選択変形と混線しない

#### Task 7: 新規作成をコマンド化する

状態:

1. 完了
2. 通常新規作成の Undo / Redo はブラウザ上で挙動確認済み

対象ファイル:

1. `www/js/command/new_file_command.js`
2. `www/js/scene/editor_scene.js`

作業内容:

1. 通常新規作成を `NewFileCommand` 化する
2. 必要な before/after を最小スナップショットで保存する
3. タイルセット新規作成は Phase 1 で入れるか、Phase 3 へ寄せるかを実装時に明示する

完了条件:

1. 新規作成直後に元へ戻せる
2. ビューリセットや選択解除も undo/redo 後に破綻しない

#### Task 8: Phase 1 の手動確認を固定する

確認順:

1. ペンシルを長く描く
2. Undo / Redo をメニューで確認する
3. Undo / Redo をショートカットで確認する
4. 消しゴム、塗りつぶし、反転、回転、新規作成を同じ順で確認する
5. free モード確認後、tileset モードの通常描画でも同じ確認を行う

完了条件:

1. Phase 1 対象操作に対して戻る / やり直すが一貫して動く
2. 対象外の浮動選択操作で履歴破損が起きない

### Step 5: レイヤー操作

作業内容:

1. LayerCommand 群追加
2. `LayerPanel` を履歴対応

完了条件:

1. レイヤー追加 / 削除 / 並び替え / 可視切替 / ロック切替が戻せる
2. 可能なら複製 / 結合も同じ履歴基盤へ乗る

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

現時点の確認状況:

1. ペンシル、消しゴム、塗りつぶしの Undo / Redo は確認済み
2. 反転、回転、新規作成の Undo / Redo は確認済み
3. レイヤー操作と opacity スライダーの Undo / Redo は確認済み
4. 浮動選択変形、タイルセット操作は未確認

1. ペンシルを長く引いて1回で取り消せる
2. 消しゴムストロークが1回で戻せる
3. 塗りつぶしが戻せる
4. 反転 / 回転が戻せる
5. レイヤー追加 / 削除 / 並び順変更が戻せる
6. チップ貼り付け / 入れ替え / クリアが戻せる
7. 行追加 / 列追加 / 行削除 / 列削除が戻せる
8. 通過フラグ変更が戻せる
9. free モードと tileset モードで同じ履歴基盤が動く
10. 選択ツール実装済み環境でも Undo 非対応範囲が誤作動しない

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
3. 選択ツール系は初回対象外なら明示して混在を避ける

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
6. `NewFileCommand`
7. `EditorScene` の `EDIT_UNDO / EDIT_REDO`
8. `Ctrl+Z / Ctrl+Shift+Z / Ctrl+Y`

この段階では次を後回しにする。

1. レイヤー操作
2. タイルセット構造変更
3. 通過フラグ
4. 浮動選択の移動 / 確定 / 変形
5. パレット編集

理由:

1. まず日常編集の大部分を戻せるようにする
2. 既存コードへの影響範囲を抑える
3. コマンド基盤の妥当性を先に検証できる
4. 選択ツール系は状態遷移が多く、初回導入で同時に扱うと切り分けが難しい

補足:

1. `NewFileCommand` は Phase 1 完了条件に含まれるため、最初の実装範囲にも含める
2. tileset モードの通常描画は Phase 1 から同じ履歴基盤で扱う
3. 浮動選択は「未対応」ではなく「別フェーズへ延期」として明記する

---

## 16. 完了イメージ

Undo / Redo 完了後の最低ラインは次の状態。

1. ペンシル、消しゴム、塗りつぶしが自然に戻せる
2. 反転、回転、新規作成が戻せる
3. メニューとショートカットの両方で操作できる
4. free モード / tileset モードの両方で破綻しない
5. 選択ツール未対応範囲がある場合でも、対象外であることが設計上明確になっている
6. 今後の選択ツール、一括編集の基盤として再利用できる

この機能は単体でも価値が高いが、今後のすべての編集強化の基礎になる。