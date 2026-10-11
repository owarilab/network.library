/**
 * CharacterPreviewWindow
 * UIWindow を継承したキャラクタープレビューウィンドウ。
 *
 * 内部に CharacterPreview を保持し、向きの切替と歩行アニメを表示する。
 * キャラクターチップセットの編集中のみ表示・反応する。
 *
 * 使い方:
 *   const win = new CharacterPreviewWindow();
 *   win.render(ctx, canvas, appData);
 */
class CharacterPreviewWindow extends UIWindow {
  /**
   * @param {number} [initX]  初期 X 座標（省略時は自動配置）
   * @param {number} [initY]  初期 Y 座標
   */
  constructor(initX = null, initY = null) {
    super(initX !== null ? initX : 300, initY);
    /** @type {CharacterPreview} */
    this._preview = new CharacterPreview();
    /** appData キャッシュ (getContentSize で使う) */
    this._appDataRef = null;
    /** 初回配置フラグ */
    this._positioned = initX !== null;
  }

  // ----------------------------------------------------------------
  // UIWindow override
  // ----------------------------------------------------------------

  /** @override */
  getWindowTitle() { return 'Character Preview'; }

  /** @override */
  getContentSize() {
    return this._preview.getContentSize();
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
    return this._preview.onMouseDown(e);
  }

  /**
   * render を override して appData ref を保持し、初回配置を行う。
   * @override
   */
  render(ctx, canvas, appData) {
    this._appDataRef = appData;

    if (!CharacterChipset.isEditing(appData)) return;

    // 初回: キャンバスの右下に配置
    if (!this._positioned && canvas.width > 0) {
      const contentSize = this._preview.getContentSize();
      this._x = Math.max(8, canvas.width - contentSize.w - 16);
      this._y = canvas.height - contentSize.h - UIWindow.TITLE_H - 40;
      this._positioned = true;
    }

    super.render(ctx, canvas, appData);
  }

  /**
   * キャラクターチップセット編集中のみ反応する。
   * @override
   */
  onMouseDown(e, appData) {
    if (!CharacterChipset.isEditing(appData)) return false;
    return super.onMouseDown(e, appData);
  }

  /**
   * キャラクターチップセット編集中のみ反応する。
   * @override
   */
  onMouseMove(e, appData) {
    if (!CharacterChipset.isEditing(appData)) return;
    super.onMouseMove(e, appData);
  }
}
