/**
 * NewFileDialog
 * ドット絵エディタの新規作成ダイアログ。
 * DialogBase を継承し、canvas 上にモーダルで描画する。
 *
 * 機能:
 *   - サイズプリセット (16 / 32 / 48 / 64 / 128 px 正方形)
 *   - 幅・高さのスピナー (◄/► ボタン、フィールドクリックでキーボード入力)
 *   - 背景色選択 (透明 / 白)
 *   - キャンセル / 作成 ボタン
 */
class NewFileDialog extends DialogBase {
  static DIALOG_W = 380;
  static DIALOG_H = 316;

  static PRESETS = [16, 32, 48, 64, 128];
  static W_MIN   = 1;
  static W_MAX   = 512;
  static H_MIN   = 1;
  static H_MAX   = 512;

  /**
   * @param {(width:number, height:number, bgColor:string) => void} onConfirm
   * @param {() => void} [onCancel]
   */
  constructor(onConfirm, onCancel = () => {}) {
    super('新規作成', NewFileDialog.DIALOG_W, NewFileDialog.DIALOG_H);

    this.onConfirm = onConfirm;
    this.onCancel  = onCancel;

    /** 選択中の幅 */
    this._w = 32;
    /** 選択中の高さ */
    this._h = 32;
    /** 背景色: 'transparent' | 'white' */
    this._bgColor = 'transparent';

    /**
     * キーボード入力中フィールド: 'width' | 'height' | null
     * フィールドをクリック → 入力バッファがアクティブになる
     */
    this._focusedField = null;
    /** キーボードで入力中の文字列バッファ */
    this._inputBuf = '';

    /**
     * ホバー中の要素キー
     * @type {string|null}
     */
    this._hover = null;

    /**
     * 各 UI 要素の描画座標 (renderBody で毎フレーム更新)
     * @type {Object.<string, {x:number,y:number,w:number,h:number}>}
     */
    this._rects = {};
  }

  // ----------------------------------------------------------------
  // DialogBase オーバーライド
  // ----------------------------------------------------------------

  /**
   * @param {CanvasRenderingContext2D} ctx
   * @param {number} bx   - ボディ左上 X
   * @param {number} by   - ボディ左上 Y
   * @param {number} bw   - ボディ幅
   * @param {number} bh   - ボディ高さ
   */
  renderBody(ctx, bx, by, bw, bh) {
    ctx.save();

    const LABEL_FONT  = '13px sans-serif';
    const LABEL_COLOR = '#222222';
    const PAD         = 20;

    // ---- ラベルヘルパー ----
    const label = (text, x, y) => {
      ctx.fillStyle    = LABEL_COLOR;
      ctx.font         = LABEL_FONT;
      ctx.textBaseline = 'middle';
      ctx.fillText(text, x, y);
      ctx.textBaseline = 'alphabetic';
    };

    let y = by + 16;

    // =====================================================
    // セクション: サイズプリセット
    // =====================================================
    label('サイズ プリセット:', bx + PAD, y + 7);
    y += 24;

    const presetW  = 58;
    const presetH  = 26;
    const presetGap = 6;
    let px = bx + PAD;
    this._rects.presets = [];
    for (const s of NewFileDialog.PRESETS) {
      const key = `preset_${s}`;
      const r = { x: px, y, w: presetW, h: presetH };
      this._rects.presets.push({ key, size: s, ...r });

      const isSelected = (this._w === s && this._h === s);
      const isHovered  = this._hover === key;
      this._drawPresetButton(ctx, r.x, r.y, r.w, r.h, `${s}×${s}`, isSelected, isHovered);
      px += presetW + presetGap;
    }
    y += presetH + 20;

    // =====================================================
    // セクション: 幅 / 高さ スピナー
    // =====================================================
    const spinnerRows = [
      { key: 'width',  label: '幅 :',   val: this._w },
      { key: 'height', label: '高さ:', val: this._h },
    ];

    for (const row of spinnerRows) {
      const decKey   = `${row.key}_dec`;
      const incKey   = `${row.key}_inc`;
      const fieldKey = `${row.key}_field`;

      // ラベル
      label(row.label, bx + PAD, y + 14);

      const btnW  = 24;
      const btnH  = 26;
      const fieldW = 72;
      const startX = bx + PAD + 52;

      // ◄ ボタン
      const decR = { x: startX, y, w: btnW, h: btnH };
      this._rects[decKey] = decR;
      this._drawButton(ctx, decR.x, decR.y, decR.w, decR.h, '◄',
                       this._hover === decKey);

      // 数値フィールド
      const fieldR = { x: startX + btnW + 4, y, w: fieldW, h: btnH };
      this._rects[fieldKey] = fieldR;
      const isFocused = this._focusedField === row.key;
      this._drawSpinnerField(ctx, fieldR, isFocused
        ? (this._inputBuf || String(row.val))
        : String(row.val), isFocused);

      // ► ボタン
      const incR = { x: fieldR.x + fieldW + 4, y, w: btnW, h: btnH };
      this._rects[incKey] = incR;
      this._drawButton(ctx, incR.x, incR.y, incR.w, incR.h, '►',
                       this._hover === incKey);

      // 範囲ヒント
      ctx.fillStyle    = '#888888';
      ctx.font         = '11px sans-serif';
      ctx.textBaseline = 'middle';
      ctx.fillText(`(1 – 512)`, incR.x + incR.w + 10, y + 13);
      ctx.textBaseline = 'alphabetic';

      y += btnH + 12;
    }

    y += 8;

    // =====================================================
    // セクション: 背景色
    // =====================================================
    label('背景色:', bx + PAD, y + 10);

    const radioItems = [
      { key: 'bg_transparent', label: '透明',   value: 'transparent' },
      { key: 'bg_white',       label: '白 (白塗り)', value: 'white' },
    ];
    let rx = bx + PAD + 58;
    for (const item of radioItems) {
      const r = { x: rx, y, w: 110, h: 22 };
      this._rects[item.key] = r;
      this._drawRadio(ctx, r.x, r.y, item.label,
                      this._bgColor === item.value,
                      this._hover   === item.key);
      rx += 120;
    }

    y += 36;

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
    // ボタン行: キャンセル / 作成
    // =====================================================
    const btnW2 = 90;
    const btnH2 = 28;
    const cancelR = { x: bx + bw - PAD - btnW2 * 2 - 10, y, w: btnW2, h: btnH2 };
    const okR     = { x: bx + bw - PAD - btnW2,           y, w: btnW2, h: btnH2 };
    this._rects.btnCancel = cancelR;
    this._rects.btnOK     = okR;

    this._drawButton(ctx, cancelR.x, cancelR.y, cancelR.w, cancelR.h,
                     'キャンセル', this._hover === 'btnCancel');
    this._drawButton(ctx, okR.x,     okR.y,     okR.w,     okR.h,
                     '作成',       this._hover === 'btnOK', true);

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
    if (!key) {
      this._focusedField = null;
      this._inputBuf     = '';
      return;
    }

    // プリセットボタン
    const preset = this._rects.presets?.find(p => p.key === key);
    if (preset) {
      this._w = preset.size;
      this._h = preset.size;
      this._focusedField = null;
      this._inputBuf     = '';
      return;
    }

    // スピナー
    if (key === 'width_dec')    { this._w = Math.max(NewFileDialog.W_MIN, this._w - 1); return; }
    if (key === 'width_inc')    { this._w = Math.min(NewFileDialog.W_MAX, this._w + 1); return; }
    if (key === 'height_dec')   { this._h = Math.max(NewFileDialog.H_MIN, this._h - 1); return; }
    if (key === 'height_inc')   { this._h = Math.min(NewFileDialog.H_MAX, this._h + 1); return; }

    // フィールドフォーカス
    if (key === 'width_field')  { this._focusedField = 'width';  this._inputBuf = ''; return; }
    if (key === 'height_field') { this._focusedField = 'height'; this._inputBuf = ''; return; }

    // 背景色
    if (key === 'bg_transparent') { this._bgColor = 'transparent'; return; }
    if (key === 'bg_white')        { this._bgColor = 'white';       return; }

    // ボタン
    if (key === 'btnCancel') { this.hide(); this.onCancel(); return; }
    if (key === 'btnOK')     { this._confirm(); return; }
  }

  onMouseUpBody(e) {}

  onKeyDownBody(e) {
    if (!this._focusedField) return;

    if (e.key === 'Escape') {
      this._focusedField = null;
      this._inputBuf     = '';
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
      // 最大4桁
      if (this._inputBuf.length > 4) this._inputBuf = this._inputBuf.slice(-4);
    }
  }

  // ----------------------------------------------------------------
  // 内部ロジック
  // ----------------------------------------------------------------

  /** キーボード入力中の値を確定する */
  _commitInput() {
    if (!this._focusedField || this._inputBuf === '') {
      this._focusedField = null;
      return;
    }
    const v = Math.max(1, Math.min(512, parseInt(this._inputBuf, 10) || 1));
    if (this._focusedField === 'width')  this._w = v;
    if (this._focusedField === 'height') this._h = v;
    this._focusedField = null;
    this._inputBuf     = '';
  }

  /** 作成ボタン押下 */
  _confirm() {
    this._commitInput();
    this.hide();
    this.onConfirm(this._w, this._h, this._bgColor);
  }

  /**
   * 座標からヒットした UI キーを返す
   * @param {number} x
   * @param {number} y
   * @returns {string|null}
   */
  _hitTest(x, y) {
    // プリセット
    for (const p of (this._rects.presets ?? [])) {
      if (this._inRect(x, y, p)) return p.key;
    }
    // それ以外
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

  _drawSpinnerField(ctx, r, text, focused) {
    ctx.fillStyle   = focused ? '#ffffff' : '#f8f8f6';
    ctx.strokeStyle = focused ? '#0078d7' : '#a0a0a0';
    ctx.lineWidth   = focused ? 2 : 1;
    this._roundRect(ctx, r.x, r.y, r.w, r.h, 3);
    ctx.fill();
    ctx.stroke();

    ctx.fillStyle    = '#000000';
    ctx.font         = '13px sans-serif';
    ctx.textBaseline = 'middle';
    ctx.textAlign    = 'center';
    ctx.fillText(text, r.x + r.w / 2, r.y + r.h / 2);
    // フォーカス時にカーソルを模擬
    if (focused) {
      const tw = ctx.measureText(text).width;
      const cx = r.x + r.w / 2 + tw / 2 + 2;
      const cy = r.y + r.h / 2;
      ctx.fillStyle = '#000000';
      ctx.fillRect(cx, cy - 7, 1, 14);
    }
    ctx.textAlign    = 'left';
    ctx.textBaseline = 'alphabetic';
  }

  _drawRadio(ctx, x, y, text, checked, hovered) {
    const cx = x + 10;
    const cy = y + 11;
    const r  = 7;

    // 外円
    ctx.strokeStyle = checked ? '#0078d7' : '#888880';
    ctx.lineWidth   = checked ? 2 : 1;
    ctx.beginPath();
    ctx.arc(cx, cy, r, 0, Math.PI * 2);
    ctx.stroke();

    // 内円（選択時）
    if (checked) {
      ctx.fillStyle = '#0078d7';
      ctx.beginPath();
      ctx.arc(cx, cy, 4, 0, Math.PI * 2);
      ctx.fill();
    }

    // ラベル
    ctx.fillStyle    = hovered ? '#0078d7' : '#222222';
    ctx.font         = '13px sans-serif';
    ctx.textBaseline = 'middle';
    ctx.fillText(text, cx + r + 6, cy);
    ctx.textBaseline = 'alphabetic';
  }
}
