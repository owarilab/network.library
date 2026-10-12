/**
 * PopupList
 * canvas 上のポップアップの選択リスト（enum、ref、asset の選択に使う）。
 * 開いている間はモーダル（外をクリックすると閉じる）。文字キーで絞り込み検索ができる。
 */
class PopupList {
  static ROW_HEIGHT = 26;
  static MAX_VISIBLE = 10;
  static SEARCH_HEIGHT = 28;

  constructor() {
    this._open = false;
    this._items = [];
    this._query = '';
    this._searchable = true;
    this._anchor = { x: 0, y: 0, w: 160 };
    this._selectedValue = null;
    this._highlight = 0;
    this._scroll = 0;
    this._onPick = null;
    this._onClose = null;
    this._rect = { x: 0, y: 0, w: 0, h: 0 };
    this._canvasSize = { w: 0, h: 0 };
    this._title = '';
  }

  get isOpen() {
    return this._open;
  }

  /**
   * @param {{
   *   x: number, y: number, w?: number,
   *   items: Array<{ value: any, label: string, detail?: string }>,
   *   selectedValue?: any,
   *   searchable?: boolean,
   *   title?: string,
   *   onPick: (value: any) => void,
   *   onClose?: () => void,
   * }} options
   */
  open(options) {
    this._open = true;
    this._items = options.items || [];
    this._query = '';
    this._searchable = options.searchable !== false;
    this._anchor = { x: options.x, y: options.y, w: options.w || 180 };
    this._selectedValue = options.selectedValue;
    this._title = options.title || '';
    this._onPick = options.onPick;
    this._onClose = options.onClose || null;
    const filtered = this._filtered();
    const index = filtered.findIndex(item => item.value === this._selectedValue);
    this._highlight = Math.max(0, index);
    this._scroll = 0;
    this._ensureHighlightVisible();
  }

  close() {
    if (!this._open) return;
    this._open = false;
    this._onClose?.();
  }

  _filtered() {
    const query = this._query.trim().toLowerCase();
    if (!query) return this._items;
    return this._items.filter(item => `${item.label} ${item.detail || ''} ${item.value}`.toLowerCase().includes(query));
  }

  _layout(canvasW, canvasH) {
    const rows = Math.max(1, Math.min(PopupList.MAX_VISIBLE, this._filtered().length));
    const header = (this._searchable ? PopupList.SEARCH_HEIGHT : 0) + (this._title ? 22 : 0);
    const w = Math.max(180, this._anchor.w);
    const h = header + rows * PopupList.ROW_HEIGHT + 4;
    let x = this._anchor.x;
    let y = this._anchor.y;
    if (x + w > canvasW - 4) x = Math.max(4, canvasW - w - 4);
    if (y + h > canvasH - 4) y = Math.max(4, this._anchor.y - h - 28);
    this._rect = { x, y, w, h };
    this._canvasSize = { w: canvasW, h: canvasH };
    return { header, rows };
  }

  _ensureHighlightVisible() {
    const total = this._filtered().length;
    const rows = PopupList.MAX_VISIBLE;
    if (this._highlight < this._scroll) this._scroll = this._highlight;
    if (this._highlight >= this._scroll + rows) this._scroll = this._highlight - rows + 1;
    this._scroll = Math.max(0, Math.min(this._scroll, Math.max(0, total - rows)));
  }

  render(ctx, canvas) {
    if (!this._open) return;
    const colors = GridTheme.COLORS;
    const { header, rows } = this._layout(canvas.width, canvas.height);
    const { x, y, w, h } = this._rect;
    const filtered = this._filtered();

    ctx.save();
    ctx.fillStyle = 'rgba(0, 0, 0, 0.25)';
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    ctx.shadowColor = 'rgba(0, 0, 0, 0.5)';
    ctx.shadowBlur = 12;
    GridTheme.roundRect(ctx, x, y, w, h, 6);
    ctx.fillStyle = colors.panel;
    ctx.fill();
    ctx.shadowBlur = 0;
    ctx.strokeStyle = colors.accent;
    ctx.lineWidth = 1.5;
    ctx.stroke();

    ctx.beginPath();
    ctx.rect(x, y, w, h);
    ctx.clip();
    let cursorY = y;
    ctx.textBaseline = 'middle';
    if (this._title) {
      ctx.font = GridTheme.FONT_SMALL;
      ctx.fillStyle = colors.muted;
      ctx.textAlign = 'left';
      ctx.fillText(this._title, x + 10, cursorY + 11);
      cursorY += 22;
    }
    if (this._searchable) {
      ctx.fillStyle = colors.panelAlt;
      ctx.fillRect(x + 4, cursorY + 2, w - 8, PopupList.SEARCH_HEIGHT - 4);
      ctx.font = GridTheme.FONT;
      ctx.textAlign = 'left';
      ctx.fillStyle = this._query ? colors.text : colors.muted;
      ctx.fillText(GridTheme.fitText(ctx, this._query || 'Type to search…', w - 24), x + 10, cursorY + PopupList.SEARCH_HEIGHT / 2);
      cursorY += PopupList.SEARCH_HEIGHT;
    }
    if (!filtered.length) {
      ctx.font = GridTheme.FONT;
      ctx.fillStyle = colors.muted;
      ctx.fillText('(no match)', x + 10, cursorY + PopupList.ROW_HEIGHT / 2);
    }
    for (let i = 0; i < rows && this._scroll + i < filtered.length; i++) {
      const index = this._scroll + i;
      const item = filtered[index];
      const rowY = cursorY + i * PopupList.ROW_HEIGHT;
      if (index === this._highlight) {
        ctx.fillStyle = colors.selectionFill;
        ctx.fillRect(x + 2, rowY, w - 4, PopupList.ROW_HEIGHT);
      }
      ctx.font = item.value === this._selectedValue ? GridTheme.FONT_BOLD : GridTheme.FONT;
      ctx.fillStyle = item.value === this._selectedValue ? colors.accent : colors.text;
      ctx.textAlign = 'left';
      ctx.fillText(GridTheme.fitText(ctx, item.label, w - 24), x + 10, rowY + PopupList.ROW_HEIGHT / 2 + 0.5);
      if (item.detail) {
        ctx.font = GridTheme.FONT_SMALL;
        ctx.fillStyle = colors.muted;
        ctx.textAlign = 'right';
        ctx.fillText(GridTheme.fitText(ctx, item.detail, w * 0.4), x + w - 10, rowY + PopupList.ROW_HEIGHT / 2 + 0.5);
      }
    }
    ctx.restore();
    void header;
  }

  _rowAt(x, y) {
    if (!GridTheme.inRect(this._rect, x, y)) return -1;
    const header = (this._searchable ? PopupList.SEARCH_HEIGHT : 0) + (this._title ? 22 : 0);
    const local = y - this._rect.y - header;
    if (local < 0) return -1;
    const index = this._scroll + Math.floor(local / PopupList.ROW_HEIGHT);
    return index < this._filtered().length ? index : -1;
  }

  /** @returns {boolean} 開いている間は常に true（モーダル） */
  onMouseDown(x, y) {
    if (!this._open) return false;
    if (!GridTheme.inRect(this._rect, x, y)) {
      this.close();
      return true;
    }
    const index = this._rowAt(x, y);
    if (index >= 0) this._pick(index);
    return true;
  }

  onMouseMove(x, y) {
    if (!this._open) return false;
    const index = this._rowAt(x, y);
    if (index >= 0) this._highlight = index;
    return true;
  }

  onWheel(deltaY) {
    if (!this._open) return false;
    const total = this._filtered().length;
    this._scroll = Math.max(0, Math.min(Math.max(0, total - PopupList.MAX_VISIBLE), this._scroll + Math.sign(deltaY)));
    return true;
  }

  onKeyDown(e) {
    if (!this._open) return false;
    const filtered = this._filtered();
    if (e.code === 'Escape') this.close();
    else if (e.code === 'ArrowDown') this._highlight = Math.min(filtered.length - 1, this._highlight + 1);
    else if (e.code === 'ArrowUp') this._highlight = Math.max(0, this._highlight - 1);
    else if (e.code === 'Enter' || e.code === 'NumpadEnter') {
      if (filtered[this._highlight]) this._pick(this._highlight);
    } else if (e.code === 'Backspace') {
      this._query = this._query.slice(0, -1);
      this._highlight = 0;
    } else if (this._searchable && e.key && e.key.length === 1 && !e.ctrl && !e.meta && !e.alt) {
      this._query += e.key;
      this._highlight = 0;
      this._scroll = 0;
    }
    this._ensureHighlightVisible();
    return true;
  }

  _pick(index) {
    const item = this._filtered()[index];
    if (!item) return;
    this._open = false;
    this._onPick?.(item.value);
  }
}
