class MapViewWindow extends UIWindow {
  constructor(scene, initX = 16, initY = 46) {
    super(initX, initY);
    this._scene = scene;
    this._hoverCell = null;
    this._gridRect = null;
    this._offscreen = null;
    this._offCtx = null;
  }

  getWindowTitle() {
    return 'Map View';
  }

  getContentSize() {
    return { w: 720, h: 470 };
  }

  renderContent(ctx, cx, cy, cw, ch, appData) {
    ctx.fillStyle = '#0f172a';
    ctx.fillRect(cx, cy, cw, ch);

    const mapData = appData?.mapData;
    if (!mapData) {
      this._gridRect = null;
      ctx.fillStyle = '#94a3b8';
      ctx.font = '14px sans-serif';
      ctx.textAlign = 'center';
      ctx.textBaseline = 'middle';
      ctx.fillText('No map data loaded', cx + cw / 2, cy + ch / 2);
      return;
    }

    const gridRect = this._getGridRect(cx, cy, cw, ch, mapData);
    this._gridRect = gridRect;

    ctx.fillStyle = '#020617';
    ctx.fillRect(gridRect.x, gridRect.y, gridRect.w, gridRect.h);

    const tilesetAsset = this._scene.getActiveTilesetAsset(appData);
    const layer = mapData.layers[mapData.selectedLayer] || mapData.layers[0] || null;
    if (layer) {
      for (let row = 0; row < mapData.height; row++) {
        for (let col = 0; col < mapData.width; col++) {
          const cellX = gridRect.x + col * gridRect.cellW;
          const cellY = gridRect.y + row * gridRect.cellH;
          const tileIndex = layer.tiles[row * mapData.width + col] | 0;
          if (tileIndex >= 0 && tilesetAsset?.tilesetData) {
            this._drawTileIndex(ctx, tilesetAsset.tilesetData, tileIndex, cellX, cellY, gridRect.cellW, gridRect.cellH);
          } else {
            ctx.fillStyle = ((row + col) % 2 === 0) ? '#111827' : '#0b1220';
            ctx.fillRect(cellX, cellY, gridRect.cellW, gridRect.cellH);
          }
        }
      }
    }

    if (mapData.view?.showGrid !== false) {
      ctx.strokeStyle = 'rgba(148, 163, 184, 0.18)';
      ctx.lineWidth = 1;
      for (let col = 0; col <= mapData.width; col++) {
        const lineX = gridRect.x + col * gridRect.cellW + 0.5;
        ctx.beginPath();
        ctx.moveTo(lineX, gridRect.y);
        ctx.lineTo(lineX, gridRect.y + gridRect.h);
        ctx.stroke();
      }
      for (let row = 0; row <= mapData.height; row++) {
        const lineY = gridRect.y + row * gridRect.cellH + 0.5;
        ctx.beginPath();
        ctx.moveTo(gridRect.x, lineY);
        ctx.lineTo(gridRect.x + gridRect.w, lineY);
        ctx.stroke();
      }
    }

    if (this._hoverCell) {
      ctx.strokeStyle = '#38bdf8';
      ctx.lineWidth = 2;
      ctx.strokeRect(
        gridRect.x + this._hoverCell.col * gridRect.cellW + 1,
        gridRect.y + this._hoverCell.row * gridRect.cellH + 1,
        gridRect.cellW - 2,
        gridRect.cellH - 2,
      );
    }

    const cursor = mapData.cursor || { x: 0, y: 0 };
    if (cursor.x >= 0 && cursor.x < mapData.width && cursor.y >= 0 && cursor.y < mapData.height) {
      ctx.strokeStyle = '#f8fafc';
      ctx.lineWidth = 2;
      ctx.strokeRect(
        gridRect.x + cursor.x * gridRect.cellW + 3,
        gridRect.y + cursor.y * gridRect.cellH + 3,
        gridRect.cellW - 6,
        gridRect.cellH - 6,
      );
    }

    ctx.fillStyle = '#94a3b8';
    ctx.font = '12px sans-serif';
    ctx.textAlign = 'left';
    ctx.textBaseline = 'middle';
    ctx.fillText(
      `Layer: ${(layer?.name || 'n/a')}  /  Grid: ${mapData.width}x${mapData.height}  /  Tile: ${mapData.tileWidth}x${mapData.tileHeight}`,
      cx + 10,
      cy + ch - 14,
    );
  }

  onContentMouseMove(e, appData) {
    this._hoverCell = this._resolveCellFromPoint(e.x, e.y, appData?.mapData);
  }

  onContentMouseDown(e, appData) {
    const cell = this._resolveCellFromPoint(e.x, e.y, appData?.mapData);
    if (!cell || !appData?.mapData) return false;
    appData.mapData.cursor = { x: cell.col, y: cell.row };
    this._scene.applyMapCellAction(cell.col, cell.row, e.button, appData);
    return true;
  }

  _resolveCellFromPoint(x, y, mapData) {
    const rect = this._gridRect;
    if (!rect || !mapData) return null;
    if (x < rect.x || y < rect.y || x >= rect.x + rect.w || y >= rect.y + rect.h) return null;
    return {
      col: Math.max(0, Math.min(mapData.width - 1, ((x - rect.x) / rect.cellW) | 0)),
      row: Math.max(0, Math.min(mapData.height - 1, ((y - rect.y) / rect.cellH) | 0)),
    };
  }

  _getGridRect(cx, cy, cw, ch, mapData) {
    const padX = 12;
    const padTop = 12;
    const padBottom = 32;
    const availW = cw - padX * 2;
    const availH = ch - padTop - padBottom;
    const zoom = Math.max(1, mapData.view?.zoom | 0 || 2);
    const baseCellW = Math.max(10, mapData.tileWidth * zoom);
    const baseCellH = Math.max(10, mapData.tileHeight * zoom);
    const fitScale = Math.min(1, availW / (mapData.width * baseCellW), availH / (mapData.height * baseCellH));
    const cellW = Math.max(8, Math.floor(baseCellW * fitScale));
    const cellH = Math.max(8, Math.floor(baseCellH * fitScale));
    const w = cellW * mapData.width;
    const h = cellH * mapData.height;
    return {
      x: cx + ((cw - w) / 2) | 0,
      y: cy + ((availH - h) / 2 + padTop / 2) | 0,
      w,
      h,
      cellW,
      cellH,
    };
  }

  _drawTileIndex(ctx, tilesetData, tileIndex, dx, dy, dw, dh) {
    const col = tileIndex % tilesetData.columns;
    const row = (tileIndex / tilesetData.columns) | 0;
    const chipPd = tilesetData.compositeChip(col, row);
    if (!chipPd) return;
    this._drawPixelDataScaled(ctx, chipPd, dx, dy, dw, dh);
  }

  _drawPixelDataScaled(ctx, pixelData, dx, dy, dw, dh) {
    const sw = pixelData.width;
    const sh = pixelData.height;
    if (sw === 0 || sh === 0) return;

    if (!this._offscreen) {
      this._offscreen = document.createElement('canvas');
      this._offCtx = this._offscreen.getContext('2d');
    }
    this._offscreen.width = sw;
    this._offscreen.height = sh;

    const imageData = this._offCtx.createImageData(sw, sh);
    const data = imageData.data;
    const pixels = pixelData.pixels;
    for (let i = 0; i < pixels.length; i++) {
      const color = pixels[i];
      const j = i * 4;
      data[j] = (color >>> 16) & 0xFF;
      data[j + 1] = (color >>> 8) & 0xFF;
      data[j + 2] = color & 0xFF;
      data[j + 3] = (color >>> 24) & 0xFF;
    }
    this._offCtx.putImageData(imageData, 0, 0);
    ctx.save();
    ctx.imageSmoothingEnabled = false;
    ctx.drawImage(this._offscreen, 0, 0, sw, sh, dx, dy, dw, dh);
    ctx.restore();
  }
}