/**
 * TransformCommand
 * 通常レイヤーへの反転 / 回転をスナップショットベースで保持する。
 */
class TransformCommand extends CommandBase {
  /**
   * @param {{
   *   mode: 'free'|'tileset',
   *   chip: { col: number, row: number }|null,
   *   layerIndices: number[],
   *   kind: 'flipH'|'flipV'|'rotate90CW'|'rotate90CCW',
   *   label?: string,
   * }} params
   */
  constructor(params) {
    super();
    this._mode = params?.mode === 'tileset' ? 'tileset' : 'free';
    this._chip = this._mode === 'tileset' && params?.chip
      ? { col: params.chip.col | 0, row: params.chip.row | 0 }
      : null;
    this._layerIndices = Array.isArray(params?.layerIndices)
      ? params.layerIndices.map(index => index | 0)
      : [];
    this._kind = params?.kind || 'flipH';
    this._label = typeof params?.label === 'string' && params.label
      ? params.label
      : this._kind;
    this._before = null;
    this._after = null;
  }

  /**
   * @param {AppData} appData
   * @returns {boolean}
   */
  execute(appData) {
    const layerData = this._resolveLayerData(appData);
    if (!layerData || this._layerIndices.length === 0) return false;

    if (!this._before) {
      this._before = this._captureSnapshots(layerData);
    }

    if (this._after) {
      return this._applySnapshots(layerData, this._after);
    }

    let changed = false;
    for (const index of this._layerIndices) {
      const layer = layerData.layers?.[index];
      if (!layer?.pixelData) continue;
      this._applyTransform(layer.pixelData);
      changed = true;
    }
    if (!changed) return false;

    layerData.markCompositeDirty();
    this._after = this._captureSnapshots(layerData);
    return true;
  }

  /**
   * @param {AppData} appData
   * @returns {boolean}
   */
  undo(appData) {
    const layerData = this._resolveLayerData(appData);
    if (!layerData || !this._before) return false;
    return this._applySnapshots(layerData, this._before);
  }

  /**
   * @returns {string}
   */
  getLabel() {
    return this._label;
  }

  /**
   * @param {AppData} appData
   * @returns {LayerData|null}
   */
  _resolveLayerData(appData) {
    if (!appData) return null;
    if (this._mode === 'tileset') {
      if (!appData.tilesetData || !this._chip) return null;
      return appData.tilesetData.getChipLayerData(this._chip.col, this._chip.row);
    }
    return appData._layerData || null;
  }

  /**
   * @param {LayerData} layerData
   * @returns {Array<{ index: number, width: number, height: number, pixels: Uint32Array }>}
   */
  _captureSnapshots(layerData) {
    const snapshots = [];
    for (const index of this._layerIndices) {
      const layer = layerData.layers?.[index];
      if (!layer?.pixelData?.pixels) continue;
      snapshots.push({
        index,
        width: layer.pixelData.width,
        height: layer.pixelData.height,
        pixels: new Uint32Array(layer.pixelData.pixels),
      });
    }
    return snapshots;
  }

  /**
   * @param {LayerData} layerData
   * @param {Array<{ index: number, width: number, height: number, pixels: Uint32Array }>} snapshots
   * @returns {boolean}
   */
  _applySnapshots(layerData, snapshots) {
    let changed = false;
    for (const snapshot of snapshots) {
      const layer = layerData.layers?.[snapshot.index];
      if (!layer?.pixelData) continue;
      layer.pixelData.width = snapshot.width;
      layer.pixelData.height = snapshot.height;
      layer.pixelData.pixels = new Uint32Array(snapshot.pixels);
      changed = true;
    }
    if (!changed) return false;
    layerData.markCompositeDirty();
    return true;
  }

  /**
   * @param {PixelData} pixelData
   */
  _applyTransform(pixelData) {
    switch (this._kind) {
      case 'flipH':
        pixelData.flipH();
        break;
      case 'flipV':
        pixelData.flipV();
        break;
      case 'rotate90CW':
        pixelData.rotate90CW();
        break;
      case 'rotate90CCW':
        pixelData.rotate90CCW();
        break;
    }
  }
}