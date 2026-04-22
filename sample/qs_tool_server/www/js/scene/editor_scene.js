/**
 * EditorScene
 * ドットエディタのメインシーン。
 */
class EditorScene extends Scene {
  constructor() {
    super();
    /** @type {MenuBar} */
    this._menuBar = new MenuBar();

    /** @type {PixelCanvas} */
    this._pixelCanvas = new PixelCanvas();

    // PixelCanvas コールバック ― ツールディスパッチ
    this._pixelCanvas.onPixelDown = (px, py, button, appData) => {
      this._applyTool(px, py, button, appData, true);
      console.log(`[EditorScene] pixel down (${px}, ${py}) tool=${appData.activeTool}`);
    };
    this._pixelCanvas.onPixelMove = (px, py, button, appData) => {
      this._applyTool(px, py, button, appData, false);
    };
    this._pixelCanvas.onPixelUp = (px, py, button, appData) => {
      this._onPixelToolUp(px, py, button, appData);
      console.log(`[EditorScene] pixel up (${px}, ${py}) button=${button}`);
    };

    /** 選択ツールドラッグ状態 @type {{ startX: number, startY: number, lastX: number, lastY: number }|null} */
    this._selectionDrag = null;

    /** 浮動選択ドラッグ状態 @type {{ offsetX: number, offsetY: number }|null} */
    this._floatingDrag = null;

    /** 通常選択のドラッグ開始待機状態 @type {{ offsetX: number, offsetY: number }|null} */
    this._selectionMovePrimed = null;

    /** @type {NewFileDialog} */
    this._newFileDialog = new NewFileDialog(
      (width, height, bgColor) => {
        if (!this._appData) return;
        const fillColor = bgColor === 'white'
          ? PixelData.rgba(255, 255, 255, 255)
          : 0x00000000;
        this._appData.editMode    = 'free';
        this._appData.tilesetData = null;
        this._appData.clearSelection();
        this._appData.createPixelData(width, height, fillColor);
        this._pixelCanvas.resetView();
        this._pixelCanvas.markDirty();
        console.log(`[EditorScene] new file: ${width}x${height} bg=${bgColor}`);
      },
      () => console.log('[EditorScene] new file dialog cancelled'),
    );

    /** タイルセット新規作成ダイアログ @type {NewTilesetDialog} */
    this._newTilesetDialog = new NewTilesetDialog(
      (chipW, chipH, cols, rows, bgColor) => {
        if (!this._appData) return;
        const fillColor = bgColor === 'white'
          ? PixelData.rgba(255, 255, 255, 255)
          : 0x00000000;
        this._appData.tilesetData  = new TilesetData(chipW, chipH, cols, rows, fillColor);
        this._appData.editMode     = 'tileset';
        this._appData.selectedChip = { col: 0, row: 0 };
        this._appData.clearSelection();
        this._pixelCanvas.resetView();
        this._pixelCanvas.markDirty();
        console.log(`[EditorScene] new tileset: ${chipW}x${chipH} chip, ${cols}x${rows} grid, bg=${bgColor}`);
      },
      () => console.log('[EditorScene] new tileset dialog cancelled'),
    );

    /** カラーパレットウィンドウ (EditablePalette32 コンテンツ) @type {ColorPaletteWindow} */
    this._colorPaletteWin = new ColorPaletteWindow(new EditablePalette32());

    /** カラーピッカーダイアログ @type {ColorPickerDialog} */
    this._colorPickerDialog = new ColorPickerDialog(
      (color) => {
        // パレットセル編集時
        if (this._paletteEditCallback) {
          this._paletteEditCallback(color);
          this._paletteEditCallback = null;
          // 選択中の前景色もパレット編集結果に更新
          if (this._appData) this._appData.foreColor = color;
          return;
        }
        // クォータービュータイルダイアログのスウォッチ色変更時
        if (this._quarterViewTileColorCallback) {
          this._quarterViewTileColorCallback(color);
          this._quarterViewTileColorCallback = null;
          return;
        }
        if (!this._appData) return;
        if (this._colorPickerTarget === 'back') {
          this._appData.backColor = color;
        } else {
          this._appData.foreColor = color;
        }
        console.log(`[EditorScene] color picked: 0x${color.toString(16).padStart(8, '0')}`);
      },
      () => {
        this._paletteEditCallback = null;
        this._quarterViewTileColorCallback = null;
        console.log('[EditorScene] color picker cancelled');
      },
    );
    /** @type {'fore' | 'back'} */
    this._colorPickerTarget = 'fore';

    /** パレット色編集用コールバック @type {((color: number) => void)|null} */
    this._paletteEditCallback = null;

    // パレットのスウォッチクリックでカラーピッカーを開く
    this._colorPaletteWin.getPalette().onSwatchClick = (target, appData) => {
      this._colorPickerTarget = target;
      const color = target === 'back' ? appData.backColor : appData.foreColor;
      this._colorPickerDialog.showWithColor(color);
    };

    // パレットセルのダブルクリックでカラーピッカーを開いてパレット色を編集
    this._colorPaletteWin.getPalette().onCellDoubleClick = (index, currentColor, applyFn) => {
      this._paletteEditCallback = applyFn;
      // 透明色（A=0）の場合はアルファ255の黒で開く（ユーザーが色を選びやすくするため）
      const a = (currentColor >>> 24) & 0xFF;
      const initColor = a === 0 ? PixelData.rgba(0, 0, 0, 255) : currentColor;
      this._colorPickerDialog.showWithColor(initColor);
    };

    /** ツールバーウィンドウ @type {ToolBarWindow} */
    this._toolBarWin = new ToolBarWindow();

    /** レイヤーパネルウィンドウ @type {LayerPanelWindow} */
    this._layerPanelWin = new LayerPanelWindow();
    this._layerPanelWin.onChange = () => {
      this._pixelCanvas.markDirty();
    };

    /** チップパレットウィンドウ @type {ChipPaletteWindow} */
    this._chipPaletteWin = new ChipPaletteWindow();

    /** タイルプレビューウィンドウ @type {TilePreviewWindow} */
    this._tilePreviewWin = new TilePreviewWindow();
    /** タイルプレビュー表示フラグ */
    this._tilePreviewVisible = true;
    this._chipPaletteWin.onChipSelect = (col, row) => {
      if (!this._appData) return;
      const prev = this._appData.selectedChip;
      const changed = prev.col !== col || prev.row !== row;
      this._appData.selectedChip = { col, row };
      if (changed) {
        // チップ切り替え時: パンをリセットして中央表示に戻す
        this._pixelCanvas.resetPan();
      }
      this._appData.clearSelection();
      this._pixelCanvas.markDirty();
      console.log(`[EditorScene] chip selected: (${col}, ${row})`);
    };
    this._chipPaletteWin.onChipDoubleClick = (col, row) => {
      if (!this._appData) return;
      this._appData.selectedChip = { col, row };
      this._appData.clearSelection();
      this._pixelCanvas.resetView();
      this._pixelCanvas.markDirty();
      console.log(`[EditorScene] chip double-clicked (focus): (${col}, ${row})`);
    };
    this._chipPaletteWin.onChipContextMenu = (col, row, screenX, screenY) => {
      if (!this._appData || !this._appData.tilesetData) return;
      if (!this._appData.showPassFlags) return;
      const td = this._appData.tilesetData;
      if (row < 0 || row >= td.rows || col < 0 || col >= td.columns) return;
      // 通過フラグをトグル
      td.passFlags[row][col] = !td.passFlags[row][col];
      const state = td.passFlags[row][col] ? 'ON (通過可能)' : 'OFF (通過不可)';
      console.log(`[EditorScene] chip (${col}, ${row}) 通過フラグ: ${state}`);
    };

    /** エクスポートダイアログ @type {SaveDialog} */
    this._saveDialog = new SaveDialog(
      (filename, format) => {
        if (!this._appData) return;
        const isTileset = this._appData.editMode === 'tileset' && this._appData.tilesetData;
        if (format === 'png') {
          if (isTileset) {
            // タイルセットモード: 全チップ合成PNG
            PixelDataConverter.exportTilesetAsPng(this._appData.tilesetData, filename)
              .catch(err => console.error('[EditorScene] tileset PNG エクスポートエラー:', err));
          } else {
            const composited = this._appData.layerData.composite();
            PixelDataConverter.exportAsPng(composited, filename)
              .catch(err => console.error('[EditorScene] PNG エクスポートエラー:', err));
          }
        } else if (format === 'qts') {
          if (isTileset && this._appData.palette) {
            PixelDataConverter.exportTilesetAsQts(
              this._appData.tilesetData, this._appData.palette, filename
            );
          } else {
            console.warn('[EditorScene] QTS エクスポートはタイルセットモード専用です');
          }
        } else {
          if (isTileset) {
            // タイルセットモード: v2 JSON
            PixelDataConverter.exportTilesetAsJson(this._appData.tilesetData, filename);
          } else {
            PixelDataConverter.exportAsJson(this._appData.pixelData, filename);
          }
        }
        console.log(`[EditorScene] export: ${filename} (${format})`);
      },
      () => console.log('[EditorScene] export cancelled'),
    );

    /** クォータービュータイル生成ダイアログ @type {QuarterViewTileDialog} */
    this._quarterViewTileDialog = new QuarterViewTileDialog(
      (params) => this._onQuarterViewTileConfirm(params),
      () => console.log('[EditorScene] quarter view tile dialog cancelled'),
    );
    // スウォッチクリック時: カラーピッカーを開く
    this._quarterViewTileDialog.onColorSwatchClick = (currentColor, callback) => {
      this._quarterViewTileColorCallback = callback;
      this._colorPickerDialog.showWithColor(currentColor);
    };
    /** カラーピッカーから色を受け取るコールバック @type {((color:number) => void)|null} */
    this._quarterViewTileColorCallback = null;

    /** チップ入れ替え用: 入れ替え元チップ座標 (null = 未選択) @type {{ col: number, row: number }|null} */
    this._swapSource = null;

    /** onEnter でセットされる共有データへの参照 */
    this._appData = null;

    /** スペースキー押下中フラグ（パンモード） */
    this._spaceDown = false;

    /** render() で受け取った canvas を保持（カーソル変更に使用） */
    this._canvas = null;

    /** タイルセットインポートダイアログ @type {ImportTilesetDialog} */
    this._importTilesetDialog = new ImportTilesetDialog(
      (chipW, chipH) => {
        if (!this._appData || !this._pendingImportPd) return;
        const td = PixelDataConverter.tilesetFromPixelData(this._pendingImportPd, chipW, chipH);
        this._appData.tilesetData  = td;
        this._appData.editMode     = 'tileset';
        this._appData.selectedChip = { col: 0, row: 0 };
        this._pixelCanvas.resetView();
        this._pixelCanvas.markDirty();
        this._pendingImportPd = null;
        console.log(`[EditorScene] tileset imported: ${chipW}x${chipH} chip, ${td.columns}x${td.rows} grid`);
      },
      () => {
        this._pendingImportPd = null;
        console.log('[EditorScene] import tileset dialog cancelled');
      },
    );

    /** PNG → タイルセット変換待ちの PixelData @type {PixelData|null} */
    this._pendingImportPd = null;

    /** ファイル入力用の隠し <input type="file"> (onEnter で生成) */
    this._fileInput = null;

    /** タイルセットインポート用の隠し <input type="file"> (onEnter で生成) */
    this._tilesetFileInput = null;
  }

  /**
   * シーン開始時に入力ハンドラを登録する
   * @param {Input}   input
   * @param {AppData} appData
   */
  onEnter(input, appData) {
    this._appData = appData;

    // パレットを AppData に登録
    appData.palette = this._colorPaletteWin.getPalette();

    // --- 隠し <input type="file"> を作成 ---
    this._fileInput = document.createElement('input');
    this._fileInput.type    = 'file';
    this._fileInput.accept  = '.png,.json,.qts';
    this._fileInput.style.display = 'none';
    document.body.appendChild(this._fileInput);
    this._fileInput.addEventListener('change', () => {
      const file = this._fileInput.files?.[0];
      if (!file) return;
      // 次回同一ファイルを選んでも change が発火されるようにリセット
      this._fileInput.value = '';
      PixelDataConverter.importFromFile(file)
        .then(result => {
          if (result && result.tilesetData && result.palette) {
            // .qts インポート結果
            this._appData.tilesetData  = result.tilesetData;
            this._appData.editMode     = 'tileset';
            this._appData.selectedChip = { col: 0, row: 0 };
            this._appData.clearSelection();
            // パレットを復元
            const paletteWin = this._colorPaletteWin.getPalette();
            if (paletteWin instanceof EditablePalette32) {
              const importedColors = result.palette.getColors();
              for (let i = 1; i < importedColors.length; i++) {
                paletteWin.setColor(i, importedColors[i]);
              }
            }
            this._appData.palette = this._colorPaletteWin.getPalette();
            this._pixelCanvas.resetView();
            this._pixelCanvas.markDirty();
            console.log(`[EditorScene] QTS imported: ${file.name}`);
          } else if (result instanceof TilesetData) {
            // v2 タイルセット JSON
            this._appData.tilesetData  = result;
            this._appData.editMode     = 'tileset';
            this._appData.selectedChip = { col: 0, row: 0 };
            this._appData.clearSelection();
            this._pixelCanvas.resetView();
            this._pixelCanvas.markDirty();
            console.log(`[EditorScene] tileset JSON imported: ${file.name}`);
          } else {
            // v1 従来形式 (PixelData)
            this._appData.editMode    = 'free';
            this._appData.tilesetData = null;
            this._appData.pixelData = result;
            this._appData.clearSelection();
            this._pixelCanvas.resetView();
            this._pixelCanvas.markDirty();
            console.log(`[EditorScene] import: ${file.name} → ${result.width}x${result.height}`);
          }
        })
        .catch(err => {
          console.error('[EditorScene] インポートエラー:', err.message);
        });
    });

    // --- タイルセットインポート用の隠し <input type="file"> を作成 ---
    this._tilesetFileInput = document.createElement('input');
    this._tilesetFileInput.type    = 'file';
    this._tilesetFileInput.accept  = '.png,.json,.qts';
    this._tilesetFileInput.style.display = 'none';
    document.body.appendChild(this._tilesetFileInput);
    this._tilesetFileInput.addEventListener('change', () => {
      const file = this._tilesetFileInput.files?.[0];
      if (!file) return;
      this._tilesetFileInput.value = '';
      const name = file.name.toLowerCase();
      if (name.endsWith('.qts')) {
        // QTS の場合: バイナリタイルセットとしてインポート
        PixelDataConverter.importFromQts(file)
          .then(result => {
            this._appData.tilesetData  = result.tilesetData;
            this._appData.editMode     = 'tileset';
            this._appData.selectedChip = { col: 0, row: 0 };
            this._appData.clearSelection();
            // パレットを復元
            const paletteWin = this._colorPaletteWin.getPalette();
            if (paletteWin instanceof EditablePalette32) {
              const importedColors = result.palette.getColors();
              for (let i = 1; i < importedColors.length; i++) {
                paletteWin.setColor(i, importedColors[i]);
              }
            }
            this._appData.palette = this._colorPaletteWin.getPalette();
            this._pixelCanvas.resetView();
            this._pixelCanvas.markDirty();
            console.log(`[EditorScene] QTS tileset opened: ${file.name}`);
          })
          .catch(err => console.error('[EditorScene] QTS読み込みエラー:', err.message));
      } else if (name.endsWith('.json')) {
        // JSON の場合: v2 タイルセットとしてインポート試行
        PixelDataConverter.importFromJson(file)
          .then(result => {
            if (result instanceof TilesetData) {
              this._appData.tilesetData  = result;
              this._appData.editMode     = 'tileset';
              this._appData.selectedChip = { col: 0, row: 0 };
              this._appData.clearSelection();
              this._pixelCanvas.resetView();
              this._pixelCanvas.markDirty();
              console.log(`[EditorScene] tileset JSON opened: ${file.name}`);
            } else {
              // v1 JSON → チップサイズ指定ダイアログを表示
              this._pendingImportPd = result;
              this._importTilesetDialog.showWithImage(result.width, result.height);
            }
          })
          .catch(err => console.error('[EditorScene] タイルセットインポートエラー:', err.message));
      } else {
        // PNG の場合: 読み込んでチップサイズ指定ダイアログを表示
        PixelDataConverter.importFromPng(file)
          .then(pd => {
            this._pendingImportPd = pd;
            this._importTilesetDialog.showWithImage(pd.width, pd.height);
          })
          .catch(err => console.error('[EditorScene] PNG読み込みエラー:', err.message));
      }
    });

    // --- MenuBar 選択コールバック ---
    this._menuBar.onSelect = (id) => {
      console.log('[EditorScene] menu selected:', id);
      if (id === MenuConstants.FILE_NEW) {
        this._newFileDialog.show();
        return;
      }
      if (id === MenuConstants.FILE_NEW_TILESET) {
        this._newTilesetDialog.show();
        return;
      }
      if (id === MenuConstants.FILE_OPEN) {
        this._fileInput.click();
        return;
      }
      if (id === MenuConstants.FILE_OPEN_TILESET) {
        this._tilesetFileInput.click();
        return;
      }
      if (id === MenuConstants.FILE_SAVE) {
        if (this._appData?.editMode === 'tileset' && this._appData.tilesetData) {
          this._saveDialog.show();
        } else if (this._appData?.pixelData?.pixels) {
          this._saveDialog.show();
        } else {
          console.warn('[EditorScene] エクスポート: ファイルが作成されていません');
        }
        return;
      }
      if (id === MenuConstants.FILE_EXPORT_TILESET) {
        if (this._appData?.editMode !== 'tileset' || !this._appData.tilesetData) {
          console.warn('[EditorScene] タイルセットモードではありません');
          return;
        }
        PixelDataConverter.exportTilesetAsPng(this._appData.tilesetData)
          .catch(err => console.error('[EditorScene] タイルセットPNGエクスポートエラー:', err));
        return;
      }
      if (id === MenuConstants.FILE_EXPORT_CHIP) {
        if (this._appData?.editMode !== 'tileset' || !this._appData.tilesetData) {
          console.warn('[EditorScene] タイルセットモードではありません');
          return;
        }
        const { col, row } = this._appData.selectedChip;
        PixelDataConverter.exportChipAsPng(
          this._appData.tilesetData, col, row, `chip_${col}_${row}.png`
        ).catch(err => console.error('[EditorScene] チップPNGエクスポートエラー:', err));
        return;
      }
      if (id === MenuConstants.VIEW_GRID) {
        this._pixelCanvas.showGrid = !this._pixelCanvas.showGrid;
        console.log('[EditorScene] grid:', this._pixelCanvas.showGrid);
        return;
      }

      if (id === MenuConstants.EDIT_SELECT_ALL) {
        this._selectAll();
        return;
      }
      if (id === MenuConstants.EDIT_COPY) {
        this._copySelectionToClipboard(false);
        return;
      }
      if (id === MenuConstants.EDIT_CUT) {
        this._copySelectionToClipboard(true);
        return;
      }
      if (id === MenuConstants.EDIT_PASTE) {
        this._pasteSelectionClipboard();
        return;
      }

      // ---- 反転 ----
      if (id === MenuConstants.EDIT_FLIP_H || id === MenuConstants.EDIT_FLIP_V) {
        const isH = id === MenuConstants.EDIT_FLIP_H;
        if (this._appData?.hasFloatingSelection?.()) {
          this._transformFloatingSelection(this._appData, isH ? 'flipH' : 'flipV');
          console.log(`[EditorScene] floating selection ${isH ? 'flipH' : 'flipV'}`);
          return;
        }
        if (this._appData?.editMode === 'tileset' && this._appData.tilesetData) {
          const { col, row } = this._appData.selectedChip;
          const layerData = this._appData.tilesetData.getChipLayerData(col, row);
          if (layerData) {
            layerData.layers.forEach(layer => {
              if (layer.pixelData) isH ? layer.pixelData.flipH() : layer.pixelData.flipV();
            });
            layerData.markCompositeDirty();
            this._pixelCanvas.markDirty();
          }
        } else if (this._appData?.layerData) {
          this._appData.layerData.layers.forEach(layer => {
            if (layer.pixelData) isH ? layer.pixelData.flipH() : layer.pixelData.flipV();
          });
          this._appData.layerData.markCompositeDirty();
          this._pixelCanvas.markDirty();
        }
        console.log(`[EditorScene] ${isH ? '左右' : '上下'}に反転`);
        return;
      }

      // ---- 回転 ----
      if (id === MenuConstants.EDIT_ROTATE_CW || id === MenuConstants.EDIT_ROTATE_CCW) {
        const isCW = id === MenuConstants.EDIT_ROTATE_CW;
        if (this._appData?.hasFloatingSelection?.()) {
          this._transformFloatingSelection(this._appData, isCW ? 'rotate90CW' : 'rotate90CCW');
          console.log(`[EditorScene] floating selection ${isCW ? 'rotate90CW' : 'rotate90CCW'}`);
          return;
        }
        if (this._appData?.editMode === 'tileset' && this._appData.tilesetData) {
          const { col, row } = this._appData.selectedChip;
          const layerData = this._appData.tilesetData.getChipLayerData(col, row);
          if (layerData) {
            layerData.layers.forEach(layer => {
              if (layer.pixelData) isCW ? layer.pixelData.rotate90CW() : layer.pixelData.rotate90CCW();
            });
            layerData.markCompositeDirty();
            this._pixelCanvas.markDirty();
          }
        } else if (this._appData?.layerData) {
          this._appData.layerData.layers.forEach(layer => {
            if (layer.pixelData) isCW ? layer.pixelData.rotate90CW() : layer.pixelData.rotate90CCW();
          });
          this._appData.layerData.markCompositeDirty();
          this._pixelCanvas.markDirty();
        }
        console.log(`[EditorScene] ${isCW ? '時計回り' : '反時計回り'}に90度回転`);
        return;
      }

      // ---- タイルセットメニュー ----
      if (id === MenuConstants.TILESET_COPY_CHIP) {
        if (this._appData.editMode !== 'tileset' || !this._appData.tilesetData) return;
        const { col, row } = this._appData.selectedChip;
        this._appData.chipClipboard = this._appData.tilesetData.cloneChipLayerData(col, row);
        console.log(`[EditorScene] chip copied: (${col}, ${row})`);
        return;
      }
      if (id === MenuConstants.TILESET_PASTE_CHIP) {
        if (this._appData.editMode !== 'tileset' || !this._appData.tilesetData) return;
        if (!this._appData.chipClipboard) {
          console.warn('[EditorScene] チップクリップボードが空です');
          return;
        }
        const { col, row } = this._appData.selectedChip;
        const ok = this._appData.tilesetData.pasteChipLayerData(this._appData.chipClipboard, col, row);
        if (ok) {
          this._pixelCanvas.markDirty();
          console.log(`[EditorScene] chip pasted: (${col}, ${row})`);
        }
        return;
      }
      if (id === MenuConstants.TILESET_CLEAR_CHIP) {
        if (this._appData.editMode !== 'tileset' || !this._appData.tilesetData) return;
        const { col, row } = this._appData.selectedChip;
        this._appData.tilesetData.clearChip(col, row);
        this._pixelCanvas.markDirty();
        console.log(`[EditorScene] chip cleared: (${col}, ${row})`);
        return;
      }
      if (id === MenuConstants.TILESET_SWAP_CHIP) {
        if (this._appData.editMode !== 'tileset' || !this._appData.tilesetData) return;
        const { col, row } = this._appData.selectedChip;
        if (!this._swapSource) {
          // 1回目: 入れ替え元を記録
          this._swapSource = { col, row };
          console.log(`[EditorScene] swap source set: (${col}, ${row}) — 別のチップを選択して再度実行`);
        } else {
          // 2回目: 入れ替え実行
          this._appData.tilesetData.swapChips(this._swapSource.col, this._swapSource.row, col, row);
          this._pixelCanvas.markDirty();
          console.log(`[EditorScene] chips swapped: (${this._swapSource.col}, ${this._swapSource.row}) <-> (${col}, ${row})`);
          this._swapSource = null;
        }
        return;
      }
      if (id === MenuConstants.TILESET_ADD_ROW) {
        if (this._appData.editMode !== 'tileset' || !this._appData.tilesetData) return;
        this._appData.tilesetData.addRow();
        this._pixelCanvas.markDirty();
        console.log(`[EditorScene] row added: now ${this._appData.tilesetData.rows} rows`);
        return;
      }
      if (id === MenuConstants.TILESET_ADD_COL) {
        if (this._appData.editMode !== 'tileset' || !this._appData.tilesetData) return;
        this._appData.tilesetData.addColumn();
        this._pixelCanvas.markDirty();
        console.log(`[EditorScene] column added: now ${this._appData.tilesetData.columns} cols`);
        return;
      }
      if (id === MenuConstants.TILESET_REMOVE_ROW) {
        if (this._appData.editMode !== 'tileset' || !this._appData.tilesetData) return;
        const td = this._appData.tilesetData;
        td.removeRow();
        // 選択チップが範囲外になった場合の補正
        if (this._appData.selectedChip.row >= td.rows) {
          this._appData.selectedChip.row = td.rows - 1;
        }
        this._pixelCanvas.markDirty();
        console.log(`[EditorScene] row removed: now ${td.rows} rows`);
        return;
      }
      if (id === MenuConstants.TILESET_REMOVE_COL) {
        if (this._appData.editMode !== 'tileset' || !this._appData.tilesetData) return;
        const td = this._appData.tilesetData;
        td.removeColumn();
        // 選択チップが範囲外になった場合の補正
        if (this._appData.selectedChip.col >= td.columns) {
          this._appData.selectedChip.col = td.columns - 1;
        }
        this._pixelCanvas.markDirty();
        console.log(`[EditorScene] column removed: now ${td.columns} cols`);
        return;
      }

      if (id === MenuConstants.TILESET_TILE_PREVIEW) {
        this._tilePreviewVisible = !this._tilePreviewVisible;
        console.log(`[EditorScene] tile preview: ${this._tilePreviewVisible ? 'show' : 'hide'}`);
        return;
      }

      if (id === MenuConstants.VIEW_PASS_FLAGS) {
        this._appData.showPassFlags = !this._appData.showPassFlags;
        console.log(`[EditorScene] pass flags display: ${this._appData.showPassFlags ? 'show' : 'hide'}`);
        return;
      }

      // ---- 生成メニュー ----
      if (id === MenuConstants.GENERATE_QUARTER_VIEW_TILE) {
        this._quarterViewTileDialog.show();
        return;
      }
    };

    // --- キーボード ---
    this._onKeyDown = e => {
      const key = e.key;
      const code = e.code;
      const raw = e.raw;

      if (this._colorPickerDialog.isVisible)    { this._colorPickerDialog.onKeyDown(e);    return; }
      if (this._quarterViewTileDialog.isVisible) { this._quarterViewTileDialog.onKeyDown(e); return; }
      if (this._newFileDialog.isVisible)        { this._newFileDialog.onKeyDown(e);        return; }
      if (this._newTilesetDialog.isVisible)     { this._newTilesetDialog.onKeyDown(e);     return; }
      if (this._importTilesetDialog.isVisible)  { this._importTilesetDialog.onKeyDown(e);  return; }
      if (this._saveDialog.isVisible)           { this._saveDialog.onKeyDown(e);           return; }
      if (key === ' ') {
        raw?.preventDefault?.();
        if (!this._spaceDown) {
          this._spaceDown = true;
          if (this._canvas) this._canvas.style.cursor = 'grab';
        }
        return;
      }
      if ((e.ctrl || e.meta) && (key === 'a' || key === 'A')) {
        raw?.preventDefault?.();
        this._selectAll();
        return;
      }
      if ((e.ctrl || e.meta) && (key === 'c' || key === 'C')) {
        raw?.preventDefault?.();
        this._copySelectionToClipboard(false);
        return;
      }
      if ((e.ctrl || e.meta) && (key === 'x' || key === 'X')) {
        raw?.preventDefault?.();
        this._copySelectionToClipboard(true);
        return;
      }
      if ((e.ctrl || e.meta) && (key === 'v' || key === 'V')) {
        raw?.preventDefault?.();
        this._pasteSelectionClipboard();
        return;
      }
      if (key === 'Enter' || code === 'Enter' || code === 'NumpadEnter') {
        if (this._appData?.hasFloatingSelection?.()) {
          raw?.preventDefault?.();
          this._commitFloatingSelection(this._appData);
        }
        return;
      }
      if (key === 'Escape' || key === 'Esc' || code === 'Escape') {
        raw?.preventDefault?.();
        if (this._appData?.hasFloatingSelection?.()) {
          this._cancelFloatingSelection(this._appData);
        } else {
          this._appData?.clearSelection();
        }
        this._selectionDrag = null;
        this._floatingDrag = null;
        this._selectionMovePrimed = null;
        this._pixelCanvas.markDirty();
        return;
      }
      console.log('[EditorScene] keydown', e);
    };
    this._onKeyUp = e => {
      if (e.key === ' ') {
    // カーソルを spaceDown 解除時にツール状態へ戻す
        this._spaceDown = false;
        this._pixelCanvas.endPan();
        this._updateToolCursor();
        return;
      }
      console.log('[EditorScene] keyup', e);
    };
    input.on('keydown', this._onKeyDown);
    input.on('keyup',   this._onKeyUp);

    // --- マウス ---
    this._onMouseMove = e => {
      this._quarterViewTileDialog.onMouseMove(e);
      this._newFileDialog.onMouseMove(e);
      this._newTilesetDialog.onMouseMove(e);
      this._importTilesetDialog.onMouseMove(e);
      this._saveDialog.onMouseMove(e);
      this._colorPickerDialog.onMouseMove(e);
      this._colorPaletteWin.onMouseMove(e, appData);
      this._toolBarWin.onMouseMove(e, appData);
      this._layerPanelWin.onMouseMove(e, appData);
      this._chipPaletteWin.onMouseMove(e, appData);
      if (this._tilePreviewVisible) this._tilePreviewWin.onMouseMove(e, appData);
      if (!this._newFileDialog.isVisible && !this._newTilesetDialog.isVisible && !this._importTilesetDialog.isVisible) {
        this._menuBar.onMouseMove(e);
        if (this._spaceDown) {
          // パンモード: ドラッグ中なら位置を更新
          this._pixelCanvas.movePan(e.x, e.y);
        } else {
          this._pixelCanvas.onMouseMove(e, appData);
        }
      }
    };
    this._onMouseDown = e => {
      if (this._quarterViewTileDialog.isVisible && !this._colorPickerDialog.isVisible) { this._quarterViewTileDialog.onMouseDown(e); return; }
      if (this._newFileDialog.isVisible)        { this._newFileDialog.onMouseDown(e);        return; }
      if (this._newTilesetDialog.isVisible)     { this._newTilesetDialog.onMouseDown(e);     return; }
      if (this._importTilesetDialog.isVisible)  { this._importTilesetDialog.onMouseDown(e);  return; }
      if (this._saveDialog.isVisible)           { this._saveDialog.onMouseDown(e);           return; }
      if (this._colorPickerDialog.isVisible)    { this._colorPickerDialog.onMouseDown(e);    return; }
      // メニューバー領域のクリックはピクセル操作に渡さない
      if (e.y < MenuBar.HEIGHT) {
        this._menuBar.onMouseDown(e);
        return;
      }
      if (this._spaceDown) {
        // パンモード開始
        this._pixelCanvas.startPan(e.x, e.y);
        if (this._canvas) this._canvas.style.cursor = 'grabbing';
        return;
      }
      if (!this._menuBar.isOpen) {
        // UIWindow 群(ツールバーまたはパレット)が消費した場合はピクセル操作に渡さない
        const consumed = this._chipPaletteWin.onMouseDown(e, appData) ||
                         (this._tilePreviewVisible && this._tilePreviewWin.onMouseDown(e, appData)) ||
                         this._colorPaletteWin.onMouseDown(e, appData) ||
                         this._toolBarWin.onMouseDown(e, appData) ||
                         this._layerPanelWin.onMouseDown(e, appData);
        if (consumed) {
          // ツール切り替え時はカーソルを更新
          this._updateToolCursor();
        } else {
          this._pixelCanvas.onMouseDown(e, appData);
        }
      }
      this._menuBar.onMouseDown(e);
    };
    this._onMouseUp = e => {
      if (this._quarterViewTileDialog.isVisible && !this._colorPickerDialog.isVisible) { this._quarterViewTileDialog.onMouseUp(e); return; }
      if (this._newFileDialog.isVisible)        { this._newFileDialog.onMouseUp(e);        return; }
      if (this._newTilesetDialog.isVisible)     { this._newTilesetDialog.onMouseUp(e);     return; }
      if (this._importTilesetDialog.isVisible)  { this._importTilesetDialog.onMouseUp(e);  return; }
      if (this._saveDialog.isVisible)           { this._saveDialog.onMouseUp(e);           return; }
      if (this._colorPickerDialog.isVisible)    { this._colorPickerDialog.onMouseUp(e);    return; }
      if (this._pixelCanvas._isPanning) {
        this._pixelCanvas.endPan();
        if (this._canvas) this._canvas.style.cursor = this._spaceDown ? 'grab' : '';
        return;
      }
      this._colorPaletteWin.onMouseUp(e, appData);
      this._toolBarWin.onMouseUp(e, appData);
      this._layerPanelWin.onMouseUp(e, appData);
      this._chipPaletteWin.onMouseUp(e, appData);
      if (this._tilePreviewVisible) this._tilePreviewWin.onMouseUp(e, appData);
      this._menuBar.onMouseUp(e);
      this._pixelCanvas.onMouseUp(e, appData);
    };
    this._onWheel = e => {
      if (this._quarterViewTileDialog.isVisible || this._newFileDialog.isVisible || this._newTilesetDialog.isVisible || this._importTilesetDialog.isVisible || this._saveDialog.isVisible || this._colorPickerDialog.isVisible) return;
      this._pixelCanvas.zoom(-e.deltaY, e.x, e.y);
    };
    this._onContextMenu = e => {
      this._chipPaletteWin.onContextMenu(e, appData);
    };
    input.on('mousemove',   this._onMouseMove);
    input.on('mousedown',   this._onMouseDown);
    input.on('mouseup',     this._onMouseUp);
    input.on('wheel',       this._onWheel);
    input.on('contextmenu', this._onContextMenu);

    // --- タッチ ---
    this._onTouchStart  = e => console.log('[EditorScene] touchstart',  e);
    this._onTouchEnd    = e => console.log('[EditorScene] touchend',    e);
    this._onTouchMove   = e => console.log('[EditorScene] touchmove',   e);
    this._onTouchCancel = e => console.log('[EditorScene] touchcancel', e);
    input.on('touchstart',  this._onTouchStart);
    input.on('touchend',    this._onTouchEnd);
    input.on('touchmove',   this._onTouchMove);
    input.on('touchcancel', this._onTouchCancel);
  }

  /**
   * @param {number}  dt
   * @param {AppData} appData
   */
  update(dt, appData) {
    // 今後：appData.pixelData への書き込みなどをここに追加
  }

  /**
   * @param {CanvasRenderingContext2D} ctx
   * @param {HTMLCanvasElement}       canvas
   * @param {AppData}                 appData
   */
  render(ctx, canvas, appData) {
    // 背景を灰色で塗りつぶす
    ctx.fillStyle = '#808080';
    ctx.fillRect(0, 0, canvas.width, canvas.height);

    // canvas 参照を保持（カーソル変更に使用）
    this._canvas = canvas;

    // 全レイヤーを合成してピクセルデータを画面中央に描画
    this._pixelCanvas.render(ctx, canvas, appData.layerData.composite(), appData);

    // ツールバーウィンドウ
    this._toolBarWin.render(ctx, canvas, appData);

    // レイヤーパネルウィンドウ
    this._layerPanelWin.render(ctx, canvas, appData);

    // チップパレットウィンドウ (タイルセットモード時のみ表示)
    this._chipPaletteWin.render(ctx, canvas, appData);

    // タイルプレビューウィンドウ (タイルセットモード時のみ表示)
    if (this._tilePreviewVisible) {
      this._tilePreviewWin.render(ctx, canvas, appData);
    }

    // カラーパレットウィンドウ (ダイアログより前、ピクセルキャンバスの上)
    this._colorPaletteWin.render(ctx, canvas, appData);

    // ダイアログ（メニューバーより前、オーバーレイがメニューを覆う）
    this._newFileDialog.render(ctx, canvas);
    this._newTilesetDialog.render(ctx, canvas);
    this._importTilesetDialog.render(ctx, canvas);
    this._saveDialog.render(ctx, canvas);
    this._quarterViewTileDialog.render(ctx, canvas);
    this._colorPickerDialog.render(ctx, canvas);

    // メニューバーを最前面に描画
    this._menuBar.render(ctx, canvas);
  }

  // ----------------------------------------------------------------
  // ツール処理
  // ----------------------------------------------------------------

  /**
   * アクティブツールに応じてピクセル操作を実行する。
   * @param {number}  px
   * @param {number}  py
   * @param {number}  button    0=左 / 2=右
   * @param {AppData} appData
   * @param {boolean} isDown    mousedown なら true、mousemove なら false
   */
  _applyTool(px, py, button, appData, isDown) {
    if (appData.hasFloatingSelection?.() && appData.activeTool !== 'selectRect') {
      this._commitFloatingSelection(appData);
    }

    const tool      = appData.activeTool;
    const drawColor = button === 2 ? appData.backColor : appData.foreColor;

    switch (tool) {
      case 'selectRect':
        this._applySelectionTool(px, py, appData, isDown);
        break;

      case 'pencil':
        appData.pixelData.setPixel(px, py, drawColor);
        appData.layerData.markCompositeDirty();
        this._pixelCanvas.markDirty();
        break;

      case 'eraser':
        appData.pixelData.setPixel(px, py, 0x00000000);
        appData.layerData.markCompositeDirty();
        this._pixelCanvas.markDirty();
        break;

      case 'fill':
        // 塗りつぶしは mousedown のみ（ドラッグは不要）
        if (isDown) {
          this._floodFill(appData.pixelData, px, py, drawColor);
          appData.layerData.markCompositeDirty();
          this._pixelCanvas.markDirty();
        }
        break;

      case 'eyedropper':
        // mousedown のみで色を拾う（合成結果から色を拾う）
        if (isDown) {
          const composited = appData.layerData.composite();
          const picked = composited.getPixel(px, py);
          if (button === 2) appData.backColor  = picked;
          else              appData.foreColor  = picked;
        }
        break;
    }
  }

  /**
   * 矩形選択ツールを適用する。
   * @param {number} px
   * @param {number} py
   * @param {AppData} appData
   * @param {boolean} isDown
   */
  _applySelectionTool(px, py, appData, isDown) {
    if (isDown) {
      if (appData.hasFloatingSelection()) {
        if (this._isPointInSelection(px, py, appData.selection)) {
          this._floatingDrag = {
            offsetX: px - appData.selection.x,
            offsetY: py - appData.selection.y,
          };
          return;
        }
        this._commitFloatingSelection(appData);
      }

      if (appData.hasSelection() && this._isPointInSelection(px, py, appData.selection)) {
        this._selectionMovePrimed = {
          offsetX: px - appData.selection.x,
          offsetY: py - appData.selection.y,
        };
        this._selectionDrag = null;
        return;
      }

      this._selectionDrag = {
        startX: px,
        startY: py,
        lastX: px,
        lastY: py,
      };
      this._selectionMovePrimed = null;
    } else if (this._selectionMovePrimed && appData.hasSelection()) {
      if (!this._liftSelectionToFloating(appData)) {
        this._selectionMovePrimed = null;
        return;
      }
      this._floatingDrag = {
        offsetX: this._selectionMovePrimed.offsetX,
        offsetY: this._selectionMovePrimed.offsetY,
      };
      this._selectionMovePrimed = null;

      const nx = px - this._floatingDrag.offsetX;
      const ny = py - this._floatingDrag.offsetY;
      const floating = appData.selection.floating;
      floating.dstX = nx;
      floating.dstY = ny;
      appData.selection.x = nx;
      appData.selection.y = ny;
      appData.selection.w = floating.width;
      appData.selection.h = floating.height;
      appData.layerData.markCompositeDirty();
      this._pixelCanvas.markDirty();
      return;
    } else if (this._floatingDrag && appData.hasFloatingSelection()) {
      const nx = px - this._floatingDrag.offsetX;
      const ny = py - this._floatingDrag.offsetY;
      const floating = appData.selection.floating;
      floating.dstX = nx;
      floating.dstY = ny;
      appData.selection.x = nx;
      appData.selection.y = ny;
      appData.selection.w = floating.width;
      appData.selection.h = floating.height;
      this._pixelCanvas.markDirty();
      return;
    } else if (this._selectionDrag) {
      this._selectionDrag.lastX = px;
      this._selectionDrag.lastY = py;
    } else {
      return;
    }

    const rect = this._makeSelectionRect(
      this._selectionDrag.startX,
      this._selectionDrag.startY,
      this._selectionDrag.lastX,
      this._selectionDrag.lastY,
    );
    appData.setSelectionRect(rect.x, rect.y, rect.w, rect.h);
    this._pixelCanvas.markDirty();
  }

  /**
   * ピクセルキャンバス上でのツール mouseup を処理する。
   * @param {number} px
   * @param {number} py
   * @param {number} button
   * @param {AppData} appData
   */
  _onPixelToolUp(px, py, button, appData) {
    if (appData.activeTool !== 'selectRect' || button !== 0) return;

    if (this._selectionMovePrimed) {
      this._selectionMovePrimed = null;
      this._pixelCanvas.markDirty();
      return;
    }

    if (this._floatingDrag) {
      this._floatingDrag = null;
      this._pixelCanvas.markDirty();
      return;
    }

    if (!this._selectionDrag) return;

    this._selectionDrag.lastX = px;
    this._selectionDrag.lastY = py;
    const rect = this._makeSelectionRect(
      this._selectionDrag.startX,
      this._selectionDrag.startY,
      this._selectionDrag.lastX,
      this._selectionDrag.lastY,
    );
    appData.setSelectionRect(rect.x, rect.y, rect.w, rect.h);
    this._selectionDrag = null;
    this._pixelCanvas.markDirty();
  }

  /**
   * ドラッグ座標から選択矩形を生成する。
   * @param {number} startX
   * @param {number} startY
   * @param {number} endX
   * @param {number} endY
   * @returns {{ x:number, y:number, w:number, h:number }}
   */
  _makeSelectionRect(startX, startY, endX, endY) {
    const x = Math.min(startX, endX);
    const y = Math.min(startY, endY);
    const w = Math.abs(endX - startX) + 1;
    const h = Math.abs(endY - startY) + 1;
    return { x, y, w, h };
  }

  /** 画像全体を選択する。 */
  _selectAll() {
    if (!this._appData?.pixelData) return;
    if (this._appData.hasFloatingSelection()) {
      this._commitFloatingSelection(this._appData);
    }
    this._appData.setSelectionRect(
      0,
      0,
      this._appData.pixelData.width,
      this._appData.pixelData.height,
    );
    this._selectionDrag = null;
    this._pixelCanvas.markDirty();
  }

  /**
   * 現在の選択範囲をクリップボードへコピーする。
   * cut=true の場合はコピー後に選択範囲を消去する。
   * @param {boolean} cut
   */
  _copySelectionToClipboard(cut) {
    if (this._appData?.hasFloatingSelection?.()) {
      this._commitFloatingSelection(this._appData);
    }

    if (!this._appData?.hasSelection?.() || !this._appData.pixelData) {
      console.warn('[EditorScene] 選択範囲がありません');
      return;
    }

    const sel = this._appData.selection;
    const clipPixelData = this._copyRectFromPixelData(this._appData.pixelData, sel.x, sel.y, sel.w, sel.h);
    this._appData.selectionClipboard = {
      pixelData: clipPixelData,
      width: clipPixelData.width,
      height: clipPixelData.height,
    };

    if (cut) {
      this._clearRect(this._appData.pixelData, sel.x, sel.y, sel.w, sel.h);
      this._appData.layerData.markCompositeDirty();
      this._beginFloatingSelection(this._appData, clipPixelData, sel.x, sel.y, true);
      this._pixelCanvas.markDirty();
      console.log(`[EditorScene] selection cut: (${sel.x}, ${sel.y}) ${sel.w}x${sel.h}`);
      return;
    }

    console.log(`[EditorScene] selection copied: (${sel.x}, ${sel.y}) ${sel.w}x${sel.h}`);
  }

  /** 現在の選択クリップボードを貼り付ける。 */
  _pasteSelectionClipboard() {
    if (!this._appData?.selectionClipboard?.pixelData || !this._appData.pixelData) {
      console.warn('[EditorScene] 選択クリップボードが空です');
      return;
    }

    if (this._appData.hasFloatingSelection()) {
      this._commitFloatingSelection(this._appData);
    }

    const clipboard = this._appData.selectionClipboard;
    const baseX = this._appData.hasSelection() ? this._appData.selection.x : 0;
    const baseY = this._appData.hasSelection() ? this._appData.selection.y : 0;
    this._beginFloatingSelection(this._appData, clipboard.pixelData, baseX, baseY, false);
    this._pixelCanvas.markDirty();
    console.log(`[EditorScene] selection pasted as floating: (${baseX}, ${baseY}) ${clipboard.width}x${clipboard.height}`);
  }

  /**
   * 浮動選択を開始する。
   * @param {AppData} appData
   * @param {PixelData} pixelData
   * @param {number} x
   * @param {number} y
   * @param {boolean} cut
   */
  _beginFloatingSelection(appData, pixelData, x, y, cut) {
    const floatingPixelData = this._copyRectFromPixelData(pixelData, 0, 0, pixelData.width, pixelData.height);
    appData.selection.active = true;
    appData.selection.x = x;
    appData.selection.y = y;
    appData.selection.w = floatingPixelData.width;
    appData.selection.h = floatingPixelData.height;
    appData.selection.mode = 'rect';
    appData.selection.floating = {
      pixelData: floatingPixelData,
      srcX: x,
      srcY: y,
      dstX: x,
      dstY: y,
      width: floatingPixelData.width,
      height: floatingPixelData.height,
      cut,
    };
  }

  /**
   * 現在の通常選択を浮動選択へ持ち上げる。
   * @param {AppData} appData
   * @returns {boolean}
   */
  _liftSelectionToFloating(appData) {
    if (!appData.hasSelection() || !appData.pixelData) return false;

    const sel = appData.selection;
    const clipPixelData = this._copyRectFromPixelData(appData.pixelData, sel.x, sel.y, sel.w, sel.h);
    this._clearRect(appData.pixelData, sel.x, sel.y, sel.w, sel.h);
    appData.layerData.markCompositeDirty();
    this._beginFloatingSelection(appData, clipPixelData, sel.x, sel.y, true);
    return true;
  }

  /**
   * 浮動選択を現在位置へ確定する。
   * @param {AppData} appData
   */
  _commitFloatingSelection(appData) {
    const floating = appData.selection.floating;
    if (!floating?.pixelData || !appData.pixelData) return;

    const pasted = this._blitPixelData(appData.pixelData, floating.pixelData, floating.dstX, floating.dstY);
    appData.selection.floating = null;
    appData.clearSelection();
    if (pasted.w > 0 && pasted.h > 0) {
      appData.setSelectionRect(pasted.x, pasted.y, pasted.w, pasted.h);
      appData.clearSelection();
    }
    appData.layerData.markCompositeDirty();
    this._selectionDrag = null;
    this._floatingDrag = null;
    this._selectionMovePrimed = null;
    this._pixelCanvas.markDirty();
  }

  /**
   * 浮動選択を取り消す。cut 由来なら元位置に戻す。
   * @param {AppData} appData
   */
  _cancelFloatingSelection(appData) {
    const floating = appData.selection.floating;
    if (!floating?.pixelData || !appData.pixelData) {
      appData.clearSelection();
      return;
    }

    if (floating.cut) {
      this._blitPixelData(appData.pixelData, floating.pixelData, floating.srcX, floating.srcY);
      appData.selection.floating = null;
      appData.clearSelection();
      appData.layerData.markCompositeDirty();
    } else {
      appData.clearSelection();
    }
    this._selectionDrag = null;
    this._floatingDrag = null;
    this._selectionMovePrimed = null;
    this._pixelCanvas.markDirty();
  }

  /**
   * 浮動選択の内容を変形する。
   * @param {AppData} appData
   * @param {'flipH'|'flipV'|'rotate90CW'|'rotate90CCW'} operation
   */
  _transformFloatingSelection(appData, operation) {
    const floating = appData.selection.floating;
    const pixelData = floating?.pixelData;
    if (!pixelData || typeof pixelData[operation] !== 'function') return;

    pixelData[operation]();

    floating.width = pixelData.width;
    floating.height = pixelData.height;
    appData.selection.x = floating.dstX;
    appData.selection.y = floating.dstY;
    appData.selection.w = floating.width;
    appData.selection.h = floating.height;
    this._pixelCanvas.markDirty();
  }

  /**
   * 座標が選択矩形内か判定する。
   * @param {number} px
   * @param {number} py
   * @param {{ x:number, y:number, w:number, h:number }} selection
   * @returns {boolean}
   */
  _isPointInSelection(px, py, selection) {
    return px >= selection.x && px < selection.x + selection.w &&
      py >= selection.y && py < selection.y + selection.h;
  }

  /**
   * PixelData の矩形領域をコピーする。
   * @param {PixelData} source
   * @param {number} x
   * @param {number} y
   * @param {number} w
   * @param {number} h
   * @returns {PixelData}
   */
  _copyRectFromPixelData(source, x, y, w, h) {
    const result = new PixelData();
    result.createPixelData(w, h, 0x00000000);

    for (let iy = 0; iy < h; iy++) {
      for (let ix = 0; ix < w; ix++) {
        result.setPixel(ix, iy, source.getPixel(x + ix, y + iy));
      }
    }

    return result;
  }

  /**
   * PixelData 上の矩形領域を透明でクリアする。
   * @param {PixelData} pixelData
   * @param {number} x
   * @param {number} y
   * @param {number} w
   * @param {number} h
   */
  _clearRect(pixelData, x, y, w, h) {
    for (let iy = 0; iy < h; iy++) {
      for (let ix = 0; ix < w; ix++) {
        pixelData.setPixel(x + ix, y + iy, 0x00000000);
      }
    }
  }

  /**
   * src の内容を dst に貼り付け、実際に貼れた矩形を返す。
   * @param {PixelData} dst
   * @param {PixelData} src
   * @param {number} dstX
   * @param {number} dstY
   * @returns {{ x:number, y:number, w:number, h:number }}
   */
  _blitPixelData(dst, src, dstX, dstY) {
    const startX = Math.max(0, dstX);
    const startY = Math.max(0, dstY);
    const endX = Math.min(dst.width, dstX + src.width);
    const endY = Math.min(dst.height, dstY + src.height);

    for (let y = startY; y < endY; y++) {
      for (let x = startX; x < endX; x++) {
        const sx = x - dstX;
        const sy = y - dstY;
        dst.setPixel(x, y, src.getPixel(sx, sy));
      }
    }

    return {
      x: startX,
      y: startY,
      w: Math.max(0, endX - startX),
      h: Math.max(0, endY - startY),
    };
  }

  /**
   * BFS フラッドフィル (Flood Fill)。
   * @param {PixelData} pixelData
   * @param {number}    startX
   * @param {number}    startY
   * @param {number}    fillColor  0xAARRGGBB
   */
  _floodFill(pixelData, startX, startY, fillColor) {
    const target = pixelData.getPixel(startX, startY);
    if (target === fillColor) return;  // 塗り替え不要

    const w       = pixelData.width;
    const h       = pixelData.height;
    const visited = new Uint8Array(w * h);
    const queue   = [startX + startY * w];
    visited[startX + startY * w] = 1;

    while (queue.length > 0) {
      const idx = queue.pop();  // pop() は shift() より高速
      const x   = idx % w;
      const y   = (idx / w) | 0;
      pixelData.setPixel(x, y, fillColor);

      const neighbors = [
        x + 1 < w ? idx + 1 : -1,
        x - 1 >= 0 ? idx - 1 : -1,
        y + 1 < h ? idx + w : -1,
        y - 1 >= 0 ? idx - w : -1,
      ];
      for (const ni of neighbors) {
        if (ni >= 0 && !visited[ni] && pixelData.getPixel(ni % w, (ni / w) | 0) === target) {
          visited[ni] = 1;
          queue.push(ni);
        }
      }
    }
  }

  /**
   * クォータービュータイル生成確認コールバック
   * @param {Object} params
   */
  _onQuarterViewTileConfirm(params) {
    if (!this._appData) return;

    if (params.target === 'overwrite') {
      // 既存キャンバスのアクティブレイヤーに描画（サイズ変更なし）
      QuarterViewTileGenerator.generate(params, this._appData.layerData);
      this._appData.layerData.markCompositeDirty();
      this._pixelCanvas.markDirty();
    } else {
      // 新規作成
      const ld = QuarterViewTileGenerator.generate(params);
      if (ld) {
        this._appData.editMode    = 'free';
        this._appData.tilesetData = null;
        this._appData._layerData  = ld;
        this._pixelCanvas.markDirty();
        this._pixelCanvas.resetView();
      }
    }
    console.log(`[EditorScene] quarter view tile generated: ${params.type} ${params.width}x${this._appData.layerData.height}`);
  }

  /**
   * アクティブツールに応じてカーソルを更新する。
   * spaceDown 中は何もしない（パン機能が優先）。
   */
  _updateToolCursor() {
    if (!this._canvas || this._spaceDown) return;
    this._canvas.style.cursor = '';
  }
}
