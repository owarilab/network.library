/**
 * PixelStrokeCommand
 * 1ストローク分のピクセル差分を保持する。
 */
class PixelStrokeCommand extends CommandBase {
  /**
   * @param {{
   *   mode: 'free'|'tileset',
   *   chip: { col: number, row: number }|null,
   *   layerIndex: number,
   *   changes: Array<{ x: number, y: number, before: number, after: number }>,
   *   label?: string,
   * }} params
   */
  constructor(params) {
    super();
    this._mode = params?.mode === 'tileset' ? 'tileset' : 'free';
    this._chip = this._mode === 'tileset' && params?.chip
      ? { col: params.chip.col | 0, row: params.chip.row | 0 }
      : null;
    this._layerIndex = params?.layerIndex | 0;
    this._label = typeof params?.label === 'string' && params.label
      ? params.label
      : 'stroke';
    this._changes = Array.isArray(params?.changes)
      ? params.changes.map(change => ({
          x: change.x | 0,
          y: change.y | 0,
          before: change.before >>> 0,
          after: change.after >>> 0,
        }))
      : [];
  }

  /**
   * @param {AppData} appData
   * @returns {boolean}
   */
  execute(appData) {
    return this._apply(appData, 'after');
  }

  /**
   * @param {AppData} appData
   * @returns {boolean}
   */
  undo(appData) {
    return this._apply(appData, 'before');
  }

  /**
   * @returns {string}
   */
  getLabel() {
    return this._label;
  }

  /**
   * @param {AppData} appData
   * @param {'before'|'after'} key
   * @returns {boolean}
   */
  _apply(appData, key) {
    if (!appData || this._changes.length === 0) return false;

    const layerData = this._resolveLayerData(appData);
    if (!layerData) return false;
    const layer = layerData.layers?.[this._layerIndex];
    if (!layer?.pixelData) return false;

    for (const change of this._changes) {
      layer.pixelData.setPixel(change.x, change.y, change[key]);
    }
    layerData.markCompositeDirty();
    return true;
  }

  /**
   * @param {AppData} appData
   * @returns {LayerData|null}
   */
  _resolveLayerData(appData) {
    if (this._mode === 'tileset') {
      if (!appData.tilesetData || !this._chip) return null;
      return appData.tilesetData.getChipLayerData(this._chip.col, this._chip.row);
    }
    return appData._layerData || null;
  }
}