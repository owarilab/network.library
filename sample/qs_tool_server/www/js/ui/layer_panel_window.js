/**
 * LayerPanelWindow
 * UIWindow を継承したレイヤー管理ウィンドウ。
 *
 * 内部に LayerPanel インスタンスを保持し、
 * タイトルバードラッグで自由に移動できる。
 */
class LayerPanelWindow extends UIWindow {
  /**
   * @param {number} [initX]  初期 X 座標（省略時は画面右寄り）
   * @param {number} [initY]  初期 Y 座標
   */
  constructor(initX = null, initY = null) {
    super(initX !== null ? initX : 300, initY);
    /** @type {LayerPanel} */
    this._panel = new LayerPanel();
    /** appData キャッシュ (getContentSize で使う) */
    this._appDataRef = null;
    /** 初回配置フラグ */
    this._positioned = initX !== null;
  }

  /**
   * レイヤー操作時のコールバックを設定する。
   * @param {()=>void} fn
   */
  set onChange(fn) {
    this._panel.onChange = fn;
  }

  // ----------------------------------------------------------------
  // UIWindow override
  // ----------------------------------------------------------------

  /** @override */
  getWindowTitle() { return 'Layers'; }

  /** @override */
  getContentSize() {
    return this._panel.getContentSize(this._appDataRef);
  }

  /** @override */
  renderContent(ctx, cx, cy, cw, ch, appData) {
    this._panel.render(ctx, cx, cy, appData);
  }

  /** @override */
  onContentMouseMove(e, appData) {
    this._panel.onMouseMove(e, appData);
  }

  /** @override */
  onContentMouseDown(e, appData) {
    return this._panel.onMouseDown(e, appData);
  }

  /** @override */
  onContentMouseUp(e, appData) {
    this._panel.onMouseUp(e, appData);
  }

  /**
   * render を override して appData ref を保持する。
   * @override
   */
  render(ctx, canvas, appData) {
    this._appDataRef = appData;
    // 初回: 画面幅に応じて右寄せ
    if (!this._positioned && canvas.width > 0) {
      this._x = Math.max(8, canvas.width - LayerPanel.WIDTH - 16);
      this._positioned = true;
    }
    super.render(ctx, canvas, appData);
  }
}
