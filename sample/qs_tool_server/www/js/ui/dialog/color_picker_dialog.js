/**
 * ColorPickerDialog
 * HSV カラーピッカーダイアログ。
 * DialogBase を継承し、canvas 上にモーダルで描画する。
 *
 * 機能:
 *   - SV 領域 (彩度/明度) + Hue バー による直感的な色選択
 *   - R / G / B / A スピナー入力
 *   - Hex (#RRGGBB) テキスト入力
 *   - 旧色 / 新色プレビュー
 *   - キャンセル / OK ボタン
 *
 * 使用例:
 *   const dialog = new ColorPickerDialog((color) => {
 *     appData.foreColor = color;
 *   });
 *   dialog.showWithColor(appData.foreColor);
 */
class ColorPickerDialog extends DialogBase {
  static DIALOG_W = 420;
  static DIALOG_H = 370;

  // SV 領域サイズ
  static SV_W = 200;
  static SV_H = 180;

  // Hue バーサイズ
  static HUE_W = 20;
  static HUE_H = 180;

  /**
   * @param {(color: number) => void} onConfirm  OK 時コールバック (0xAARRGGBB)
   * @param {() => void} [onCancel]
   */
  constructor(onConfirm, onCancel = () => {}) {
    super('カラーピッカー', ColorPickerDialog.DIALOG_W, ColorPickerDialog.DIALOG_H);

    /** @type {(color: number) => void} */
    this.onConfirm = onConfirm;
    /** @type {() => void} */
    this.onCancel = onCancel;

    // ---- HSV 状態 ----
    /** @type {number} 0-360 */
    this._h = 0;
    /** @type {number} 0-1 */
    this._s = 1;
    /** @type {number} 0-1 */
    this._v = 1;
    /** @type {number} 0-255 */
    this._a = 255;

    // ---- 旧色 (show 時点の色) ----
    /** @type {number} 0xAARRGGBB */
    this._oldColor = 0xFF000000;

    // ---- ドラッグ状態 ----
    /** @type {'sv' | 'hue' | null} */
    this._dragging = null;

    // ---- キーボード入力 ----
    /** @type {string|null} 'r'|'g'|'b'|'a'|'hex' */
    this._focusedField = null;
    /** @type {string} */
    this._inputBuf = '';

    /** @type {string|null} */
    this._hover = null;

    /** @type {Object.<string, {x:number,y:number,w:number,h:number}>} */
    this._rects = {};

    // ---- SV グラデーションキャッシュ ----
    /** @type {OffscreenCanvas|HTMLCanvasElement|null} */
    this._svCanvas = null;
    /** @type {number} キャッシュ時の Hue */
    this._svCacheHue = -1;
  }

  // ----------------------------------------------------------------
  // 表示制御
  // ----------------------------------------------------------------

  /**
   * 指定色でダイアログを開く。
   * @param {number} color  0xAARRGGBB
   */
  showWithColor(color) {
    const { r, g, b, a } = PixelData.unpack(color);
    this._a = a;
    this._oldColor = color;
    const hsv = ColorPickerDialog.rgbToHsv(r, g, b);
    this._h = hsv.h;
    this._s = hsv.s;
    this._v = hsv.v;
    this._focusedField = null;
    this._inputBuf = '';
    this._dragging = null;
    this._hover = null;
    this.show();
  }

  // ----------------------------------------------------------------
  // 色変換 (static)
  // ----------------------------------------------------------------

  /**
   * RGB → HSV
   * @param {number} r 0-255
   * @param {number} g 0-255
   * @param {number} b 0-255
   * @returns {{ h: number, s: number, v: number }}  h: 0-360, s: 0-1, v: 0-1
   */
  static rgbToHsv(r, g, b) {
    r /= 255; g /= 255; b /= 255;
    const max = Math.max(r, g, b);
    const min = Math.min(r, g, b);
    const d = max - min;
    let h = 0;
    if (d > 0) {
      if (max === r)      h = ((g - b) / d + 6) % 6;
      else if (max === g) h = (b - r) / d + 2;
      else                h = (r - g) / d + 4;
      h *= 60;
    }
    const s = max > 0 ? d / max : 0;
    return { h, s, v: max };
  }

  /**
   * HSV → RGB
   * @param {number} h 0-360
   * @param {number} s 0-1
   * @param {number} v 0-1
   * @returns {{ r: number, g: number, b: number }}  各 0-255
   */
  static hsvToRgb(h, s, v) {
    const c = v * s;
    const x = c * (1 - Math.abs((h / 60) % 2 - 1));
    const m = v - c;
    let r1, g1, b1;
    if      (h < 60)  { r1 = c; g1 = x; b1 = 0; }
    else if (h < 120) { r1 = x; g1 = c; b1 = 0; }
    else if (h < 180) { r1 = 0; g1 = c; b1 = x; }
    else if (h < 240) { r1 = 0; g1 = x; b1 = c; }
    else if (h < 300) { r1 = x; g1 = 0; b1 = c; }
    else              { r1 = c; g1 = 0; b1 = x; }
    return {
      r: Math.round((r1 + m) * 255),
      g: Math.round((g1 + m) * 255),
      b: Math.round((b1 + m) * 255),
    };
  }

  // ----------------------------------------------------------------
  // 現在の色を取得
  // ----------------------------------------------------------------

  /** @returns {{ r:number, g:number, b:number }} */
  _currentRgb() {
    return ColorPickerDialog.hsvToRgb(this._h, this._s, this._v);
  }

  /** @returns {number} 0xAARRGGBB */
  _currentColor() {
    const { r, g, b } = this._currentRgb();
    return PixelData.rgba(r, g, b, this._a);
  }

  /** @returns {string} '#RRGGBB' */
  _currentHex() {
    const { r, g, b } = this._currentRgb();
    return '#' + ((1 << 24) | (r << 16) | (g << 8) | b).toString(16).slice(1).toUpperCase();
  }

  // ----------------------------------------------------------------
  // SV グラデーション生成
  // ----------------------------------------------------------------

  /**
   * 現在の Hue に基づいて SV グラデーションを描画した canvas を返す。
   * Hue が変わるまでキャッシュを再利用する。
   */
  _getSvCanvas() {
    const w = ColorPickerDialog.SV_W;
    const h = ColorPickerDialog.SV_H;
    const hue = Math.round(this._h);

    if (this._svCanvas && this._svCacheHue === hue) {
      return this._svCanvas;
    }

    if (!this._svCanvas) {
      if (typeof OffscreenCanvas !== 'undefined') {
        this._svCanvas = new OffscreenCanvas(w, h);
      } else {
        this._svCanvas = document.createElement('canvas');
        this._svCanvas.width = w;
        this._svCanvas.height = h;
      }
    }

    const ctx = this._svCanvas.getContext('2d');
    const imgData = ctx.createImageData(w, h);
    const data = imgData.data;

    for (let y = 0; y < h; y++) {
      const v = 1 - y / (h - 1);
      for (let x = 0; x < w; x++) {
        const s = x / (w - 1);
        const { r, g, b } = ColorPickerDialog.hsvToRgb(hue, s, v);
        const idx = (y * w + x) * 4;
        data[idx]     = r;
        data[idx + 1] = g;
        data[idx + 2] = b;
        data[idx + 3] = 255;
      }
    }
    ctx.putImageData(imgData, 0, 0);
    this._svCacheHue = hue;
    return this._svCanvas;
  }

  // ----------------------------------------------------------------
  // 描画
  // ----------------------------------------------------------------

  renderBody(ctx, bx, by, bw, bh) {
    ctx.save();

    const PAD = 16;
    const LABEL_FONT  = '13px sans-serif';
    const LABEL_COLOR = '#222222';
    const SMALL_FONT  = '11px sans-serif';

    const label = (text, x, y) => {
      ctx.fillStyle    = LABEL_COLOR;
      ctx.font         = LABEL_FONT;
      ctx.textBaseline = 'middle';
      ctx.fillText(text, x, y);
      ctx.textBaseline = 'alphabetic';
    };

    const { SV_W, SV_H, HUE_W, HUE_H } = ColorPickerDialog;

    // ---- SV 領域 ----
    const svX = bx + PAD;
    const svY = by + PAD;
    this._rects.sv = { x: svX, y: svY, w: SV_W, h: SV_H };

    // 市松模様背景 (透明色のため)
    this._drawCheckerBoard(ctx, svX, svY, SV_W, SV_H);

    // SV グラデーション
    const svCanvas = this._getSvCanvas();
    ctx.drawImage(svCanvas, svX, svY, SV_W, SV_H);

    // 枠線
    ctx.strokeStyle = '#888880';
    ctx.lineWidth = 1;
    ctx.strokeRect(svX + 0.5, svY + 0.5, SV_W - 1, SV_H - 1);

    // クロスヘア (現在の S/V 位置)
    const crossX = svX + this._s * (SV_W - 1);
    const crossY = svY + (1 - this._v) * (SV_H - 1);
    ctx.save();
    ctx.strokeStyle = '#ffffff';
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.arc(crossX, crossY, 5, 0, Math.PI * 2);
    ctx.stroke();
    ctx.strokeStyle = '#000000';
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.arc(crossX, crossY, 6, 0, Math.PI * 2);
    ctx.stroke();
    ctx.restore();

    // ---- Hue バー ----
    const hueX = svX + SV_W + 10;
    const hueY = svY;
    this._rects.hue = { x: hueX, y: hueY, w: HUE_W, h: HUE_H };

    this._drawHueBar(ctx, hueX, hueY, HUE_W, HUE_H);

    // Hue インジケータ
    const hueIndicY = hueY + (this._h / 360) * (HUE_H - 1);
    ctx.save();
    ctx.strokeStyle = '#000000';
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.moveTo(hueX - 2, hueIndicY);
    ctx.lineTo(hueX + HUE_W + 2, hueIndicY);
    ctx.stroke();
    ctx.strokeStyle = '#ffffff';
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(hueX - 1, hueIndicY - 1);
    ctx.lineTo(hueX + HUE_W + 1, hueIndicY - 1);
    ctx.moveTo(hueX - 1, hueIndicY + 1);
    ctx.lineTo(hueX + HUE_W + 1, hueIndicY + 1);
    ctx.stroke();
    ctx.restore();

    // --- 枠線
    ctx.strokeStyle = '#888880';
    ctx.lineWidth = 1;
    ctx.strokeRect(hueX + 0.5, hueY + 0.5, HUE_W - 1, HUE_H - 1);

    // ---- 右カラム (プレビュー + RGBA + Hex) ----
    const rightX = hueX + HUE_W + 20;
    let ry = svY;

    // 旧色ラベル + スウォッチ
    label('旧色:', rightX, ry + 10);
    const oldSwR = { x: rightX + 42, y: ry, w: 48, h: 22 };
    this._drawCheckerRect(ctx, oldSwR.x, oldSwR.y, oldSwR.w, oldSwR.h);
    ctx.fillStyle = PixelData.toCssColor(this._oldColor);
    ctx.fillRect(oldSwR.x, oldSwR.y, oldSwR.w, oldSwR.h);
    ctx.strokeStyle = '#888880';
    ctx.lineWidth = 1;
    ctx.strokeRect(oldSwR.x + 0.5, oldSwR.y + 0.5, oldSwR.w - 1, oldSwR.h - 1);

    ry += 28;

    // 新色ラベル + スウォッチ
    label('新色:', rightX, ry + 10);
    const newSwR = { x: rightX + 42, y: ry, w: 48, h: 22 };
    this._drawCheckerRect(ctx, newSwR.x, newSwR.y, newSwR.w, newSwR.h);
    ctx.fillStyle = PixelData.toCssColor(this._currentColor());
    ctx.fillRect(newSwR.x, newSwR.y, newSwR.w, newSwR.h);
    ctx.strokeStyle = '#888880';
    ctx.lineWidth = 1;
    ctx.strokeRect(newSwR.x + 0.5, newSwR.y + 0.5, newSwR.w - 1, newSwR.h - 1);

    ry += 36;

    // ---- RGBA フィールド ----
    const { r, g, b } = this._currentRgb();
    const rgbaRows = [
      { key: 'r', label: 'R:', val: r },
      { key: 'g', label: 'G:', val: g },
      { key: 'b', label: 'B:', val: b },
      { key: 'a', label: 'A:', val: this._a },
    ];

    const fieldW = 52;
    const fieldH = 22;
    for (const row of rgbaRows) {
      label(row.label, rightX, ry + fieldH / 2);
      const fR = { x: rightX + 22, y: ry, w: fieldW, h: fieldH };
      this._rects[`field_${row.key}`] = fR;
      const isFocused = this._focusedField === row.key;
      this._drawInputField(ctx, fR,
        isFocused ? this._inputBuf : String(row.val), isFocused);
      ry += fieldH + 4;
    }

    ry += 4;

    // ---- Hex フィールド ----
    label('Hex:', rightX, ry + fieldH / 2);
    const hexR = { x: rightX + 32, y: ry, w: 72, h: fieldH };
    this._rects.field_hex = hexR;
    const hexFocused = this._focusedField === 'hex';
    this._drawInputField(ctx, hexR,
      hexFocused ? this._inputBuf : this._currentHex(), hexFocused);

    // ---- 区切り線 ----
    const sepY = by + bh - 52;
    ctx.strokeStyle = '#c0bdb8';
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(bx + PAD, sepY + 0.5);
    ctx.lineTo(bx + bw - PAD, sepY + 0.5);
    ctx.stroke();

    // ---- ボタン行 ----
    const btnW = 90;
    const btnH = 28;
    const btnY = sepY + 12;
    const cancelR = { x: bx + bw - PAD - btnW * 2 - 10, y: btnY, w: btnW, h: btnH };
    const okR     = { x: bx + bw - PAD - btnW,           y: btnY, w: btnW, h: btnH };
    this._rects.btnCancel = cancelR;
    this._rects.btnOK     = okR;

    this._drawButton(ctx, cancelR.x, cancelR.y, cancelR.w, cancelR.h,
      'キャンセル', this._hover === 'btnCancel');
    this._drawButton(ctx, okR.x, okR.y, okR.w, okR.h,
      'OK', this._hover === 'btnOK', true);

    ctx.restore();
  }

  // ----------------------------------------------------------------
  // Hue バー描画
  // ----------------------------------------------------------------

  _drawHueBar(ctx, x, y, w, h) {
    for (let i = 0; i < h; i++) {
      const hue = (i / (h - 1)) * 360;
      const { r, g, b } = ColorPickerDialog.hsvToRgb(hue, 1, 1);
      ctx.fillStyle = `rgb(${r},${g},${b})`;
      ctx.fillRect(x, y + i, w, 1);
    }
  }

  // ----------------------------------------------------------------
  // 市松模様描画 (透明表示用)
  // ----------------------------------------------------------------

  _drawCheckerBoard(ctx, x, y, w, h) {
    const size = 8;
    for (let cy = 0; cy < h; cy += size) {
      for (let cx = 0; cx < w; cx += size) {
        const even = ((cx / size + cy / size) & 1) === 0;
        ctx.fillStyle = even ? '#cccccc' : '#999999';
        const rw = Math.min(size, w - cx);
        const rh = Math.min(size, h - cy);
        ctx.fillRect(x + cx, y + cy, rw, rh);
      }
    }
  }

  _drawCheckerRect(ctx, x, y, w, h) {
    const size = 4;
    for (let cy = 0; cy < h; cy += size) {
      for (let cx = 0; cx < w; cx += size) {
        const even = ((cx / size + cy / size) & 1) === 0;
        ctx.fillStyle = even ? '#cccccc' : '#999999';
        const rw = Math.min(size, w - cx);
        const rh = Math.min(size, h - cy);
        ctx.fillRect(x + cx, y + cy, rw, rh);
      }
    }
  }

  // ----------------------------------------------------------------
  // 入力フィールド描画
  // ----------------------------------------------------------------

  _drawInputField(ctx, r, text, focused) {
    ctx.fillStyle   = focused ? '#ffffff' : '#f8f8f6';
    ctx.strokeStyle = focused ? '#0078d7' : '#a0a0a0';
    ctx.lineWidth   = focused ? 2 : 1;
    this._roundRect(ctx, r.x, r.y, r.w, r.h, 3);
    ctx.fill();
    ctx.stroke();

    ctx.fillStyle    = '#000000';
    ctx.font         = '12px monospace';
    ctx.textBaseline = 'middle';
    ctx.textAlign    = 'left';
    // テキストをフィールドの左パディングに描画
    ctx.save();
    ctx.beginPath();
    ctx.rect(r.x + 2, r.y, r.w - 4, r.h);
    ctx.clip();
    ctx.fillText(text, r.x + 4, r.y + r.h / 2);
    ctx.restore();

    // フォーカス中のカーソル
    if (focused) {
      const tw = ctx.measureText(text).width;
      const curX = r.x + 4 + tw + 1;
      ctx.fillStyle = '#000000';
      ctx.fillRect(curX, r.y + r.h / 2 - 7, 1, 14);
    }

    ctx.textAlign    = 'left';
    ctx.textBaseline = 'alphabetic';
  }

  // ----------------------------------------------------------------
  // 入力ハンドラ
  // ----------------------------------------------------------------

  onMouseMoveBody(e, dx, bodyY) {
    // ドラッグ中は色を追従
    if (this._dragging === 'sv') {
      this._pickSv(e.x, e.y);
      return;
    }
    if (this._dragging === 'hue') {
      this._pickHue(e.x, e.y);
      return;
    }
    this._hover = this._hitTest(e.x, e.y);
  }

  onMouseDownBody(e, dx, bodyY) {
    if (e.button !== 0) return;
    this._commitInput();

    // SV 領域
    const sv = this._rects.sv;
    if (sv && this._inRect(e.x, e.y, sv)) {
      this._dragging = 'sv';
      this._pickSv(e.x, e.y);
      return;
    }

    // Hue バー
    const hue = this._rects.hue;
    if (hue && this._inRect(e.x, e.y, hue)) {
      this._dragging = 'hue';
      this._pickHue(e.x, e.y);
      return;
    }

    const key = this._hitTest(e.x, e.y);
    if (!key) {
      this._focusedField = null;
      return;
    }

    // RGBA フィールド
    for (const ch of ['r', 'g', 'b', 'a']) {
      if (key === `field_${ch}`) {
        this._focusedField = ch;
        this._inputBuf = '';
        return;
      }
    }

    // Hex フィールド
    if (key === 'field_hex') {
      this._focusedField = 'hex';
      this._inputBuf = this._currentHex();
      return;
    }

    if (key === 'btnCancel') { this._cancel(); return; }
    if (key === 'btnOK')     { this._confirmAction(); return; }
  }

  onMouseUpBody(e, dx, bodyY) {
    this._dragging = null;
  }

  onKeyDownBody(e) {
    // Escape: ダイアログ閉じる
    if (e.key === 'Escape') {
      if (this._focusedField) {
        this._focusedField = null;
        this._inputBuf = '';
      } else {
        this._cancel();
      }
      return;
    }

    // Enter: フィールドフォーカス中は確定、それ以外は OK
    if (e.key === 'Enter') {
      if (this._focusedField) {
        this._commitInput();
      } else {
        this._confirmAction();
      }
      return;
    }

    if (e.key === 'Tab') {
      this._commitInput();
      // フィールド順送り
      const order = ['r', 'g', 'b', 'a', 'hex'];
      const idx = order.indexOf(this._focusedField);
      if (idx >= 0) {
        this._focusedField = order[(idx + 1) % order.length];
        this._inputBuf = '';
      }
      return;
    }

    if (!this._focusedField) return;

    if (e.key === 'Backspace') {
      this._inputBuf = this._inputBuf.slice(0, -1);
      return;
    }

    if (this._focusedField === 'hex') {
      // Hex: #, 0-9, A-F
      if (/^[0-9a-fA-F#]$/.test(e.key) && this._inputBuf.length < 7) {
        this._inputBuf += e.key;
      }
    } else {
      // RGBA: 数字のみ
      if (/^[0-9]$/.test(e.key) && this._inputBuf.length < 3) {
        this._inputBuf += e.key;
      }
    }
  }

  // ----------------------------------------------------------------
  // SV / Hue ピッキング
  // ----------------------------------------------------------------

  _pickSv(mx, my) {
    const r = this._rects.sv;
    if (!r) return;
    this._s = Math.max(0, Math.min(1, (mx - r.x) / (r.w - 1)));
    this._v = Math.max(0, Math.min(1, 1 - (my - r.y) / (r.h - 1)));
  }

  _pickHue(mx, my) {
    const r = this._rects.hue;
    if (!r) return;
    this._h = Math.max(0, Math.min(360, ((my - r.y) / (r.h - 1)) * 360));
  }

  // ----------------------------------------------------------------
  // 入力確定
  // ----------------------------------------------------------------

  _commitInput() {
    if (!this._focusedField || this._inputBuf === '') {
      this._focusedField = null;
      this._inputBuf = '';
      return;
    }

    if (this._focusedField === 'hex') {
      this._commitHex();
    } else {
      this._commitRgba();
    }
    this._focusedField = null;
    this._inputBuf = '';
  }

  _commitRgba() {
    const v = Math.max(0, Math.min(255, parseInt(this._inputBuf, 10) || 0));
    const { r, g, b } = this._currentRgb();
    let nr = r, ng = g, nb = b, na = this._a;

    switch (this._focusedField) {
      case 'r': nr = v; break;
      case 'g': ng = v; break;
      case 'b': nb = v; break;
      case 'a': na = v; this._a = na; break;
    }

    if (this._focusedField !== 'a') {
      const hsv = ColorPickerDialog.rgbToHsv(nr, ng, nb);
      this._h = hsv.h;
      this._s = hsv.s;
      this._v = hsv.v;
    }
  }

  _commitHex() {
    let hex = this._inputBuf.replace(/^#/, '');
    // 3桁ショートハンド展開
    if (hex.length === 3) {
      hex = hex[0] + hex[0] + hex[1] + hex[1] + hex[2] + hex[2];
    }
    if (hex.length !== 6) return;
    const val = parseInt(hex, 16);
    if (isNaN(val)) return;
    const r = (val >> 16) & 0xff;
    const g = (val >> 8) & 0xff;
    const b = val & 0xff;
    const hsv = ColorPickerDialog.rgbToHsv(r, g, b);
    this._h = hsv.h;
    this._s = hsv.s;
    this._v = hsv.v;
  }

  // ----------------------------------------------------------------
  // ボタンアクション
  // ----------------------------------------------------------------

  _confirmAction() {
    this._commitInput();
    this.hide();
    this.onConfirm(this._currentColor());
  }

  _cancel() {
    this.hide();
    this.onCancel();
  }

  // ----------------------------------------------------------------
  // ヒットテスト
  // ----------------------------------------------------------------

  _hitTest(x, y) {
    for (const [key, r] of Object.entries(this._rects)) {
      if (key === 'sv' || key === 'hue') continue; // SV/Hue は別処理
      if (this._inRect(x, y, r)) return key;
    }
    return null;
  }

  _inRect(x, y, r) {
    return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
  }
}
