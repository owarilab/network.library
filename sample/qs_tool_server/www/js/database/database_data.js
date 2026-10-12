/**
 * DatabaseData
 * プロジェクトのデータベース（project.database）の定義を扱う静的ユーティリティ。
 *
 * 構造:
 *   { version: 1, tables: [{ id, name, label, kind, columns: [...], rows: [{ key, cells }] }] }
 * - kind: 'const'（実行時は読み取り専用）/ 'mutable'（実行時にセルの値を変更できる）
 * - 識別子（テーブル名、列名、行の key）は IDENTIFIER_PATTERN に従う。実行時のパスの区切りが '.' のため。
 * - ref 列は参照先テーブルを内部 ID（refTableId）で持つ（テーブル名を変えても参照が切れない）。
 *
 * 実行時のパス: db.<system|const|mutable>.<table>.<row>.<column>（DATABASE_DESIGN.md を参照）
 */
class DatabaseData {
  static VERSION = 1;
  static TABLE_KINDS = ['const', 'mutable'];
  static COLUMN_TYPES = ['string', 'text', 'number', 'integer', 'boolean', 'enum', 'ref', 'asset', 'json'];
  static ASSET_TYPES = ['map', 'playUnit', 'pixelDocument', 'tileset', 'characterChipset'];
  /** 実行時のパスに現れる DB の種類 */
  static PATH_KINDS = ['system', 'const', 'mutable'];
  static IDENTIFIER_PATTERN = /^[A-Za-z_][A-Za-z0-9_]*$/;
  static MAX_NAME_LENGTH = 64;
  static MAX_TABLES = 128;
  static MAX_COLUMNS = 64;
  static MAX_ROWS = 5000;

  /**
   * 空の DB 定義を作る。
   * @returns {{ version: number, tables: object[] }}
   */
  static createDefault() {
    return { version: DatabaseData.VERSION, tables: [] };
  }

  /**
   * 内部 ID を作る（tbl_ / col_ など）。
   * @param {string} prefix
   * @returns {string}
   */
  static createId(prefix) {
    const random = Math.random().toString(36).slice(2, 8).padEnd(6, '0');
    return `${prefix}_${Date.now().toString(36)}_${random}`;
  }

  /**
   * @param {any} name
   * @returns {boolean}
   */
  static isIdentifier(name) {
    return typeof name === 'string'
      && name.length > 0
      && name.length <= DatabaseData.MAX_NAME_LENGTH
      && DatabaseData.IDENTIFIER_PATTERN.test(name);
  }

  // ---------------------------------------------------------------------------
  // 検索
  // ---------------------------------------------------------------------------

  static findTableByName(database, name) {
    return (database?.tables || []).find(table => table.name === name) || null;
  }

  static findTableById(database, id) {
    return (database?.tables || []).find(table => table.id === id) || null;
  }

  static findColumnByName(table, name) {
    return (table?.columns || []).find(column => column.name === name) || null;
  }

  static findRowByKey(table, key) {
    return (table?.rows || []).find(row => row.key === key) || null;
  }

  // ---------------------------------------------------------------------------
  // 列の型
  // ---------------------------------------------------------------------------

  /**
   * 列の既定値（セルが無いとき、行を追加したときの値）。
   * @param {object} column
   * @returns {any}
   */
  static getColumnDefault(column) {
    const result = DatabaseData.coerceCellValue(column?.default, column);
    if (result.ok) return result.value;
    return DatabaseData.getTypeDefault(column);
  }

  /**
   * 型ごとの初期値（column.default が使えないときの値）。
   * @param {object} column
   * @returns {any}
   */
  static getTypeDefault(column) {
    switch (column?.type) {
      case 'number':
      case 'integer':
        return 0;
      case 'boolean':
        return false;
      case 'enum':
        return Array.isArray(column.options) && column.options.length ? column.options[0] : '';
      case 'json':
        return null;
      default:
        return '';
    }
  }

  /**
   * セルの値を列の型へ変換する。変換できなければ ok: false と理由を返す。
   * ctx を渡すと、ref の行と asset の存在も確認する（渡さなければ型の形だけ見る）。
   * @param {any} value
   * @param {object} column
   * @param {{ database?: object, assetExists?: (assetType: string, id: string) => boolean, hasRow?: (table: object, key: string) => boolean }|null} [ctx]
   * @returns {{ ok: boolean, value?: any, message?: string }}
   */
  static coerceCellValue(value, column, ctx = null) {
    const type = column?.type;
    switch (type) {
      case 'string':
      case 'text': {
        if (typeof value === 'string') return { ok: true, value };
        if (typeof value === 'number' || typeof value === 'boolean') return { ok: true, value: String(value) };
        return { ok: false, message: 'value must be a string' };
      }
      case 'number': {
        const result = DatabaseData._coerceNumber(value);
        return result.ok ? result : { ok: false, message: 'value must be a finite number' };
      }
      case 'integer': {
        const result = DatabaseData._coerceNumber(value);
        if (!result.ok || !Number.isInteger(result.value)) return { ok: false, message: 'value must be an integer' };
        return result;
      }
      case 'boolean': {
        if (typeof value === 'boolean') return { ok: true, value };
        if (value === 1 || value === 0) return { ok: true, value: value === 1 };
        if (typeof value === 'string') {
          const normalized = value.trim().toLowerCase();
          if (normalized === 'true' || normalized === '1') return { ok: true, value: true };
          if (normalized === 'false' || normalized === '0') return { ok: true, value: false };
        }
        return { ok: false, message: 'value must be a boolean' };
      }
      case 'enum': {
        const options = Array.isArray(column.options) ? column.options : [];
        if (typeof value === 'string' && options.includes(value)) return { ok: true, value };
        return { ok: false, message: `value must be one of: ${options.join(', ')}` };
      }
      case 'ref': {
        if (typeof value !== 'string') return { ok: false, message: 'value must be a row key' };
        if (value === '' || !ctx?.database) return { ok: true, value };
        const table = DatabaseData.findTableById(ctx.database, column.refTableId);
        if (!table) return { ok: false, message: 'referenced table was not found' };
        // ctx.hasRow は検証が大量のセルを調べるときの索引（無ければ線形に探す）
        const exists = typeof ctx.hasRow === 'function' ? ctx.hasRow(table, value) : !!DatabaseData.findRowByKey(table, value);
        if (!exists) return { ok: false, message: `row not found in ${table.name}: ${value}` };
        return { ok: true, value };
      }
      case 'asset': {
        if (typeof value !== 'string') return { ok: false, message: 'value must be an asset id' };
        if (value === '' || typeof ctx?.assetExists !== 'function') return { ok: true, value };
        if (!ctx.assetExists(column.assetType, value)) {
          return { ok: false, message: `${column.assetType} not found: ${value}` };
        }
        return { ok: true, value };
      }
      case 'json': {
        if (value === undefined) return { ok: false, message: 'value must be JSON serializable' };
        try {
          const text = JSON.stringify(value);
          if (text === undefined) return { ok: false, message: 'value must be JSON serializable' };
          return { ok: true, value: JSON.parse(text) };
        } catch {
          return { ok: false, message: 'value must be JSON serializable' };
        }
      }
      default:
        return { ok: false, message: 'unsupported column type' };
    }
  }

  static _coerceNumber(value) {
    if (typeof value === 'number' && Number.isFinite(value)) return { ok: true, value };
    if (typeof value === 'string' && value.trim() !== '') {
      const parsed = Number(value.trim());
      if (Number.isFinite(parsed)) return { ok: true, value: parsed };
    }
    return { ok: false };
  }

  /**
   * project から ref / asset の確認用 ctx を作る。
   * @param {ProjectData|null} project
   * @returns {{ database: object|null, assetExists: (assetType: string, id: string) => boolean }}
   */
  static createContext(project) {
    return {
      database: project?.database || null,
      assetExists: (assetType, id) => {
        if (!project) return false;
        switch (assetType) {
          case 'map': return !!project.findMapById?.(id);
          case 'playUnit': return !!project.findPlayUnitById?.(id);
          case 'pixelDocument': return !!project.findPixelDocumentById?.(id);
          case 'tileset': return !!project.findTilesetById?.(id);
          case 'characterChipset': return !!project.findCharacterChipsetById?.(id);
          default: return false;
        }
      },
    };
  }

  // ---------------------------------------------------------------------------
  // 正規化
  // ---------------------------------------------------------------------------

  /**
   * 読み込んだ DB 定義の形を整える（壊れた入力で落ちないようにする）。
   * 値の存在確認（ref / asset）はしない。名前の重複や上限は validate が報告する。
   * @param {any} source
   * @returns {{ version: number, tables: object[] }}
   */
  static normalize(source) {
    const root = source && typeof source === 'object' && !Array.isArray(source) ? source : {};
    const tables = Array.isArray(root.tables) ? root.tables : [];
    return {
      version: Number.isInteger(root.version) ? root.version : DatabaseData.VERSION,
      tables: tables
        .filter(table => table && typeof table === 'object' && !Array.isArray(table))
        .map(table => DatabaseData._normalizeTable(table)),
    };
  }

  static _normalizeTable(source) {
    const columns = (Array.isArray(source.columns) ? source.columns : [])
      .filter(column => column && typeof column === 'object' && !Array.isArray(column))
      .map(column => DatabaseData._normalizeColumn(column));
    const rows = (Array.isArray(source.rows) ? source.rows : [])
      .filter(row => row && typeof row === 'object' && !Array.isArray(row))
      .map(row => DatabaseData._normalizeRow(row, columns));
    return {
      id: typeof source.id === 'string' && source.id ? source.id : DatabaseData.createId('tbl'),
      name: typeof source.name === 'string' ? source.name.trim() : '',
      label: typeof source.label === 'string' ? source.label : '',
      kind: DatabaseData.TABLE_KINDS.includes(source.kind) ? source.kind : 'const',
      columns,
      rows,
    };
  }

  static _normalizeColumn(source) {
    const type = DatabaseData.COLUMN_TYPES.includes(source.type) ? source.type : 'string';
    const column = {
      id: typeof source.id === 'string' && source.id ? source.id : DatabaseData.createId('col'),
      name: typeof source.name === 'string' ? source.name.trim() : '',
      label: typeof source.label === 'string' ? source.label : '',
      type,
      default: undefined,
    };
    if (type === 'enum') {
      const options = (Array.isArray(source.options) ? source.options : [])
        .filter(option => typeof option === 'string' && option !== '');
      column.options = [...new Set(options)];
    }
    if (type === 'ref') column.refTableId = typeof source.refTableId === 'string' ? source.refTableId : '';
    if (type === 'asset') {
      column.assetType = DatabaseData.ASSET_TYPES.includes(source.assetType) ? source.assetType : DatabaseData.ASSET_TYPES[0];
    }
    column.default = DatabaseData.getColumnDefault({ ...column, default: source.default });
    return column;
  }

  static _normalizeRow(source, columns) {
    const sourceCells = source.cells && typeof source.cells === 'object' && !Array.isArray(source.cells) ? source.cells : {};
    const cells = {};
    for (const column of columns) {
      if (!Object.prototype.hasOwnProperty.call(sourceCells, column.name)) {
        cells[column.name] = DatabaseData.getColumnDefault(column);
        continue;
      }
      const result = DatabaseData.coerceCellValue(sourceCells[column.name], column);
      cells[column.name] = result.ok ? result.value : DatabaseData.getColumnDefault(column);
    }
    return { key: typeof source.key === 'string' ? source.key.trim() : '', cells };
  }

  /**
   * DB 定義を深くコピーする。
   * @param {object} database
   * @returns {object}
   */
  static clone(database) {
    return JSON.parse(JSON.stringify(DatabaseData.normalize(database)));
  }

  // ---------------------------------------------------------------------------
  // 検証
  // ---------------------------------------------------------------------------

  /**
   * DB 定義を検証する。保存前と、エディタの警告表示に使う。
   * @param {object|null|undefined} database
   * @param {ProjectData|null} [project]  ref / asset の存在確認に使う（無ければ形だけ見る）
   * @returns {{ ok: boolean, message: string, issues: Array<{ tableName: string, rowKey?: string, columnName?: string, message: string }> }}
   */
  static validate(database, project = null) {
    const issues = [];
    const add = (tableName, message, rowKey, columnName) => {
      const issue = { tableName, message };
      if (rowKey !== undefined) issue.rowKey = rowKey;
      if (columnName !== undefined) issue.columnName = columnName;
      issues.push(issue);
    };
    const tables = Array.isArray(database?.tables) ? database.tables : [];
    const ctx = project ? DatabaseData.createContext(project) : null;
    // ref の確認が行数の二乗にならないよう、テーブルごとの行 key の集合を一度だけ作る
    const rowKeySets = new Map();
    const hasRow = (table, key) => {
      if (!rowKeySets.has(table)) rowKeySets.set(table, new Set(table.rows.map(row => row.key)));
      return rowKeySets.get(table).has(key);
    };
    const ctxWithDb = { ...(ctx || {}), database, hasRow };

    if (tables.length > DatabaseData.MAX_TABLES) add('', `too many tables (max ${DatabaseData.MAX_TABLES})`);

    const tableNames = new Set();
    const tableIds = new Set();
    for (const table of tables) {
      const tableName = table.name || '(unnamed)';
      if (!DatabaseData.isIdentifier(table.name)) add(tableName, 'table name must be an identifier (letters, digits, _ ; max 64)');
      else if (tableNames.has(table.name)) add(tableName, 'duplicate table name');
      tableNames.add(table.name);
      if (tableIds.has(table.id)) add(tableName, 'duplicate table id');
      tableIds.add(table.id);

      if (table.columns.length > DatabaseData.MAX_COLUMNS) add(tableName, `too many columns (max ${DatabaseData.MAX_COLUMNS})`);
      if (table.rows.length > DatabaseData.MAX_ROWS) add(tableName, `too many rows (max ${DatabaseData.MAX_ROWS})`);

      const columnNames = new Set();
      for (const column of table.columns) {
        if (!DatabaseData.isIdentifier(column.name)) add(tableName, 'column name must be an identifier', undefined, column.name || '(unnamed)');
        else if (columnNames.has(column.name)) add(tableName, 'duplicate column name', undefined, column.name);
        columnNames.add(column.name);
        if (column.type === 'enum' && !(column.options || []).length) add(tableName, 'enum column needs at least one option', undefined, column.name);
        if (column.type === 'ref' && !DatabaseData.findTableById(database, column.refTableId)) {
          add(tableName, 'ref column points to a missing table', undefined, column.name);
        }
      }

      const rowKeys = new Set();
      for (const row of table.rows) {
        if (!DatabaseData.isIdentifier(row.key)) add(tableName, 'row key must be an identifier', row.key || '(empty)');
        else if (rowKeys.has(row.key)) add(tableName, 'duplicate row key', row.key);
        rowKeys.add(row.key);
        for (const column of table.columns) {
          const result = DatabaseData.coerceCellValue(row.cells?.[column.name], column, ctxWithDb);
          if (!result.ok) add(tableName, result.message, row.key, column.name);
        }
      }
    }

    return { ok: issues.length === 0, message: issues[0] ? DatabaseData.formatIssue(issues[0]) : '', issues };
  }

  /**
   * @param {{ tableName: string, rowKey?: string, columnName?: string, message: string }} issue
   * @returns {string}
   */
  static formatIssue(issue) {
    const location = [issue.tableName, issue.rowKey, issue.columnName].filter(part => part !== undefined && part !== '').join('.');
    return location ? `${location}: ${issue.message}` : issue.message;
  }

  // ---------------------------------------------------------------------------
  // 実行時のパス
  // ---------------------------------------------------------------------------

  /**
   * "db.<kind>.<table>[.<row>[.<column>]]" を分解する。db のパスでなければ null、形が不正なら { error }。
   * @param {any} path
   * @returns {{ kind: string, table: string, row: string|null, column: string|null }|{ error: string }|null}
   */
  static parsePath(path) {
    if (typeof path !== 'string') return null;
    const trimmed = path.trim();
    if (!trimmed.startsWith('db.')) return null;
    const parts = trimmed.split('.');
    if (parts.length < 3 || parts.length > 5) {
      return { error: `invalid database path: ${trimmed} (db.<kind>.<table>[.<row>[.<column>]])` };
    }
    const [, kind, table, row = null, column = null] = parts;
    if (!DatabaseData.PATH_KINDS.includes(kind)) {
      return { error: `unknown database kind: ${kind} (system, const, mutable)` };
    }
    for (const part of [table, row, column]) {
      if (part !== null && !DatabaseData.isIdentifier(part)) {
        return { error: `invalid database path: ${trimmed}` };
      }
    }
    return { kind, table, row, column };
  }

  // ---------------------------------------------------------------------------
  // 実行用の状態（可変 DB のコピー）
  // ---------------------------------------------------------------------------

  /**
   * 可変 DB のセルの値だけを取り出して、実行用の状態を作る。
   * 定義は変更しない（行・列・初期値は定義側に残る）。
   * @param {object} database
   * @returns {{ [tableName: string]: { [rowKey: string]: { [columnName: string]: any } } }}
   */
  static createMutableState(database) {
    const state = {};
    for (const table of database?.tables || []) {
      if (table.kind !== 'mutable' || !DatabaseData.isIdentifier(table.name)) continue;
      const rows = {};
      for (const row of table.rows) {
        if (!DatabaseData.isIdentifier(row.key)) continue;
        const cells = {};
        for (const column of table.columns) {
          if (!DatabaseData.isIdentifier(column.name)) continue;
          const result = DatabaseData.coerceCellValue(row.cells?.[column.name], column);
          cells[column.name] = result.ok ? result.value : DatabaseData.getColumnDefault(column);
        }
        rows[row.key] = cells;
      }
      state[table.name] = rows;
    }
    return state;
  }

  /**
   * 状態を、定義に合わせて読み込む（将来のセーブデータ用）。
   * 定義に無いテーブル・行・列は無視し、状態に無いセルは初期値のまま、型に合わない値も初期値のままにする。
   * @param {object} database
   * @param {any} savedState
   * @returns {{ [tableName: string]: { [rowKey: string]: { [columnName: string]: any } } }}
   */
  static applySavedState(database, savedState) {
    const state = DatabaseData.createMutableState(database);
    if (!savedState || typeof savedState !== 'object') return state;
    for (const table of database?.tables || []) {
      const savedRows = savedState[table.name];
      if (table.kind !== 'mutable' || !savedRows || typeof savedRows !== 'object') continue;
      for (const column of table.columns) {
        for (const [rowKey, cells] of Object.entries(state[table.name] || {})) {
          const savedCells = savedRows[rowKey];
          if (!savedCells || !Object.prototype.hasOwnProperty.call(savedCells, column.name)) continue;
          const result = DatabaseData.coerceCellValue(savedCells[column.name], column, { database });
          if (result.ok) cells[column.name] = result.value;
        }
      }
    }
    return state;
  }
}
