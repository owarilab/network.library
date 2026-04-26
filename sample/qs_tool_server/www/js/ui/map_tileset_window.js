class MapTilesetWindow extends UIWindow {
  constructor(scene, initX = 756, initY = 46) {
    super(initX, initY);
    this._scene = scene;
    this._hoverTab = -1;
    this._hoverChip = null;
    this._tabRects = [];
    this._chipRects = [];
    this._offscreen = null;
    this._offCtx = null;
  }

  getWindowTitle() {
    return 'Tileset Browser';
  }

  getContentSize() {
    return { w: 300, h: 330 };
  }

  renderContent(ctx, cx, cy, cw, ch, appData) {
    ctx.fillStyle = '#111827';
    ctx.fillRect(cx, cy, cw, ch);

    const project = appData?.currentProject;
    const tilesets = project?.assets?.tilesets || [];
    const mapData = appData?.mapData;
    this._tabRects = [];
    this._chipRects = [];

    if (!tilesets.length) {
      ctx.fillStyle = '#94a3b8';
      ctx.font = '13px sans-serif';
      ctx.textAlign = 'center';
      ctx.textBaseline = 'middle';
      ctx.fillText('No tilesets in this project', cx + cw / 2, cy + ch / 2 - 8);
      ctx.fillText('Create one in the dot editor first', cx + cw / 2, cy + ch / 2 + 14);
      return;
    }

    const activeTileset = this._scene.getActiveTilesetAsset(appData);
    const tabPad = 8;
    const tabY = cy + 8;
    const tabH = 24;
    let tabX = cx + 8;
    for (let index = 0; index < tilesets.length; index++) {
      const asset = tilesets[index];
      const tabW = Math.min(110, Math.max(72, asset.name.length * 8 + 18));
      const isActive = activeTileset?.id === asset.id;
      const isHover = this._hoverTab === index;
      this._tabRects.push({ x: tabX, y: tabY, w: tabW, h: tabH, index, tilesetId: asset.id });
      ctx.fillStyle = isActive ? '#2563eb' : isHover ? '#1d4ed8' : '#1f2937';
      ctx.strokeStyle = isActive ? '#93c5fd' : '#334155';
      ctx.lineWidth = 1.5;
      ctx.beginPath();
      ctx.roundRect(tabX, tabY, tabW, tabH, 8);
      ctx.fill();
      ctx.stroke();
      ctx.fillStyle = '#e5e7eb';
      ctx.font = '12px sans-serif';
      ctx.textAlign = 'center';
      ctx.textBaseline = 'middle';
      ctx.fillText(asset.name.slice(0, 14), tabX + tabW / 2, tabY + tabH / 2);
      tabX += tabW + tabPad;
      if (tabX > cx + cw - 90) break;
    }

    if (!activeTileset?.tilesetData || !mapData) return;

    const td = activeTileset.tilesetData;
    const cellSize = Math.max(24, Math.min(44, Math.floor((cw - 28) / Math.max(1, td.columns)) - 4));
    const gap = 4;
    const startX = cx + 12;
    const startY = cy + 42;
    const selected = mapData.selectedTileRef;
    for (let row = 0; row < td.rows; row++) {
      for (let col = 0; col < td.columns; col++) {
        const x = startX + col * (cellSize + gap);
        const y = startY + row * (cellSize + gap);
        if (y + cellSize > cy + ch - 34) continue;
        const tileIndex = row * td.columns + col;
        this._chipRects.push({ x, y, w: cellSize, h: cellSize, col, row, tileIndex, tilesetId: activeTileset.id });
        ctx.fillStyle = '#0b1220';
        ctx.fillRect(x, y, cellSize, cellSize);
        const chipPd = td.compositeChip(col, row);
        if (chipPd) this._drawPixelDataScaled(ctx, chipPd, x, y, cellSize, cellSize);
        const isHover = this._hoverChip && this._hoverChip.col === col && this._hoverChip.row === row;
        const isSelected = selected && selected.tilesetId === activeTileset.id && selected.index === tileIndex;
        ctx.strokeStyle = isSelected ? '#f59e0b' : isHover ? '#93c5fd' : '#334155';
        ctx.lineWidth = isSelected ? 2.5 : 1.5;
        ctx.strokeRect(x + 0.5, y + 0.5, cellSize - 1, cellSize - 1);
      }
    }

    ctx.fillStyle = '#94a3b8';
    ctx.font = '12px sans-serif';
    ctx.textAlign = 'left';
    ctx.textBaseline = 'middle';
    const label = selected && selected.tilesetId === activeTileset.id
      ? `Selected tile #${selected.index} (${selected.col}, ${selected.row})`
      : 'Select a tile to paint into the map';
    ctx.fillText(label, cx + 12, cy + ch - 16);
  }

  onContentMouseMove(e) {
    this._hoverTab = this._tabRects.findIndex(rect => this._inRect(e.x, e.y, rect));
    const hit = this._chipRects.find(rect => this._inRect(e.x, e.y, rect));
    this._hoverChip = hit ? { col: hit.col, row: hit.row } : null;
  }

  onContentMouseDown(e, appData) {
    const tab = this._tabRects.find(rect => this._inRect(e.x, e.y, rect));
    if (tab) {
      this._scene.selectTileset(tab.tilesetId, appData);
      return true;
    }
    const chip = this._chipRects.find(rect => this._inRect(e.x, e.y, rect));
    if (!chip) return false;
    this._scene.selectPaintTile({
      tilesetId: chip.tilesetId,
      col: chip.col,
      row: chip.row,
      index: chip.tileIndex,
    }, appData);
    return true;
  }

  _inRect(x, y, rect) {
    return x >= rect.x && y >= rect.y && x < rect.x + rect.w && y < rect.y + rect.h;
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