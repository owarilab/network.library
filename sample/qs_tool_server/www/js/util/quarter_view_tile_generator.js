/**
 * QuarterViewTileGenerator
 * クォータービュー（2:1 斜め見下ろし）タイルの雛形を自動生成するユーティリティクラス。
 * 静的メソッドのみで構成される。
 *
 * パラメータ型 QuarterViewTileParams:
 * {
 *   type:           'ground' | 'box',
 *   width:          number,            // キャンバス幅（4の倍数）
 *   wallHeight:     number,            // 壁の高さ（boxのみ、groundは0）
 *   separateLayers: boolean,           // true=面ごとにレイヤー分け
 *   topColor:       number,            // 上面塗りつぶし 0xAARRGGBB
 *   leftColor:      number,            // 左面塗りつぶし
 *   rightColor:     number,            // 右面塗りつぶし
 *   outlineColor:   number,            // 輪郭線色
 *   target:         'new' | 'overwrite',
 * }
 */
class QuarterViewTileGenerator {

  /**
   * パラメータから PixelData 群を生成する。
   * target='new' 時: 新規 LayerData を作成して返す。
   * target='overwrite' 時: 渡された既存 LayerData のアクティブレイヤーに描画し null を返す。
   * @param {Object} params - QuarterViewTileParams
   * @param {LayerData|null} existingLayerData - overwrite 時に渡す既存データ
   * @returns {LayerData|null}
   */
  static generate(params, existingLayerData = null) {
    const W   = params.width;
    const cx  = W / 2;
    const ry  = W / 4;
    const by  = 2 * ry - 1;
    const H_w = params.type === 'box' ? params.wallHeight : 0;
    const H   = params.type === 'box' ? (W / 2 + H_w) : (W / 2);

    // 各面の頂点
    const topVerts = [
      { x: cx,    y: 0  },
      { x: W - 1, y: ry },
      { x: cx,    y: by },
      { x: 0,     y: ry },
    ];

    const leftVerts = params.type === 'box' ? [
      { x: 0,  y: ry         },
      { x: cx, y: by         },
      { x: cx, y: by + H_w   },
      { x: 0,  y: ry + H_w   },
    ] : null;

    const rightVerts = params.type === 'box' ? [
      { x: cx,    y: by         },
      { x: W - 1, y: ry         },
      { x: W - 1, y: ry + H_w   },
      { x: cx,    y: by + H_w   },
    ] : null;

    // overwrite モード: 既存レイヤーのアクティブレイヤーに描画
    if (params.target === 'overwrite' && existingLayerData) {
      const pd = existingLayerData.getActiveLayer();
      if (!pd) return null;

      // 1レイヤーに全描画（塗りつぶし → 全輪郭）
      QuarterViewTileGenerator._fillPolygon(pd, topVerts, params.topColor);
      if (params.type === 'box') {
        QuarterViewTileGenerator._fillPolygon(pd, leftVerts, params.leftColor);
        QuarterViewTileGenerator._fillPolygon(pd, rightVerts, params.rightColor);
      }
      // 輪郭線
      QuarterViewTileGenerator._drawOutline(pd, topVerts, params.outlineColor);
      if (params.type === 'box') {
        QuarterViewTileGenerator._drawOutline(pd, leftVerts, params.outlineColor);
        QuarterViewTileGenerator._drawOutline(pd, rightVerts, params.outlineColor);
      }
      return null;
    }

    // new モード: 新規 LayerData を作成
    const ld = new LayerData();

    if (params.separateLayers && params.type === 'box') {
      // レイヤー分け: layers[0]=右面(最背面), [1]=左面, [2]=上面(最前面)
      ld.width  = W;
      ld.height = H;
      ld.layers = [];
      ld._composite.createPixelData(W, H);
      ld._compositeDirty = true;

      // 右面レイヤー
      const pdRight = new PixelData();
      pdRight.createPixelData(W, H, 0x00000000);
      QuarterViewTileGenerator._drawPolygon(pdRight, rightVerts, params.rightColor, params.outlineColor);
      ld.layers.push({ pixelData: pdRight, name: '右面', visible: true, opacity: 255 });

      // 左面レイヤー
      const pdLeft = new PixelData();
      pdLeft.createPixelData(W, H, 0x00000000);
      QuarterViewTileGenerator._drawPolygon(pdLeft, leftVerts, params.leftColor, params.outlineColor);
      ld.layers.push({ pixelData: pdLeft, name: '左面', visible: true, opacity: 255 });

      // 上面レイヤー
      const pdTop = new PixelData();
      pdTop.createPixelData(W, H, 0x00000000);
      QuarterViewTileGenerator._drawPolygon(pdTop, topVerts, params.topColor, params.outlineColor);
      ld.layers.push({ pixelData: pdTop, name: '上面', visible: true, opacity: 255 });

      ld.activeIndex = 2; // 上面を選択状態に

    } else if (params.separateLayers && params.type === 'ground') {
      // 地面タイルのレイヤー分け: 1レイヤーのみ
      ld.width  = W;
      ld.height = H;
      ld.layers = [];
      ld._composite.createPixelData(W, H);
      ld._compositeDirty = true;

      const pdTop = new PixelData();
      pdTop.createPixelData(W, H, 0x00000000);
      QuarterViewTileGenerator._drawPolygon(pdTop, topVerts, params.topColor, params.outlineColor);
      ld.layers.push({ pixelData: pdTop, name: '上面', visible: true, opacity: 255 });

      ld.activeIndex = 0;

    } else {
      // 1レイヤー描画
      ld.init(W, H, 0x00000000);

      const pd = ld.getActiveLayer();
      // 塗りつぶしを先に全面行う
      QuarterViewTileGenerator._fillPolygon(pd, topVerts, params.topColor);
      if (params.type === 'box') {
        QuarterViewTileGenerator._fillPolygon(pd, leftVerts, params.leftColor);
        QuarterViewTileGenerator._fillPolygon(pd, rightVerts, params.rightColor);
      }
      // 輪郭線を後から全面描画（常に最前面に出る）
      QuarterViewTileGenerator._drawOutline(pd, topVerts, params.outlineColor);
      if (params.type === 'box') {
        QuarterViewTileGenerator._drawOutline(pd, leftVerts, params.outlineColor);
        QuarterViewTileGenerator._drawOutline(pd, rightVerts, params.outlineColor);
      }
      ld.markCompositeDirty();
    }

    return ld;
  }

  /**
   * PixelData に単一面の塗りつぶしと輪郭線を描画する。
   * separateLayers=true 時に面ごとの描画を行うユーティリティ。
   * @param {PixelData} pd
   * @param {Array<{x:number,y:number}>} vertices - 頂点配列（時計回り）
   * @param {number} fillColor   - 0xAARRGGBB, 0=塗りつぶしなし
   * @param {number} outlineColor - 0xAARRGGBB
   */
  static _drawPolygon(pd, vertices, fillColor, outlineColor) {
    if (fillColor !== 0) {
      QuarterViewTileGenerator._fillPolygon(pd, vertices, fillColor);
    }
    QuarterViewTileGenerator._drawOutline(pd, vertices, outlineColor);
  }

  /**
   * ポリゴンの輪郭線を描画する。
   * @param {PixelData} pd
   * @param {Array<{x:number,y:number}>} vertices
   * @param {number} color
   */
  static _drawOutline(pd, vertices, color) {
    const n = vertices.length;
    for (let i = 0; i < n; i++) {
      const a = vertices[i];
      const b = vertices[(i + 1) % n];
      QuarterViewTileGenerator._drawLine(pd, a.x, a.y, b.x, b.y, color);
    }
  }

  /**
   * Bresenham 直線アルゴリズムで線を描画する。
   * 始点・終点ともに描画する（両端込み）。
   * @param {PixelData} pd
   * @param {number} x0
   * @param {number} y0
   * @param {number} x1
   * @param {number} y1
   * @param {number} color
   */
  static _drawLine(pd, x0, y0, x1, y1, color) {
    let dx = Math.abs(x1 - x0);
    let dy = Math.abs(y1 - y0);
    const sx = x0 < x1 ? 1 : -1;
    const sy = y0 < y1 ? 1 : -1;
    let err = dx - dy;

    for (;;) {
      pd.setPixel(x0, y0, color);
      if (x0 === x1 && y0 === y1) break;
      const e2 = 2 * err;
      if (e2 > -dy) { err -= dy; x0 += sx; }
      if (e2 <  dx) { err += dx; y0 += sy; }
    }
  }

  /**
   * スキャンライン法でポリゴン内部を塗りつぶす。
   * @param {PixelData} pd
   * @param {Array<{x:number,y:number}>} vertices
   * @param {number} color
   */
  static _fillPolygon(pd, vertices, color) {
    if (color === 0) return;

    const n = vertices.length;
    if (n < 3) return;

    // y の範囲を求める
    let yMin = vertices[0].y;
    let yMax = vertices[0].y;
    for (let i = 1; i < n; i++) {
      if (vertices[i].y < yMin) yMin = vertices[i].y;
      if (vertices[i].y > yMax) yMax = vertices[i].y;
    }

    // 各スキャンライン
    for (let y = yMin; y <= yMax; y++) {
      const xIntersections = [];

      for (let i = 0; i < n; i++) {
        const a = vertices[i];
        const b = vertices[(i + 1) % n];

        // 辺が水平な場合はスキップ
        if (a.y === b.y) continue;

        // y がこの辺の範囲内か判定（上端は含む、下端は含まない = 半開区間）
        const yLo = Math.min(a.y, b.y);
        const yHi = Math.max(a.y, b.y);
        if (y < yLo || y >= yHi) continue;

        // 交点の x 座標を計算
        const t = (y - a.y) / (b.y - a.y);
        const x = a.x + t * (b.x - a.x);
        xIntersections.push(Math.round(x));
      }

      // x でソートしてペアで塗りつぶし
      xIntersections.sort((a, b) => a - b);

      for (let i = 0; i + 1 < xIntersections.length; i += 2) {
        const xStart = xIntersections[i];
        const xEnd   = xIntersections[i + 1];
        for (let x = xStart; x <= xEnd; x++) {
          pd.setPixel(x, y, color);
        }
      }
    }
  }
}
