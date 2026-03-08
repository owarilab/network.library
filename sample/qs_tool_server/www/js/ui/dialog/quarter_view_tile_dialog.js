/**
 * QuarterViewTileDialog
 * クォータービュータイル自動生成ダイアログ。
 * DialogBase を継承し、canvas 上にモーダルで描画する。
 *
 * 機能:
 *   - タイプ選択（地面タイル / 箱タイル）
 *   - サイズプリセットボタン群（タイプごとに切り替え）
 *   - 幅・壁高さスピナー（カスタムサイズ）
 *   - レイヤー分けチェックボックス
 *   - 色設定（上面/左面/右面/輪郭）スウォッチ
 *   - 透明化オプション（上面/左面/右面）
 *   - 適用先選択（新規作成 / 上書き）
 *   - リアルタイムプレビュー
 *   - キャンセル / 生成 ボタン
 */
class QuarterViewTileDialog extends DialogBase {
  static DIALOG_W = 440;
  static DIALOG_H = 520;

  static PRESETS_GROUND = [
    { label: '16×8',  w: 16, wallHeight: 0 },
    { label: '32×16', w: 32, wallHeight: 0 },
    { label: '48×24', w: 48, wallHeight: 0 },
    { label: '64×32', w: 64, wallHeight: 0 },
  ];

  static PRESETS_BOX = [
    { label: '16×16', w: 16, wallHeight: 8  },
    { label: '32×24', w: 32, wallHeight: 8  },
    { label: '32×32', w: 32, wallHeight: 16 },
    { label: '64×48', w: 64, wallHeight: 16 },
    { label: '64×64', w: 64, wallHeight: 32 },
  ];

  static W_MIN = 4;
  static W_MAX = 512;
  static W_STEP = 4;
  static WALL_MIN = 1;
  static WALL_MAX = 256;

  /**
   * @param {(params: Object) => void} onConfirm  生成時コールバック
   * @param {() => void} [onCancel]
   */
  constructor(onConfirm, onCancel = () => {}) {
    super('クォータービュータイル生成', QuarterViewTileDialog.DIALOG_W, QuarterViewTileDialog.DIALOG_H);

    this.onConfirm = onConfirm;
    this.onCancel  = onCancel;

    /** @type {'ground' | 'box'} */
    this._type = 'box';
    /** @type {number} キャンバス幅 */
    this._w = 32;
    /** @type {number} 壁高さ */
    this._wallHeight = 16;
    /** @type {boolean} レイヤー分け */
    this._separateLayers = false;
    /** @type {number} 上面色 0xAARRGGBB */
    this._topColor = 0xFFC8C8C8;
    /** @type {number} 左面色 */
    this._leftColor = 0xFF808080;
    /** @type {number} 右面色 */
    this._rightColor = 0xFF606060;
    /** @type {number} 輪郭色 */
    this._outlineColor = 0xFF000000;
    /** @type {boolean} 上面透明化 */
    this._topTransparent = false;
    /** @type {boolean} 左面透明化 */
    this._leftTransparent = false;
    /** @type {boolean} 右面透明化 */
    this._rightTransparent = false;
    /** @type {'new' | 'overwrite'} */
    this._target = 'new';

    /** @type {string|null} */
    this._focusedField = null;
    /** @type {string} */
    this._inputBuf = '';
    /** @type {string|null} */
    this._hover = null;
    /** @type {Object.<string, {x:number,y:number,w:number,h:number}>} */
    this._rects = {};

    /**
     * 色変更用コールバック: スウォッチクリック時にセットされ、
     * ColorPickerDialog から色が返ったときに適用する。
     * @type {((color:number) => void)|null}
     */
    this._colorPickerCallback = null;

    /** プレビュー用キャッシュ @type {PixelData|null} */
    this._previewPd = null;
    this._previewDirty = true;
  }

  // ----------------------------------------------------------------
  // 表示制御
  // ----------------------------------------------------------------

  show() {
    super.show();
    this._previewDirty = true;
    this._focusedField = null;
    this._inputBuf = '';
    this._hover = null;
  }

  // ----------------------------------------------------------------
  // プレビュー生成
  // ----------------------------------------------------------------

  /** @returns {PixelData} */
  _getPreview() {
    if (!this._previewDirty && this._previewPd) return this._previewPd;

    const params = this._buildParams();
    // プレビュー用に常に 1レイヤー・新規生成
    const previewParams = Object.assign({}, params, {
      separateLayers: false,
      target: 'new',
    });
    const ld = QuarterViewTileGenerator.generate(previewParams);
    this._previewPd = ld ? ld.composite() : null;
    this._previewDirty = false;
    return this._previewPd;
  }

  /** 現在の設定からパラメータオブジェクトを組み立てる */
  _buildParams() {
    return {
      type:           this._type,
      width:          this._w,
      wallHeight:     this._type === 'box' ? this._wallHeight : 0,
      separateLayers: this._separateLayers,
      topColor:       this._topTransparent ? 0x00000000 : this._topColor,
      leftColor:      this._leftTransparent ? 0x00000000 : this._leftColor,
      rightColor:     this._rightTransparent ? 0x00000000 : this._rightColor,
      outlineColor:   this._outlineColor,
      target:         this._target,
    };
  }

  /** 設定変更時にプレビューを無効化する */
  _markPreviewDirty() {
    this._previewDirty = true;
  }

  // ----------------------------------------------------------------
  // 描画ヘルパー
  // ----------------------------------------------------------------

  _calcHeight() {
    return this._type === 'box'
      ? (this._w / 2 + this._wallHeight)
      : (this._w / 2);
  }

  // ----------------------------------------------------------------
  // renderBody
  // ----------------------------------------------------------------

  renderBody(ctx, bx, by, bw, bh) {
    ctx.save();

    const LABEL_FONT  = '13px sans-serif';
    const LABEL_COLOR = '#222222';
    const PAD         = 16;
    const SMALL_FONT  = '11px sans-serif';

    const label = (text, x, y) => {
      ctx.fillStyle    = LABEL_COLOR;
      ctx.font         = LABEL_FONT;
      ctx.textBaseline = 'middle';
      ctx.fillText(text, x, y);
      ctx.textBaseline = 'alphabetic';
    };

    // 左右分割: 左側 = 設定、右側 = プレビュー
    const previewAreaW = 110;
    const settingsW = bw - PAD * 2 - previewAreaW;
    let y = by + 12;

    // =====================================================
    // タイプ選択
    // =====================================================
    label('タイプ:', bx + PAD, y + 7);
    const radioItems = [
      { key: 'type_ground', label: '地面タイル', value: 'ground' },
      { key: 'type_box',    label: '箱タイル',   value: 'box'    },
    ];
    let rx = bx + PAD + 56;
    for (const item of radioItems) {
      const r = { x: rx, y: y - 3, w: 100, h: 22 };
      this._rects[item.key] = r;
      this._drawRadio(ctx, r.x, r.y, item.label,
                      this._type === item.value,
                      this._hover === item.key);
      rx += 108;
    }
    y += 26;

    // =====================================================
    // サイズプリセット
    // =====================================================
    label('プリセット:', bx + PAD, y + 7);
    y += 22;

    const presets = this._type === 'box'
      ? QuarterViewTileDialog.PRESETS_BOX
      : QuarterViewTileDialog.PRESETS_GROUND;
    const presetW  = 56;
    const presetH  = 24;
    const presetGap = 5;
    let px = bx + PAD;
    this._rects._presets = [];
    for (let i = 0; i < presets.length; i++) {
      const p = presets[i];
      const key = `preset_${i}`;
      const r = { x: px, y, w: presetW, h: presetH };
      this._rects._presets.push({ key, ...r, preset: p });

      const isSelected = (this._w === p.w && this._wallHeight === p.wallHeight);
      const isHovered  = this._hover === key;
      this._drawPresetButton(ctx, r.x, r.y, r.w, r.h, p.label, isSelected, isHovered);
      px += presetW + presetGap;
    }
    y += presetH + 14;

    // =====================================================
    // カスタムサイズ: 幅
    // =====================================================
    {
      label('幅:', bx + PAD, y + 13);
      const startX = bx + PAD + 72;
      const btnW = 24, btnH = 24, fieldW = 60;

      const decR = { x: startX, y, w: btnW, h: btnH };
      this._rects.w_dec = decR;
      this._drawButton(ctx, decR.x, decR.y, decR.w, decR.h, '◄', this._hover === 'w_dec');

      const fieldR = { x: startX + btnW + 3, y, w: fieldW, h: btnH };
      this._rects.w_field = fieldR;
      const wFocused = this._focusedField === 'width';
      this._drawSpinnerField(ctx, fieldR, wFocused ? (this._inputBuf || String(this._w)) : String(this._w), wFocused);

      const incR = { x: fieldR.x + fieldW + 3, y, w: btnW, h: btnH };
      this._rects.w_inc = incR;
      this._drawButton(ctx, incR.x, incR.y, incR.w, incR.h, '►', this._hover === 'w_inc');

      ctx.fillStyle = '#888888'; ctx.font = SMALL_FONT; ctx.textBaseline = 'middle';
      ctx.fillText('(4の倍数)', incR.x + incR.w + 8, y + 12);
      ctx.textBaseline = 'alphabetic';
    }
    y += 30;

    // =====================================================
    // カスタムサイズ: 壁の高さ（boxのみ）
    // =====================================================
    if (this._type === 'box') {
      label('壁の高さ:', bx + PAD, y + 13);
      const startX = bx + PAD + 72;
      const btnW = 24, btnH = 24, fieldW = 60;

      const decR = { x: startX, y, w: btnW, h: btnH };
      this._rects.wh_dec = decR;
      this._drawButton(ctx, decR.x, decR.y, decR.w, decR.h, '◄', this._hover === 'wh_dec');

      const fieldR = { x: startX + btnW + 3, y, w: fieldW, h: btnH };
      this._rects.wh_field = fieldR;
      const whFocused = this._focusedField === 'wallHeight';
      this._drawSpinnerField(ctx, fieldR,
        whFocused ? (this._inputBuf || String(this._wallHeight)) : String(this._wallHeight),
        whFocused);

      const incR = { x: fieldR.x + fieldW + 3, y, w: btnW, h: btnH };
      this._rects.wh_inc = incR;
      this._drawButton(ctx, incR.x, incR.y, incR.w, incR.h, '►', this._hover === 'wh_inc');

      ctx.fillStyle = '#888888'; ctx.font = SMALL_FONT; ctx.textBaseline = 'middle';
      ctx.fillText(`(1–256)`, incR.x + incR.w + 8, y + 12);
      ctx.textBaseline = 'alphabetic';

      y += 30;
    } else {
      // box以外の場合、壁高さ系のrectsをクリア
      delete this._rects.wh_dec;
      delete this._rects.wh_field;
      delete this._rects.wh_inc;
    }

    // キャンバスサイズ表示
    const calcH = this._calcHeight();
    ctx.fillStyle = '#555555'; ctx.font = SMALL_FONT; ctx.textBaseline = 'middle';
    ctx.fillText(`キャンバスサイズ: ${this._w} × ${calcH}`, bx + PAD, y + 6);
    ctx.textBaseline = 'alphabetic';
    y += 20;

    // =====================================================
    // レイヤー分け
    // =====================================================
    {
      const r = { x: bx + PAD, y, w: 200, h: 20 };
      this._rects.chk_layers = r;
      this._drawCheckbox(ctx, r.x, r.y, 'レイヤーを分ける', this._separateLayers, this._hover === 'chk_layers');
    }
    y += 26;

    // =====================================================
    // 色設定
    // =====================================================
    label('色設定:', bx + PAD, y + 7);
    y += 22;

    const swatchSize = 22;
    const swatchGap = 6;
    const swatchItems = [
      { key: 'color_top',     label: '上面',  color: this._topColor,     transparent: this._topTransparent },
      { key: 'color_left',    label: '左面',  color: this._leftColor,    transparent: this._leftTransparent },
      { key: 'color_right',   label: '右面',  color: this._rightColor,   transparent: this._rightTransparent },
      { key: 'color_outline', label: '輪郭',  color: this._outlineColor, transparent: false },
    ];

    let sx = bx + PAD;
    for (const item of swatchItems) {
      // 左面/右面は boxタイプのみ表示
      if ((item.key === 'color_left' || item.key === 'color_right') && this._type !== 'box') continue;

      const r = { x: sx, y, w: swatchSize, h: swatchSize };
      this._rects[item.key] = r;
      this._drawSwatch(ctx, r.x, r.y, swatchSize, item.color, item.transparent, this._hover === item.key);

      ctx.fillStyle = LABEL_COLOR; ctx.font = '11px sans-serif'; ctx.textBaseline = 'middle';
      ctx.fillText(item.label, r.x + swatchSize + 3, r.y + swatchSize / 2);
      ctx.textBaseline = 'alphabetic';

      sx += swatchSize + 40 + swatchGap;
    }
    y += swatchSize + 10;

    // =====================================================
    // 透明化オプション
    // =====================================================
    {
      const chkItems = [
        { key: 'chk_top_tr', label: '上面を透明にする', field: '_topTransparent' },
      ];
      if (this._type === 'box') {
        chkItems.push({ key: 'chk_left_tr',  label: '左面を透明にする', field: '_leftTransparent' });
        chkItems.push({ key: 'chk_right_tr', label: '右面を透明にする', field: '_rightTransparent' });
      }
      let cx = bx + PAD;
      for (const item of chkItems) {
        const r = { x: cx, y, w: 130, h: 18 };
        this._rects[item.key] = r;
        this._drawCheckbox(ctx, r.x, r.y, item.label, this[item.field], this._hover === item.key);
        cx += 138;
      }
    }
    y += 24;

    // =====================================================
    // 適用先
    // =====================================================
    label('適用先:', bx + PAD, y + 7);
    const targetItems = [
      { key: 'target_new',       label: '新規作成',           value: 'new' },
      { key: 'target_overwrite', label: '現在のキャンバスに上書き', value: 'overwrite' },
    ];
    let tx = bx + PAD + 56;
    for (const item of targetItems) {
      const r = { x: tx, y: y - 3, w: 160, h: 22 };
      this._rects[item.key] = r;
      this._drawRadio(ctx, r.x, r.y, item.label,
                      this._target === item.value,
                      this._hover === item.key);
      tx += 168;
    }
    y += 28;

    // =====================================================
    // 区切り線
    // =====================================================
    ctx.strokeStyle = '#c0bdb8';
    ctx.lineWidth   = 1;
    ctx.beginPath();
    ctx.moveTo(bx + PAD,      y + 0.5);
    ctx.lineTo(bx + bw - PAD, y + 0.5);
    ctx.stroke();
    y += 10;

    // =====================================================
    // ボタン行: キャンセル / 生成
    // =====================================================
    const btnW2 = 90;
    const btnH2 = 28;
    const cancelR = { x: bx + bw - PAD - btnW2 * 2 - 10, y, w: btnW2, h: btnH2 };
    const okR     = { x: bx + bw - PAD - btnW2,           y, w: btnW2, h: btnH2 };
    this._rects.btnCancel = cancelR;
    this._rects.btnOK     = okR;

    this._drawButton(ctx, cancelR.x, cancelR.y, cancelR.w, cancelR.h,
                     'キャンセル', this._hover === 'btnCancel');
    this._drawButton(ctx, okR.x, okR.y, okR.w, okR.h,
                     '生成', this._hover === 'btnOK', true);

    // =====================================================
    // プレビュー
    // =====================================================
    const previewX = bx + bw - PAD - previewAreaW + 4;
    const previewY = by + 12;
    const previewBoxW = previewAreaW - 8;
    const previewBoxH = previewAreaW - 8;

    // 枠
    ctx.strokeStyle = '#a0a0a0';
    ctx.lineWidth = 1;
    ctx.strokeRect(previewX + 0.5, previewY + 0.5, previewBoxW, previewBoxH);

    // チェッカーボード背景
    this._drawCheckerboard(ctx, previewX + 1, previewY + 1, previewBoxW - 1, previewBoxH - 1, 6);

    // プレビュー描画
    const preview = this._getPreview();
    if (preview && preview.pixels) {
      // プレビュー領域にフィットさせる
      const scale = Math.min(
        (previewBoxW - 4) / preview.width,
        (previewBoxH - 4) / preview.height,
        4 // 最大4倍
      );
      const drawW = Math.floor(preview.width * scale);
      const drawH = Math.floor(preview.height * scale);
      const drawX = previewX + Math.floor((previewBoxW - drawW) / 2);
      const drawY = previewY + Math.floor((previewBoxH - drawH) / 2);

      this._drawPixelDataScaled(ctx, preview, drawX, drawY, scale);
    }

    // ラベル
    ctx.fillStyle = '#555555'; ctx.font = '10px sans-serif'; ctx.textBaseline = 'top';
    ctx.textAlign = 'center';
    ctx.fillText('プレビュー', previewX + previewBoxW / 2, previewY + previewBoxH + 4);
    ctx.textAlign = 'left'; ctx.textBaseline = 'alphabetic';

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
      this._commitInput();
      return;
    }

    // プリセットボタン
    const preset = this._rects._presets?.find(p => p.key === key);
    if (preset) {
      this._w = preset.preset.w;
      this._wallHeight = preset.preset.wallHeight;
      this._focusedField = null;
      this._inputBuf = '';
      this._markPreviewDirty();
      return;
    }

    // タイプ選択
    if (key === 'type_ground') { this._setType('ground'); return; }
    if (key === 'type_box')    { this._setType('box');    return; }

    // 幅スピナー
    if (key === 'w_dec') { this._w = Math.max(QuarterViewTileDialog.W_MIN, this._w - QuarterViewTileDialog.W_STEP); this._markPreviewDirty(); return; }
    if (key === 'w_inc') { this._w = Math.min(QuarterViewTileDialog.W_MAX, this._w + QuarterViewTileDialog.W_STEP); this._markPreviewDirty(); return; }
    if (key === 'w_field') { this._focusedField = 'width'; this._inputBuf = ''; return; }

    // 壁高さスピナー
    if (key === 'wh_dec') { this._wallHeight = Math.max(QuarterViewTileDialog.WALL_MIN, this._wallHeight - 1); this._markPreviewDirty(); return; }
    if (key === 'wh_inc') { this._wallHeight = Math.min(QuarterViewTileDialog.WALL_MAX, this._wallHeight + 1); this._markPreviewDirty(); return; }
    if (key === 'wh_field') { this._focusedField = 'wallHeight'; this._inputBuf = ''; return; }

    // レイヤー分け
    if (key === 'chk_layers') { this._separateLayers = !this._separateLayers; return; }

    // 色スウォッチ
    if (key === 'color_top')     { this._openColorPicker('_topColor');     return; }
    if (key === 'color_left')    { this._openColorPicker('_leftColor');    return; }
    if (key === 'color_right')   { this._openColorPicker('_rightColor');   return; }
    if (key === 'color_outline') { this._openColorPicker('_outlineColor'); return; }

    // 透明化チェック
    if (key === 'chk_top_tr')   { this._topTransparent   = !this._topTransparent;   this._markPreviewDirty(); return; }
    if (key === 'chk_left_tr')  { this._leftTransparent  = !this._leftTransparent;  this._markPreviewDirty(); return; }
    if (key === 'chk_right_tr') { this._rightTransparent = !this._rightTransparent; this._markPreviewDirty(); return; }

    // 適用先
    if (key === 'target_new')       { this._target = 'new';       return; }
    if (key === 'target_overwrite') { this._target = 'overwrite'; return; }

    // ボタン
    if (key === 'btnCancel') { this.hide(); this.onCancel(); return; }
    if (key === 'btnOK')     { this._confirm(); return; }
  }

  onMouseUpBody(e) {}

  onKeyDownBody(e) {
    if (e.key === 'Escape') {
      if (this._focusedField) {
        this._focusedField = null;
        this._inputBuf     = '';
      } else {
        this.hide();
        this.onCancel();
      }
      return;
    }
    if (e.key === 'Enter') {
      if (this._focusedField) {
        this._commitInput();
      } else {
        this._confirm();
      }
      return;
    }

    if (!this._focusedField) return;

    if (e.key === 'Tab') {
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

  // ----------------------------------------------------------------
  // 内部ロジック
  // ----------------------------------------------------------------

  _setType(type) {
    if (this._type === type) return;
    this._type = type;
    // タイプ変更時にプリセットのデフォルトを選択
    if (type === 'ground') {
      this._w = 32;
      this._wallHeight = 0;
    } else {
      this._w = 32;
      this._wallHeight = 16;
    }
    this._focusedField = null;
    this._inputBuf = '';
    this._markPreviewDirty();
  }

  _commitInput() {
    if (!this._focusedField || this._inputBuf === '') {
      this._focusedField = null;
      return;
    }
    const v = parseInt(this._inputBuf, 10) || 0;
    if (this._focusedField === 'width') {
      // 4の倍数に丸める
      const clamped = Math.max(QuarterViewTileDialog.W_MIN,
                               Math.min(QuarterViewTileDialog.W_MAX, v));
      this._w = Math.round(clamped / 4) * 4;
      if (this._w < QuarterViewTileDialog.W_MIN) this._w = QuarterViewTileDialog.W_MIN;
    } else if (this._focusedField === 'wallHeight') {
      this._wallHeight = Math.max(QuarterViewTileDialog.WALL_MIN,
                                  Math.min(QuarterViewTileDialog.WALL_MAX, v));
    }
    this._focusedField = null;
    this._inputBuf = '';
    this._markPreviewDirty();
  }

  _confirm() {
    this._commitInput();
    this.hide();
    this.onConfirm(this._buildParams());
  }

  /**
   * 色変更リクエストを発行する。
   * EditorScene 側で ColorPickerDialog を開くためのコールバックを呼ぶ。
   * @param {string} field - '_topColor','_leftColor','_rightColor','_outlineColor'
   */
  _openColorPicker(field) {
    if (this._colorPickerCallback) {
      // コールバック準備: EditorScene 側で onColorSwatchClick を参照
    }
    if (this.onColorSwatchClick) {
      this.onColorSwatchClick(this[field], (color) => {
        this[field] = color;
        this._markPreviewDirty();
      });
    }
  }

  // ----------------------------------------------------------------
  // ヒットテスト
  // ----------------------------------------------------------------

  _hitTest(x, y) {
    // プリセット
    for (const p of (this._rects._presets ?? [])) {
      if (this._inRect(x, y, p)) return p.key;
    }
    // その他
    for (const [key, r] of Object.entries(this._rects)) {
      if (key === '_presets') continue;
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

  _drawPresetButton(ctx, x, y, w, h, text, selected, hovered) {
    const bg = selected ? '#0078d7' : (hovered ? '#dcdad5' : '#e8e6e1');
    const fg = selected ? '#ffffff' : '#000000';
    const border = selected ? '#005a9e' : '#a0a0a0';

    ctx.fillStyle   = bg;
    ctx.strokeStyle = border;
    ctx.lineWidth   = 1;
    this._roundRect(ctx, x, y, w, h, 3);
    ctx.fill();
    ctx.stroke();

    ctx.fillStyle    = fg;
    ctx.font         = '11px sans-serif';
    ctx.textBaseline = 'middle';
    ctx.textAlign    = 'center';
    ctx.fillText(text, x + w / 2, y + h / 2);
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

    ctx.strokeStyle = checked ? '#0078d7' : '#888880';
    ctx.lineWidth   = checked ? 2 : 1;
    ctx.beginPath();
    ctx.arc(cx, cy, r, 0, Math.PI * 2);
    ctx.stroke();

    if (checked) {
      ctx.fillStyle = '#0078d7';
      ctx.beginPath();
      ctx.arc(cx, cy, 4, 0, Math.PI * 2);
      ctx.fill();
    }

    ctx.fillStyle    = hovered ? '#0078d7' : '#222222';
    ctx.font         = '13px sans-serif';
    ctx.textBaseline = 'middle';
    ctx.fillText(text, cx + r + 6, cy);
    ctx.textBaseline = 'alphabetic';
  }

  _drawCheckbox(ctx, x, y, text, checked, hovered) {
    const boxSize = 14;
    const bx = x;
    const by = y + 2;

    ctx.fillStyle   = checked ? '#0078d7' : '#ffffff';
    ctx.strokeStyle = checked ? '#005a9e' : '#888880';
    ctx.lineWidth   = 1;
    ctx.fillRect(bx, by, boxSize, boxSize);
    ctx.strokeRect(bx + 0.5, by + 0.5, boxSize - 1, boxSize - 1);

    if (checked) {
      // チェックマーク
      ctx.strokeStyle = '#ffffff';
      ctx.lineWidth   = 2;
      ctx.beginPath();
      ctx.moveTo(bx + 3, by + 7);
      ctx.lineTo(bx + 6, by + 10);
      ctx.lineTo(bx + 11, by + 4);
      ctx.stroke();
    }

    ctx.fillStyle    = hovered ? '#0078d7' : '#222222';
    ctx.font         = '12px sans-serif';
    ctx.textBaseline = 'middle';
    ctx.fillText(text, bx + boxSize + 6, by + boxSize / 2);
    ctx.textBaseline = 'alphabetic';
  }

  _drawSwatch(ctx, x, y, size, color, transparent, hovered) {
    // チェッカーボード背景（透明度表示用）
    this._drawCheckerboard(ctx, x, y, size, size, 4);

    // 色を描画
    if (!transparent) {
      ctx.fillStyle = PixelData.toCssColor(color);
      ctx.fillRect(x, y, size, size);
    }

    // 枠
    ctx.strokeStyle = hovered ? '#0078d7' : '#666666';
    ctx.lineWidth   = hovered ? 2 : 1;
    ctx.strokeRect(x + 0.5, y + 0.5, size - 1, size - 1);
  }

  _drawCheckerboard(ctx, x, y, w, h, cellSize) {
    ctx.save();
    ctx.beginPath();
    ctx.rect(x, y, w, h);
    ctx.clip();
    for (let cy = 0; cy < h; cy += cellSize) {
      for (let cx = 0; cx < w; cx += cellSize) {
        const dark = ((Math.floor(cx / cellSize) + Math.floor(cy / cellSize)) % 2) === 1;
        ctx.fillStyle = dark ? '#c0c0c0' : '#ffffff';
        ctx.fillRect(x + cx, y + cy, cellSize, cellSize);
      }
    }
    ctx.restore();
  }

  /**
   * PixelData を拡大描画する
   * @param {CanvasRenderingContext2D} ctx
   * @param {PixelData} pd
   * @param {number} drawX
   * @param {number} drawY
   * @param {number} scale
   */
  _drawPixelDataScaled(ctx, pd, drawX, drawY, scale) {
    const s = Math.max(1, Math.floor(scale));
    for (let py = 0; py < pd.height; py++) {
      for (let px = 0; px < pd.width; px++) {
        const c = pd.getPixel(px, py);
        if ((c >>> 24) === 0) continue; // 完全透明はスキップ
        ctx.fillStyle = PixelData.toCssColor(c);
        ctx.fillRect(drawX + px * s, drawY + py * s, s, s);
      }
    }
  }
}
