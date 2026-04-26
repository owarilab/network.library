class MapInspectorWindow extends UIWindow {
  constructor(scene, initX = 756, initY = 388) {
    super(initX, initY);
    this._scene = scene;
    this._hoverButton = '';
    this._gridButtonRect = null;
    this._backButtonRect = null;
  }

  getWindowTitle() {
    return 'Map Inspector';
  }

  getContentSize() {
    return { w: 300, h: 170 };
  }

  renderContent(ctx, cx, cy, cw, ch, appData) {
    ctx.fillStyle = '#111827';
    ctx.fillRect(cx, cy, cw, ch);

    const project = appData?.currentProject;
    const asset = appData?.getActiveProjectAsset?.() || null;
    const mapData = appData?.mapData;
    const selectedTile = mapData?.selectedTileRef;
    const cursor = mapData?.cursor || { x: 0, y: 0 };
    const activeTileset = this._scene.getActiveTilesetAsset(appData);

    const lines = [
      `Project: ${project?.name || 'n/a'}`,
      `Map: ${asset?.name || 'n/a'}`,
      `Map Size: ${mapData?.width | 0} x ${mapData?.height | 0} cells`,
      `Tile Size: ${mapData?.tileWidth | 0} x ${mapData?.tileHeight | 0}px`,
      `Cursor: (${cursor.x | 0}, ${cursor.y | 0})`,
      selectedTile
        ? `Paint Tile: #${selectedTile.index} (${selectedTile.col}, ${selectedTile.row})`
        : 'Paint Tile: none',
      `Tileset: ${activeTileset?.name || 'none'}`,
    ];

    ctx.fillStyle = '#e5e7eb';
    ctx.font = '13px sans-serif';
    ctx.textAlign = 'left';
    ctx.textBaseline = 'top';
    for (let index = 0; index < lines.length; index++) {
      ctx.fillText(lines[index], cx + 12, cy + 12 + index * 18);
    }

    ctx.fillStyle = '#94a3b8';
    ctx.font = '12px sans-serif';
    ctx.fillText('LMB: paint  /  RMB: clear  /  G: toggle grid  /  Esc: project top', cx + 12, cy + 138);

    this._gridButtonRect = { x: cx + 12, y: cy + ch - 34, w: 110, h: 22 };
    this._backButtonRect = { x: cx + cw - 106, y: cy + ch - 34, w: 94, h: 22 };
    this._drawButton(ctx, this._gridButtonRect, 'Toggle Grid', this._hoverButton === 'grid');
    this._drawButton(ctx, this._backButtonRect, 'Back', this._hoverButton === 'back');
  }

  onContentMouseMove(e) {
    if (this._gridButtonRect && this._inRect(e.x, e.y, this._gridButtonRect)) {
      this._hoverButton = 'grid';
      return;
    }
    if (this._backButtonRect && this._inRect(e.x, e.y, this._backButtonRect)) {
      this._hoverButton = 'back';
      return;
    }
    this._hoverButton = '';
  }

  onContentMouseDown(e, appData) {
    if (this._gridButtonRect && this._inRect(e.x, e.y, this._gridButtonRect)) {
      this._scene.toggleGrid(appData);
      return true;
    }
    if (this._backButtonRect && this._inRect(e.x, e.y, this._backButtonRect)) {
      this._scene.backToProjectTop(appData);
      return true;
    }
    return false;
  }

  _drawButton(ctx, rect, label, hovered) {
    ctx.fillStyle = hovered ? '#2563eb' : '#1f2937';
    ctx.strokeStyle = hovered ? '#93c5fd' : '#475569';
    ctx.lineWidth = 1.5;
    ctx.beginPath();
    ctx.roundRect(rect.x, rect.y, rect.w, rect.h, 8);
    ctx.fill();
    ctx.stroke();
    ctx.fillStyle = '#f8fafc';
    ctx.font = '12px sans-serif';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillText(label, rect.x + rect.w / 2, rect.y + rect.h / 2);
  }

  _inRect(x, y, rect) {
    return x >= rect.x && y >= rect.y && x < rect.x + rect.w && y < rect.y + rect.h;
  }
}