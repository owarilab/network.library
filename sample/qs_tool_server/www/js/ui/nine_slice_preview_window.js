/**
 * NineSlicePreviewWindow
 * UIWindow を継承した 9 スライスのプレビューウィンドウ。
 *
 * pixelDocument の編集中で、9 スライスの境界が有効なときだけ表示・反応する。
 */
class NineSlicePreviewWindow extends UIWindow {
  constructor(initX = null, initY = null) {
    super(initX !== null ? initX : 300, initY);
    /** @type {NineSlicePreview} */
    this._preview = new NineSlicePreview();
    this._positioned = initX !== null;
  }

  /**
   * 9 スライスを編集できる状態か（free モードで境界が有効）。
   * @param {AppData} appData
   * @returns {boolean}
   */
  static isActive(appData) {
    return appData?.editMode === 'free' && !!appData.getActiveNineSlice?.();
  }

  /** @override */
  getWindowTitle() { return 'Nine-slice Preview'; }

  /** @override */
  getContentSize() {
    return this._preview.getContentSize();
  }

  /** @override */
  renderContent(ctx, cx, cy, cw, ch, appData) {
    this._preview.render(ctx, cx, cy, appData);
  }

  /** @override */
  onContentMouseMove(e) {
    this._preview.onMouseMove(e);
  }

  /** @override */
  onContentMouseDown(e) {
    return this._preview.onMouseDown(e);
  }

  /** @override */
  render(ctx, canvas, appData) {
    if (!NineSlicePreviewWindow.isActive(appData)) return;

    // 初回: キャンバスの右下に配置
    if (!this._positioned && canvas.width > 0) {
      const contentSize = this._preview.getContentSize();
      this._x = Math.max(8, canvas.width - contentSize.w - 16);
      this._y = canvas.height - contentSize.h - UIWindow.TITLE_H - 40;
      this._positioned = true;
    }

    super.render(ctx, canvas, appData);
  }

  /** @override */
  onMouseDown(e, appData) {
    if (!NineSlicePreviewWindow.isActive(appData)) return false;
    return super.onMouseDown(e, appData);
  }

  /** @override */
  onMouseMove(e, appData) {
    if (!NineSlicePreviewWindow.isActive(appData)) return;
    super.onMouseMove(e, appData);
  }
}
