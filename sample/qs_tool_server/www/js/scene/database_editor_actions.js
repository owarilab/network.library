/**
 * DatabaseEditorScene の編集処理（database_editor_scene.js のクラスへメソッドを足す）。
 *
 * - セルの値の変更は DatabaseSetCellsCommand（差分）、行・列・テーブルの変更は DatabaseTablesCommand（スナップショット）。
 * - 構造の変更は _applyStructural() にまとめている。DB の作業用コピーを渡し、変わったテーブルだけを履歴に積む。
 */
Object.assign(DatabaseEditorScene.prototype, {
  // ---------------------------------------------------------------------------
  // 検証・表示用の行
  // ---------------------------------------------------------------------------

  _runValidation() {
    this._validationDirty = false;
    const result = DatabaseData.validate(this._project.database, this._project);
    this._issueList = result.issues;
    this._issues = new Map();
    for (const issue of result.issues) {
      const key = `${issue.tableName}|${issue.rowKey ?? ''}|${issue.columnName ?? ''}`;
      if (!this._issues.has(key)) this._issues.set(key, issue.message);
    }
  },

  _issueFor(table, row, column) {
    if (this._isReadOnly()) return '';
    return this._issues.get(`${table.name}|${row ? row.key : ''}|${column ? column.name : ''}`) || '';
  },

  _afterEdit() {
    this._project.touch?.();
    this._appData.projectSession?.markDirty?.();
    this._appData.resetRuntimeDatabase?.();
    this._validationDirty = true;
    this._usage = { key: '', items: [] };
    this._ensureCurrentTable();
    this._rebuildView();
  },

  _setStatus(message, tone = 'muted') {
    this._statusMessage = message;
    this._statusTone = tone;
  },

  // ---------------------------------------------------------------------------
  // グリッドのモデル
  // ---------------------------------------------------------------------------

  _rebuildView() {
    const table = this._getCurrentTable();
    const rows = table ? table.rows : [];
    const query = this._filterText.trim().toLowerCase();
    this._viewRows = [];
    for (let index = 0; index < rows.length; index++) {
      if (!query || this._rowMatches(table, rows[index], query)) this._viewRows.push(index);
    }
    this._grid.setModel(this._buildGridModel());
  },

  _rowMatches(table, row, query) {
    if (row.key.toLowerCase().includes(query)) return true;
    for (const column of table.columns) {
      if (DatabaseOps.cellToText(row.cells[column.name], column).toLowerCase().includes(query)) return true;
    }
    return false;
  },

  _defaultColumnWidth(column) {
    switch (column.type) {
      case 'number': case 'integer': return 100;
      case 'boolean': return 90;
      case 'enum': return 130;
      case 'text': case 'json': return 240;
      default: return 170;
    }
  },

  _columnBadge(column) {
    switch (column.type) {
      case 'enum': return `enum (${(column.options || []).length})`;
      case 'ref': return `ref → ${DatabaseData.findTableById(this._project.database, column.refTableId)?.name || '?'}`;
      case 'asset': return `asset: ${column.assetType}`;
      default: return column.type;
    }
  },

  _buildGridModel() {
    const table = this._getCurrentTable();
    if (!table) return { columns: [], frozenColumns: 0, rowCount: 0, getCell: () => ({ text: '' }), getRowLabel: () => '' };

    const readOnly = this._isReadOnly();
    const widthOf = (id, fallback) => this._columnWidths.get(id) || fallback;
    const columns = [
      { id: DatabaseEditorScene.KEY_COLUMN_ID, title: 'key', badge: 'identifier', badgeColor: GridTheme.COLORS.accent, width: widthOf(`${table.id}:key`, 150) },
      ...table.columns.map(column => ({
        id: column.id,
        title: column.name || '(unnamed)',
        badge: this._columnBadge(column),
        width: widthOf(column.id, this._defaultColumnWidth(column)),
        align: column.type === 'number' || column.type === 'integer' ? 'right' : 'left',
      })),
    ];
    const project = this._project;
    return {
      columns,
      frozenColumns: 1,
      rowCount: this._viewRows.length,
      getRowLabel: (viewRow) => String(this._viewRows[viewRow] + 1),
      getCell: (viewRow, gridCol) => {
        const row = table.rows[this._viewRows[viewRow]];
        if (!row) return { text: '' };
        if (gridCol === 0) {
          const error = this._issueFor(table, row, null);
          return { text: row.key, readOnly, error: !!error };
        }
        const column = table.columns[gridCol - 1];
        if (!column) return { text: '' };
        const value = row.cells[column.name];
        const error = !!this._issueFor(table, row, column);
        if (column.type === 'boolean') return { kind: 'checkbox', checked: value === true, readOnly, error };
        let text = DatabaseOps.cellToText(value, column);
        if (column.type === 'asset' && value) {
          const name = this._assetName(column.assetType, value);
          text = name ? `${name}  (${value})` : text;
        }
        return { text, readOnly, error, mono: column.type === 'json', muted: text === '' };
      },
    };
    void project;
  },

  _assetName(assetType, id) {
    const project = this._project;
    switch (assetType) {
      case 'map': return project.findMapById?.(id)?.name || '';
      case 'playUnit': return project.findPlayUnitById?.(id)?.name || '';
      case 'pixelDocument': return project.findPixelDocumentById?.(id)?.name || '';
      case 'tileset': return project.findTilesetById?.(id)?.name || '';
      case 'characterChipset': return project.findCharacterChipsetById?.(id)?.name || '';
      default: return '';
    }
  },

  _listAssets(assetType) {
    const assets = this._project.assets;
    const list = {
      map: assets.maps, playUnit: assets.playUnits, pixelDocument: assets.pixelDocuments,
      tileset: assets.tilesets, characterChipset: assets.characterChipsets,
    }[assetType] || [];
    return list.map(asset => ({ value: asset.id, label: asset.name || asset.id, detail: asset.id }));
  },

  _setColumnWidth(gridCol, width) {
    const table = this._getCurrentTable();
    const column = this._grid.model.columns[gridCol];
    if (!table || !column) return;
    column.width = width;
    this._columnWidths.set(gridCol === 0 ? `${table.id}:key` : column.id, width);
  },

  _onGridSelectionChange() {
    const table = this._getCurrentTable();
    if (!table) return;
    const sel = this._grid.getSelection();
    const row = table.rows[this._viewRows[sel.focusRow]];
    const column = sel.focusCol > 0 ? table.columns[sel.focusCol - 1] : null;
    const issue = row ? this._issueFor(table, row, column) : '';
    if (issue) this._setStatus(`${table.name}.${row.key}${column ? `.${column.name}` : ''}: ${issue}`, 'error');
  },

  // ---------------------------------------------------------------------------
  // テーブルの選択
  // ---------------------------------------------------------------------------

  _selectTable(kind, id) {
    this._editor.destroy();
    this._editing = null;
    this._current = { kind, id };
    this._filterText = '';
    this._inspectorScroll = 0;
    this._grid.selection = { anchorRow: 0, anchorCol: 0, focusRow: 0, focusCol: 0 };
    this._grid.scrollX = 0;
    this._grid.scrollY = 0;
    this._rebuildView();
    const table = this._getCurrentTable();
    this._setStatus(kind === 'system' ? `${table?.name}: built-in table (read-only)` : `${table?.name || ''}`, 'muted');
  },

  _createTable(kind) {
    let createdId = '';
    const applied = this._applyStructural('Add table', (working) => {
      if (working.tables.length >= DatabaseData.MAX_TABLES) {
        this._setStatus(`Too many tables (max ${DatabaseData.MAX_TABLES})`, 'error');
        return false;
      }
      const table = DatabaseOps.createTable(working, kind);
      working.tables.push(table);
      createdId = table.id;
      return true;
    });
    if (applied) {
      this._selectTable(kind, createdId);
      this._setStatus('Table created. Rename it in the Inspector, then add rows and columns', 'ok');
    }
  },

  // ---------------------------------------------------------------------------
  // 履歴
  // ---------------------------------------------------------------------------

  _execute(command, message) {
    if (!this._history.execute(command, this._appData)) return false;
    this._afterEdit();
    if (message) this._setStatus(message, 'ok');
    return true;
  },

  _undo() {
    this._editor.commit();
    const label = this._history.getUndoLabel?.() || '';
    if (this._history.undo(this._appData)) {
      this._afterEdit();
      this._grid.invalidate();
      this._setStatus(`Undo: ${label}`, 'muted');
    }
  },

  _redo() {
    this._editor.commit();
    const label = this._history.getRedoLabel?.() || '';
    if (this._history.redo(this._appData)) {
      this._afterEdit();
      this._grid.invalidate();
      this._setStatus(`Redo: ${label}`, 'muted');
    }
  },

  /**
   * 構造の変更を行う。mutator に DB の作業用コピーを渡し、変わったテーブルだけをスナップショットとして履歴に積む。
   * @param {string} label
   * @param {(working: { version: number, tables: object[] }) => boolean|void} mutator  false を返すと中止
   * @returns {boolean}
   */
  _applyStructural(label, mutator) {
    const database = this._project.database;
    const beforeTables = JSON.parse(JSON.stringify(database.tables));
    const working = { version: database.version, tables: JSON.parse(JSON.stringify(database.tables)) };
    if (mutator(working) === false) return false;

    const beforeInfo = new Map(beforeTables.map((table, index) => [table.id, { table, index, json: JSON.stringify(table) }]));
    const entries = [];
    working.tables.forEach((table, index) => {
      const before = beforeInfo.get(table.id);
      if (!before) {
        entries.push(DatabaseTablesCommand.entry(null, table, -1, index));
        return;
      }
      beforeInfo.delete(table.id);
      if (before.index !== index || before.json !== JSON.stringify(table)) {
        entries.push(DatabaseTablesCommand.entry(before.table, table, before.index, index));
      }
    });
    for (const before of beforeInfo.values()) entries.push(DatabaseTablesCommand.entry(before.table, null, before.index, -1));
    if (!entries.length) return false;
    return this._execute(new DatabaseTablesCommand(this._project, label, entries));
  },

  /** 現在のテーブルを作業用コピーから取り出す。 */
  _workingTable(working) {
    return working.tables.find(table => table.id === this._current.id) || null;
  },

  // ---------------------------------------------------------------------------
  // 選択範囲 → 行・列
  // ---------------------------------------------------------------------------

  _selectedActualRows() {
    const sel = this._grid.getSelection();
    const rows = [];
    for (let viewRow = sel.top; viewRow <= sel.bottom; viewRow++) {
      if (this._viewRows[viewRow] !== undefined) rows.push(this._viewRows[viewRow]);
    }
    return rows;
  },

  /** 実際の行の位置（table.rows 内）を、表示行へ直して選択する。 */
  _selectActualRows(actualIndices, gridCol = 0) {
    if (!actualIndices.length) return;
    const views = actualIndices.map(index => this._viewRows.indexOf(index)).filter(view => view >= 0);
    if (!views.length) return;
    const col = Math.min(gridCol, Math.max(0, this._grid.model.columns.length - 1));
    this._grid.selectRange(Math.min(...views), col, Math.max(...views), col);
    this._grid.ensureVisible(Math.max(...views), col);
  },

  _requireEditable() {
    if (!this._getCurrentTable()) return false;
    if (this._isReadOnly()) {
      this._setStatus('System tables are read-only', 'warning');
      return false;
    }
    return true;
  },

  // ---------------------------------------------------------------------------
  // 行の操作
  // ---------------------------------------------------------------------------

  _addRow(where) {
    if (!this._requireEditable()) return;
    const table = this._getCurrentTable();
    const actual = this._selectedActualRows();
    const last = actual.length ? actual[actual.length - 1] : table.rows.length - 1;
    const first = actual.length ? actual[0] : table.rows.length;
    const index = where === 'before' ? first : last + 1;
    let createdIndex = -1;
    const applied = this._applyStructural('Add row', (working) => {
      const row = DatabaseOps.addRow(this._workingTable(working), index);
      if (!row) {
        this._setStatus(`Too many rows (max ${DatabaseData.MAX_ROWS})`, 'error');
        return false;
      }
      createdIndex = this._workingTable(working).rows.indexOf(row);
      return true;
    });
    if (applied) {
      if (this._filterText) this._setFilter('');
      this._selectActualRows([createdIndex], 0);
      this._setStatus('Row added', 'ok');
    }
  },

  _duplicateSelectedRows() {
    if (!this._requireEditable()) return;
    const actual = this._selectedActualRows();
    if (!actual.length) return;
    let created = [];
    const applied = this._applyStructural('Duplicate rows', (working) => {
      created = DatabaseOps.duplicateRows(this._workingTable(working), actual);
      return created.length > 0;
    });
    if (applied) {
      if (this._filterText) this._setFilter('');
      this._selectActualRows(created, 0);
      this._setStatus(`${created.length} row${created.length > 1 ? 's' : ''} duplicated`, 'ok');
    }
  },

  _deleteSelectedRows() {
    if (!this._requireEditable()) return;
    const actual = this._selectedActualRows();
    if (!actual.length) return;
    const applied = this._applyStructural('Delete rows', (working) => DatabaseOps.deleteRows(this._workingTable(working), actual) > 0);
    if (applied) {
      const next = Math.min(this._grid.getSelection().top, Math.max(0, this._viewRows.length - 1));
      this._grid.select(next, 0);
      this._setStatus(`${actual.length} row${actual.length > 1 ? 's' : ''} deleted (Ctrl+Z to undo)`, 'ok');
    }
  },

  _moveSelectedRows(delta) {
    if (!this._requireEditable()) return;
    if (this._filterText) {
      this._setStatus('Clear the search filter to reorder rows', 'warning');
      return;
    }
    const actual = this._selectedActualRows();
    let moved = [];
    const col = this._grid.getSelection().focusCol;
    const applied = this._applyStructural('Move rows', (working) => {
      moved = DatabaseOps.moveRows(this._workingTable(working), actual, delta);
      return moved.length > 0;
    });
    if (applied) this._selectActualRows(moved, col);
  },

  // ---------------------------------------------------------------------------
  // 列の操作
  // ---------------------------------------------------------------------------

  _addColumn(where) {
    if (!this._requireEditable()) return;
    const focusCol = this._grid.getSelection().focusCol;
    const index = where === 'before' ? Math.max(0, focusCol - 1) : Math.max(0, focusCol);
    let createdIndex = -1;
    const applied = this._applyStructural('Add column', (working) => {
      const table = this._workingTable(working);
      const column = DatabaseOps.addColumn(table, index);
      if (!column) {
        this._setStatus(`Too many columns (max ${DatabaseData.MAX_COLUMNS})`, 'error');
        return false;
      }
      createdIndex = table.columns.indexOf(column);
      return true;
    });
    if (applied) {
      this._grid.select(this._grid.getSelection().focusRow, createdIndex + 1);
      this._setStatus('Column added. Set its name and type in the Inspector', 'ok');
    }
  },

  _deleteFocusColumn() {
    if (!this._requireEditable()) return;
    const colIndex = this._grid.getSelection().focusCol - 1;
    const table = this._getCurrentTable();
    const column = table?.columns[colIndex];
    if (!column) {
      this._setStatus('The key column cannot be deleted', 'warning');
      return;
    }
    const referencing = DatabaseOps.findReferencingColumns(this._project.database, table.id);
    void referencing;
    const applied = this._applyStructural('Delete column', (working) => DatabaseOps.deleteColumn(this._workingTable(working), colIndex));
    if (applied) {
      this._grid.select(this._grid.getSelection().focusRow, Math.max(1, colIndex));
      this._setStatus(`Column ${column.name} deleted (Ctrl+Z to undo)`, 'ok');
    }
  },

  _moveFocusColumn(delta) {
    if (!this._requireEditable()) return;
    const colIndex = this._grid.getSelection().focusCol - 1;
    if (colIndex < 0) {
      this._setStatus('The key column cannot be moved', 'warning');
      return;
    }
    let target = -1;
    const applied = this._applyStructural('Move column', (working) => {
      target = DatabaseOps.moveColumn(this._workingTable(working), colIndex, delta);
      return target >= 0;
    });
    if (applied) this._grid.select(this._grid.getSelection().focusRow, target + 1);
  },

  // ---------------------------------------------------------------------------
  // セルの編集
  // ---------------------------------------------------------------------------

  _beginCellEdit(viewRow, gridCol, initialText) {
    const table = this._getCurrentTable();
    if (!table || !this._requireEditable()) return;
    const row = table.rows[this._viewRows[viewRow]];
    if (!row) return;
    const cellRect = this._grid.getCellRect(viewRow, gridCol);
    const popupAnchor = { x: cellRect.x, y: cellRect.y + cellRect.h, w: cellRect.w };

    if (gridCol === 0) {
      this._openTextEditor({ target: 'cell', viewRow, gridCol }, cellRect, initialText ?? row.key, false, initialText != null,
        (text, nav) => this._commitKey(viewRow, text, nav));
      return;
    }
    const column = table.columns[gridCol - 1];
    if (!column) return;
    const current = row.cells[column.name];

    if (column.type === 'boolean') {
      this._toggleBoolean(viewRow, gridCol);
      return;
    }
    if (column.type === 'enum') {
      this._popup.open({
        ...popupAnchor, title: `${column.name} (enum)`, searchable: (column.options || []).length > 8,
        items: (column.options || []).map(option => ({ value: option, label: option })),
        selectedValue: current,
        onPick: (value) => this._setCellValues([{ viewRow, gridCol, value }]),
      });
      return;
    }
    if (column.type === 'ref') {
      const target = DatabaseData.findTableById(this._project.database, column.refTableId);
      if (!target) {
        this._setStatus('This ref column points to a missing table. Choose a target table in the Inspector', 'error');
        return;
      }
      const labelColumn = target.columns.find(c => c.type === 'string' || c.type === 'text');
      this._popup.open({
        ...popupAnchor, title: `${column.name} → ${target.name}`,
        items: [{ value: '', label: '(none)' }, ...target.rows.map(r => ({
          value: r.key, label: r.key, detail: labelColumn ? DatabaseOps.cellToText(r.cells[labelColumn.name], labelColumn) : '',
        }))],
        selectedValue: current,
        onPick: (value) => this._setCellValues([{ viewRow, gridCol, value }]),
      });
      return;
    }
    if (column.type === 'asset') {
      this._popup.open({
        ...popupAnchor, title: `${column.name} (${column.assetType})`,
        items: [{ value: '', label: '(none)' }, ...this._listAssets(column.assetType)],
        selectedValue: current,
        onPick: (value) => this._setCellValues([{ viewRow, gridCol, value }]),
      });
      return;
    }

    const multiline = column.type === 'text' || column.type === 'json';
    this._openTextEditor({ target: 'cell', viewRow, gridCol }, cellRect,
      initialText ?? DatabaseOps.cellToText(current, column), multiline, initialText != null,
      (text, nav) => this._commitCellText(viewRow, gridCol, text, nav));
  },

  _openTextEditor(editing, rect, text, multiline, caretAtEnd, onCommit) {
    this._editing = editing;
    this._editor.begin({
      rect, text, multiline, caretAtEnd,
      onCommit: (value, nav) => {
        const accepted = onCommit(value, nav);
        if (accepted !== false) this._editing = null;
        return accepted;
      },
      onCancel: () => { this._editing = null; },
    });
  },

  _syncEditorPosition() {
    if (!this._editor.isEditing || !this._editing) return;
    if (this._editing.target === 'cell') {
      this._editor.reposition(this._grid.getCellRect(this._editing.viewRow, this._editing.gridCol));
    } else if (this._editing.target === 'search' && this._layout?.search) {
      this._editor.reposition(this._layout.search);
    }
  },

  _commitKey(viewRow, text, nav) {
    const table = this._getCurrentTable();
    const row = table?.rows[this._viewRows[viewRow]];
    if (!row) return true;
    const newKey = text.trim();
    if (newKey === row.key) { this._navigateAfterEdit(viewRow, 0, nav); return true; }
    if (!DatabaseData.isIdentifier(newKey)) {
      this._setStatus('Row key must be an identifier: letters, digits and _ (not starting with a digit, max 64)', 'error');
      return false;
    }
    if (table.rows.some(r => r !== row && r.key === newKey)) {
      this._setStatus(`Row key already exists: ${newKey}`, 'error');
      return false;
    }
    const oldKey = row.key;
    const rowIndex = this._viewRows[viewRow];
    const usage = this._getUsageCount();
    const applied = this._applyStructural('Rename key', (working) => {
      const workingTable = this._workingTable(working);
      DatabaseOps.renameRowKey(working, workingTable, workingTable.rows[rowIndex], newKey);
    });
    if (applied) {
      this._setStatus(usage
        ? `Key renamed: ${oldKey} → ${newKey}. ref cells were updated, but ${usage} use(s) in PlayUnits are not`
        : `Key renamed: ${oldKey} → ${newKey}`, usage ? 'warning' : 'ok');
      this._navigateAfterEdit(viewRow, 0, nav);
    }
    return true;
  },

  _commitCellText(viewRow, gridCol, text, nav) {
    const table = this._getCurrentTable();
    const column = table?.columns[gridCol - 1];
    if (!column) return true;
    const ctx = DatabaseData.createContext(this._project);
    let result = DatabaseOps.textToCellValue(text, column, ctx);
    // 数値などの空欄は既定値にする（スプレッドシートの空セルと同じ扱い）
    if (!result.ok && text.trim() === '' && column.type !== 'enum' && column.type !== 'json') {
      result = { ok: true, value: DatabaseData.getColumnDefault(column) };
    }
    if (!result.ok) {
      this._setStatus(`${column.name}: ${result.message}`, 'error');
      return false;
    }
    this._setCellValues([{ viewRow, gridCol, value: result.value }]);
    this._navigateAfterEdit(viewRow, gridCol, nav);
    return true;
  },

  _navigateAfterEdit(viewRow, gridCol, nav) {
    if (nav === 'down') this._grid.select(viewRow + 1, gridCol);
    else if (nav === 'right') this._grid.select(viewRow, gridCol + 1);
    else if (nav === 'left') this._grid.select(viewRow, gridCol - 1);
    else this._grid.select(viewRow, gridCol);
  },

  /**
   * @param {Array<{ viewRow: number, gridCol: number, value: any }>} edits  gridCol は 1 以上（データ列）
   * @param {string} [label]
   */
  _setCellValues(edits, label = 'Edit cells') {
    const table = this._getCurrentTable();
    if (!table || this._isReadOnly()) return false;
    const changes = [];
    for (const edit of edits) {
      const row = table.rows[this._viewRows[edit.viewRow]];
      const column = table.columns[edit.gridCol - 1];
      if (!row || !column) continue;
      const before = row.cells[column.name];
      if (JSON.stringify(before) === JSON.stringify(edit.value)) continue;
      changes.push({ rowKey: row.key, columnName: column.name, before, after: edit.value });
    }
    if (!changes.length) return false;
    return this._execute(new DatabaseSetCellsCommand(this._project, table.id, changes, label));
  },

  _toggleBoolean(viewRow, gridCol) {
    if (!this._requireEditable()) return;
    const table = this._getCurrentTable();
    const row = table.rows[this._viewRows[viewRow]];
    const column = table.columns[gridCol - 1];
    if (!row || !column) return;
    this._setCellValues([{ viewRow, gridCol, value: row.cells[column.name] !== true }], 'Toggle');
  },

  _clearSelection() {
    if (!this._requireEditable()) return;
    const table = this._getCurrentTable();
    const sel = this._grid.getSelection();
    const edits = [];
    for (let viewRow = sel.top; viewRow <= sel.bottom; viewRow++) {
      for (let gridCol = Math.max(1, sel.left); gridCol <= sel.right; gridCol++) {
        const column = table.columns[gridCol - 1];
        if (column) edits.push({ viewRow, gridCol, value: DatabaseData.getColumnDefault(column) });
      }
    }
    if (!edits.length) {
      this._setStatus('The key column cannot be cleared. Edit it, or delete the row', 'warning');
      return;
    }
    if (this._setCellValues(edits, 'Clear cells')) this._setStatus('Cleared to default values', 'ok');
  },

  // ---------------------------------------------------------------------------
  // 検索
  // ---------------------------------------------------------------------------

  _beginSearchEdit() {
    if (!this._layout?.search) return;
    this._editor.commit();
    this._editing = { target: 'search' };
    this._editor.begin({
      rect: this._layout.search,
      text: this._filterText,
      caretAtEnd: true,
      onInput: (text) => this._setFilter(text),
      onCommit: (text) => { this._setFilter(text); this._editing = null; },
      onCancel: () => { this._editing = null; },
    });
  },

  _setFilter(text) {
    this._filterText = text;
    this._grid.scrollY = 0;
    this._rebuildView();
    this._grid.invalidate();
  },

  // ---------------------------------------------------------------------------
  // クリップボード
  // ---------------------------------------------------------------------------

  _selectionToText() {
    const table = this._getCurrentTable();
    const sel = this._grid.getSelection();
    const rows = [];
    for (let viewRow = sel.top; viewRow <= sel.bottom; viewRow++) {
      const row = table.rows[this._viewRows[viewRow]];
      if (!row) continue;
      const cells = [];
      for (let gridCol = sel.left; gridCol <= sel.right; gridCol++) {
        cells.push(gridCol === 0 ? row.key : DatabaseOps.cellToText(row.cells[table.columns[gridCol - 1]?.name], table.columns[gridCol - 1]));
      }
      rows.push(cells);
    }
    return DatabaseOps.formatTsv(rows);
  },

  _onCopy(event) {
    if (this._editor.isEditing || this._popup.isOpen || !this._getCurrentTable() || !this._viewRows.length) return;
    const text = this._selectionToText();
    event.preventDefault();
    event.clipboardData?.setData('text/plain', text);
    this._clipboardText = text;
    const sel = this._grid.getSelection();
    this._setStatus(`Copied ${(sel.bottom - sel.top + 1) * (sel.right - sel.left + 1)} cell(s) as TSV`, 'ok');
  },

  _onCut(event) {
    if (this._editor.isEditing || this._popup.isOpen || !this._getCurrentTable() || !this._viewRows.length) return;
    this._onCopy(event);
    if (!this._isReadOnly()) this._clearSelection();
  },

  _onPaste(event) {
    if (this._editor.isEditing || this._popup.isOpen || !this._getCurrentTable()) return;
    const text = event.clipboardData?.getData('text/plain') ?? '';
    event.preventDefault();
    this._pasteText(text);
  },

  /**
   * TSV を選択位置へ貼り付ける。1 つの値なら選択範囲全体に入れる。範囲が足りなければ行を足す（検索中は足さない）。
   * 型に合わないセルは取り込まず、件数を知らせる。
   */
  _pasteText(text) {
    if (!this._requireEditable()) return;
    const data = DatabaseOps.parseTsv(text);
    if (!data.length || !this._viewRows.length) return;
    const table = this._getCurrentTable();
    const sel = this._grid.getSelection();
    const fill = data.length === 1 && data[0].length === 1 && (sel.bottom > sel.top || sel.right > sel.left);
    const startRow = sel.top;
    const startCol = sel.left;
    const height = fill ? sel.bottom - sel.top + 1 : data.length;
    const width = fill ? sel.right - sel.left + 1 : Math.max(...data.map(row => row.length));
    const colCount = table.columns.length + 1;
    const actualRows = this._viewRows.slice();
    const rowAtOffset = (offset) => actualRows[startRow + offset];
    const wantsNewRows = Math.max(0, startRow + height - actualRows.length);
    if (wantsNewRows && this._filterText) {
      this._setStatus('The paste does not fit the filtered rows. Clear the search filter first', 'warning');
      return;
    }

    let skipped = 0;
    let written = 0;
    let truncated = false;
    const applied = this._applyStructural('Paste', (working) => {
      const target = this._workingTable(working);
      const ctx = { ...DatabaseData.createContext(this._project), database: working };
      for (let i = 0; i < wantsNewRows; i++) {
        const row = DatabaseOps.addRow(target, target.rows.length);
        if (!row) { truncated = true; break; }
        actualRows.push(target.rows.indexOf(row));
      }
      for (let r = 0; r < height; r++) {
        const actual = rowAtOffset(r);
        const row = actual === undefined ? null : target.rows[actual];
        if (!row) continue;
        for (let c = 0; c < width; c++) {
          const gridCol = startCol + c;
          if (gridCol >= colCount) { skipped++; continue; }
          const source = fill ? data[0][0] : data[r][c];
          if (source === undefined) continue;
          if (gridCol === 0) {
            const key = String(source).trim();
            if (key === row.key) continue;
            if (!DatabaseData.isIdentifier(key) || target.rows.some(other => other !== row && other.key === key)) { skipped++; continue; }
            DatabaseOps.renameRowKey(working, target, row, key);
            written++;
            continue;
          }
          const column = target.columns[gridCol - 1];
          let result = DatabaseOps.textToCellValue(source, column, ctx);
          if (!result.ok && String(source).trim() === '' && column.type !== 'enum' && column.type !== 'json') {
            result = { ok: true, value: DatabaseData.getColumnDefault(column) };
          }
          if (!result.ok) { skipped++; continue; }
          row.cells[column.name] = result.value;
          written++;
        }
      }
      return written > 0 || wantsNewRows > 0;
    });
    if (applied) {
      this._setStatus(`Pasted ${written} cell(s)${skipped ? `, skipped ${skipped} that did not fit the column type` : ''}${truncated ? ', row limit reached' : ''}`,
        skipped || truncated ? 'warning' : 'ok');
    } else if (skipped) {
      this._setStatus(`Nothing pasted: ${skipped} cell(s) did not fit the column types`, 'error');
    }
  },

  // ---------------------------------------------------------------------------
  // 右クリックメニュー
  // ---------------------------------------------------------------------------

  _openContextMenu(hit, x, y) {
    if (!this._getCurrentTable()) return;
    const readOnly = this._isReadOnly();
    const items = [];
    const add = (value, label, detail) => items.push({ value, label, detail });
    if (hit.type === 'colHeader') {
      if (!readOnly && hit.col > 0) {
        add('colLeft', 'Insert column left');
        add('colRight', 'Insert column right');
        add('colMoveLeft', 'Move column left', 'Alt+←');
        add('colMoveRight', 'Move column right', 'Alt+→');
        add('colDelete', 'Delete column');
      } else if (!readOnly) {
        add('colRight', 'Insert column right');
      }
    } else {
      if (!readOnly) {
        add('rowAbove', 'Insert row above');
        add('rowBelow', 'Insert row below');
        add('rowDuplicate', 'Duplicate rows');
        add('rowDelete', 'Delete rows');
        add('rowUp', 'Move rows up', 'Alt+↑');
        add('rowDown', 'Move rows down', 'Alt+↓');
      }
      add('copy', 'Copy', 'Ctrl+C');
      if (!readOnly) {
        add('cut', 'Cut', 'Ctrl+X');
        add('paste', 'Paste', 'Ctrl+V');
        add('clear', 'Clear to default', 'Del');
      }
    }
    if (!items.length) return;
    this._popup.open({
      x, y, w: 220, searchable: false, items,
      onPick: (value) => this._runMenuAction(value),
    });
  },

  _runMenuAction(action) {
    switch (action) {
      case 'rowAbove': this._addRow('before'); break;
      case 'rowBelow': this._addRow('after'); break;
      case 'rowDuplicate': this._duplicateSelectedRows(); break;
      case 'rowDelete': this._deleteSelectedRows(); break;
      case 'rowUp': this._moveSelectedRows(-1); break;
      case 'rowDown': this._moveSelectedRows(1); break;
      case 'colLeft': this._addColumn('before'); break;
      case 'colRight': this._addColumn('after'); break;
      case 'colMoveLeft': this._moveFocusColumn(-1); break;
      case 'colMoveRight': this._moveFocusColumn(1); break;
      case 'colDelete': this._deleteFocusColumn(); break;
      case 'copy': this._copyFromMenu(false); break;
      case 'cut': this._copyFromMenu(true); break;
      case 'paste': this._pasteFromMenu(); break;
      case 'clear': this._clearSelection(); break;
      default: break;
    }
  },

  _copyFromMenu(cut) {
    const text = this._selectionToText();
    this._clipboardText = text;
    navigator.clipboard?.writeText(text).catch(() => {});
    this._setStatus('Copied as TSV', 'ok');
    if (cut) this._clearSelection();
  },

  _pasteFromMenu() {
    const read = navigator.clipboard?.readText ? navigator.clipboard.readText() : Promise.reject(new Error('unavailable'));
    read.then(text => this._pasteText(text)).catch(() => {
      if (this._clipboardText) this._pasteText(this._clipboardText);
      else this._setStatus('The browser did not allow reading the clipboard. Use Ctrl+V', 'warning');
    });
  },

  // ---------------------------------------------------------------------------
  // 保存
  // ---------------------------------------------------------------------------

  _saveProject() {
    this._editor.commit();
    if (!this._project || !this._appData.projectSession) return;
    this._runValidation();
    if (this._issueList.length) {
      const first = this._issueList[0];
      this._setStatus(`Cannot save: ${DatabaseData.formatIssue(first)}${this._issueList.length > 1 ? ` (+${this._issueList.length - 1} more)` : ''}`, 'error');
      return;
    }
    this._appData.syncEditorStateToProjectSession?.();
    ProjectBrowserStorage.saveProject(this._project, this._appData.projectSession, this._appData.palette)
      .then(() => {
        this._appData.projectSession?.clearDirty?.();
        this._setStatus('Saved to browser storage', 'ok');
      })
      .catch((error) => this._setStatus(error?.message || 'Browser save failed', 'error'));
  },

  // ---------------------------------------------------------------------------
  // PlayUnit からの使用箇所
  // ---------------------------------------------------------------------------

  /**
   * このテーブルのパス（db.<kind>.<table>.…）を使っている PlayUnit のコンポーネントを探す（読み取りだけ）。
   * @returns {string[]}  "PlayUnit / Object / Component"
   */
  _scanUsages(kind, tableName) {
    const prefix = `db.${kind}.${tableName}`;
    const found = [];
    for (const playUnit of this._project.assets.playUnits || []) {
      for (const object of playUnit.objects || []) {
        for (const component of object.components || []) {
          const text = JSON.stringify(component.data ?? {});
          const at = text.indexOf(prefix);
          if (at < 0) continue;
          const next = text[at + prefix.length];
          if (next === undefined || next === '.' || next === '"' || next === '}') {
            found.push(`${playUnit.name || playUnit.id} / ${object.name || object.id} / ${component.type}`);
          }
        }
      }
    }
    return found;
  },

  _getUsage() {
    const table = this._getCurrentTable();
    if (!table) return [];
    const key = `${this._current.kind}:${table.name}`;
    if (this._usage.key !== key) this._usage = { key, items: this._scanUsages(this._current.kind, table.name) };
    return this._usage.items;
  },

  _getUsageCount() {
    return this._getUsage().length;
  },
});
