/**
 * EditablePalette32
 * 編集可能な32色カラーパレット。ColorPalette を継承。
 *
 * インデックス0は透明色（固定・編集不可）。
 * インデックス1〜31はユーザーが自由に編集可能。
 * パレットセル上でダブルクリックすると ColorPickerDialog が開く
 * （ColorPalette.onCellDoubleClick コールバック経由）。
 */
class EditablePalette32 extends ColorPalette {
  /**
   * デフォルト32色 (0xAARRGGBB 形式)
   * @type {number[]}
   */
  static DEFAULT_COLORS = [
    // 0: 透明（固定・編集不可）
    0x00000000,
    // --- 1〜4: グレースケール ---
    PixelData.rgba(  0,   0,   0, 255),  // 1: 黒
    PixelData.rgba( 64,  64,  64, 255),  // 2: 濃いグレー
    PixelData.rgba(160, 160, 160, 255),  // 3: 薄いグレー
    PixelData.rgba(255, 255, 255, 255),  // 4: 白
    // --- 5〜8: 暖色系 ---
    PixelData.rgba(128,   0,   0, 255),  // 5: 暗赤
    PixelData.rgba(255,   0,   0, 255),  // 6: 赤
    PixelData.rgba(255, 128,   0, 255),  // 7: オレンジ
    PixelData.rgba(255, 255,   0, 255),  // 8: 黄
    // --- 9〜12: 緑・シアン系 ---
    PixelData.rgba(  0, 128,   0, 255),  // 9: 暗緑
    PixelData.rgba(  0, 255,   0, 255),  // 10: 黄緑
    PixelData.rgba(  0, 128, 128, 255),  // 11: 暗シアン
    PixelData.rgba(  0, 255, 255, 255),  // 12: シアン
    // --- 13〜16: 青・紫系 ---
    PixelData.rgba(  0,   0, 128, 255),  // 13: 暗青
    PixelData.rgba(  0,   0, 255, 255),  // 14: 青
    PixelData.rgba(128,   0, 128, 255),  // 15: 紫
    PixelData.rgba(255,   0, 255, 255),  // 16: マゼンタ
    // --- 17〜21: 肌・茶系 ---
    PixelData.rgba(255, 224, 189, 255),  // 17: 肌色（明）
    PixelData.rgba(224, 172, 105, 255),  // 18: 肌色（暗）
    PixelData.rgba(160, 100,  50, 255),  // 19: 茶
    PixelData.rgba(100,  60,  20, 255),  // 20: 暗茶
    PixelData.rgba(210, 180, 140, 255),  // 21: タン
    // --- 22〜25: パステル系 ---
    PixelData.rgba(255, 182, 193, 255),  // 22: ピンク
    PixelData.rgba(255, 218, 185, 255),  // 23: ピーチ
    PixelData.rgba(173, 216, 230, 255),  // 24: ライトブルー
    PixelData.rgba(144, 238, 144, 255),  // 25: ライトグリーン
    // --- 26〜29: 中間色 ---
    PixelData.rgba(255, 165,   0, 255),  // 26: ダークオレンジ
    PixelData.rgba( 70, 130, 180, 255),  // 27: スチールブルー
    PixelData.rgba( 34, 139,  34, 255),  // 28: フォレストグリーン
    PixelData.rgba(220,  20,  60, 255),  // 29: クリムゾン
    // --- 30〜31: 明暗補助 ---
    PixelData.rgba( 32,  32,  32, 255),  // 30: ほぼ黒
    PixelData.rgba(224, 224, 224, 255),  // 31: ほぼ白
  ];

  constructor() {
    super();
    /** @type {number[]} 32色配列（ミュータブル） */
    this._colors = EditablePalette32.DEFAULT_COLORS.slice();
  }

  /** @override */
  getColors() {
    return this._colors;
  }

  /** @override */
  getPaletteName() {
    return 'Palette (32)';
  }

  /**
   * 指定インデックスの色を変更する。
   * インデックス0（透明色）は変更不可。
   * @param {number} index  1〜31
   * @param {number} color  0xAARRGGBB
   */
  setColor(index, color) {
    if (index <= 0 || index >= this._colors.length) return;
    this._colors[index] = color;
  }

  /**
   * 指定インデックスの色を取得する。
   * @param {number} index  0〜31
   * @returns {number} 0xAARRGGBB
   */
  getColor(index) {
    if (index < 0 || index >= this._colors.length) return 0x00000000;
    return this._colors[index];
  }

  /** デフォルト32色にリセットする。 */
  resetToDefaults() {
    this._colors = EditablePalette32.DEFAULT_COLORS.slice();
  }

  /**
   * ディープコピーを返す。
   * @returns {EditablePalette32}
   */
  clone() {
    const c = new EditablePalette32();
    c._colors = this._colors.slice();
    return c;
  }
}
