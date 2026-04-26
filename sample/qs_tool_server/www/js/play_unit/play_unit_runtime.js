/**
 * PlayUnitRuntime
 * PlayUnitData から最小の実行時表示データを生成する。
 */
class PlayUnitRuntime {
  constructor() {
    this.id = '';
    this.name = 'PlayUnit';
    this.textEntries = [];
  }

  /**
   * @param {PlayUnitData|object|null} playUnit
   * @returns {PlayUnitRuntime}
   */
  static fromPlayUnit(playUnit) {
    const runtime = new PlayUnitRuntime();
    runtime.id = typeof playUnit?.id === 'string' ? playUnit.id : '';
    runtime.name = typeof playUnit?.name === 'string' && playUnit.name.trim() ? playUnit.name.trim() : 'PlayUnit';

    const objects = Array.isArray(playUnit?.objects) ? playUnit.objects : [];
    const textEntries = [];
    for (let index = 0; index < objects.length; index++) {
      const objectData = objects[index];
      if (!objectData || objectData.enabled === false) continue;

      const transform = PlayUnitRuntime._findEnabledComponent(objectData, 'Transform');
      const text = PlayUnitRuntime._findEnabledComponent(objectData, 'Text');
      if (!transform || !text) continue;

      const textValue = typeof text.data?.text === 'string' ? text.data.text : '';
      if (!textValue) continue;

      textEntries.push({
        objectId: objectData.id || '',
        objectName: objectData.name || 'Object',
        text: textValue,
        font: typeof text.data?.font === 'string' && text.data.font.trim() ? text.data.font.trim() : '24px sans-serif',
        color: typeof text.data?.color === 'string' && text.data.color.trim() ? text.data.color.trim() : '#ffffff',
        alpha: PlayUnitRuntime._normalizeAlpha(text.data?.alpha),
        align: PlayUnitRuntime._normalizeTextAlign(text.data?.align),
        baseline: PlayUnitRuntime._normalizeTextBaseline(text.data?.baseline),
        wrap: text.data?.wrap === true,
        maxWidth: PlayUnitRuntime._normalizePositiveNumber(text.data?.maxWidth, 0),
        lineHeight: PlayUnitRuntime._normalizePositiveNumber(text.data?.lineHeight, 28),
        strokeColor: typeof text.data?.strokeColor === 'string' && text.data.strokeColor.trim() ? text.data.strokeColor.trim() : '',
        strokeWidth: PlayUnitRuntime._normalizePositiveNumber(text.data?.strokeWidth, 0),
        backgroundColor: typeof text.data?.backgroundColor === 'string' && text.data.backgroundColor.trim() ? text.data.backgroundColor.trim() : '',
        padding: PlayUnitRuntime._normalizePositiveNumber(text.data?.padding, 0),
        x: Number.isFinite(Number(transform.data?.x)) ? Number(transform.data.x) : 0,
        y: Number.isFinite(Number(transform.data?.y)) ? Number(transform.data.y) : 0,
        z: Number.isFinite(Number(transform.data?.z)) ? Number(transform.data.z) : 0,
        order: index,
      });
    }

    runtime.textEntries = textEntries.sort((a, b) => {
      if (a.z !== b.z) return a.z - b.z;
      return a.order - b.order;
    });
    return runtime;
  }

  /**
   * @param {PlayObjectData|object|null} objectData
   * @param {string} type
   * @returns {ComponentData|object|null}
   */
  static _findEnabledComponent(objectData, type) {
    const components = Array.isArray(objectData?.components) ? objectData.components : [];
    for (const component of components) {
      if (!component || component.enabled === false) continue;
      if (component.type === type) return component;
    }
    return null;
  }

  /**
   * @param {string|undefined|null} value
   * @returns {'left'|'center'|'right'}
   */
  static _normalizeTextAlign(value) {
    return value === 'center' || value === 'right' ? value : 'left';
  }

  /**
   * @param {string|undefined|null} value
   * @returns {'top'|'middle'|'bottom'}
   */
  static _normalizeTextBaseline(value) {
    return value === 'middle' || value === 'bottom' ? value : 'top';
  }

  /**
   * @param {number|string|undefined|null} value
   * @param {number} fallback
   * @returns {number}
   */
  static _normalizePositiveNumber(value, fallback) {
    const nextValue = Number(value);
    return Number.isFinite(nextValue) && nextValue > 0 ? nextValue : fallback;
  }

  /**
   * @param {number|string|undefined|null} value
   * @returns {number}
   */
  static _normalizeAlpha(value) {
    const nextValue = Number(value);
    if (!Number.isFinite(nextValue)) return 1;
    if (nextValue < 0) return 0;
    if (nextValue > 1) return 1;
    return nextValue;
  }
}