/**
 * GridView
 * Canvas 上の表（スプレッドシート風）。見える範囲の行と列だけを描く（仮想化）。
 *
 * モデル（setModel で渡す）:
 *   {
 *     columns: [{ id, title, badge?, width, align? }],
 *     frozenColumns: number,            // 左端に固定する列数
 *     rowCount: number,                 // 表示する行数（フィルタ後）
 *     getCell(row, col): { text, kind?: 'text'|'checkbox', checked?, error?, muted?, readOnly? },
 *     getRowLabel(row): string,
 *   }
 *
 * 座標はすべて canvas 座標。コールバック（onXxx）はシーンが代入する。
 */
class GridView {
  static ROW_HEIGHT = 28;
  static HEADER_HEIGHT = 44;
  static ROW_HEADER_WIDTH = 52;
  static MIN_COLUMN_WIDTH = 56;
  static SCROLLBAR = 10;
  static RESIZE_GRIP = 4;
  static DOUBLE_CLICK_MS = 350;
  static CELL_PADDING = 8;

  constructor() {
    this.model = { columns: [], frozenColumns: 0, rowCount: 0, getCell: () => ({ text: '' }), getRowLabel: () => '' };
    this.rect = { x: 0, y: 0, w: 0, h: 0 };
    this.scrollX = 0;
    this.scrollY = 0;
    /** 選択範囲。anchor が起点、focus が現在のセル */
    this.selection = { anchorRow: 0, anchorCol: 0, focusRow: 0, focusCol: 0 };

    this.onSelectionChange = null;
    this.onEditRequest = null;   // (row, col, initialText|null)
    this.onToggle = null;        // (row, col)
    this.onClear = null;         // ()
    this.onColumnResize = null;  // (col, width)
    this.onHeaderClick = null;   // (col, mouseEvent)
    this.onRowHeaderClick = null; // (row, mouseEvent)
    this.onContextMenu = null;   // (hit, x, y)
    this.onMoveRows = null;      // (delta)
    this.onMoveColumns = null;   // (delta)

    this._drag = null; // { type: 'select'|'resize'|'vbar'|'hbar', ... }
    this._lastClick = { time: 0, row: -1, col: -1 };
    this._hover = null;
    this._metricsCache = null;
  }

  // ---------------------------------------------------------------------------
  // モデルと配置
  // ---------------------------------------------------------------------------

  setModel(model) {
    this.model = model;
    this._metricsCache = null;
    this._clampSelection();
    this._clampScroll();
  }

  setRect(rect) {
    this.rect = { x: rect.x, y: rect.y, w: Math.max(0, rect.w), h: Math.max(0, rect.h) };
    this._metricsCache = null;
    this._clampScroll();
  }

  /** 列幅や行数が変わったとき、レイアウトを計算し直す。 */
  invalidate() {
    this._metricsCache = null;
    this._clampSelection();
    this._clampScroll();
  }

  _metrics() {
    if (this._metricsCache) return this._metricsCache;
    const columns = this.model.columns;
    const offsets = [];
    let total = 0;
    for (const column of columns) {
      offsets.push(total);
      total += Math.max(GridView.MIN_COLUMN_WIDTH, column.width || 120);
    }
    const frozenCount = Math.min(this.model.frozenColumns || 0, columns.length);
    // 固定列の合計幅（offsets[n] は n 番目の列の左端 = 手前の列の幅の合計。全列が固定なら total）
    const frozenW = offsets[frozenCount] ?? total;
    const totalH = this.model.rowCount * GridView.ROW_HEIGHT;
    const rowHeaderW = GridView.ROW_HEADER_WIDTH;
    const headerH = GridView.HEADER_HEIGHT;

    let needV = false;
    let needH = false;
    for (let pass = 0; pass < 2; pass++) {
      needV = totalH > this.rect.h - headerH - (needH ? GridView.SCROLLBAR : 0);
      needH = total > this.rect.w - rowHeaderW - (needV ? GridView.SCROLLBAR : 0);
    }
    const viewW = Math.max(0, this.rect.w - rowHeaderW - (needV ? GridView.SCROLLBAR : 0));
    const viewH = Math.max(0, this.rect.h - headerH - (needH ? GridView.SCROLLBAR : 0));
    this._metricsCache = {
      offsets, total, totalH, frozenCount, frozenW, rowHeaderW, headerH, needV, needH, viewW, viewH,
      bodyX: this.rect.x + rowHeaderW,
      bodyY: this.rect.y + headerH,
      maxScrollX: Math.max(0, total - viewW),
      maxScrollY: Math.max(0, totalH - viewH),
    };
    return this._metricsCache;
  }

  getColumnWidth(col) {
    return Math.max(GridView.MIN_COLUMN_WIDTH, this.model.columns[col]?.width || 120);
  }

  _colScreenX(col) {
    const m = this._metrics();
    if (col < m.frozenCount) return m.bodyX + m.offsets[col];
    return m.bodyX + m.offsets[col] - this.scrollX;
  }

  _rowScreenY(row) {
    return this._metrics().bodyY + row * GridView.ROW_HEIGHT - this.scrollY;
  }

  /**
   * セルの canvas 上の矩形（見えていなくても計算する）。
   * @returns {{x:number,y:number,w:number,h:number}}
   */
  getCellRect(row, col) {
    return { x: this._colScreenX(col), y: this._rowScreenY(row), w: this.getColumnWidth(col), h: GridView.ROW_HEIGHT };
  }

  /** セルがスクロール領域の中で完全に見えているか。 */
  isCellVisible(row, col) {
    const m = this._metrics();
    const rect = this.getCellRect(row, col);
    const left = col < m.frozenCount ? m.bodyX : m.bodyX + m.frozenW;
    return rect.x >= left - 0.5 && rect.x + rect.w <= m.bodyX + m.viewW + 0.5
      && rect.y >= m.bodyY - 0.5 && rect.y + rect.h <= m.bodyY + m.viewH + 0.5;
  }

  _clampScroll() {
    const m = this._metrics();
    this.scrollX = Math.min(Math.max(0, this.scrollX), m.maxScrollX);
    this.scrollY = Math.min(Math.max(0, this.scrollY), m.maxScrollY);
  }

  /** 指定セルが見えるようにスクロールする。 */
  ensureVisible(row, col) {
    const m = this._metrics();
    if (row >= 0 && row < this.model.rowCount) {
      const top = row * GridView.ROW_HEIGHT;
      if (top < this.scrollY) this.scrollY = top;
      else if (top + GridView.ROW_HEIGHT > this.scrollY + m.viewH) this.scrollY = top + GridView.ROW_HEIGHT - m.viewH;
    }
    if (col >= m.frozenCount && col < this.model.columns.length) {
      const left = m.offsets[col];
      const width = this.getColumnWidth(col);
      const visibleLeft = this.scrollX + m.frozenW;
      if (left < visibleLeft) this.scrollX = left - m.frozenW;
      else if (left + width > this.scrollX + m.viewW) this.scrollX = left + width - m.viewW;
    }
    this._clampScroll();
  }

  // ---------------------------------------------------------------------------
  // 選択
  // ---------------------------------------------------------------------------

  /** 選択範囲（正規化済み）。 */
  getSelection() {
    const s = this.selection;
    return {
      top: Math.min(s.anchorRow, s.focusRow),
      bottom: Math.max(s.anchorRow, s.focusRow),
      left: Math.min(s.anchorCol, s.focusCol),
      right: Math.max(s.anchorCol, s.focusCol),
      focusRow: s.focusRow,
      focusCol: s.focusCol,
    };
  }

  select(row, col, extend = false) {
    const rowCount = this.model.rowCount;
    const colCount = this.model.columns.length;
    if (rowCount === 0 || colCount === 0) return;
    const nextRow = Math.min(Math.max(0, row), rowCount - 1);
    const nextCol = Math.min(Math.max(0, col), colCount - 1);
    this.selection.focusRow = nextRow;
    this.selection.focusCol = nextCol;
    if (!extend) {
      this.selection.anchorRow = nextRow;
      this.selection.anchorCol = nextCol;
    }
    this.ensureVisible(nextRow, nextCol);
    this.onSelectionChange?.();
  }

  selectRange(top, left, bottom, right) {
    if (this.model.rowCount === 0 || this.model.columns.length === 0) return;
    this.selection = {
      anchorRow: Math.max(0, top),
      anchorCol: Math.max(0, left),
      focusRow: Math.min(this.model.rowCount - 1, bottom),
      focusCol: Math.min(this.model.columns.length - 1, right),
    };
    this.onSelectionChange?.();
  }

  selectAll() {
    this.selectRange(0, 0, this.model.rowCount - 1, this.model.columns.length - 1);
  }

  _clampSelection() {
    const maxRow = Math.max(0, this.model.rowCount - 1);
    const maxCol = Math.max(0, this.model.columns.length - 1);
    const s = this.selection;
    s.anchorRow = Math.min(s.anchorRow, maxRow);
    s.focusRow = Math.min(s.focusRow, maxRow);
    s.anchorCol = Math.min(s.anchorCol, maxCol);
    s.focusCol = Math.min(s.focusCol, maxCol);
  }

  // ---------------------------------------------------------------------------
  // ヒットテスト
  // ---------------------------------------------------------------------------

  /**
   * canvas 座標が指すものを返す。
   * @returns {{type:'cell',row:number,col:number}|{type:'colHeader',col:number,resize:boolean}|{type:'rowHeader',row:number}|{type:'corner'}|{type:'vbar'}|{type:'hbar'}|null}
   */
  hitTest(x, y) {
    if (!GridTheme.inRect(this.rect, x, y)) return null;
    const m = this._metrics();
    if (m.needV && x >= this.rect.x + this.rect.w - GridView.SCROLLBAR && y >= m.bodyY) return { type: 'vbar' };
    if (m.needH && y >= this.rect.y + this.rect.h - GridView.SCROLLBAR && x >= m.bodyX) return { type: 'hbar' };

    const inHeader = y < m.bodyY;
    const inRowHeader = x < m.bodyX;
    if (inHeader && inRowHeader) return { type: 'corner' };
    if (inRowHeader) {
      const row = Math.floor((y - m.bodyY + this.scrollY) / GridView.ROW_HEIGHT);
      return row >= 0 && row < this.model.rowCount ? { type: 'rowHeader', row } : null;
    }

    const col = this._columnAt(x, m);
    if (inHeader) {
      if (col < 0) return null;
      const right = this._colScreenX(col) + this.getColumnWidth(col);
      return { type: 'colHeader', col, resize: Math.abs(x - right) <= GridView.RESIZE_GRIP };
    }
    const row = Math.floor((y - m.bodyY + this.scrollY) / GridView.ROW_HEIGHT);
    if (col < 0 || row < 0 || row >= this.model.rowCount) return null;
    return { type: 'cell', row, col };
  }

  _columnAt(x, m) {
    const count = this.model.columns.length;
    // 固定列は常にスクロール領域の上に描かれるので先に調べる
    for (let col = 0; col < m.frozenCount; col++) {
      const left = this._colScreenX(col);
      if (x >= left && x < left + this.getColumnWidth(col)) return col;
    }
    if (x >= m.bodyX + m.viewW) return -1;
    for (let col = m.frozenCount; col < count; col++) {
      const left = this._colScreenX(col);
      if (left > m.bodyX + m.viewW) break;
      if (x >= Math.max(left, m.bodyX + m.frozenW) && x < left + this.getColumnWidth(col)) return col;
    }
    return -1;
  }

  // ---------------------------------------------------------------------------
  // マウス
  // ---------------------------------------------------------------------------

  /** @returns {boolean} 表が処理したか */
  onMouseDown(x, y, event) {
    const hit = this.hitTest(x, y);
    if (!hit) return false;
    const m = this._metrics();
    const shift = !!event?.raw?.shiftKey;

    if (hit.type === 'vbar' || hit.type === 'hbar') {
      this._drag = { type: hit.type, startX: x, startY: y, startScrollX: this.scrollX, startScrollY: this.scrollY };
      return true;
    }
    if (hit.type === 'corner') {
      this.selectAll();
      return true;
    }
    if (hit.type === 'colHeader') {
      if (hit.resize) {
        this._drag = { type: 'resize', col: hit.col, startX: x, startWidth: this.getColumnWidth(hit.col) };
        return true;
      }
      if (this.model.rowCount > 0) {
        const anchorCol = shift ? this.selection.anchorCol : hit.col;
        this.selectRange(0, anchorCol, this.model.rowCount - 1, hit.col);
        this.selection.anchorRow = 0;
        this.selection.focusRow = this.model.rowCount - 1;
      }
      this.onHeaderClick?.(hit.col, event);
      return true;
    }
    if (hit.type === 'rowHeader') {
      const anchorRow = shift ? this.selection.anchorRow : hit.row;
      this.selectRange(anchorRow, 0, hit.row, this.model.columns.length - 1);
      this._drag = { type: 'rowSelect', anchorRow };
      this.onRowHeaderClick?.(hit.row, event);
      return true;
    }

    // セル
    const cell = this.model.getCell(hit.row, hit.col);
    if (cell?.kind === 'checkbox' && !cell.readOnly) {
      const rect = this.getCellRect(hit.row, hit.col);
      const box = this._checkboxRect(rect);
      if (GridTheme.inRect(box, x, y)) {
        this.select(hit.row, hit.col);
        this.onToggle?.(hit.row, hit.col);
        return true;
      }
    }
    const now = performance.now();
    const isDouble = now - this._lastClick.time < GridView.DOUBLE_CLICK_MS
      && this._lastClick.row === hit.row && this._lastClick.col === hit.col;
    this._lastClick = { time: now, row: hit.row, col: hit.col };

    this.select(hit.row, hit.col, shift);
    if (isDouble) {
      this._lastClick = { time: 0, row: -1, col: -1 };
      this.onEditRequest?.(hit.row, hit.col, null);
      return true;
    }
    this._drag = { type: 'select' };
    void m;
    return true;
  }

  onMouseMove(x, y) {
    if (this._drag) {
      const m = this._metrics();
      const drag = this._drag;
      if (drag.type === 'resize') {
        const width = Math.max(GridView.MIN_COLUMN_WIDTH, drag.startWidth + x - drag.startX);
        this.onColumnResize?.(drag.col, width);
        this.invalidate();
      } else if (drag.type === 'vbar') {
        const track = Math.max(1, m.viewH - this._thumbSize(m.viewH, m.totalH, m.viewH));
        this.scrollY = drag.startScrollY + (y - drag.startY) * (m.maxScrollY / track);
        this._clampScroll();
      } else if (drag.type === 'hbar') {
        const track = Math.max(1, m.viewW - this._thumbSize(m.viewW, m.total, m.viewW));
        this.scrollX = drag.startScrollX + (x - drag.startX) * (m.maxScrollX / track);
        this._clampScroll();
      } else if (drag.type === 'select') {
        // 範囲外へドラッグしたら端のセルまでを選ぶ。端に近いときは自動でスクロールする
        const col = this._columnAtClamped(x, m);
        const row = Math.floor((Math.min(Math.max(y, m.bodyY), m.bodyY + m.viewH - 1) - m.bodyY + this.scrollY) / GridView.ROW_HEIGHT);
        if (y > m.bodyY + m.viewH) this.scrollY = Math.min(m.maxScrollY, this.scrollY + GridView.ROW_HEIGHT / 2);
        else if (y < m.bodyY) this.scrollY = Math.max(0, this.scrollY - GridView.ROW_HEIGHT / 2);
        this.select(Math.min(Math.max(row, 0), this.model.rowCount - 1), col, true);
      } else if (drag.type === 'rowSelect') {
        const row = Math.floor((Math.min(Math.max(y, m.bodyY), m.bodyY + m.viewH - 1) - m.bodyY + this.scrollY) / GridView.ROW_HEIGHT);
        this.selectRange(drag.anchorRow, 0, Math.min(Math.max(row, 0), this.model.rowCount - 1), this.model.columns.length - 1);
        this.selection.anchorRow = drag.anchorRow;
      }
      return true;
    }
    this._hover = this.hitTest(x, y);
    return false;
  }

  _columnAtClamped(x, m) {
    const found = this._columnAt(x, m);
    if (found >= 0) return found;
    return x < m.bodyX ? 0 : this.model.columns.length - 1;
  }

  onMouseUp() {
    const had = !!this._drag;
    this._drag = null;
    return had;
  }

  /** @returns {'col-resize'|'default'} */
  getCursor(x, y) {
    if (this._drag?.type === 'resize') return 'col-resize';
    const hit = this.hitTest(x, y);
    return hit?.type === 'colHeader' && hit.resize ? 'col-resize' : 'default';
  }

  onContextMenuAt(x, y) {
    const hit = this.hitTest(x, y);
    if (!hit) return false;
    if (hit.type === 'cell') {
      const s = this.getSelection();
      const inside = hit.row >= s.top && hit.row <= s.bottom && hit.col >= s.left && hit.col <= s.right;
      if (!inside) this.select(hit.row, hit.col);
    } else if (hit.type === 'rowHeader') {
      this.selectRange(hit.row, 0, hit.row, this.model.columns.length - 1);
    } else if (hit.type === 'colHeader') {
      this.selectRange(0, hit.col, this.model.rowCount - 1, hit.col);
    }
    this.onContextMenu?.(hit, x, y);
    return true;
  }

  onWheel(x, y, deltaX, deltaY, shift = false) {
    if (!GridTheme.inRect(this.rect, x, y)) return false;
    const m = this._metrics();
    if (shift || (Math.abs(deltaX) > Math.abs(deltaY))) {
      const amount = shift && deltaX === 0 ? deltaY : deltaX;
      this.scrollX = Math.min(m.maxScrollX, Math.max(0, this.scrollX + amount));
    } else {
      this.scrollY = Math.min(m.maxScrollY, Math.max(0, this.scrollY + deltaY));
    }
    return true;
  }

  // ---------------------------------------------------------------------------
  // キーボード
  // ---------------------------------------------------------------------------

  /**
   * 移動・編集開始のキーを処理する。Ctrl の組み合わせ（コピーなど）はシーンが処理する。
   * @returns {boolean} 処理したか
   */
  onKeyDown(e) {
    if (this.model.rowCount === 0 || this.model.columns.length === 0) return false;
    const s = this.selection;
    const shift = !!e.shift;
    const page = Math.max(1, Math.floor(this._metrics().viewH / GridView.ROW_HEIGHT) - 1);
    const lastRow = this.model.rowCount - 1;
    const lastCol = this.model.columns.length - 1;

    if (e.ctrl || e.meta) {
      if (e.code === 'KeyA') { this.selectAll(); return true; }
      if (e.code === 'ArrowUp') { this.select(0, s.focusCol, shift); return true; }
      if (e.code === 'ArrowDown') { this.select(lastRow, s.focusCol, shift); return true; }
      if (e.code === 'ArrowLeft') { this.select(s.focusRow, 0, shift); return true; }
      if (e.code === 'ArrowRight') { this.select(s.focusRow, lastCol, shift); return true; }
      return false;
    }
    if (e.alt) {
      if (e.code === 'ArrowUp') { this.onMoveRows?.(-1); return true; }
      if (e.code === 'ArrowDown') { this.onMoveRows?.(1); return true; }
      if (e.code === 'ArrowLeft') { this.onMoveColumns?.(-1); return true; }
      if (e.code === 'ArrowRight') { this.onMoveColumns?.(1); return true; }
      return false;
    }

    switch (e.code) {
      case 'ArrowUp': this.select(s.focusRow - 1, s.focusCol, shift); return true;
      case 'ArrowDown': this.select(s.focusRow + 1, s.focusCol, shift); return true;
      case 'ArrowLeft': this.select(s.focusRow, s.focusCol - 1, shift); return true;
      case 'ArrowRight': this.select(s.focusRow, s.focusCol + 1, shift); return true;
      case 'Home': this.select(s.focusRow, 0, shift); return true;
      case 'End': this.select(s.focusRow, lastCol, shift); return true;
      case 'PageUp': this.select(s.focusRow - page, s.focusCol, shift); return true;
      case 'PageDown': this.select(s.focusRow + page, s.focusCol, shift); return true;
      case 'Tab': {
        if (shift) {
          if (s.focusCol > 0) this.select(s.focusRow, s.focusCol - 1);
          else if (s.focusRow > 0) this.select(s.focusRow - 1, lastCol);
        } else if (s.focusCol < lastCol) this.select(s.focusRow, s.focusCol + 1);
        else if (s.focusRow < lastRow) this.select(s.focusRow + 1, 0);
        return true;
      }
      case 'Enter':
      case 'NumpadEnter':
      case 'F2':
        this.onEditRequest?.(s.focusRow, s.focusCol, null);
        return true;
      case 'Delete':
      case 'Backspace':
        this.onClear?.();
        return true;
      case 'Space': {
        const cell = this.model.getCell(s.focusRow, s.focusCol);
        if (cell?.kind === 'checkbox' && !cell.readOnly) {
          this.onToggle?.(s.focusRow, s.focusCol);
          return true;
        }
        break;
      }
      default:
        break;
    }
    // 文字キーで編集を始める（その文字で置き換える）。IME の最初の 1 打は取れないので Enter / F2 で始める
    if (e.key && e.key.length === 1 && !e.ctrl && !e.meta && !e.alt) {
      this.onEditRequest?.(s.focusRow, s.focusCol, e.key);
      return true;
    }
    return false;
  }

  // ---------------------------------------------------------------------------
  // 描画
  // ---------------------------------------------------------------------------

  _checkboxRect(cellRect) {
    const size = 14;
    return { x: cellRect.x + GridView.CELL_PADDING, y: cellRect.y + (cellRect.h - size) / 2, w: size, h: size };
  }

  _thumbSize(trackLength, contentLength, viewLength) {
    if (contentLength <= viewLength) return trackLength;
    return Math.max(24, trackLength * (viewLength / contentLength));
  }

  render(ctx) {
    const colors = GridTheme.COLORS;
    const m = this._metrics();
    const { x, y, w, h } = this.rect;
    if (w <= 0 || h <= 0) return;

    ctx.save();
    ctx.beginPath();
    ctx.rect(x, y, w, h);
    ctx.clip();
    ctx.fillStyle = colors.bg;
    ctx.fillRect(x, y, w, h);

    const firstRow = Math.max(0, Math.floor(this.scrollY / GridView.ROW_HEIGHT));
    const lastRow = Math.min(this.model.rowCount - 1, Math.floor((this.scrollY + m.viewH) / GridView.ROW_HEIGHT));
    const columnsToDraw = this._visibleColumns(m);
    const sel = this.getSelection();

    // 本体: スクロール列 → 固定列の順（固定列がスクロール列の上に重なる）
    ctx.save();
    ctx.beginPath();
    ctx.rect(m.bodyX, m.bodyY, m.viewW, m.viewH);
    ctx.clip();
    for (const scrollable of [true, false]) {
      ctx.save();
      if (scrollable) {
        ctx.beginPath();
        ctx.rect(m.bodyX + m.frozenW, m.bodyY, Math.max(0, m.viewW - m.frozenW), m.viewH);
        ctx.clip();
      }
      for (const col of columnsToDraw) {
        if ((col >= m.frozenCount) !== scrollable) continue;
        const cx = this._colScreenX(col);
        const cw = this.getColumnWidth(col);
        for (let row = firstRow; row <= lastRow; row++) {
          this._drawCell(ctx, row, col, cx, this._rowScreenY(row), cw, sel);
        }
        if (!scrollable) {
          ctx.strokeStyle = colors.border;
          ctx.lineWidth = 1;
          if (col === m.frozenCount - 1) {
            ctx.beginPath();
            ctx.moveTo(cx + cw - 0.5, m.bodyY);
            ctx.lineTo(cx + cw - 0.5, m.bodyY + m.viewH);
            ctx.stroke();
          }
        }
      }
      ctx.restore();
    }
    this._drawSelectionOutline(ctx, sel, m);
    ctx.restore();

    this._drawHeaders(ctx, m, firstRow, lastRow, columnsToDraw, sel);
    this._drawScrollbars(ctx, m);
    ctx.restore();
  }

  _visibleColumns(m) {
    const result = [];
    const count = this.model.columns.length;
    for (let col = 0; col < count; col++) {
      if (col < m.frozenCount) { result.push(col); continue; }
      const left = this._colScreenX(col);
      if (left > m.bodyX + m.viewW) break;
      if (left + this.getColumnWidth(col) >= m.bodyX + m.frozenW) result.push(col);
    }
    return result;
  }

  _drawCell(ctx, row, col, x, y, w, sel) {
    const colors = GridTheme.COLORS;
    const cell = this.model.getCell(row, col) || { text: '' };
    ctx.fillStyle = row % 2 ? colors.rowAlt : colors.bg;
    ctx.fillRect(x, y, w, GridView.ROW_HEIGHT);
    if (cell.readOnly) {
      ctx.fillStyle = colors.readOnlyFill;
      ctx.fillRect(x, y, w, GridView.ROW_HEIGHT);
    }
    const inSelection = row >= sel.top && row <= sel.bottom && col >= sel.left && col <= sel.right;
    if (inSelection) {
      ctx.fillStyle = colors.selectionFill;
      ctx.fillRect(x, y, w, GridView.ROW_HEIGHT);
    }
    if (cell.error) {
      ctx.fillStyle = colors.errorFill;
      ctx.fillRect(x, y, w, GridView.ROW_HEIGHT);
    }
    ctx.strokeStyle = colors.border;
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(x, y + GridView.ROW_HEIGHT - 0.5);
    ctx.lineTo(x + w, y + GridView.ROW_HEIGHT - 0.5);
    ctx.moveTo(x + w - 0.5, y);
    ctx.lineTo(x + w - 0.5, y + GridView.ROW_HEIGHT);
    ctx.stroke();

    const pad = GridView.CELL_PADDING;
    ctx.textBaseline = 'middle';
    if (cell.kind === 'checkbox') {
      const box = this._checkboxRect({ x, y, w, h: GridView.ROW_HEIGHT });
      ctx.strokeStyle = cell.readOnly ? colors.muted : colors.accent;
      ctx.lineWidth = 1.5;
      ctx.strokeRect(box.x + 0.5, box.y + 0.5, box.w - 1, box.h - 1);
      if (cell.checked) {
        ctx.fillStyle = cell.readOnly ? colors.muted : colors.accent;
        ctx.fillRect(box.x + 3, box.y + 3, box.w - 6, box.h - 6);
      }
      return;
    }
    ctx.font = cell.mono ? GridTheme.FONT_MONO : GridTheme.FONT;
    ctx.fillStyle = cell.error ? '#fecaca' : cell.muted || cell.readOnly ? colors.muted : colors.text;
    const align = this.model.columns[col]?.align || cell.align || 'left';
    ctx.textAlign = align;
    const text = GridTheme.fitText(ctx, cell.text, w - pad * 2);
    ctx.fillText(text, align === 'right' ? x + w - pad : x + pad, y + GridView.ROW_HEIGHT / 2 + 0.5);
  }

  _drawSelectionOutline(ctx, sel, m) {
    const colors = GridTheme.COLORS;
    if (this.model.rowCount === 0 || this.model.columns.length === 0) return;
    const topLeft = this.getCellRect(sel.top, sel.left);
    const bottomRight = this.getCellRect(sel.bottom, sel.right);
    ctx.strokeStyle = colors.selectionBorder;
    ctx.lineWidth = 1.5;
    ctx.strokeRect(topLeft.x + 0.5, topLeft.y + 0.5, bottomRight.x + bottomRight.w - topLeft.x - 1, bottomRight.y + bottomRight.h - topLeft.y - 1);
    const focus = this.getCellRect(sel.focusRow, sel.focusCol);
    ctx.strokeStyle = colors.focusBorder;
    ctx.lineWidth = 2;
    ctx.strokeRect(focus.x + 1, focus.y + 1, focus.w - 2, focus.h - 2);
    void m;
  }

  _drawHeaders(ctx, m, firstRow, lastRow, columnsToDraw, sel) {
    const colors = GridTheme.COLORS;
    const { x, y, w } = this.rect;

    // 列ヘッダ（固定列は上に重ねる）
    for (const scrollable of [true, false]) {
      ctx.save();
      ctx.beginPath();
      if (scrollable) ctx.rect(m.bodyX + m.frozenW, y, Math.max(0, m.viewW - m.frozenW), m.headerH);
      else ctx.rect(m.bodyX, y, m.frozenW, m.headerH);
      ctx.clip();
      for (const col of columnsToDraw) {
        if ((col >= m.frozenCount) !== scrollable) continue;
        const column = this.model.columns[col];
        const cx = this._colScreenX(col);
        const cw = this.getColumnWidth(col);
        const selected = col >= sel.left && col <= sel.right && sel.top === 0 && sel.bottom === this.model.rowCount - 1;
        ctx.fillStyle = selected ? colors.headerHover : colors.header;
        ctx.fillRect(cx, y, cw, m.headerH);
        ctx.strokeStyle = colors.border;
        ctx.lineWidth = 1;
        ctx.strokeRect(cx + 0.5, y + 0.5, cw - 1, m.headerH - 1);
        ctx.textBaseline = 'middle';
        ctx.textAlign = 'left';
        ctx.font = GridTheme.FONT_BOLD;
        ctx.fillStyle = colors.headerText;
        ctx.fillText(GridTheme.fitText(ctx, column.title, cw - 14), cx + 8, y + 15);
        if (column.badge) {
          ctx.font = GridTheme.FONT_SMALL;
          ctx.fillStyle = column.badgeColor || colors.muted;
          ctx.fillText(GridTheme.fitText(ctx, column.badge, cw - 14), cx + 8, y + 32);
        }
      }
      ctx.restore();
    }

    // 行ヘッダ（行番号）
    ctx.save();
    ctx.beginPath();
    ctx.rect(x, m.bodyY, m.rowHeaderW, m.viewH);
    ctx.clip();
    for (let row = firstRow; row <= lastRow; row++) {
      const ry = this._rowScreenY(row);
      const selected = row >= sel.top && row <= sel.bottom && sel.left === 0 && sel.right === this.model.columns.length - 1;
      ctx.fillStyle = selected ? colors.headerHover : colors.header;
      ctx.fillRect(x, ry, m.rowHeaderW, GridView.ROW_HEIGHT);
      ctx.strokeStyle = colors.border;
      ctx.strokeRect(x + 0.5, ry + 0.5, m.rowHeaderW - 1, GridView.ROW_HEIGHT - 1);
      ctx.font = GridTheme.FONT_SMALL;
      ctx.fillStyle = colors.muted;
      ctx.textAlign = 'center';
      ctx.textBaseline = 'middle';
      ctx.fillText(this.model.getRowLabel(row), x + m.rowHeaderW / 2, ry + GridView.ROW_HEIGHT / 2 + 0.5);
    }
    ctx.restore();

    // 左上の角
    ctx.fillStyle = colors.header;
    ctx.fillRect(x, y, m.rowHeaderW, m.headerH);
    ctx.strokeStyle = colors.border;
    ctx.strokeRect(x + 0.5, y + 0.5, m.rowHeaderW - 1, m.headerH - 1);
    void w;
  }

  _drawScrollbars(ctx, m) {
    const colors = GridTheme.COLORS;
    const sb = GridView.SCROLLBAR;
    ctx.fillStyle = 'rgba(148, 163, 184, 0.45)';
    if (m.needV) {
      const trackX = this.rect.x + this.rect.w - sb;
      const thumb = this._thumbSize(m.viewH, m.totalH, m.viewH);
      const top = m.bodyY + (m.maxScrollY ? (this.scrollY / m.maxScrollY) * (m.viewH - thumb) : 0);
      ctx.fillStyle = colors.panel;
      ctx.fillRect(trackX, m.bodyY, sb, m.viewH);
      ctx.fillStyle = 'rgba(148, 163, 184, 0.45)';
      GridTheme.roundRect(ctx, trackX + 1, top, sb - 2, thumb, 4);
      ctx.fill();
    }
    if (m.needH) {
      const trackY = this.rect.y + this.rect.h - sb;
      const thumb = this._thumbSize(m.viewW, m.total, m.viewW);
      const left = m.bodyX + (m.maxScrollX ? (this.scrollX / m.maxScrollX) * (m.viewW - thumb) : 0);
      ctx.fillStyle = colors.panel;
      ctx.fillRect(m.bodyX, trackY, m.viewW, sb);
      ctx.fillStyle = 'rgba(148, 163, 184, 0.45)';
      GridTheme.roundRect(ctx, left, trackY + 1, thumb, sb - 2, 4);
      ctx.fill();
    }
  }
}
