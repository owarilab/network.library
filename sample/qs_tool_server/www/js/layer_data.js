/**
 * LayerData
 * 複数の PixelData レイヤーを管理し、ボトムアップでアルファ合成を行うクラス。
 *
 * 各レイヤーは { pixelData, name, visible, opacity } のオブジェクト。
 * layers[0] が最背面、layers[length-1] が最前面。
 *
 * 合成結果は _composite にキャッシュされ、markCompositeDirty() で無効化する。
 */
class LayerData {
  constructor() {
    /**
     * レイヤー配列。[0] が最背面。
     * @type {Array<{pixelData: PixelData, name: string, visible: boolean, opacity: number}>}
     */
    this.layers = [];

    /** アクティブレイヤーのインデックス */
    this.activeIndex = 0;

    /** キャンバスサイズ（全レイヤー共通） */
    this.width  = 0;
    this.height = 0;

    /** 合成済みキャッシュ */
    this._composite    = new PixelData();
    this._compositeDirty = true;
  }

  // ----------------------------------------------------------------
  // 初期化
  // ----------------------------------------------------------------

  /**
   * 指定サイズで1レイヤー構成に初期化する。
   * @param {number} width
   * @param {number} height
   * @param {number} [fillColor=0x00000000]
   */
  init(width, height, fillColor = 0x00000000) {
    this.width  = width;
    this.height = height;
    this.layers = [];
    this.activeIndex = 0;

    const pd = new PixelData();
    pd.createPixelData(width, height, fillColor);
    this.layers.push({
      pixelData: pd,
      name:      'レイヤー 1',
      visible:   true,
      opacity:   255,
    });

    this._composite.createPixelData(width, height);
    this._compositeDirty = true;
  }

  // ----------------------------------------------------------------
  // レイヤー操作
  // ----------------------------------------------------------------

  /**
   * 新規レイヤーをアクティブレイヤーの上に追加する。
   * @param {string} [name] レイヤー名（省略時は自動命名）
   * @returns {number} 追加されたレイヤーのインデックス
   */
  addLayer(name) {
    const n = this.layers.length + 1;
    const pd = new PixelData();
    pd.createPixelData(this.width, this.height, 0x00000000);
    const insertAt = this.activeIndex + 1;
    this.layers.splice(insertAt, 0, {
      pixelData: pd,
      name:      name || `レイヤー ${n}`,
      visible:   true,
      opacity:   255,
    });
    this.activeIndex = insertAt;
    this._compositeDirty = true;
    return insertAt;
  }

  /**
   * 指定インデックスのレイヤーを削除する。
   * レイヤーが1枚しかない場合は削除しない。
   * @param {number} index
   * @returns {boolean} 削除できたか
   */
  removeLayer(index) {
    if (this.layers.length <= 1) return false;
    if (index < 0 || index >= this.layers.length) return false;
    this.layers.splice(index, 1);
    // activeIndex の補正
    if (this.activeIndex >= this.layers.length) {
      this.activeIndex = this.layers.length - 1;
    } else if (this.activeIndex > index) {
      this.activeIndex--;
    }
    this._compositeDirty = true;
    return true;
  }

  /**
   * レイヤーを移動する。
   * @param {number} from 移動元インデックス
   * @param {number} to   移動先インデックス
   */
  moveLayer(from, to) {
    if (from === to) return;
    if (from < 0 || from >= this.layers.length) return;
    if (to   < 0 || to   >= this.layers.length) return;
    const layer = this.layers.splice(from, 1)[0];
    this.layers.splice(to, 0, layer);
    // activeIndex の追従
    if (this.activeIndex === from) {
      this.activeIndex = to;
    } else {
      if (this.activeIndex > from) this.activeIndex--;
      if (this.activeIndex >= to)  this.activeIndex++;
    }
    this._compositeDirty = true;
  }

  /**
   * アクティブレイヤーを設定する。
   * @param {number} index
   */
  setActiveIndex(index) {
    if (index >= 0 && index < this.layers.length) {
      this.activeIndex = index;
    }
  }

  /**
   * アクティブレイヤーの PixelData を返す。
   * @returns {PixelData}
   */
  getActiveLayer() {
    return this.layers[this.activeIndex]?.pixelData || null;
  }

  /**
   * レイヤーの表示/非表示をトグルする。
   * @param {number} index
   */
  toggleVisibility(index) {
    if (index >= 0 && index < this.layers.length) {
      this.layers[index].visible = !this.layers[index].visible;
      this._compositeDirty = true;
    }
  }

  /**
   * レイヤーの不透明度を設定する。
   * @param {number} index
   * @param {number} opacity 0〜255
   */
  setOpacity(index, opacity) {
    if (index >= 0 && index < this.layers.length) {
      this.layers[index].opacity = Math.max(0, Math.min(255, opacity | 0));
      this._compositeDirty = true;
    }
  }

  // ----------------------------------------------------------------
  // 合成
  // ----------------------------------------------------------------

  /** 合成キャッシュを無効化する。レイヤーのピクセルを変更したら呼ぶ。 */
  markCompositeDirty() {
    this._compositeDirty = true;
  }

  /**
   * 全レイヤーをボトムアップでアルファ合成した結果を返す。
   * キャッシュが有効な場合は再計算しない。
   * @returns {PixelData}
   */
  composite() {
    if (!this._compositeDirty) return this._composite;

    // サイズが変わっていたら再確保
    if (this._composite.width !== this.width ||
        this._composite.height !== this.height) {
      this._composite.createPixelData(this.width, this.height);
    }

    const dst = this._composite.pixels;
    const len = this.width * this.height;

    // 透明でクリア
    dst.fill(0);

    for (let li = 0; li < this.layers.length; li++) {
      const layer = this.layers[li];
      if (!layer.visible || layer.opacity === 0) continue;

      const src     = layer.pixelData.pixels;
      const layerOp = layer.opacity;  // 0〜255

      for (let i = 0; i < len; i++) {
        const sc = src[i];
        const sa = ((sc >>> 24) & 0xff);
        if (sa === 0) continue;  // 完全透明はスキップ

        // レイヤー不透明度を適用した実効アルファ
        const srcA = (sa * layerOp + 127) / 255 | 0;
        if (srcA === 0) continue;

        const dc = dst[i];
        const dstA = (dc >>> 24) & 0xff;

        if (dstA === 0) {
          // 背景が完全に透明 → ソースをそのまま置く
          dst[i] = (((srcA & 0xff) << 24) |
                    (((sc >>> 16) & 0xff) << 16) |
                    (((sc >>> 8)  & 0xff) << 8)  |
                    (sc & 0xff)) >>> 0;
        } else if (srcA === 255) {
          // ソースが完全不透明 → 上書き
          dst[i] = ((0xff << 24) |
                    (((sc >>> 16) & 0xff) << 16) |
                    (((sc >>> 8)  & 0xff) << 8)  |
                    (sc & 0xff)) >>> 0;
        } else {
          // 一般的な Porter-Duff "source over" 合成
          const srcR = (sc >>> 16) & 0xff;
          const srcG = (sc >>> 8)  & 0xff;
          const srcB =  sc         & 0xff;
          const dstR = (dc >>> 16) & 0xff;
          const dstG = (dc >>> 8)  & 0xff;
          const dstB =  dc         & 0xff;

          // outA = srcA + dstA * (1 - srcA/255)
          const outA = srcA + ((dstA * (255 - srcA) + 127) / 255 | 0);
          if (outA === 0) {
            dst[i] = 0;
          } else {
            const outR = ((srcR * srcA + dstR * dstA * (255 - srcA) / 255 + 127) / outA) | 0;
            const outG = ((srcG * srcA + dstG * dstA * (255 - srcA) / 255 + 127) / outA) | 0;
            const outB = ((srcB * srcA + dstB * dstA * (255 - srcA) / 255 + 127) / outA) | 0;
            dst[i] = (((outA & 0xff) << 24) |
                      ((outR & 0xff) << 16) |
                      ((outG & 0xff) << 8)  |
                      (outB  & 0xff)) >>> 0;
          }
        }
      }
    }

    this._compositeDirty = false;
    return this._composite;
  }
}
