/**
 * ToolBarWindow
 * UIWindow を継承したツール選択ウィンドウ。
 *
 * 内部に ToolBar インスタンスを保持し、
 * タイトルバードラッグで自由に移動できる。
 *
 * 使い方:
 *   const win = new ToolBarWindow();
 *   win.render(ctx, canvas, appData);
 *   win.onMouseMove(e, appData);
 *   const consumed = win.onMouseDown(e, appData);
 *   win.onMouseUp(e, appData);
 */
class ToolBarWindow extends UIWindow {
  /**
   * @param {number} [initX]  初期 X 座標（省略時はカラーパレット右隣に配置）
   * @param {number} [initY]  初期 Y 座標（省略時は MenuBar 直下）
   */
  constructor(initX = null, initY = null) {
    // ColorPaletteWindow のデフォルト幅 (PADDING*2 + COLS*CELL + (COLS-1)*GAP = 70) + 初期X(8) + gap(8)
    const defaultX = 8 + 70 + 8;  // = 86
    super(initX !== null ? initX : defaultX, initY);
    /** @type {ToolBar} */
    this._toolBar = new ToolBar();
  }

  // ----------------------------------------------------------------
  // UIWindow override
  // ----------------------------------------------------------------

  /** @override */
  getWindowTitle() { return 'Tools'; }

  /** @override */
  getContentSize() { return this._toolBar.getContentSize(); }

  /** @override */
  renderContent(ctx, cx, cy, cw, ch, appData) {
    this._toolBar.render(ctx, cx, cy, appData);
  }

  /** @override */
  onContentMouseMove(e, appData) {
    this._toolBar.onMouseMove(e);
  }

  /** @override */
  onContentMouseDown(e, appData) {
    return this._toolBar.onMouseDown(e, appData);
  }
}
