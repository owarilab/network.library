/**
 * ProjectTopScene
 * 現在のプロジェクトから各ツールへ入るハブシーン。
 */
class ProjectTopScene extends Scene {
  constructor() {
    super();
    this._buttons = [];
    this._hoverIndex = -1;
    this._assetItems = [];
    this._hoverAssetIndex = -1;
    this._assetPanelRect = null;
    this._assetScrollOffset = 0;
    this._assetVisibleRows = 0;
    this._appData = null;
    this._statusMessage = '';
    this._statusTone = 'muted';

    this._onMouseMove = this._onMouseMove.bind(this);
    this._onMouseDown = this._onMouseDown.bind(this);
    this._onWheel = this._onWheel.bind(this);
    this._onKeyDown = this._onKeyDown.bind(this);
  }

  onEnter(input, appData) {
    this._appData = appData;
    input.on('mousemove', this._onMouseMove);
    input.on('mousedown', this._onMouseDown);
    input.on('wheel', this._onWheel);
    input.on('keydown', this._onKeyDown);
  }

  onLeave() {
    this._hoverIndex = -1;
    this._hoverAssetIndex = -1;
    this._assetPanelRect = null;
  }

  render(ctx, canvas, appData) {
    ctx.fillStyle = '#101827';
    ctx.fillRect(0, 0, canvas.width, canvas.height);

    const centerX = canvas.width * 0.5;
    const topY = Math.max(72, canvas.height * 0.16);
    const project = appData.currentProject;

    ctx.fillStyle = '#f8fafc';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.font = 'bold 34px sans-serif';
    ctx.fillText(project ? project.name : 'No Project', centerX, topY);

    ctx.fillStyle = '#94a3b8';
    ctx.font = '15px sans-serif';
    const docCount = project ? project.assets.pixelDocuments.length : 0;
    const tilesetCount = project ? project.assets.tilesets.length : 0;
    const mapCount = project ? project.assets.maps.length : 0;
    const playUnitCount = project ? (project.assets.playUnits || []).length : 0;
    ctx.fillText(`Pixel Docs: ${docCount}  /  Tilesets: ${tilesetCount}  /  Maps: ${mapCount}  /  PlayUnits: ${playUnitCount}`, centerX, topY + 34);

    ctx.fillStyle = '#64748b';
    ctx.font = '13px sans-serif';
    ctx.fillText('Shortcut: Enter opens active asset / M opens map editor / P opens PlayUnit editor', centerX, topY + 84);

    if (this._statusMessage) {
      ctx.fillStyle = this._statusTone === 'error' ? '#fca5a5' : '#93c5fd';
      ctx.font = '13px sans-serif';
      ctx.fillText(this._statusMessage, centerX, topY + 106);
    }

    this._buttons = this._buildButtons(canvas);
    for (let index = 0; index < this._buttons.length; index++) {
      this._drawButton(ctx, this._buttons[index], index === this._hoverIndex);
    }

    this._assetItems = this._buildAssetItems(canvas, appData);
    this._drawAssetSection(ctx, canvas, appData);
  }

  _buildButtons(canvas) {
    const buttonW = canvas.width >= 860 ? 240 : Math.min(320, canvas.width - 48);
    const buttonH = 52;
    const gapX = 16;
    const gapY = 16;
    const startY = Math.max(186, canvas.height * 0.3);
    const defs = [
      {
        label: 'Open Active Asset',
        action: () => this._openDotEditor(),
      },
      {
        label: 'Save To Browser',
        action: () => this._saveProject(),
      },
      {
        label: 'Save As (Browser)',
        action: () => this._saveProjectAs(),
      },
      {
        label: 'Export .qsproj',
        action: () => this._exportProject(),
      },
      {
        label: 'Open Map Editor',
        action: () => this._openMapEditor(),
      },
      {
        label: 'Open PlayUnit Editor',
        action: () => this._openPlayUnitEditor(),
      },
      {
        label: 'Back to Title',
        action: () => this._appData?.changeScene(new TitleScene()),
      },
    ];

    const columns = canvas.width >= 860 ? 2 : 1;
    const totalW = columns * buttonW + Math.max(0, columns - 1) * gapX;
    const startX = ((canvas.width - totalW) / 2) | 0;

    return defs.map((button, index) => {
      const col = index % columns;
      const row = (index / columns) | 0;
      return {
        ...button,
        rect: {
          x: startX + col * (buttonW + gapX),
          y: startY + row * (buttonH + gapY),
          w: buttonW,
          h: buttonH,
        },
      };
    });
  }

  _drawButton(ctx, button, hovered) {
    const { x, y, w, h } = button.rect;
    const disabled = !!button.disabled;
    ctx.fillStyle = disabled ? '#374151' : hovered ? '#0ea5e9' : '#111827';
    ctx.strokeStyle = hovered ? '#bae6fd' : '#475569';
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.roundRect(x, y, w, h, 12);
    ctx.fill();
    ctx.stroke();

    ctx.fillStyle = disabled ? '#6b7280' : '#f8fafc';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.font = 'bold 18px sans-serif';
    ctx.fillText(button.label, x + w / 2, y + h / 2);
  }

  _drawAssetSection(ctx, canvas, appData) {
    const panel = this._getAssetPanelRect(canvas);
    const activeAsset = appData.getActiveProjectAsset();

    ctx.fillStyle = '#0b1220';
    ctx.strokeStyle = '#334155';
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.roundRect(panel.x, panel.y, panel.w, panel.h, 14);
    ctx.fill();
    ctx.stroke();

    ctx.textAlign = 'left';
    ctx.textBaseline = 'middle';
    ctx.fillStyle = '#e2e8f0';
    ctx.font = 'bold 18px sans-serif';
    ctx.fillText('Project Assets', panel.x + 18, panel.y + 24);

    ctx.fillStyle = '#94a3b8';
    ctx.font = '13px sans-serif';
    const totalAssets = this._collectProjectAssets(appData.currentProject).length;
    if (this._assetItems.length === 0) {
      ctx.fillText('No assets yet. Create or import from Editor.', panel.x + 18, panel.y + 54);
      return;
    }

    ctx.fillText(`Wheel: scroll list (${Math.min(totalAssets, this._assetVisibleRows)}/${totalAssets})`, panel.x + 18, panel.y + 54);

    for (let index = 0; index < this._assetItems.length; index++) {
      const item = this._assetItems[index];
      const hovered = index === this._hoverAssetIndex;
      const isActive = !!activeAsset && activeAsset.id === item.asset.id && activeAsset.type === item.asset.type;
      this._drawAssetItem(ctx, item, hovered, isActive);
    }
  }

  _drawAssetItem(ctx, item, hovered, isActive) {
    const { x, y, w, h } = item.rect;
    const accent = item.asset.type === 'tileset'
      ? '#f59e0b'
      : item.asset.type === 'map'
        ? '#38bdf8'
        : item.asset.type === 'playUnit'
          ? '#f472b6'
        : '#22c55e';

    ctx.fillStyle = hovered ? '#132238' : '#0f172a';
    ctx.strokeStyle = isActive ? accent : hovered ? '#64748b' : '#334155';
    ctx.lineWidth = isActive ? 3 : 1.5;
    ctx.beginPath();
    ctx.roundRect(x, y, w, h, 10);
    ctx.fill();
    ctx.stroke();

    ctx.fillStyle = accent;
    ctx.font = 'bold 12px sans-serif';
    ctx.textAlign = 'left';
    ctx.textBaseline = 'middle';
    ctx.fillText(
      item.asset.type === 'tileset'
        ? 'TILESET'
        : item.asset.type === 'map'
          ? 'MAP'
          : item.asset.type === 'playUnit'
            ? 'PLAYUNIT'
          : 'PIXEL DOC',
      x + 14,
      y + 16,
    );

    ctx.fillStyle = '#f8fafc';
    ctx.font = 'bold 16px sans-serif';
    ctx.fillText(item.asset.name, x + 14, y + 38);

    ctx.fillStyle = '#94a3b8';
    ctx.font = '12px sans-serif';
    ctx.fillText(item.summary, x + 14, y + 58);

    if (isActive) {
      ctx.fillStyle = '#cbd5e1';
      ctx.font = '12px sans-serif';
      ctx.textAlign = 'right';
      ctx.fillText('active', x + w - 12, y + 16);
    }
  }

  _buildAssetItems(canvas, appData) {
    const panel = this._getAssetPanelRect(canvas);
    this._assetPanelRect = panel;
    const assets = this._collectProjectAssets(appData.currentProject);
    const itemGap = 10;
    const itemH = 70;
    const leftPad = 14;
    const rightPad = 14;
    const topPad = 64;
    const maxRows = Math.max(1, Math.floor((panel.h - topPad - 12 + itemGap) / (itemH + itemGap)));
    this._assetVisibleRows = maxRows;
    const maxOffset = this._getMaxAssetScrollOffset(assets.length, maxRows);
    if (this._assetScrollOffset > maxOffset) {
      this._assetScrollOffset = maxOffset;
    }
    const visibleAssets = assets.slice(this._assetScrollOffset, this._assetScrollOffset + maxRows);

    return visibleAssets.map((asset, index) => ({
      asset,
      summary: this._getAssetSummary(asset),
      rect: {
        x: panel.x + leftPad,
        y: panel.y + topPad + (itemH + itemGap) * index,
        w: panel.w - leftPad - rightPad,
        h: itemH,
      },
    }));
  }

  _collectProjectAssets(project) {
    if (!project) return [];
    return [
      ...(project.assets.pixelDocuments || []),
      ...(project.assets.tilesets || []),
      ...(project.assets.maps || []),
      ...(project.assets.playUnits || []),
    ];
  }

  _getAssetSummary(asset) {
    if (!asset) return '';
    if (asset.type === 'tileset') {
      return `${asset.columns | 0}x${asset.rows | 0} chips / ${asset.chipWidth | 0}x${asset.chipHeight | 0}px`;
    }
    if (asset.type === 'map') {
      return `${asset.width | 0}x${asset.height | 0} cells`;
    }
    if (asset.type === 'playUnit') {
      return `${Array.isArray(asset.objects) ? asset.objects.length : 0} objects`;
    }
    return `${asset.width | 0}x${asset.height | 0}px`;
  }

  _getAssetPanelRect(canvas) {
    const buttonBottom = this._buttons.reduce((maxY, button) => {
      const bottom = button.rect.y + button.rect.h;
      return bottom > maxY ? bottom : maxY;
    }, Math.max(240, canvas.height * 0.3));
    const top = buttonBottom + 24;
    const w = Math.min(560, canvas.width - 48);
    const h = Math.min(320, Math.max(120, canvas.height - top - 28));
    const x = ((canvas.width - w) / 2) | 0;
    const y = Math.min(Math.max(top, 390), canvas.height - h - 28);
    return { x, y, w, h };
  }

  _onMouseMove(e) {
    this._hoverIndex = this._buttons.findIndex(button => this._inRect(e.x, e.y, button.rect));
    this._hoverAssetIndex = this._assetItems.findIndex(item => this._inRect(e.x, e.y, item.rect));
  }

  _onMouseDown(e) {
    if (e.button !== 0) return;
    const button = this._buttons.find(item => this._inRect(e.x, e.y, item.rect));
    if (button && !button.disabled && button.action) {
      button.action();
      return;
    }

    const assetItem = this._assetItems.find(item => this._inRect(e.x, e.y, item.rect));
    if (!assetItem) return;
    this._openAsset(assetItem.asset);
  }

  _onWheel(e) {
    if (!this._assetPanelRect || !this._inRect(e.x, e.y, this._assetPanelRect)) return;
    const assetCount = this._collectProjectAssets(this._appData?.currentProject).length;
    const maxOffset = this._getMaxAssetScrollOffset(assetCount, this._assetVisibleRows);
    if (maxOffset <= 0) return;

    const nextOffset = e.deltaY > 0
      ? Math.min(maxOffset, this._assetScrollOffset + 1)
      : e.deltaY < 0
        ? Math.max(0, this._assetScrollOffset - 1)
        : this._assetScrollOffset;

    if (nextOffset === this._assetScrollOffset) return;
    this._assetScrollOffset = nextOffset;
    this._hoverAssetIndex = -1;
  }

  _onKeyDown(e) {
    if (e.key === 'Enter') {
      this._openDotEditor();
      return;
    }
    if (e.key === 'm' || e.key === 'M') {
      this._openMapEditor();
      return;
    }
    if (e.key === 'p' || e.key === 'P') {
      this._openPlayUnitEditor();
      return;
    }
    if (e.key === 's' || e.key === 'S') {
      this._saveProject();
      return;
    }
    if (e.key === 'Escape') {
      this._appData?.changeScene(new TitleScene());
    }
  }

  _openDotEditor() {
    if (!this._appData?.currentProject || !this._appData.projectSession) return;

    let asset = this._appData.getActiveProjectAsset();
    if (asset) {
      this._openAsset(asset);
      return;
    }
    if (!asset) {
      asset = this._appData.currentProject.assets.pixelDocuments[0] || null;
    }
    if (!asset) {
      asset = this._appData.currentProject.assets.tilesets[0] || null;
    }
    if (!asset) {
      this._appData.createPixelData(32, 32, 0x00000000);
      asset = this._appData.currentProject.addPixelDocument({
        name: 'untitled',
        width: 32,
        height: 32,
        layerData: this._appData.createEditStateSnapshot().layerData,
      });
    }

    this._openAsset(asset);
  }

  _getMaxAssetScrollOffset(assetCount, visibleRows) {
    return Math.max(0, assetCount - Math.max(1, visibleRows | 0));
  }

  _openMapEditor() {
    if (!this._appData?.currentProject || !this._appData.projectSession) return;

    let asset = this._appData.getActiveProjectAsset();
    if (!asset || asset.type !== 'map') {
      asset = this._appData.currentProject.assets.maps[0] || null;
    }

    if (!asset) {
      const defaultTileset = this._appData.currentProject.assets.tilesets[0] || null;
      const mapData = this._appData.createMapData(
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
      asset = this._appData.currentProject.addMap({
        name: `map_${this._appData.currentProject.assets.maps.length + 1}`,
        width: mapData.width,
        height: mapData.height,
        mapData,
      });
      this._appData.projectSession.markDirty();
    }

    this._openAsset(asset);
  }

  _openAsset(asset) {
    if (!asset || !this._appData?.projectSession) return;
    this._appData.projectSession.setActiveDocument(asset.type, asset.id);
    this._appData.syncEditorStateToProjectSession();
    if (asset.type === 'map') {
      this._appData.changeScene(new MapEditorScene());
      return;
    }
    if (asset.type === 'playUnit') {
      this._appData.changeScene(new PlayUnitEditorScene());
      return;
    }
    this._appData.changeScene(new EditorScene());
  }

  _openPlayUnitEditor() {
    if (!this._appData?.currentProject || !this._appData.projectSession) return;

    let asset = this._appData.getActiveProjectAsset();
    if (!asset || asset.type !== 'playUnit') {
      asset = this._appData.currentProject.assets.playUnits[0] || null;
    }

    if (!asset) {
      const playUnit = PlayUnitData.createDefault(`play_unit_${(this._appData.currentProject.assets.playUnits?.length || 0) + 1}`);
      playUnit.addObject({ name: 'Root', parentId: null, children: [], components: [] });
      asset = this._appData.currentProject.addPlayUnit(playUnit);
      this._appData.projectSession.markDirty();
    }

    this._openAsset(asset);
  }

  _saveProject() {
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
        this._statusTone = 'info';
        this._statusMessage = 'Saved to browser storage';
        console.log('[ProjectTopScene] project saved to browser storage');
      })
      .catch(err => {
        this._statusTone = 'error';
        this._statusMessage = err?.message || 'Browser save failed';
        console.error('[ProjectTopScene] browser save error:', err?.message || err);
      });
  }

  _saveProjectAs() {
    if (!this._appData?.currentProject || !this._appData.projectSession) return;

    const suggestedName = `${this._appData.currentProject.name || 'Project'} Copy`;
    const nextName = window.prompt('Save As project name', suggestedName);
    if (nextName == null) return;
    if (!nextName.trim()) {
      this._statusTone = 'error';
      this._statusMessage = 'Project name is required';
      return;
    }

    this._appData.saveActiveProjectAssetState();
    this._appData.syncEditorStateToProjectSession();
    ProjectBrowserStorage.saveProjectAs(
      this._appData.currentProject,
      this._appData.projectSession,
      nextName,
      this._appData.palette,
    )
      .then(({ project, session }) => {
        this._appData.setCurrentProject(project, session);
        this._statusTone = 'info';
        this._statusMessage = `Saved As: ${project.name}`;
        console.log(`[ProjectTopScene] project saved as: ${project.name}`);
      })
      .catch(err => {
        this._statusTone = 'error';
        this._statusMessage = err?.message || 'Save As failed';
        console.error('[ProjectTopScene] browser save as error:', err?.message || err);
      });
  }

  _exportProject() {
    if (!this._appData?.currentProject) return;
    this._appData.saveActiveProjectAssetState();
    this._appData.syncEditorStateToProjectSession();
    const safeName = (this._appData.currentProject.name || 'project')
      .trim()
      .replace(/[\\/:*?"<>|]+/g, '_')
      .replace(/\s+/g, '_');
    ProjectSerializer.exportProject(
      this._appData.currentProject,
      this._appData.projectSession,
      `${safeName || 'project'}.qsproj`,
      this._appData.palette,
    );
    this._statusTone = 'info';
    this._statusMessage = 'Exported .qsproj';
    console.log('[ProjectTopScene] project exported as qsproj');
  }

  _inRect(x, y, rect) {
    return x >= rect.x && y >= rect.y && x < rect.x + rect.w && y < rect.y + rect.h;
  }
}