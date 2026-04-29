/**
 * PlayUnitRuntime
 * PlayUnitData から最小の実行時表示データを生成する。
 */
class PlayUnitRuntime {
  constructor() {
    this.id = '';
    this.name = 'PlayUnit';
    this.camera = {
      objectId: '',
      objectName: 'DefaultCamera',
      followTargetObjectId: '',
      followTargetObjectName: '',
      followLerp: 1,
      sourceX: 0,
      sourceY: 0,
      targetX: 0,
      targetY: 0,
      x: 0,
      y: 0,
      zoom: 1,
    };
    this.imageEntries = [];
    this.textEntries = [];
    this.rectangleEntries = [];
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
    runtime.camera = PlayUnitRuntime._resolveCamera(objects);
    const imageEntries = [];
    const textEntries = [];
    const rectangleEntries = [];
    for (let index = 0; index < objects.length; index++) {
      const objectData = objects[index];
      if (!objectData || objectData.enabled === false) continue;

      const transform = PlayUnitRuntime._findEnabledComponent(objectData, 'Transform');
      if (!transform) continue;

      const image = PlayUnitRuntime._findEnabledComponent(objectData, 'Image');
      if (image) {
        const pixelDocumentId = typeof image.data?.pixelDocumentId === 'string' ? image.data.pixelDocumentId.trim() : '';
        if (pixelDocumentId) {
          imageEntries.push({
            objectId: objectData.id || '',
            objectName: objectData.name || 'Object',
            pixelDocumentId,
            alpha: PlayUnitRuntime._normalizeAlpha(image.data?.alpha),
            width: PlayUnitRuntime._normalizePositiveNumber(image.data?.width, 0),
            height: PlayUnitRuntime._normalizePositiveNumber(image.data?.height, 0),
            keepAspect: image.data?.keepAspect !== false,
            originX: PlayUnitRuntime._normalizeUnitInterval(image.data?.originX, 0),
            originY: PlayUnitRuntime._normalizeUnitInterval(image.data?.originY, 0),
            x: Number.isFinite(Number(transform.data?.x)) ? Number(transform.data.x) : 0,
            y: Number.isFinite(Number(transform.data?.y)) ? Number(transform.data.y) : 0,
            z: Number.isFinite(Number(transform.data?.z)) ? Number(transform.data.z) : 0,
            order: index,
          });
        }
      }

      const text = PlayUnitRuntime._findEnabledComponent(objectData, 'Text');
      if (text) {
        const textValue = typeof text.data?.text === 'string' ? text.data.text : '';
        if (textValue) {
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
      }

      const rectangle = PlayUnitRuntime._findEnabledComponent(objectData, 'Rectangle');
      if (rectangle) {
        const shape = typeof rectangle.data?.shape === 'string' ? rectangle.data.shape.trim() : 'rectangle';
        rectangleEntries.push({
          objectId: objectData.id || '',
          objectName: objectData.name || 'Object',
          shape,
          width: PlayUnitRuntime._normalizePositiveNumber(rectangle.data?.width, 64),
          height: PlayUnitRuntime._normalizePositiveNumber(rectangle.data?.height, 32),
          fillColor: typeof rectangle.data?.fillColor === 'string' && rectangle.data.fillColor.trim() ? rectangle.data.fillColor.trim() : '#ffffff',
          fillAlpha: PlayUnitRuntime._normalizeAlpha(rectangle.data?.fillAlpha),
          strokeColor: typeof rectangle.data?.strokeColor === 'string' && rectangle.data.strokeColor.trim() ? rectangle.data.strokeColor.trim() : '#000000',
          strokeWidth: PlayUnitRuntime._normalizePositiveNumber(rectangle.data?.strokeWidth, 2),
          strokeAlpha: PlayUnitRuntime._normalizeAlpha(rectangle.data?.strokeAlpha),
          rotation: Number.isFinite(Number(rectangle.data?.rotation)) ? Number(rectangle.data.rotation) : 0,
          originX: PlayUnitRuntime._normalizeUnitInterval(rectangle.data?.originX, 0),
          originY: PlayUnitRuntime._normalizeUnitInterval(rectangle.data?.originY, 0),
          sides: PlayUnitRuntime._normalizePositiveNumber(rectangle.data?.sides, 4),
          points: PlayUnitRuntime._normalizePositiveNumber(rectangle.data?.points, 5),
          innerRadius: PlayUnitRuntime._normalizeUnitInterval(rectangle.data?.innerRadius, 0.4),
          x: Number.isFinite(Number(transform.data?.x)) ? Number(transform.data.x) : 0,
          y: Number.isFinite(Number(transform.data?.y)) ? Number(transform.data.y) : 0,
          z: Number.isFinite(Number(transform.data?.z)) ? Number(transform.data.z) : 0,
          order: index,
        });
      }
    }

    runtime.textEntries = textEntries.sort((a, b) => {
      if (a.z !== b.z) return a.z - b.z;
      return a.order - b.order;
    });
    runtime.imageEntries = imageEntries.sort((a, b) => {
      if (a.z !== b.z) return a.z - b.z;
      return a.order - b.order;
    });
    runtime.rectangleEntries = rectangleEntries.sort((a, b) => {
      if (a.z !== b.z) return a.z - b.z;
      return a.order - b.order;
    });
    return runtime;
  }

  /**
   * @param {Array<PlayObjectData|object>} objects
   * @returns {{objectId: string, objectName: string, x: number, y: number, zoom: number}}
   */
  static _resolveCamera(objects) {
    const fallbackCamera = {
      objectId: '',
      objectName: 'DefaultCamera',
      followTargetObjectId: '',
      followTargetObjectName: '',
      followLerp: 1,
      sourceX: 0,
      sourceY: 0,
      targetX: 0,
      targetY: 0,
      x: 0,
      y: 0,
      zoom: 1,
    };

    if (!Array.isArray(objects) || !objects.length) return fallbackCamera;

    const playSettingsObject = objects.find((objectData) => {
      if (!objectData || objectData.enabled === false) return false;
      return !!PlayUnitRuntime._findEnabledComponent(objectData, 'PlaySettings');
    }) || null;

    const defaultCameraObjectId = typeof PlayUnitRuntime._findEnabledComponent(playSettingsObject, 'PlaySettings')?.data?.defaultCameraObjectId === 'string'
      ? PlayUnitRuntime._findEnabledComponent(playSettingsObject, 'PlaySettings').data.defaultCameraObjectId.trim()
      : '';

    let cameraObject = null;
    if (defaultCameraObjectId) {
      cameraObject = objects.find((objectData) => objectData?.id === defaultCameraObjectId) || null;
    }

    if (!cameraObject) {
      cameraObject = objects.find((objectData) => {
        if (!objectData || objectData.enabled === false) return false;
        return !!PlayUnitRuntime._findEnabledComponent(objectData, 'Camera');
      }) || null;
    }

    const cameraComponent = PlayUnitRuntime._findEnabledComponent(cameraObject, 'Camera');
    const transformComponent = PlayUnitRuntime._findEnabledComponent(cameraObject, 'Transform');
    if (!cameraObject || !cameraComponent || !transformComponent) return fallbackCamera;

    const followTargetObjectId = typeof cameraComponent.data?.followTargetObjectId === 'string'
      ? cameraComponent.data.followTargetObjectId.trim()
      : '';
    const followTargetObject = followTargetObjectId
      ? objects.find((objectData) => objectData?.id === followTargetObjectId && objectData.enabled !== false) || null
      : null;
    const followTargetTransform = PlayUnitRuntime._findEnabledComponent(followTargetObject, 'Transform');
    const sourceX = Number.isFinite(Number(transformComponent.data?.x)) ? Number(transformComponent.data.x) : 0;
    const sourceY = Number.isFinite(Number(transformComponent.data?.y)) ? Number(transformComponent.data.y) : 0;
    const targetX = followTargetTransform && Number.isFinite(Number(followTargetTransform.data?.x))
      ? Number(followTargetTransform.data.x)
      : sourceX;
    const targetY = followTargetTransform && Number.isFinite(Number(followTargetTransform.data?.y))
      ? Number(followTargetTransform.data.y)
      : sourceY;
    const followLerp = PlayUnitRuntime._normalizeUnitInterval(cameraComponent.data?.followLerp, 1);

    const cameraX = followTargetTransform ? targetX : sourceX;
    const cameraY = followTargetTransform ? targetY : sourceY;

    return {
      objectId: typeof cameraObject.id === 'string' ? cameraObject.id : '',
      objectName: typeof cameraObject.name === 'string' && cameraObject.name.trim() ? cameraObject.name.trim() : 'CameraObject',
      followTargetObjectId,
      followTargetObjectName: typeof followTargetObject?.name === 'string' && followTargetObject.name.trim() ? followTargetObject.name.trim() : '',
      followLerp,
      sourceX,
      sourceY,
      targetX,
      targetY,
      x: cameraX,
      y: cameraY,
      zoom: PlayUnitRuntime._normalizePositiveNumber(cameraComponent.data?.zoom, 1),
    };
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

  /**
   * @param {number|string|undefined|null} value
   * @param {number} fallback
   * @returns {number}
   */
  static _normalizeUnitInterval(value, fallback) {
    const nextValue = Number(value);
    if (!Number.isFinite(nextValue)) return fallback;
    if (nextValue < 0) return 0;
    if (nextValue > 1) return 1;
    return nextValue;
  }
}