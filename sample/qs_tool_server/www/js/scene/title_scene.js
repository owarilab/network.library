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
    this._browserProjects = [];
    this._browserProjectItems = [];
    this._browserPanelRect = null;
    this._browserProjectScrollOffset = 0;
    this._browserProjectVisibleRows = 0;
    this._hoverBrowserProjectIndex = -1;
    this._hoverBrowserProjectDeleteIndex = -1;
    this._statusMessage = '';
    this._statusTone = 'muted';

    this._onMouseMove = this._onMouseMove.bind(this);
    this._onMouseDown = this._onMouseDown.bind(this);
    this._onWheel = this._onWheel.bind(this);
    this._onKeyDown = this._onKeyDown.bind(this);
    this._onFileChange = this._onFileChange.bind(this);
  }

  onEnter(input, appData) {
    this._appData = appData;
    this._ensureFileInput();
    this._refreshBrowserProjects();
    input.on('mousemove', this._onMouseMove);
    input.on('mousedown', this._onMouseDown);
    input.on('wheel', this._onWheel);
    input.on('keydown', this._onKeyDown);
  }

  onLeave() {
    this._hoverIndex = -1;
    this._hoverBrowserProjectIndex = -1;
    this._hoverBrowserProjectDeleteIndex = -1;
    this._browserPanelRect = null;
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

    if (this._statusMessage) {
      ctx.fillStyle = this._statusTone === 'error' ? '#fca5a5' : '#93c5fd';
      ctx.font = '13px sans-serif';
      ctx.fillText(this._statusMessage, centerX, topY + 104);
    }

    this._buttons = this._buildButtons(canvas, appData);
    for (let index = 0; index < this._buttons.length; index++) {
      this._drawButton(ctx, this._buttons[index], index === this._hoverIndex);
    }

    this._browserProjectItems = this._buildBrowserProjectItems(canvas);
    this._drawBrowserProjectSection(ctx, canvas);

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

    buttons.push({
      label: 'Open Browser Project',
      action: null,
      disabled: this._browserProjects.length === 0,
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
    this._hoverBrowserProjectIndex = this._browserProjectItems.findIndex(item => this._inRect(e.x, e.y, item.rect));
    this._hoverBrowserProjectDeleteIndex = this._browserProjectItems.findIndex(item => this._inRect(e.x, e.y, item.deleteRect));
  }

  _onMouseDown(e) {
    if (e.button !== 0) return;
    const button = this._buttons.find(item => this._inRect(e.x, e.y, item.rect));
    if (button && !button.disabled && button.action) {
      button.action();
      return;
    }

    const deleteItem = this._browserProjectItems.find(item => this._inRect(e.x, e.y, item.deleteRect));
    if (deleteItem) {
      this._deleteBrowserProject(deleteItem.project.id, deleteItem.project.name);
      return;
    }

    const browserItem = this._browserProjectItems.find(item => this._inRect(e.x, e.y, item.rect));
    if (!browserItem) return;
    this._openBrowserProject(browserItem.project.id);
  }

  _onWheel(e) {
    if (!this._browserPanelRect || !this._inRect(e.x, e.y, this._browserPanelRect)) return;
    const maxOffset = this._getMaxBrowserProjectScrollOffset(this._browserProjects.length, this._browserProjectVisibleRows);
    if (maxOffset <= 0) return;

    const nextOffset = e.deltaY > 0
      ? Math.min(maxOffset, this._browserProjectScrollOffset + 1)
      : e.deltaY < 0
        ? Math.max(0, this._browserProjectScrollOffset - 1)
        : this._browserProjectScrollOffset;

    if (nextOffset === this._browserProjectScrollOffset) return;
    this._browserProjectScrollOffset = nextOffset;
    this._hoverBrowserProjectIndex = -1;
    this._hoverBrowserProjectDeleteIndex = -1;
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

  _refreshBrowserProjects() {
    if (!ProjectBrowserStorage.isAvailable()) {
      this._browserProjects = [];
      this._browserProjectScrollOffset = 0;
      this._statusTone = 'error';
      this._statusMessage = 'IndexedDB unavailable';
      return;
    }

    ProjectBrowserStorage.listProjects()
      .then(rows => {
        this._browserProjects = rows;
        if (!rows.length) this._browserProjectScrollOffset = 0;
        if (!rows.length) {
          this._statusTone = 'muted';
          this._statusMessage = '';
          return;
        }
        this._statusTone = 'info';
        this._statusMessage = `${rows.length} browser project(s) available`;
      })
      .catch(err => {
        this._browserProjects = [];
        this._browserProjectScrollOffset = 0;
        this._statusTone = 'error';
        this._statusMessage = err?.message || 'Browser projects load failed';
        console.error('[TitleScene] browser project list error:', err?.message || err);
      });
  }

  _buildBrowserProjectItems(canvas) {
    const panel = this._getBrowserPanelRect(canvas);
    this._browserPanelRect = panel;
    const itemH = 56;
    const itemGap = 10;
    const topPad = 64;
    const leftPad = 14;
    const rightPad = 14;
    const maxRows = Math.max(1, Math.floor((panel.h - topPad - 12 + itemGap) / (itemH + itemGap)));
    this._browserProjectVisibleRows = maxRows;
    const maxOffset = this._getMaxBrowserProjectScrollOffset(this._browserProjects.length, maxRows);
    if (this._browserProjectScrollOffset > maxOffset) {
      this._browserProjectScrollOffset = maxOffset;
    }
    const visible = this._browserProjects.slice(this._browserProjectScrollOffset, this._browserProjectScrollOffset + maxRows);
    return visible.map((project, index) => ({
      project,
      rect: {
        x: panel.x + leftPad,
        y: panel.y + topPad + (itemH + itemGap) * index,
        w: panel.w - leftPad - rightPad,
        h: itemH,
      },
      deleteRect: {
        x: panel.x + panel.w - rightPad - 72,
        y: panel.y + topPad + (itemH + itemGap) * index + 12,
        w: 60,
        h: 24,
      },
    }));
  }

  _drawBrowserProjectSection(ctx, canvas) {
    const panel = this._getBrowserPanelRect(canvas);
    ctx.fillStyle = '#0f172a';
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
    ctx.fillText('Browser Projects', panel.x + 18, panel.y + 24);

    if (!this._browserProjectItems.length) {
      ctx.fillStyle = '#64748b';
      ctx.font = '13px sans-serif';
      ctx.fillText('No saved browser projects yet.', panel.x + 18, panel.y + 54);
      return;
    }

    ctx.fillStyle = '#94a3b8';
    ctx.font = '13px sans-serif';
    ctx.fillText(`Wheel: scroll list (${Math.min(this._browserProjects.length, this._browserProjectVisibleRows)}/${this._browserProjects.length})`, panel.x + 18, panel.y + 54);

    for (let i = 0; i < this._browserProjectItems.length; i++) {
      this._drawBrowserProjectItem(
        ctx,
        this._browserProjectItems[i],
        i === this._hoverBrowserProjectIndex,
        i === this._hoverBrowserProjectDeleteIndex,
      );
    }
  }

  _drawBrowserProjectItem(ctx, item, hovered, deleteHovered) {
    const { x, y, w, h } = item.rect;
    ctx.fillStyle = hovered ? '#132238' : '#111827';
    ctx.strokeStyle = hovered ? '#60a5fa' : '#334155';
    ctx.lineWidth = hovered ? 2 : 1.5;
    ctx.beginPath();
    ctx.roundRect(x, y, w, h, 10);
    ctx.fill();
    ctx.stroke();

    ctx.textAlign = 'left';
    ctx.textBaseline = 'middle';
    ctx.fillStyle = '#f8fafc';
    ctx.font = 'bold 14px sans-serif';
    ctx.fillText(item.project.name || 'Untitled Project', x + 12, y + 18);

    const counts = item.project.assetCounts || {};
    const summary = `Pixel ${counts.pixelDocuments | 0} / Tileset ${counts.tilesets | 0} / Map ${counts.maps | 0} / PlayUnit ${counts.playUnits | 0}`;
    ctx.fillStyle = '#94a3b8';
    ctx.font = '12px sans-serif';
    ctx.fillText(summary, x + 12, y + 38);

    const dr = item.deleteRect;
    const dateLabel = this._formatTimestamp(item.project.updatedAt);
    ctx.textAlign = 'right';
    ctx.fillText(dateLabel, dr.x - 10, y + 18);

    ctx.fillStyle = deleteHovered ? '#dc2626' : '#7f1d1d';
    ctx.strokeStyle = deleteHovered ? '#fca5a5' : '#ef4444';
    ctx.lineWidth = 1.5;
    ctx.beginPath();
    ctx.roundRect(dr.x, dr.y, dr.w, dr.h, 8);
    ctx.fill();
    ctx.stroke();

    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillStyle = '#fee2e2';
    ctx.font = 'bold 11px sans-serif';
    ctx.fillText('Delete', dr.x + dr.w / 2, dr.y + dr.h / 2 + 1);
  }

  _openBrowserProject(projectId) {
    if (!this._appData) return;
    ProjectBrowserStorage.loadProject(projectId)
      .then(({ project, session }) => {
        this._appData.setCurrentProject(project, session);
        window.localStorage?.setItem?.('qs_tool_server.lastOpenedProjectId', project.id);
        this._appData.changeScene(new ProjectTopScene());
        console.log(`[TitleScene] browser project loaded: ${project.name}`);
      })
      .catch(err => {
        this._statusTone = 'error';
        this._statusMessage = err?.message || 'Browser project load failed';
        console.error('[TitleScene] browser project load error:', err?.message || err);
      });
  }

  _deleteBrowserProject(projectId, projectName) {
    const label = projectName || 'Untitled Project';
    if (!window.confirm(`Delete browser project \"${label}\"?`)) return;

    ProjectBrowserStorage.deleteProject(projectId)
      .then(() => {
        if (window.localStorage?.getItem?.('qs_tool_server.lastOpenedProjectId') === projectId) {
          window.localStorage.removeItem('qs_tool_server.lastOpenedProjectId');
        }
        this._statusTone = 'info';
        this._statusMessage = `Deleted browser project: ${label}`;
        this._refreshBrowserProjects();
        console.log(`[TitleScene] browser project deleted: ${label}`);
      })
      .catch(err => {
        this._statusTone = 'error';
        this._statusMessage = err?.message || 'Browser project delete failed';
        console.error('[TitleScene] browser project delete error:', err?.message || err);
      });
  }

  _getBrowserPanelRect(canvas) {
    const w = Math.min(620, canvas.width - 48);
    const h = Math.min(280, Math.max(140, canvas.height - 440));
    const x = ((canvas.width - w) / 2) | 0;
    const y = Math.max(430, canvas.height - h - 32);
    return { x, y, w, h };
  }

  _formatTimestamp(timestamp) {
    const value = Number(timestamp);
    if (!value) return '-';
    try {
      const date = new Date(value);
      const yyyy = date.getFullYear();
      const mm = `${date.getMonth() + 1}`.padStart(2, '0');
      const dd = `${date.getDate()}`.padStart(2, '0');
      const hh = `${date.getHours()}`.padStart(2, '0');
      const mi = `${date.getMinutes()}`.padStart(2, '0');
      return `${yyyy}-${mm}-${dd} ${hh}:${mi}`;
    } catch {
      return '-';
    }
  }

  _getMaxBrowserProjectScrollOffset(projectCount, visibleRows) {
    return Math.max(0, projectCount - Math.max(1, visibleRows | 0));
  }

  _inRect(x, y, rect) {
    return x >= rect.x && y >= rect.y && x < rect.x + rect.w && y < rect.y + rect.h;
  }
}