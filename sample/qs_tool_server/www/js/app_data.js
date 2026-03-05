/**
 * AppData
 * シーン間で共有するアプリケーションデータを保持するクラス。
 * ピクセルデータは PixelData クラスで管理し、createPixelData() で初期化する。
 */
class AppData {
  constructor() {
    /** @type {PixelData} */
    this.pixelData = new PixelData();

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
   * ピクセルデータを指定サイズで初期化する
   * @param {number} width
   * @param {number} height
   * @param {number} [fillColor=0x00000000]
   */
  createPixelData(width, height, fillColor = 0x00000000) {
    this.pixelData.createPixelData(width, height, fillColor);
  }
}
