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
