/**
 * AppData
 * シーン間で共有するアプリケーションデータを保持するクラス。
 * レイヤー管理は LayerData クラスに委譲し、
 * pixelData プロパティはアクティブレイヤーの PixelData を返す getter。
 *
 * タイルセットモード時は layerData getter が選択チップの LayerData を返すため、
 * PixelCanvas / LayerPanel / ツール群は無改修で動作する。
 */
class AppData {
  constructor() {
    /**
     * free モード用の LayerData。
     * layerData getter 経由でアクセスされる。
     * @type {LayerData}
     */
    this._layerData = new LayerData();

    /**
     * 前景色 (描画色)。0xAARRGGBB 形式。
     * @type {number}
     */
    this.foreColor = PixelData.rgba(0, 0, 0, 255);       // 黒

    /**
     * 背景色 (消しゴム色 / 塗りつぶし背景)。0xAARRGGBB 形式。
     * @type {number}
     */
    this.backColor = PixelData.rgba(255, 255, 255, 255);  // 白

    /**
     * 現在選択中のツール ID。
    * 'pencil' | 'eraser' | 'fill' | 'eyedropper' | 'selectRect'
     * @type {string}
     */
    this.activeTool = 'pencil';

    // ---- タイルセットモード用プロパティ ----

    /**
     * タイルセットデータ。tileset モード時のみ有効。
     * @type {TilesetData|null}
     */
    this.tilesetData = null;

    /**
     * タイルセットモード時の選択中チップ位置。
     * @type {{ col: number, row: number }}
     */
    this.selectedChip = { col: 0, row: 0 };

    /**
     * 編集モード。'free' = 従来のドット絵エディタ、'tileset' = タイルセットモード。
     * @type {'free'|'tileset'}
     */
    this.editMode = 'free';

    /**
     * チップクリップボード。コピーしたチップの LayerData ディープコピーを保持する。
     * @type {LayerData|null}
     */
    this.chipClipboard = null;

    /**
     * 編集可能カラーパレット（32色）。
     * EditorScene.onEnter() で EditablePalette32 インスタンスが設定される。
     * @type {EditablePalette32|null}
     */
    this.palette = null;

    /**
     * 通過フラグの表示切替。true = チップパレット上に通過フラグを表示する。
     * @type {boolean}
     */
    this.showPassFlags = true;

    /**
     * 現在の選択範囲。
     * @type {{ active: boolean, x: number, y: number, w: number, h: number, mode: string, floating: object|null }}
     */
    this.selection = {
      active: false,
      x: 0,
      y: 0,
      w: 0,
      h: 0,
      mode: 'rect',
      floating: null,
    };

    /**
     * 将来のコピー/貼り付け用選択クリップボード。
     * @type {{ pixelData: PixelData, width: number, height: number }|null}
     */
    this.selectionClipboard = null;
  }

  /**
   * モードに応じた LayerData を返す。
   * tileset モード: 選択中チップの LayerData
   * free モード: 従来の _layerData
   * → これにより PixelCanvas / LayerPanel / ツール群は無改修で動作する。
   * @type {LayerData}
   */
  get layerData() {
    if (this.editMode === 'tileset' && this.tilesetData) {
      return this.tilesetData.getChipLayerData(
        this.selectedChip.col, this.selectedChip.row
      );
    }
    return this._layerData;
  }

  /**
   * アクティブレイヤーの PixelData を返す。
   * 既存コードとの後方互換のための getter。
   * @type {PixelData}
   */
  get pixelData() {
    return this.layerData.getActiveLayer();
  }

  /**
   * pixelData を直接差し替える (インポート時の互換用)。
   * アクティブレイヤーの pixelData を入れ替え、LayerData のサイズも更新する。
   * 常に free モードの _layerData に書き込む。
   * @param {PixelData} pd
   */
  set pixelData(pd) {
    if (!pd) return;
    const ld = this._layerData;
    ld.width  = pd.width;
    ld.height = pd.height;
    // インポート時は1レイヤーにリセット
    ld.layers = [{
      pixelData: pd,
      name:      'レイヤー 1',
      visible:   true,
      opacity:   255,
    }];
    ld.activeIndex = 0;
    ld._composite.createPixelData(pd.width, pd.height);
    ld.markCompositeDirty();
  }

  /**
   * レイヤーデータを指定サイズで初期化する（1レイヤー構成）。
   * 常に free モードの _layerData を初期化する。
   * @param {number} width
   * @param {number} height
   * @param {number} [fillColor=0x00000000]
   */
  createPixelData(width, height, fillColor = 0x00000000) {
    this._layerData.init(width, height, fillColor);
    this.clearSelection();
  }

  /** 選択範囲を解除する。 */
  clearSelection() {
    this.selection.active = false;
    this.selection.x = 0;
    this.selection.y = 0;
    this.selection.w = 0;
    this.selection.h = 0;
    this.selection.mode = 'rect';
    this.selection.floating = null;
  }

  /**
   * 矩形選択を設定する。サイズが 0 以下なら選択解除する。
   * @param {number} x
   * @param {number} y
   * @param {number} w
   * @param {number} h
   */
  setSelectionRect(x, y, w, h) {
    const nx = x | 0;
    const ny = y | 0;
    const nw = w | 0;
    const nh = h | 0;
    if (nw <= 0 || nh <= 0) {
      this.clearSelection();
      return;
    }
    this.selection.active = true;
    this.selection.x = nx;
    this.selection.y = ny;
    this.selection.w = nw;
    this.selection.h = nh;
    this.selection.mode = 'rect';
    this.selection.floating = null;
  }

  /**
   * 選択範囲が存在するか返す。
   * @returns {boolean}
   */
  hasSelection() {
    return this.selection.active && this.selection.w > 0 && this.selection.h > 0;
  }

  /**
   * 浮動選択が存在するか返す。
   * @returns {boolean}
   */
  hasFloatingSelection() {
    return !!this.selection.floating;
  }
}
