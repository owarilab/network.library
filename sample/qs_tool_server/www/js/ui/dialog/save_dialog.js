/**
 * SaveDialog
 * エクスポートダイアログ。
 * DialogBase を継承し、canvas 上にモーダルで描画する。
 *
 * 機能:
 *   - ファイル名入力フィールド（キーボード入力対応）
 *   - エクスポート形式選択: PNG / JSON (ラジオボタン)
 *   - キャンセル / エクスポート ボタン
 *
 * 使用例:
 *   const dialog = new SaveDialog((filename, format) => {
 *     if (format === 'png') PixelDataConverter.exportAsPng(pixelData, filename);
 *     else                  PixelDataConverter.exportAsJson(pixelData, filename);
 *   });
 *   dialog.show();
 */
class SaveDialog extends DialogBase {
  static DIALOG_W = 380;
  static DIALOG_H = 240;

  /** ファイル名に使えない文字 (Windows/Unix 共通で安全な範囲) */
  static FORBIDDEN_CHARS = /[\\/:*?"<>|]/;

  /** ファイル名最大文字数 */
  static MAX_FILENAME_LEN = 80;

  /**
   * @param {(filename: string, format: 'png' | 'json' | 'qts') => void} onConfirm
   * @param {() => void} [onCancel]
   */
  constructor(onConfirm, onCancel = () => {}) {
    super('エクスポート', SaveDialog.DIALOG_W, SaveDialog.DIALOG_H);

    /** @type {(filename: string, format: 'png' | 'json' | 'qts') => void} */
    this.onConfirm = onConfirm;
    /** @type {() => void} */
    this.onCancel  = onCancel;

    /** 現在のファイル名ベース (拡張子なし) @type {string} */
    this._filename = 'pixel_art';
    /** エクスポート形式 @type {'png' | 'json' | 'qts'} */
    this._format   = 'qts';

    /** エクスポート対象コンテキスト @type {'free' | 'tileset'} */
    this._exportContext = 'free';

    /** ファイル名フィールドにフォーカスがあるか @type {boolean} */
    this._filenameFocused = false;
    /** キーボード入力バッファ @type {string} */
    this._inputBuf = '';

    /** ホバー中の要素キー @type {string|null} */
    this._hover = null;

    /**
     * 各 UI 要素の描画座標 (renderBody で毎フレーム更新)
     * @type {Object.<string, {x:number, y:number, w:number, h:number}>}
     */
    this._rects = {};
  }

  // ----------------------------------------------------------------
  // DialogBase オーバーライド
  // ----------------------------------------------------------------

  /**
   * @param {CanvasRenderingContext2D} ctx
   * @param {number} bx  - ボディ左上 X
   * @param {number} by  - ボディ左上 Y
   * @param {number} bw  - ボディ幅
   * @param {number} bh  - ボディ高さ
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

    const LABEL_COL_W = 80;   // "ファイル名:" 列幅

    let y = by + 16;

    // =====================================================
    // セクション 1: ファイル名
    // =====================================================
    label('ファイル名:', bx + PAD, y + 14);

    const fieldX = bx + PAD + LABEL_COL_W;
    const fieldW = bw - PAD * 2 - LABEL_COL_W;
    const fieldH = 28;
    const fieldR = { x: fieldX, y, w: fieldW, h: fieldH };
    this._rects.filenameField = fieldR;

    const displayText = this._filenameFocused
      ? (this._inputBuf !== '' ? this._inputBuf : '')
      : this._filename;
    this._drawTextField(ctx, fieldR, displayText, this._filenameFocused);

    y += fieldH + 16;

    // =====================================================
    // セクション 2: 形式選択
    // =====================================================
    label('形式:', bx + PAD, y + 11);

    const radioX   = bx + PAD + 48;
    const radioGap = 80;

    const fmtPngR  = { x: radioX,               y, w: 70, h: 22 };
    const fmtJsonR = { x: radioX + radioGap,     y, w: 80, h: 22 };
    const fmtQtsR  = { x: radioX + radioGap * 2, y, w: 80, h: 22 };
    this._rects.fmtPng  = fmtPngR;
    this._rects.fmtJson = fmtJsonR;
    this._rects.fmtQts  = fmtQtsR;

    this._drawRadio(ctx, fmtPngR.x,  fmtPngR.y,  'PNG',
                    this._format === 'png',
                    this._hover  === 'fmtPng');
    this._drawRadio(ctx, fmtJsonR.x, fmtJsonR.y, 'JSON',
                    this._format === 'json',
                    this._hover  === 'fmtJson');
    this._drawRadio(ctx, fmtQtsR.x,  fmtQtsR.y,  'QTS',
                    this._format === 'qts',
                    this._hover  === 'fmtQts');

    y += 22 + 6;

    // 現在のファイル名+拡張子を次の行に表示
    const extMap = { png: '.png', json: '.json', qts: '.qts' };
    const ext = extMap[this._format] || '.png';
    ctx.fillStyle    = '#888888';
    ctx.font         = '11px sans-serif';
    ctx.textBaseline = 'middle';
    ctx.fillText(`→ ${this._filename}${ext}`, bx + PAD + LABEL_COL_W, y + 6);
    ctx.textBaseline = 'alphabetic';

    y += 16;

    if (this._format === 'qts') {
      ctx.fillStyle = '#666666';
      ctx.font = '11px sans-serif';
      ctx.textBaseline = 'middle';
      const note = this._exportContext === 'free'
        ? '通常ドット絵は 1x1 タイルセットとして保存されます'
        : 'QTS はタイルセットとパレット情報を保存します';
      ctx.fillText(note, bx + PAD + LABEL_COL_W, y + 6);
      ctx.textBaseline = 'alphabetic';
      y += 14;
    }

    // =====================================================
    // 区切り線
    // =====================================================
    y += 4;
    ctx.strokeStyle = '#c0bdb8';
    ctx.lineWidth   = 1;
    ctx.beginPath();
    ctx.moveTo(bx + PAD,      y + 0.5);
    ctx.lineTo(bx + bw - PAD, y + 0.5);
    ctx.stroke();
    y += 14;

    // =====================================================
    // ボタン行: キャンセル / エクスポート
    // =====================================================
    const btnW = 90;
    const btnH = 28;
    const cancelR = { x: bx + bw - PAD - btnW * 2 - 10, y, w: btnW, h: btnH };
    const okR     = { x: bx + bw - PAD - btnW,           y, w: btnW, h: btnH };
    this._rects.btnCancel = cancelR;
    this._rects.btnOK     = okR;

    this._drawButton(ctx, cancelR.x, cancelR.y, cancelR.w, cancelR.h,
                     'キャンセル', this._hover === 'btnCancel');
    this._drawButton(ctx, okR.x,     okR.y,     okR.w,     okR.h,
                     'エクスポート', this._hover === 'btnOK', true);

    ctx.restore();
  }

  // ----------------------------------------------------------------
  // 入力ハンドラ (DialogBase オーバーライド)
  // ----------------------------------------------------------------

  onMouseMoveBody(e) {
    this._hover = this._hitTest(e.x, e.y);
  }

  onMouseDownBody(e) {
    if (e.button !== 0) return;
    const key = this._hitTest(e.x, e.y);

    // ---- フォーカス解除 (フィールド外クリック) ----
    if (!key) {
      this._commitFilename();
      return;
    }

    if (key === 'filenameField') {
      // フォーカスON; バッファを現在のファイル名で初期化
      this._filenameFocused = true;
      this._inputBuf        = this._filename;
      return;
    }
    if (key === 'fmtPng')  { this._format = 'png';  this._commitFilename(); return; }
    if (key === 'fmtJson') { this._format = 'json'; this._commitFilename(); return; }
    if (key === 'fmtQts')  { this._format = 'qts';  this._commitFilename(); return; }
    if (key === 'btnCancel') { this._cancel(); return; }
    if (key === 'btnOK')     { this._confirm(); return; }
  }

  onMouseUpBody(e) {}

  onKeyDownBody(e) {
    if (!this._filenameFocused) {
      // フォーカスなし: Enter でエクスポート、Escape でキャンセル
      if (e.key === 'Enter')  { this._confirm(); return; }
      if (e.key === 'Escape') { this._cancel();  return; }
      return;
    }

    // ---- ファイル名フィールドにフォーカスあり ----
    if (e.key === 'Escape') {
      // 編集キャンセル → 元の名前に戻す
      this._filenameFocused = false;
      this._inputBuf        = '';
      return;
    }
    if (e.key === 'Enter' || e.key === 'Tab') {
      this._commitFilename();
      return;
    }
    if (e.key === 'Backspace') {
      this._inputBuf = this._inputBuf.slice(0, -1);
      return;
    }

    // 印刷可能な1文字 かつ禁止文字でなければ追加
    if (
      e.key.length === 1 &&
      !SaveDialog.FORBIDDEN_CHARS.test(e.key) &&
      this._inputBuf.length < SaveDialog.MAX_FILENAME_LEN
    ) {
      this._inputBuf += e.key;
    }
  }

  // ----------------------------------------------------------------
  // 内部ロジック
  // ----------------------------------------------------------------

  /** ファイル名入力を確定する */
  _commitFilename() {
    if (this._filenameFocused) {
      const trimmed = this._inputBuf.trim();
      if (trimmed.length > 0) this._filename = trimmed;
    }
    this._filenameFocused = false;
    this._inputBuf        = '';
  }

  /**
   * ダイアログ表示前にエクスポート対象コンテキストを設定する。
   * @param {'free' | 'tileset'} context
   */
  setExportContext(context) {
    this._exportContext = context === 'tileset' ? 'tileset' : 'free';
  }

  /** エクスポートボタン押下 */
  _confirm() {
    this._commitFilename();
    const name = (this._filename.trim() || 'pixel_art');
    const extMap = { png: '.png', json: '.json', qts: '.qts' };
    const ext  = extMap[this._format] || '.png';
    this.hide();
    this.onConfirm(name + ext, this._format);
  }

  /** キャンセルボタン押下 */
  _cancel() {
    this._commitFilename();
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
    for (const [key, r] of Object.entries(this._rects)) {
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

  /**
   * テキスト入力フィールドを描画する。
   * テキストが長い場合は末尾側を表示するようにクリッピングする。
   * @param {CanvasRenderingContext2D} ctx
   * @param {{x:number,y:number,w:number,h:number}} r
   * @param {string}  text
   * @param {boolean} focused
   */
  _drawTextField(ctx, r, text, focused) {
    // 背景
    ctx.fillStyle   = focused ? '#ffffff' : '#f8f8f6';
    ctx.strokeStyle = focused ? '#0078d7' : '#a0a0a0';
    ctx.lineWidth   = focused ? 2 : 1;
    this._roundRect(ctx, r.x, r.y, r.w, r.h, 3);
    ctx.fill();
    ctx.stroke();

    // テキスト (はみ出しをクリップ)
    ctx.save();
    ctx.beginPath();
    ctx.rect(r.x + 4, r.y + 1, r.w - 8, r.h - 2);
    ctx.clip();

    ctx.fillStyle    = text.length > 0 ? '#000000' : '#aaaaaa';
    ctx.font         = '13px sans-serif';
    ctx.textBaseline = 'middle';
    ctx.textAlign    = 'left';

    const PAD_L = 8;
    const displayStr = text.length > 0 ? text : (focused ? '' : '(ファイル名を入力)');

    // 長いテキストは末尾を右端に揃えて表示
    const tw = ctx.measureText(displayStr).width;
    const availW = r.w - PAD_L * 2;
    const textX  = tw > availW
      ? r.x + r.w - PAD_L - tw   // 末尾寄せ
      : r.x + PAD_L;              // 先頭寄せ

    ctx.fillText(displayStr, textX, r.y + r.h / 2);

    // フォーカス中のカーソル
    if (focused) {
      const cursorX = Math.min(textX + tw + 2, r.x + r.w - PAD_L);
      ctx.fillStyle = '#000000';
      ctx.fillRect(cursorX, r.y + r.h / 2 - 7, 1, 14);
    }

    ctx.textAlign    = 'left';
    ctx.textBaseline = 'alphabetic';
    ctx.restore();
  }

  /**
   * ラジオボタンを描画する。
   * @param {CanvasRenderingContext2D} ctx
   * @param {number}  x
   * @param {number}  y
   * @param {string}  text
   * @param {boolean} checked
   * @param {boolean} hovered
   */
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
