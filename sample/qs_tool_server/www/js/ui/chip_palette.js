/**
 * ChipPalette
 * タイルセット内の全チップをサムネイル一覧として表示するコンテンツクラス。
 * UIWindow のコンテンツとして使われることを前提とし、
 * 与えられた (x, y) を起点にチップサムネイルを格子表示する。
 *
 * LayerPanel と同パターン:
 *   const size = palette.getContentSize(appData);
 *   palette.render(ctx, x, y, appData);
 *   palette.onMouseDown(e, appData);  // → true なら消費
 */
class ChipPalette {
  // ---- レイアウト定数 ----
  static PADDING   = 4;
  static CELL_SIZE = 32;     // サムネイル1チップのスクリーン表示サイズ (px)
  static GAP       = 2;      // チップ間の間隔 (px)
  static LABEL_H   = 18;     // 下部ラベル（選択チップ座標）の高さ

  // ---- スタイル定数 ----
  static BORDER_SELECTED = '#7ab0ff';
  static BORDER_HOVER    = 'rgba(120,160,220,0.7)';
  static BORDER_NORMAL   = 'rgba(80,80,80,0.5)';
  static TEXT_COLOR       = '#dcdcdc';
  static TEXT_FONT        = '10px sans-serif';
  static CHECKER_LIGHT    = '#555555';
  static CHECKER_DARK     = '#444444';

  constructor() {
    /** ホバー中のチップ座標 (-1 = なし) */
    this._hoverChip = { col: -1, row: -1 };

    /** 描画時に生成されるヒットテスト矩形の配列 */
    this._cells = [];

    /** render() の描画起点座標キャッシュ */
    this._renderX = 0;
    this._renderY = 0;

    // ---- コールバック ----
    /** @type {((col: number, row: number) => void)|null} */
    this.onChipSelect = null;
    /** @type {((col: number, row: number) => void)|null} */
    this.onChipDoubleClick = null;

    // ---- ダブルクリック検出 ----
    this._lastClickTime = 0;
    this._lastClickChip = { col: -1, row: -1 };
    /** @type {number} ダブルクリック判定の最大間隔 (ms) */
    this._dblClickThresh = 400;

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
    const { PADDING, CELL_SIZE, GAP, LABEL_H } = ChipPalette;
    if (!appData || appData.editMode !== 'tileset' || !appData.tilesetData) {
      // タイルセットモードでない場合の最小サイズ
      return {
        w: PADDING * 2 + CELL_SIZE * 4 + GAP * 3,
        h: PADDING * 2 + CELL_SIZE + LABEL_H,
      };
    }
    const td = appData.tilesetData;
    return {
      w: PADDING * 2 + td.columns * CELL_SIZE + Math.max(0, td.columns - 1) * GAP,
      h: PADDING * 2 + td.rows * CELL_SIZE + Math.max(0, td.rows - 1) * GAP + LABEL_H,
    };
  }

  // ----------------------------------------------------------------
  // 描画
  // ----------------------------------------------------------------

  /**
   * チップパレットを描画する。
   * @param {CanvasRenderingContext2D} ctx
   * @param {number}  x       描画起点 X
   * @param {number}  y       描画起点 Y
   * @param {AppData} appData
   */
  render(ctx, x, y, appData) {
    const { PADDING, CELL_SIZE, GAP, LABEL_H,
            BORDER_SELECTED, BORDER_HOVER, BORDER_NORMAL,
            TEXT_COLOR, TEXT_FONT } = ChipPalette;

    this._renderX = x;
    this._renderY = y;
    this._cells   = [];

    if (!appData || appData.editMode !== 'tileset' || !appData.tilesetData) return;

    const td  = appData.tilesetData;
    const sel = appData.selectedChip;

    // ---- チップサムネイル格子描画 ----
    for (let r = 0; r < td.rows; r++) {
      for (let c = 0; c < td.columns; c++) {
        const cx = x + PADDING + c * (CELL_SIZE + GAP);
        const cy = y + PADDING + r * (CELL_SIZE + GAP);

        this._cells.push({ x: cx, y: cy, w: CELL_SIZE, h: CELL_SIZE, col: c, row: r });

        // 市松模様背景（透明チップ用）
        this._drawChecker(ctx, cx, cy, CELL_SIZE, CELL_SIZE);

        // チップの合成結果をサムネイル描画
        const chipPd = td.compositeChip(c, r);
        if (chipPd) {
          this._drawPixelDataScaled(ctx, chipPd, cx, cy, CELL_SIZE, CELL_SIZE);
        }

        // 枠線
        const isSelected = sel.col === c && sel.row === r;
        const isHover    = this._hoverChip.col === c && this._hoverChip.row === r;

        ctx.strokeStyle = isSelected ? BORDER_SELECTED
                        : isHover    ? BORDER_HOVER
                        :              BORDER_NORMAL;
        ctx.lineWidth   = isSelected ? 2 : 1;
        ctx.strokeRect(
          cx + (isSelected ? 0.5 : 0.5),
          cy + (isSelected ? 0.5 : 0.5),
          CELL_SIZE - 1,
          CELL_SIZE - 1,
        );
      }
    }

    // ---- 下部ラベル: 選択チップ座標 + タイルインデックス ----
    const labelY = y + PADDING + td.rows * (CELL_SIZE + GAP) + 2;

    ctx.font         = TEXT_FONT;
    ctx.fillStyle    = TEXT_COLOR;
    ctx.textAlign    = 'left';
    ctx.textBaseline = 'top';
    ctx.fillText(`チップ: (${sel.col}, ${sel.row})`, x + PADDING, labelY);

    const idx = sel.row * td.columns + sel.col;
    const { w: totalW } = this.getContentSize(appData);
    ctx.textAlign = 'right';
    ctx.fillText(`#${idx}`, x + totalW - PADDING, labelY);

    // リセット
    ctx.textAlign    = 'left';
    ctx.textBaseline = 'alphabetic';
  }

  /**
   * 市松模様を描画する（透明チップの背景用）。
   */
  _drawChecker(ctx, x, y, w, h) {
    const size = 4;
    const { CHECKER_LIGHT, CHECKER_DARK } = ChipPalette;
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
   * @param {CanvasRenderingContext2D} ctx
   * @param {PixelData} pixelData
   * @param {number} dx  描画先 X
   * @param {number} dy  描画先 Y
   * @param {number} dw  描画先幅
   * @param {number} dh  描画先高さ
   */
  _drawPixelDataScaled(ctx, pixelData, dx, dy, dw, dh) {
    const sw = pixelData.width;
    const sh = pixelData.height;
    if (sw === 0 || sh === 0) return;

    // オフスクリーン canvas の生成 or サイズ変更
    if (!this._offscreen || this._offscreen.width !== sw || this._offscreen.height !== sh) {
      this._offscreen = document.createElement('canvas');
      this._offscreen.width  = sw;
      this._offscreen.height = sh;
      this._offCtx = this._offscreen.getContext('2d');
    }

    const imageData = this._offCtx.createImageData(sw, sh);
    const buf    = new Uint8Array(imageData.data.buffer);
    const pixels = pixelData.pixels;

    for (let i = 0; i < pixels.length; i++) {
      const c = pixels[i];
      // PixelData フォーマット: 0xAARRGGBB → ImageData: R, G, B, A
      const j = i * 4;
      buf[j]     = (c >> 16) & 0xFF;  // R
      buf[j + 1] = (c >> 8)  & 0xFF;  // G
      buf[j + 2] =  c        & 0xFF;  // B
      buf[j + 3] = (c >> 24) & 0xFF;  // A
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
   * マウス移動。ホバーチップの更新。
   * @param {{x: number, y: number}} e  スクリーン座標
   */
  onMouseMove(e) {
    this._hoverChip = { col: -1, row: -1 };

    for (const cell of this._cells) {
      if (e.x >= cell.x && e.x < cell.x + cell.w &&
          e.y >= cell.y && e.y < cell.y + cell.h) {
        this._hoverChip = { col: cell.col, row: cell.row };
        break;
      }
    }
  }

  /**
   * マウスダウン。チップ選択 / ダブルクリック判定。
   * @param {{x: number, y: number, button: number}} e
   * @param {AppData} appData
   * @returns {boolean} true = consumed
   */
  onMouseDown(e, appData) {
    const now = Date.now();

    for (const cell of this._cells) {
      if (e.x >= cell.x && e.x < cell.x + cell.w &&
          e.y >= cell.y && e.y < cell.y + cell.h) {

        // ダブルクリック判定
        if (cell.col === this._lastClickChip.col &&
            cell.row === this._lastClickChip.row &&
            now - this._lastClickTime < this._dblClickThresh) {
          this.onChipDoubleClick?.(cell.col, cell.row);
          this._lastClickChip = { col: -1, row: -1 };
          return true;
        }

        // シングルクリック: チップ選択
        this._lastClickTime = now;
        this._lastClickChip = { col: cell.col, row: cell.row };
        this.onChipSelect?.(cell.col, cell.row);
        return true;
      }
    }

    return false;
  }
}
