/**
 * ImportTilesetDialog
 * 既存PNGからタイルセットとして読み込む際のチップサイズ指定ダイアログ。
 * DialogBase を継承し、canvas 上にモーダルで描画する。
 *
 * 機能:
 *   - 読み込み画像サイズの表示
 *   - チップサイズプリセット (8×8 / 16×16 / 32×32 / 48×48 / 64×64)
 *   - 自動算出される列数・行数の表示
 *   - 割り切れない場合の警告表示
 *   - キャンセル / 開く ボタン
 */
class ImportTilesetDialog extends DialogBase {
  static DIALOG_W = 400;
  static DIALOG_H = 280;

  static CHIP_PRESETS = [8, 16, 32, 48, 64];

  /**
   * @param {(chipW: number, chipH: number) => void} onConfirm
   * @param {() => void} [onCancel]
   */
  constructor(onConfirm, onCancel = () => {}) {
    super('タイルセットとして開く', ImportTilesetDialog.DIALOG_W, ImportTilesetDialog.DIALOG_H);

    /** @type {(chipW: number, chipH: number) => void} */
    this.onConfirm = onConfirm;
    /** @type {() => void} */
    this.onCancel  = onCancel;

    /** 読み込み画像の幅 @type {number} */
    this._imageWidth  = 0;
    /** 読み込み画像の高さ @type {number} */
    this._imageHeight = 0;
    /** 選択中のチップ幅 @type {number} */
    this._chipW = 16;
    /** 選択中のチップ高さ @type {number} */
    this._chipH = 16;

    /** ホバー中の要素キー @type {string|null} */
    this._hover = null;

    /** 各 UI 要素の描画座標 @type {Object.<string, {x:number,y:number,w:number,h:number}>} */
    this._rects = {};
  }

  // ----------------------------------------------------------------
  // 表示制御
  // ----------------------------------------------------------------

  /**
   * 画像サイズを指定してダイアログを開く。
   * @param {number} imageWidth
   * @param {number} imageHeight
   */
  showWithImage(imageWidth, imageHeight) {
    this._imageWidth  = imageWidth;
    this._imageHeight = imageHeight;
    this._chipW = 16;
    this._chipH = 16;
    this._hover = null;
    this._rects = {};
    this.show();
  }

  // ----------------------------------------------------------------
  // 算出プロパティ
  // ----------------------------------------------------------------

  get _columns() { return this._chipW > 0 ? Math.floor(this._imageWidth / this._chipW) : 0; }
  get _rows()    { return this._chipH > 0 ? Math.floor(this._imageHeight / this._chipH) : 0; }
  get _hasRemainder() {
    return (this._imageWidth % this._chipW !== 0) || (this._imageHeight % this._chipH !== 0);
  }

  // ----------------------------------------------------------------
  // DialogBase オーバーライド
  // ----------------------------------------------------------------

  /**
   * @param {CanvasRenderingContext2D} ctx
   * @param {number} bx - ボディ左上 X
   * @param {number} by - ボディ左上 Y
   * @param {number} bw - ボディ幅
   * @param {number} bh - ボディ高さ
   */
  renderBody(ctx, bx, by, bw, bh) {
    ctx.save();

    const LABEL_FONT  = '13px sans-serif';
    const LABEL_COLOR = '#222222';
    const PAD         = 20;

    const label = (text, x, y) => {
      ctx.fillStyle    = LABEL_COLOR;
      ctx.font         = LABEL_FONT;
      ctx.textBaseline = 'middle';
      ctx.fillText(text, x, y);
      ctx.textBaseline = 'alphabetic';
    };

    let y = by + 16;

    // =====================================================
    // セクション: 画像情報
    // =====================================================
    label(`画像サイズ: ${this._imageWidth} × ${this._imageHeight} px`, bx + PAD, y + 7);
    y += 32;

    // =====================================================
    // セクション: チップサイズ プリセット
    // =====================================================
    label('チップサイズ:', bx + PAD, y + 7);
    y += 24;

    const presetW   = 58;
    const presetH   = 26;
    const presetGap = 6;
    let px = bx + PAD;
    this._rects.presets = [];
    for (const s of ImportTilesetDialog.CHIP_PRESETS) {
      const key = `chip_${s}`;
      const r = { x: px, y, w: presetW, h: presetH };
      this._rects.presets.push({ key, size: s, ...r });

      const isSelected = (this._chipW === s && this._chipH === s);
      const isHovered  = this._hover === key;
      this._drawPresetButton(ctx, r.x, r.y, r.w, r.h, `${s}×${s}`, isSelected, isHovered);
      px += presetW + presetGap;
    }
    y += presetH + 20;

    // =====================================================
    // セクション: 算出結果
    // =====================================================
    const cols = this._columns;
    const rows = this._rows;
    ctx.fillStyle    = '#555555';
    ctx.font         = '12px sans-serif';
    ctx.textBaseline = 'middle';
    ctx.fillText(
      `タイルセット: ${cols} 列 × ${rows} 行  (${cols * rows} チップ)`,
      bx + PAD, y + 7
    );
    ctx.textBaseline = 'alphabetic';
    y += 22;

    // 割り切れない場合の警告
    if (this._hasRemainder) {
      ctx.fillStyle    = '#cc4400';
      ctx.font         = '11px sans-serif';
      ctx.textBaseline = 'middle';
      ctx.fillText(
        '⚠ 画像サイズがチップサイズで割り切れません。端のピクセルは無視されます。',
        bx + PAD, y + 7
      );
      ctx.textBaseline = 'alphabetic';
      y += 22;
    }

    y += 16;

    // =====================================================
    // 区切り線
    // =====================================================
    ctx.strokeStyle = '#c0bdb8';
    ctx.lineWidth   = 1;
    ctx.beginPath();
    ctx.moveTo(bx + PAD,      y + 0.5);
    ctx.lineTo(bx + bw - PAD, y + 0.5);
    ctx.stroke();
    y += 14;

    // =====================================================
    // ボタン行: キャンセル / 開く
    // =====================================================
    const btnW = 90;
    const btnH = 28;
    const canConfirm = cols > 0 && rows > 0;

    const cancelR = { x: bx + bw - PAD - btnW * 2 - 10, y, w: btnW, h: btnH };
    const okR     = { x: bx + bw - PAD - btnW,           y, w: btnW, h: btnH };
    this._rects.btnCancel = cancelR;
    this._rects.btnOK     = okR;

    this._drawButton(ctx, cancelR.x, cancelR.y, cancelR.w, cancelR.h,
                     'キャンセル', this._hover === 'btnCancel');
    this._drawButton(ctx, okR.x, okR.y, okR.w, okR.h,
                     '開く', this._hover === 'btnOK' && canConfirm, canConfirm);

    ctx.restore();
  }

  // ----------------------------------------------------------------
  // 入力ハンドラ
  // ----------------------------------------------------------------

  onMouseMoveBody(e) {
    this._hover = this._hitTest(e.x, e.y);
  }

  onMouseDownBody(e) {
    if (e.button !== 0) return;
    const key = this._hitTest(e.x, e.y);
    if (!key) return;

    // チップサイズプリセット
    const preset = this._rects.presets?.find(p => p.key === key);
    if (preset) {
      this._chipW = preset.size;
      this._chipH = preset.size;
      return;
    }

    if (key === 'btnCancel') { this._cancel();  return; }
    if (key === 'btnOK')     { this._confirm(); return; }
  }

  onMouseUpBody(e) {}

  onKeyDownBody(e) {
    if (e.key === 'Escape') { this._cancel();  return; }
    if (e.key === 'Enter')  { this._confirm(); return; }
  }

  // ----------------------------------------------------------------
  // 内部ロジック
  // ----------------------------------------------------------------

  _confirm() {
    if (this._columns <= 0 || this._rows <= 0) return;
    this.hide();
    this.onConfirm(this._chipW, this._chipH);
  }

  _cancel() {
    this.hide();
    this.onCancel();
  }

  /**
   * 座標からヒットした UI キーを返す
   * @param {number} x
   * @param {number} y
   * @returns {string|null}
   */
  _hitTest(x, y) {
    for (const p of (this._rects.presets ?? [])) {
      if (this._inRect(x, y, p)) return p.key;
    }
    for (const [key, r] of Object.entries(this._rects)) {
      if (key === 'presets') continue;
      if (this._inRect(x, y, r)) return key;
    }
    return null;
  }

  _inRect(x, y, r) {
    return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
  }

  // ----------------------------------------------------------------
  // 描画ユーティリティ
  // ----------------------------------------------------------------

  _drawPresetButton(ctx, x, y, w, h, label, selected, hovered) {
    const bg = selected
      ? '#0078d7'
      : (hovered ? '#dcdad5' : '#e8e6e1');
    const fg     = selected ? '#ffffff' : '#000000';
    const border = selected ? '#005a9e' : '#a0a0a0';

    ctx.fillStyle   = bg;
    ctx.strokeStyle = border;
    ctx.lineWidth   = 1;
    this._roundRect(ctx, x, y, w, h, 3);
    ctx.fill();
    ctx.stroke();

    ctx.fillStyle    = fg;
    ctx.font         = '12px sans-serif';
    ctx.textBaseline = 'middle';
    ctx.textAlign    = 'center';
    ctx.fillText(label, x + w / 2, y + h / 2);
    ctx.textAlign    = 'left';
    ctx.textBaseline = 'alphabetic';
  }
}
