/**
 * NewFileCommand
 * 通常新規作成を状態スナップショット付きで扱う。
 */
class NewFileCommand extends CommandBase {
  /**
   * @param {{ width: number, height: number, fillColor: number, label?: string }} params
   */
  constructor(params) {
    super();
    this._width = params?.width | 0;
    this._height = params?.height | 0;
    this._fillColor = params?.fillColor >>> 0;
    this._label = typeof params?.label === 'string' && params.label ? params.label : 'newFile';
    this._before = null;
    this._after = null;
  }

  /**
   * @param {AppData} appData
   * @returns {boolean}
   */
  execute(appData) {
    if (!appData || this._width <= 0 || this._height <= 0) return false;

    if (!this._before) {
      this._before = appData.createEditStateSnapshot();
      this._applyNewFile(appData);
      this._after = appData.createEditStateSnapshot();
      return true;
    }

    return appData.applyEditStateSnapshot(this._after);
  }

  /**
   * @param {AppData} appData
   * @returns {boolean}
   */
  undo(appData) {
    if (!appData || !this._before) return false;
    return appData.applyEditStateSnapshot(this._before);
  }

  /**
   * @returns {string}
   */
  getLabel() {
    return this._label;
  }

  /**
   * @param {AppData} appData
   */
  _applyNewFile(appData) {
    appData.editMode = 'free';
    appData.tilesetData = null;
    appData.clearSelection();
    appData.createPixelData(this._width, this._height, this._fillColor);
  }
}