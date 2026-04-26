/**
 * ProjectTopScene
 * 現在のプロジェクトから各ツールへ入るハブシーン。
 */
class ProjectTopScene extends Scene {
  constructor() {
    super();
    this._buttons = [];
    this._hoverIndex = -1;
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
  }

  _buildButtons(canvas) {
    const buttonW = 320;
    const buttonH = 52;
    const gap = 16;
    const startY = Math.max(170, canvas.height * 0.34);
    const x = ((canvas.width - buttonW) / 2) | 0;
    return [
      {
        label: 'Open Dot Editor',
        action: () => this._openDotEditor(),
        rect: { x, y: startY, w: buttonW, h: buttonH },
      },
      {
        label: 'Map Editor (Coming Soon)',
        action: null,
        disabled: true,
        rect: { x, y: startY + buttonH + gap, w: buttonW, h: buttonH },
      },
      {
        label: 'Back to Title',
        action: () => this._appData?.changeScene(new TitleScene()),
        rect: { x, y: startY + (buttonH + gap) * 2, w: buttonW, h: buttonH },
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

  _onMouseMove(e) {
    this._hoverIndex = this._buttons.findIndex(button => this._inRect(e.x, e.y, button.rect));
  }

  _onMouseDown(e) {
    if (e.button !== 0) return;
    const button = this._buttons.find(item => this._inRect(e.x, e.y, item.rect));
    if (!button || button.disabled || !button.action) return;
    button.action();
  }

  _onKeyDown(e) {
    if (e.key === 'Enter') {
      this._openDotEditor();
      return;
    }
    if (e.key === 'Escape') {
      this._appData?.changeScene(new TitleScene());
    }
  }

  _openDotEditor() {
    if (!this._appData?.currentProject || !this._appData.projectSession) return;

    let asset = this._appData.getActiveProjectAsset();
    if (!asset || asset.type !== 'pixelDocument') {
      asset = this._appData.currentProject.assets.pixelDocuments[0] || null;
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

    this._appData.projectSession.setActiveDocument('pixelDocument', asset.id);
    this._appData.syncEditorStateToProjectSession();
    this._appData.changeScene(new EditorScene());
  }

  _inRect(x, y, rect) {
    return x >= rect.x && y >= rect.y && x < rect.x + rect.w && y < rect.y + rect.h;
  }
}