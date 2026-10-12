/**
 * DatabaseOps
 * DB 定義への編集操作（行・列・テーブルの追加、削除、並べ替え、型変更、名前変更）と、TSV の入出力。
 * UI から独立した純粋な関数で、渡されたテーブルやデータベースのオブジェクトをその場で書き換える。
 * Undo は呼び出し側（DatabaseTablesCommand）が、操作の前後のテーブルを保存して行う。
 */
class DatabaseOps {
  /**
   * 既存の名前と重なりにくい識別子を作る（base、base2、base3 …）。
   * @param {Iterable<string>} existing
   * @param {string} base
   * @returns {string}
   */
  static uniqueIdentifier(existing, base) {
    const used = new Set(existing);
    if (!used.has(base)) return base;
    for (let n = 2; n < 100000; n++) {
      const candidate = `${base}${n}`;
      if (!used.has(candidate)) return candidate;
    }
    return `${base}_${Date.now().toString(36)}`;
  }

  // ---------------------------------------------------------------------------
  // テーブル
  // ---------------------------------------------------------------------------

  /**
   * 新しいテーブルを作る（database には追加しない）。列は label の 1 つだけ。
   * @param {object} database
   * @param {'const'|'mutable'} kind
   * @returns {object}
   */
  static createTable(database, kind) {
    const name = DatabaseOps.uniqueIdentifier(database.tables.map(table => table.name), kind === 'mutable' ? 'state' : 'table');
    return {
      id: DatabaseData.createId('tbl'),
      name,
      label: '',
      kind: DatabaseData.TABLE_KINDS.includes(kind) ? kind : 'const',
      columns: [{ id: DatabaseData.createId('col'), name: 'label', label: '', type: 'string', default: '' }],
      rows: [],
    };
  }

  /**
   * このテーブルを参照している ref 列（他テーブルも含む）を返す。
   * @returns {Array<{ table: object, column: object }>}
   */
  static findReferencingColumns(database, tableId) {
    const result = [];
    for (const table of database.tables) {
      for (const column of table.columns) {
        if (column.type === 'ref' && column.refTableId === tableId) result.push({ table, column });
      }
    }
    return result;
  }

  // ---------------------------------------------------------------------------
  // 行
  // ---------------------------------------------------------------------------

  /**
   * 既定値の行を作って挿入する。
   * @param {object} table
   * @param {number} [index]  挿入位置（省略で末尾）
   * @param {string} [baseKey='row']
   * @returns {object|null}  上限に達したら null
   */
  static addRow(table, index = table.rows.length, baseKey = 'row') {
    if (table.rows.length >= DatabaseData.MAX_ROWS) return null;
    const key = DatabaseOps.uniqueIdentifier(table.rows.map(row => row.key), baseKey);
    const cells = {};
    for (const column of table.columns) cells[column.name] = DatabaseData.getColumnDefault(column);
    const row = { key, cells };
    table.rows.splice(Math.min(Math.max(0, index), table.rows.length), 0, row);
    return row;
  }

  /**
   * 指定した行を、それぞれの直後に複製する。
   * @param {object} table
   * @param {number[]} indices
   * @returns {number[]}  複製された行の新しい位置
   */
  static duplicateRows(table, indices) {
    const sorted = [...new Set(indices)].sort((a, b) => a - b).filter(i => i >= 0 && i < table.rows.length);
    const created = [];
    let shift = 0;
    for (const original of sorted) {
      if (table.rows.length >= DatabaseData.MAX_ROWS) break;
      const source = table.rows[original + shift];
      const key = DatabaseOps.uniqueIdentifier(table.rows.map(row => row.key), `${source.key}_copy`);
      const copy = { key, cells: JSON.parse(JSON.stringify(source.cells)) };
      table.rows.splice(original + shift + 1, 0, copy);
      shift += 1;
      created.push(original + shift);
    }
    return created;
  }

  /**
   * @param {object} table
   * @param {number[]} indices
   * @returns {number}  削除した行数
   */
  static deleteRows(table, indices) {
    const set = new Set(indices);
    const before = table.rows.length;
    table.rows = table.rows.filter((_, index) => !set.has(index));
    return before - table.rows.length;
  }

  /**
   * 選択した行（連続していなくてよい）を delta だけ動かす。端でそれ以上動けなければ動かさない。
   * @param {object} table
   * @param {number[]} indices
   * @param {number} delta  -1（上へ）または 1（下へ）
   * @returns {number[]}  動いた後の位置（動けなければ空）
   */
  static moveRows(table, indices, delta) {
    const sorted = [...new Set(indices)].sort((a, b) => a - b);
    if (!sorted.length || (delta < 0 && sorted[0] === 0) || (delta > 0 && sorted[sorted.length - 1] === table.rows.length - 1)) return [];
    const order = delta < 0 ? sorted : [...sorted].reverse();
    for (const index of order) {
      const target = index + delta;
      [table.rows[index], table.rows[target]] = [table.rows[target], table.rows[index]];
    }
    return sorted.map(index => index + delta);
  }

  /**
   * 行の key を変える。他テーブルの ref セルのうち、この行を指すものも新しい key に直す。
   * @param {object} database
   * @param {object} table
   * @param {object} row
   * @param {string} newKey  識別子であること、重複しないことは呼び出し側が確認する
   * @returns {object[]}  書き換えたテーブル（table 自身を含む）
   */
  static renameRowKey(database, table, row, newKey) {
    const oldKey = row.key;
    row.key = newKey;
    const touched = new Set([table]);
    for (const { table: refTable, column } of DatabaseOps.findReferencingColumns(database, table.id)) {
      if (column.default === oldKey) column.default = newKey;
      for (const refRow of refTable.rows) {
        if (refRow.cells[column.name] === oldKey) {
          refRow.cells[column.name] = newKey;
          touched.add(refTable);
        }
      }
      touched.add(refTable);
    }
    return [...touched];
  }

  // ---------------------------------------------------------------------------
  // 列
  // ---------------------------------------------------------------------------

  /**
   * @param {object} table
   * @param {number} [index]
   * @returns {object|null}  上限に達したら null
   */
  static addColumn(table, index = table.columns.length) {
    if (table.columns.length >= DatabaseData.MAX_COLUMNS) return null;
    const name = DatabaseOps.uniqueIdentifier(table.columns.map(column => column.name), 'column');
    const column = { id: DatabaseData.createId('col'), name, label: '', type: 'string', default: '' };
    table.columns.splice(Math.min(Math.max(0, index), table.columns.length), 0, column);
    for (const row of table.rows) row.cells[name] = DatabaseData.getColumnDefault(column);
    return column;
  }

  static deleteColumn(table, index) {
    const column = table.columns[index];
    if (!column) return false;
    table.columns.splice(index, 1);
    for (const row of table.rows) delete row.cells[column.name];
    return true;
  }

  /** @returns {number} 動いた後の位置。動けなければ -1 */
  static moveColumn(table, index, delta) {
    const target = index + delta;
    if (index < 0 || index >= table.columns.length || target < 0 || target >= table.columns.length) return -1;
    [table.columns[index], table.columns[target]] = [table.columns[target], table.columns[index]];
    return target;
  }

  /** 列の名前を変える（セルの持ち主のキーも直す）。 */
  static renameColumn(table, column, newName) {
    const oldName = column.name;
    column.name = newName;
    for (const row of table.rows) {
      if (Object.prototype.hasOwnProperty.call(row.cells, oldName)) {
        row.cells[newName] = row.cells[oldName];
        delete row.cells[oldName];
      }
    }
  }

  /**
   * 列の型や設定（type / options / refTableId / assetType / default）を変え、既存のセルを新しい型へ変換する。
   * 変換できないセルは列の既定値にして、その一覧を返す。
   * @param {object} database
   * @param {object} table
   * @param {object} column
   * @param {object} patch
   * @returns {Array<{ rowKey: string, value: any }>}  既定値に置き換えたセル
   */
  static reconfigureColumn(database, table, column, patch) {
    const previousType = column.type;
    for (const key of ['type', 'options', 'refTableId', 'assetType', 'default', 'label']) {
      if (Object.prototype.hasOwnProperty.call(patch, key)) column[key] = patch[key];
    }
    // 型に属さない設定は捨てる
    if (column.type !== 'enum') delete column.options;
    if (column.type !== 'ref') delete column.refTableId;
    if (column.type !== 'asset') delete column.assetType;
    if (column.type === 'enum' && !Array.isArray(column.options)) column.options = [];
    if (column.type === 'asset' && !DatabaseData.ASSET_TYPES.includes(column.assetType)) column.assetType = DatabaseData.ASSET_TYPES[0];
    if (column.type === 'ref' && typeof column.refTableId !== 'string') column.refTableId = '';

    // ref は参照先に行があるかも確認する（参照先を変えたとき、存在しない key を残さない）
    const ctx = { database };
    const defaultResult = DatabaseData.coerceCellValue(column.default, column, ctx);
    column.default = defaultResult.ok ? defaultResult.value : DatabaseData.getTypeDefault(column);

    const failed = [];
    for (const row of table.rows) {
      const value = row.cells[column.name];
      const converted = DatabaseOps.convertValue(value, previousType, column, ctx);
      if (converted.ok) {
        row.cells[column.name] = converted.value;
      } else {
        failed.push({ rowKey: row.key, value });
        row.cells[column.name] = DatabaseData.getColumnDefault(column);
      }
    }
    return failed;
  }

  /** 型が変わるときの値の変換（text として読み直せる値は読み直す）。 */
  static convertValue(value, previousType, column, ctx = null) {
    const direct = DatabaseData.coerceCellValue(value, column, ctx);
    if (direct.ok) return direct;
    if (column.type === 'json' || value === null || value === undefined) return { ok: false };
    const text = DatabaseOps.cellToText(value, { type: previousType });
    return DatabaseOps.textToCellValue(text, column, ctx);
  }

  // ---------------------------------------------------------------------------
  // セルの値とテキスト（コピー&ペースト、セルの編集に共通）
  // ---------------------------------------------------------------------------

  /**
   * セルの値を、編集欄やクリップボードに出すテキストにする。
   * @param {any} value
   * @param {{ type: string }} column
   * @returns {string}
   */
  static cellToText(value, column) {
    if (value === null || value === undefined) return '';
    if (column?.type === 'json') return JSON.stringify(value);
    if (typeof value === 'object') return JSON.stringify(value);
    return String(value);
  }

  /**
   * 入力したテキストを、列の型の値へ変換する。
   * @param {string} text
   * @param {object} column
   * @param {object|null} [ctx]  ref / asset の存在確認（省略すると型の形だけ見る）
   * @returns {{ ok: boolean, value?: any, message?: string }}
   */
  static textToCellValue(text, column, ctx = null) {
    const source = String(text ?? '');
    switch (column.type) {
      case 'json': {
        if (source.trim() === '') return { ok: true, value: null };
        try {
          return DatabaseData.coerceCellValue(JSON.parse(source), column, ctx);
        } catch {
          return { ok: false, message: 'value must be valid JSON' };
        }
      }
      case 'number':
      case 'integer':
        return DatabaseData.coerceCellValue(source.trim(), column, ctx);
      case 'boolean':
        return DatabaseData.coerceCellValue(source.trim(), column, ctx);
      case 'enum': {
        const trimmed = source.trim();
        const options = column.options || [];
        const exact = options.includes(trimmed) ? trimmed : options.find(option => option.toLowerCase() === trimmed.toLowerCase());
        return DatabaseData.coerceCellValue(exact ?? trimmed, column, ctx);
      }
      case 'ref':
      case 'asset':
        return DatabaseData.coerceCellValue(source.trim(), column, ctx);
      default:
        return DatabaseData.coerceCellValue(source, column, ctx);
    }
  }

  // ---------------------------------------------------------------------------
  // TSV
  // ---------------------------------------------------------------------------

  /**
   * 2 次元の文字列を TSV にする（タブ・改行・引用符を含むセルは "…" で囲む。Excel と同じ）。
   * @param {string[][]} rows
   * @returns {string}
   */
  static formatTsv(rows) {
    return rows.map(row => row.map((cell) => {
      const text = String(cell ?? '');
      return /[\t\n\r"]/.test(text) ? `"${text.replace(/"/g, '""')}"` : text;
    }).join('\t')).join('\n');
  }

  /**
   * TSV を 2 次元の文字列にする（引用符で囲んだセルの中のタブ・改行・"" を扱う）。末尾の空行は無視する。
   * @param {string} text
   * @returns {string[][]}
   */
  static parseTsv(text) {
    const source = String(text ?? '').replace(/\r\n?/g, '\n');
    const rows = [];
    let row = [];
    let cell = '';
    let inQuotes = false;
    let cellStart = true;
    for (let i = 0; i < source.length; i++) {
      const char = source[i];
      if (inQuotes) {
        if (char === '"') {
          if (source[i + 1] === '"') { cell += '"'; i++; } else inQuotes = false;
        } else cell += char;
        continue;
      }
      if (char === '"' && cellStart) { inQuotes = true; cellStart = false; continue; }
      if (char === '\t') { row.push(cell); cell = ''; cellStart = true; continue; }
      if (char === '\n') { row.push(cell); rows.push(row); row = []; cell = ''; cellStart = true; continue; }
      cell += char;
      cellStart = false;
    }
    if (cell !== '' || row.length) { row.push(cell); rows.push(row); }
    return rows;
  }
}
