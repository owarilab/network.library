/**
 * UIWindow
 * canvas 上に配置できる「ウィンドウ」の共通基底クラス。
 *
 * ・タイトルバーをドラッグして自由に移動できる
 * ・ウィンドウ枠・背景・影をこのクラスが描画し、
 *   コンテンツ描画はサブクラスの renderContent() に委譲する
 *
 * サブクラスが override すべきメソッド:
 *   getWindowTitle()            → string
 *   getContentSize()            → { w: number, h: number }
 *   renderContent(ctx, cx, cy, cw, ch, appData)
 *   onContentMouseMove(e, appData)
 *   onContentMouseDown(e, appData) → boolean  (true = consumed)
 *   onContentMouseUp(e, appData)
 *
 * EditorScene からの呼び出し方:
 *   window.render(ctx, canvas, appData);
 *   window.onMouseMove(e, appData);
 *   const consumed = window.onMouseDown(e, appData);   // false なら素通し
 *   window.onMouseUp(e, appData);
 */
class UIWindow {
  // ---- スタイル定数 ----
  static TITLE_H       = 18;
  static BG_COLOR      = 'rgba(40,40,40,0.93)';
  static BORDER_COLOR  = '#606060';
  static TITLE_BG      = '#1e3050';
  static TITLE_BG_DRAG = '#2855a0';
  static TITLE_TEXT    = '#e0e0e0';
  static TITLE_FONT    = 'bold 11px sans-serif';
  static RADIUS        = 4;
  static SHADOW_COLOR  = 'rgba(0,0,0,0.55)';
  static SHADOW_BLUR   = 10;
  static SHADOW_OFF_Y  = 3;
  /** ウィンドウがキャンバス内に留まるための最小余白 */
  static EDGE_MARGIN   = 4;

  /**
   * @param {number} [initX=8]
   * @param {number} [initY]   省略時は MenuBar.HEIGHT + 8
   */
  constructor(initX = 8, initY = null) {
    /** ウィンドウ左上座標 */
    this._x = initX;
    this._y = initY !== null ? initY : (UIWindow._menuBarHeight() + 8);

    /** ドラッグ中フラグ */
    this._isDragging = false;
    /** ドラッグ開始時のマウス→ウィンドウ左上オフセット */
    this._dragOffX = 0;
    this._dragOffY = 0;

    /**
     * render() 時にキャンバスサイズをキャッシュし、
     * onMouseMove 中のクランプに使う
     */
    this._canvasW = 0;
    this._canvasH = 0;
  }

  // ----------------------------------------------------------------
  // サブクラスが override するメソッド
  // ----------------------------------------------------------------

  /** ウィンドウタイトルバーに表示する文字列 */
  getWindowTitle() { return ''; }

  /**
   * コンテンツ領域のサイズを返す。
   * ウィンドウ全体サイズ = {w: this.getContentSize().w, h: TITLE_H + this.getContentSize().h}
   * @returns {{ w: number, h: number }}
   */
  getContentSize() { return { w: 60, h: 60 }; }

  /**
   * コンテンツ領域を描画する。サブクラスでオーバーライド。
   * @param {CanvasRenderingContext2D} ctx
   * @param {number} cx       コンテンツ左上 X (スクリーン座標)
   * @param {number} cy       コンテンツ左上 Y (スクリーン座標)
   * @param {number} cw       コンテンツ幅
   * @param {number} ch       コンテンツ高
   * @param {AppData} appData
   */
  renderContent(ctx, cx, cy, cw, ch, appData) {}

  /** コンテンツ領域へのマウス移動通知 */
  onContentMouseMove(e, appData) {}

  /**
   * コンテンツ領域のマウスダウン通知。
   * @returns {boolean} true = イベントを消費した
   */
  onContentMouseDown(e, appData) { return false; }

  /** コンテンツ領域のマウスアップ通知 */
  onContentMouseUp(e, appData) {}

  // ----------------------------------------------------------------
  // 公開 API（EditorScene から呼ぶ）
  // ----------------------------------------------------------------

  /**
   * ウィンドウ全体を描画する。
   * @param {CanvasRenderingContext2D} ctx
   * @param {HTMLCanvasElement}       canvas
   * @param {AppData}                 appData
   */
  render(ctx, canvas, appData) {
    this._canvasW = canvas.width;
    this._canvasH = canvas.height;

    // 位置をキャンバス内にクランプ
    this._clamp();

    const { TITLE_H, BG_COLOR, BORDER_COLOR,
            TITLE_BG, TITLE_BG_DRAG, TITLE_TEXT, TITLE_FONT,
            RADIUS, SHADOW_COLOR, SHADOW_BLUR, SHADOW_OFF_Y } = UIWindow;

    const { w: cw, h: ch } = this.getContentSize();
    const ww = cw;                  // ウィンドウ幅 = コンテンツ幅
    const wh = TITLE_H + ch;       // ウィンドウ高
    const wx = this._x;
    const wy = this._y;

    ctx.save();

    // ---- 影 ----
    ctx.shadowColor   = SHADOW_COLOR;
    ctx.shadowBlur    = SHADOW_BLUR;
    ctx.shadowOffsetY = SHADOW_OFF_Y;

    // ---- 背景 ----
    ctx.fillStyle = BG_COLOR;
    this._roundRect(ctx, wx, wy, ww, wh, RADIUS);
    ctx.fill();

    ctx.shadowColor = 'transparent';
    ctx.shadowBlur  = 0;
    ctx.shadowOffsetY = 0;

    // ---- 枠線 ----
    ctx.strokeStyle = BORDER_COLOR;
    ctx.lineWidth   = 1;
    this._roundRect(ctx, wx + 0.5, wy + 0.5, ww - 1, wh - 1, RADIUS);
    ctx.stroke();

    // ---- タイトルバー ----
    const titleColor = this._isDragging ? TITLE_BG_DRAG : TITLE_BG;
    this._roundRectTop(ctx, wx, wy, ww, TITLE_H, RADIUS);
    ctx.fillStyle = titleColor;
    ctx.fill();

    // タイトルバー下端セパレータ
    ctx.strokeStyle = BORDER_COLOR;
    ctx.lineWidth   = 1;
    ctx.beginPath();
    ctx.moveTo(wx,      wy + TITLE_H - 0.5);
    ctx.lineTo(wx + ww, wy + TITLE_H - 0.5);
    ctx.stroke();

    // タイトルテキスト
    ctx.font         = TITLE_FONT;
    ctx.fillStyle    = TITLE_TEXT;
    ctx.textAlign    = 'left';
    ctx.textBaseline = 'middle';
    ctx.fillText(this.getWindowTitle(), wx + 8, wy + TITLE_H / 2);

    ctx.restore();

    // ---- コンテンツ ----
    const cx = wx;
    const cy = wy + TITLE_H;
    this.renderContent(ctx, cx, cy, cw, ch, appData);
  }

  /**
   * マウス移動。ドラッグ処理 + コンテンツへの委譲。
   * @param {{x: number, y: number}} e
   * @param {AppData} [appData]
   */
  onMouseMove(e, appData) {
    if (this._isDragging) {
      this._x = e.x - this._dragOffX;
      this._y = e.y - this._dragOffY;
      this._clamp();
      return; // ドラッグ中はコンテンツへ伝えない
    }
    this.onContentMouseMove(e, appData);
  }

  /**
   * マウスダウン。タイトルバーならドラッグ開始、コンテンツなら委譲。
   * ウィンドウ外なら false を返す。
   * @param {{x: number, y: number, button: number}} e
   * @param {AppData} [appData]
   * @returns {boolean}
   */
  onMouseDown(e, appData) {
    if (!this.containsPoint(e.x, e.y)) return false;

    if (this._inTitleBar(e.x, e.y)) {
      this._isDragging = true;
      this._dragOffX   = e.x - this._x;
      this._dragOffY   = e.y - this._y;
      return true;
    }

    return this.onContentMouseDown(e, appData);
  }

  /**
   * マウスアップ。ドラッグ終了 + コンテンツへの委譲。
   * @param {{x: number, y: number}} e
   * @param {AppData} [appData]
   */
  onMouseUp(e, appData) {
    this._isDragging = false;
    this.onContentMouseUp(e, appData);
  }

  /**
   * 指定座標がウィンドウ内（タイトルバー含む）にあるかを返す。
   * @param {number} x
   * @param {number} y
   * @returns {boolean}
   */
  containsPoint(x, y) {
    const { w: cw, h: ch } = this.getContentSize();
    const ww = cw;
    const wh = UIWindow.TITLE_H + ch;
    return x >= this._x && x < this._x + ww &&
           y >= this._y && y < this._y + wh;
  }

  // ----------------------------------------------------------------
  // 内部ヘルパー
  // ----------------------------------------------------------------

  /**
   * 指定座標がタイトルバー内にあるかを返す。
   * @param {number} x
   * @param {number} y
   * @returns {boolean}
   */
  _inTitleBar(x, y) {
    const { w: cw } = this.getContentSize();
    return x >= this._x && x < this._x + cw &&
           y >= this._y && y < this._y + UIWindow.TITLE_H;
  }

  /** ウィンドウ位置をキャンバス内にクランプする */
  _clamp() {
    if (this._canvasW === 0 || this._canvasH === 0) return;
    const { w: cw, h: ch } = this.getContentSize();
    const ww = cw;
    const wh = UIWindow.TITLE_H + ch;
    const m  = UIWindow.EDGE_MARGIN;
    this._x = Math.max(m, Math.min(this._canvasW  - ww - m, this._x));
    this._y = Math.max(MenuBar.HEIGHT + m, Math.min(this._canvasH - wh - m, this._y));
  }

  /** ユーティリティ: MenuBar.HEIGHT が定義されていれば返す（安全取得） */
  static _menuBarHeight() {
    return (typeof MenuBar !== 'undefined') ? MenuBar.HEIGHT : 24;
  }

  /** 角丸矩形パスを作成 */
  _roundRect(ctx, x, y, w, h, r) {
    if (ctx.roundRect) {
      ctx.beginPath();
      ctx.roundRect(x, y, w, h, r);
    } else {
      ctx.beginPath();
      ctx.moveTo(x + r, y);
      ctx.lineTo(x + w - r, y);
      ctx.quadraticCurveTo(x + w, y, x + w, y + r);
      ctx.lineTo(x + w, y + h - r);
      ctx.quadraticCurveTo(x + w, y + h, x + w - r, y + h);
      ctx.lineTo(x + r, y + h);
      ctx.quadraticCurveTo(x, y + h, x, y + h - r);
      ctx.lineTo(x, y + r);
      ctx.quadraticCurveTo(x, y, x + r, y);
      ctx.closePath();
    }
  }

  /** 上側だけ角丸のパスを作成（タイトルバー用） */
  _roundRectTop(ctx, x, y, w, h, r) {
    ctx.beginPath();
    ctx.moveTo(x + r, y);
    ctx.lineTo(x + w - r, y);
    ctx.quadraticCurveTo(x + w, y, x + w, y + r);
    ctx.lineTo(x + w, y + h);
    ctx.lineTo(x, y + h);
    ctx.lineTo(x, y + r);
    ctx.quadraticCurveTo(x, y, x + r, y);
    ctx.closePath();
  }
}
