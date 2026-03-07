/**
 * ColorPalette
 * カラーパレット UI のコンテンツクラス（基底）。
 *
 * このクラスはウィンドウ枠・背景は描画しない。
 * UIWindow のコンテンツとして使われることを前提とし、
 * 与えられた (x, y) 座標を起点にパレット内容を描画する。
 *
 * サブクラスは getColors() / getPaletteName() をオーバーライドして実装する。
 *
 * 使い方（UIWindow 経由が推奨）:
 *   const size = palette.getContentSize();    // {w, h}
 *   palette.render(ctx, x, y, appData);       // コンテンツを描画
 *   palette.onMouseMove(e);                   // ホバー更新
 *   palette.onMouseDown(e, appData);          // → true なら消費
 */
class ColorPalette {
  // ---- レイアウト定数 ----
  /** コンテンツ内余白 */
  static PADDING   = 4;
  /** 色セル 1 個のサイズ (px) */
  static CELL_SIZE = 14;
  /** セル間の隙間 (px) */
  static CELL_GAP  = 2;
  /** 1 行のセル数 */
  static COLS      = 4;
  /** FG/BG スウォッチ領域の高さ */
  static SWATCH_H  = 38;
  /** パレット名ラベルの高さ */
  static LABEL_H   = 14;

  // ---- スタイル定数 ----
  static HOVER_STROKE  = '#ffffff';
  static LABEL_FONT    = '10px sans-serif';
  static LABEL_COLOR   = '#aaaaaa';
  static TITLE_FONT    = 'bold 10px sans-serif';
  static TITLE_COLOR   = '#dddddd';

  constructor() {
    /** ホバー中のセルインデックス (-1 = なし) */
    this._hoverIndex = -1;

    /** render() 後に screen 座標で格納されるセル情報 */
    this._cells = [];

    /** FG スウォッチ矩形 (render 後に設定) @type {{x:number,y:number,w:number,h:number}|null} */
    this._fgSwatch = null;
    /** BG スウォッチ矩形 (render 後に設定) @type {{x:number,y:number,w:number,h:number}|null} */
    this._bgSwatch = null;

    /**
     * スウォッチクリック時のコールバック。
     * @type {((target: 'fore' | 'back', appData: AppData) => void)|null}
     */
    this.onSwatchClick = null;
  }

  // ----------------------------------------------------------------
  // サブクラスが override するメソッド
  // ----------------------------------------------------------------

  /**
   * パレットが保持する色配列を返す。
   * @returns {number[]}  0xAARRGGBB 形式
   */
  getColors() { return []; }

  /**
   * パレット名（UIWindow のタイトルバーに使われる）。
   * @returns {string}
   */
  getPaletteName() { return 'Palette'; }

  // ----------------------------------------------------------------
  // サイズ計算
  // ----------------------------------------------------------------

  /**
   * コンテンツ領域のサイズを返す。UIWindow.getContentSize() から呼ばれる。
   * @returns {{ w: number, h: number }}
   */
  getContentSize() {
    const { PADDING, CELL_SIZE, CELL_GAP, COLS, SWATCH_H, LABEL_H } = ColorPalette;
    const colors = this.getColors();
    const rows   = Math.ceil(colors.length / COLS);
    const cellsH = rows > 0 ? rows * CELL_SIZE + (rows - 1) * CELL_GAP : 0;

    const w = PADDING * 2 + COLS * CELL_SIZE + (COLS - 1) * CELL_GAP;
    const h = PADDING + LABEL_H + PADDING + SWATCH_H + PADDING + cellsH + PADDING;
    return { w, h };
  }

  // ----------------------------------------------------------------
  // 描画
  // ----------------------------------------------------------------

  /**
   * パレットのコンテンツを指定座標に描画する。
   * (ウィンドウ枠・背景は UIWindow 側が描画済みであること)
   * @param {CanvasRenderingContext2D} ctx
   * @param {number}  x       コンテンツ領域の左上 X
   * @param {number}  y       コンテンツ領域の左上 Y
   * @param {AppData} appData
   */
  render(ctx, x, y, appData) {
    const { PADDING, CELL_SIZE, CELL_GAP, COLS, SWATCH_H, LABEL_H,
            LABEL_FONT, LABEL_COLOR, TITLE_FONT, TITLE_COLOR } = ColorPalette;

    const colors = this.getColors();
    const { w: contentW } = this.getContentSize();

    let curY = y + PADDING;

    // ---- パレット名ラベル ----
    ctx.save();
    ctx.font         = TITLE_FONT;
    ctx.fillStyle    = TITLE_COLOR;
    ctx.textAlign    = 'center';
    ctx.textBaseline = 'top';
    ctx.fillText(this.getPaletteName(), x + contentW / 2, curY);
    ctx.restore();
    curY += LABEL_H + PADDING;

    // ---- FG / BG スウォッチ ----
    this._renderSwatches(ctx, x, curY, contentW, SWATCH_H, appData, LABEL_FONT, LABEL_COLOR);
    curY += SWATCH_H + PADDING;

    // ---- カラーセル ----
    this._cells = [];
    for (let i = 0; i < colors.length; i++) {
      const col = i % COLS;
      const row = Math.floor(i / COLS);
      const cx  = x + PADDING + col * (CELL_SIZE + CELL_GAP);
      const cy  = curY + row * (CELL_SIZE + CELL_GAP);
      this._cells.push({ x: cx, y: cy, w: CELL_SIZE, h: CELL_SIZE, color: colors[i] });

      this._drawChecker(ctx, cx, cy, CELL_SIZE);
      ctx.fillStyle = PixelData.toCssColor(colors[i]);
      ctx.fillRect(cx, cy, CELL_SIZE, CELL_SIZE);

      if (this._hoverIndex === i) {
        ctx.save();
        ctx.strokeStyle = ColorPalette.HOVER_STROKE;
        ctx.lineWidth   = 1.5;
        ctx.strokeRect(cx + 0.75, cy + 0.75, CELL_SIZE - 1.5, CELL_SIZE - 1.5);
        ctx.restore();
      }
    }
  }

  /**
   * FG / BG スウォッチ領域を描画する。
   * BG スウォッチを右下にずらして重ね表示する古典的レイアウト。
   */
  _renderSwatches(ctx, x, y, areaW, areaH, appData, labelFont, labelColor) {
    const sw     = 18;
    const offset = 7;
    const totalW = sw + offset;
    const startX = x + Math.floor((areaW - totalW) / 2);
    const startY = y + Math.floor((areaH - (sw + offset)) / 2);
    const fgX = startX;
    const fgY = startY;
    const bgX = startX + offset;
    const bgY = startY + offset;

    // スウォッチ矩形を保持 (ヒットテスト用)
    this._fgSwatch = { x: fgX, y: fgY, w: sw, h: sw };
    this._bgSwatch = { x: bgX, y: bgY, w: sw, h: sw };

    // BG スウォッチ (奥)
    this._drawChecker(ctx, bgX, bgY, sw);
    ctx.fillStyle   = PixelData.toCssColor(appData.backColor);
    ctx.fillRect(bgX, bgY, sw, sw);
    ctx.strokeStyle = '#777777';
    ctx.lineWidth   = 1;
    ctx.strokeRect(bgX + 0.5, bgY + 0.5, sw - 1, sw - 1);

    // FG スウォッチ (手前)
    this._drawChecker(ctx, fgX, fgY, sw);
    ctx.fillStyle   = PixelData.toCssColor(appData.foreColor);
    ctx.fillRect(fgX, fgY, sw, sw);
    ctx.strokeStyle = '#cccccc';
    ctx.lineWidth   = 1;
    ctx.strokeRect(fgX + 0.5, fgY + 0.5, sw - 1, sw - 1);

    // ラベル
    ctx.save();
    ctx.font         = labelFont;
    ctx.fillStyle    = labelColor;
    ctx.textAlign    = 'left';
    ctx.textBaseline = 'bottom';
    ctx.fillText('F', fgX + 1, fgY + sw - 1);
    ctx.fillStyle = '#888888';
    ctx.fillText('B', bgX + 1, bgY + sw - 1);
    ctx.restore();
  }

  /** 小さな市松模様をセル背景に描く */
  _drawChecker(ctx, x, y, size) {
    const half = Math.floor(size / 2);
    const rest = size - half;
    ctx.fillStyle = '#bbbbbb';
    ctx.fillRect(x,        y,        half, half);
    ctx.fillRect(x + half, y + half, rest, rest);
    ctx.fillStyle = '#888888';
    ctx.fillRect(x + half, y,        rest, half);
    ctx.fillRect(x,        y + half, half, rest);
  }

  // ----------------------------------------------------------------
  // 入力処理
  // ----------------------------------------------------------------

  /**
   * マウス移動。ホバー状態を更新する。
   * @param {{x: number, y: number}} e
   */
  onMouseMove(e) {
    this._hoverIndex = this._hitTestCell(e.x, e.y);
  }

  /**
   * マウスダウン。
   * 左クリック → appData.foreColor 更新。右クリック → appData.backColor 更新。
   * @param {{x: number, y: number, button: number}} e
   * @param {AppData} appData
   * @returns {boolean} セルにヒットした場合 true
   */
  onMouseDown(e, appData) {
    // スウォッチクリック判定 (カラーピッカー呼び出し)
    if (e.button === 0 && this.onSwatchClick) {
      if (this._fgSwatch && this._hitRect(e.x, e.y, this._fgSwatch)) {
        this.onSwatchClick('fore', appData);
        return true;
      }
      if (this._bgSwatch && this._hitRect(e.x, e.y, this._bgSwatch)) {
        this.onSwatchClick('back', appData);
        return true;
      }
    }

    const idx = this._hitTestCell(e.x, e.y);
    if (idx < 0) return false;
    const color = this.getColors()[idx];
    if (e.button === 2) {
      appData.backColor = color;
    } else {
      appData.foreColor = color;
    }
    return true;
  }

  /** 矩形ヒットテスト */
  _hitRect(x, y, r) {
    return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
  }

  /**
   * セルのヒットテスト。ヒットしたインデックスを返す (-1 = なし)。
   */
  _hitTestCell(x, y) {
    for (let i = 0; i < this._cells.length; i++) {
      const c = this._cells[i];
      if (x >= c.x && x < c.x + c.w && y >= c.y && y < c.y + c.h) return i;
    }
    return -1;
  }
}
