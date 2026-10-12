/**
 * NineSlicePreview
 * 9 スライスの伸縮結果を確認するプレビュー内容。
 * 描画先の幅・高さを ◄ ► で変えて、角と辺が想定どおり伸びるかを見る。
 */
class NineSlicePreview {
  static PAD = 8;
  static AREA_W = 256;
  static AREA_H = 160;
  static BUTTON_W = 24;
  static BUTTON_H = 22;
  static FIELD_W = 40;
  static STEP = 8;
  static MIN_W = 16;
  static MAX_W = 256;
  static MIN_H = 16;
  static MAX_H = 160;

  constructor() {
    this._destW = 96;
    this._destH = 64;
    this._hover = null;
    this._rects = {};
  }

  getContentSize() {
    const { PAD, AREA_W, AREA_H, BUTTON_H } = NineSlicePreview;
    return {
      w: PAD * 2 + AREA_W,
      h: PAD + BUTTON_H + PAD + AREA_H + PAD,
    };
  }

  /**
   * @param {CanvasRenderingContext2D} ctx
   * @param {number} x  コンテンツ左上 X（絶対座標）
   * @param {number} y  コンテンツ左上 Y（絶対座標）
   * @param {AppData} appData
   */
  render(ctx, x, y, appData) {
    const { PAD, AREA_W, AREA_H, BUTTON_W, BUTTON_H, FIELD_W } = NineSlicePreview;
    const row = y + PAD;

    this._rects = {};
    const dims = [
      { key: 'w', label: 'W', value: this._destW, x: x + PAD },
      { key: 'h', label: 'H', value: this._destH, x: x + PAD + 124 },
    ];
    for (const dim of dims) {
      ctx.fillStyle = '#e2e8f0';
      ctx.font = 'bold 12px sans-serif';
      ctx.textBaseline = 'middle';
      ctx.fillText(dim.label, dim.x, row + BUTTON_H / 2);

      const decR = { x: dim.x + 16, y: row, w: BUTTON_W, h: BUTTON_H };
      const fieldR = { x: decR.x + BUTTON_W + 2, y: row, w: FIELD_W, h: BUTTON_H };
      const incR = { x: fieldR.x + FIELD_W + 2, y: row, w: BUTTON_W, h: BUTTON_H };
      this._rects[`${dim.key}_dec`] = decR;
      this._rects[`${dim.key}_inc`] = incR;
      this._drawButton(ctx, decR, '◄', this._hover === `${dim.key}_dec`);
      this._drawButton(ctx, incR, '►', this._hover === `${dim.key}_inc`);

      ctx.fillStyle = '#0f172a';
      ctx.strokeStyle = '#334155';
      ctx.lineWidth = 1;
      ctx.beginPath();
      ctx.roundRect(fieldR.x, fieldR.y, fieldR.w, fieldR.h, 3);
      ctx.fill();
      ctx.stroke();
      ctx.fillStyle = '#e2e8f0';
      ctx.font = '12px sans-serif';
      ctx.textAlign = 'center';
      ctx.fillText(String(dim.value), fieldR.x + fieldR.w / 2, fieldR.y + fieldR.h / 2);
      ctx.textAlign = 'left';
      ctx.textBaseline = 'alphabetic';
    }

    const areaX = x + PAD;
    const areaY = row + BUTTON_H + PAD;
    ctx.fillStyle = '#020617';
    ctx.strokeStyle = '#1e293b';
    ctx.beginPath();
    ctx.roundRect(areaX, areaY, AREA_W, AREA_H, 6);
    ctx.fill();
    ctx.stroke();

    const insets = appData?.getActiveNineSlice?.() || null;
    const source = NineSliceRenderer.canvasFromPixelData(appData?.layerData?.composite?.() || null);
    if (!source) return;

    const destX = areaX + Math.round((AREA_W - this._destW) / 2);
    const destY = areaY + Math.round((AREA_H - this._destH) / 2);
    NineSliceRenderer.draw(ctx, source, insets, { x: destX, y: destY, w: this._destW, h: this._destH }, 1);
  }

  /**
   * @param {MouseEvent|{x:number,y:number,button?:number}} e
   */
  onMouseMove(e) {
    this._hover = this._hitTest(e.x, e.y);
  }

  /**
   * @returns {boolean} ボタンを押したら true
   */
  onMouseDown(e) {
    if (e.button !== undefined && e.button !== 0) return false;
    const key = this._hitTest(e.x, e.y);
    if (!key) return false;

    const { STEP, MIN_W, MAX_W, MIN_H, MAX_H } = NineSlicePreview;
    if (key === 'w_dec') this._destW = Math.max(MIN_W, this._destW - STEP);
    if (key === 'w_inc') this._destW = Math.min(MAX_W, this._destW + STEP);
    if (key === 'h_dec') this._destH = Math.max(MIN_H, this._destH - STEP);
    if (key === 'h_inc') this._destH = Math.min(MAX_H, this._destH + STEP);
    return true;
  }

  _hitTest(x, y) {
    for (const [key, rect] of Object.entries(this._rects)) {
      if (x >= rect.x && x < rect.x + rect.w && y >= rect.y && y < rect.y + rect.h) return key;
    }
    return null;
  }

  _drawButton(ctx, rect, label, hovered) {
    ctx.fillStyle = hovered ? '#1d4ed8' : '#1e293b';
    ctx.strokeStyle = '#334155';
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.roundRect(rect.x, rect.y, rect.w, rect.h, 3);
    ctx.fill();
    ctx.stroke();
    ctx.fillStyle = '#e2e8f0';
    ctx.font = '12px sans-serif';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillText(label, rect.x + rect.w / 2, rect.y + rect.h / 2);
    ctx.textAlign = 'left';
    ctx.textBaseline = 'alphabetic';
  }
}
