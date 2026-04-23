/**
 * LayerData
 * 複数の PixelData レイヤーを管理し、ボトムアップでアルファ合成を行うクラス。
 *
 * 各レイヤーは { pixelData, name, visible, opacity, locked } のオブジェクト。
 * layers[0] が最背面、layers[length-1] が最前面。
 *
 * 合成結果は _composite にキャッシュされ、markCompositeDirty() で無効化する。
 */
class LayerData {
  constructor() {
    /**
     * レイヤー配列。[0] が最背面。
    * @type {Array<{pixelData: PixelData, name: string, visible: boolean, opacity: number, locked: boolean}>}
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

  /**
   * 新しいレイヤーエントリを作成する。
   * @param {PixelData} pixelData
   * @param {string} name
   * @param {boolean} [visible=true]
   * @param {number} [opacity=255]
   * @param {boolean} [locked=false]
   * @returns {{pixelData: PixelData, name: string, visible: boolean, opacity: number, locked: boolean}}
   */
  _createLayerEntry(pixelData, name, visible = true, opacity = 255, locked = false) {
    return {
      pixelData,
      name: this._sanitizeLayerName(name, `レイヤー ${this.layers.length + 1}`),
      visible: visible !== false,
      opacity: Math.max(0, Math.min(255, opacity | 0)),
      locked: !!locked,
    };
  }

  /**
   * レイヤー名を正規化する。
   * @param {string} name
   * @param {string} fallback
   * @returns {string}
   */
  _sanitizeLayerName(name, fallback = 'レイヤー') {
    if (typeof name !== 'string') return fallback;
    const trimmed = name.trim();
    if (!trimmed) return fallback;
    return trimmed.slice(0, 32);
  }

  /**
   * PixelData をディープコピーする。
   * @param {PixelData} src
   * @returns {PixelData}
   */
  _clonePixelData(src) {
    const pd = new PixelData();
    pd.createPixelData(src.width, src.height);
    pd.pixels.set(src.pixels);
    return pd;
  }

  /**
   * レイヤーエントリをディープコピーする。
   * @param {{pixelData: PixelData, name: string, visible: boolean, opacity: number, locked: boolean}} layer
   * @param {string} [nameOverride]
   * @returns {{pixelData: PixelData, name: string, visible: boolean, opacity: number, locked: boolean}}
   */
  _cloneLayerEntry(layer, nameOverride) {
    return this._createLayerEntry(
      this._clonePixelData(layer.pixelData),
      typeof nameOverride === 'string' ? nameOverride : layer.name,
      layer.visible,
      layer.opacity,
      layer.locked
    );
  }

  /**
   * src を layerOpacity 付きで dst に source-over 合成する。
   * @param {Uint32Array} dst
   * @param {Uint32Array} src
   * @param {number} layerOpacity
   */
  _blendPixels(dst, src, layerOpacity) {
    const effectiveOpacity = Math.max(0, Math.min(255, layerOpacity | 0));
    if (effectiveOpacity === 0) return;

    const len = this.width * this.height;
    for (let i = 0; i < len; i++) {
      const sc = src[i];
      const sa = ((sc >>> 24) & 0xff);
      if (sa === 0) continue;

      const srcA = (sa * effectiveOpacity + 127) / 255 | 0;
      if (srcA === 0) continue;

      const dc = dst[i];
      const dstA = (dc >>> 24) & 0xff;

      if (dstA === 0) {
        dst[i] = (((srcA & 0xff) << 24) |
                  (((sc >>> 16) & 0xff) << 16) |
                  (((sc >>> 8)  & 0xff) << 8)  |
                  (sc & 0xff)) >>> 0;
      } else if (srcA === 255) {
        dst[i] = ((0xff << 24) |
                  (((sc >>> 16) & 0xff) << 16) |
                  (((sc >>> 8)  & 0xff) << 8)  |
                  (sc & 0xff)) >>> 0;
      } else {
        const srcR = (sc >>> 16) & 0xff;
        const srcG = (sc >>> 8)  & 0xff;
        const srcB =  sc         & 0xff;
        const dstR = (dc >>> 16) & 0xff;
        const dstG = (dc >>> 8)  & 0xff;
        const dstB =  dc         & 0xff;

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
    this.layers.push(this._createLayerEntry(pd, 'レイヤー 1'));

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
    this.layers.splice(insertAt, 0, this._createLayerEntry(pd, name || `レイヤー ${n}`));
    this.activeIndex = insertAt;
    this._compositeDirty = true;
    return insertAt;
  }

  /**
   * レイヤー名を変更する。
   * @param {number} index
   * @param {string} newName
   * @returns {boolean}
   */
  renameLayer(index, newName) {
    if (index < 0 || index >= this.layers.length) return false;
    const layer = this.layers[index];
    if (!layer) return false;
    const sanitized = this._sanitizeLayerName(newName, layer.name);
    if (!sanitized || sanitized === layer.name) return false;
    layer.name = sanitized;
    return true;
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

  /**
   * レイヤーのロック状態を設定する。
   * @param {number} index
   * @param {boolean} locked
   * @returns {boolean}
   */
  setLocked(index, locked) {
    if (index < 0 || index >= this.layers.length) return false;
    this.layers[index].locked = !!locked;
    return true;
  }

  /**
   * レイヤーのロック状態をトグルする。
   * @param {number} index
   * @returns {boolean}
   */
  toggleLocked(index) {
    if (index < 0 || index >= this.layers.length) return false;
    this.layers[index].locked = !this.layers[index].locked;
    return true;
  }

  /**
   * 編集可能なレイヤーかを返す。
   * @param {number} [index=this.activeIndex]
   * @returns {boolean}
   */
  canEditLayer(index = this.activeIndex) {
    if (index < 0 || index >= this.layers.length) return false;
    return !this.layers[index].locked;
  }

  /**
   * レイヤーを複製し、複製側をアクティブにする。
   * @param {number} index
   * @returns {number} 複製後のインデックス。失敗時は -1。
   */
  duplicateLayer(index) {
    if (index < 0 || index >= this.layers.length) return -1;
    const source = this.layers[index];
    if (!source) return -1;
    const insertAt = index + 1;
    const cloned = this._cloneLayerEntry(source, `${source.name} コピー`);
    this.layers.splice(insertAt, 0, cloned);
    this.activeIndex = insertAt;
    this._compositeDirty = true;
    return insertAt;
  }

  /**
   * 指定レイヤーを1つ下のレイヤーへ結合する。
   * @param {number} index
   * @returns {boolean}
   */
  mergeLayerDown(index) {
    if (index <= 0 || index >= this.layers.length) return false;

    const upper = this.layers[index];
    const lower = this.layers[index - 1];
    if (!upper || !lower) return false;
    if (upper.locked || lower.locked) return false;

    const mergedPixelData = new PixelData();
    mergedPixelData.createPixelData(this.width, this.height, 0x00000000);
    const dst = mergedPixelData.pixels;

    if (lower.visible) {
      this._blendPixels(dst, lower.pixelData.pixels, lower.opacity);
    }
    if (upper.visible) {
      this._blendPixels(dst, upper.pixelData.pixels, upper.opacity);
    }

    const mergedLayer = this._createLayerEntry(
      mergedPixelData,
      lower.name,
      lower.visible || upper.visible,
      255,
      false
    );

    this.layers.splice(index - 1, 2, mergedLayer);
    this.activeIndex = index - 1;
    this._compositeDirty = true;
    return true;
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
      this._blendPixels(dst, layer.pixelData.pixels, layer.opacity);
    }

    this._compositeDirty = false;
    return this._composite;
  }
}
