/**
 * TitleScene
 * アプリ起動時の入口となるタイトルシーン。
 */
class TitleScene extends Scene {
  static LAYOUT = {
    pad: 24,
    footerH: 40,
    buttonW: 220,
    buttonH: 52,
    buttonGap: 16,
    panelMaxW: 620,
    panelMaxH: 420,
    panelEmptyH: 120,
    panelHeadH: 52,
    rowH: 56,
    rowGap: 10,
  };

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
    const layout = this._computeLayout(canvas, appData);

    ctx.fillStyle = '#1b1f2a';
    ctx.fillRect(0, 0, canvas.width, canvas.height);

    this._drawHeader(ctx, canvas, appData, layout);

    this._buttons = this._buildButtons(layout, appData);
    for (let index = 0; index < this._buttons.length; index++) {
      this._drawButton(ctx, this._buttons[index], index === this._hoverIndex);
    }

    this._browserPanelRect = layout.panel;
    this._browserProjectVisibleRows = layout.visibleRows;
    this._browserProjectItems = this._buildBrowserProjectItems(layout.panel);
    this._drawBrowserProjectSection(ctx, layout.panel);

    ctx.fillStyle = '#64748b';
    ctx.font = '13px sans-serif';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillText('Enter: Continue / New, N: New, L: Load', canvas.width * 0.5, layout.footerY);
  }

  // ヘッダ・ボタン・パネル・フッタの配置を一度に計算する。
  // 縦方向は「ヘッダ + ボタン + パネル」の塊を、フッタを除いた領域の中央に置く。
  _computeLayout(canvas, appData) {
    const L = TitleScene.LAYOUT;
    const w = canvas.width;
    const h = canvas.height;
    const hasContinue = !!appData.currentProject;
    const hasStatus = !!this._statusMessage;
    const buttonCount = hasContinue ? 3 : 2;
    const buttonsTotalW = buttonCount * L.buttonW + (buttonCount - 1) * L.buttonGap;
    const horizontal = w - L.pad * 2 >= buttonsTotalW;
    const actionsH = horizontal
      ? L.buttonH
      : buttonCount * L.buttonH + (buttonCount - 1) * L.buttonGap;

    // ヘッダ先頭からパネル上端までの高さ（下記 header の各 offset と対応）
    const stackH = (hasStatus ? 190 : 160) + actionsH;
    const empty = this._browserProjects.length === 0;
    const minPanelH = L.panelHeadH + L.rowH + 12;
    const panelH = empty
      ? L.panelEmptyH
      : Math.min(L.panelMaxH, Math.max(minPanelH, h - L.footerH - stackH - L.pad * 2));
    const top = Math.max(12, Math.floor((h - L.footerH - stackH - panelH) / 2));
    const panelTop = top + stackH;
    const actionsY = panelTop - L.pad - actionsH;

    const panelW = Math.min(L.panelMaxW, w - L.pad * 2);
    const panel = { x: Math.floor((w - panelW) / 2), y: panelTop, w: panelW, h: panelH };
    const visibleRows = Math.max(1, Math.floor((panelH - L.panelHeadH - 12 + L.rowGap) / (L.rowH + L.rowGap)));

    const buttonRects = [];
    for (let i = 0; i < buttonCount; i++) {
      buttonRects.push(horizontal
        ? { x: Math.floor((w - buttonsTotalW) / 2) + i * (L.buttonW + L.buttonGap), y: actionsY, w: L.buttonW, h: L.buttonH }
        : { x: Math.floor((w - L.buttonW) / 2), y: actionsY + i * (L.buttonH + L.buttonGap), w: L.buttonW, h: L.buttonH });
    }

    return {
      panel,
      visibleRows,
      buttonRects,
      titleY: top + 24,
      subtitleY: top + 59,
      projectY: top + 82,
      statusY: top + 129,
      footerY: h - L.footerH / 2,
    };
  }

  _drawHeader(ctx, canvas, appData, layout) {
    const L = TitleScene.LAYOUT;
    const centerX = canvas.width * 0.5;
    const maxTextW = canvas.width - L.pad * 2;
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';

    ctx.fillStyle = '#f3f4f6';
    ctx.font = 'bold 40px sans-serif';
    ctx.fillText('QS Tool Server', centerX, layout.titleY);

    ctx.fillStyle = '#94a3b8';
    ctx.font = '16px sans-serif';
    ctx.fillText('Project-based tool flow prototype', centerX, layout.subtitleY);

    const projectName = appData.currentProject ? appData.currentProject.name : 'No project loaded';
    ctx.font = '13px sans-serif';
    const label = this._truncateText(ctx, `Current Project: ${projectName}`, maxTextW - 32);
    const pillW = Math.ceil(ctx.measureText(label).width) + 32;
    const pillX = Math.round(centerX - pillW / 2);
    ctx.fillStyle = '#0f172a';
    ctx.strokeStyle = '#334155';
    ctx.lineWidth = 1.5;
    ctx.beginPath();
    ctx.roundRect(pillX, layout.projectY, pillW, 26, 13);
    ctx.fill();
    ctx.stroke();
    ctx.fillStyle = '#cbd5e1';
    ctx.fillText(label, centerX, layout.projectY + 13);

    if (this._statusMessage) {
      ctx.fillStyle = this._statusTone === 'error' ? '#fca5a5' : '#93c5fd';
      ctx.font = '13px sans-serif';
      ctx.fillText(this._truncateText(ctx, this._statusMessage, maxTextW), centerX, layout.statusY);
    }
  }

  _buildButtons(layout, appData) {
    const actions = [
      { label: 'New Project', action: () => this._createNewProject() },
    ];
    if (appData.currentProject) {
      actions.push({ label: 'Continue Project', action: () => appData.changeScene(new ProjectTopScene()) });
    }
    actions.push({ label: 'Load Project', action: () => this._openProjectPicker() });

    const primaryLabel = appData.currentProject ? 'Continue Project' : 'New Project';
    return actions.map((item, index) => ({
      ...item,
      primary: item.label === primaryLabel,
      rect: layout.buttonRects[index],
    }));
  }

  _drawButton(ctx, button, hovered) {
    const { x, y, w, h } = button.rect;
    if (button.primary) {
      ctx.fillStyle = hovered ? '#2563eb' : '#1d4ed8';
      ctx.strokeStyle = hovered ? '#93c5fd' : '#3b82f6';
    } else {
      ctx.fillStyle = hovered ? '#1e293b' : '#0f172a';
      ctx.strokeStyle = hovered ? '#93c5fd' : '#475569';
    }
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.roundRect(x, y, w, h, 12);
    ctx.fill();
    ctx.stroke();

    ctx.fillStyle = '#e5e7eb';
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
    if (button && button.action) {
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

  _buildBrowserProjectItems(panel) {
    const L = TitleScene.LAYOUT;
    const rows = this._browserProjectVisibleRows;
    const maxOffset = this._getMaxBrowserProjectScrollOffset(this._browserProjects.length, rows);
    if (this._browserProjectScrollOffset > maxOffset) {
      this._browserProjectScrollOffset = maxOffset;
    }
    const leftPad = 14;
    const rightPad = maxOffset > 0 ? 26 : 14;
    const rowW = panel.w - leftPad - rightPad;
    const visible = this._browserProjects.slice(this._browserProjectScrollOffset, this._browserProjectScrollOffset + rows);
    return visible.map((project, index) => {
      const y = panel.y + L.panelHeadH + (L.rowH + L.rowGap) * index;
      return {
        project,
        rect: { x: panel.x + leftPad, y, w: rowW, h: L.rowH },
        deleteRect: { x: panel.x + leftPad + rowW - 72, y: y + (L.rowH - 24) / 2, w: 60, h: 24 },
      };
    });
  }

  _drawBrowserProjectSection(ctx, panel) {
    const L = TitleScene.LAYOUT;
    ctx.fillStyle = '#0f172a';
    ctx.strokeStyle = '#334155';
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.roundRect(panel.x, panel.y, panel.w, panel.h, 14);
    ctx.fill();
    ctx.stroke();

    const total = this._browserProjects.length;
    const headY = panel.y + 26;
    ctx.textAlign = 'left';
    ctx.textBaseline = 'middle';
    ctx.fillStyle = '#e2e8f0';
    ctx.font = 'bold 18px sans-serif';
    ctx.fillText('Browser Projects', panel.x + 18, headY);

    if (!total) {
      ctx.textAlign = 'center';
      ctx.fillStyle = '#64748b';
      ctx.font = '13px sans-serif';
      ctx.fillText('No saved browser projects yet.', panel.x + panel.w / 2, panel.y + L.panelHeadH + (panel.h - L.panelHeadH) / 2);
      return;
    }

    const scrollable = total > this._browserProjectVisibleRows;
    ctx.textAlign = 'right';
    ctx.fillStyle = '#94a3b8';
    ctx.font = '13px sans-serif';
    const countLabel = scrollable
      ? `${total} projects · Wheel: scroll`
      : `${total} ${total === 1 ? 'project' : 'projects'}`;
    ctx.fillText(countLabel, panel.x + panel.w - 18, headY);

    for (let i = 0; i < this._browserProjectItems.length; i++) {
      this._drawBrowserProjectItem(
        ctx,
        this._browserProjectItems[i],
        i === this._hoverBrowserProjectIndex,
        i === this._hoverBrowserProjectDeleteIndex,
      );
    }

    if (scrollable) this._drawBrowserScrollbar(ctx, panel);
  }

  _drawBrowserScrollbar(ctx, panel) {
    const L = TitleScene.LAYOUT;
    const total = this._browserProjects.length;
    const rows = this._browserProjectVisibleRows;
    const maxOffset = this._getMaxBrowserProjectScrollOffset(total, rows);
    const trackX = panel.x + panel.w - 14;
    const trackY = panel.y + L.panelHeadH;
    const trackH = panel.h - L.panelHeadH - 12;
    const thumbH = Math.max(24, Math.round(trackH * rows / total));
    const thumbY = trackY + Math.round((trackH - thumbH) * this._browserProjectScrollOffset / maxOffset);

    ctx.fillStyle = '#1e293b';
    ctx.beginPath();
    ctx.roundRect(trackX, trackY, 4, trackH, 2);
    ctx.fill();
    ctx.fillStyle = '#64748b';
    ctx.beginPath();
    ctx.roundRect(trackX, thumbY, 4, thumbH, 2);
    ctx.fill();
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

    const dr = item.deleteRect;
    const dateX = dr.x - 12;
    ctx.textAlign = 'right';
    ctx.textBaseline = 'middle';
    ctx.fillStyle = '#94a3b8';
    ctx.font = '12px sans-serif';
    const dateLabel = this._formatTimestamp(item.project.updatedAt);
    ctx.fillText(dateLabel, dateX, y + 18);
    const dateW = ctx.measureText(dateLabel).width;

    ctx.textAlign = 'left';
    ctx.fillStyle = '#f8fafc';
    ctx.font = 'bold 14px sans-serif';
    const nameMaxW = dateX - dateW - 12 - (x + 12);
    const name = this._truncateText(ctx, item.project.name || 'Untitled Project', nameMaxW);
    ctx.fillText(name, x + 12, y + 18);

    const counts = item.project.assetCounts || {};
    const summary = `Pixel ${counts.pixelDocuments | 0} / Tileset ${counts.tilesets | 0} / Map ${counts.maps | 0} / PlayUnit ${counts.playUnits | 0}`;
    ctx.fillStyle = '#94a3b8';
    ctx.font = '12px sans-serif';
    ctx.fillText(summary, x + 12, y + 38);

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

  // 現在のフォントで maxW に収まるように末尾を … で省略する
  _truncateText(ctx, text, maxW) {
    if (ctx.measureText(text).width <= maxW) return text;
    const ellipsis = '…';
    let end = text.length;
    while (end > 0 && ctx.measureText(text.slice(0, end) + ellipsis).width > maxW) end--;
    return end > 0 ? text.slice(0, end) + ellipsis : ellipsis;
  }

  _inRect(x, y, rect) {
    return x >= rect.x && y >= rect.y && x < rect.x + rect.w && y < rect.y + rect.h;
  }
}
