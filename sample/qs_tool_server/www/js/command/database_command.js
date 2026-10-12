/**
 * Database Editor 用の Undo / Redo コマンド。
 * Database Editor は専用の HistoryManager を使う（appData.history はシーンをまたいで残るため）。
 */

/**
 * 複数のセルの値の変更（編集、貼り付け、クリア）。
 */
class DatabaseSetCellsCommand extends CommandBase {
  /**
   * @param {ProjectData} project
   * @param {string} tableId
   * @param {Array<{ rowKey: string, columnName: string, before: any, after: any }>} changes
   * @param {string} [label]
   */
  constructor(project, tableId, changes, label = 'Edit cells') {
    super();
    this._project = project;
    this._tableId = tableId;
    this._changes = changes;
    this._label = label;
  }

  execute() {
    return this._apply('after');
  }

  undo() {
    return this._apply('before');
  }

  _apply(field) {
    const table = DatabaseData.findTableById(this._project.database, this._tableId);
    if (!table) return false;
    for (const change of this._changes) {
      const row = DatabaseData.findRowByKey(table, change.rowKey);
      if (!row) continue;
      row.cells[change.columnName] = JSON.parse(JSON.stringify(change[field] ?? null));
    }
    return true;
  }

  getLabel() {
    return this._label;
  }
}

/**
 * テーブル単位のスナップショット（行・列・テーブルの追加、削除、並べ替え、型変更、名前変更）。
 * 複数のテーブルを同時に扱える（行 key の変更で、参照している他テーブルも直るため）。
 */
class DatabaseTablesCommand extends CommandBase {
  /**
   * @param {ProjectData} project
   * @param {string} label
   * @param {Array<{ tableId: string, before: object|null, after: object|null, beforeIndex: number, afterIndex: number }>} entries
   */
  constructor(project, label, entries) {
    super();
    this._project = project;
    this._label = label;
    this._entries = entries;
  }

  execute() {
    return this._apply('after', 'afterIndex');
  }

  undo() {
    return this._apply('before', 'beforeIndex');
  }

  _apply(field, indexField) {
    const tables = this._project.database.tables;
    // 先に全て取り除いてから、位置の小さい順に戻す（複数のテーブルの位置が入れ替わっても崩れない）
    for (const entry of this._entries) {
      const index = tables.findIndex(table => table.id === entry.tableId);
      if (index >= 0) tables.splice(index, 1);
    }
    const inserts = this._entries
      .filter(entry => entry[field])
      .sort((a, b) => a[indexField] - b[indexField]);
    for (const entry of inserts) {
      const index = Math.min(Math.max(0, entry[indexField]), tables.length);
      tables.splice(index, 0, JSON.parse(JSON.stringify(entry[field])));
    }
    return true;
  }

  getLabel() {
    return this._label;
  }

  /**
   * 変更前のテーブルから、スナップショットのエントリを作る補助。
   * @param {object|null} before  変更前のテーブル（JSON）。新規なら null
   * @param {object|null} after   変更後のテーブル。削除なら null
   * @param {number} beforeIndex
   * @param {number} afterIndex
   */
  static entry(before, after, beforeIndex, afterIndex) {
    return {
      tableId: (before || after).id,
      before: before ? JSON.parse(JSON.stringify(before)) : null,
      after: after ? JSON.parse(JSON.stringify(after)) : null,
      beforeIndex,
      afterIndex,
    };
  }
}
