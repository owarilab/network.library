/**
 * PixelDataConverter
 *
 * PixelData を PNG / JSON 形式に変換してダウンロードするユーティリティクラス。
 * すべてのメソッドは static。インスタンス化不要。
 *
 * --- PNG ---
 *   PixelData の Uint32Array (0xAARRGGBB) を
 *   オフスクリーン canvas 経由で PNG Blob に変換し、a.download でDL。
 *
 * --- JSON ---
 *   独自バイナリ形式を Base64 エンコードして保存。
 *   再インポート時に完全復元可能。
 *
 *   スキーマ:
 *   {
 *     "version": 1,
 *     "width":   <number>,
 *     "height":  <number>,
 *     "format":  "ARGB32",   // 各ピクセルは 0xAARRGGBB (Little-Endian Uint32)
 *     "data":    "<base64>"  // Uint32Array のバイト列を Base64 エンコード
 *   }
 */
class PixelDataConverter {

  // ----------------------------------------------------------------
  // PNG
  // ----------------------------------------------------------------

  /**
   * PixelData → PNG Blob (Promise)
   * @param {PixelData} pixelData
   * @returns {Promise<Blob>}
   */
  static toPngBlob(pixelData) {
    return new Promise((resolve, reject) => {
      const { width, height, pixels } = pixelData;
      if (!pixels || width === 0 || height === 0) {
        reject(new Error('PixelData が空です'));
        return;
      }

      const offscreen = document.createElement('canvas');
      offscreen.width  = width;
      offscreen.height = height;
      const ctx2d = offscreen.getContext('2d');

      const imageData = ctx2d.createImageData(width, height);
      const d = imageData.data;

      for (let i = 0; i < pixels.length; i++) {
        const color = pixels[i];                   // 0xAARRGGBB
        d[i * 4 + 0] = (color >>> 16) & 0xFF;     // R
        d[i * 4 + 1] = (color >>>  8) & 0xFF;     // G
        d[i * 4 + 2] =  color         & 0xFF;      // B
        d[i * 4 + 3] = (color >>> 24) & 0xFF;     // A
      }

      ctx2d.putImageData(imageData, 0, 0);
      offscreen.toBlob(blob => {
        if (blob) resolve(blob);
        else      reject(new Error('toBlob() が null を返しました'));
      }, 'image/png');
    });
  }

  /**
   * PixelData を PNG としてダウンロードする
   * @param {PixelData} pixelData
   * @param {string}    [filename='pixel_art.png']
   * @returns {Promise<void>}
   */
  static async exportAsPng(pixelData, filename = 'pixel_art.png') {
    const blob = await PixelDataConverter.toPngBlob(pixelData);
    PixelDataConverter._downloadBlob(filename, blob);
  }

  // ----------------------------------------------------------------
  // JSON
  // ----------------------------------------------------------------

  /**
   * PixelData → JSON 文字列
   * ピクセルデータは Uint32Array のバイト列を Base64 エンコードして格納する。
   * @param {PixelData} pixelData
   * @returns {string}
   */
  static toJsonString(pixelData) {
    const { width, height, pixels } = pixelData;
    if (!pixels) throw new Error('PixelData が空です');

    // Uint32Array → Uint8Array → Base64
    const bytes = new Uint8Array(pixels.buffer, pixels.byteOffset, pixels.byteLength);
    let binary = '';
    // btoa は 64KB 以上のバイナリでスタックオーバーフローを起こしやすいため
    // チャンク処理で安全に変換する
    const CHUNK = 8192;
    for (let i = 0; i < bytes.length; i += CHUNK) {
      binary += String.fromCharCode(...bytes.subarray(i, i + CHUNK));
    }
    const data = btoa(binary);

    return JSON.stringify(
      { version: 1, width, height, format: 'ARGB32', data },
      null,
      2,
    );
  }

  /**
   * PixelData を JSON としてダウンロードする
   * @param {PixelData} pixelData
   * @param {string}    [filename='pixel_art.json']
   */
  static exportAsJson(pixelData, filename = 'pixel_art.json') {
    const json = PixelDataConverter.toJsonString(pixelData);
    const blob = new Blob([json], { type: 'application/json' });
    PixelDataConverter._downloadBlob(filename, blob);
  }

  // ----------------------------------------------------------------
  // インポート
  // ----------------------------------------------------------------

  /**
   * File オブジェクトから拡張子を判別して PixelData を返す。
   * 対応形式: .png / .json
   * @param {File} file
   * @returns {Promise<PixelData>}
   */
  static importFromFile(file) {
    const name = file.name.toLowerCase();
    if (name.endsWith('.png')) {
      return PixelDataConverter.importFromPng(file);
    }
    if (name.endsWith('.json')) {
      return PixelDataConverter.importFromJson(file);
    }
    if (name.endsWith('.qts')) {
      return PixelDataConverter.importFromQts(file);
    }
    return Promise.reject(new Error(`未対応のファイル形式です: ${file.name}`));
  }

  /**
   * PNG ファイル → PixelData (Promise)
   * Image → オフスクリーン canvas → getImageData → RGBA→0xAARRGGBB
   * @param {File} file
   * @returns {Promise<PixelData>}
   */
  static importFromPng(file) {
    return new Promise((resolve, reject) => {
      const url = URL.createObjectURL(file);
      const img = new Image();
      img.onload = () => {
        URL.revokeObjectURL(url);
        const w = img.naturalWidth;
        const h = img.naturalHeight;
        if (w === 0 || h === 0) {
          reject(new Error('PNG の対応外です (0x0)'));
          return;
        }
        const offscreen = document.createElement('canvas');
        offscreen.width  = w;
        offscreen.height = h;
        const ctx2d = offscreen.getContext('2d');
        ctx2d.drawImage(img, 0, 0);
        const imageData = ctx2d.getImageData(0, 0, w, h);
        const d = imageData.data;  // Uint8ClampedArray: R,G,B,A,...

        const pd = new PixelData();
        pd.createPixelData(w, h);
        for (let i = 0; i < pd.pixels.length; i++) {
          const r = d[i * 4 + 0];
          const g = d[i * 4 + 1];
          const b = d[i * 4 + 2];
          const a = d[i * 4 + 3];
          pd.pixels[i] = ((a << 24) | (r << 16) | (g << 8) | b) >>> 0;
        }
        resolve(pd);
      };
      img.onerror = () => {
        URL.revokeObjectURL(url);
        reject(new Error('PNG 画像の読み込みに失敗しました'));
      };
      img.src = url;
    });
  }

  /**
   * JSON ファイル → PixelData (Promise)
   * エクスポート時のスキーマ { version, width, height, format:"ARGB32", data:<base64> } を復元する。
   * @param {File} file
   * @returns {Promise<PixelData>}
   */
  static importFromJson(file) {
    return new Promise((resolve, reject) => {
      const reader = new FileReader();
      reader.onload = (e) => {
        try {
          const obj = JSON.parse(e.target.result);
          // v2 タイルセット形式の検出
          if (obj.type === 'tileset' && obj.version >= 2) {
            resolve(PixelDataConverter._parseTilesetJson(obj));
            return;
          }
          if (!obj.width || !obj.height || !obj.data) {
            reject(new Error('JSON ファイルのフォーマットが不正です'));
            return;
          }
          if (obj.format !== 'ARGB32') {
            reject(new Error(`未対応のフォーマットです: ${obj.format}`));
            return;
          }
          const { width, height } = obj;

          // Base64 → Uint8Array → Uint32Array
          const binary = atob(obj.data);
          const bytes  = new Uint8Array(binary.length);
          for (let i = 0; i < binary.length; i++) {
            bytes[i] = binary.charCodeAt(i);
          }
          // バイト数とピクセル数の整合性チェック
          if (bytes.length !== width * height * 4) {
            reject(new Error('JSON データのサイズが一致しません'));
            return;
          }

          const pd = new PixelData();
          pd.createPixelData(width, height);
          // Uint8Array を Uint32Array のバッファにコピー
          new Uint8Array(pd.pixels.buffer, pd.pixels.byteOffset, pd.pixels.byteLength)
            .set(bytes);
          resolve(pd);
        } catch (err) {
          reject(new Error(`JSON 解析エラー: ${err.message}`));
        }
      };
      reader.onerror = () => reject(new Error('JSON ファイルの読み込みに失敗しました'));
      reader.readAsText(file);
    });
  }

  // ----------------------------------------------------------------
  // タイルセット PNG エクスポート
  // ----------------------------------------------------------------

  /**
   * TilesetData の全チップを合成した1枚のPNGとしてダウンロードする。
   * @param {TilesetData} tilesetData
   * @param {string} [filename='tileset.png']
   * @returns {Promise<void>}
   */
  static async exportTilesetAsPng(tilesetData, filename = 'tileset.png') {
    const composited = tilesetData.compositeAll();
    await PixelDataConverter.exportAsPng(composited, filename);
  }

  /**
   * 選択チップのみPNGとしてダウンロードする。
   * @param {TilesetData} tilesetData
   * @param {number} col
   * @param {number} row
   * @param {string} [filename='chip.png']
   * @returns {Promise<void>}
   */
  static async exportChipAsPng(tilesetData, col, row, filename = 'chip.png') {
    const chipPd = tilesetData.compositeChip(col, row);
    if (!chipPd) throw new Error('チップ位置が不正です');
    await PixelDataConverter.exportAsPng(chipPd, filename);
  }

  // ----------------------------------------------------------------
  // タイルセット JSON v2
  // ----------------------------------------------------------------

  /**
   * PixelData の Uint32Array バイト列を Base64 エンコードする。
   * @param {PixelData} pixelData
   * @returns {string}
   */
  static _encodePixelDataBase64(pixelData) {
    const bytes = new Uint8Array(
      pixelData.pixels.buffer,
      pixelData.pixels.byteOffset,
      pixelData.pixels.byteLength
    );
    let binary = '';
    const CHUNK = 8192;
    for (let i = 0; i < bytes.length; i += CHUNK) {
      binary += String.fromCharCode(...bytes.subarray(i, i + CHUNK));
    }
    return btoa(binary);
  }

  /**
   * Base64 文字列から PixelData を復元する。
   * @param {string} base64Str
   * @param {number} width
   * @param {number} height
   * @returns {PixelData}
   */
  static _decodePixelDataBase64(base64Str, width, height) {
    const binary = atob(base64Str);
    const bytes = new Uint8Array(binary.length);
    for (let i = 0; i < binary.length; i++) {
      bytes[i] = binary.charCodeAt(i);
    }
    const pd = new PixelData();
    pd.createPixelData(width, height);
    new Uint8Array(pd.pixels.buffer, pd.pixels.byteOffset, pd.pixels.byteLength)
      .set(bytes);
    return pd;
  }

  /**
   * TilesetData を v2 JSON 文字列にシリアライズする。
   * @param {TilesetData} tilesetData
   * @returns {string}
   */
  static toTilesetJsonString(tilesetData) {
    const chips = [];
    for (let r = 0; r < tilesetData.rows; r++) {
      for (let c = 0; c < tilesetData.columns; c++) {
        const ld = tilesetData.chips[r][c];
        const layersArr = ld.layers.map(layer => ({
          name:    layer.name,
          visible: layer.visible,
          opacity: layer.opacity,
          locked:  !!layer.locked,
          data:    PixelDataConverter._encodePixelDataBase64(layer.pixelData),
        }));
        chips.push({ col: c, row: r, layers: layersArr });
      }
    }
    return JSON.stringify({
      version:    2,
      type:       'tileset',
      chipWidth:  tilesetData.chipWidth,
      chipHeight: tilesetData.chipHeight,
      columns:    tilesetData.columns,
      rows:       tilesetData.rows,
      chips,
    }, null, 2);
  }

  /**
   * TilesetData を v2 JSON としてダウンロードする。
   * @param {TilesetData} tilesetData
   * @param {string} [filename='tileset.json']
   */
  static exportTilesetAsJson(tilesetData, filename = 'tileset.json') {
    const json = PixelDataConverter.toTilesetJsonString(tilesetData);
    const blob = new Blob([json], { type: 'application/json' });
    PixelDataConverter._downloadBlob(filename, blob);
  }

  /**
   * LayerData を 1x1 の TilesetData に包んで返す。
   * 通常ドット絵を既存の QTS 出力経路へ流すために使用する。
   * @param {LayerData} layerData
   * @returns {TilesetData}
   */
  static wrapLayerDataAsSingleChipTileset(layerData) {
    if (!layerData || !Array.isArray(layerData.layers) || layerData.layers.length === 0) {
      throw new Error('LayerData が空です');
    }

    const width = layerData.width | 0;
    const height = layerData.height | 0;
    if (width <= 0 || height <= 0) {
      throw new Error('LayerData のサイズが不正です');
    }

    const td = new TilesetData(width, height, 1, 1, 0x00000000);
    const copiedLayers = layerData.layers.map(layer => {
      const pixelData = new PixelData();
      pixelData.createPixelData(width, height);
      pixelData.pixels.set(layer.pixelData.pixels);
      return {
        pixelData,
        name: layer.name || 'レイヤー 1',
        visible: layer.visible !== false,
        opacity: typeof layer.opacity === 'number' ? layer.opacity : 255,
        locked: !!layer.locked,
      };
    });

    const chipLayerData = td.chips[0][0];
    chipLayerData.layers = copiedLayers;
    chipLayerData.width = width;
    chipLayerData.height = height;
    chipLayerData.activeIndex = Math.max(0, Math.min(layerData.activeIndex | 0, copiedLayers.length - 1));
    chipLayerData._composite.createPixelData(width, height);
    chipLayerData._compositeDirty = true;
    td.passFlags[0][0] = true;
    return td;
  }

  /**
   * LayerData + パレットを 1x1 QTS としてダウンロードする。
   * @param {LayerData}         layerData
   * @param {EditablePalette32} palette
   * @param {string}            [filename='pixel_art.qts']
   */
  static exportLayerDataAsQts(layerData, palette, filename = 'pixel_art.qts') {
    const tilesetData = PixelDataConverter.wrapLayerDataAsSingleChipTileset(layerData);
    PixelDataConverter.exportTilesetAsQts(tilesetData, palette, filename);
  }

  /**
   * ArrayBuffer を Base64 文字列へ変換する。
   * @param {ArrayBuffer} buffer
   * @returns {string}
   */
  static arrayBufferToBase64(buffer) {
    const bytes = new Uint8Array(buffer);
    let binary = '';
    const chunkSize = 8192;
    for (let i = 0; i < bytes.length; i += chunkSize) {
      binary += String.fromCharCode(...bytes.subarray(i, i + chunkSize));
    }
    return btoa(binary);
  }

  /**
   * Base64 文字列を ArrayBuffer に戻す。
   * @param {string} base64
   * @returns {ArrayBuffer}
   */
  static base64ToArrayBuffer(base64) {
    const binary = atob(base64);
    const bytes = new Uint8Array(binary.length);
    for (let i = 0; i < binary.length; i++) {
      bytes[i] = binary.charCodeAt(i);
    }
    return bytes.buffer;
  }

  /**
   * 1x1 の TilesetData から単体画像用 LayerData を復元する。
   * @param {TilesetData} tilesetData
   * @returns {LayerData}
   */
  static unwrapSingleChipTilesetToLayerData(tilesetData) {
    if (!tilesetData || tilesetData.columns !== 1 || tilesetData.rows !== 1) {
      throw new Error('1x1 の TilesetData ではありません');
    }

    const src = tilesetData.chips[0][0];
    if (!src || !Array.isArray(src.layers) || src.layers.length === 0) {
      throw new Error('チップの LayerData が空です');
    }

    const ld = new LayerData();
    ld.width = tilesetData.chipWidth;
    ld.height = tilesetData.chipHeight;
    ld.layers = src.layers.map(layer => {
      const pixelData = new PixelData();
      pixelData.createPixelData(ld.width, ld.height);
      pixelData.pixels.set(layer.pixelData.pixels);
      return {
        pixelData,
        name: layer.name || 'レイヤー 1',
        visible: layer.visible !== false,
        opacity: typeof layer.opacity === 'number' ? layer.opacity : 255,
        locked: !!layer.locked,
      };
    });
    ld.activeIndex = Math.max(0, Math.min(src.activeIndex | 0, ld.layers.length - 1));
    ld._composite.createPixelData(ld.width, ld.height);
    ld._compositeDirty = true;
    return ld;
  }

  /**
   * v2 タイルセット JSON オブジェクトを TilesetData に変換する。
   * @param {Object} obj  パース済み JSON オブジェクト
   * @returns {TilesetData}
   */
  static _parseTilesetJson(obj) {
    const { chipWidth, chipHeight, columns, rows, chips } = obj;
    const td = new TilesetData(chipWidth, chipHeight, columns, rows);
    for (const chipObj of chips) {
      const { col, row, layers } = chipObj;
      if (row < 0 || row >= rows || col < 0 || col >= columns) continue;
      if (!layers || layers.length === 0) continue;
      const ld = td.chips[row][col];
      ld.layers = layers.map(layerObj => ({
        pixelData: PixelDataConverter._decodePixelDataBase64(
          layerObj.data, chipWidth, chipHeight
        ),
        name:    layerObj.name || 'レイヤー 1',
        visible: layerObj.visible !== false,
        opacity: typeof layerObj.opacity === 'number' ? layerObj.opacity : 255,
        locked:  layerObj.locked === true,
      }));
      ld.activeIndex = 0;
      ld._composite.createPixelData(chipWidth, chipHeight);
      ld._compositeDirty = true;
    }
    return td;
  }

  /**
   * 既存の PixelData をチップサイズで分割して TilesetData を生成する。
   * PNG インポート時に使用する。
   * @param {PixelData} pixelData  全体画像
   * @param {number} chipW  チップ幅
   * @param {number} chipH  チップ高さ
   * @returns {TilesetData}
   */
  static tilesetFromPixelData(pixelData, chipW, chipH) {
    const cols = Math.floor(pixelData.width / chipW);
    const rows = Math.floor(pixelData.height / chipH);
    const td = new TilesetData(chipW, chipH, cols, rows);
    for (let r = 0; r < rows; r++) {
      for (let c = 0; c < cols; c++) {
        const layer = td.chips[r][c].layers[0];
        const ox = c * chipW;
        const oy = r * chipH;
        for (let y = 0; y < chipH; y++) {
          for (let x = 0; x < chipW; x++) {
            layer.pixelData.setPixel(x, y, pixelData.getPixel(ox + x, oy + y));
          }
        }
        td.chips[r][c].markCompositeDirty();
      }
    }
    return td;
  }

  // ----------------------------------------------------------------
  // QTS バイナリフォーマット (.qts)
  // ----------------------------------------------------------------

  // ---- 1バイトピクセル操作 ----

  /** カラーID取得 (0〜31) */
  static PIXEL_COLOR_ID(b) { return b & 0x1F; }
  /** 通過フラグ取得 (0 or 1) */
  static PIXEL_PASSABLE(b) { return (b >> 7) & 0x01; }
  /** バイト生成 */
  static PIXEL_MAKE(colorId, passable = 0, reserved = 0) {
    return ((passable & 1) << 7) | ((reserved & 3) << 5) | (colorId & 0x1F);
  }

  /**
   * ARGB32 ピクセルを最近傍パレットインデックスに量子化する。
   * @param {number}   argb32     0xAARRGGBB
   * @param {number[]} palette    EditablePalette32.getColors() の結果（32要素）
   * @param {number}   [passable=0] 通過フラグ
   * @returns {number} 0〜255 の1バイト
   */
  static quantizePixel(argb32, palette, passable = 0) {
    if (((argb32 >>> 24) & 0xFF) === 0) return PixelDataConverter.PIXEL_MAKE(0, 0, 0);

    const r = (argb32 >>> 16) & 0xFF;
    const g = (argb32 >>>  8) & 0xFF;
    const b =  argb32         & 0xFF;

    let bestId   = 1;
    let bestDist = Infinity;
    for (let i = 1; i < palette.length; i++) {
      if (((palette[i] >>> 24) & 0xFF) === 0) continue;
      const pr = (palette[i] >>> 16) & 0xFF;
      const pg = (palette[i] >>>  8) & 0xFF;
      const pb =  palette[i]         & 0xFF;
      const d  = (r - pr) ** 2 + (g - pg) ** 2 + (b - pb) ** 2;
      if (d < bestDist) { bestDist = d; bestId = i; }
    }
    return PixelDataConverter.PIXEL_MAKE(bestId, passable);
  }

  /**
   * 1バイトピクセルを ARGB32 に展開する。
   * @param {number}   byte     1バイトピクセル値
   * @param {number[]} palette  EditablePalette32.getColors() の結果
   * @returns {number} 0xAARRGGBB
   */
  static expandPixel(byte, palette) {
    const colorId = PixelDataConverter.PIXEL_COLOR_ID(byte);
    if (colorId === 0) return 0x00000000;
    const c = palette[colorId];
    if (!c || ((c >>> 24) & 0xFF) === 0) return 0x00000000;
    return (c & 0x00FFFFFF) | 0xFF000000;
  }

  // ---- QTS エクスポート ----

  /**
   * TilesetData + パレットを .qts バイナリとしてダウンロードする。
   * @param {TilesetData}       tilesetData
   * @param {EditablePalette32} palette
   * @param {string}            [filename='tileset.qts']
   */
  static createQtsArrayBuffer(tilesetData, palette) {
    const { chipWidth, chipHeight, columns, rows } = tilesetData;
    const paletteColors = palette.getColors();
    const encoder = new TextEncoder();

    // --- バッファサイズを事前計算 ---
    let totalSize = 8 + 8 + 129; // ヘッダ + メタ + パレット
    for (let r = 0; r < rows; r++) {
      for (let c = 0; c < columns; c++) {
        const ld = tilesetData.chips[r][c];
        totalSize += 1; // レイヤー数
        for (const layer of ld.layers) {
          const nameBytes = encoder.encode(layer.name || '');
          totalSize += 2 + nameBytes.byteLength + 1 + 1; // L + name + visible + opacity
          totalSize += chipWidth * chipHeight; // ピクセルデータ
        }
      }
    }

    const buffer = new ArrayBuffer(totalSize);
    const bytes  = new Uint8Array(buffer);
    const view   = new DataView(buffer);
    let pos = 0;

    // --- ファイルヘッダ (8バイト) ---
    bytes[pos++] = 0x51; // 'Q'
    bytes[pos++] = 0x53; // 'S'
    bytes[pos++] = 0x54; // 'T'
    bytes[pos++] = 0x53; // 'S'
    bytes[pos++] = 0x01; // メジャーバージョン
    bytes[pos++] = 0x00; // マイナーバージョン
    bytes[pos++] = 0x00; // 予約
    bytes[pos++] = 0x00; // 予約

    // --- タイルセットメタ情報 (8バイト, uint16 LE) ---
    view.setUint16(pos, chipWidth,  true); pos += 2;
    view.setUint16(pos, chipHeight, true); pos += 2;
    view.setUint16(pos, columns,    true); pos += 2;
    view.setUint16(pos, rows,       true); pos += 2;

    // --- パレットブロック (129バイト) ---
    bytes[pos++] = paletteColors.length; // 有効色数 N
    for (let i = 0; i < 32; i++) {
      const c = i < paletteColors.length ? paletteColors[i] : 0;
      bytes[pos++] = (c >>> 16) & 0xFF; // R
      bytes[pos++] = (c >>>  8) & 0xFF; // G
      bytes[pos++] =  c         & 0xFF; // B
      bytes[pos++] = (c >>> 24) & 0xFF; // A
    }

    // --- チップブロック (row-major) ---
    for (let r = 0; r < rows; r++) {
      for (let c = 0; c < columns; c++) {
        const ld = tilesetData.chips[r][c];
        bytes[pos++] = ld.layers.length; // レイヤー数

        for (const layer of ld.layers) {
          // レイヤーメタ
          const nameBytes = encoder.encode(layer.name || '');
          view.setUint16(pos, nameBytes.byteLength, true); pos += 2;
          bytes.set(nameBytes, pos); pos += nameBytes.byteLength;
          bytes[pos++] = layer.visible ? 1 : 0;
          bytes[pos++] = layer.opacity & 0xFF;

          // ピクセルデータ (量子化)
          const pd = layer.pixelData;
          const passable = (tilesetData.passFlags && tilesetData.passFlags[r] &&
                            tilesetData.passFlags[r][c] !== false) ? 1 : 0;
          for (let y = 0; y < chipHeight; y++) {
            for (let x = 0; x < chipWidth; x++) {
              bytes[pos++] = PixelDataConverter.quantizePixel(
                pd.getPixel(x, y), paletteColors, passable
              );
            }
          }
        }
      }
    }

    return buffer;
  }

  /**
   * TilesetData + パレットを .qts バイナリとしてダウンロードする。
   * @param {TilesetData}       tilesetData
   * @param {EditablePalette32} palette
   * @param {string}            [filename='tileset.qts']
   */
  static exportTilesetAsQts(tilesetData, palette, filename = 'tileset.qts') {
    const buffer = PixelDataConverter.createQtsArrayBuffer(tilesetData, palette);
    const blob = new Blob([buffer], { type: 'application/octet-stream' });
    PixelDataConverter._downloadBlob(filename, blob);
  }

  // ---- QTS インポート ----

  /**
   * .qts ファイルを読み込んで TilesetData + EditablePalette32 を返す。
   * @param {File} file
   * @returns {Promise<{ tilesetData: TilesetData, palette: EditablePalette32 }>}
   */
  static parseQtsArrayBuffer(buffer) {
    const bytes  = new Uint8Array(buffer);
    const view   = new DataView(buffer);
    let pos = 0;

    if (bytes.length < 8 + 8 + 129) {
      throw new Error('ファイルサイズが不正です');
    }
    if (bytes[0] !== 0x51 || bytes[1] !== 0x53 ||
        bytes[2] !== 0x54 || bytes[3] !== 0x53) {
      throw new Error('マジックナンバーが不正です (QSTS でない)');
    }
    pos = 8;

    const chipWidth  = view.getUint16(pos, true); pos += 2;
    const chipHeight = view.getUint16(pos, true); pos += 2;
    const columns    = view.getUint16(pos, true); pos += 2;
    const rows       = view.getUint16(pos, true); pos += 2;
    if (chipWidth === 0 || chipHeight === 0 || columns === 0 || rows === 0) {
      throw new Error('タイルセットメタ情報が不正です');
    }

    const paletteN = bytes[pos++];
    const palette = new EditablePalette32();
    const paletteColors = [];
    for (let i = 0; i < 32; i++) {
      const r = bytes[pos++];
      const g = bytes[pos++];
      const b = bytes[pos++];
      const a = bytes[pos++];
      const color = PixelData.rgba(r, g, b, a);
      paletteColors.push(color);
      if (i > 0 && i < paletteN) {
        palette.setColor(i, color);
      }
    }

    const decoder = new TextDecoder();
    const td = new TilesetData(chipWidth, chipHeight, columns, rows);

    for (let r = 0; r < rows; r++) {
      for (let c = 0; c < columns; c++) {
        if (pos >= bytes.length) {
          throw new Error('ファイルが途中で終了しています');
        }
        const layerCount = bytes[pos++];
        const layers = [];
        let chipPassable = true;

        for (let li = 0; li < layerCount; li++) {
          const nameLen = view.getUint16(pos, true); pos += 2;
          const name = nameLen > 0
            ? decoder.decode(bytes.subarray(pos, pos + nameLen))
            : '';
          pos += nameLen;
          const visible = bytes[pos++] === 1;
          const opacity = bytes[pos++];

          const pd = new PixelData();
          pd.createPixelData(chipWidth, chipHeight);
          let foundPassFlag = false;
          for (let y = 0; y < chipHeight; y++) {
            for (let x = 0; x < chipWidth; x++) {
              const byteVal = bytes[pos++];
              pd.setPixel(x, y, PixelDataConverter.expandPixel(byteVal, paletteColors));
              if (li === 0 && !foundPassFlag && PixelDataConverter.PIXEL_COLOR_ID(byteVal) !== 0) {
                chipPassable = PixelDataConverter.PIXEL_PASSABLE(byteVal) === 1;
                foundPassFlag = true;
              }
            }
          }

          layers.push({ pixelData: pd, name, visible, opacity, locked: false });
        }

        if (layers.length > 0) {
          const ld = td.chips[r][c];
          ld.layers = layers;
          ld.activeIndex = 0;
          ld._composite.createPixelData(chipWidth, chipHeight);
          ld._compositeDirty = true;
        }
        td.passFlags[r][c] = chipPassable;
      }
    }

    return { tilesetData: td, palette };
  }

  static importFromQts(file) {
    return new Promise((resolve, reject) => {
      const reader = new FileReader();
      reader.onload = (e) => {
        try {
          resolve(PixelDataConverter.parseQtsArrayBuffer(e.target.result));
        } catch (err) {
          reject(new Error(`QTS 解析エラー: ${err.message}`));
        }
      };
      reader.onerror = () => reject(new Error('QTS ファイルの読み込みに失敗しました'));
      reader.readAsArrayBuffer(file);
    });
  }

  // ----------------------------------------------------------------
  // 内部ユーティリティ
  // ----------------------------------------------------------------

  /**
   * Blob をファイルとしてダウンロードさせる
   * @param {string} filename
   * @param {Blob}   blob
   */
  static _downloadBlob(filename, blob) {
    const url = URL.createObjectURL(blob);
    const anchor = document.createElement('a');
    anchor.href     = url;
    anchor.download = filename;
    anchor.style.display = 'none';
    document.body.appendChild(anchor);
    anchor.click();
    document.body.removeChild(anchor);
    // 少し遅延させてから解放する（Safari 対策）
    setTimeout(() => URL.revokeObjectURL(url), 1000);
  }
}
