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
        this._appData.createPixelData(width, height, fillColor);
        this._pixelCanvas.resetView();
        this._pixelCanvas.markDirty();
        console.log(`[EditorScene] new file: ${width}x${height} bg=${bgColor}`);
      },
      () => console.log('[EditorScene] new file dialog cancelled'),
    );

    /** カラーパレットウィンドウ @type {ColorPaletteWindow} */
    this._colorPaletteWin = new ColorPaletteWindow(new ColorPalette16());

    /** ツールバーウィンドウ @type {ToolBarWindow} */
    this._toolBarWin = new ToolBarWindow();

    /** レイヤーパネルウィンドウ @type {LayerPanelWindow} */
    this._layerPanelWin = new LayerPanelWindow();
    this._layerPanelWin.onChange = () => {
      this._pixelCanvas.markDirty();
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
      // 今後: 他の id で分岐して各アクションを実装
    };

    // --- キーボード ---
    this._onKeyDown = e => {
      if (this._newFileDialog.isVisible) { this._newFileDialog.onKeyDown(e); return; }
      if (this._saveDialog.isVisible)    { this._saveDialog.onKeyDown(e);    return; }
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
      this._saveDialog.onMouseMove(e);
      this._colorPaletteWin.onMouseMove(e, appData);
      this._toolBarWin.onMouseMove(e, appData);
      this._layerPanelWin.onMouseMove(e, appData);
      if (!this._newFileDialog.isVisible) {
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
      if (this._newFileDialog.isVisible) { this._newFileDialog.onMouseDown(e); return; }
      if (this._saveDialog.isVisible)    { this._saveDialog.onMouseDown(e);    return; }
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
        const consumed = this._colorPaletteWin.onMouseDown(e, appData) ||
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
      if (this._newFileDialog.isVisible) { this._newFileDialog.onMouseUp(e); return; }
      if (this._saveDialog.isVisible)    { this._saveDialog.onMouseUp(e);    return; }
      if (this._pixelCanvas._isPanning) {
        this._pixelCanvas.endPan();
        if (this._canvas) this._canvas.style.cursor = this._spaceDown ? 'grab' : '';
        return;
      }
      this._colorPaletteWin.onMouseUp(e, appData);
      this._toolBarWin.onMouseUp(e, appData);
      this._layerPanelWin.onMouseUp(e, appData);
      this._menuBar.onMouseUp(e);
      this._pixelCanvas.onMouseUp(e, appData);
    };
    this._onWheel = e => {
      if (this._newFileDialog.isVisible) return;
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

    // カラーパレットウィンドウ (ダイアログより前、ピクセルキャンバスの上)
    this._colorPaletteWin.render(ctx, canvas, appData);

    // ダイアログ（メニューバーより前、オーバーレイがメニューを覆う）
    this._newFileDialog.render(ctx, canvas);
    this._saveDialog.render(ctx, canvas);

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
