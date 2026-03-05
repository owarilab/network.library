/**
 * DialogBase
 * canvas 上にモーダルダイアログを描画する基底クラス。
 * サブクラスは renderBody() と各入力ハンドラをオーバーライドして実装する。
 */
class DialogBase {
  // スタイル定数
  static OVERLAY_COLOR   = 'rgba(0,0,0,0.45)';
  static BG_COLOR        = '#f0eeea';
  static BORDER_COLOR    = '#888880';
  static TITLE_BG        = '#0078d7';
  static TITLE_TEXT      = '#ffffff';
  static TITLE_HEIGHT    = 28;
  static TITLE_FONT      = 'bold 13px sans-serif';
  static RADIUS          = 4;
  static SHADOW_COLOR    = 'rgba(0,0,0,0.35)';
  static SHADOW_BLUR     = 12;
  static SHADOW_OFFSET_Y = 4;

  /**
   * @param {string} title    - タイトルバーのテキスト
   * @param {number} dialogW  - ダイアログ幅 (px)
   * @param {number} dialogH  - ダイアログ高さ (px)
   */
  constructor(title, dialogW, dialogH) {
    this._title   = title;
    this._dialogW = dialogW;
    this._dialogH = dialogH;
    this._visible = false;

    /** ダイアログ左上座標 (render で canvas 中央に算出) */
    this._dx = 0;
    this._dy = 0;
  }

  // ----------------------------------------------------------------
  // 表示制御
  // ----------------------------------------------------------------

  get isVisible() { return this._visible; }

  show() { this._visible = true;  }
  hide() { this._visible = false; }

  // ----------------------------------------------------------------
  // 描画
  // ----------------------------------------------------------------

  /**
   * ダイアログ全体を描画する。
   * EditorScene の render() 末尾（MenuBar より前）で呼ぶこと。
   * @param {CanvasRenderingContext2D} ctx
   * @param {HTMLCanvasElement}       canvas
   */
  render(ctx, canvas) {
    if (!this._visible) return;

    const {
      OVERLAY_COLOR, BG_COLOR, BORDER_COLOR,
      TITLE_BG, TITLE_TEXT, TITLE_HEIGHT, TITLE_FONT,
      RADIUS, SHADOW_COLOR, SHADOW_BLUR, SHADOW_OFFSET_Y,
    } = DialogBase;

    // ---- canvas 中央に配置 ----
    this._dx = Math.round((canvas.width  - this._dialogW) / 2);
    this._dy = Math.round((canvas.height - this._dialogH) / 2);
    const { _dx: dx, _dy: dy, _dialogW: dw, _dialogH: dh } = this;

    ctx.save();

    // ---- 半透明オーバーレイ ----
    ctx.fillStyle = OVERLAY_COLOR;
    ctx.fillRect(0, 0, canvas.width, canvas.height);

    // ---- 影 ----
    ctx.shadowColor   = SHADOW_COLOR;
    ctx.shadowBlur    = SHADOW_BLUR;
    ctx.shadowOffsetY = SHADOW_OFFSET_Y;

    // ---- ダイアログ背景（角丸） ----
    this._roundRect(ctx, dx, dy, dw, dh, RADIUS);
    ctx.fillStyle = BG_COLOR;
    ctx.fill();

    ctx.shadowColor = 'transparent';
    ctx.shadowBlur  = 0;
    ctx.shadowOffsetY = 0;

    // ---- 枠線 ----
    ctx.strokeStyle = BORDER_COLOR;
    ctx.lineWidth   = 1;
    this._roundRect(ctx, dx + 0.5, dy + 0.5, dw - 1, dh - 1, RADIUS);
    ctx.stroke();

    // ---- タイトルバー ----
    this._roundRectTop(ctx, dx, dy, dw, TITLE_HEIGHT, RADIUS);
    ctx.fillStyle = TITLE_BG;
    ctx.fill();

    ctx.fillStyle  = TITLE_TEXT;
    ctx.font       = TITLE_FONT;
    ctx.textBaseline = 'middle';
    ctx.fillText(this._title, dx + 12, dy + TITLE_HEIGHT / 2);
    ctx.textBaseline = 'alphabetic';

    // ---- ボディ（サブクラスが描画） ----
    const bodyY = dy + TITLE_HEIGHT;
    const bodyH = dh - TITLE_HEIGHT;
    this.renderBody(ctx, dx, bodyY, dw, bodyH);

    ctx.restore();
  }

  /**
   * ダイアログ本体を描画する。サブクラスでオーバーライドする。
   * @param {CanvasRenderingContext2D} ctx
   * @param {number} x       - ボディ左上 X
   * @param {number} y       - ボディ左上 Y
   * @param {number} width   - ボディ幅
   * @param {number} height  - ボディ高さ
   */
  renderBody(ctx, x, y, width, height) {}

  // ----------------------------------------------------------------
  // 入力ハンドラ (EditorScene から呼ぶ)
  // ----------------------------------------------------------------

  /** @param {{ x:number, y:number }} e */
  onMouseMove(e) {
    if (!this._visible) return;
    this.onMouseMoveBody(e, this._dx, this._dy + DialogBase.TITLE_HEIGHT);
  }

  /** @param {{ x:number, y:number, button:number }} e */
  onMouseDown(e) {
    if (!this._visible) return;
    this.onMouseDownBody(e, this._dx, this._dy + DialogBase.TITLE_HEIGHT);
  }

  /** @param {{ x:number, y:number }} e */
  onMouseUp(e) {
    if (!this._visible) return;
    this.onMouseUpBody(e, this._dx, this._dy + DialogBase.TITLE_HEIGHT);
  }

  /** @param {{ key:string, code:string }} e */
  onKeyDown(e) {
    if (!this._visible) return;
    this.onKeyDownBody(e);
  }

  // サブクラス向けオーバーライドポイント
  onMouseMoveBody(e, dx, bodyY) {}
  onMouseDownBody(e, dx, bodyY) {}
  onMouseUpBody(e, dx, bodyY)   {}
  onKeyDownBody(e)               {}

  // ----------------------------------------------------------------
  // ユーティリティ
  // ----------------------------------------------------------------

  /** 角丸矩形パスを作成 */
  _roundRect(ctx, x, y, w, h, r) {
    ctx.beginPath();
    ctx.moveTo(x + r, y);
    ctx.lineTo(x + w - r, y);
    ctx.arcTo(x + w, y,     x + w, y + r,     r);
    ctx.lineTo(x + w, y + h - r);
    ctx.arcTo(x + w, y + h, x + w - r, y + h, r);
    ctx.lineTo(x + r, y + h);
    ctx.arcTo(x,     y + h, x,     y + h - r, r);
    ctx.lineTo(x,     y + r);
    ctx.arcTo(x,     y,     x + r, y,         r);
    ctx.closePath();
  }

  /** 上だけ角丸の矩形パス（タイトルバー用） */
  _roundRectTop(ctx, x, y, w, h, r) {
    ctx.beginPath();
    ctx.moveTo(x + r, y);
    ctx.lineTo(x + w - r, y);
    ctx.arcTo(x + w, y, x + w, y + r, r);
    ctx.lineTo(x + w, y + h);
    ctx.lineTo(x,     y + h);
    ctx.lineTo(x,     y + r);
    ctx.arcTo(x, y, x + r, y, r);
    ctx.closePath();
  }

  /**
   * ボタン風の矩形を描画するユーティリティ
   * @param {CanvasRenderingContext2D} ctx
   * @param {number} x
   * @param {number} y
   * @param {number} w
   * @param {number} h
   * @param {string} label
   * @param {boolean} [hovered=false]
   * @param {boolean} [active=false]    - プライマリボタン（青）
   */
  _drawButton(ctx, x, y, w, h, label, hovered = false, active = false) {
    const bg = active
      ? (hovered ? '#005a9e' : '#0078d7')
      : (hovered ? '#dcdad5' : '#e8e6e1');
    const fg = active ? '#ffffff' : '#000000';
    const border = active ? '#005a9e' : '#a0a0a0';

    ctx.fillStyle   = bg;
    ctx.strokeStyle = border;
    ctx.lineWidth   = 1;
    this._roundRect(ctx, x, y, w, h, 3);
    ctx.fill();
    ctx.stroke();

    ctx.fillStyle    = fg;
    ctx.font         = '13px sans-serif';
    ctx.textBaseline = 'middle';
    ctx.textAlign    = 'center';
    ctx.fillText(label, x + w / 2, y + h / 2);
    ctx.textAlign    = 'left';
    ctx.textBaseline = 'alphabetic';
  }
}
