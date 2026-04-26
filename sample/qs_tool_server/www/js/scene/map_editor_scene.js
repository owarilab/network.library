/**
 * MapEditorScene
 * マップ編集機能の雛形シーン。UIWindow ベースの各パネルを組み合わせる。
 */
class MapEditorScene extends Scene {
  constructor() {
    super();
    this._appData = null;
    this._statusMessage = 'Map editor scaffold';
    this._statusTone = 'muted';
    this._spaceDown = false;

    this._menuBar = new MapEditorMenuBar();
    this._menuBar.onSelect = (id) => this._onMenuSelect(id);

    this._mapResizeDialog = new MapResizeDialog(
      (width, height) => this.resizeMap(width, height, this._appData),
      () => this.setStatus('Map resize cancelled', 'muted'),
    );

    this._mapViewWindow = new MapViewWindow(this);
    this._tilesetWindow = new MapTilesetWindow(this);
    this._inspectorWindow = new MapInspectorWindow(this);
    this._windows = [
      this._mapViewWindow,
      this._tilesetWindow,
      this._inspectorWindow,
    ];

    this._onMouseMove = this._onMouseMove.bind(this);
    this._onMouseDown = this._onMouseDown.bind(this);
    this._onMouseUp = this._onMouseUp.bind(this);
    this._onWheel = this._onWheel.bind(this);
    this._onKeyDown = this._onKeyDown.bind(this);
    this._onKeyUp = this._onKeyUp.bind(this);
  }

  onEnter(input, appData) {
    this._appData = appData;
    this._ensureMapAsset(appData);
    input.on('mousemove', this._onMouseMove);
    input.on('mousedown', this._onMouseDown);
    input.on('mouseup', this._onMouseUp);
    input.on('wheel', this._onWheel);
    input.on('keydown', this._onKeyDown);
    input.on('keyup', this._onKeyUp);
  }

  onLeave(input, appData) {
    appData?.saveActiveProjectAssetState();
    appData?.syncEditorStateToProjectSession();
  }

  render(ctx, canvas, appData) {
    ctx.fillStyle = '#0b1020';
    ctx.fillRect(0, 0, canvas.width, canvas.height);

    const asset = appData.getActiveProjectAsset();

    ctx.fillStyle = '#f8fafc';
    ctx.textAlign = 'left';
    ctx.textBaseline = 'middle';
    ctx.font = 'bold 13px sans-serif';
    ctx.fillText(`Map Editor: ${asset?.name || 'untitled map'}`, 14, MenuBar.HEIGHT + 12);

    ctx.fillStyle = this._statusTone === 'error' ? '#fca5a5' : '#93c5fd';
    ctx.font = '11px sans-serif';
    ctx.textAlign = 'right';
    ctx.fillText(this._statusMessage, canvas.width - 16, MenuBar.HEIGHT + 12);

    for (const window of this._windows) {
      window.render(ctx, canvas, appData);
    }

    this._mapResizeDialog.render(ctx, canvas);

    this._menuBar.render(ctx, canvas);
  }

  update(dt, appData) {}

  getActiveTilesetAsset(appData = this._appData) {
    const project = appData?.currentProject;
    const mapData = appData?.mapData;
    if (!project || !mapData) return null;
    if (mapData.tilesetId) {
      const direct = project.findTilesetById(mapData.tilesetId);
      if (direct) return direct;
    }
    return project.assets.tilesets[0] || null;
  }

  selectTileset(tilesetId, appData = this._appData) {
    if (!appData?.mapData || !appData.currentProject) return false;
    const asset = appData.currentProject.findTilesetById(tilesetId);
    if (!asset) return false;
    appData.mapData.tilesetId = asset.id;
    appData.mapData.tileWidth = asset.chipWidth | 0;
    appData.mapData.tileHeight = asset.chipHeight | 0;
    if (!appData.mapData.selectedTileRef || appData.mapData.selectedTileRef.tilesetId !== asset.id) {
      appData.mapData.selectedTileRef = {
        tilesetId: asset.id,
        col: 0,
        row: 0,
        index: 0,
      };
    }
    appData.projectSession?.markDirty();
    this.setStatus(`Tileset selected: ${asset.name}`, 'info');
    return true;
  }

  selectPaintTile(tileRef, appData = this._appData) {
    if (!appData?.mapData || !tileRef) return false;
    appData.mapData.tilesetId = tileRef.tilesetId;
    appData.mapData.selectedTileRef = {
      tilesetId: tileRef.tilesetId,
      col: tileRef.col | 0,
      row: tileRef.row | 0,
      index: tileRef.index | 0,
    };
    appData.projectSession?.markDirty();
    this.setStatus(`Paint tile #${tileRef.index} selected`, 'info');
    return true;
  }

  applyMapCellAction(col, row, button, appData = this._appData) {
    const mapData = appData?.mapData;
    if (!mapData) return false;
    const layer = mapData.layers[mapData.selectedLayer] || mapData.layers[0] || null;
    if (!layer || layer.locked) return false;
    const index = row * mapData.width + col;
    if (button === 2) {
      layer.tiles[index] = -1;
      appData.projectSession?.markDirty();
      this.setStatus(`Cleared cell (${col}, ${row})`, 'info');
      return true;
    }
    const selectedTile = mapData.selectedTileRef;
    if (!selectedTile) {
      this.setStatus('Select a tile before painting', 'error');
      return true;
    }
    layer.tiles[index] = selectedTile.index | 0;
    mapData.tilesetId = selectedTile.tilesetId;
    appData.projectSession?.markDirty();
    this.setStatus(`Painted tile #${selectedTile.index} at (${col}, ${row})`, 'info');
    return true;
  }

  toggleGrid(appData = this._appData) {
    if (!appData?.mapData) return false;
    appData.mapData.view.showGrid = appData.mapData.view.showGrid === false;
    appData.projectSession?.markDirty();
    this.setStatus(`Grid ${appData.mapData.view.showGrid ? 'enabled' : 'hidden'}`, 'info');
    return true;
  }

  resizeMap(width, height, appData = this._appData) {
    const mapData = appData?.mapData;
    if (!mapData) return false;

    const nextWidth = Math.max(1, Math.min(512, width | 0 || mapData.width | 0));
    const nextHeight = Math.max(1, Math.min(512, height | 0 || mapData.height | 0));
    const prevWidth = mapData.width | 0;
    const prevHeight = mapData.height | 0;
    if (nextWidth === prevWidth && nextHeight === prevHeight) {
      this.setStatus('Map size unchanged', 'muted');
      return false;
    }

    const layers = Array.isArray(mapData.layers) && mapData.layers.length
      ? mapData.layers
      : [{ id: 'layer_ground', name: 'Ground', visible: true, locked: false, tiles: [] }];
    const copyWidth = Math.min(prevWidth, nextWidth);
    const copyHeight = Math.min(prevHeight, nextHeight);

    for (const layer of layers) {
      const prevTiles = Array.isArray(layer.tiles) ? layer.tiles : [];
      const nextTiles = new Array(nextWidth * nextHeight).fill(-1);
      for (let row = 0; row < copyHeight; row++) {
        for (let col = 0; col < copyWidth; col++) {
          nextTiles[row * nextWidth + col] = prevTiles[row * prevWidth + col] ?? -1;
        }
      }
      layer.tiles = nextTiles;
    }

    mapData.layers = layers;
    mapData.width = nextWidth;
    mapData.height = nextHeight;
    mapData.cursor = {
      x: Math.max(0, Math.min(nextWidth - 1, mapData.cursor?.x | 0)),
      y: Math.max(0, Math.min(nextHeight - 1, mapData.cursor?.y | 0)),
    };
    this._mapViewWindow.clampView(appData);

    appData.projectSession?.markDirty();
    this.setStatus(`Map resized to ${nextWidth}x${nextHeight}`, 'info');
    return true;
  }

  resetMapView(appData = this._appData) {
    if (!this._mapViewWindow.resetView(appData)) return false;
    this.setStatus('Map view reset', 'info');
    return true;
  }

  getMapZoomLabel(appData = this._appData) {
    return this._mapViewWindow.getZoomLabel(appData);
  }

  isPanModifierActive() {
    return this._spaceDown;
  }

  backToProjectTop(appData = this._appData) {
    appData?.saveActiveProjectAssetState();
    appData?.syncEditorStateToProjectSession();
    appData?.changeScene(new ProjectTopScene());
  }

  setStatus(message, tone = 'muted') {
    this._statusMessage = message || '';
    this._statusTone = tone;
  }

  _ensureMapAsset(appData) {
    if (!appData?.currentProject || !appData.projectSession) return;
    let asset = appData.getActiveProjectAsset();
    if (!asset || asset.type !== 'map') {
      asset = appData.currentProject.assets.maps[0] || null;
    }
    if (!asset) {
      const defaultTileset = appData.currentProject.assets.tilesets[0] || null;
      const mapData = appData.createMapData(
        24,
        18,
        defaultTileset?.chipWidth | 0,
        defaultTileset?.chipHeight | 0,
        defaultTileset?.id || null,
      );
      if (defaultTileset) {
        mapData.selectedTileRef = {
          tilesetId: defaultTileset.id,
          col: 0,
          row: 0,
          index: 0,
        };
      }
      asset = appData.currentProject.addMap({
        name: 'map_01',
        width: mapData.width,
        height: mapData.height,
        mapData,
      });
      appData.projectSession.markDirty();
    }
    if (!asset.mapData) {
      asset.mapData = appData.createMapData(asset.width | 0, asset.height | 0);
    }
    appData.projectSession.setActiveDocument('map', asset.id);
    appData.setMapData(asset.mapData);
    if (!appData.mapData.tilesetId && appData.currentProject.assets.tilesets[0]) {
      this.selectTileset(appData.currentProject.assets.tilesets[0].id, appData);
    }
    this.setStatus('Map editor scaffold ready', 'info');
  }

  _onMouseMove(e) {
    if (this._mapResizeDialog.isVisible) {
      this._mapResizeDialog.onMouseMove(e);
      return;
    }
    this._menuBar.onMouseMove(e);
    if (this._menuBar.isOpen || this._menuBar.containsInteractivePoint(e.x, e.y)) return;
    for (const window of this._windows) {
      window.onMouseMove(e, this._appData);
    }
  }

  _onMouseDown(e) {
    if (this._mapResizeDialog.isVisible) {
      this._mapResizeDialog.onMouseDown(e);
      return;
    }
    if (this._menuBar.isOpen || this._menuBar.containsInteractivePoint(e.x, e.y)) {
      this._menuBar.onMouseDown(e);
      return;
    }
    for (let index = this._windows.length - 1; index >= 0; index--) {
      const window = this._windows[index];
      if (!window.onMouseDown(e, this._appData)) continue;
      this._focusWindow(index);
      return;
    }
  }

  _onMouseUp(e) {
    if (this._mapResizeDialog.isVisible) {
      this._mapResizeDialog.onMouseUp(e);
      return;
    }
    this._menuBar.onMouseUp(e);
    for (const window of this._windows) {
      window.onMouseUp(e, this._appData);
    }
  }

  _onKeyDown(e) {
    if (this._mapResizeDialog.isVisible) {
      this._mapResizeDialog.onKeyDown(e);
      return;
    }
    if (e.key === ' ' || e.code === 'Space') {
      this._spaceDown = true;
      return;
    }
    if (e.key === 'Escape') {
      this.backToProjectTop(this._appData);
      return;
    }
    if (e.key === 'g' || e.key === 'G') {
      this.toggleGrid(this._appData);
    }
  }

  _onKeyUp(e) {
    if (e.key === ' ' || e.code === 'Space') {
      this._spaceDown = false;
    }
  }

  _onWheel(e) {
    if (this._mapResizeDialog.isVisible) return;
    if (this._menuBar.isOpen || this._menuBar.containsInteractivePoint(e.x, e.y)) return;
    if (!this._mapViewWindow.containsViewportPoint(e.x, e.y)) return;
    if (this._mapViewWindow.zoomAt(-e.deltaY, e.x, e.y, this._appData)) {
      this.setStatus(`Zoom ${this.getMapZoomLabel(this._appData)}`, 'info');
    }
  }

  _onMenuSelect(id) {
    if (id === MenuConstants.FILE_SAVE) {
      if (!this._appData?.currentProject) return;
      this._appData.saveActiveProjectAssetState();
      this._appData.syncEditorStateToProjectSession();
      ProjectBrowserStorage.saveProject(
        this._appData.currentProject,
        this._appData.projectSession,
        this._appData.palette,
      )
        .then(() => {
          this._appData.projectSession?.clearDirty();
          this.setStatus('Saved to browser storage', 'info');
        })
        .catch(err => {
          this.setStatus(err?.message || 'Browser save failed', 'error');
        });
      return;
    }

    if (id === MenuConstants.FILE_MAP_RESIZE) {
      this._mapResizeDialog.showWithSize(this._appData?.mapData?.width, this._appData?.mapData?.height);
      return;
    }

    if (id === MenuConstants.FILE_EXIT) {
      this.backToProjectTop(this._appData);
      return;
    }

    if (id === MenuConstants.VIEW_GRID) {
      this.toggleGrid(this._appData);
    }
  }

  _focusWindow(index) {
    if (index < 0 || index >= this._windows.length) return;
    const win = this._windows.splice(index, 1)[0];
    this._windows.push(win);
  }
}