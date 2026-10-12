/**
 * SystemDatabase
 * エンジンが持つ列挙・定数の表（読み取り専用）。プロジェクトには保存しない。
 * 実行時は db.system.<table>.<row>.<column> で読める。内容の追加はアプリの更新で行う。
 */
class SystemDatabase {
  /** @type {{ version: number, tables: object[] }|null} */
  static _database = null;

  /**
   * システム DB を返す（凍結済み。変更しようとすると無視される / strict mode ではエラーになる）。
   * @returns {{ version: number, tables: object[] }}
   */
  static getDatabase() {
    if (!SystemDatabase._database) {
      SystemDatabase._database = SystemDatabase._deepFreeze(SystemDatabase._build());
    }
    return SystemDatabase._database;
  }

  static _build() {
    const label = { id: 'col_label', name: 'label', label: 'Label', type: 'string', default: '' };
    const description = { id: 'col_description', name: 'description', label: 'Description', type: 'text', default: '' };
    const integer = (name) => ({ id: `col_${name}`, name, label: name, type: 'integer', default: 0 });
    const table = (name, labelText, columns, rows) => ({
      id: `tbl_system_${name}`,
      name,
      label: labelText,
      kind: 'system',
      columns,
      rows: rows.map(([key, cells]) => ({ key, cells })),
    });

    return {
      version: 1,
      tables: [
        table('direction', '方向', [label, integer('dx'), integer('dy')], [
          ['down', { label: 'Down', dx: 0, dy: 1 }],
          ['left', { label: 'Left', dx: -1, dy: 0 }],
          ['right', { label: 'Right', dx: 1, dy: 0 }],
          ['up', { label: 'Up', dx: 0, dy: -1 }],
        ]),
        table('easing', 'イージング', [label, description], [
          ['linear', { label: 'Linear', description: '一定の速さ' }],
          ['easeIn', { label: 'Ease In', description: 'ゆっくり始まり、速く終わる' }],
          ['easeOut', { label: 'Ease Out', description: '速く始まり、ゆっくり終わる' }],
          ['easeInOut', { label: 'Ease In Out', description: '両端がゆっくり' }],
        ]),
        table('variableType', 'グローバル変数の型', [label, description], [
          ['string', { label: 'String', description: '文字列' }],
          ['number', { label: 'Number', description: '数値' }],
          ['boolean', { label: 'Boolean', description: '真偽値' }],
          ['json', { label: 'JSON', description: 'JSON 値（オブジェクト、配列）' }],
        ]),
        table('columnType', 'DB の列の型', [label, description], [
          ['string', { label: 'String', description: '1 行の文字列' }],
          ['text', { label: 'Text', description: '複数行の文字列' }],
          ['number', { label: 'Number', description: '数値（小数可）' }],
          ['integer', { label: 'Integer', description: '整数' }],
          ['boolean', { label: 'Boolean', description: '真偽値' }],
          ['enum', { label: 'Enum', description: '選択肢のいずれか' }],
          ['ref', { label: 'Ref', description: '別のテーブルの行 key' }],
          ['asset', { label: 'Asset', description: 'アセットの ID' }],
          ['json', { label: 'JSON', description: 'JSON 値' }],
        ]),
        table('assetType', 'アセットの種類', [label], [
          ['map', { label: 'Map' }],
          ['playUnit', { label: 'PlayUnit' }],
          ['pixelDocument', { label: 'Pixel Document' }],
          ['tileset', { label: 'Tileset' }],
          ['characterChipset', { label: 'Character Chipset' }],
        ]),
      ],
    };
  }

  static _deepFreeze(value) {
    if (value && typeof value === 'object' && !Object.isFrozen(value)) {
      Object.freeze(value);
      for (const child of Object.values(value)) SystemDatabase._deepFreeze(child);
    }
    return value;
  }
}
