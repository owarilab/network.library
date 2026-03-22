/**
 * ChipPaletteWindow
 * UIWindow を継承したチップパレットウィンドウ。
 *
 * 内部に ChipPalette インスタンスを保持し、
 * タイルセットの全チップをサムネイル一覧として表示する。
 * クリックでチップを選択し、ダブルクリックでフォーカスズームを行う。
 *
 * 使い方:
 *   const win = new ChipPaletteWindow();
 *   win.onChipSelect      = (col, row) => { ... };
 *   win.onChipDoubleClick = (col, row) => { ... };
 *   win.render(ctx, canvas, appData);
 */
class ChipPaletteWindow extends UIWindow {
  /**
   * @param {number} [initX]  初期 X 座標（省略時は画面右寄り）
   * @param {number} [initY]  初期 Y 座標
   */
  constructor(initX = null, initY = null) {
    super(initX !== null ? initX : 300, initY);
    /** @type {ChipPalette} */
    this._palette = new ChipPalette();
    /** appData キャッシュ (getContentSize で使う) */
    this._appDataRef = null;
    /** 初回配置フラグ */
    this._positioned = initX !== null;
  }

  /**
   * チップ選択時のコールバックを設定する。
   * @param {(col: number, row: number) => void} fn
   */
  set onChipSelect(fn) {
    this._palette.onChipSelect = fn;
  }

  /**
   * チップダブルクリック時のコールバックを設定する。
   * @param {(col: number, row: number) => void} fn
   */
  set onChipDoubleClick(fn) {
    this._palette.onChipDoubleClick = fn;
  }

  /**
   * チップ右クリック時のコールバックを設定する。
   * @param {(col: number, row: number, screenX: number, screenY: number) => void} fn
   */
  set onChipContextMenu(fn) {
    this._palette.onChipContextMenu = fn;
  }

  // ----------------------------------------------------------------
  // UIWindow override
  // ----------------------------------------------------------------

  /** @override */
  getWindowTitle() { return 'Chip Palette'; }

  /** @override */
  getContentSize() {
    return this._palette.getContentSize(this._appDataRef);
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

  /**
   * render を override して appData ref を保持し、初回配置を行う。
   * @override
   */
  render(ctx, canvas, appData) {
    this._appDataRef = appData;

    // タイルセットモードでなければ描画しない
    if (!appData || appData.editMode !== 'tileset' || !appData.tilesetData) return;

    // 初回: レイヤーパネルの左隣に配置
    if (!this._positioned && canvas.width > 0) {
      const contentSize = this._palette.getContentSize(appData);
      this._x = Math.max(8, canvas.width - contentSize.w - LayerPanel.WIDTH - 32);
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

  /**
   * 右クリック。タイルセットモード時 + ウィンドウ内ならチップパレットに委譲。
   * @param {{x: number, y: number}} e
   * @param {AppData} appData
   * @returns {boolean}
   */
  onContextMenu(e, appData) {
    if (!appData || appData.editMode !== 'tileset' || !appData.tilesetData) return false;
    if (!this.containsPoint(e.x, e.y)) return false;
    return this._palette.onContextMenu(e, appData);
  }
}
