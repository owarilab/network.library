class MapResizeDialog extends DialogBase {
  static DIALOG_W = 360;
  static DIALOG_H = 244;
  static SIZE_MIN = 1;
  static SIZE_MAX = 512;

  constructor(onConfirm, onCancel = () => {}) {
    super('マップサイズ変更', MapResizeDialog.DIALOG_W, MapResizeDialog.DIALOG_H);
    this.onConfirm = onConfirm;
    this.onCancel = onCancel;

    this._w = 24;
    this._h = 18;
    this._focusedField = null;
    this._inputBuf = '';
    this._hover = null;
    this._rects = {};
  }

  showWithSize(width, height) {
    this._w = Math.max(MapResizeDialog.SIZE_MIN, Math.min(MapResizeDialog.SIZE_MAX, width | 0 || 24));
    this._h = Math.max(MapResizeDialog.SIZE_MIN, Math.min(MapResizeDialog.SIZE_MAX, height | 0 || 18));
    this._focusedField = null;
    this._inputBuf = '';
    this.show();
  }

  renderBody(ctx, bx, by, bw, bh) {
    ctx.save();

    const PAD = 20;
    const label = (text, x, y) => {
      ctx.fillStyle = '#222222';
      ctx.font = '13px sans-serif';
      ctx.textBaseline = 'middle';
      ctx.fillText(text, x, y);
      ctx.textBaseline = 'alphabetic';
    };

    let y = by + 18;
    ctx.fillStyle = '#4b5563';
    ctx.font = '12px sans-serif';
    ctx.textBaseline = 'top';
    ctx.fillText('左上固定でマップセル数を変更します。', bx + PAD, y);
    ctx.fillText('拡大時は空セルを追加し、縮小時は範囲外セルを破棄します。', bx + PAD, y + 18);
    ctx.textBaseline = 'alphabetic';
    y += 52;

    const rows = [
      { key: 'width', label: '幅 :', value: this._w },
      { key: 'height', label: '高さ:', value: this._h },
    ];

    for (const row of rows) {
      const decKey = `${row.key}_dec`;
      const incKey = `${row.key}_inc`;
      const fieldKey = `${row.key}_field`;
      label(row.label, bx + PAD, y + 14);

      const btnW = 24;
      const btnH = 26;
      const fieldW = 80;
      const startX = bx + PAD + 52;

      const decR = { x: startX, y, w: btnW, h: btnH };
      this._rects[decKey] = decR;
      this._drawButton(ctx, decR.x, decR.y, decR.w, decR.h, '◄', this._hover === decKey);

      const fieldR = { x: startX + btnW + 4, y, w: fieldW, h: btnH };
      this._rects[fieldKey] = fieldR;
      this._drawField(
        ctx,
        fieldR,
        this._focusedField === row.key ? (this._inputBuf || String(row.value)) : String(row.value),
        this._focusedField === row.key,
      );

      const incR = { x: fieldR.x + fieldW + 4, y, w: btnW, h: btnH };
      this._rects[incKey] = incR;
      this._drawButton(ctx, incR.x, incR.y, incR.w, incR.h, '►', this._hover === incKey);

      ctx.fillStyle = '#888888';
      ctx.font = '11px sans-serif';
      ctx.textBaseline = 'middle';
      ctx.fillText('(1 - 512 cells)', incR.x + incR.w + 10, y + 13);
      ctx.textBaseline = 'alphabetic';
      y += btnH + 14;
    }

    y += 8;
    ctx.strokeStyle = '#c0bdb8';
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(bx + PAD, y + 0.5);
    ctx.lineTo(bx + bw - PAD, y + 0.5);
    ctx.stroke();
    y += 16;

    const btnW = 90;
    const btnH = 28;
    const cancelR = { x: bx + bw - PAD - btnW * 2 - 10, y, w: btnW, h: btnH };
    const okR = { x: bx + bw - PAD - btnW, y, w: btnW, h: btnH };
    this._rects.btnCancel = cancelR;
    this._rects.btnOK = okR;

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

    if (key === 'width_dec') { this._w = Math.max(MapResizeDialog.SIZE_MIN, this._w - 1); return; }
    if (key === 'width_inc') { this._w = Math.min(MapResizeDialog.SIZE_MAX, this._w + 1); return; }
    if (key === 'height_dec') { this._h = Math.max(MapResizeDialog.SIZE_MIN, this._h - 1); return; }
    if (key === 'height_inc') { this._h = Math.min(MapResizeDialog.SIZE_MAX, this._h + 1); return; }

    if (key === 'width_field') { this._focusedField = 'width'; this._inputBuf = ''; return; }
    if (key === 'height_field') { this._focusedField = 'height'; this._inputBuf = ''; return; }

    if (key === 'btnCancel') {
      this.hide();
      this.onCancel();
      return;
    }
    if (key === 'btnOK') {
      this._confirm();
    }
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

  _confirm() {
    this._commitInput();
    this.hide();
    this.onConfirm(this._w, this._h);
  }

  _commitInput() {
    if (!this._focusedField || this._inputBuf === '') {
      this._focusedField = null;
      return;
    }
    const value = Math.max(MapResizeDialog.SIZE_MIN, Math.min(MapResizeDialog.SIZE_MAX, parseInt(this._inputBuf, 10) || 1));
    if (this._focusedField === 'width') this._w = value;
    if (this._focusedField === 'height') this._h = value;
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
      const cursorX = rect.x + rect.w / 2 + textW / 2 + 2;
      const cursorY = rect.y + rect.h / 2;
      ctx.fillRect(cursorX, cursorY - 7, 1, 14);
    }
    ctx.textAlign = 'left';
    ctx.textBaseline = 'alphabetic';
  }
}