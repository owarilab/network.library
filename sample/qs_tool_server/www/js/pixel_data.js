/**
 * PixelData
 * ドット絵のフルカラーピクセルデータを管理するクラス。
 * コンストラクタではデータを確保せず、createPixelData() で初期化する。
 */
class PixelData {
  constructor() {
    /** @type {number} */
    this.width = 0;
    /** @type {number} */
    this.height = 0;
    /** @type {Uint32Array|null} 各要素は 0xAARRGGBB */
    this.pixels = null;
  }

  // ----------------------------------------------------------------
  // 初期化
  // ----------------------------------------------------------------

  /**
   * ピクセルデータを指定サイズで初期化する。
   * 既存データは破棄される。
   * @param {number} width
   * @param {number} height
   * @param {number} [fillColor=0x00000000] - 初期塗りつぶし色 (0xAARRGGBB)
   */
  createPixelData(width, height, fillColor = 0x00000000) {
    this.width  = width;
    this.height = height;
    this.pixels = new Uint32Array(width * height);
    if (fillColor !== 0) {
      this.pixels.fill(fillColor >>> 0);
    }
  }

  // ----------------------------------------------------------------
  // ピクセル操作
  // ----------------------------------------------------------------

  /**
   * 指定座標のピクセル色を返す
   * @param {number} x
   * @param {number} y
   * @returns {number} 0xAARRGGBB
   */
  getPixel(x, y) {
    if (!this.pixels || x < 0 || x >= this.width || y < 0 || y >= this.height) return 0;
    return this.pixels[y * this.width + x];
  }

  /**
   * 指定座標にピクセル色を設定する
   * @param {number} x
   * @param {number} y
   * @param {number} color - 0xAARRGGBB
   */
  setPixel(x, y, color) {
    if (!this.pixels || x < 0 || x >= this.width || y < 0 || y >= this.height) return;
    this.pixels[y * this.width + x] = color >>> 0;
  }

  /**
   * 全ピクセルを指定色で塗りつぶす
   * @param {number} color - 0xAARRGGBB
   */
  fill(color) {
    this.pixels?.fill(color >>> 0);
  }

  // ----------------------------------------------------------------
  // 色ユーティリティ (static)
  // ----------------------------------------------------------------

  /**
   * RGBA 各成分から 0xAARRGGBB を生成する
   * @param {number} r 0-255
   * @param {number} g 0-255
   * @param {number} b 0-255
   * @param {number} [a=255] 0-255
   * @returns {number}
   */
  static rgba(r, g, b, a = 255) {
    return (((a & 0xff) << 24) | ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff)) >>> 0;
  }

  /**
   * 0xAARRGGBB を { r, g, b, a } に分解する
   * @param {number} color
   * @returns {{ r: number, g: number, b: number, a: number }}
   */
  static unpack(color) {
    return {
      a: (color >>> 24) & 0xff,
      r: (color >>> 16) & 0xff,
      g: (color >>>  8) & 0xff,
      b:  color         & 0xff,
    };
  }

  /**
   * 0xAARRGGBB を CSS カラー文字列に変換する
   * @param {number} color
   * @returns {string} 例: "rgba(255,0,0,1)"
   */
  static toCssColor(color) {
    const { r, g, b, a } = PixelData.unpack(color);
    return `rgba(${r},${g},${b},${(a / 255).toFixed(3)})`;
  }
}
