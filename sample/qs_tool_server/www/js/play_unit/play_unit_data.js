/**
 * PlayUnitData
 * ゲーム側の編集対象単位を保持する最小データクラス。
 */
class PlayUnitData {
  constructor() {
    /** @type {string} */
    this.id = '';
    /** @type {string} */
    this.name = 'PlayUnit';
    /** @type {PlayObjectData[]} */
    this.objects = [];
  }

  /**
   * 空の PlayUnitData を生成する。
   * @param {string} [name='PlayUnit']
   * @returns {PlayUnitData}
   */
  static createDefault(name = 'PlayUnit') {
    const playUnit = new PlayUnitData();
    playUnit.id = PlayUnitData.createId('pu');
    playUnit.name = PlayUnitData.normalizeName(name, 'PlayUnit');
    const rootId = PlayUnitData.createId('obj');
    const cameraObjectId = PlayUnitData.createId('obj');
    const playSettingsObjectId = PlayUnitData.createId('obj');

    playUnit.objects = [
      PlayObjectData.from({
        id: rootId,
        name: 'Root',
        parentId: null,
        children: [cameraObjectId, playSettingsObjectId],
        components: [],
      }),
      PlayObjectData.from({
        id: cameraObjectId,
        name: 'CameraObject',
        parentId: rootId,
        children: [],
        components: [
          {
            type: 'Transform',
            enabled: true,
            data: {
              x: 0,
              y: 0,
              z: 0,
              rotation: 0,
              scaleX: 1,
              scaleY: 1,
            },
          },
          {
            type: 'Camera',
            enabled: true,
            data: {
              zoom: 1,
              viewportX: 0,
              viewportY: 0,
              viewportWidth: 0,
              viewportHeight: 0,
              followTargetObjectId: '',
              followLerp: 1,
            },
          },
        ],
      }),
      PlayObjectData.from({
        id: playSettingsObjectId,
        name: 'PlaySettingsObject',
        parentId: rootId,
        children: [],
        components: [
          {
            type: 'PlaySettings',
            enabled: true,
            data: {
              defaultCameraObjectId: cameraObjectId,
            },
          },
        ],
      }),
    ];
    return playUnit;
  }

  /**
   * 既存データから PlayUnitData を生成する。
   * @param {Partial<PlayUnitData>|null} [src=null]
   * @returns {PlayUnitData}
   */
  static from(src = null) {
    const playUnit = new PlayUnitData();
    playUnit.id = typeof src?.id === 'string' && src.id.trim()
      ? src.id.trim()
      : PlayUnitData.createId('pu');
    playUnit.name = PlayUnitData.normalizeName(src?.name, 'PlayUnit');
    playUnit.objects = Array.isArray(src?.objects)
      ? src.objects.map((objectData) => PlayObjectData.from(objectData))
      : [];
    return playUnit;
  }

  /**
   * object を追加する。
   * @param {Partial<PlayObjectData>|null} [objectData=null]
   * @returns {PlayObjectData}
   */
  addObject(objectData = null) {
    const playObject = PlayObjectData.from(objectData);
    this.objects.push(playObject);
    return playObject;
  }

  /**
   * 指定 id の object を返す。
   * @param {string} id
   * @returns {PlayObjectData|null}
   */
  findObjectById(id) {
    if (typeof id !== 'string' || !id.trim()) return null;
    return this.objects.find((objectData) => objectData.id === id.trim()) || null;
  }

  /**
   * 指定 parentId を持つ object 一覧を返す。
   * @param {string|null} parentId
   * @returns {PlayObjectData[]}
   */
  getObjectsByParentId(parentId = null) {
    const normalizedParentId = typeof parentId === 'string' && parentId.trim()
      ? parentId.trim()
      : null;
    return this.objects.filter((objectData) => objectData.parentId === normalizedParentId);
  }

  /**
   * 指定 id の object を削除する。
   * @param {string} id
   * @returns {boolean}
   */
  removeObjectById(id) {
    const nextId = typeof id === 'string' ? id.trim() : '';
    if (!nextId) return false;

    const removeIds = new Set([nextId]);
    let expanded = true;
    while (expanded) {
      expanded = false;
      for (const objectData of this.objects) {
        if (removeIds.has(objectData.id)) continue;
        if (objectData.parentId && removeIds.has(objectData.parentId)) {
          removeIds.add(objectData.id);
          expanded = true;
        }
      }
    }

    const prevLength = this.objects.length;
    this.objects = this.objects.filter((objectData) => !removeIds.has(objectData.id));

    for (const objectData of this.objects) {
      objectData.children = objectData.children.filter((childId) => !removeIds.has(childId));
      if (objectData.parentId && removeIds.has(objectData.parentId)) {
        objectData.parentId = null;
      }
    }

    return this.objects.length !== prevLength;
  }

  /**
   * @param {string} prefix
   * @returns {string}
   */
  static createId(prefix) {
    return `${prefix}_${Date.now().toString(36)}_${Math.random().toString(36).slice(2, 8)}`;
  }

  /**
   * @param {string|undefined|null} name
   * @param {string} fallback
   * @returns {string}
   */
  static normalizeName(name, fallback) {
    return typeof name === 'string' && name.trim() ? name.trim() : fallback;
  }
}

/**
 * PlayObjectData
 * PlayUnit 内に置かれる object の最小データクラス。
 */
class PlayObjectData {
  constructor() {
    /** @type {string} */
    this.id = '';
    /** @type {string} */
    this.name = 'Object';
    /** @type {boolean} */
    this.enabled = true;
    /** @type {string|null} */
    this.parentId = null;
    /** @type {string[]} */
    this.children = [];
    /** @type {ComponentData[]} */
    this.components = [];
  }

  /**
   * @param {Partial<PlayObjectData>|null} [src=null]
   * @returns {PlayObjectData}
   */
  static from(src = null) {
    const objectData = new PlayObjectData();
    objectData.id = typeof src?.id === 'string' && src.id.trim()
      ? src.id.trim()
      : PlayUnitData.createId('obj');
    objectData.name = PlayUnitData.normalizeName(src?.name, 'Object');
    objectData.enabled = src?.enabled !== false;
    objectData.parentId = typeof src?.parentId === 'string' && src.parentId.trim()
      ? src.parentId.trim()
      : null;
    objectData.children = Array.isArray(src?.children)
      ? src.children
        .filter((childId) => typeof childId === 'string' && childId.trim())
        .map((childId) => childId.trim())
      : [];
    objectData.components = Array.isArray(src?.components)
      ? src.components.map((component) => ComponentData.from(component))
      : [];
    return objectData;
  }

  /**
   * component を追加する。
   * @param {Partial<ComponentData>|null} [componentData=null]
   * @returns {ComponentData}
   */
  addComponent(componentData = null) {
    const component = ComponentData.from(componentData);
    this.components.push(component);
    return component;
  }

  /**
   * @param {string} type
   * @returns {ComponentData|null}
   */
  findComponentByType(type) {
    if (typeof type !== 'string' || !type.trim()) return null;
    const nextType = type.trim();
    return this.components.find((component) => component.type === nextType) || null;
  }

  /**
   * @param {string} type
   * @returns {ComponentData[]}
   */
  findComponentsByType(type) {
    if (typeof type !== 'string' || !type.trim()) return [];
    const nextType = type.trim();
    return this.components.filter((component) => component.type === nextType);
  }

  /**
   * @param {string} type
   * @returns {number}
   */
  removeComponentsByType(type) {
    if (typeof type !== 'string' || !type.trim()) return 0;
    const nextType = type.trim();
    const prevLength = this.components.length;
    this.components = this.components.filter((component) => component.type !== nextType);
    return prevLength - this.components.length;
  }
}

/**
 * ComponentData
 * PlayObject に付与される component の最小データクラス。
 */
class ComponentData {
  constructor() {
    /** @type {string} */
    this.type = 'Transform';
    /** @type {boolean} */
    this.enabled = true;
    /** @type {object} */
    this.data = {};
  }

  /**
   * @param {Partial<ComponentData>|null} [src=null]
   * @returns {ComponentData}
   */
  static from(src = null) {
    const component = new ComponentData();
    component.type = PlayUnitData.normalizeName(src?.type, 'Transform');
    component.enabled = src?.enabled !== false;
    component.data = src && src.data && typeof src.data === 'object' && !Array.isArray(src.data)
      ? { ...src.data }
      : {};
    return component;
  }
}