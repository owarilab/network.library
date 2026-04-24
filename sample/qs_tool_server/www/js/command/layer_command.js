/**
 * LayerCommandBase
 * レイヤー操作コマンド共通の対象解決とスナップショット処理を持つ。
 */
class LayerCommandBase extends CommandBase {
  /**
   * @param {{ mode?: 'free'|'tileset', chip?: { col: number, row: number }|null, label?: string }} params
   */
  constructor(params) {
    super();
    this._mode = params?.mode === 'tileset' ? 'tileset' : 'free';
    this._chip = this._mode === 'tileset' && params?.chip
      ? { col: params.chip.col | 0, row: params.chip.row | 0 }
      : null;
    this._label = typeof params?.label === 'string' && params.label
      ? params.label
      : 'layer';
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
   * @param {PixelData} src
   * @returns {PixelData}
   */
  _clonePixelData(src) {
    const pixelData = new PixelData();
    pixelData.createPixelData(src.width, src.height);
    pixelData.pixels.set(src.pixels);
    return pixelData;
  }

  /**
   * @param {LayerData} layerData
   * @returns {{ width: number, height: number, activeIndex: number, layers: Array<{ pixelData: PixelData, name: string, visible: boolean, opacity: number, locked: boolean }> }}
   */
  _captureSnapshot(layerData) {
    return {
      width: layerData.width,
      height: layerData.height,
      activeIndex: layerData.activeIndex,
      layers: layerData.layers.map(layer => ({
        pixelData: this._clonePixelData(layer.pixelData),
        name: layer.name,
        visible: layer.visible !== false,
        opacity: layer.opacity | 0,
        locked: !!layer.locked,
      })),
    };
  }

  /**
   * @param {LayerData} layerData
   * @param {{ width: number, height: number, activeIndex: number, layers: Array<{ pixelData: PixelData, name: string, visible: boolean, opacity: number, locked: boolean }> }} snapshot
   * @returns {boolean}
   */
  _applySnapshot(layerData, snapshot) {
    if (!layerData || !snapshot || !Array.isArray(snapshot.layers)) return false;

    layerData.width = snapshot.width | 0;
    layerData.height = snapshot.height | 0;
    layerData.layers = snapshot.layers.map(layer => ({
      pixelData: this._clonePixelData(layer.pixelData),
      name: layer.name,
      visible: layer.visible !== false,
      opacity: layer.opacity | 0,
      locked: !!layer.locked,
    }));
    layerData.activeIndex = Math.max(0, Math.min(snapshot.activeIndex | 0, layerData.layers.length - 1));
    layerData._composite.createPixelData(layerData.width, layerData.height);
    layerData.markCompositeDirty();
    return true;
  }
}

/**
 * StructureLayerCommandBase
 * add/remove/move/duplicate/merge など構造変更をスナップショットで扱う。
 */
class StructureLayerCommandBase extends LayerCommandBase {
  /**
   * @param {{ mode?: 'free'|'tileset', chip?: { col: number, row: number }|null, label?: string }} params
   */
  constructor(params) {
    super(params);
    this._before = null;
    this._after = null;
  }

  /**
   * @param {AppData} appData
   * @returns {boolean}
   */
  execute(appData) {
    const layerData = this._resolveLayerData(appData);
    if (!layerData) return false;

    if (this._after) {
      return this._applySnapshot(layerData, this._after);
    }

    this._before = this._captureSnapshot(layerData);
    const changed = this._applyInitial(layerData);
    if (changed === false) {
      this._before = null;
      return false;
    }
    this._after = this._captureSnapshot(layerData);
    return true;
  }

  /**
   * @param {AppData} appData
   * @returns {boolean}
   */
  undo(appData) {
    const layerData = this._resolveLayerData(appData);
    if (!layerData || !this._before) return false;
    return this._applySnapshot(layerData, this._before);
  }
}

class AddLayerCommand extends StructureLayerCommandBase {
  /**
   * @param {{ mode?: 'free'|'tileset', chip?: { col: number, row: number }|null, name?: string, label?: string }} params
   */
  constructor(params) {
    super({ ...params, label: params?.label || 'addLayer' });
    this._name = typeof params?.name === 'string' ? params.name : undefined;
  }

  _applyInitial(layerData) {
    return layerData.addLayer(this._name) >= 0;
  }
}

class RemoveLayerCommand extends StructureLayerCommandBase {
  /**
   * @param {{ mode?: 'free'|'tileset', chip?: { col: number, row: number }|null, index: number, label?: string }} params
   */
  constructor(params) {
    super({ ...params, label: params?.label || 'removeLayer' });
    this._index = params?.index | 0;
  }

  _applyInitial(layerData) {
    return layerData.removeLayer(this._index);
  }
}

class MoveLayerCommand extends StructureLayerCommandBase {
  /**
   * @param {{ mode?: 'free'|'tileset', chip?: { col: number, row: number }|null, from: number, to: number, label?: string }} params
   */
  constructor(params) {
    super({ ...params, label: params?.label || 'moveLayer' });
    this._from = params?.from | 0;
    this._to = params?.to | 0;
  }

  _applyInitial(layerData) {
    if (this._from === this._to) return false;
    if (this._from < 0 || this._from >= layerData.layers.length) return false;
    if (this._to < 0 || this._to >= layerData.layers.length) return false;
    layerData.moveLayer(this._from, this._to);
    return true;
  }
}

class DuplicateLayerCommand extends StructureLayerCommandBase {
  /**
   * @param {{ mode?: 'free'|'tileset', chip?: { col: number, row: number }|null, index: number, label?: string }} params
   */
  constructor(params) {
    super({ ...params, label: params?.label || 'duplicateLayer' });
    this._index = params?.index | 0;
  }

  _applyInitial(layerData) {
    return layerData.duplicateLayer(this._index) >= 0;
  }
}

class MergeLayerDownCommand extends StructureLayerCommandBase {
  /**
   * @param {{ mode?: 'free'|'tileset', chip?: { col: number, row: number }|null, index: number, label?: string }} params
   */
  constructor(params) {
    super({ ...params, label: params?.label || 'mergeLayerDown' });
    this._index = params?.index | 0;
  }

  _applyInitial(layerData) {
    return layerData.mergeLayerDown(this._index);
  }
}

class ToggleLayerVisibilityCommand extends LayerCommandBase {
  /**
   * @param {{ mode?: 'free'|'tileset', chip?: { col: number, row: number }|null, index: number, label?: string }} params
   */
  constructor(params) {
    super({ ...params, label: params?.label || 'toggleLayerVisibility' });
    this._index = params?.index | 0;
    this._before = null;
    this._after = null;
  }

  execute(appData) {
    const layerData = this._resolveLayerData(appData);
    const layer = layerData?.layers?.[this._index];
    if (!layer) return false;

    if (this._after === null) {
      this._before = !!layer.visible;
      layerData.toggleVisibility(this._index);
      this._after = !!layerData.layers[this._index].visible;
      return this._before !== this._after;
    }

    return this._applyValue(layerData, this._after);
  }

  undo(appData) {
    const layerData = this._resolveLayerData(appData);
    if (!layerData || this._before === null) return false;
    return this._applyValue(layerData, this._before);
  }

  _applyValue(layerData, value) {
    const layer = layerData?.layers?.[this._index];
    if (!layer) return false;
    layer.visible = !!value;
    layerData.markCompositeDirty();
    return true;
  }
}

class ToggleLayerLockedCommand extends LayerCommandBase {
  /**
   * @param {{ mode?: 'free'|'tileset', chip?: { col: number, row: number }|null, index: number, label?: string }} params
   */
  constructor(params) {
    super({ ...params, label: params?.label || 'toggleLayerLocked' });
    this._index = params?.index | 0;
    this._before = null;
    this._after = null;
  }

  execute(appData) {
    const layerData = this._resolveLayerData(appData);
    const layer = layerData?.layers?.[this._index];
    if (!layer) return false;

    if (this._after === null) {
      this._before = !!layer.locked;
      layerData.toggleLocked(this._index);
      this._after = !!layerData.layers[this._index].locked;
      return this._before !== this._after;
    }

    return this._applyValue(layerData, this._after);
  }

  undo(appData) {
    const layerData = this._resolveLayerData(appData);
    if (!layerData || this._before === null) return false;
    return this._applyValue(layerData, this._before);
  }

  _applyValue(layerData, value) {
    const layer = layerData?.layers?.[this._index];
    if (!layer) return false;
    layer.locked = !!value;
    return true;
  }
}

class RenameLayerCommand extends LayerCommandBase {
  /**
   * @param {{ mode?: 'free'|'tileset', chip?: { col: number, row: number }|null, index: number, newName: string, label?: string }} params
   */
  constructor(params) {
    super({ ...params, label: params?.label || 'renameLayer' });
    this._index = params?.index | 0;
    this._newName = typeof params?.newName === 'string' ? params.newName : '';
    this._before = null;
    this._after = null;
  }

  execute(appData) {
    const layerData = this._resolveLayerData(appData);
    const layer = layerData?.layers?.[this._index];
    if (!layer) return false;

    if (this._after === null) {
      this._before = layer.name;
      if (!layerData.renameLayer(this._index, this._newName)) return false;
      this._after = layerData.layers[this._index].name;
      return this._before !== this._after;
    }

    return this._applyValue(layerData, this._after);
  }

  undo(appData) {
    const layerData = this._resolveLayerData(appData);
    if (!layerData || this._before === null) return false;
    return this._applyValue(layerData, this._before);
  }

  _applyValue(layerData, value) {
    const layer = layerData?.layers?.[this._index];
    if (!layer) return false;
    layer.name = value;
    return true;
  }
}

class SetLayerOpacityCommand extends LayerCommandBase {
  /**
   * @param {{ mode?: 'free'|'tileset', chip?: { col: number, row: number }|null, index: number, opacity: number, beforeOpacity?: number, label?: string }} params
   */
  constructor(params) {
    super({ ...params, label: params?.label || 'setLayerOpacity' });
    this._index = params?.index | 0;
    this._opacity = Math.max(0, Math.min(255, params?.opacity | 0));
    this._before = typeof params?.beforeOpacity === 'number'
      ? Math.max(0, Math.min(255, params.beforeOpacity | 0))
      : null;
  }

  execute(appData) {
    const layerData = this._resolveLayerData(appData);
    const layer = layerData?.layers?.[this._index];
    if (!layer) return false;
    if (this._before === null) {
      this._before = layer.opacity | 0;
    }
    if (this._before === this._opacity) return false;
    return this._applyValue(layerData, this._opacity);
  }

  undo(appData) {
    const layerData = this._resolveLayerData(appData);
    if (!layerData || this._before === null) return false;
    return this._applyValue(layerData, this._before);
  }

  _applyValue(layerData, value) {
    const layer = layerData?.layers?.[this._index];
    if (!layer) return false;
    layer.opacity = Math.max(0, Math.min(255, value | 0));
    layerData.markCompositeDirty();
    return true;
  }
}