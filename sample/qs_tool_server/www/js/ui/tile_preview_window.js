/**
 * TilePreviewWindow
 * UIWindow を継承したタイルプレビューウィンドウ。
 *
 * 内部に TilePreview インスタンスを保持し、
 * 選択中チップのタイリング（繰り返し配置）プレビューを表示する。
 * チップ境界がつながるかを確認できる。
 *
 * 使い方:
 *   const win = new TilePreviewWindow();
 *   win.render(ctx, canvas, appData);
 */
class TilePreviewWindow extends UIWindow {
  /**
   * @param {number} [initX]  初期 X 座標（省略時は自動配置）
   * @param {number} [initY]  初期 Y 座標
   */
  constructor(initX = null, initY = null) {
    super(initX !== null ? initX : 300, initY);
    /** @type {TilePreview} */
    this._preview = new TilePreview();
    /** appData キャッシュ (getContentSize で使う) */
    this._appDataRef = null;
    /** 初回配置フラグ */
    this._positioned = initX !== null;
  }

  // ----------------------------------------------------------------
  // UIWindow override
  // ----------------------------------------------------------------

  /** @override */
  getWindowTitle() { return 'Tile Preview'; }

  /** @override */
  getContentSize() {
    return this._preview.getContentSize(this._appDataRef);
  }

  /** @override */
  renderContent(ctx, cx, cy, cw, ch, appData) {
    this._preview.render(ctx, cx, cy, appData);
  }

  /** @override */
  onContentMouseMove(e, appData) {
    this._preview.onMouseMove(e);
  }

  /** @override */
  onContentMouseDown(e, appData) {
    return this._preview.onMouseDown(e, appData);
  }

  /**
   * render を override して appData ref を保持し、初回配置を行う。
   * @override
   */
  render(ctx, canvas, appData) {
    this._appDataRef = appData;

    // タイルセットモードでなければ描画しない
    if (!appData || appData.editMode !== 'tileset' || !appData.tilesetData) return;

    // 初回: チップパレットの下方に配置
    if (!this._positioned && canvas.width > 0) {
      const contentSize = this._preview.getContentSize(appData);
      this._x = Math.max(8, canvas.width - contentSize.w - 16);
      this._y = canvas.height - contentSize.h - UIWindow.TITLE_H - 40;
      this._positioned = true;
    }

    super.render(ctx, canvas, appData);
  }

  /**
   * onMouseDown を override してタイルセットモード時のみ反応する。
   * @override
   */
  onMouseDown(e, appData) {
    if (!appData || appData.editMode !== 'tileset' || !appData.tilesetData) return false;
    return super.onMouseDown(e, appData);
  }

  /**
   * onMouseMove を override してタイルセットモード時のみ反応する。
   * @override
   */
  onMouseMove(e, appData) {
    if (!appData || appData.editMode !== 'tileset' || !appData.tilesetData) return;
    super.onMouseMove(e, appData);
  }
}
