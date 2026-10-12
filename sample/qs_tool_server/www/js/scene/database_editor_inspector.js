/**
 * DatabaseEditorScene の Inspector（右の設定パネル）。テーブル・列・行の設定と、PlayUnit からの使用箇所を出す。
 * 項目は毎フレーム作り直し、クリックで編集（オーバーレイ入力、ポップアップ、ボタン）を始める。
 */
Object.assign(DatabaseEditorScene.prototype, {
  _buildInspectorItems(rect) {
    const items = [];
    let y = rect.y + 8 - this._inspectorScroll;
    const push = (item, h) => {
      items.push({ ...item, rect: { x: rect.x, y, w: rect.w, h } });
      y += h;
    };
    const header = (label) => push({ type: 'header', label }, 28);
    const field = (id, label, value, action, extra = {}) => push({ type: 'field', id, label, value: String(value ?? ''), action, ...extra }, 26);
    const button = (id, label, action, enabled = true) => push({ type: 'button', id, label, action, enabled }, 32);
    const note = (lines, tone) => push({ type: 'note', lines, tone }, 10 + lines.length * 15);

    const table = this._getCurrentTable();
    if (!table) {
      header('Inspector');
      note(['Select a table on the left.'], 'muted');
      return items;
    }
    const readOnly = this._isReadOnly();
    const editable = (fn) => (readOnly ? null : fn);
    const sel = this._grid.getSelection();
    const row = table.rows[this._viewRows[sel.focusRow]];
    const column = sel.focusCol > 0 ? table.columns[sel.focusCol - 1] : null;

    // テーブル
    header(`Table  ${table.name}`);
    field('tableName', 'Name', table.name, editable((r) => this._editTableText(r, 'name')));
    field('tableLabel', 'Label', table.label || '(none)', editable((r) => this._editTableText(r, 'label')));
    field('tableKind', 'Kind', readOnly ? 'system' : table.kind, editable((r) => this._pickTableKind(r)));
    field('tableRows', 'Rows', table.rows.length, null);
    field('tableColumns', 'Columns', table.columns.length, null);
    if (readOnly) note(['Built into the app. Read-only,', 'also at runtime (db.system.…).'], 'muted');
    else button('deleteTable', 'Delete table', () => this._deleteTable());
    y += 6;

    // 選択中のセルのパス（クリックでコピー）
    if (row && this._current.kind) {
      const path = this._cellPath(table, row, column);
      field('path', 'Path', path, () => this._copyText(path, 'Path copied'), { mono: true });
    }
    y += 6;

    // 列 / key 列
    if (column) {
      header(`Column  ${column.name || '(unnamed)'}`);
      field('colName', 'Name', column.name, editable((r) => this._editColumnText(r, column, 'name')));
      field('colLabel', 'Label', column.label || '(none)', editable((r) => this._editColumnText(r, column, 'label')));
      field('colType', 'Type', column.type, editable((r) => this._pickColumnType(r, column)));
      if (column.type === 'enum') {
        field('colOptions', 'Options', (column.options || []).join(', ') || '(none)', editable((r) => this._editColumnOptions(r, column)));
      }
      if (column.type === 'ref') {
        field('colRef', 'Target', DatabaseData.findTableById(this._project.database, column.refTableId)?.name || '(missing)',
          editable((r) => this._pickRefTable(r, column)), { error: !DatabaseData.findTableById(this._project.database, column.refTableId) });
      }
      if (column.type === 'asset') {
        field('colAsset', 'Asset type', column.assetType, editable((r) => this._pickAssetType(r, column)));
      }
      field('colDefault', 'Default', DatabaseOps.cellToText(column.default, column) || '(empty)', editable((r) => this._editColumnDefault(r, column)));
      if (!readOnly) {
        button('colLeft', 'Move column left', () => this._moveFocusColumn(-1), sel.focusCol > 1);
        button('colRight', 'Move column right', () => this._moveFocusColumn(1), sel.focusCol < table.columns.length);
        button('colDelete', 'Delete column', () => this._deleteFocusColumn());
      }
    } else if (table.columns.length || row) {
      header('Key column');
      note(['The row key identifies the row in paths:', `db.${this._current.kind}.${table.name}.<key>.<column>`], 'muted');
    }
    y += 6;

    // 行
    if (row) {
      header(`Row  ${row.key}`);
      field('rowKey', 'Key', row.key, editable((r) => this._editRowKey(r, sel.focusRow)));
      if (!readOnly) {
        button('rowDup', 'Duplicate rows', () => this._duplicateSelectedRows());
        button('rowDel', 'Delete rows', () => this._deleteSelectedRows());
      }
      y += 6;
    }

    // 問題と使用箇所
    const tableIssues = this._issueList.filter(issue => issue.tableName === table.name);
    if (!readOnly && tableIssues.length) {
      header(`Issues  (${tableIssues.length})`);
      note(tableIssues.slice(0, 4).map(issue => `• ${DatabaseData.formatIssue(issue)}`), 'error');
      y += 6;
    }
    if (!readOnly) {
      const usage = this._getUsage();
      header(`Used in PlayUnits  (${usage.length})`);
      if (usage.length) {
        note([...usage.slice(0, 5), ...(usage.length > 5 ? [`… and ${usage.length - 5} more`] : [])], 'muted');
        note(['Renaming this table or a row key does not', 'update these paths.'], 'warning');
      } else {
        note(['No PlayUnit component refers to this table.'], 'muted');
      }
    }
    return items;
  },

  /**
   * Inspector の項目は毎フレーム作り直すが、Undo と同じフレームに操作されても古い列を触らないよう、操作の時点で列を引き直す。
   * @returns {object|null}
   */
  _freshColumn(column) {
    return this._getCurrentTable()?.columns.find(other => other.id === column.id) || null;
  },

  /** @returns {string} 選択中のセルを指すパス。 */
  _cellPath(table, row, column) {
    const base = `db.${this._current.kind}.${table.name}.${row.key}`;
    return column ? `${base}.${column.name}` : base;
  },

  _copyText(text, message) {
    this._clipboardText = text;
    if (navigator.clipboard?.writeText) {
      navigator.clipboard.writeText(text).then(() => this._setStatus(message, 'ok')).catch(() => this._setStatus(`${message} (internal only)`, 'warning'));
    } else {
      this._setStatus(`${message} (internal only)`, 'warning');
    }
  },

  // ---------------------------------------------------------------------------
  // 項目の編集（オーバーレイ入力）
  // ---------------------------------------------------------------------------

  /** Inspector の項目の値の位置にオーバーレイ入力を出す。 */
  _openFieldEditor(itemRect, text, onCommit) {
    const rect = { x: itemRect.x + 96, y: itemRect.y + 1, w: itemRect.w - 108, h: itemRect.h - 2 };
    this._openTextEditor({ target: 'field' }, rect, text, false, false, onCommit);
  },

  _editTableText(itemRect, property) {
    const table = this._getCurrentTable();
    this._openFieldEditor(itemRect, table[property] || '', (text) => {
      const value = text.trim();
      if (value === (table[property] || '')) return true;
      if (property === 'name') {
        if (!DatabaseData.isIdentifier(value)) {
          this._setStatus('Table name must be an identifier: letters, digits and _ (max 64)', 'error');
          return false;
        }
        if (this._project.database.tables.some(other => other.id !== table.id && other.name === value)) {
          this._setStatus(`Table name already exists: ${value}`, 'error');
          return false;
        }
      }
      const usage = property === 'name' ? this._getUsageCount() : 0;
      this._applyStructural(`Edit table ${property}`, (working) => {
        this._workingTable(working)[property] = value;
      });
      if (usage) this._setStatus(`Table renamed. ${usage} PlayUnit use(s) of the old name are not updated`, 'warning');
      return true;
    });
  },

  _pickTableKind(itemRect) {
    const table = this._getCurrentTable();
    this._popup.open({
      x: itemRect.x + 96, y: itemRect.y + itemRect.h, w: 160, searchable: false, title: 'Kind',
      items: [
        { value: 'const', label: 'const', detail: 'read-only at runtime' },
        { value: 'mutable', label: 'mutable', detail: 'saved state' },
      ],
      selectedValue: table.kind,
      onPick: (kind) => {
        if (kind === table.kind) return;
        const usage = this._getUsageCount();
        const applied = this._applyStructural('Change table kind', (working) => { this._workingTable(working).kind = kind; });
        if (applied) {
          this._current.kind = kind;
          this._usage = { key: '', items: [] };
          this._setStatus(usage ? `Kind changed. ${usage} PlayUnit use(s) refer to the old db.${table.kind}… path` : `Kind changed to ${kind}`, usage ? 'warning' : 'ok');
        }
      },
    });
  },

  _deleteTable() {
    if (this._isReadOnly()) return;
    const table = this._getCurrentTable();
    const referencing = DatabaseOps.findReferencingColumns(this._project.database, table.id)
      .filter(({ table: other }) => other.id !== table.id);
    if (referencing.length) {
      const first = referencing[0];
      this._setStatus(`Cannot delete: ${first.table.name}.${first.column.name} refers to this table${referencing.length > 1 ? ` (+${referencing.length - 1} more)` : ''}`, 'error');
      return;
    }
    const usage = this._getUsageCount();
    const id = table.id;
    const applied = this._applyStructural('Delete table', (working) => {
      working.tables = working.tables.filter(other => other.id !== id);
    });
    if (applied) {
      const next = this._project.database.tables[0];
      if (next) this._selectTable(next.kind, next.id);
      else { this._current = { kind: 'const', id: '' }; this._rebuildView(); }
      this._setStatus(`Table ${table.name} deleted (Ctrl+Z to undo)${usage ? `. ${usage} PlayUnit use(s) now point to nothing` : ''}`, usage ? 'warning' : 'ok');
    }
  },

  // ---------------------------------------------------------------------------
  // 列の設定
  // ---------------------------------------------------------------------------

  _editColumnText(itemRect, staleColumn, property) {
    const column = this._freshColumn(staleColumn);
    if (!column) return;
    const columnId = column.id;
    this._openFieldEditor(itemRect, column[property] || '', (text) => {
      const value = text.trim();
      if (value === (column[property] || '')) return true;
      const table = this._getCurrentTable();
      if (property === 'name') {
        if (!DatabaseData.isIdentifier(value)) {
          this._setStatus('Column name must be an identifier: letters, digits and _ (max 64)', 'error');
          return false;
        }
        if (table.columns.some(other => other.id !== columnId && other.name === value)) {
          this._setStatus(`Column name already exists: ${value}`, 'error');
          return false;
        }
      }
      const usage = property === 'name' ? this._getUsageCount() : 0;
      this._applyStructural(`Edit column ${property}`, (working) => {
        const workingTable = this._workingTable(working);
        const target = workingTable.columns.find(other => other.id === columnId);
        if (property === 'name') DatabaseOps.renameColumn(workingTable, target, value);
        else target[property] = value;
      });
      if (usage) this._setStatus(`Column renamed. ${usage} PlayUnit use(s) of the old name are not updated`, 'warning');
      return true;
    });
  },

  _pickColumnType(itemRect, staleColumn) {
    const column = this._freshColumn(staleColumn);
    if (!column) return;
    this._popup.open({
      x: itemRect.x + 96, y: itemRect.y + itemRect.h, w: 180, searchable: false, title: 'Column type',
      items: DatabaseData.COLUMN_TYPES.map(type => ({ value: type, label: type })),
      selectedValue: column.type,
      onPick: (type) => {
        if (type === column.type) return;
        const patch = { type };
        if (type === 'enum') patch.options = column.options?.length ? column.options : ['option1'];
        if (type === 'ref') patch.refTableId = column.refTableId || this._getCurrentTable().id;
        if (type === 'asset') patch.assetType = column.assetType || DatabaseData.ASSET_TYPES[0];
        patch.default = type === 'enum' ? patch.options[0] : DatabaseData.getTypeDefault({ ...column, ...patch });
        this._reconfigureColumn(column.id, patch, 'Change column type');
      },
    });
  },

  _pickRefTable(itemRect, staleColumn) {
    const column = this._freshColumn(staleColumn);
    if (!column) return;
    this._popup.open({
      x: itemRect.x + 96, y: itemRect.y + itemRect.h, w: 200, title: 'Target table',
      items: this._project.database.tables.map(table => ({ value: table.id, label: table.name, detail: table.kind })),
      selectedValue: column.refTableId,
      onPick: (tableId) => {
        if (tableId !== column.refTableId) this._reconfigureColumn(column.id, { refTableId: tableId, default: '' }, 'Change ref target');
      },
    });
  },

  _pickAssetType(itemRect, staleColumn) {
    const column = this._freshColumn(staleColumn);
    if (!column) return;
    this._popup.open({
      x: itemRect.x + 96, y: itemRect.y + itemRect.h, w: 200, searchable: false, title: 'Asset type',
      items: DatabaseData.ASSET_TYPES.map(type => ({ value: type, label: type })),
      selectedValue: column.assetType,
      onPick: (assetType) => {
        if (assetType !== column.assetType) this._reconfigureColumn(column.id, { assetType, default: '' }, 'Change asset type');
      },
    });
  },

  _editColumnOptions(itemRect, staleColumn) {
    const column = this._freshColumn(staleColumn);
    if (!column) return;
    this._openFieldEditor(itemRect, (column.options || []).join(', '), (text) => {
      const options = [...new Set(text.split(',').map(option => option.trim()).filter(Boolean))];
      if (!options.length) {
        this._setStatus('An enum column needs at least one option (comma separated)', 'error');
        return false;
      }
      const patch = { options, default: options.includes(column.default) ? column.default : options[0] };
      this._reconfigureColumn(column.id, patch, 'Edit enum options');
      return true;
    });
  },

  _editColumnDefault(itemRect, staleColumn) {
    const column = this._freshColumn(staleColumn);
    if (!column) return;
    if (column.type === 'boolean') {
      this._reconfigureColumn(column.id, { default: column.default !== true }, 'Edit default');
      return;
    }
    if (column.type === 'enum') {
      this._popup.open({
        x: itemRect.x + 96, y: itemRect.y + itemRect.h, w: 180, searchable: false, title: 'Default',
        items: (column.options || []).map(option => ({ value: option, label: option })),
        selectedValue: column.default,
        onPick: (value) => this._reconfigureColumn(column.id, { default: value }, 'Edit default'),
      });
      return;
    }
    this._openFieldEditor(itemRect, DatabaseOps.cellToText(column.default, column), (text) => {
      const result = DatabaseOps.textToCellValue(text, column, DatabaseData.createContext(this._project));
      if (!result.ok) {
        this._setStatus(`Default: ${result.message}`, 'error');
        return false;
      }
      this._reconfigureColumn(column.id, { default: result.value }, 'Edit default');
      return true;
    });
  },

  /**
   * 列の型や設定を変え、既存のセルを新しい型へ変換する（変換できないセルは既定値に戻して知らせる）。
   */
  _reconfigureColumn(columnId, patch, label) {
    let failed = [];
    const applied = this._applyStructural(label, (working) => {
      const table = this._workingTable(working);
      const column = table.columns.find(other => other.id === columnId);
      if (!column) return false;
      failed = DatabaseOps.reconfigureColumn(working, table, column, patch);
      return true;
    });
    if (applied && failed.length) {
      const sample = failed.slice(0, 3).map(item => item.rowKey).join(', ');
      this._setStatus(`${failed.length} cell(s) could not be converted and were reset to the default (${sample}${failed.length > 3 ? ', …' : ''}). Ctrl+Z to undo`, 'warning');
    }
    return applied;
  },

  _editRowKey(itemRect, viewRow) {
    const table = this._getCurrentTable();
    const row = table.rows[this._viewRows[viewRow]];
    this._openFieldEditor(itemRect, row.key, (text) => this._commitKey(viewRow, text, 'none'));
  },
});
