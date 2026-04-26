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
    this._appData = null;

    this._onMouseMove = this._onMouseMove.bind(this);
    this._onMouseDown = this._onMouseDown.bind(this);
    this._onKeyDown = this._onKeyDown.bind(this);
  }

  onEnter(input, appData) {
    this._appData = appData;
    input.on('mousemove', this._onMouseMove);
    input.on('mousedown', this._onMouseDown);
    input.on('keydown', this._onKeyDown);
  }

  onLeave() {
    this._hoverIndex = -1;
    this._hoverAssetIndex = -1;
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
    ctx.fillText(`Pixel Docs: ${docCount}  /  Tilesets: ${tilesetCount}`, centerX, topY + 34);

    this._buttons = this._buildButtons(canvas);
    for (let index = 0; index < this._buttons.length; index++) {
      this._drawButton(ctx, this._buttons[index], index === this._hoverIndex);
    }

    this._assetItems = this._buildAssetItems(canvas, appData);
    this._drawAssetSection(ctx, canvas, appData);
  }

  _buildButtons(canvas) {
    const buttonW = 320;
    const buttonH = 52;
    const gap = 16;
    const startY = Math.max(170, canvas.height * 0.34);
    const x = ((canvas.width - buttonW) / 2) | 0;
    return [
      {
        label: 'Open Active Asset',
        action: () => this._openDotEditor(),
        rect: { x, y: startY, w: buttonW, h: buttonH },
      },
      {
        label: 'Save Project',
        action: () => this._saveProject(),
        rect: { x, y: startY + buttonH + gap, w: buttonW, h: buttonH },
      },
      {
        label: 'Map Editor (Coming Soon)',
        action: null,
        disabled: true,
        rect: { x, y: startY + (buttonH + gap) * 2, w: buttonW, h: buttonH },
      },
      {
        label: 'Back to Title',
        action: () => this._appData?.changeScene(new TitleScene()),
        rect: { x, y: startY + (buttonH + gap) * 3, w: buttonW, h: buttonH },
      },
    ];
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
    if (this._assetItems.length === 0) {
      ctx.fillText('No assets yet. Create or import from Editor.', panel.x + 18, panel.y + 54);
      return;
    }

    for (let index = 0; index < this._assetItems.length; index++) {
      const item = this._assetItems[index];
      const hovered = index === this._hoverAssetIndex;
      const isActive = !!activeAsset && activeAsset.id === item.asset.id && activeAsset.type === item.asset.type;
      this._drawAssetItem(ctx, item, hovered, isActive);
    }
  }

  _drawAssetItem(ctx, item, hovered, isActive) {
    const { x, y, w, h } = item.rect;
    const accent = item.asset.type === 'tileset' ? '#f59e0b' : '#22c55e';

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
    ctx.fillText(item.asset.type === 'tileset' ? 'TILESET' : 'PIXEL DOC', x + 14, y + 16);

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
    const assets = this._collectProjectAssets(appData.currentProject);
    const itemGap = 10;
    const itemH = 70;
    const leftPad = 14;
    const rightPad = 14;
    const topPad = 42;
    const maxRows = Math.max(1, Math.floor((panel.h - topPad - 12 + itemGap) / (itemH + itemGap)));
    const visibleAssets = assets.slice(0, maxRows);

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
    ];
  }

  _getAssetSummary(asset) {
    if (!asset) return '';
    if (asset.type === 'tileset') {
      return `${asset.columns | 0}x${asset.rows | 0} chips / ${asset.chipWidth | 0}x${asset.chipHeight | 0}px`;
    }
    return `${asset.width | 0}x${asset.height | 0}px`;
  }

  _getAssetPanelRect(canvas) {
    const w = Math.min(560, canvas.width - 48);
    const h = Math.min(320, Math.max(160, canvas.height - 420));
    const x = ((canvas.width - w) / 2) | 0;
    const y = Math.max(390, canvas.height - h - 36);
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

  _onKeyDown(e) {
    if (e.key === 'Enter') {
      this._openDotEditor();
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

  _openAsset(asset) {
    if (!asset || !this._appData?.projectSession) return;
    this._appData.projectSession.setActiveDocument(asset.type, asset.id);
    this._appData.syncEditorStateToProjectSession();
    this._appData.changeScene(new EditorScene());
  }

  _saveProject() {
    if (!this._appData?.currentProject) return;
    this._appData.saveActiveProjectAssetState();
    this._appData.syncEditorStateToProjectSession();
    this._appData.projectSession?.clearDirty();
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
    console.log('[ProjectTopScene] project saved');
  }

  _inRect(x, y, rect) {
    return x >= rect.x && y >= rect.y && x < rect.x + rect.w && y < rect.y + rect.h;
  }
}