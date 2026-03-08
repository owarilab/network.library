/**
 * TilePreview
 * 選択中チップのタイリング（繰り返し配置）プレビューを描画するコンテンツクラス。
 * UIWindow のコンテンツとして使われることを前提とし、
 * 与えられた (x, y) を起点にタイリングされたチップを描画する。
 *
 * ChipPalette と同パターン:
 *   const size = preview.getContentSize(appData);
 *   preview.render(ctx, x, y, appData);
 */
class TilePreview {
  // ---- レイアウト定数 ----
  static PADDING    = 4;
  static CELL_SIZE  = 32;     // 1チップのスクリーン表示サイズ (px)
  static GAP        = 0;      // チップ間のギャップ (タイリングなので0)
  static LABEL_H    = 20;     // 下部ラベル（サイズ切り替え表示）の高さ
  static BTN_W      = 40;     // サイズ切り替えボタンの幅
  static BTN_H      = 16;     // サイズ切り替えボタンの高さ

  // ---- スタイル定数 ----
  static GRID_COLOR     = 'rgba(255,255,255,0.12)';
  static TEXT_COLOR     = '#dcdcdc';
  static TEXT_FONT      = '10px sans-serif';
  static CHECKER_LIGHT  = '#555555';
  static CHECKER_DARK   = '#444444';
  static BTN_BG         = '#2a3a50';
  static BTN_BG_HOVER   = '#3a5070';
  static BTN_BG_ACTIVE  = '#4a6090';
  static BTN_BORDER     = '#607090';
  static BTN_TEXT       = '#dcdcdc';
  static BTN_FONT       = '10px sans-serif';

  // ---- プリセットサイズ ----
  static PRESETS = [
    { label: '3×3', repeatX: 3, repeatY: 3 },
    { label: '5×5', repeatX: 5, repeatY: 5 },
  ];

  constructor() {
    /** 横方向繰り返し数 */
    this.repeatX = 3;
    /** 縦方向繰り返し数 */
    this.repeatY = 3;
    /** 現在のプリセットインデックス */
    this._presetIndex = 0;

    /** 描画時の起点座標キャッシュ */
    this._renderX = 0;
    this._renderY = 0;

    /** ホバー中のボタンインデックス (-1 = なし) */
    this._hoverBtn = -1;

    /** ボタンのヒットテスト矩形 */
    this._btnRects = [];

    // ---- オフスクリーン canvas (サムネイル描画用) ----
    /** @type {HTMLCanvasElement|null} */
    this._offscreen = null;
    /** @type {CanvasRenderingContext2D|null} */
    this._offCtx = null;
  }

  // ----------------------------------------------------------------
  // サイズ計算
  // ----------------------------------------------------------------

  /**
   * コンテンツ領域のサイズを返す。
   * @param {AppData} appData
   * @returns {{ w: number, h: number }}
   */
  getContentSize(appData) {
    const { PADDING, CELL_SIZE, GAP, LABEL_H } = TilePreview;
    const tileW = this.repeatX * (CELL_SIZE + GAP);
    const tileH = this.repeatY * (CELL_SIZE + GAP);
    return {
      w: PADDING * 2 + tileW,
      h: PADDING * 2 + tileH + LABEL_H,
    };
  }

  // ----------------------------------------------------------------
  // 描画
  // ----------------------------------------------------------------

  /**
   * タイルプレビューを描画する。
   * @param {CanvasRenderingContext2D} ctx
   * @param {number}  x       描画起点 X
   * @param {number}  y       描画起点 Y
   * @param {AppData} appData
   */
  render(ctx, x, y, appData) {
    const { PADDING, CELL_SIZE, GAP, LABEL_H,
            GRID_COLOR, TEXT_COLOR, TEXT_FONT } = TilePreview;

    this._renderX = x;
    this._renderY = y;
    this._btnRects = [];

    if (!appData || appData.editMode !== 'tileset' || !appData.tilesetData) return;

    const td  = appData.tilesetData;
    const sel = appData.selectedChip;

    // 選択チップの合成結果を取得
    const chipPd = td.compositeChip(sel.col, sel.row);
    if (!chipPd) return;

    // ---- タイリング描画 ----
    for (let r = 0; r < this.repeatY; r++) {
      for (let c = 0; c < this.repeatX; c++) {
        const cx = x + PADDING + c * (CELL_SIZE + GAP);
        const cy = y + PADDING + r * (CELL_SIZE + GAP);

        // 市松模様背景（透明部分の可視化）
        this._drawChecker(ctx, cx, cy, CELL_SIZE, CELL_SIZE);

        // チップを描画
        this._drawPixelDataScaled(ctx, chipPd, cx, cy, CELL_SIZE, CELL_SIZE);
      }
    }

    // ---- グリッド線（チップ境界を薄く表示） ----
    ctx.strokeStyle = GRID_COLOR;
    ctx.lineWidth   = 1;
    const totalW = this.repeatX * (CELL_SIZE + GAP);
    const totalH = this.repeatY * (CELL_SIZE + GAP);
    const ox = x + PADDING;
    const oy = y + PADDING;

    for (let c = 1; c < this.repeatX; c++) {
      const lx = ox + c * (CELL_SIZE + GAP) - 0.5;
      ctx.beginPath();
      ctx.moveTo(lx, oy);
      ctx.lineTo(lx, oy + totalH);
      ctx.stroke();
    }
    for (let r = 1; r < this.repeatY; r++) {
      const ly = oy + r * (CELL_SIZE + GAP) - 0.5;
      ctx.beginPath();
      ctx.moveTo(ox, ly);
      ctx.lineTo(ox + totalW, ly);
      ctx.stroke();
    }

    // ---- 下部: サイズ切り替えボタン ----
    const labelY = y + PADDING + totalH + 2;
    this._renderSizeButtons(ctx, x + PADDING, labelY, appData);
  }

  /**
   * サイズ切り替えボタンを描画する。
   */
  _renderSizeButtons(ctx, x, y, appData) {
    const { BTN_W, BTN_H, BTN_BG, BTN_BG_HOVER, BTN_BG_ACTIVE,
            BTN_BORDER, BTN_TEXT, BTN_FONT, PRESETS } = TilePreview;

    ctx.font         = BTN_FONT;
    ctx.textAlign    = 'center';
    ctx.textBaseline = 'middle';

    for (let i = 0; i < PRESETS.length; i++) {
      const bx = x + i * (BTN_W + 4);
      const by = y;
      this._btnRects.push({ x: bx, y: by, w: BTN_W, h: BTN_H, index: i });

      const isActive = i === this._presetIndex;
      const isHover  = i === this._hoverBtn;

      ctx.fillStyle   = isActive ? BTN_BG_ACTIVE : isHover ? BTN_BG_HOVER : BTN_BG;
      ctx.strokeStyle = BTN_BORDER;
      ctx.lineWidth   = 1;
      ctx.beginPath();
      ctx.rect(bx, by, BTN_W, BTN_H);
      ctx.fill();
      ctx.stroke();

      ctx.fillStyle = BTN_TEXT;
      ctx.fillText(PRESETS[i].label, bx + BTN_W / 2, by + BTN_H / 2);
    }

    // リセット
    ctx.textAlign    = 'left';
    ctx.textBaseline = 'alphabetic';
  }

  /**
   * 市松模様を描画する（透明部分の背景用）。
   */
  _drawChecker(ctx, x, y, w, h) {
    const size = 4;
    const { CHECKER_LIGHT, CHECKER_DARK } = TilePreview;
    ctx.save();
    ctx.beginPath();
    ctx.rect(x, y, w, h);
    ctx.clip();
    for (let cy = 0; cy < h; cy += size) {
      for (let cx = 0; cx < w; cx += size) {
        ctx.fillStyle = ((cx / size + cy / size) % 2 === 0)
          ? CHECKER_LIGHT : CHECKER_DARK;
        ctx.fillRect(x + cx, y + cy, size, size);
      }
    }
    ctx.restore();
  }

  /**
   * PixelData をスケーリングして描画する。
   * ニアレストネイバーで拡縮するためオフスクリーン canvas を使用。
   */
  _drawPixelDataScaled(ctx, pixelData, dx, dy, dw, dh) {
    const sw = pixelData.width;
    const sh = pixelData.height;
    if (sw === 0 || sh === 0) return;

    // オフスクリーン canvas の生成（初回のみ）
    if (!this._offscreen) {
      this._offscreen = document.createElement('canvas');
      this._offCtx = this._offscreen.getContext('2d');
    }
    // Safari では putImageData 後の drawImage でテクスチャキャッシュが更新されないことがある。
    // pixel_canvas.js と同様に毎回 width/height を代入してキャッシュを強制フラッシュする。
    this._offscreen.width  = sw;
    this._offscreen.height = sh;

    const imageData = this._offCtx.createImageData(sw, sh);
    const data   = imageData.data;
    const pixels = pixelData.pixels;

    for (let i = 0; i < pixels.length; i++) {
      const c = pixels[i];
      // PixelData フォーマット: 0xAARRGGBB → ImageData: R, G, B, A
      const j = i * 4;
      data[j]     = (c >>> 16) & 0xFF;  // R
      data[j + 1] = (c >>> 8)  & 0xFF;  // G
      data[j + 2] =  c         & 0xFF;  // B
      data[j + 3] = (c >>> 24) & 0xFF;  // A
    }

    this._offCtx.putImageData(imageData, 0, 0);

    // ニアレストネイバーで拡大描画
    ctx.save();
    ctx.imageSmoothingEnabled = false;
    ctx.drawImage(this._offscreen, 0, 0, sw, sh, dx, dy, dw, dh);
    ctx.restore();
  }

  // ----------------------------------------------------------------
  // イベント
  // ----------------------------------------------------------------

  /**
   * マウス移動。ホバーボタンの更新。
   * @param {{x: number, y: number}} e  スクリーン座標
   */
  onMouseMove(e) {
    this._hoverBtn = -1;
    for (const btn of this._btnRects) {
      if (e.x >= btn.x && e.x < btn.x + btn.w &&
          e.y >= btn.y && e.y < btn.y + btn.h) {
        this._hoverBtn = btn.index;
        break;
      }
    }
  }

  /**
   * マウスダウン。サイズ切り替えボタンのクリック判定。
   * @param {{x: number, y: number, button: number}} e
   * @param {AppData} appData
   * @returns {boolean} true = consumed
   */
  onMouseDown(e, appData) {
    for (const btn of this._btnRects) {
      if (e.x >= btn.x && e.x < btn.x + btn.w &&
          e.y >= btn.y && e.y < btn.y + btn.h) {
        this._setPreset(btn.index);
        return true;
      }
    }
    return false;
  }

  /**
   * プリセットを切り替える。
   * @param {number} index
   */
  _setPreset(index) {
    const presets = TilePreview.PRESETS;
    if (index < 0 || index >= presets.length) return;
    this._presetIndex = index;
    this.repeatX = presets[index].repeatX;
    this.repeatY = presets[index].repeatY;
  }
}
