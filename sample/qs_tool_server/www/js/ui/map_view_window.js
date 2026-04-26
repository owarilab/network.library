class MapViewWindow extends UIWindow {
  static ZOOM_STEPS = [1, 2, 3, 4, 5, 6, 8];
  static VISIBLE_OVERSCAN = 1;

  constructor(scene, initX = 16, initY = 46) {
    super(initX, initY);
    this._scene = scene;
    this._hoverCell = null;
    this._gridRect = null;
    this._viewportRect = null;
    this._offscreen = null;
    this._offCtx = null;
    this._isPanning = false;
    this._panStartX = 0;
    this._panStartY = 0;
    this._panOriginX = 0;
    this._panOriginY = 0;
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
      this._viewportRect = null;
      ctx.fillStyle = '#94a3b8';
      ctx.font = '14px sans-serif';
      ctx.textAlign = 'center';
      ctx.textBaseline = 'middle';
      ctx.fillText('No map data loaded', cx + cw / 2, cy + ch / 2);
      return;
    }

    const metrics = this._getViewportMetrics(cx, cy, cw, ch, mapData);
    const gridRect = metrics.gridRect;
    const viewportRect = metrics.viewportRect;
    this._gridRect = gridRect;
    this._viewportRect = viewportRect;

    ctx.fillStyle = '#020617';
    ctx.fillRect(viewportRect.x, viewportRect.y, viewportRect.w, viewportRect.h);

    ctx.save();
    ctx.beginPath();
    ctx.rect(viewportRect.x, viewportRect.y, viewportRect.w, viewportRect.h);
    ctx.clip();

    const tilesetAsset = this._scene.getActiveTilesetAsset(appData);
    const layer = mapData.layers[mapData.selectedLayer] || mapData.layers[0] || null;
    const visible = this._getVisibleCellRange(metrics, mapData);
    if (layer) {
      for (let row = visible.startRow; row <= visible.endRow; row++) {
        for (let col = visible.startCol; col <= visible.endCol; col++) {
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
      for (let col = visible.startCol; col <= visible.endCol + 1; col++) {
        const lineX = gridRect.x + col * gridRect.cellW + 0.5;
        ctx.beginPath();
        ctx.moveTo(lineX, viewportRect.y);
        ctx.lineTo(lineX, viewportRect.y + viewportRect.h);
        ctx.stroke();
      }
      for (let row = visible.startRow; row <= visible.endRow + 1; row++) {
        const lineY = gridRect.y + row * gridRect.cellH + 0.5;
        ctx.beginPath();
        ctx.moveTo(viewportRect.x, lineY);
        ctx.lineTo(viewportRect.x + viewportRect.w, lineY);
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

    ctx.restore();

    ctx.strokeStyle = '#334155';
    ctx.lineWidth = 1;
    ctx.strokeRect(viewportRect.x + 0.5, viewportRect.y + 0.5, viewportRect.w - 1, viewportRect.h - 1);

    ctx.fillStyle = '#94a3b8';
    ctx.font = '12px sans-serif';
    ctx.textAlign = 'left';
    ctx.textBaseline = 'middle';
    ctx.fillText(
      `Layer: ${(layer?.name || 'n/a')}  /  Grid: ${mapData.width}x${mapData.height}  /  Tile: ${mapData.tileWidth}x${mapData.tileHeight}  /  Zoom: ${this.getZoomLabel(appData)}`,
      cx + 10,
      cy + ch - 14,
    );
  }

  onContentMouseMove(e, appData) {
    if (this._isPanning) {
      this.movePan(e.x, e.y, appData?.mapData, this._viewportRect);
      this._hoverCell = null;
      return;
    }
    this._hoverCell = this._resolveCellFromPoint(e.x, e.y, appData?.mapData);
  }

  onContentMouseDown(e, appData) {
    if (this._shouldStartPan(e)) {
      this.startPan(e.x, e.y, appData?.mapData, this._viewportRect);
      this._hoverCell = null;
      return true;
    }
    const cell = this._resolveCellFromPoint(e.x, e.y, appData?.mapData);
    if (!cell || !appData?.mapData) return false;
    appData.mapData.cursor = { x: cell.col, y: cell.row };
    this._scene.applyMapCellAction(cell.col, cell.row, e.button, appData);
    return true;
  }

  onContentMouseUp(e, appData) {
    if (!this._isPanning) return;
    this.endPan(appData?.mapData, this._viewportRect);
  }

  zoomAt(delta, sx, sy, appData) {
    const mapData = appData?.mapData;
    if (!mapData) return false;
    const metrics = this._getViewportMetricsFromCache(mapData);
    if (!metrics) return false;

    const view = this._ensureView(mapData);
    const steps = MapViewWindow.ZOOM_STEPS;
    let index = steps.indexOf(view.zoom);
    if (index < 0) {
      index = steps.findIndex(step => step >= view.zoom);
      if (index < 0) index = steps.length - 1;
      else if (steps[index] > view.zoom && index > 0) index -= 1;
    }
    const nextIndex = delta > 0
      ? Math.min(steps.length - 1, index + 1)
      : Math.max(0, index - 1);
    const nextZoom = steps[nextIndex];
    if (nextZoom === view.zoom) return false;

    const anchorX = (sx >= metrics.viewportRect.x && sx < metrics.viewportRect.x + metrics.viewportRect.w)
      ? sx
      : metrics.viewportRect.x + metrics.viewportRect.w / 2;
    const anchorY = (sy >= metrics.viewportRect.y && sy < metrics.viewportRect.y + metrics.viewportRect.h)
      ? sy
      : metrics.viewportRect.y + metrics.viewportRect.h / 2;
    const worldX = (anchorX - metrics.gridRect.x) / metrics.gridRect.cellW;
    const worldY = (anchorY - metrics.gridRect.y) / metrics.gridRect.cellH;

    view.zoom = nextZoom;
    const nextMetrics = this._buildViewportMetrics(metrics.viewportRect, mapData, view.zoom, view.panX, view.panY);
    view.panX = Math.round(anchorX - nextMetrics.baseX - worldX * nextMetrics.cellW);
    view.panY = Math.round(anchorY - nextMetrics.baseY - worldY * nextMetrics.cellH);
    this._clampViewPan(view, metrics.viewportRect, nextMetrics.mapW, nextMetrics.mapH);
    appData.projectSession?.markDirty();
    return true;
  }

  startPan(sx, sy, mapData, viewportRect = this._viewportRect) {
    if (!mapData || !viewportRect) return false;
    const view = this._ensureView(mapData);
    const mapSize = this._getMapPixelSize(mapData, view.zoom);
    this._clampViewPan(view, viewportRect, mapSize.w, mapSize.h);
    this._isPanning = true;
    this._panStartX = sx;
    this._panStartY = sy;
    this._panOriginX = view.panX;
    this._panOriginY = view.panY;
    return true;
  }

  movePan(sx, sy, mapData, viewportRect = this._viewportRect) {
    if (!this._isPanning || !mapData || !viewportRect) return false;
    const view = this._ensureView(mapData);
    view.panX = this._panOriginX + (sx - this._panStartX);
    view.panY = this._panOriginY + (sy - this._panStartY);
    const mapSize = this._getMapPixelSize(mapData, view.zoom);
    this._clampViewPan(view, viewportRect, mapSize.w, mapSize.h);
    return true;
  }

  endPan(mapData, viewportRect = this._viewportRect) {
    this._isPanning = false;
    if (!mapData || !viewportRect) return false;
    const view = this._ensureView(mapData);
    const mapSize = this._getMapPixelSize(mapData, view.zoom);
    this._clampViewPan(view, viewportRect, mapSize.w, mapSize.h);
    return true;
  }

  resetView(appData) {
    const mapData = appData?.mapData;
    if (!mapData) return false;
    const view = this._ensureView(mapData);
    view.zoom = 2;
    view.panX = 0;
    view.panY = 0;
    appData.projectSession?.markDirty();
    return true;
  }

  clampView(appData) {
    const mapData = appData?.mapData;
    if (!mapData || !this._viewportRect) return false;
    const view = this._ensureView(mapData);
    const mapSize = this._getMapPixelSize(mapData, view.zoom);
    this._clampViewPan(view, this._viewportRect, mapSize.w, mapSize.h);
    return true;
  }

  getZoomLabel(appData) {
    const zoom = this._ensureView(appData?.mapData).zoom;
    return `${zoom}x`;
  }

  containsViewportPoint(x, y) {
    const rect = this._viewportRect;
    return !!rect && x >= rect.x && x < rect.x + rect.w && y >= rect.y && y < rect.y + rect.h;
  }

  _resolveCellFromPoint(x, y, mapData) {
    const rect = this._gridRect;
    const viewportRect = this._viewportRect;
    if (!rect || !mapData) return null;
    if (!viewportRect || x < viewportRect.x || y < viewportRect.y || x >= viewportRect.x + viewportRect.w || y >= viewportRect.y + viewportRect.h) return null;
    if (x < rect.x || y < rect.y || x >= rect.x + rect.w || y >= rect.y + rect.h) return null;
    return {
      col: Math.max(0, Math.min(mapData.width - 1, ((x - rect.x) / rect.cellW) | 0)),
      row: Math.max(0, Math.min(mapData.height - 1, ((y - rect.y) / rect.cellH) | 0)),
    };
  }

  _getViewportMetrics(cx, cy, cw, ch, mapData) {
    const viewportRect = this._getViewportRect(cx, cy, cw, ch);
    const view = this._ensureView(mapData);
    return this._buildViewportMetrics(viewportRect, mapData, view.zoom, view.panX, view.panY);
  }

  _getViewportMetricsFromCache(mapData) {
    if (!this._viewportRect) return null;
    const view = this._ensureView(mapData);
    return this._buildViewportMetrics(this._viewportRect, mapData, view.zoom, view.panX, view.panY);
  }

  _getViewportRect(cx, cy, cw, ch) {
    const padX = 12;
    const padTop = 12;
    const padBottom = 32;
    return {
      x: cx + padX,
      y: cy + padTop,
      w: cw - padX * 2,
      h: ch - padTop - padBottom,
    };
  }

  _buildViewportMetrics(viewportRect, mapData, zoom, panX, panY) {
    const cellW = Math.max(8, Math.floor((mapData.tileWidth | 0 || 16) * Math.max(1, zoom | 0 || 2)));
    const cellH = Math.max(8, Math.floor((mapData.tileHeight | 0 || 16) * Math.max(1, zoom | 0 || 2)));
    const mapW = cellW * Math.max(1, mapData.width | 0);
    const mapH = cellH * Math.max(1, mapData.height | 0);
    const baseX = viewportRect.x + (mapW < viewportRect.w ? ((viewportRect.w - mapW) / 2) | 0 : 0);
    const baseY = viewportRect.y + (mapH < viewportRect.h ? ((viewportRect.h - mapH) / 2) | 0 : 0);
    const view = this._ensureView(mapData);
    view.zoom = Math.max(1, zoom | 0 || view.zoom);
    view.panX = panX | 0;
    view.panY = panY | 0;
    this._clampViewPan(view, viewportRect, mapW, mapH);
    return {
      viewportRect,
      gridRect: {
        x: baseX + view.panX,
        y: baseY + view.panY,
        w: mapW,
        h: mapH,
        cellW,
        cellH,
      },
      baseX,
      baseY,
      cellW,
      cellH,
      mapW,
      mapH,
    };
  }

  _getVisibleCellRange(metrics, mapData) {
    const { viewportRect, gridRect, cellW, cellH } = metrics;
    const overscan = MapViewWindow.VISIBLE_OVERSCAN;
    const right = viewportRect.x + viewportRect.w - 1;
    const bottom = viewportRect.y + viewportRect.h - 1;
    return {
      startCol: Math.max(0, Math.min(mapData.width - 1, Math.floor((viewportRect.x - gridRect.x) / cellW) - overscan)),
      endCol: Math.max(0, Math.min(mapData.width - 1, Math.floor((right - gridRect.x) / cellW) + overscan)),
      startRow: Math.max(0, Math.min(mapData.height - 1, Math.floor((viewportRect.y - gridRect.y) / cellH) - overscan)),
      endRow: Math.max(0, Math.min(mapData.height - 1, Math.floor((bottom - gridRect.y) / cellH) + overscan)),
    };
  }

  _getMapPixelSize(mapData, zoom) {
    const cellW = Math.max(8, Math.floor((mapData.tileWidth | 0 || 16) * Math.max(1, zoom | 0 || 2)));
    const cellH = Math.max(8, Math.floor((mapData.tileHeight | 0 || 16) * Math.max(1, zoom | 0 || 2)));
    return {
      w: Math.max(1, mapData.width | 0) * cellW,
      h: Math.max(1, mapData.height | 0) * cellH,
    };
  }

  _ensureView(mapData) {
    if (!mapData) return { zoom: 2, panX: 0, panY: 0 };
    if (!mapData.view) mapData.view = {};
    if (!Number.isFinite(mapData.view.zoom) || mapData.view.zoom < 1) mapData.view.zoom = 2;
    if (!Number.isFinite(mapData.view.panX)) mapData.view.panX = 0;
    if (!Number.isFinite(mapData.view.panY)) mapData.view.panY = 0;
    if (!Number.isFinite(mapData.view.minZoom)) mapData.view.minZoom = MapViewWindow.ZOOM_STEPS[0];
    if (!Number.isFinite(mapData.view.maxZoom)) mapData.view.maxZoom = MapViewWindow.ZOOM_STEPS[MapViewWindow.ZOOM_STEPS.length - 1];
    return mapData.view;
  }

  _clampViewPan(view, viewportRect, mapW, mapH) {
    if (!view || !viewportRect) return;
    if (mapW <= viewportRect.w) view.panX = 0;
    else view.panX = Math.max(viewportRect.w - mapW, Math.min(0, view.panX | 0));
    if (mapH <= viewportRect.h) view.panY = 0;
    else view.panY = Math.max(viewportRect.h - mapH, Math.min(0, view.panY | 0));
  }

  _shouldStartPan(e) {
    return e.button === 1 || (e.button === 0 && this._scene?.isPanModifierActive?.());
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