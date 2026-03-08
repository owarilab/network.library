/**
 * TilesetData
 * タイルセット全体のメタ情報とチップアクセスを担う中核クラス。
 * 各チップが独立した LayerData を持ち、チップごとに自由なレイヤー構成が可能。
 *
 * chips[row][col] = LayerData
 */
class TilesetData {
  /**
   * @param {number} chipW     1チップの幅 (px)
   * @param {number} chipH     1チップの高さ (px)
   * @param {number} cols      横方向のチップ数
   * @param {number} rows      縦方向のチップ数
   * @param {number} [fillColor=0x00000000] 初期塗りつぶし色
   */
  constructor(chipW, chipH, cols, rows, fillColor = 0x00000000) {
    /** @type {number} */
    this.chipWidth = chipW;
    /** @type {number} */
    this.chipHeight = chipH;
    /** @type {number} */
    this.columns = cols;
    /** @type {number} */
    this.rows = rows;

    /**
     * chips[row][col] = LayerData（チップごとのレイヤー管理）
     * @type {LayerData[][]}
     */
    this.chips = [];

    for (let r = 0; r < rows; r++) {
      this.chips[r] = [];
      for (let c = 0; c < cols; c++) {
        const ld = new LayerData();
        ld.init(chipW, chipH, fillColor);
        this.chips[r][c] = ld;
      }
    }
  }

  // ----------------------------------------------------------------
  // チップの LayerData アクセス
  // ----------------------------------------------------------------

  /**
   * 指定チップの LayerData を返す。
   * @param {number} col 列番号 (0始まり)
   * @param {number} row 行番号 (0始まり)
   * @returns {LayerData|null}
   */
  getChipLayerData(col, row) {
    if (row < 0 || row >= this.rows || col < 0 || col >= this.columns) return null;
    return this.chips[row][col];
  }

  // ----------------------------------------------------------------
  // タイルセット全体画像の組み立て
  // ----------------------------------------------------------------

  /**
   * 全チップの composite() を1枚の PixelData に配置して返す。
   * チップパレット表示やPNGエクスポートで使用する。
   * @returns {PixelData}
   */
  compositeAll() {
    const fullPd = new PixelData();
    fullPd.createPixelData(this.chipWidth * this.columns, this.chipHeight * this.rows);
    for (let r = 0; r < this.rows; r++) {
      for (let c = 0; c < this.columns; c++) {
        const chipPd = this.chips[r][c].composite();
        const ox = c * this.chipWidth;
        const oy = r * this.chipHeight;
        for (let y = 0; y < this.chipHeight; y++) {
          for (let x = 0; x < this.chipWidth; x++) {
            fullPd.setPixel(ox + x, oy + y, chipPd.getPixel(x, y));
          }
        }
      }
    }
    return fullPd;
  }

  /**
   * 指定チップの composite() を返す（単純委譲）。
   * @param {number} col
   * @param {number} row
   * @returns {PixelData|null}
   */
  compositeChip(col, row) {
    const ld = this.getChipLayerData(col, row);
    return ld ? ld.composite() : null;
  }

  // ----------------------------------------------------------------
  // チップ単位の操作
  // ----------------------------------------------------------------

  /**
   * チップの全レイヤーをクリアする。
   * @param {number} col
   * @param {number} row
   */
  clearChip(col, row) {
    const ld = this.getChipLayerData(col, row);
    if (!ld) return;
    for (const layer of ld.layers) {
      layer.pixelData.fill(0x00000000);
    }
    ld.markCompositeDirty();
  }

  /**
   * チップ間コピー（LayerData のディープコピー）。
   * @param {number} srcCol
   * @param {number} srcRow
   * @param {number} dstCol
   * @param {number} dstRow
   */
  copyChip(srcCol, srcRow, dstCol, dstRow) {
    const src = this.getChipLayerData(srcCol, srcRow);
    const dst = this.getChipLayerData(dstCol, dstRow);
    if (!src || !dst) return;

    dst.layers = src.layers.map(layer => {
      const pd = new PixelData();
      pd.createPixelData(this.chipWidth, this.chipHeight);
      pd.pixels.set(layer.pixelData.pixels);
      return {
        pixelData: pd,
        name:      layer.name,
        visible:   layer.visible,
        opacity:   layer.opacity,
      };
    });
    dst.activeIndex = src.activeIndex;
    dst.markCompositeDirty();
  }

  /**
   * 2つのチップの LayerData 参照を入れ替える。
   * @param {number} col1
   * @param {number} row1
   * @param {number} col2
   * @param {number} row2
   */
  swapChips(col1, row1, col2, row2) {
    if (row1 < 0 || row1 >= this.rows || col1 < 0 || col1 >= this.columns) return;
    if (row2 < 0 || row2 >= this.rows || col2 < 0 || col2 >= this.columns) return;
    const tmp = this.chips[row1][col1];
    this.chips[row1][col1] = this.chips[row2][col2];
    this.chips[row2][col2] = tmp;
  }

  // ----------------------------------------------------------------
  // タイルセット構造変更
  // ----------------------------------------------------------------

  /** 右端に1列追加する。 */
  addColumn() {
    this.columns++;
    for (let r = 0; r < this.rows; r++) {
      const ld = new LayerData();
      ld.init(this.chipWidth, this.chipHeight, 0x00000000);
      this.chips[r].push(ld);
    }
  }

  /** 下端に1行追加する。 */
  addRow() {
    this.rows++;
    const row = [];
    for (let c = 0; c < this.columns; c++) {
      const ld = new LayerData();
      ld.init(this.chipWidth, this.chipHeight, 0x00000000);
      row.push(ld);
    }
    this.chips.push(row);
  }

  /** 右端の1列を削除する。列が1以下なら何もしない。 */
  removeColumn() {
    if (this.columns <= 1) return;
    this.columns--;
    for (let r = 0; r < this.rows; r++) {
      this.chips[r].pop();
    }
  }

  /** 下端の1行を削除する。行が1以下なら何もしない。 */
  removeRow() {
    if (this.rows <= 1) return;
    this.rows--;
    this.chips.pop();
  }

  /**
   * 列・行数を変更する（既存チップは保持）。
   * @param {number} newCols
   * @param {number} newRows
   */
  resize(newCols, newRows) {
    if (newCols < 1 || newRows < 1) return;

    // 行数の調整
    while (this.rows < newRows) this.addRow();
    while (this.rows > newRows) this.removeRow();

    // 列数の調整
    while (this.columns < newCols) this.addColumn();
    while (this.columns > newCols) this.removeColumn();
  }
}
