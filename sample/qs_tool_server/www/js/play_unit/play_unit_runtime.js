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
      viewportWidth: 800,
      viewportHeight: 600,
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
    this.uiEntries = [];
  }

  /**
   * @param {PlayUnitData|object|null} playUnit
   * @param {AppData|null} appData
   * @returns {PlayUnitRuntime}
   */
  static fromPlayUnit(playUnit, appData = null) {
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
          const resolvedText = PlayUnitRuntime._resolveTemplateText(textValue, appData);
          textEntries.push({
            objectId: objectData.id || '',
            objectName: objectData.name || 'Object',
            text: resolvedText,
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
    runtime.uiEntries = PlayUnitRuntime._resolveUIEntries(objects, appData);
    return runtime;
  }

  /**
   * @param {Array<PlayObjectData|object>} objects
   * @param {AppData|null} appData
   * @returns {Array<object>}
   */
  static _resolveUIEntries(objects, appData) {
    if (!Array.isArray(objects) || !objects.length) return [];

    const objectMap = new Map();
    for (const objectData of objects) {
      if (!objectData || typeof objectData.id !== 'string' || !objectData.id) continue;
      objectMap.set(objectData.id, objectData);
    }

    const uiEntries = [];
    let order = 0;
    for (const objectData of objects) {
      if (!objectData || objectData.enabled === false) continue;
      const uiCanvas = PlayUnitRuntime._findEnabledComponent(objectData, 'UICanvas');
      if (!uiCanvas) continue;
      if (PlayUnitRuntime._hasEnabledAncestorCanvas(objectData, objectMap)) continue;

      const canvasSortOrder = Number.isFinite(Number(uiCanvas.data?.sortOrder)) ? Number(uiCanvas.data.sortOrder) : 0;
      const referenceWidth = PlayUnitRuntime._normalizePositiveNumber(uiCanvas.data?.referenceWidth, 640);
      const referenceHeight = PlayUnitRuntime._normalizePositiveNumber(uiCanvas.data?.referenceHeight, 360);
      const rootRect = {
        left: 0,
        top: 0,
        width: referenceWidth,
        height: referenceHeight,
        right: referenceWidth,
        bottom: referenceHeight,
      };

      const childIds = Array.isArray(objectData.children) ? objectData.children : [];
      for (const childId of childIds) {
        order = PlayUnitRuntime._collectUIEntries({
          objectId: childId,
          objectMap,
          parentRect: rootRect,
          canvasObject: objectData,
          canvasSortOrder,
            canvasWidth: referenceWidth,
            canvasHeight: referenceHeight,
          uiEntries,
          order,
          appData,
        });
      }
    }

    return uiEntries.sort((a, b) => {
      if (a.canvasSortOrder !== b.canvasSortOrder) return a.canvasSortOrder - b.canvasSortOrder;
      return a.order - b.order;
    });
  }

  /**
   * @param {{
   *   objectId: string,
   *   objectMap: Map<string, PlayObjectData|object>,
   *   parentRect: {left:number,top:number,width:number,height:number,right:number,bottom:number},
   *   canvasObject: PlayObjectData|object,
   *   canvasSortOrder: number,
   *   uiEntries: Array<object>,
   *   order: number,
   *   appData: AppData|null,
   * }} params
   * @returns {number}
   */
  static _collectUIEntries(params) {
    const objectData = params.objectMap.get(params.objectId) || null;
    if (!objectData || objectData.enabled === false) return params.order;

    const uiTransform = PlayUnitRuntime._findEnabledComponent(objectData, 'UITransform');
    if (!uiTransform) return params.order;

    const rect = PlayUnitRuntime._resolveUIRect(uiTransform, params.parentRect);
    let nextOrder = params.order;
    nextOrder = PlayUnitRuntime._appendUIRenderableEntries({
      objectData,
      rect,
      canvasObject: params.canvasObject,
      canvasSortOrder: params.canvasSortOrder,
      canvasWidth: params.canvasWidth,
      canvasHeight: params.canvasHeight,
      uiEntries: params.uiEntries,
      order: nextOrder,
      appData: params.appData,
    });

    const childIds = Array.isArray(objectData.children) ? objectData.children : [];
    for (const childId of childIds) {
      nextOrder = PlayUnitRuntime._collectUIEntries({
        ...params,
        objectId: childId,
        parentRect: rect,
        order: nextOrder,
      });
    }
    return nextOrder;
  }

  /**
   * @param {{
   *   objectData: PlayObjectData|object,
   *   rect: {left:number,top:number,width:number,height:number,right:number,bottom:number},
   *   canvasObject: PlayObjectData|object,
   *   canvasSortOrder: number,
   *   uiEntries: Array<object>,
   *   order: number,
   *   appData: AppData|null,
   * }} params
   * @returns {number}
   */
  static _appendUIRenderableEntries(params) {
    const { objectData, rect, canvasObject, canvasSortOrder, uiEntries } = params;
    let nextOrder = params.order;

    const baseEntry = {
      objectId: objectData.id || '',
      objectName: objectData.name || 'Object',
      canvasObjectId: canvasObject.id || '',
      canvasObjectName: canvasObject.name || 'UICanvas',
      canvasSortOrder,
        canvasWidth: params.canvasWidth,
        canvasHeight: params.canvasHeight,
      rect,
      order: nextOrder,
      ui: true,
    };

    const image = PlayUnitRuntime._findEnabledComponent(objectData, 'Image');
    if (image) {
      const pixelDocumentId = typeof image.data?.pixelDocumentId === 'string' ? image.data.pixelDocumentId.trim() : '';
      if (pixelDocumentId) {
        uiEntries.push({
          ...baseEntry,
          kind: 'image',
          pixelDocumentId,
          alpha: PlayUnitRuntime._normalizeAlpha(image.data?.alpha),
          width: rect.width,
          height: rect.height,
          keepAspect: image.data?.keepAspect !== false,
          originX: 0,
          originY: 0,
          x: rect.left,
          y: rect.top,
        });
        nextOrder += 1;
      }
    }

    const text = PlayUnitRuntime._findEnabledComponent(objectData, 'Text');
    if (text) {
      const textValue = typeof text.data?.text === 'string' ? text.data.text : '';
      if (textValue) {
        uiEntries.push({
          ...baseEntry,
          kind: 'text',
          text: PlayUnitRuntime._resolveTemplateText(textValue, params.appData),
          font: typeof text.data?.font === 'string' && text.data.font.trim() ? text.data.font.trim() : '24px sans-serif',
          color: typeof text.data?.color === 'string' && text.data.color.trim() ? text.data.color.trim() : '#ffffff',
          alpha: PlayUnitRuntime._normalizeAlpha(text.data?.alpha),
          align: PlayUnitRuntime._normalizeTextAlign(text.data?.align),
          baseline: PlayUnitRuntime._normalizeTextBaseline(text.data?.baseline),
          wrap: text.data?.wrap === true,
          maxWidth: rect.width,
          lineHeight: PlayUnitRuntime._normalizePositiveNumber(text.data?.lineHeight, 28),
          strokeColor: typeof text.data?.strokeColor === 'string' && text.data.strokeColor.trim() ? text.data.strokeColor.trim() : '',
          strokeWidth: PlayUnitRuntime._normalizePositiveNumber(text.data?.strokeWidth, 0),
          backgroundColor: typeof text.data?.backgroundColor === 'string' && text.data.backgroundColor.trim() ? text.data.backgroundColor.trim() : '',
          padding: PlayUnitRuntime._normalizePositiveNumber(text.data?.padding, 0),
          x: rect.left,
          y: rect.top,
        });
        nextOrder += 1;
      }
    }

    const rectangle = PlayUnitRuntime._findEnabledComponent(objectData, 'Rectangle');
    if (rectangle) {
      uiEntries.push({
        ...baseEntry,
        kind: 'rectangle',
        shape: typeof rectangle.data?.shape === 'string' ? rectangle.data.shape.trim() : 'rectangle',
        width: rect.width,
        height: rect.height,
        fillColor: typeof rectangle.data?.fillColor === 'string' && rectangle.data.fillColor.trim() ? rectangle.data.fillColor.trim() : '#ffffff',
        fillAlpha: PlayUnitRuntime._normalizeAlpha(rectangle.data?.fillAlpha),
        strokeColor: typeof rectangle.data?.strokeColor === 'string' && rectangle.data.strokeColor.trim() ? rectangle.data.strokeColor.trim() : '#000000',
        strokeWidth: PlayUnitRuntime._normalizePositiveNumber(rectangle.data?.strokeWidth, 2),
        strokeAlpha: PlayUnitRuntime._normalizeAlpha(rectangle.data?.strokeAlpha),
        rotation: Number.isFinite(Number(rectangle.data?.rotation)) ? Number(rectangle.data.rotation) : 0,
        originX: 0,
        originY: 0,
        sides: PlayUnitRuntime._normalizePositiveNumber(rectangle.data?.sides, 4),
        points: PlayUnitRuntime._normalizePositiveNumber(rectangle.data?.points, 5),
        innerRadius: PlayUnitRuntime._normalizeUnitInterval(rectangle.data?.innerRadius, 0.4),
        x: rect.left,
        y: rect.top,
      });
      nextOrder += 1;
    }

    return nextOrder;
  }

  /**
   * @param {ComponentData|object} uiTransform
   * @param {{left:number,top:number,width:number,height:number,right:number,bottom:number}} parentRect
   * @returns {{left:number,top:number,width:number,height:number,right:number,bottom:number}}
   */
  static _resolveUIRect(uiTransform, parentRect) {
    const width = PlayUnitRuntime._normalizePositiveNumber(uiTransform?.data?.width, 0);
    const height = PlayUnitRuntime._normalizePositiveNumber(uiTransform?.data?.height, 0);
    const anchorX = PlayUnitRuntime._normalizeUnitInterval(uiTransform?.data?.anchorX, 0);
    const anchorY = PlayUnitRuntime._normalizeUnitInterval(uiTransform?.data?.anchorY, 0);
    const pivotX = PlayUnitRuntime._normalizeUnitInterval(uiTransform?.data?.pivotX, 0);
    const pivotY = PlayUnitRuntime._normalizeUnitInterval(uiTransform?.data?.pivotY, 0);
    const x = Number.isFinite(Number(uiTransform?.data?.x)) ? Number(uiTransform.data.x) : 0;
    const y = Number.isFinite(Number(uiTransform?.data?.y)) ? Number(uiTransform.data.y) : 0;
    const anchorPointX = parentRect.left + parentRect.width * anchorX;
    const anchorPointY = parentRect.top + parentRect.height * anchorY;
    const left = anchorPointX + x - width * pivotX;
    const top = anchorPointY + y - height * pivotY;
    return {
      left,
      top,
      width,
      height,
      right: left + width,
      bottom: top + height,
    };
  }

  /**
   * @param {PlayObjectData|object|null} objectData
   * @param {Map<string, PlayObjectData|object>} objectMap
   * @returns {boolean}
   */
  static _hasEnabledAncestorCanvas(objectData, objectMap) {
    let parentId = typeof objectData?.parentId === 'string' ? objectData.parentId : '';
    while (parentId) {
      const parentObject = objectMap.get(parentId) || null;
      if (!parentObject) return false;
      if (parentObject.enabled !== false && PlayUnitRuntime._findEnabledComponent(parentObject, 'UICanvas')) {
        return true;
      }
      parentId = typeof parentObject.parentId === 'string' ? parentObject.parentId : '';
    }
    return false;
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
      viewportWidth: 800,
      viewportHeight: 600,
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
    const followTargetCollider = PlayUnitRuntime._findEnabledComponent(followTargetObject, 'Collider');
    const followTargetRectangle = PlayUnitRuntime._findEnabledComponent(followTargetObject, 'Rectangle');
    const sourceX = Number.isFinite(Number(transformComponent.data?.x)) ? Number(transformComponent.data.x) : 0;
    const sourceY = Number.isFinite(Number(transformComponent.data?.y)) ? Number(transformComponent.data.y) : 0;
    const followLerp = PlayUnitRuntime._normalizeUnitInterval(cameraComponent.data?.followLerp, 1);
    const zoom = PlayUnitRuntime._normalizePositiveNumber(cameraComponent.data?.zoom, 1);
    const viewportWidth = PlayUnitRuntime._normalizePositiveNumber(cameraComponent.data?.viewportWidth, 800);
    const viewportHeight = PlayUnitRuntime._normalizePositiveNumber(cameraComponent.data?.viewportHeight, 600);

    const followTargetBaseX = followTargetTransform && Number.isFinite(Number(followTargetTransform.data?.x))
      ? Number(followTargetTransform.data.x)
      : sourceX;
    const followTargetBaseY = followTargetTransform && Number.isFinite(Number(followTargetTransform.data?.y))
      ? Number(followTargetTransform.data.y)
      : sourceY;
    const followTargetWidth = followTargetCollider
      ? PlayUnitRuntime._normalizePositiveNumber(followTargetCollider.data?.width, 0)
      : PlayUnitRuntime._normalizePositiveNumber(followTargetRectangle?.data?.width, 0);
    const followTargetHeight = followTargetCollider
      ? PlayUnitRuntime._normalizePositiveNumber(followTargetCollider.data?.height, 0)
      : PlayUnitRuntime._normalizePositiveNumber(followTargetRectangle?.data?.height, 0);
    const followTargetCenterX = followTargetBaseX + followTargetWidth * 0.5;
    const followTargetCenterY = followTargetBaseY + followTargetHeight * 0.5;
    const targetX = followTargetTransform
      ? followTargetCenterX - viewportWidth * 0.5 / zoom
      : sourceX;
    const targetY = followTargetTransform
      ? followTargetCenterY - viewportHeight * 0.5 / zoom
      : sourceY;

    const cameraX = followTargetTransform ? targetX : sourceX;
    const cameraY = followTargetTransform ? targetY : sourceY;

    return {
      objectId: typeof cameraObject.id === 'string' ? cameraObject.id : '',
      objectName: typeof cameraObject.name === 'string' && cameraObject.name.trim() ? cameraObject.name.trim() : 'CameraObject',
      followTargetObjectId,
      followTargetObjectName: typeof followTargetObject?.name === 'string' && followTargetObject.name.trim() ? followTargetObject.name.trim() : '',
      followLerp,
      viewportWidth,
      viewportHeight,
      sourceX,
      sourceY,
      targetX,
      targetY,
      x: cameraX,
      y: cameraY,
      zoom,
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

  /**
   * テンプレート変数 ${path} を解析して、グローバル変数値に置換
   * @param {string} text - テンプレート文字列 (例: "score: ${user.persistent.score}")
   * @param {AppData|null} appData - グローバル変数にアクセスするためのAppData
   * @returns {string} - 解析後のテキスト
   */
  static _resolveTemplateText(text, appData) {
    if (!text || !appData) return text;
    
    // ${...} マッチの正規表現
    const templatePattern = /\$\{([^}]+)\}/g;
    
    return text.replace(templatePattern, (match, variablePath) => {
      const trimmedPath = variablePath.trim();
      if (!trimmedPath) return match;
      
      // appData の getRuntimeGlobalVariable メソッドを使用
      const value = appData.getRuntimeGlobalVariable?.(trimmedPath);
      if (value === undefined || value === null) return match;
      
      return String(value);
    });
  }
}