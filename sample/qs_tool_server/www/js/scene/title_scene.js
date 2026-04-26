/**
 * TitleScene
 * アプリ起動時の入口となるタイトルシーン。
 */
class TitleScene extends Scene {
  constructor() {
    super();
    this._buttons = [];
    this._hoverIndex = -1;
    this._appData = null;
    this._fileInput = null;

    this._onMouseMove = this._onMouseMove.bind(this);
    this._onMouseDown = this._onMouseDown.bind(this);
    this._onKeyDown = this._onKeyDown.bind(this);
    this._onFileChange = this._onFileChange.bind(this);
  }

  onEnter(input, appData) {
    this._appData = appData;
    this._ensureFileInput();
    input.on('mousemove', this._onMouseMove);
    input.on('mousedown', this._onMouseDown);
    input.on('keydown', this._onKeyDown);
  }

  onLeave() {
    this._hoverIndex = -1;
    this._disposeFileInput();
  }

  render(ctx, canvas, appData) {
    ctx.fillStyle = '#1b1f2a';
    ctx.fillRect(0, 0, canvas.width, canvas.height);

    const centerX = canvas.width * 0.5;
    const topY = Math.max(80, canvas.height * 0.2);

    ctx.fillStyle = '#f3f4f6';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.font = 'bold 40px sans-serif';
    ctx.fillText('QS Tool Server', centerX, topY);

    ctx.fillStyle = '#94a3b8';
    ctx.font = '16px sans-serif';
    ctx.fillText('Project-based tool flow prototype', centerX, topY + 42);

    const projectName = appData.currentProject ? appData.currentProject.name : 'No project loaded';
    ctx.fillStyle = '#cbd5e1';
    ctx.font = '15px sans-serif';
    ctx.fillText(`Current Project: ${projectName}`, centerX, topY + 78);

    this._buttons = this._buildButtons(canvas, appData);
    for (let index = 0; index < this._buttons.length; index++) {
      this._drawButton(ctx, this._buttons[index], index === this._hoverIndex);
    }

    ctx.fillStyle = '#64748b';
    ctx.font = '13px sans-serif';
    ctx.fillText('Enter: New Project / Continue, L: Load Project', centerX, canvas.height - 40);
  }

  _buildButtons(canvas, appData) {
    const buttonW = 280;
    const buttonH = 52;
    const gap = 16;
    const startY = Math.max(180, canvas.height * 0.38);
    const x = ((canvas.width - buttonW) / 2) | 0;
    const buttons = [
      {
        label: 'New Project',
        action: () => this._createNewProject(),
        rect: { x, y: startY, w: buttonW, h: buttonH },
      },
    ];

    if (appData.currentProject) {
      buttons.push({
        label: 'Continue Project',
        action: () => appData.changeScene(new ProjectTopScene()),
        rect: { x, y: startY + (buttonH + gap), w: buttonW, h: buttonH },
      });
    }

    buttons.push({
      label: 'Load Project',
      action: () => this._openProjectPicker(),
      rect: { x, y: startY + ((buttonH + gap) * (buttons.length)), w: buttonW, h: buttonH },
    });

    return buttons;
  }

  _drawButton(ctx, button, hovered) {
    const { x, y, w, h } = button.rect;
    const disabled = !!button.disabled;
    ctx.fillStyle = disabled ? '#334155' : hovered ? '#2563eb' : '#0f172a';
    ctx.strokeStyle = hovered ? '#93c5fd' : '#475569';
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.roundRect(x, y, w, h, 12);
    ctx.fill();
    ctx.stroke();

    ctx.fillStyle = disabled ? '#64748b' : '#e5e7eb';
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
      if (this._appData?.currentProject) {
        this._appData.changeScene(new ProjectTopScene());
      } else {
        this._createNewProject();
      }
      return;
    }
    if (e.key === 'n' || e.key === 'N') {
      this._createNewProject();
      return;
    }
    if (e.key === 'l' || e.key === 'L') {
      this._openProjectPicker();
    }
  }

  _createNewProject() {
    if (!this._appData) return;
    const project = ProjectData.createDefault('Untitled Project');
    this._appData.setCurrentProject(project);
    this._appData.createPixelData(32, 32, 0x00000000);
    const pixelDocument = project.addPixelDocument({
      name: 'untitled',
      width: 32,
      height: 32,
      layerData: this._appData.createEditStateSnapshot().layerData,
    });
    this._appData.projectSession.setActiveDocument('pixelDocument', pixelDocument.id);
    this._appData.projectSession.clearDirty();
    this._appData.syncEditorStateToProjectSession();
    this._appData.changeScene(new ProjectTopScene());
  }

  _ensureFileInput() {
    if (this._fileInput) return;
    this._fileInput = document.createElement('input');
    this._fileInput.type = 'file';
    this._fileInput.accept = '.qsproj,.json';
    this._fileInput.style.display = 'none';
    this._fileInput.addEventListener('change', this._onFileChange);
    document.body.appendChild(this._fileInput);
  }

  _disposeFileInput() {
    if (!this._fileInput) return;
    this._fileInput.removeEventListener('change', this._onFileChange);
    if (this._fileInput.parentNode) this._fileInput.parentNode.removeChild(this._fileInput);
    this._fileInput = null;
  }

  _openProjectPicker() {
    this._fileInput?.click();
  }

  _onFileChange() {
    const file = this._fileInput?.files?.[0];
    if (!file || !this._appData) return;
    this._fileInput.value = '';
    ProjectSerializer.importFromFile(file)
      .then(({ project, session }) => {
        this._appData.setCurrentProject(project, session);
        this._appData.changeScene(new ProjectTopScene());
        console.log(`[TitleScene] project loaded: ${file.name}`);
      })
      .catch(err => console.error('[TitleScene] project load error:', err.message));
  }

  _inRect(x, y, rect) {
    return x >= rect.x && y >= rect.y && x < rect.x + rect.w && y < rect.y + rect.h;
  }
}