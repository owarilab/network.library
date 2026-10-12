/**
 * NineSliceRenderer
 * 9 スライス（角は原寸、辺は一方向、中央は両方向に伸ばす）で矩形を描画する。
 * PlayTest（ワールドと UI）と EditorScene のプレビューで共用する。
 */
class NineSliceRenderer {
  /**
   * @param {CanvasRenderingContext2D} ctx
   * @param {HTMLCanvasElement} source  画像全体の canvas
   * @param {{left: number, top: number, right: number, bottom: number}|null} insets  ソース画素単位。null なら全体を伸ばす
   * @param {{x: number, y: number, w: number, h: number}} dest  描画先（ctx の座標系）
   * @param {number} [scale=1]  角と辺の厚みに掛ける倍率
   */
  static draw(ctx, source, insets, dest, scale = 1) {
    if (!source || !(dest.w > 0) || !(dest.h > 0)) return;

    ctx.save();
    ctx.imageSmoothingEnabled = false;
    if (!insets) {
      ctx.drawImage(source, 0, 0, source.width, source.height, dest.x, dest.y, dest.w, dest.h);
      ctx.restore();
      return;
    }

    const cornerLeft = insets.left * scale;
    const cornerRight = insets.right * scale;
    const cornerTop = insets.top * scale;
    const cornerBottom = insets.bottom * scale;

    // 角の合計が描画先を超えるときは、角を比率で縮める
    const fitX = cornerLeft + cornerRight > dest.w ? dest.w / (cornerLeft + cornerRight) : 1;
    const fitY = cornerTop + cornerBottom > dest.h ? dest.h / (cornerTop + cornerBottom) : 1;
    const left = cornerLeft * fitX;
    const right = cornerRight * fitX;
    const top = cornerTop * fitY;
    const bottom = cornerBottom * fitY;

    const destX = [dest.x, dest.x + left, dest.x + dest.w - right, dest.x + dest.w];
    const destY = [dest.y, dest.y + top, dest.y + dest.h - bottom, dest.y + dest.h];
    const sourceX = [0, insets.left, source.width - insets.right, source.width];
    const sourceY = [0, insets.top, source.height - insets.bottom, source.height];

    for (let row = 0; row < 3; row++) {
      for (let col = 0; col < 3; col++) {
        const sourceWidth = sourceX[col + 1] - sourceX[col];
        const sourceHeight = sourceY[row + 1] - sourceY[row];
        if (sourceWidth <= 0 || sourceHeight <= 0) continue;

        // 隣り合うセルの境界を整数座標で揃え、継ぎ目を防ぐ
        const x0 = Math.round(destX[col]);
        const x1 = Math.round(destX[col + 1]);
        const y0 = Math.round(destY[row]);
        const y1 = Math.round(destY[row + 1]);
        if (x1 <= x0 || y1 <= y0) continue;

        ctx.drawImage(
          source,
          sourceX[col], sourceY[row], sourceWidth, sourceHeight,
          x0, y0, x1 - x0, y1 - y0,
        );
      }
    }
    ctx.restore();
  }

  /**
   * PixelData（ARGB32）を描画用の canvas にする。
   * @param {{width: number, height: number, pixels: Uint32Array|number[]}} pixelData
   * @returns {HTMLCanvasElement|null}
   */
  static canvasFromPixelData(pixelData) {
    if (!pixelData?.pixels || !(pixelData.width > 0) || !(pixelData.height > 0)) return null;
    const canvas = document.createElement('canvas');
    canvas.width = pixelData.width;
    canvas.height = pixelData.height;
    const canvasCtx = canvas.getContext('2d');
    const imageData = canvasCtx.createImageData(pixelData.width, pixelData.height);
    const data = imageData.data;
    const pixels = pixelData.pixels;
    for (let i = 0; i < pixels.length; i++) {
      const color = pixels[i];
      const j = i * 4;
      data[j] = (color >>> 16) & 0xFF;
      data[j + 1] = (color >>> 8) & 0xFF;
      data[j + 2] = color & 0xFF;
      data[j + 3] = (color >>> 24) & 0xFF;
    }
    canvasCtx.putImageData(imageData, 0, 0);
    return canvas;
  }
}
