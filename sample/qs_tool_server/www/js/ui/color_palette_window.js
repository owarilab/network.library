/**
 * ColorPaletteWindow
 * UIWindow を継承したカラーパレットウィンドウ。
 *
 * コンストラクタに ColorPalette インスタンスを渡すことで、
 * 任意のパレット実装（ColorPalette16 など）を表示できる。
 *
 * 使い方:
 *   const win = new ColorPaletteWindow(new ColorPalette16());
 *   // EditorScene.render() で:
 *   win.render(ctx, canvas, appData);
 *   // イベント:
 *   win.onMouseMove(e, appData);
 *   const consumed = win.onMouseDown(e, appData);
 *   win.onMouseUp(e, appData);
 */
class ColorPaletteWindow extends UIWindow {
  /**
   * @param {ColorPalette} palette  表示するパレットのインスタンス
   * @param {number} [initX=8]
   * @param {number} [initY]
   */
  constructor(palette, initX = 8, initY = null) {
    super(initX, initY);
    /** @type {ColorPalette} */
    this._palette = palette;
  }

  // ----------------------------------------------------------------
  // UIWindow override
  // ----------------------------------------------------------------

  /** @override */
  getWindowTitle() {
    return this._palette.getPaletteName();
  }

  /** @override */
  getContentSize() {
    return this._palette.getContentSize();
  }

  /** @override */
  renderContent(ctx, cx, cy, cw, ch, appData) {
    this._palette.render(ctx, cx, cy, appData);
  }

  /** @override */
  onContentMouseMove(e, appData) {
    this._palette.onMouseMove(e);
  }

  /** @override */
  onContentMouseDown(e, appData) {
    return this._palette.onMouseDown(e, appData);
  }

  // ----------------------------------------------------------------
  // パレット切り替え
  // ----------------------------------------------------------------

  /**
   * 表示するパレットを切り替える。
   * @param {ColorPalette} palette
   */
  setPalette(palette) {
    this._palette = palette;
  }

  /** 現在のパレットを返す */
  getPalette() {
    return this._palette;
  }
}
