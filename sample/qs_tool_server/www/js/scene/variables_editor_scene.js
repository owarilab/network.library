/**
 * VariablesSnapshotCommand
 * グローバル変数定義（project.globalVariables）全体の前後のスナップショットを切り替える Undo / Redo コマンド。
 * 変数定義は小さいので、差分ではなく全体を保存する。
 */
class VariablesSnapshotCommand extends CommandBase {
  /**
   * @param {ProjectData} project
   * @param {object} before  変更前の globalVariables
   * @param {object} after   変更後の globalVariables
   * @param {string} label
   */
  constructor(project, before, after, label) {
    super();
    this._project = project;
    this._before = JSON.parse(JSON.stringify(before));
    this._after = JSON.parse(JSON.stringify(after));
    this._label = label;
  }

  execute() {
    this._project.globalVariables = JSON.parse(JSON.stringify(this._after));
    return true;
  }

  undo() {
    this._project.globalVariables = JSON.parse(JSON.stringify(this._before));
    return true;
  }

  getLabel() {
    return this._label;
  }
}

/**
 * VariablesEditorScene
 * プロジェクトのグローバル変数定義（system / user × fixed / persistent）を表で編集する。
 * Database Editor と同じ表の部品（GridView、CellEditor、PopupList）の上に作っている。
 * 列: name（固定）/ type / initial value / description。初期値のセルは変数の型に合わせて編集する。
 */
class VariablesEditorScene extends Scene {
  static SCOPES = ['system.fixed', 'system.persistent', 'user.fixed', 'user.persistent'];
  static TYPES = ['string', 'number', 'boolean', 'json'];
  /** 実行時が参照する名前（消したり改名したりすると動作が変わるので、警告を出す） */
  static RUNTIME_NAMES = new Set([
    'system.fixed.timer', 'system.fixed.startupPlayUnitId', 'system.fixed.requestedPlayUnitId',
    'system.fixed.currentPlayUnitId', 'system.fixed.returnPlayUnitId', 'system.fixed.isPaused',
    'system.persistent.masterVolume',
  ]);
  static COLUMNS = [
    { id: 'name', title: 'name', badge: 'identifier', width: 200 },
    { id: 'type', title: 'type', badge: 'string / number / boolean / json', width: 110 },
    { id: 'initialValue', title: 'initial value', badge: 'matches the type', width: 280 },
    { id: 'description', title: 'description', badge: 'text', width: 360 },
  ];

  constructor() {
    super();
    this._appData = null;
    this._history = new HistoryManager(200);
    this._grid = new GridView();
    this._editor = new CellEditor('mainCanvas');
    this._popup = new PopupList();
    this._scope = 'system.fixed';
    this._filterText = '';
    this._viewRows = [];
    this._buttons = [];
    this._navItems = [];
    this._hover = '';
    this._layout = null;
    this._editing = null;
    this._statusMessage = 'Edit variable definitions. They are the initial values at the start of Play Test';
    this._statusTone = 'muted';

    this._onMouseMove = this._onMouseMove.bind(this);
    this._onMouseDown = this._onMouseDown.bind(this);
    this._onWindowMouseUp = () => this._grid.onMouseUp();
    this._onWheel = this._onWheel.bind(this);
    this._onKeyDown = this._onKeyDown.bind(this);
    this._onContextMenu = this._onContextMenu.bind(this);
    this._onCopy = this._onCopy.bind(this);
    this._onCut = this._onCut.bind(this);
    this._onPaste = this._onPaste.bind(this);

    this._grid.onEditRequest = (row, col, text) => this._beginEdit(row, col, text);
    this._grid.onToggle = (row, col) => this._toggleBoolean(row, col);
    this._grid.onClear = () => this._clearSelection();
    this._grid.onColumnResize = (col, width) => { VariablesEditorScene.COLUMNS[col].width = width; };
    this._grid.onContextMenu = (hit, x, y) => this._openContextMenu(hit, x, y);
    this._grid.onMoveRows = (delta) => this._moveSelectedRows(delta);
    this._grid.onSelectionChange = () => this._onSelectionChange();
  }

  // ---------------------------------------------------------------------------
  // ライフサイクル
  // ---------------------------------------------------------------------------

  onEnter(input, appData) {
    this._appData = appData;
    this._history = new HistoryManager(200);
    if (!appData?.currentProject) return;
    this._ensureProjectGlobalVariables();
    this._rebuildView();
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
  }

  get _project() {
    return this._appData?.currentProject || null;
  }

  _ensureProjectGlobalVariables() {
    this._project.globalVariables = ProjectData.normalizeGlobalVariables(this._project.globalVariables);
  }

  // ---------------------------------------------------------------------------
  // データ
  // ---------------------------------------------------------------------------

  /** 選択中のスコープの変数の一覧: [{ name, definition }]（定義の順）。 */
  _entries(globalVariables = this._project.globalVariables) {
    const [scope, tier] = this._scope.split('.');
    return Object.entries(globalVariables[scope][tier]).map(([name, definition]) => ({ name, definition }));
  }

  _rebuildView() {
    const query = this._filterText.trim().toLowerCase();
    const entries = this._entries();
    this._viewRows = [];
    entries.forEach((entry, index) => {
      const text = `${entry.name} ${entry.definition.type} ${entry.definition.description || ''} ${VariablesEditorScene._valueToText(entry.definition.initialValue, entry.definition.type)}`.toLowerCase();
      if (!query || text.includes(query)) this._viewRows.push(index);
    });
    this._grid.setModel(this._buildGridModel());
  }

  static _valueToText(value, type) {
    if (type === 'json') return JSON.stringify(value ?? null);
    if (value === null || value === undefined) return '';
    return String(value);
  }

  _buildGridModel() {
    const entries = this._entries();
    return {
      columns: VariablesEditorScene.COLUMNS.map(column => ({ ...column })),
      frozenColumns: 1,
      rowCount: this._viewRows.length,
      getRowLabel: (viewRow) => String(this._viewRows[viewRow] + 1),
      getCell: (viewRow, col) => {
        const entry = entries[this._viewRows[viewRow]];
        if (!entry) return { text: '' };
        const { definition } = entry;
        switch (col) {
          case 0: return { text: entry.name };
          case 1: return { text: definition.type, muted: true };
          case 2:
            if (definition.type === 'boolean') return { kind: 'checkbox', checked: definition.initialValue === true };
            return { text: VariablesEditorScene._valueToText(definition.initialValue, definition.type), mono: definition.type === 'json' };
          default: return { text: definition.description || '', muted: !definition.description };
        }
      },
    };
  }

  _setStatus(message, tone = 'muted') {
    this._statusMessage = message;
    this._statusTone = tone;
  }

  // ---------------------------------------------------------------------------
  // 変更（スナップショット方式）
  // ---------------------------------------------------------------------------

  /**
   * 作業用のコピーを mutator に渡し、変わったら 1 回の Undo にまとめて適用する。
   * @param {string} label
   * @param {(globalVariables: object, bucket: object) => boolean|void} mutator  false で中止
   * @returns {boolean}
   */
  _apply(label, mutator) {
    const before = this._project.globalVariables;
    const working = JSON.parse(JSON.stringify(before));
    const [scope, tier] = this._scope.split('.');
    if (mutator(working, working[scope][tier]) === false) return false;
    if (JSON.stringify(working) === JSON.stringify(before)) return false;
    if (!this._history.execute(new VariablesSnapshotCommand(this._project, before, working, label), this._appData)) return false;
    this._afterEdit();
    return true;
  }

  _afterEdit() {
    this._project.touch?.();
    this._appData.projectSession?.markDirty?.();
    this._appData.resetRuntimeGlobalVariables?.();
    this._rebuildView();
  }

  _undo() {
    this._editor.commit();
    const label = this._history.getUndoLabel?.() || '';
    if (this._history.undo(this._appData)) {
      this._afterEdit();
      this._setStatus(`Undo: ${label}`);
    }
  }

  _redo() {
    this._editor.commit();
    const label = this._history.getRedoLabel?.() || '';
    if (this._history.redo(this._appData)) {
      this._afterEdit();
      this._setStatus(`Redo: ${label}`);
    }
  }

  /** 名前の検証。問題があれば理由を返す（無ければ ''）。 */
  _checkName(name, bucket, selfName = null) {
    if (!name) return 'Variable name is required';
    if (/\s/.test(name)) return 'Variable name cannot contain spaces';
    if (/[.${}]/.test(name)) return 'Variable name cannot contain . $ { } (they are used in paths and templates)';
    if (name !== selfName && Object.prototype.hasOwnProperty.call(bucket, name)) return `Variable already exists: ${name}`;
    return '';
  }

  /** 順序を保ったまま、キーの名前だけ変えたオブジェクトを返す。 */
  static _renameKey(bucket, oldName, newName) {
    const result = {};
    for (const [name, definition] of Object.entries(bucket)) result[name === oldName ? newName : name] = definition;
    return result;
  }

  /**
   * セルの値をテキストから適用する（編集と貼り付けで共通）。
   * @returns {{ ok: boolean, message?: string }}
   */
  _applyCellText(working, bucket, name, col, text) {
    const definition = bucket[name];
    if (!definition) return { ok: false, message: 'variable not found' };
    const [scope, tier] = this._scope.split('.');
    switch (col) {
      case 0: {
        const newName = text.trim();
        if (newName === name) return { ok: true, name };
        const problem = this._checkName(newName, bucket, name);
        if (problem) return { ok: false, message: problem };
        working[scope][tier] = VariablesEditorScene._renameKey(bucket, name, newName);
        return { ok: true, name: newName };
      }
      case 1: {
        const type = text.trim().toLowerCase();
        if (!VariablesEditorScene.TYPES.includes(type)) return { ok: false, message: `type must be one of: ${VariablesEditorScene.TYPES.join(', ')}` };
        if (type !== definition.type) {
          definition.initialValue = ProjectData.normalizeGlobalVariableInitialValue(definition.initialValue, type);
          definition.type = type;
        }
        return { ok: true, name };
      }
      case 2: {
        const result = VariablesEditorScene._parseValue(text, definition.type);
        if (!result.ok) return result;
        definition.initialValue = result.value;
        return { ok: true, name };
      }
      default:
        definition.description = text;
        return { ok: true, name };
    }
  }

  static _parseValue(text, type) {
    switch (type) {
      case 'number': {
        const trimmed = String(text).trim();
        const value = trimmed === '' ? NaN : Number(trimmed);
        return Number.isFinite(value) ? { ok: true, value } : { ok: false, message: 'initial value must be a finite number' };
      }
      case 'boolean': {
        const normalized = String(text).trim().toLowerCase();
        if (normalized === 'true' || normalized === '1') return { ok: true, value: true };
        if (normalized === 'false' || normalized === '0' || normalized === '') return { ok: true, value: false };
        return { ok: false, message: 'initial value must be true or false' };
      }
      case 'json': {
        if (String(text).trim() === '') return { ok: true, value: null };
        try {
          return { ok: true, value: JSON.parse(text) };
        } catch {
          return { ok: false, message: 'initial value must be valid JSON' };
        }
      }
      default:
        return { ok: true, value: String(text) };
    }
  }

  _warnRuntimeName(name, verb) {
    if (VariablesEditorScene.RUNTIME_NAMES.has(`${this._scope}.${name}`)) {
      this._setStatus(`${this._scope}.${name} is used by the runtime. ${verb} may change how Play Test behaves`, 'warning');
    }
  }

  // ---------------------------------------------------------------------------
  // 操作
  // ---------------------------------------------------------------------------

  _selectedEntryNames() {
    const entries = this._entries();
    const sel = this._grid.getSelection();
    const names = [];
    for (let viewRow = sel.top; viewRow <= sel.bottom; viewRow++) {
      const entry = entries[this._viewRows[viewRow]];
      if (entry) names.push(entry.name);
    }
    return names;
  }

  _addVariable() {
    this._editor.commit();
    let created = '';
    const applied = this._apply('Add variable', (working, bucket) => {
      let name = 'variable';
      for (let n = 2; Object.prototype.hasOwnProperty.call(bucket, name); n++) name = `variable${n}`;
      bucket[name] = { type: 'string', initialValue: '', description: '' };
      created = name;
    });
    if (applied) {
      if (this._filterText) this._setFilter('');
      const index = this._entries().findIndex(entry => entry.name === created);
      this._grid.select(this._viewRows.indexOf(index), 0);
      this._setStatus('Variable added. Press Enter to rename it', 'ok');
    }
  }

  _deleteSelected() {
    this._editor.commit();
    const names = this._selectedEntryNames();
    if (!names.length) return;
    const applied = this._apply('Delete variables', (working, bucket) => { for (const name of names) delete bucket[name]; });
    if (applied) {
      this._grid.select(Math.min(this._grid.getSelection().top, Math.max(0, this._viewRows.length - 1)), 0);
      const runtime = names.filter(name => VariablesEditorScene.RUNTIME_NAMES.has(`${this._scope}.${name}`));
      this._setStatus(runtime.length
        ? `Deleted ${names.length}. ${runtime.map(name => `${this._scope}.${name}`).join(', ')} is used by the runtime (Ctrl+Z to undo)`
        : `Deleted ${names.length} variable(s) (Ctrl+Z to undo)`, runtime.length ? 'warning' : 'ok');
    }
  }

  _moveSelectedRows(delta) {
    if (this._filterText) {
      this._setStatus('Clear the search filter to reorder variables', 'warning');
      return;
    }
    const names = this._selectedEntryNames();
    if (!names.length) return;
    const col = this._grid.getSelection().focusCol;
    const applied = this._apply('Move variables', (working, bucket) => {
      const order = Object.keys(bucket);
      const indices = names.map(name => order.indexOf(name)).sort((a, b) => a - b);
      if ((delta < 0 && indices[0] === 0) || (delta > 0 && indices[indices.length - 1] === order.length - 1)) return false;
      for (const index of (delta < 0 ? indices : [...indices].reverse())) {
        [order[index], order[index + delta]] = [order[index + delta], order[index]];
      }
      const [scope, tier] = this._scope.split('.');
      const next = {};
      for (const name of order) next[name] = bucket[name];
      working[scope][tier] = next;
    });
    if (applied) {
      const entries = this._entries();
      const rows = names.map(name => entries.findIndex(entry => entry.name === name));
      this._grid.selectRange(Math.min(...rows), col, Math.max(...rows), col);
    }
  }

  _toggleBoolean(viewRow, col) {
    const entry = this._entries()[this._viewRows[viewRow]];
    if (!entry || col !== 2 || entry.definition.type !== 'boolean') return;
    this._apply('Toggle initial value', (working, bucket) => { bucket[entry.name].initialValue = entry.definition.initialValue !== true; });
  }

  _clearSelection() {
    const sel = this._grid.getSelection();
    const entries = this._entries();
    const applied = this._apply('Clear cells', (working, bucket) => {
      for (let viewRow = sel.top; viewRow <= sel.bottom; viewRow++) {
        const entry = entries[this._viewRows[viewRow]];
        if (!entry) continue;
        if (sel.left <= 2 && sel.right >= 2) {
          // json の既定は null（normalize は {value} を返すので使わない）
          bucket[entry.name].initialValue = entry.definition.type === 'json'
            ? null
            : ProjectData.normalizeGlobalVariableInitialValue(undefined, entry.definition.type);
        }
        if (sel.right >= 3) bucket[entry.name].description = '';
      }
    });
    if (!applied) this._setStatus('Names and types cannot be cleared. Edit them, or delete the variable', 'warning');
  }

  // ---------------------------------------------------------------------------
  // セルの編集
  // ---------------------------------------------------------------------------

  _beginEdit(viewRow, col, initialText) {
    const entry = this._entries()[this._viewRows[viewRow]];
    if (!entry) return;
    const rect = this._grid.getCellRect(viewRow, col);
    if (col === 1) {
      this._popup.open({
        x: rect.x, y: rect.y + rect.h, w: rect.w, searchable: false, title: 'Type',
        items: VariablesEditorScene.TYPES.map(type => ({ value: type, label: type })),
        selectedValue: entry.definition.type,
        onPick: (type) => this._commitCell(viewRow, col, type, 'none'),
      });
      return;
    }
    if (col === 2 && entry.definition.type === 'boolean') {
      this._toggleBoolean(viewRow, col);
      return;
    }
    const text = initialText ?? (col === 0 ? entry.name : col === 3 ? entry.definition.description || ''
      : VariablesEditorScene._valueToText(entry.definition.initialValue, entry.definition.type));
    const multiline = col === 2 && entry.definition.type === 'json';
    this._editing = { viewRow, col };
    this._editor.begin({
      rect, text, multiline, caretAtEnd: initialText != null,
      onCommit: (value, nav) => {
        const accepted = this._commitCell(viewRow, col, value, nav);
        if (accepted !== false) this._editing = null;
        return accepted;
      },
      onCancel: () => { this._editing = null; },
    });
  }

  _commitCell(viewRow, col, text, nav) {
    const entry = this._entries()[this._viewRows[viewRow]];
    if (!entry) return true;
    let failure = '';
    let finalName = entry.name;
    this._apply('Edit variable', (working, bucket) => {
      const result = this._applyCellText(working, bucket, entry.name, col, text);
      if (!result.ok) { failure = result.message; return false; }
      finalName = result.name;
      return true;
    });
    if (failure) {
      this._setStatus(failure, 'error');
      return false;
    }
    const row = this._viewRows.indexOf(this._entries().findIndex(item => item.name === finalName));
    const targetRow = row >= 0 ? row : viewRow;
    if (nav === 'down') this._grid.select(targetRow + 1, col);
    else if (nav === 'right') this._grid.select(targetRow, col + 1);
    else if (nav === 'left') this._grid.select(targetRow, col - 1);
    else this._grid.select(targetRow, col);
    // 選択の変更はステータスを書き換えるので、警告は選択のあとに出す
    if (col === 0 && finalName !== entry.name) this._warnRuntimeName(entry.name, 'Renaming it');
    return true;
  }

  _onSelectionChange() {
    const entry = this._entries()[this._viewRows[this._grid.getSelection().focusRow]];
    if (entry) this._setStatus(`${this._scope}.${entry.name}`, 'muted');
  }

  _setFilter(text) {
    this._filterText = text;
    this._grid.scrollY = 0;
    this._rebuildView();
    this._grid.invalidate();
  }

  _beginSearchEdit() {
    if (!this._layout?.search) return;
    this._editor.commit();
    this._editing = { search: true };
    this._editor.begin({
      rect: this._layout.search, text: this._filterText, caretAtEnd: true,
      onInput: (text) => this._setFilter(text),
      onCommit: (text) => { this._setFilter(text); this._editing = null; },
      onCancel: () => { this._editing = null; },
    });
  }

  // ---------------------------------------------------------------------------
  // クリップボード（TSV）
  // ---------------------------------------------------------------------------

  _selectionToText() {
    const entries = this._entries();
    const sel = this._grid.getSelection();
    const rows = [];
    for (let viewRow = sel.top; viewRow <= sel.bottom; viewRow++) {
      const entry = entries[this._viewRows[viewRow]];
      if (!entry) continue;
      const values = [entry.name, entry.definition.type, VariablesEditorScene._valueToText(entry.definition.initialValue, entry.definition.type), entry.definition.description || ''];
      rows.push(values.slice(sel.left, sel.right + 1));
    }
    return DatabaseOps.formatTsv(rows);
  }

  _onCopy(event) {
    if (this._editor.isEditing || this._popup.isOpen || !this._viewRows.length) return;
    event.preventDefault();
    event.clipboardData?.setData('text/plain', this._selectionToText());
    this._setStatus('Copied as TSV', 'ok');
  }

  _onCut(event) {
    if (this._editor.isEditing || this._popup.isOpen || !this._viewRows.length) return;
    this._onCopy(event);
    this._clearSelection();
  }

  _onPaste(event) {
    if (this._editor.isEditing || this._popup.isOpen) return;
    event.preventDefault();
    this._pasteText(event.clipboardData?.getData('text/plain') ?? '');
  }

  /** TSV を選択位置へ貼る。足りない行は新しい変数として追加する（名前が重なれば番号を付ける）。 */
  _pasteText(text) {
    const data = DatabaseOps.parseTsv(text);
    if (!data.length) return;
    const sel = this._grid.getSelection();
    const startRow = this._viewRows.length ? sel.top : 0;
    const startCol = this._viewRows.length ? sel.left : 0;
    if (this._filterText && startRow + data.length > this._viewRows.length) {
      this._setStatus('The paste does not fit the filtered rows. Clear the search filter first', 'warning');
      return;
    }
    const [scope, tier] = this._scope.split('.');
    let written = 0;
    let skipped = 0;
    const applied = this._apply('Paste', (working) => {
      // 名前の変更で bucket のオブジェクトが入れ替わるので、毎回 working から取り直す
      const bucketOf = () => working[scope][tier];
      const names = this._entries().map(entry => entry.name);
      for (let r = 0; r < data.length; r++) {
        const actual = this._viewRows[startRow + r];
        let name;
        let firstColumn = 0;
        if (actual !== undefined) {
          name = names[actual];
        } else {
          let wanted = startCol === 0 ? (data[r][0] || '').trim() : '';
          if (!wanted || this._checkName(wanted, bucketOf(), null)) wanted = 'variable';
          name = wanted;
          for (let n = 2; Object.prototype.hasOwnProperty.call(bucketOf(), name); n++) name = `${wanted}${n}`;
          bucketOf()[name] = { type: 'string', initialValue: '', description: '' };
          names.push(name);
          written++;
          if (startCol === 0) firstColumn = 1; // 名前は作成時に使った
        }
        const slot = actual !== undefined ? actual : names.length - 1;
        for (let c = firstColumn; c < data[r].length && startCol + c <= 3; c++) {
          const result = this._applyCellText(working, bucketOf(), name, startCol + c, data[r][c]);
          if (!result.ok) { skipped++; continue; }
          written++;
          if (result.name !== name) { name = result.name; names[slot] = name; }
        }
      }
      return written > 0;
    });
    if (applied) this._setStatus(`Pasted ${written} cell(s)${skipped ? `, skipped ${skipped} that did not fit` : ''}`, skipped ? 'warning' : 'ok');
    else if (skipped) this._setStatus(`Nothing pasted: ${skipped} cell(s) did not fit`, 'error');
  }

  // ---------------------------------------------------------------------------
  // 右クリックメニュー
  // ---------------------------------------------------------------------------

  _openContextMenu(hit, x, y) {
    const items = [];
    if (hit.type !== 'colHeader') {
      items.push({ value: 'add', label: 'Add variable' });
      items.push({ value: 'delete', label: 'Delete variables', detail: 'Del' });
      items.push({ value: 'up', label: 'Move up', detail: 'Alt+↑' });
      items.push({ value: 'down', label: 'Move down', detail: 'Alt+↓' });
      items.push({ value: 'copy', label: 'Copy', detail: 'Ctrl+C' });
      items.push({ value: 'paste', label: 'Paste', detail: 'Ctrl+V' });
    } else {
      items.push({ value: 'add', label: 'Add variable' });
    }
    this._popup.open({
      x, y, w: 200, searchable: false, items,
      onPick: (value) => {
        if (value === 'add') this._addVariable();
        else if (value === 'delete') this._deleteSelected();
        else if (value === 'up') this._moveSelectedRows(-1);
        else if (value === 'down') this._moveSelectedRows(1);
        else if (value === 'copy') {
          navigator.clipboard?.writeText(this._selectionToText()).catch(() => {});
          this._setStatus('Copied as TSV', 'ok');
        } else if (value === 'paste') {
          (navigator.clipboard?.readText ? navigator.clipboard.readText() : Promise.reject(new Error('unavailable')))
            .then(text => this._pasteText(text))
            .catch(() => this._setStatus('The browser did not allow reading the clipboard. Use Ctrl+V', 'warning'));
        }
      },
    });
  }

  // ---------------------------------------------------------------------------
  // 保存と画面遷移
  // ---------------------------------------------------------------------------

  _saveProject() {
    this._editor.commit();
    if (!this._project || !this._appData.projectSession) return;
    const validation = ProjectData.validateGlobalVariables(this._project.globalVariables);
    if (!validation.ok) {
      this._setStatus(validation.message, 'error');
      if (validation.scopePath && VariablesEditorScene.SCOPES.includes(validation.scopePath)) this._selectScope(validation.scopePath);
      return;
    }
    this._appData.syncEditorStateToProjectSession?.();
    ProjectBrowserStorage.saveProject(this._project, this._appData.projectSession, this._appData.palette)
      .then(() => {
        this._appData.projectSession?.clearDirty?.();
        this._setStatus('Saved to browser storage', 'ok');
      })
      .catch((error) => this._setStatus(error?.message || 'Browser save failed', 'error'));
  }

  _goBack() {
    this._editor.commit();
    this._appData.syncEditorStateToProjectSession?.();
    this._appData.changeScene(new ProjectTopScene());
  }

  _selectScope(scope) {
    this._editor.destroy();
    this._editing = null;
    this._scope = scope;
    this._filterText = '';
    this._grid.selection = { anchorRow: 0, anchorCol: 0, focusRow: 0, focusCol: 0 };
    this._grid.scrollX = 0;
    this._grid.scrollY = 0;
    this._rebuildView();
    this._setStatus(scope, 'muted');
  }

  // ---------------------------------------------------------------------------
  // 描画
  // ---------------------------------------------------------------------------

  render(ctx, canvas) {
    const colors = GridTheme.COLORS;
    const W = canvas.width;
    const H = canvas.height;
    const header = 52;
    const toolbar = 44;
    const status = 28;
    const bodyY = header + toolbar;
    const navW = W < 760 ? 150 : 224;
    const layout = {
      nav: { x: 0, y: bodyY, w: navW, h: H - bodyY - status },
      grid: { x: navW + 1, y: bodyY, w: Math.max(0, W - navW - 1), h: Math.max(0, H - bodyY - status) },
    };
    this._layout = layout;
    this._grid.setRect(layout.grid);

    // ボタン
    this._buttons = [];
    let hx = W - 16;
    for (const def of [{ id: 'save', label: 'Save', action: () => this._saveProject() }, { id: 'back', label: 'Back', action: () => this._goBack() }]) {
      hx -= 76;
      this._buttons.push({ ...def, area: 'header', rect: { x: hx, y: 12, w: 76, h: 30 } });
      hx -= 8;
    }
    const hasRows = this._viewRows.length > 0;
    let tx = 12;
    for (const def of [
      { id: 'add', label: '+ Variable', w: 96, enabled: true, action: () => this._addVariable() },
      { id: 'delete', label: 'Delete', w: 76, enabled: hasRows, action: () => this._deleteSelected() },
      { id: 'sep', sep: true, w: 10 },
      { id: 'undo', label: 'Undo', w: 64, enabled: this._history.canUndo(), action: () => this._undo() },
      { id: 'redo', label: 'Redo', w: 64, enabled: this._history.canRedo(), action: () => this._redo() },
    ]) {
      if (!def.sep) this._buttons.push({ ...def, area: 'toolbar', rect: { x: tx, y: header + 7, w: def.w, h: 30 } });
      tx += def.w + 8;
    }
    layout.search = { x: tx + 8, y: header + 7, w: Math.max(120, Math.min(320, W - tx - 16)), h: 30 };

    // ナビ
    this._navItems = [];
    let ny = layout.nav.y + 8;
    this._navItems.push({ type: 'section', label: 'SCOPE', rect: { x: 0, y: ny, w: navW, h: 26 } });
    ny += 26;
    for (const scope of VariablesEditorScene.SCOPES) {
      const [s, t] = scope.split('.');
      this._navItems.push({
        type: 'scope', scope, label: scope, count: Object.keys(this._project.globalVariables[s][t]).length,
        selected: scope === this._scope, rect: { x: 0, y: ny, w: navW, h: 30 },
      });
      ny += 30;
    }
    const notes = {
      'system.fixed': 'Runtime values, reset each start',
      'system.persistent': 'Engine settings to keep',
      'user.fixed': 'Your values, reset each start',
      'user.persistent': 'Your values to save (save data)',
    };
    this._navItems.push({ type: 'note', label: notes[this._scope], rect: { x: 0, y: ny + 10, w: navW, h: 20 } });

    ctx.fillStyle = colors.bg;
    ctx.fillRect(0, 0, W, H);
    // ヘッダ
    ctx.fillStyle = colors.panel;
    ctx.fillRect(0, 0, W, header);
    ctx.textBaseline = 'middle';
    ctx.textAlign = 'left';
    ctx.fillStyle = colors.text;
    ctx.font = 'bold 18px sans-serif';
    ctx.fillText('Variables Editor', 20, 22);
    ctx.font = GridTheme.FONT_SMALL;
    ctx.fillStyle = colors.muted;
    ctx.fillText(`${this._project.name || 'Project'}${this._appData.projectSession?.dirty ? '  •  unsaved changes' : ''}`, 20, 40);
    // ツールバー
    ctx.fillStyle = colors.panelAlt;
    ctx.fillRect(0, header, W, toolbar);
    for (const button of this._buttons) {
      GridTheme.drawButton(ctx, button.rect, button.label, { hover: this._hover === `button:${button.id}`, disabled: button.enabled === false });
    }
    const search = layout.search;
    GridTheme.roundRect(ctx, search.x, search.y, search.w, search.h, 5);
    ctx.fillStyle = colors.bg;
    ctx.fill();
    ctx.strokeStyle = this._editing?.search ? colors.accent : colors.border;
    ctx.stroke();
    ctx.font = GridTheme.FONT;
    ctx.fillStyle = this._filterText ? colors.text : colors.muted;
    ctx.fillText(GridTheme.fitText(ctx, this._filterText || 'Search variables (Ctrl+F)', search.w - 16), search.x + 8, search.y + search.h / 2 + 0.5);
    // ナビ
    ctx.fillStyle = colors.panel;
    ctx.fillRect(layout.nav.x, layout.nav.y, layout.nav.w, layout.nav.h);
    for (const item of this._navItems) {
      const r = item.rect;
      if (item.type === 'section') {
        ctx.font = GridTheme.FONT_SMALL; ctx.fillStyle = colors.muted; ctx.textAlign = 'left';
        ctx.fillText(item.label, r.x + 12, r.y + r.h / 2);
      } else if (item.type === 'note') {
        ctx.font = GridTheme.FONT_SMALL; ctx.fillStyle = colors.muted; ctx.textAlign = 'left';
        ctx.fillText(GridTheme.fitText(ctx, item.label, r.w - 20), r.x + 12, r.y + r.h / 2);
      } else {
        if (item.selected) {
          ctx.fillStyle = colors.selectionFill; ctx.fillRect(r.x, r.y, r.w, r.h);
          ctx.fillStyle = colors.accent; ctx.fillRect(r.x, r.y, 3, r.h);
        } else if (this._hover === `scope:${item.scope}`) {
          ctx.fillStyle = colors.headerHover; ctx.fillRect(r.x, r.y, r.w, r.h);
        }
        ctx.font = GridTheme.FONT; ctx.fillStyle = colors.text; ctx.textAlign = 'left';
        ctx.fillText(item.label, r.x + 16, r.y + r.h / 2 + 0.5);
        ctx.font = GridTheme.FONT_SMALL; ctx.fillStyle = colors.muted; ctx.textAlign = 'right';
        ctx.fillText(String(item.count), r.x + r.w - 12, r.y + r.h / 2 + 0.5);
      }
    }
    // グリッド
    this._syncEditorPosition();
    this._grid.render(ctx);
    if (!this._viewRows.length) {
      ctx.fillStyle = colors.muted; ctx.font = GridTheme.FONT; ctx.textAlign = 'center';
      ctx.fillText(this._filterText ? 'No variable matches the search' : 'No variables here yet. Click “+ Variable”', layout.grid.x + layout.grid.w / 2, layout.grid.y + layout.grid.h / 2);
    }
    // ステータスバー
    ctx.fillStyle = colors.panelAlt;
    ctx.fillRect(0, H - status, W, status);
    ctx.font = GridTheme.FONT_SMALL;
    ctx.textAlign = 'right';
    ctx.fillStyle = colors.muted;
    const right = `${this._viewRows.length} variable${this._viewRows.length === 1 ? '' : 's'}`;
    ctx.fillText(right, W - 12, H - status / 2 + 0.5);
    ctx.textAlign = 'left';
    ctx.fillStyle = this._statusTone === 'error' ? '#fca5a5' : this._statusTone === 'warning' ? colors.warning : this._statusTone === 'ok' ? colors.ok : colors.muted;
    ctx.fillText(GridTheme.fitText(ctx, this._statusMessage, W - 160), 12, H - status / 2 + 0.5);
    this._popup.render(ctx, canvas);
  }

  _syncEditorPosition() {
    if (!this._editor.isEditing || !this._editing) return;
    if (this._editing.search) this._editor.reposition(this._layout.search);
    else this._editor.reposition(this._grid.getCellRect(this._editing.viewRow, this._editing.col));
  }

  // ---------------------------------------------------------------------------
  // 入力
  // ---------------------------------------------------------------------------

  _hitHover(x, y) {
    for (const button of this._buttons) if (button.enabled !== false && GridTheme.inRect(button.rect, x, y)) return `button:${button.id}`;
    for (const item of this._navItems) if (item.type === 'scope' && GridTheme.inRect(item.rect, x, y)) return `scope:${item.scope}`;
    return '';
  }

  _onMouseMove(e) {
    if (this._popup.isOpen) { this._popup.onMouseMove(e.x, e.y); return; }
    const canvas = document.getElementById('mainCanvas');
    if (this._grid.onMouseMove(e.x, e.y)) { if (canvas) canvas.style.cursor = this._grid.getCursor(e.x, e.y); return; }
    this._hover = this._hitHover(e.x, e.y);
    if (canvas) canvas.style.cursor = this._grid.getCursor(e.x, e.y);
  }

  _onMouseDown(e) {
    if (e.button !== 0) return;
    if (this._popup.isOpen) { this._popup.onMouseDown(e.x, e.y); return; }
    this._editor.commit();
    for (const button of this._buttons) {
      if (GridTheme.inRect(button.rect, e.x, e.y)) { if (button.enabled !== false) button.action(); return; }
    }
    if (this._layout?.search && GridTheme.inRect(this._layout.search, e.x, e.y)) { this._beginSearchEdit(); return; }
    for (const item of this._navItems) {
      if (item.type === 'scope' && GridTheme.inRect(item.rect, e.x, e.y)) { this._selectScope(item.scope); return; }
    }
    this._grid.onMouseDown(e.x, e.y, e);
  }

  _onContextMenu(e) {
    if (!this._popup.isOpen) this._grid.onContextMenuAt(e.x, e.y);
  }

  _onWheel(e) {
    if (this._popup.isOpen) { this._popup.onWheel(e.deltaY); return; }
    this._editor.commit();
    this._grid.onWheel(e.x, e.y, e.deltaX || 0, e.deltaY || 0, !!e.raw?.shiftKey);
  }

  _onKeyDown(e) {
    const raw = e.raw;
    if (this._popup.isOpen) { this._popup.onKeyDown(e); raw?.preventDefault(); return; }
    if ((e.ctrl || e.meta) && !e.alt) {
      if (e.code === 'KeyS') { raw?.preventDefault(); this._saveProject(); return; }
      if (e.code === 'KeyZ') { raw?.preventDefault(); if (e.shift) this._redo(); else this._undo(); return; }
      if (e.code === 'KeyY') { raw?.preventDefault(); this._redo(); return; }
      if (e.code === 'KeyF') { raw?.preventDefault(); this._beginSearchEdit(); return; }
      if (e.code === 'KeyC' || e.code === 'KeyX' || e.code === 'KeyV') return;
    }
    if (e.code === 'Escape') {
      if (this._filterText) { this._setFilter(''); return; }
      this._goBack();
      return;
    }
    if (e.code === 'Delete' && this._grid.getSelection().left === 0 && this._grid.getSelection().right === VariablesEditorScene.COLUMNS.length - 1) {
      raw?.preventDefault();
      this._deleteSelected();
      return;
    }
    if (this._grid.onKeyDown(e)) raw?.preventDefault();
  }
}
