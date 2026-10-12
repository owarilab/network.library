/**
 * TilemapRuntime
 * PlayTest 用のタイルマップ描画と通行判定を担当する。
 * 描画と判定が同じセル計算を使うように 1 クラスに集約する。
 * マップ・タイルセットは PlayTest 中に変わらないため、解決結果と描画用アトラスはキャッシュする。
 */
class TilemapRuntime {
  constructor() {
    /** key: `${mapAssetId}|${layerId}`, value: 解決済みの空間情報（解決できない場合は null） */
    this._spaceCache = new Map();
    /** key: tilesetId, value: タイルセット全体を 1 枚にした canvas */
    this._atlasCache = new Map();
  }

  clear() {
    this._spaceCache.clear();
    this._atlasCache.clear();
  }

  /**
   * Tilemap エントリをタイルマップの描画を行う。
   * @param {CanvasRenderingContext2D} ctx  カメラ変換済みの世界座標系
   * @param {object} entry  PlayUnitRuntime.tilemapEntries の要素
   * @param {ProjectData|null} project
   * @param {{left: number, top: number, right: number, bottom: number}} view  世界座標の可視範囲
   */
  draw(ctx, entry, project, view) {
    const space = this._getSpace(entry, project);
    if (!space) return;

    const atlas = this._getAtlas(space);
    const cellW = space.chipWidth * entry.scale;
    const cellH = space.chipHeight * entry.scale;
    const originX = entry.x + entry.offsetX;
    const originY = entry.y + entry.offsetY;
    const range = TilemapRuntime._cellRange(originX, originY, cellW, cellH, view.left, view.top, view.right, view.bottom);
    const fromCol = Math.max(0, range.fromCol);
    const toCol = Math.min(space.width - 1, range.toCol);
    const fromRow = Math.max(0, range.fromRow);
    const toRow = Math.min(space.height - 1, range.toRow);

    ctx.save();
    ctx.imageSmoothingEnabled = false;
    for (const layer of space.layers) {
      for (let row = fromRow; row <= toRow; row++) {
        for (let col = fromCol; col <= toCol; col++) {
          const tile = layer.tiles[row * space.width + col];
          if (!Number.isInteger(tile) || tile < 0) continue;

          const sourceX = (tile % space.columns) * space.chipWidth;
          const sourceY = Math.floor(tile / space.columns) * space.chipHeight;
          if (sourceY >= atlas.height) continue;

          // 隣接セルの端を整数座標で揃えて、継ぎ目が出ないようにする
          const drawX = Math.round(originX + col * cellW);
          const drawY = Math.round(originY + row * cellH);
          const drawWidth = Math.round(originX + (col + 1) * cellW) - drawX;
          const drawHeight = Math.round(originY + (row + 1) * cellH) - drawY;
          ctx.drawImage(atlas, sourceX, sourceY, space.chipWidth, space.chipHeight, drawX, drawY, drawWidth, drawHeight);
        }
      }
    }
    ctx.restore();
  }

  /**
   * 矩形（世界座標）がいずれかの Tilemap の通行不可セルに触れているか判定する。
   * マップ外のセルは通行不可とする。ただし他の Tilemap が覆っている位置はマップ外扱いしない。
   * @param {Array<object>} entries  PlayUnitRuntime.tilemapEntries
   * @param {ProjectData|null} project
   * @param {{left: number, top: number, right: number, bottom: number}} bounds
   * @returns {boolean}
   */
  isRectBlocked(entries, project, bounds) {
    const spaces = [];
    for (const entry of entries) {
      const space = this._getSpace(entry, project);
      if (!space) continue;
      spaces.push({
        space,
        originX: entry.x + entry.offsetX,
        originY: entry.y + entry.offsetY,
        cellW: space.chipWidth * entry.scale,
        cellH: space.chipHeight * entry.scale,
      });
    }

    for (const target of spaces) {
      const { space, originX, originY, cellW, cellH } = target;
      const range = TilemapRuntime._cellRange(originX, originY, cellW, cellH, bounds.left, bounds.top, bounds.right, bounds.bottom);
      for (let row = range.fromRow; row <= range.toRow; row++) {
        for (let col = range.fromCol; col <= range.toCol; col++) {
          if (!TilemapRuntime._isInside(space, col, row)) {
            if (!TilemapRuntime._isCellCenterCoveredByOther(spaces, target, col, row)) return true;
            continue;
          }
          if (space.blocked[row * space.width + col]) return true;
        }
      }
    }
    return false;
  }

  /**
   * Tilemap の描画・判定に必要な情報をまとめる。表示レイヤーが無い場合は null。
   * @param {object} entry
   * @param {ProjectData|null} project
   * @returns {object|null}
   */
  _getSpace(entry, project) {
    const key = `${entry.mapAssetId}|${entry.layerId}`;
    if (!this._spaceCache.has(key)) {
      this._spaceCache.set(key, TilemapRuntime._createSpace(entry, project));
    }
    return this._spaceCache.get(key);
  }

  /**
   * @param {object} entry
   * @param {ProjectData|null} project
   * @returns {object|null}
   */
  static _createSpace(entry, project) {
    const map = project?.findMapById?.(entry.mapAssetId) || null;
    const mapData = map?.mapData || null;
    if (!mapData) return null;

    const tileset = project.findTilesetById?.(mapData.tilesetId) || null;
    const tilesetData = tileset?.tilesetData || null;
    if (!tilesetData) return null;

    const width = mapData.width | 0;
    const height = mapData.height | 0;
    if (width <= 0 || height <= 0) return null;

    // layerId が空なら表示中の全レイヤー。指定時はそのレイヤーだけ
    const layers = (Array.isArray(mapData.layers) ? mapData.layers : [])
      .filter(layer => layer && layer.visible !== false && (!entry.layerId || layer.id === entry.layerId));
    if (!layers.length) return null;

    const space = {
      width,
      height,
      layers,
      tilesetId: tileset.id,
      tilesetData,
      chipWidth: tilesetData.chipWidth,
      chipHeight: tilesetData.chipHeight,
      columns: tilesetData.columns,
      rows: tilesetData.rows,
      blocked: new Uint8Array(width * height),
    };

    // 通過フラグが false のチップを置いたセルを塞ぐ（どの表示レイヤーのチップでも可）
    for (const layer of layers) {
      const tiles = Array.isArray(layer.tiles) ? layer.tiles : [];
      for (let index = 0; index < width * height; index++) {
        const tile = tiles[index];
        if (!Number.isInteger(tile) || tile < 0) continue;
        const col = tile % space.columns;
        const row = Math.floor(tile / space.columns);
        if (tilesetData.passFlags?.[row]?.[col] === false) space.blocked[index] = 1;
      }
    }
    return space;
  }

  /**
   * タイルセット全体を 1 枚の canvas にする（キャッシュ付き）。
   * @param {object} space
   * @returns {HTMLCanvasElement}
   */
  _getAtlas(space) {
    if (this._atlasCache.has(space.tilesetId)) return this._atlasCache.get(space.tilesetId);

    const canvas = document.createElement('canvas');
    canvas.width = space.columns * space.chipWidth;
    canvas.height = space.rows * space.chipHeight;
    const canvasCtx = canvas.getContext('2d');
    const imageData = canvasCtx.createImageData(canvas.width, canvas.height);
    const data = imageData.data;

    for (let row = 0; row < space.rows; row++) {
      for (let col = 0; col < space.columns; col++) {
        const pixelData = space.tilesetData.compositeChip(col, row);
        if (!pixelData?.pixels) continue;
        for (let y = 0; y < space.chipHeight; y++) {
          for (let x = 0; x < space.chipWidth; x++) {
            const color = pixelData.pixels[y * space.chipWidth + x];
            const target = (((row * space.chipHeight + y) * canvas.width) + col * space.chipWidth + x) * 4;
            data[target] = (color >>> 16) & 0xFF;
            data[target + 1] = (color >>> 8) & 0xFF;
            data[target + 2] = color & 0xFF;
            data[target + 3] = (color >>> 24) & 0xFF;
          }
        }
      }
    }
    canvasCtx.putImageData(imageData, 0, 0);
    this._atlasCache.set(space.tilesetId, canvas);
    return canvas;
  }

  /**
   * 世界座標の矩形が覆うセル範囲。right / bottom は排他的に扱う。
   * @returns {{fromCol: number, toCol: number, fromRow: number, toRow: number}}
   */
  static _cellRange(originX, originY, cellW, cellH, left, top, right, bottom) {
    const fromCol = Math.floor((left - originX) / cellW);
    const fromRow = Math.floor((top - originY) / cellH);
    return {
      fromCol,
      toCol: Math.max(fromCol, Math.ceil((right - originX) / cellW) - 1),
      fromRow,
      toRow: Math.max(fromRow, Math.ceil((bottom - originY) / cellH) - 1),
    };
  }

  static _isInside(space, col, row) {
    return col >= 0 && row >= 0 && col < space.width && row < space.height;
  }

  /**
   * マップ外のセルが、別の Tilemap の範囲内にあるかを中心点で判定する。
   * @param {Array<object>} spaces
   * @param {object} target
   * @param {number} col
   * @param {number} row
   * @returns {boolean}
   */
  static _isCellCenterCoveredByOther(spaces, target, col, row) {
    const centerX = target.originX + (col + 0.5) * target.cellW;
    const centerY = target.originY + (row + 0.5) * target.cellH;
    return spaces.some(other => {
      if (other === target) return false;
      const otherCol = Math.floor((centerX - other.originX) / other.cellW);
      const otherRow = Math.floor((centerY - other.originY) / other.cellH);
      return TilemapRuntime._isInside(other.space, otherCol, otherRow);
    });
  }
}
