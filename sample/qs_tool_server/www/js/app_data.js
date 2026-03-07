/**
 * AppData
 * シーン間で共有するアプリケーションデータを保持するクラス。
 * レイヤー管理は LayerData クラスに委譲し、
 * pixelData プロパティはアクティブレイヤーの PixelData を返す getter。
 */
class AppData {
  constructor() {
    /** @type {LayerData} */
    this.layerData = new LayerData();

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
     * 'pencil' | 'eraser' | 'fill' | 'eyedropper'
     * @type {string}
     */
    this.activeTool = 'pencil';
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
   * @param {PixelData} pd
   */
  set pixelData(pd) {
    if (!pd) return;
    const ld = this.layerData;
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
   * @param {number} width
   * @param {number} height
   * @param {number} [fillColor=0x00000000]
   */
  createPixelData(width, height, fillColor = 0x00000000) {
    this.layerData.init(width, height, fillColor);
  }
}
