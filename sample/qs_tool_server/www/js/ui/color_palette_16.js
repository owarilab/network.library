/**
 * ColorPalette16
 * クラシック 16 色カラーパレット。ColorPalette を継承。
 *
 * 4×4 グリッド配色:
 *   Row 0: 黒 / 濃いグレー / 薄いグレー / 白
 *   Row 1: 暗赤 / 赤 / オレンジ / 黄
 *   Row 2: 暗緑 / 黄緑 / 暗シアン / シアン
 *   Row 3: 暗青 / 青 / 紫 / マゼンタ
 */
class ColorPalette16 extends ColorPalette {
  /**
   * クラシック 16 色 (0xAARRGGBB 形式)
   * @type {number[]}
   */
  static COLORS = [
    // --- Row 0: グレースケール ---
    PixelData.rgba(  0,   0,   0, 255),  // 黒
    PixelData.rgba( 64,  64,  64, 255),  // 濃いグレー
    PixelData.rgba(160, 160, 160, 255),  // 薄いグレー
    PixelData.rgba(255, 255, 255, 255),  // 白

    // --- Row 1: 暖色系 ---
    PixelData.rgba(128,   0,   0, 255),  // 暗赤
    PixelData.rgba(255,   0,   0, 255),  // 赤
    PixelData.rgba(255, 128,   0, 255),  // オレンジ
    PixelData.rgba(255, 255,   0, 255),  // 黄

    // --- Row 2: 緑・青緑系 ---
    PixelData.rgba(  0, 128,   0, 255),  // 暗緑
    PixelData.rgba(  0, 255,   0, 255),  // 黄緑
    PixelData.rgba(  0, 128, 128, 255),  // 暗シアン
    PixelData.rgba(  0, 255, 255, 255),  // シアン

    // --- Row 3: 青・紫系 ---
    PixelData.rgba(  0,   0, 128, 255),  // 暗青
    PixelData.rgba(  0,   0, 255, 255),  // 青
    PixelData.rgba(128,   0, 128, 255),  // 紫
    PixelData.rgba(255,   0, 255, 255),  // マゼンタ
  ];

  constructor() {
    super();
  }

  /** @override */
  getColors() {
    return ColorPalette16.COLORS;
  }

  /** @override */
  getPaletteName() {
    return '16 Color';
  }
}
