/**
 * NineSliceDialog
 * 9 スライスの境界（左・上・右・下、ソース画素単位）を数値で設定するダイアログ。
 *
 * 左右の合計は画像の幅以下、上下の合計は画像の高さ以下でなければ適用できない。
 * 「解除」は境界を無効にして、画像を全体伸縮として扱う状態に戻す。
 */
class NineSliceDialog extends DialogBase {
  static DIALOG_W = 400;
  static DIALOG_H = 318;
  static EDGE_MIN = 0;

  /**
   * @param {(insets: {left: number, top: number, right: number, bottom: number}) => void} onConfirm
   * @param {() => void} [onClear]
   * @param {() => void} [onCancel]
   */
  constructor(onConfirm, onClear = () => {}, onCancel = () => {}) {
    super('9スライス設定', NineSliceDialog.DIALOG_W, NineSliceDialog.DIALOG_H);
    this.onConfirm = onConfirm;
    this.onClear = onClear;
    this.onCancel = onCancel;

    this._values = { left: 0, top: 0, right: 0, bottom: 0 };
    this._imageW = 0;
    this._imageH = 0;
    this._error = '';
    this._focusedField = null;
    this._inputBuf = '';
    this._hover = null;
    this._rects = {};
  }

  /**
   * @param {{left: number, top: number, right: number, bottom: number}|null} insets
   * @param {number} imageW
   * @param {number} imageH
   */
  showWithInsets(insets, imageW, imageH) {
    this._imageW = imageW | 0;
    this._imageH = imageH | 0;
    const source = insets || { left: 0, top: 0, right: 0, bottom: 0 };
    this._values = { left: source.left, top: source.top, right: source.right, bottom: source.bottom };
    this._error = '';
    this._focusedField = null;
    this._inputBuf = '';
    this.show();
  }

  renderBody(ctx, bx, by, bw, bh) {
    ctx.save();

    const PAD = 20;
    let y = by + 14;
    ctx.fillStyle = '#4b5563';
    ctx.font = '12px sans-serif';
    ctx.textBaseline = 'top';
    ctx.fillText('画像の四辺から、伸縮しない境界の幅（px）を入力します。', bx + PAD, y);
    ctx.fillText(`画像サイズ: ${this._imageW} × ${this._imageH}px`, bx + PAD, y + 18);
    ctx.textBaseline = 'alphabetic';
    y += 48;

    const rows = [
      { key: 'left', label: '左 :', max: this._imageW },
      { key: 'top', label: '上 :', max: this._imageH },
      { key: 'right', label: '右 :', max: this._imageW },
      { key: 'bottom', label: '下 :', max: this._imageH },
    ];
    const btnW = 24;
    const btnH = 26;
    const fieldW = 80;
    const startX = bx + PAD + 52;

    for (const row of rows) {
      ctx.fillStyle = '#222222';
      ctx.font = '13px sans-serif';
      ctx.textBaseline = 'middle';
      ctx.fillText(row.label, bx + PAD, y + 13);
      ctx.textBaseline = 'alphabetic';

      const decR = { x: startX, y, w: btnW, h: btnH };
      this._rects[`${row.key}_dec`] = decR;
      this._drawButton(ctx, decR.x, decR.y, decR.w, decR.h, '◄', this._hover === `${row.key}_dec`);

      const fieldR = { x: startX + btnW + 4, y, w: fieldW, h: btnH };
      this._rects[`${row.key}_field`] = fieldR;
      this._drawField(
        ctx,
        fieldR,
        this._focusedField === row.key ? (this._inputBuf || String(this._values[row.key])) : String(this._values[row.key]),
        this._focusedField === row.key,
      );

      const incR = { x: fieldR.x + fieldW + 4, y, w: btnW, h: btnH };
      this._rects[`${row.key}_inc`] = incR;
      this._drawButton(ctx, incR.x, incR.y, incR.w, incR.h, '►', this._hover === `${row.key}_inc`);

      ctx.fillStyle = '#888888';
      ctx.font = '11px sans-serif';
      ctx.textBaseline = 'middle';
      ctx.fillText(`(0 - ${row.max})`, incR.x + incR.w + 10, y + 13);
      ctx.textBaseline = 'alphabetic';
      y += btnH + 10;
    }

    if (this._error) {
      ctx.fillStyle = '#b91c1c';
      ctx.font = '12px sans-serif';
      ctx.textBaseline = 'middle';
      ctx.fillText(this._error, bx + PAD, y + 8);
      ctx.textBaseline = 'alphabetic';
    }
    y += 22;

    ctx.strokeStyle = '#c0bdb8';
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(bx + PAD, y + 0.5);
    ctx.lineTo(bx + bw - PAD, y + 0.5);
    ctx.stroke();
    y += 14;

    const footerW = 90;
    const footerH = 28;
    const clearR = { x: bx + PAD, y, w: footerW, h: footerH };
    const cancelR = { x: bx + bw - PAD - footerW * 2 - 10, y, w: footerW, h: footerH };
    const okR = { x: bx + bw - PAD - footerW, y, w: footerW, h: footerH };
    this._rects.btnClear = clearR;
    this._rects.btnCancel = cancelR;
    this._rects.btnOK = okR;

    this._drawButton(ctx, clearR.x, clearR.y, clearR.w, clearR.h, '解除', this._hover === 'btnClear');
    this._drawButton(ctx, cancelR.x, cancelR.y, cancelR.w, cancelR.h, 'キャンセル', this._hover === 'btnCancel');
    this._drawButton(ctx, okR.x, okR.y, okR.w, okR.h, '適用', this._hover === 'btnOK', true);
    ctx.restore();
  }

  onMouseMoveBody(e) {
    this._hover = this._hitTest(e.x, e.y);
  }

  onMouseDownBody(e) {
    if (e.button !== 0) return;
    const key = this._hitTest(e.x, e.y);
    if (!key) {
      this._focusedField = null;
      this._inputBuf = '';
      return;
    }

    const field = key.match(/^(left|top|right|bottom)_(dec|inc|field)$/);
    if (field) {
      const [, edge, action] = field;
      if (action === 'dec') this._setEdge(edge, this._values[edge] - 1);
      if (action === 'inc') this._setEdge(edge, this._values[edge] + 1);
      if (action === 'field') {
        this._focusedField = edge;
        this._inputBuf = '';
      }
      return;
    }

    if (key === 'btnClear') {
      this.hide();
      this.onClear();
      return;
    }
    if (key === 'btnCancel') {
      this.hide();
      this.onCancel();
      return;
    }
    if (key === 'btnOK') this._confirm();
  }

  onKeyDownBody(e) {
    if (!this._focusedField) {
      if (e.key === 'Escape') {
        this.hide();
        this.onCancel();
      }
      if (e.key === 'Enter') this._confirm();
      return;
    }

    if (e.key === 'Escape') {
      this._focusedField = null;
      this._inputBuf = '';
      return;
    }
    if (e.key === 'Enter' || e.key === 'Tab') {
      this._commitInput();
      return;
    }
    if (e.key === 'Backspace') {
      this._inputBuf = this._inputBuf.slice(0, -1);
      return;
    }
    if (/^[0-9]$/.test(e.key)) {
      this._inputBuf += e.key;
      if (this._inputBuf.length > 4) this._inputBuf = this._inputBuf.slice(-4);
    }
  }

  _setEdge(edge, value) {
    const max = edge === 'left' || edge === 'right' ? this._imageW : this._imageH;
    this._values[edge] = Math.max(NineSliceDialog.EDGE_MIN, Math.min(max, value));
    this._error = '';
  }

  _confirm() {
    this._commitInput();
    const insets = { ...this._values };
    if (insets.left + insets.right > this._imageW || insets.top + insets.bottom > this._imageH) {
      this._error = '左右・上下の合計が画像サイズを超えています';
      return;
    }
    this.hide();
    this.onConfirm(insets);
  }

  _commitInput() {
    if (!this._focusedField || this._inputBuf === '') {
      this._focusedField = null;
      return;
    }
    this._setEdge(this._focusedField, parseInt(this._inputBuf, 10) || 0);
    this._focusedField = null;
    this._inputBuf = '';
  }

  _hitTest(x, y) {
    for (const [key, rect] of Object.entries(this._rects)) {
      if (x >= rect.x && x < rect.x + rect.w && y >= rect.y && y < rect.y + rect.h) return key;
    }
    return null;
  }

  _drawField(ctx, rect, text, focused) {
    ctx.fillStyle = focused ? '#ffffff' : '#f8f8f6';
    ctx.strokeStyle = focused ? '#0078d7' : '#a0a0a0';
    ctx.lineWidth = focused ? 2 : 1;
    this._roundRect(ctx, rect.x, rect.y, rect.w, rect.h, 3);
    ctx.fill();
    ctx.stroke();

    ctx.fillStyle = '#000000';
    ctx.font = '13px sans-serif';
    ctx.textBaseline = 'middle';
    ctx.textAlign = 'center';
    ctx.fillText(text, rect.x + rect.w / 2, rect.y + rect.h / 2);
    if (focused) {
      const textW = ctx.measureText(text).width;
      ctx.fillRect(rect.x + rect.w / 2 + textW / 2 + 2, rect.y + rect.h / 2 - 7, 1, 14);
    }
    ctx.textAlign = 'left';
    ctx.textBaseline = 'alphabetic';
  }
}
