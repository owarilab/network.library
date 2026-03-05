/**
 * PixelCanvas
 * PixelData を画面中央に描画し、マウス操作でピクセル座標を
 * コールバックでシーン側に渡す描画ビュークラス。
 *
 * 描画方式:
 *   - オフスクリーン canvas に ImageData でピクセルを書き込む
 *   - メイン canvas へ `drawImage` で拡大描画 (imageSmoothingEnabled = false)
 *   - scale >= 4 でグリッド線を表示
 *
 * コールバック:
 *   pixelCanvas.onPixelDown = (px, py, button, appData) => { ... }
 *   pixelCanvas.onPixelMove = (px, py, button, appData) => { ... }
 *   pixelCanvas.onPixelUp   = (px, py, button, appData) => { ... }
 */
class PixelCanvas {
  /** グリッド線を表示し始めるスケール閾値 */
  static GRID_THRESHOLD = 4;
  /** ドット枠線の色 */
  static BORDER_COLOR   = '#222222';
  /** グリッド線の色 */
  static GRID_COLOR     = 'rgba(0,0,0,0.18)';
  /** ズーム倍率ステップ一覧 (ピクセルアート向けの細かいステップ) */
  static SCALE_STEPS = [1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 20, 24, 28, 32, 40, 48, 56, 64];
  /** ズーム倍率の下限・上限 (SCALE_STEPS の先頭・末尾) */
  static SCALE_MIN = 1;
  static SCALE_MAX = 64;
  /** 透明ピクセルを表す市松模様の明色・暗色 */
  static CHECKER_LIGHT = '#cccccc';
  static CHECKER_DARK  = '#999999';
  /** 市松模様 1 マスの大きさ (px) ※スクリーン座標 */
  static CHECKER_SIZE  = 8;

  constructor() {
    /**
     * 表示倍率 (1 ドット = scale × scale px)
     * @type {number}
     */
    this.scale = 16;

    /**
     * ドット絵左上のスクリーン座標 (render() 内で自動算出)
     * @type {number}
     */
    this.offsetX = 0;
    this.offsetY = 0;

    /** ドラッグ中フラグ */
    this._isDragging = false;
    this._dragButton = -1;

    /**
     * パンオフセット (スクリーン座標 px)。
     * 中央配置の位置から何ピクセルずらすか。
     * @type {number}
     */
    this.panX = 0;
    this.panY = 0;

    /**
     * グリッド線を表示するかどうか
     * @type {boolean}
     */
    this.showGrid = true;

    /** パン操作中フラグ */
    this._isPanning    = false;
    this._panStartX    = 0;
    this._panStartY    = 0;
    this._panOriginX   = 0;
    this._panOriginY   = 0;

    /** オフスクリーン canvas (ピクセルデータをそのまま書き込む) */
    this._offscreen = document.createElement('canvas');
    this._offCtx    = this._offscreen.getContext('2d');

    /**
     * 描画内容が変更されたかどうか (true のときオフスクリーンを再生成)
     * PixelData を変更したら markDirty() を呼ぶこと
     */
    this._dirty = true;

    /**
     * PixelData の最後に確認したサイズ (サイズ変更を検出するため)
     * @type {{ w: number, h: number }}
     */
    this._lastSize = { w: 0, h: 0 };

    // ---- コールバック ----
    /** @type {((px:number,py:number,button:number,appData:AppData)=>void)|null} */
    this.onPixelDown = null;
    /** @type {((px:number,py:number,button:number,appData:AppData)=>void)|null} */
    this.onPixelMove = null;
    /** @type {((px:number,py:number,button:number,appData:AppData)=>void)|null} */
    this.onPixelUp   = null;
  }

  // ----------------------------------------------------------------
  // 描画
  // ----------------------------------------------------------------

  /**
   * ピクセルデータを画面中央に描画する。
   * EditorScene.render() から毎フレーム呼ぶ。
   *
   * @param {CanvasRenderingContext2D} ctx
   * @param {HTMLCanvasElement}       canvas
   * @param {PixelData}               pixelData
   */
  render(ctx, canvas, pixelData) {
    if (!pixelData || !pixelData.pixels) return;

    const { width: pw, height: ph } = pixelData;

    // ---- 中央配置 + パンオフセット (メニューバー領域を除く) ----
    const menuH  = MenuBar.HEIGHT;
    const drawW  = pw * this.scale;
    const drawH  = ph * this.scale;
    const areaH  = canvas.height - menuH;
    this.offsetX = Math.round((canvas.width - drawW) / 2) + this.panX;
    this.offsetY = Math.round((areaH - drawH) / 2) + menuH + this.panY;

    // ---- 透明市松模様の背景を描画 ----
    this._drawChecker(ctx, this.offsetX, this.offsetY, drawW, drawH);

    // ---- オフスクリーンが古い or サイズ変更なら更新 ----
    if (this._dirty ||
        this._lastSize.w !== pw ||
        this._lastSize.h !== ph) {
      this._updateOffscreen(pixelData);
      this._dirty = false;
      this._lastSize = { w: pw, h: ph };
    }

    // ---- 拡大描画（ニアレストネイバー） ----
    ctx.save();
    ctx.imageSmoothingEnabled = false;
    ctx.drawImage(this._offscreen, this.offsetX, this.offsetY, drawW, drawH);
    ctx.restore();

    // ---- グリッド線 ----
    if (this.showGrid && this.scale >= PixelCanvas.GRID_THRESHOLD) {
      this._drawGrid(ctx, pw, ph);
    }

    // ---- 枠線 ----
    ctx.save();
    ctx.strokeStyle = PixelCanvas.BORDER_COLOR;
    ctx.lineWidth   = 1;
    ctx.strokeRect(this.offsetX - 0.5, this.offsetY - 0.5, drawW + 1, drawH + 1);
    ctx.restore();
  }

  /**
   * オフスクリーン canvas を PixelData で更新する。
   * @param {PixelData} pixelData
   */
  _updateOffscreen(pixelData) {
    const { width, height, pixels } = pixelData;
    this._offscreen.width  = width;
    this._offscreen.height = height;

    const imgData = this._offCtx.createImageData(width, height);
    const buf     = imgData.data; // Uint8ClampedArray, RGBA order

    for (let i = 0; i < pixels.length; i++) {
      const c = pixels[i];
      const j = i * 4;
      buf[j]   = (c >>> 16) & 0xff; // R
      buf[j+1] = (c >>>  8) & 0xff; // G
      buf[j+2] =  c         & 0xff; // B
      buf[j+3] = (c >>> 24) & 0xff; // A
    }
    this._offCtx.putImageData(imgData, 0, 0);
  }

  /**
   * 透明ピクセル用の市松模様を描画する。
   */
  _drawChecker(ctx, x, y, w, h) {
    const cs = PixelCanvas.CHECKER_SIZE;
    ctx.save();
    ctx.beginPath();
    ctx.rect(x, y, w, h);
    ctx.clip();

    for (let row = 0; row * cs < h; row++) {
      for (let col = 0; col * cs < w; col++) {
        ctx.fillStyle = ((row + col) % 2 === 0)
          ? PixelCanvas.CHECKER_LIGHT
          : PixelCanvas.CHECKER_DARK;
        ctx.fillRect(x + col * cs, y + row * cs, cs, cs);
      }
    }
    ctx.restore();
  }

  /**
   * ピクセルグリッド線を描画する。
   */
  _drawGrid(ctx, pw, ph) {
    const { offsetX: ox, offsetY: oy, scale } = this;
    ctx.save();
    ctx.strokeStyle = PixelCanvas.GRID_COLOR;
    ctx.lineWidth   = 1;
    ctx.beginPath();

    // 縦線
    for (let x = 1; x < pw; x++) {
      const sx = ox + x * scale + 0.5;
      ctx.moveTo(sx, oy);
      ctx.lineTo(sx, oy + ph * scale);
    }
    // 横線
    for (let y = 1; y < ph; y++) {
      const sy = oy + y * scale + 0.5;
      ctx.moveTo(ox,              sy);
      ctx.lineTo(ox + pw * scale, sy);
    }
    ctx.stroke();
    ctx.restore();
  }

  // ----------------------------------------------------------------
  // 座標変換
  // ----------------------------------------------------------------

  /**
   * スクリーン座標 → PixelData 上のドット座標に変換する。
   * ドット外なら null を返す。
   * @param {number}    sx
   * @param {number}    sy
   * @param {PixelData} pixelData
   * @returns {{ x:number, y:number }|null}
   */
  screenToPixel(sx, sy, pixelData) {
    if (!pixelData) return null;
    const px = Math.floor((sx - this.offsetX) / this.scale);
    const py = Math.floor((sy - this.offsetY) / this.scale);
    if (px < 0 || px >= pixelData.width ||
        py < 0 || py >= pixelData.height) return null;
    return { x: px, y: py };
  }

  // ----------------------------------------------------------------
  // ズーム
  // ----------------------------------------------------------------

  /**
   * スクリーン座標を中心にズームする。
   * @param {number} delta   - 正で拡大、負で縮小
   * @param {number} centerX - ズームの中心 X (スクリーン座標)
   * @param {number} centerY - ズームの中心 Y (スクリーン座標)
   */
  // ----------------------------------------------------------------
  // パン (スペース+左ドラッグ)
  // ----------------------------------------------------------------

  /**
   * パン操作を開始する。
   * @param {number} sx スクリーン座標 X
   * @param {number} sy スクリーン座標 Y
   */
  startPan(sx, sy) {
    this._isPanning  = true;
    this._panStartX  = sx;
    this._panStartY  = sy;
    this._panOriginX = this.panX;
    this._panOriginY = this.panY;
  }

  /**
   * パン操作中の移動。
   * @param {number} sx スクリーン座標 X
   * @param {number} sy スクリーン座標 Y
   */
  movePan(sx, sy) {
    if (!this._isPanning) return;
    this.panX = this._panOriginX + (sx - this._panStartX);
    this.panY = this._panOriginY + (sy - this._panStartY);
  }

  /** パン操作を終了する。 */
  endPan() {
    this._isPanning = false;
  }

  /** パンオフセットを中央にリセットする。 */
  resetPan() {
    this.panX = 0;
    this.panY = 0;
  }

  /**
   * パン・ズームをデフォルト状態にリセットする。
   * ファイル読み込み・新規作成後に呼ぶ。
   */
  resetView() {
    this.panX  = 0;
    this.panY  = 0;
    this.scale = 16;
  }

  // ----------------------------------------------------------------
  // ズーム
  // ----------------------------------------------------------------

  zoom(delta, centerX, centerY) {
    const steps = PixelCanvas.SCALE_STEPS;
    // 現在スケールに最も近いインデックスを求める
    let idx = steps.findIndex(s => s >= this.scale);
    if (idx < 0) idx = steps.length - 1;
    // ちょうど現在値より大きい値が見つかった場合 → 現在値そのもののインデックスに揃える
    else if (steps[idx] > this.scale && idx > 0) idx -= 1;

    const next = delta > 0
      ? Math.min(steps.length - 1, idx + 1)
      : Math.max(0, idx - 1);
    this.scale = steps[next];
  }

  // ----------------------------------------------------------------
  // dirty フラグ
  // ----------------------------------------------------------------

  /** PixelData を書き換えたら必ず呼ぶこと */
  markDirty() {
    this._dirty = true;
  }

  // ----------------------------------------------------------------
  // 入力ハンドラ (EditorScene から Input コールバック経由で呼ぶ)
  // ----------------------------------------------------------------

  /**
   * @param {{ x:number, y:number, button:number }} e
   * @param {AppData} appData
   */
  onMouseDown(e, appData) {
    const pos = this.screenToPixel(e.x, e.y, appData?.pixelData);
    if (!pos) return;
    this._isDragging = true;
    this._dragButton = e.button;
    this.onPixelDown?.(pos.x, pos.y, e.button, appData);
  }

  /**
   * @param {{ x:number, y:number, buttons:number }} e
   * @param {AppData} appData
   */
  onMouseMove(e, appData) {
    if (!this._isDragging) return;
    const pos = this.screenToPixel(e.x, e.y, appData?.pixelData);
    if (!pos) return;
    this.onPixelMove?.(pos.x, pos.y, this._dragButton, appData);
  }

  /**
   * @param {{ x:number, y:number, button:number }} e
   * @param {AppData} appData
   */
  onMouseUp(e, appData) {
    if (!this._isDragging) return;
    const pos = this.screenToPixel(e.x, e.y, appData?.pixelData);
    this._isDragging = false;
    this._dragButton = -1;
    if (pos) {
      this.onPixelUp?.(pos.x, pos.y, e.button, appData);
    }
  }
}
