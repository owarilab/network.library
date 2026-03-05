/**
 * DropdownMenu
 * メニューバー項目のプルダウンリストを描画・管理するクラス。
 * MenuBar が生成・保持する。
 *
 * items フォーマット:
 *   { id: MenuConstants.FILE_NEW, label: '新規作成' }
 *   { id: MenuConstants.SEPARATOR }  // 区切り線
 */
class DropdownMenu {
  // スタイル定数
  static FONT         = '13px sans-serif';
  static ITEM_HEIGHT  = 22;
  static SEP_HEIGHT   = 9;
  static MIN_WIDTH    = 160;
  static PADDING_X    = 20;
  static COLOR_BG     = '#f0eeea';
  static COLOR_BORDER = '#888880';
  static COLOR_TEXT   = '#000000';
  static COLOR_HOVER_BG   = '#0078d7';
  static COLOR_HOVER_TEXT = '#ffffff';
  static COLOR_SEP    = '#b0aca4';
  static SHADOW_COLOR = 'rgba(0,0,0,0.25)';
  static SHADOW_BLUR  = 4;

  /**
   * @param {{ id: string, label?: string }[]} items
   */
  constructor(items) {
    /** @type {{ id: string, label?: string }[]} */
    this._items = items;

    /** ホバー中のアイテムインデックス (-1 = なし) */
    this._hoverIndex = -1;

    /** render() で算出した各行の rect */
    this._rects = [];

    /** ドロップダウン全体の幅 (render で確定) */
    this._width = DropdownMenu.MIN_WIDTH;
  }

  // ----------------------------------------------------------------
  // 描画
  // ----------------------------------------------------------------

  /**
   * @param {CanvasRenderingContext2D} ctx
   * @param {number} originX - ドロップダウン左上 X
   * @param {number} originY - ドロップダウン左上 Y
   */
  render(ctx, originX, originY) {
    const {
      FONT, ITEM_HEIGHT, SEP_HEIGHT, MIN_WIDTH, PADDING_X,
      COLOR_BG, COLOR_BORDER, COLOR_TEXT,
      COLOR_HOVER_BG, COLOR_HOVER_TEXT, COLOR_SEP,
      SHADOW_COLOR, SHADOW_BLUR,
    } = DropdownMenu;

    ctx.save();
    ctx.font = FONT;

    // ---- 幅を計算 ----
    let maxLabelW = 0;
    for (const item of this._items) {
      if (item.id === MenuConstants.SEPARATOR) continue;
      const w = ctx.measureText(item.label ?? '').width;
      if (w > maxLabelW) maxLabelW = w;
    }
    this._width = Math.max(MIN_WIDTH, maxLabelW + PADDING_X * 2 + 16);

    // ---- 各アイテムの rect を算出 ----
    this._rects = [];
    let curY = originY;
    for (const item of this._items) {
      const h = (item.id === MenuConstants.SEPARATOR) ? SEP_HEIGHT : ITEM_HEIGHT;
      this._rects.push({ x: originX, y: curY, w: this._width, h });
      curY += h;
    }
    const totalH = curY - originY;

    // ---- 影 ----
    ctx.shadowColor   = SHADOW_COLOR;
    ctx.shadowBlur    = SHADOW_BLUR;
    ctx.shadowOffsetX = 2;
    ctx.shadowOffsetY = 2;
    ctx.fillStyle = COLOR_BG;
    ctx.fillRect(originX, originY, this._width, totalH);
    ctx.shadowColor = 'transparent';
    ctx.shadowBlur  = 0;
    ctx.shadowOffsetX = 0;
    ctx.shadowOffsetY = 0;

    // ---- 枠線 ----
    ctx.strokeStyle = COLOR_BORDER;
    ctx.lineWidth   = 1;
    ctx.strokeRect(originX + 0.5, originY + 0.5, this._width - 1, totalH - 1);

    // ---- 各アイテム描画 ----
    this._items.forEach((item, i) => {
      const r = this._rects[i];

      if (item.id === MenuConstants.SEPARATOR) {
        // 区切り線
        const midY = Math.floor(r.y + r.h / 2) + 0.5;
        ctx.strokeStyle = COLOR_SEP;
        ctx.lineWidth   = 1;
        ctx.beginPath();
        ctx.moveTo(r.x + 4,         midY);
        ctx.lineTo(r.x + r.w - 4,   midY);
        ctx.stroke();
        return;
      }

      if (i === this._hoverIndex) {
        ctx.fillStyle = COLOR_HOVER_BG;
        ctx.fillRect(r.x + 1, r.y + 1, r.w - 2, r.h - 2);
        ctx.fillStyle = COLOR_HOVER_TEXT;
      } else {
        ctx.fillStyle = COLOR_TEXT;
      }

      ctx.font = FONT;
      const textY = r.y + Math.floor(ITEM_HEIGHT * 0.72);
      ctx.fillText(item.label ?? '', r.x + PADDING_X, textY);
    });

    ctx.restore();
  }

  // ----------------------------------------------------------------
  // ヒットテスト
  // ----------------------------------------------------------------

  /**
   * 座標がドロップダウン全体の領域内かどうか
   * @param {number} x
   * @param {number} y
   * @returns {boolean}
   */
  containsPoint(x, y) {
    if (this._rects.length === 0) return false;
    const first = this._rects[0];
    const last  = this._rects[this._rects.length - 1];
    const totalH = (last.y + last.h) - first.y;
    return x >= first.x && x < first.x + this._width &&
           y >= first.y && y < first.y + totalH;
  }

  /**
   * 座標がどのアイテム上にあるか返す
   * セパレーターや範囲外は -1
   * @param {number} x
   * @param {number} y
   * @returns {number} アイテムインデックス
   */
  hitTest(x, y) {
    for (let i = 0; i < this._rects.length; i++) {
      const r = this._rects[i];
      if (this._items[i].id === MenuConstants.SEPARATOR) continue;
      if (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h) {
        return i;
      }
    }
    return -1;
  }

  // ----------------------------------------------------------------
  // 入力ハンドラ
  // ----------------------------------------------------------------

  /** @param {{ x:number, y:number }} e */
  onMouseMove(e) {
    this._hoverIndex = this.hitTest(e.x, e.y);
  }

  /** ホバーリセット（非表示時に呼ぶ） */
  reset() {
    this._hoverIndex = -1;
  }

  /**
   * クリックされたアイテムの id を返す。なければ null
   * @param {{ x:number, y:number }} e
   * @returns {string|null}
   */
  pick(e) {
    const idx = this.hitTest(e.x, e.y);
    if (idx < 0) return null;
    return this._items[idx].id ?? null;
  }
}
