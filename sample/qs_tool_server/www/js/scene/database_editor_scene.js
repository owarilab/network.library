/**
 * DatabaseEditorScene
 * project.database（const / mutable のテーブル）と、システム DB（読み取り専用）を表で編集する。
 *
 * 画面: 左にテーブルの一覧、中央にグリッド、右に Inspector（テーブル・列・行の設定）。
 * 編集の処理は database_editor_actions.js（同じクラスにメソッドを足す）にある。
 * Undo / Redo はこのシーン専用の HistoryManager を使う。
 */
class DatabaseEditorScene extends Scene {
  static NAV_WIDTH = 224;
  static INSPECTOR_WIDTH = 300;
  static HEADER_HEIGHT = 52;
  static TOOLBAR_HEIGHT = 44;
  static STATUS_HEIGHT = 28;
  static KEY_COLUMN_ID = '__key';

  constructor() {
    super();
    this._appData = null;
    this._input = null;
    this._history = new HistoryManager(200);
    this._grid = new GridView();
    this._editor = new CellEditor('mainCanvas');
    this._popup = new PopupList();

    /** @type {{ kind: 'const'|'mutable'|'system', id: string }} */
    this._current = { kind: 'const', id: '' };
    this._filterText = '';
    this._viewRows = [];          // 表示する行の、table.rows 内の位置
    this._columnWidths = new Map(); // 列 ID → 幅
    this._issues = new Map();     // `${table}|${rowKey}|${column}` → 理由
    this._issueList = [];
    this._validationDirty = true;
    this._usage = { key: '', items: [] };
    this._inspectorItems = [];
    this._inspectorScroll = 0;
    this._buttons = [];
    this._navItems = [];
    this._navScroll = 0;
    this._hover = '';
    this._layout = null;
    this._editing = null;         // { target: 'cell'|'field'|'search', viewRow, gridCol }
    this._clipboardText = '';
    this._statusMessage = 'Select a table on the left, or create one with + New';
    this._statusTone = 'muted';
    this._lastClipboardSummary = '';

    this._onMouseMove = this._onMouseMove.bind(this);
    this._onMouseDown = this._onMouseDown.bind(this);
    this._onWindowMouseUp = this._onWindowMouseUp.bind(this);
    this._onWheel = this._onWheel.bind(this);
    this._onKeyDown = this._onKeyDown.bind(this);
    this._onContextMenu = this._onContextMenu.bind(this);
    this._onCopy = this._onCopy.bind(this);
    this._onCut = this._onCut.bind(this);
    this._onPaste = this._onPaste.bind(this);

    this._grid.onSelectionChange = () => this._onGridSelectionChange();
    this._grid.onEditRequest = (row, col, text) => this._beginCellEdit(row, col, text);
    this._grid.onToggle = (row, col) => this._toggleBoolean(row, col);
    this._grid.onClear = () => this._clearSelection();
    this._grid.onColumnResize = (col, width) => this._setColumnWidth(col, width);
    this._grid.onContextMenu = (hit, x, y) => this._openContextMenu(hit, x, y);
    this._grid.onMoveRows = (delta) => this._moveSelectedRows(delta);
    this._grid.onMoveColumns = (delta) => this._moveFocusColumn(delta);
  }

  // ---------------------------------------------------------------------------
  // ライフサイクル
  // ---------------------------------------------------------------------------

  onEnter(input, appData) {
    this._appData = appData;
    this._input = input;
    this._history = new HistoryManager(200);
    if (!appData?.currentProject) return;
    this._ensureCurrentTable();
    this._rebuildView();
    this._validationDirty = true;
    input.on('mousemove', this._onMouseMove);
    input.on('mousedown', this._onMouseDown);
    input.on('wheel', this._onWheel);
    input.on('keydown', this._onKeyDown);
    input.on('contextmenu', this._onContextMenu);
    window.addEventListener('mouseup', this._onWindowMouseUp);
    document.addEventListener('copy', this._onCopy);
    document.addEventListener('cut', this._onCut);
    document.addEventListener('paste', this._onPaste);
  }

  onLeave() {
    this._editor.destroy();
    this._popup.close();
    window.removeEventListener('mouseup', this._onWindowMouseUp);
    document.removeEventListener('copy', this._onCopy);
    document.removeEventListener('cut', this._onCut);
    document.removeEventListener('paste', this._onPaste);
    const canvas = document.getElementById('mainCanvas');
    if (canvas) canvas.style.cursor = 'default';
    this._buttons = [];
    this._navItems = [];
    this._inspectorItems = [];
  }

  // ---------------------------------------------------------------------------
  // データの参照
  // ---------------------------------------------------------------------------

  get _project() {
    return this._appData?.currentProject || null;
  }

  /** 現在選択中のテーブル（無ければ null）。 */
  _getCurrentTable() {
    const project = this._project;
    if (!project) return null;
    if (this._current.kind === 'system') {
      return SystemDatabase.getDatabase().tables.find(table => table.id === this._current.id) || null;
    }
    const table = DatabaseData.findTableById(project.database, this._current.id);
    // 種類（const / mutable）を変えた直後でも、同じテーブルを指し続ける
    if (table && table.kind !== this._current.kind) this._current.kind = table.kind;
    return table || null;
  }

  _isReadOnly() {
    return this._current.kind === 'system';
  }

  /** 選択が無効なら、先頭のテーブルへ寄せる。 */
  _ensureCurrentTable() {
    if (this._getCurrentTable()) return;
    const first = this._project?.database.tables[0];
    if (first) this._current = { kind: first.kind, id: first.id };
    else this._current = { kind: 'const', id: '' };
  }

  // ---------------------------------------------------------------------------
  // 描画
  // ---------------------------------------------------------------------------

  render(ctx, canvas) {
    const colors = GridTheme.COLORS;
    if (!this._project) {
      ctx.fillStyle = colors.bg;
      ctx.fillRect(0, 0, canvas.width, canvas.height);
      ctx.fillStyle = colors.muted;
      ctx.font = GridTheme.FONT;
      ctx.fillText('No project is open', 24, 40);
      return;
    }
    if (this._validationDirty) this._runValidation();

    const layout = this._createLayout(canvas);
    this._layout = layout;
    this._grid.setRect(layout.grid);
    this._buttons = this._buildButtons(layout);
    this._navItems = this._buildNavItems(layout.nav);
    this._inspectorItems = layout.inspector.w > 0 ? this._buildInspectorItems(layout.inspector) : [];
    this._syncEditorPosition();

    ctx.fillStyle = colors.bg;
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    this._drawHeader(ctx, layout);
    this._drawToolbar(ctx, layout);
    this._drawNav(ctx, layout.nav);
    if (this._getCurrentTable()) this._grid.render(ctx);
    else this._drawEmptyState(ctx, layout.grid);
    if (layout.inspector.w > 0) this._drawInspector(ctx, layout.inspector);
    this._drawStatusBar(ctx, layout);
    this._popup.render(ctx, canvas);
  }

  _createLayout(canvas) {
    const W = canvas.width;
    const H = canvas.height;
    const header = { x: 0, y: 0, w: W, h: DatabaseEditorScene.HEADER_HEIGHT };
    const toolbar = { x: 0, y: header.h, w: W, h: DatabaseEditorScene.TOOLBAR_HEIGHT };
    const status = { x: 0, y: H - DatabaseEditorScene.STATUS_HEIGHT, w: W, h: DatabaseEditorScene.STATUS_HEIGHT };
    const bodyY = toolbar.y + toolbar.h;
    const bodyH = Math.max(0, status.y - bodyY);
    const navW = W < 760 ? 150 : DatabaseEditorScene.NAV_WIDTH;
    const inspectorW = W >= 1040 ? DatabaseEditorScene.INSPECTOR_WIDTH : 0;
    const nav = { x: 0, y: bodyY, w: navW, h: bodyH };
    const inspector = { x: W - inspectorW, y: bodyY, w: inspectorW, h: bodyH };
    const grid = { x: navW + 1, y: bodyY, w: Math.max(0, W - navW - inspectorW - 1), h: bodyH };
    return { W, H, header, toolbar, status, nav, grid, inspector };
  }

  _drawHeader(ctx, layout) {
    const colors = GridTheme.COLORS;
    ctx.fillStyle = colors.panel;
    ctx.fillRect(0, 0, layout.W, layout.header.h);
    ctx.textBaseline = 'middle';
    ctx.textAlign = 'left';
    ctx.fillStyle = colors.text;
    ctx.font = 'bold 18px sans-serif';
    ctx.fillText('Database Editor', 20, 22);
    ctx.font = GridTheme.FONT_SMALL;
    ctx.fillStyle = colors.muted;
    ctx.fillText(`${this._project.name || 'Project'}${this._appData.projectSession?.dirty ? '  •  unsaved changes' : ''}`, 20, 40);
    for (const button of this._buttons.filter(b => b.area === 'header')) this._drawButton(ctx, button);
  }

  _drawToolbar(ctx, layout) {
    const colors = GridTheme.COLORS;
    ctx.fillStyle = colors.panelAlt;
    ctx.fillRect(0, layout.toolbar.y, layout.W, layout.toolbar.h);
    ctx.strokeStyle = colors.border;
    ctx.beginPath();
    ctx.moveTo(0, layout.toolbar.y + layout.toolbar.h - 0.5);
    ctx.lineTo(layout.W, layout.toolbar.y + layout.toolbar.h - 0.5);
    ctx.stroke();
    for (const button of this._buttons.filter(b => b.area === 'toolbar')) this._drawButton(ctx, button);

    const search = layout.search;
    if (search) {
      const active = this._editing?.target === 'search';
      GridTheme.roundRect(ctx, search.x, search.y, search.w, search.h, 5);
      ctx.fillStyle = colors.bg;
      ctx.fill();
      ctx.strokeStyle = active ? colors.accent : colors.border;
      ctx.stroke();
      ctx.font = GridTheme.FONT;
      ctx.textAlign = 'left';
      ctx.textBaseline = 'middle';
      ctx.fillStyle = this._filterText ? colors.text : colors.muted;
      ctx.fillText(GridTheme.fitText(ctx, this._filterText || 'Search rows (Ctrl+F)', search.w - 16), search.x + 8, search.y + search.h / 2 + 0.5);
    }
  }

  _drawButton(ctx, button) {
    GridTheme.drawButton(ctx, button.rect, button.label, {
      hover: this._hover === `button:${button.id}`,
      disabled: button.enabled === false,
      active: button.active === true,
    });
  }

  _drawNav(ctx, rect) {
    const colors = GridTheme.COLORS;
    ctx.save();
    ctx.beginPath();
    ctx.rect(rect.x, rect.y, rect.w, rect.h);
    ctx.clip();
    ctx.fillStyle = colors.panel;
    ctx.fillRect(rect.x, rect.y, rect.w, rect.h);
    ctx.textBaseline = 'middle';
    for (const item of this._navItems) {
      const r = item.rect;
      if (item.type === 'section') {
        ctx.font = GridTheme.FONT_SMALL;
        ctx.textAlign = 'left';
        ctx.fillStyle = colors.muted;
        ctx.fillText(item.label, r.x + 12, r.y + r.h / 2);
        continue;
      }
      const hover = this._hover === `nav:${item.key}`;
      if (item.selected) {
        ctx.fillStyle = colors.selectionFill;
        ctx.fillRect(r.x, r.y, r.w, r.h);
        ctx.fillStyle = colors.accent;
        ctx.fillRect(r.x, r.y, 3, r.h);
      } else if (hover) {
        ctx.fillStyle = colors.headerHover;
        ctx.fillRect(r.x, r.y, r.w, r.h);
      }
      ctx.font = GridTheme.FONT;
      ctx.textAlign = 'left';
      if (item.type === 'new') {
        ctx.fillStyle = colors.accent;
        ctx.fillText(item.label, r.x + 16, r.y + r.h / 2 + 0.5);
      } else {
        ctx.fillStyle = item.readOnly ? colors.muted : colors.text;
        ctx.fillText(GridTheme.fitText(ctx, item.label, r.w - 56), r.x + 16, r.y + r.h / 2 + 0.5);
        ctx.font = GridTheme.FONT_SMALL;
        ctx.fillStyle = colors.muted;
        ctx.textAlign = 'right';
        ctx.fillText(String(item.count), r.x + r.w - 12, r.y + r.h / 2 + 0.5);
        if (item.errors) {
          ctx.fillStyle = colors.errorBorder;
          ctx.beginPath();
          ctx.arc(r.x + r.w - 40, r.y + r.h / 2, 4, 0, Math.PI * 2);
          ctx.fill();
        }
      }
    }
    ctx.restore();
    ctx.strokeStyle = colors.border;
    ctx.beginPath();
    ctx.moveTo(rect.x + rect.w + 0.5, rect.y);
    ctx.lineTo(rect.x + rect.w + 0.5, rect.y + rect.h);
    ctx.stroke();
  }

  _drawEmptyState(ctx, rect) {
    const colors = GridTheme.COLORS;
    ctx.fillStyle = colors.muted;
    ctx.font = GridTheme.FONT;
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillText('No table yet. Click “+ New const table” or “+ New mutable table” on the left.', rect.x + rect.w / 2, rect.y + rect.h / 2);
  }

  _drawInspector(ctx, rect) {
    const colors = GridTheme.COLORS;
    ctx.save();
    ctx.beginPath();
    ctx.rect(rect.x, rect.y, rect.w, rect.h);
    ctx.clip();
    ctx.fillStyle = colors.panel;
    ctx.fillRect(rect.x, rect.y, rect.w, rect.h);
    ctx.textBaseline = 'middle';
    for (const item of this._inspectorItems) {
      const r = item.rect;
      if (r.y + r.h < rect.y || r.y > rect.y + rect.h) continue;
      if (item.type === 'header') {
        ctx.font = GridTheme.FONT_BOLD;
        ctx.textAlign = 'left';
        ctx.fillStyle = colors.accent;
        ctx.fillText(item.label, r.x + 12, r.y + r.h / 2);
        ctx.strokeStyle = colors.border;
        ctx.beginPath();
        ctx.moveTo(r.x + 12, r.y + r.h - 2.5);
        ctx.lineTo(r.x + r.w - 12, r.y + r.h - 2.5);
        ctx.stroke();
      } else if (item.type === 'field') {
        const hover = this._hover === `inspector:${item.id}`;
        if (hover && item.action) {
          ctx.fillStyle = colors.headerHover;
          ctx.fillRect(r.x + 4, r.y, r.w - 8, r.h);
        }
        ctx.font = GridTheme.FONT_SMALL;
        ctx.textAlign = 'left';
        ctx.fillStyle = colors.muted;
        ctx.fillText(item.label, r.x + 12, r.y + r.h / 2);
        ctx.font = item.mono ? GridTheme.FONT_MONO : GridTheme.FONT;
        ctx.fillStyle = item.action ? (item.error ? '#fecaca' : colors.text) : colors.muted;
        ctx.textAlign = 'left';
        ctx.fillText(GridTheme.fitText(ctx, item.value, r.w - 112), r.x + 100, r.y + r.h / 2 + 0.5);
      } else if (item.type === 'button') {
        GridTheme.drawButton(ctx, { x: r.x + 12, y: r.y + 2, w: r.w - 24, h: r.h - 4 }, item.label, {
          hover: this._hover === `inspector:${item.id}`,
          disabled: item.enabled === false,
        });
      } else if (item.type === 'note') {
        ctx.font = GridTheme.FONT_SMALL;
        ctx.textAlign = 'left';
        ctx.fillStyle = item.tone === 'warning' ? colors.warning : item.tone === 'error' ? '#fca5a5' : colors.muted;
        const lines = item.lines || [item.label];
        lines.forEach((line, index) => ctx.fillText(GridTheme.fitText(ctx, line, r.w - 24), r.x + 12, r.y + 9 + index * 15));
      }
    }
    ctx.restore();
    ctx.strokeStyle = colors.border;
    ctx.beginPath();
    ctx.moveTo(rect.x + 0.5, rect.y);
    ctx.lineTo(rect.x + 0.5, rect.y + rect.h);
    ctx.stroke();
  }

  _drawStatusBar(ctx, layout) {
    const colors = GridTheme.COLORS;
    const r = layout.status;
    ctx.fillStyle = colors.panelAlt;
    ctx.fillRect(r.x, r.y, r.w, r.h);
    ctx.strokeStyle = colors.border;
    ctx.beginPath();
    ctx.moveTo(0, r.y + 0.5);
    ctx.lineTo(r.w, r.y + 0.5);
    ctx.stroke();
    ctx.font = GridTheme.FONT_SMALL;
    ctx.textBaseline = 'middle';

    const table = this._getCurrentTable();
    const issueCount = this._issueList.length;
    const right = [];
    if (table) right.push(`${table.rows.length} rows`, `${table.columns.length} cols`);
    if (table && this._viewRows.length !== table.rows.length) right.push(`showing ${this._viewRows.length}`);
    right.push(issueCount ? `${issueCount} issue${issueCount > 1 ? 's' : ''}` : 'no issues');
    const rightText = right.join('  •  ');
    ctx.textAlign = 'right';
    ctx.fillStyle = issueCount ? colors.warning : colors.muted;
    ctx.fillText(rightText, r.x + r.w - 12, r.y + r.h / 2 + 0.5);
    const rightWidth = ctx.measureText(rightText).width;

    ctx.textAlign = 'left';
    ctx.fillStyle = this._statusTone === 'error' ? '#fca5a5' : this._statusTone === 'warning' ? colors.warning : this._statusTone === 'ok' ? colors.ok : colors.muted;
    ctx.fillText(GridTheme.fitText(ctx, this._statusMessage, r.w - rightWidth - 40), r.x + 12, r.y + r.h / 2 + 0.5);
  }

  // ---------------------------------------------------------------------------
  // ボタンとナビの配置
  // ---------------------------------------------------------------------------

  _buildButtons(layout) {
    const buttons = [];
    const readOnly = this._isReadOnly();
    const hasTable = !!this._getCurrentTable();
    const sel = this._grid.getSelection();
    const hasSelection = hasTable && this._viewRows.length > 0;
    void sel;

    // ヘッダの右側（右から並べる）
    let hx = layout.W - 16;
    const headerButtons = [
      { id: 'save', label: 'Save', w: 76, action: () => this._saveProject() },
      { id: 'back', label: 'Back', w: 76, action: () => this._goBack() },
    ];
    for (const def of headerButtons) {
      hx -= def.w;
      buttons.push({ ...def, area: 'header', rect: { x: hx, y: 12, w: def.w, h: 30 } });
      hx -= 8;
    }

    // ツールバー
    const defs = [
      { id: 'addRow', label: '+ Row', w: 64, enabled: hasTable && !readOnly, action: () => this._addRow('after') },
      { id: 'addColumn', label: '+ Column', w: 84, enabled: hasTable && !readOnly, action: () => this._addColumn('after') },
      { id: 'duplicate', label: 'Duplicate', w: 84, enabled: hasSelection && !readOnly, action: () => this._duplicateSelectedRows() },
      { id: 'deleteRows', label: 'Delete Rows', w: 100, enabled: hasSelection && !readOnly, action: () => this._deleteSelectedRows() },
      { id: 'sep1', sep: true, w: 10 },
      { id: 'undo', label: 'Undo', w: 64, enabled: this._history.canUndo(), action: () => this._undo() },
      { id: 'redo', label: 'Redo', w: 64, enabled: this._history.canRedo(), action: () => this._redo() },
    ];
    let tx = 12;
    const ty = layout.toolbar.y + 7;
    for (const def of defs) {
      if (!def.sep) buttons.push({ ...def, area: 'toolbar', rect: { x: tx, y: ty, w: def.w, h: 30 } });
      tx += def.w + 8;
    }
    const searchW = Math.max(120, Math.min(320, layout.W - tx - 16));
    layout.search = { x: tx + 8, y: ty, w: searchW, h: 30 };
    return buttons;
  }

  _buildNavItems(rect) {
    const items = [];
    const project = this._project;
    const issueTables = new Set(this._issueList.map(issue => issue.tableName));
    let y = rect.y + 8 - this._navScroll;
    const add = (item, h) => {
      items.push({ ...item, rect: { x: rect.x, y, w: rect.w, h } });
      y += h;
    };
    const section = (label) => add({ type: 'section', label }, 26);
    const tableRow = (kind, table, readOnly) => add({
      type: 'table',
      key: `${kind}:${table.id}`,
      kind,
      tableId: table.id,
      label: table.label ? `${table.name}  ${table.label}` : table.name,
      count: table.rows.length,
      readOnly,
      errors: !readOnly && issueTables.has(table.name),
      selected: this._current.kind === kind && this._current.id === table.id,
    }, 30);

    section('CONST  (read-only at runtime)');
    for (const table of project.database.tables.filter(t => t.kind === 'const')) tableRow('const', table, false);
    add({ type: 'new', key: 'new:const', kind: 'const', label: '+ New const table' }, 28);
    y += 8;
    section('MUTABLE  (saved state)');
    for (const table of project.database.tables.filter(t => t.kind === 'mutable')) tableRow('mutable', table, false);
    add({ type: 'new', key: 'new:mutable', kind: 'mutable', label: '+ New mutable table' }, 28);
    y += 8;
    section('SYSTEM  (built in, read-only)');
    for (const table of SystemDatabase.getDatabase().tables) tableRow('system', table, true);
    return items;
  }

  // ---------------------------------------------------------------------------
  // 入力
  // ---------------------------------------------------------------------------

  _onMouseMove(e) {
    if (this._popup.isOpen) {
      this._popup.onMouseMove(e.x, e.y);
      return;
    }
    if (this._grid.onMouseMove(e.x, e.y)) {
      this._setCursor(this._grid.getCursor(e.x, e.y));
      return;
    }
    this._hover = this._hitHover(e.x, e.y);
    this._setCursor(this._getCurrentTable() ? this._grid.getCursor(e.x, e.y) : 'default');
  }

  _onWindowMouseUp() {
    this._grid.onMouseUp();
  }

  _setCursor(cursor) {
    const canvas = document.getElementById('mainCanvas');
    if (canvas && canvas.style.cursor !== cursor) canvas.style.cursor = cursor;
  }

  /** ボタン・ナビ・Inspector のうち、座標が指しているものの ID。 */
  _hitHover(x, y) {
    for (const button of this._buttons) {
      if (button.enabled !== false && GridTheme.inRect(button.rect, x, y)) return `button:${button.id}`;
    }
    for (const item of this._navItems) {
      if ((item.type === 'table' || item.type === 'new') && GridTheme.inRect(item.rect, x, y)) return `nav:${item.key}`;
    }
    for (const item of this._inspectorItems) {
      if ((item.type === 'field' || item.type === 'button') && item.action && GridTheme.inRect(item.rect, x, y)) return `inspector:${item.id}`;
    }
    return '';
  }

  _onMouseDown(e) {
    if (e.button !== 0) return;
    if (this._popup.isOpen) {
      this._popup.onMouseDown(e.x, e.y);
      return;
    }
    this._editor.commit();

    for (const button of this._buttons) {
      if (GridTheme.inRect(button.rect, e.x, e.y)) {
        if (button.enabled !== false) button.action();
        return;
      }
    }
    if (this._layout?.search && GridTheme.inRect(this._layout.search, e.x, e.y)) {
      this._beginSearchEdit();
      return;
    }
    for (const item of this._navItems) {
      if (!GridTheme.inRect(item.rect, e.x, e.y)) continue;
      if (item.type === 'table') this._selectTable(item.kind, item.tableId);
      else if (item.type === 'new') this._createTable(item.kind);
      return;
    }
    for (const item of this._inspectorItems) {
      if ((item.type === 'field' || item.type === 'button') && item.action && item.enabled !== false && GridTheme.inRect(item.rect, e.x, e.y)) {
        item.action(item.rect);
        return;
      }
    }
    if (this._getCurrentTable()) this._grid.onMouseDown(e.x, e.y, e);
  }

  _onContextMenu(e) {
    if (this._popup.isOpen || !this._getCurrentTable()) return;
    this._grid.onContextMenuAt(e.x, e.y);
  }

  _onWheel(e) {
    if (this._popup.isOpen) {
      this._popup.onWheel(e.deltaY);
      return;
    }
    this._editor.commit();
    if (this._layout && GridTheme.inRect(this._layout.nav, e.x, e.y)) {
      this._navScroll = Math.max(0, this._navScroll + Math.sign(e.deltaY) * 40);
      return;
    }
    if (this._layout && GridTheme.inRect(this._layout.inspector, e.x, e.y)) {
      this._inspectorScroll = Math.max(0, this._inspectorScroll + Math.sign(e.deltaY) * 40);
      return;
    }
    if (this._getCurrentTable()) this._grid.onWheel(e.x, e.y, e.deltaX || 0, e.deltaY || 0, !!e.raw?.shiftKey);
  }

  _onKeyDown(e) {
    const raw = e.raw;
    if (this._popup.isOpen) {
      this._popup.onKeyDown(e);
      raw?.preventDefault();
      return;
    }
    const primary = e.ctrl || e.meta;
    if (primary && !e.alt) {
      if (e.code === 'KeyS') { raw?.preventDefault(); this._saveProject(); return; }
      if (e.code === 'KeyZ') { raw?.preventDefault(); if (e.shift) this._redo(); else this._undo(); return; }
      if (e.code === 'KeyY') { raw?.preventDefault(); this._redo(); return; }
      if (e.code === 'KeyF') { raw?.preventDefault(); this._beginSearchEdit(); return; }
      // コピー・切り取り・貼り付けは、ブラウザの copy / cut / paste イベントで受ける（ここで止めない）
      if (e.code === 'KeyC' || e.code === 'KeyX' || e.code === 'KeyV') return;
    }
    if (e.code === 'Escape') {
      if (this._filterText) { this._setFilter(''); return; }
      this._goBack();
      return;
    }
    if (this._getCurrentTable() && this._grid.onKeyDown(e)) raw?.preventDefault();
  }

  _goBack() {
    this._editor.commit();
    this._appData.syncEditorStateToProjectSession?.();
    this._appData.changeScene(new ProjectTopScene());
  }
}
