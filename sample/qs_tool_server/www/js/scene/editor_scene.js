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
      console.log(`[EditorScene] pixel up (${px}, ${py}) button=${button}`);
    };

    /** @type {NewFileDialog} */
    this._newFileDialog = new NewFileDialog(
      (width, height, bgColor) => {
        if (!this._appData) return;
        const fillColor = bgColor === 'white'
          ? PixelData.rgba(255, 255, 255, 255)
          : 0x00000000;
        this._appData.editMode    = 'free';
        this._appData.tilesetData = null;
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
        this._pixelCanvas.resetView();
        this._pixelCanvas.markDirty();
        console.log(`[EditorScene] new tileset: ${chipW}x${chipH} chip, ${cols}x${rows} grid, bg=${bgColor}`);
      },
      () => console.log('[EditorScene] new tileset dialog cancelled'),
    );

    /** カラーパレットウィンドウ @type {ColorPaletteWindow} */
    this._colorPaletteWin = new ColorPaletteWindow(new ColorPalette16());

    /** カラーピッカーダイアログ @type {ColorPickerDialog} */
    this._colorPickerDialog = new ColorPickerDialog(
      (color) => {
        if (!this._appData) return;
        if (this._colorPickerTarget === 'back') {
          this._appData.backColor = color;
        } else {
          this._appData.foreColor = color;
        }
        console.log(`[EditorScene] color picked: 0x${color.toString(16).padStart(8, '0')}`);
      },
      () => console.log('[EditorScene] color picker cancelled'),
    );
    /** @type {'fore' | 'back'} */
    this._colorPickerTarget = 'fore';

    // パレットのスウォッチクリックでカラーピッカーを開く
    this._colorPaletteWin.getPalette().onSwatchClick = (target, appData) => {
      this._colorPickerTarget = target;
      const color = target === 'back' ? appData.backColor : appData.foreColor;
      this._colorPickerDialog.showWithColor(color);
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
      this._pixelCanvas.markDirty();
      console.log(`[EditorScene] chip selected: (${col}, ${row})`);
    };
    this._chipPaletteWin.onChipDoubleClick = (col, row) => {
      if (!this._appData) return;
      this._appData.selectedChip = { col, row };
      this._pixelCanvas.resetView();
      this._pixelCanvas.markDirty();
      console.log(`[EditorScene] chip double-clicked (focus): (${col}, ${row})`);
    };

    /** エクスポートダイアログ @type {SaveDialog} */
    this._saveDialog = new SaveDialog(
      (filename, format) => {
        if (!this._appData?.pixelData?.pixels) return;
        if (format === 'png') {
          // PNG は全レイヤー合成結果をエクスポート
          const composited = this._appData.layerData.composite();
          PixelDataConverter.exportAsPng(composited, filename)
            .catch(err => console.error('[EditorScene] PNG エクスポートエラー:', err));
        } else {
          PixelDataConverter.exportAsJson(this._appData.pixelData, filename);
        }
        console.log(`[EditorScene] export: ${filename} (${format})`);
      },
      () => console.log('[EditorScene] export cancelled'),
    );

    /** チップ入れ替え用: 入れ替え元チップ座標 (null = 未選択) @type {{ col: number, row: number }|null} */
    this._swapSource = null;

    /** onEnter でセットされる共有データへの参照 */
    this._appData = null;

    /** スペースキー押下中フラグ（パンモード） */
    this._spaceDown = false;

    /** render() で受け取った canvas を保持（カーソル変更に使用） */
    this._canvas = null;

    /** ファイル入力用の隠し <input type="file"> (onEnter で生成) */
    this._fileInput = null;
  }

  /**
   * シーン開始時に入力ハンドラを登録する
   * @param {Input}   input
   * @param {AppData} appData
   */
  onEnter(input, appData) {
    this._appData = appData;

    // --- 隠し <input type="file"> を作成 ---
    this._fileInput = document.createElement('input');
    this._fileInput.type    = 'file';
    this._fileInput.accept  = '.png,.json';
    this._fileInput.style.display = 'none';
    document.body.appendChild(this._fileInput);
    this._fileInput.addEventListener('change', () => {
      const file = this._fileInput.files?.[0];
      if (!file) return;
      // 次回同一ファイルを選んでも change が発火されるようにリセット
      this._fileInput.value = '';
      PixelDataConverter.importFromFile(file)
        .then(pd => {
          this._appData.editMode    = 'free';
          this._appData.tilesetData = null;
          this._appData.pixelData = pd;
          this._pixelCanvas.resetView();
          this._pixelCanvas.markDirty();
          console.log(`[EditorScene] import: ${file.name} → ${pd.width}x${pd.height}`);
        })
        .catch(err => {
          console.error('[EditorScene] インポートエラー:', err.message);
        });
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
      if (id === MenuConstants.FILE_SAVE) {
        if (this._appData?.pixelData?.pixels) {
          this._saveDialog.show();
        } else {
          console.warn('[EditorScene] エクスポート: ファイルが作成されていません');
        }
        return;
      }
      if (id === MenuConstants.VIEW_GRID) {
        this._pixelCanvas.showGrid = !this._pixelCanvas.showGrid;
        console.log('[EditorScene] grid:', this._pixelCanvas.showGrid);
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

      // 今後: 他の id で分岐して各アクションを実装
    };

    // --- キーボード ---
    this._onKeyDown = e => {
      if (this._newFileDialog.isVisible)     { this._newFileDialog.onKeyDown(e);     return; }
      if (this._newTilesetDialog.isVisible)  { this._newTilesetDialog.onKeyDown(e);  return; }
      if (this._saveDialog.isVisible)        { this._saveDialog.onKeyDown(e);        return; }
      if (this._colorPickerDialog.isVisible) { this._colorPickerDialog.onKeyDown(e); return; }
      if (e.key === ' ') {
        e.preventDefault?.();
        if (!this._spaceDown) {
          this._spaceDown = true;
          if (this._canvas) this._canvas.style.cursor = 'grab';
        }
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
      this._newFileDialog.onMouseMove(e);
      this._newTilesetDialog.onMouseMove(e);
      this._saveDialog.onMouseMove(e);
      this._colorPickerDialog.onMouseMove(e);
      this._colorPaletteWin.onMouseMove(e, appData);
      this._toolBarWin.onMouseMove(e, appData);
      this._layerPanelWin.onMouseMove(e, appData);
      this._chipPaletteWin.onMouseMove(e, appData);
      if (this._tilePreviewVisible) this._tilePreviewWin.onMouseMove(e, appData);
      if (!this._newFileDialog.isVisible && !this._newTilesetDialog.isVisible) {
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
      if (this._newFileDialog.isVisible)     { this._newFileDialog.onMouseDown(e);     return; }
      if (this._newTilesetDialog.isVisible)  { this._newTilesetDialog.onMouseDown(e);  return; }
      if (this._saveDialog.isVisible)        { this._saveDialog.onMouseDown(e);        return; }
      if (this._colorPickerDialog.isVisible) { this._colorPickerDialog.onMouseDown(e); return; }
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
      if (this._newFileDialog.isVisible)     { this._newFileDialog.onMouseUp(e);     return; }
      if (this._newTilesetDialog.isVisible)  { this._newTilesetDialog.onMouseUp(e);  return; }
      if (this._saveDialog.isVisible)        { this._saveDialog.onMouseUp(e);        return; }
      if (this._colorPickerDialog.isVisible) { this._colorPickerDialog.onMouseUp(e); return; }
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
      if (this._newFileDialog.isVisible || this._newTilesetDialog.isVisible || this._saveDialog.isVisible || this._colorPickerDialog.isVisible) return;
      this._pixelCanvas.zoom(-e.deltaY, e.x, e.y);
    };
    this._onContextMenu = e => console.log('[EditorScene] contextmenu', e);
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
    this._pixelCanvas.render(ctx, canvas, appData.layerData.composite());

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
    this._saveDialog.render(ctx, canvas);
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
    const tool      = appData.activeTool;
    const drawColor = button === 2 ? appData.backColor : appData.foreColor;

    switch (tool) {
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
   * アクティブツールに応じてカーソルを更新する。
   * spaceDown 中は何もしない（パン機能が優先）。
   */
  _updateToolCursor() {
    if (!this._canvas || this._spaceDown) return;
    this._canvas.style.cursor = '';
  }
}
