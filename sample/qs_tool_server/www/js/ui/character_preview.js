/**
 * CharacterPreview
 * キャラクターチップセットの向き切替と歩行アニメのプレビュー内容。
 * 向きボタン（下 / 左 / 右 / 上）と再生・停止ボタン、アニメ表示領域を持つ。
 *
 * 歩行フレームは CharacterChipset.WALK_CYCLE を WALK_STEP_MS ごとに進める。
 * 停止中は立ち（フレーム 0）を表示する。
 */
class CharacterPreview {
  static BUTTON_W = 44;
  static BUTTON_H = 24;
  static PLAY_H = 22;
  static GAP = 4;
  static PAD = 8;
  static AREA_SIZE = 128;

  static DIRECTION_LABELS = { down: '下', left: '左', right: '右', up: '上' };

  constructor() {
    /** @type {string} 表示中の向き */
    this._direction = 'down';
    /** @type {boolean} 歩行アニメ再生中か */
    this._playing = true;
    /** ホバー中の要素キー */
    this._hover = null;
    /** 各ボタンの描画座標（絶対座標） */
    this._rects = {};

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
   * @returns {{ w: number, h: number }}
   */
  getContentSize() {
    const { BUTTON_W, BUTTON_H, PLAY_H, GAP, PAD, AREA_SIZE } = CharacterPreview;
    const dirs = CharacterChipset.DIRECTIONS.length;
    return {
      w: PAD * 2 + dirs * BUTTON_W + (dirs - 1) * GAP,
      h: PAD + BUTTON_H + GAP + PLAY_H + GAP + AREA_SIZE + PAD,
    };
  }

  // ----------------------------------------------------------------
  // 描画
  // ----------------------------------------------------------------

  /**
   * @param {CanvasRenderingContext2D} ctx
   * @param {number} x  コンテンツ左上 X（絶対座標）
   * @param {number} y  コンテンツ左上 Y（絶対座標）
   * @param {AppData} appData
   */
  render(ctx, x, y, appData) {
    const { BUTTON_W, BUTTON_H, PLAY_H, GAP, PAD, AREA_SIZE } = CharacterPreview;
    const size = this.getContentSize();
    this._rects = {};

    let by = y + PAD;
    CharacterChipset.DIRECTIONS.forEach((dir, index) => {
      const r = { x: x + PAD + index * (BUTTON_W + GAP), y: by, w: BUTTON_W, h: BUTTON_H };
      const key = `dir_${dir}`;
      this._rects[key] = r;
      this._drawButton(ctx, r, CharacterPreview.DIRECTION_LABELS[dir], this._direction === dir, this._hover === key);
    });

    by += BUTTON_H + GAP;
    const playR = { x: x + PAD, y: by, w: size.w - PAD * 2, h: PLAY_H };
    this._rects.play = playR;
    this._drawButton(ctx, playR, this._playing ? '停止' : '再生', false, this._hover === 'play');

    by += PLAY_H + GAP;
    const areaR = { x: x + PAD, y: by, w: size.w - PAD * 2, h: AREA_SIZE };
    this._drawAnimation(ctx, areaR, appData);
  }

  /**
   * 現在の向きとフレームを領域の中央に拡大表示する。
   * @param {CanvasRenderingContext2D} ctx
   * @param {{x:number,y:number,w:number,h:number}} area
   * @param {AppData} appData
   */
  _drawAnimation(ctx, area, appData) {
    ctx.fillStyle = '#ffffff';
    ctx.fillRect(area.x, area.y, area.w, area.h);
    ctx.strokeStyle = '#a0a0a0';
    ctx.lineWidth = 1;
    ctx.strokeRect(area.x + 0.5, area.y + 0.5, area.w - 1, area.h - 1);

    const td = appData?.tilesetData;
    if (!td) return;

    const frameIndex = this._currentFrameIndex();
    const layerData = CharacterChipset.getFrameLayerData(td, this._direction, frameIndex);
    if (!layerData) return;

    const pd = layerData.composite();
    if (!pd || pd.width === 0 || pd.height === 0) return;

    const scale = Math.max(1, Math.floor(Math.min(area.w, area.h) / Math.max(pd.width, pd.height)));
    const dw = pd.width * scale;
    const dh = pd.height * scale;
    const dx = area.x + Math.floor((area.w - dw) / 2);
    const dy = area.y + Math.floor((area.h - dh) / 2);
    this._drawPixelData(ctx, pd, dx, dy, dw, dh);
  }

  /**
   * 再生中は時刻から歩行サイクルの位置を求め、停止中は立ち（フレーム 0）にする。
   * @returns {number}
   */
  _currentFrameIndex() {
    if (!this._playing) return 0;
    const cycle = CharacterChipset.WALK_CYCLE;
    const step = Math.floor(performance.now() / CharacterChipset.WALK_STEP_MS);
    return cycle[step % cycle.length];
  }

  /**
   * PixelData を拡大表示する（TilePreview と同じ方式）。
   * @param {CanvasRenderingContext2D} ctx
   * @param {PixelData} pixelData
   * @param {number} dx
   * @param {number} dy
   * @param {number} dw
   * @param {number} dh
   */
  _drawPixelData(ctx, pixelData, dx, dy, dw, dh) {
    const sw = pixelData.width;
    const sh = pixelData.height;
    if (!this._offscreen) {
      this._offscreen = document.createElement('canvas');
      this._offCtx = this._offscreen.getContext('2d');
    }
    // Safari のテクスチャキャッシュ対策として毎回サイズを代入する
    this._offscreen.width = sw;
    this._offscreen.height = sh;

    const imageData = this._offCtx.createImageData(sw, sh);
    const data = imageData.data;
    const pixels = pixelData.pixels;
    for (let i = 0; i < pixels.length; i++) {
      const c = pixels[i];
      const j = i * 4;
      data[j] = (c >>> 16) & 0xFF;
      data[j + 1] = (c >>> 8) & 0xFF;
      data[j + 2] = c & 0xFF;
      data[j + 3] = (c >>> 24) & 0xFF;
    }
    this._offCtx.putImageData(imageData, 0, 0);

    ctx.save();
    ctx.imageSmoothingEnabled = false;
    ctx.drawImage(this._offscreen, 0, 0, sw, sh, dx, dy, dw, dh);
    ctx.restore();
  }

  /**
   * @param {CanvasRenderingContext2D} ctx
   * @param {{x:number,y:number,w:number,h:number}} r
   * @param {string} label
   * @param {boolean} selected
   * @param {boolean} hovered
   */
  _drawButton(ctx, r, label, selected, hovered) {
    ctx.fillStyle = selected ? '#0078d7' : (hovered ? '#dcdad5' : '#e8e6e1');
    ctx.strokeStyle = selected ? '#005a9e' : '#a0a0a0';
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.roundRect(r.x, r.y, r.w, r.h, 3);
    ctx.fill();
    ctx.stroke();

    ctx.fillStyle = selected ? '#ffffff' : '#000000';
    ctx.font = '12px sans-serif';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillText(label, r.x + r.w / 2, r.y + r.h / 2);
    ctx.textAlign = 'left';
    ctx.textBaseline = 'alphabetic';
  }

  // ----------------------------------------------------------------
  // 入力
  // ----------------------------------------------------------------

  /**
   * @param {{x:number, y:number}} e
   */
  onMouseMove(e) {
    this._hover = this._hitTest(e.x, e.y);
  }

  /**
   * @param {{x:number, y:number, button:number}} e
   * @returns {boolean} true = consumed
   */
  onMouseDown(e) {
    if (e.button !== 0) return false;
    const key = this._hitTest(e.x, e.y);
    if (!key) return false;
    if (key === 'play') {
      this._playing = !this._playing;
      return true;
    }
    this._direction = key.slice('dir_'.length);
    return true;
  }

  /**
   * @param {number} x
   * @param {number} y
   * @returns {string|null}
   */
  _hitTest(x, y) {
    for (const [key, r] of Object.entries(this._rects)) {
      if (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h) return key;
    }
    return null;
  }
}
