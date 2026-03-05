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
