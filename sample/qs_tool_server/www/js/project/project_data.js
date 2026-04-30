/**
 * ProjectData
 * プロジェクト全体の保存対象データを保持する最小ルート。
 * UIの一時状態は持たず、各シーンが扱うアセット実体を束ねる。
 */
class ProjectData {
  constructor() {
    /** @type {string} */
    this.id = '';
    /** @type {number} */
    this.version = 1;
    /** @type {string} */
    this.name = 'New Project';
    /** @type {number} */
    this.createdAt = 0;
    /** @type {number} */
    this.updatedAt = 0;

    /**
     * 保存対象アセット群。
     * @type {{ pixelDocuments: object[], tilesets: object[], maps: object[], playUnits: object[] }}
     */
    this.assets = {
      pixelDocuments: [],
      tilesets: [],
      maps: [],
      playUnits: [],
    };

    /**
     * プロジェクト共通設定。
     * @type {{ defaultChipWidth: number, defaultChipHeight: number }}
     */
    this.settings = {
      defaultChipWidth: 16,
      defaultChipHeight: 16,
    };

    /**
     * Project 全体で共有するグローバル変数定義。
     * @type {{ version: number, system: { fixed: object, persistent: object }, user: { fixed: object, persistent: object } }}
     */
    this.globalVariables = ProjectData.createDefaultGlobalVariables();
  }

  /**
   * 空の globalVariables 定義を生成する。
   * @returns {{ version: number, system: { fixed: object, persistent: object }, user: { fixed: object, persistent: object } }}
   */
  static createDefaultGlobalVariables() {
    return {
      version: 1,
      system: {
        fixed: {},
        persistent: {},
      },
      user: {
        fixed: {},
        persistent: {},
      },
    };
  }

  /**
   * globalVariables 定義を正規化する。
   * 未定義や壊れた形でも最小4区画へ補完する。
   * @param {object|null|undefined} source
   * @returns {{ version: number, system: { fixed: object, persistent: object }, user: { fixed: object, persistent: object } }}
   */
  static normalizeGlobalVariables(source) {
    const fallback = ProjectData.createDefaultGlobalVariables();
    const root = source && typeof source === 'object' && !Array.isArray(source) ? source : {};
    const system = root.system && typeof root.system === 'object' && !Array.isArray(root.system) ? root.system : {};
    const user = root.user && typeof root.user === 'object' && !Array.isArray(root.user) ? root.user : {};
    return {
      version: Number.isInteger(root.version) ? root.version : fallback.version,
      system: {
        fixed: ProjectData._normalizeVariableBucket(system.fixed),
        persistent: ProjectData._normalizeVariableBucket(system.persistent),
      },
      user: {
        fixed: ProjectData._normalizeVariableBucket(user.fixed),
        persistent: ProjectData._normalizeVariableBucket(user.persistent),
      },
    };
  }

  /**
   * グローバル変数定義の値型を正規化する。
   * @param {any} value
   * @param {string} type
   * @returns {any}
   */
  static normalizeGlobalVariableInitialValue(value, type) {
    switch (type) {
      case 'string':
        return ProjectData._coerceStringLikeValue(value, '');
      case 'number':
        return ProjectData._coerceFiniteNumber(value, 0);
      case 'boolean':
        return ProjectData._coerceBooleanLikeValue(value, false);
      case 'json':
        return ProjectData._coerceJsonLikeValueOrFallback(value, { value });
      default:
        return value;
    }
  }

  /**
   * runtime 書き込み用に値を型へ正規化する。
   * @param {any} value
   * @param {string} type
   * @returns {{ ok: boolean, value?: any, message?: string }}
   */
  static coerceRuntimeGlobalVariableValue(value, type) {
    switch (type) {
      case 'string': {
        if (typeof value === 'string') return { ok: true, value };
        if (typeof value === 'number' || typeof value === 'boolean') {
          return { ok: true, value: String(value) };
        }
        return { ok: false, message: 'value must be a string' };
      }
      case 'number': {
        if (typeof value === 'number' && Number.isFinite(value)) return { ok: true, value };
        if (typeof value === 'string') {
          const trimmed = value.trim();
          if (!trimmed) return { ok: false, message: 'value must be a finite number' };
          const parsed = Number(trimmed);
          if (Number.isFinite(parsed)) return { ok: true, value: parsed };
        }
        return { ok: false, message: 'value must be a finite number' };
      }
      case 'boolean': {
        if (typeof value === 'boolean') return { ok: true, value };
        if (typeof value === 'number') {
          if (value === 1) return { ok: true, value: true };
          if (value === 0) return { ok: true, value: false };
        }
        if (typeof value === 'string') {
          const normalized = value.trim().toLowerCase();
          if (normalized === 'true' || normalized === '1') return { ok: true, value: true };
          if (normalized === 'false' || normalized === '0') return { ok: true, value: false };
        }
        return { ok: false, message: 'value must be a boolean' };
      }
      case 'json': {
        const jsonValue = ProjectData._coerceJsonLikeValue(value);
        if (!jsonValue.ok) {
          return { ok: false, message: jsonValue.message || 'value must be JSON serializable' };
        }
        return { ok: true, value: jsonValue.value };
      }
      default:
        return { ok: false, message: 'unsupported type' };
    }
  }

  /**
   * グローバル変数定義を検証する。
   * @param {object|null|undefined} source
   * @returns {{ ok: boolean, message: string, scopePath?: string, variableName?: string }}
   */
  static validateGlobalVariables(source) {
    const root = ProjectData.normalizeGlobalVariables(source);
    const scopes = [
      { scope: 'system', tier: 'fixed' },
      { scope: 'system', tier: 'persistent' },
      { scope: 'user', tier: 'fixed' },
      { scope: 'user', tier: 'persistent' },
    ];
    for (const item of scopes) {
      const bucket = root?.[item.scope]?.[item.tier];
      const scopePath = `${item.scope}.${item.tier}`;
      if (!bucket || typeof bucket !== 'object' || Array.isArray(bucket)) {
        return { ok: false, message: `Invalid bucket: ${scopePath}`, scopePath };
      }
      for (const [name, definition] of Object.entries(bucket)) {
        if (typeof name !== 'string' || !name.trim()) {
          return { ok: false, message: `Variable name is required in ${scopePath}`, scopePath, variableName: name || '' };
        }
        if (!definition || typeof definition !== 'object' || Array.isArray(definition)) {
          return { ok: false, message: `Invalid definition for ${name}`, scopePath, variableName: name };
        }
        const type = typeof definition.type === 'string' ? definition.type.trim() : '';
        if (!['string', 'number', 'boolean', 'json'].includes(type)) {
          return { ok: false, message: `Unsupported type for ${name}`, scopePath, variableName: name };
        }
        const normalized = ProjectData.normalizeGlobalVariableInitialValue(definition.initialValue, type);
        if (type === 'json') {
          try {
            JSON.stringify(normalized);
          } catch (_err) {
            return { ok: false, message: `${name}: initialValue must be JSON serializable`, scopePath, variableName: name };
          }
        }
      }
    }
    return { ok: true, message: '' };
  }

  /**
   * 空のプロジェクトを生成する。
   * @param {string} [name='New Project']
   * @returns {ProjectData}
   */
  static createDefault(name = 'New Project') {
    const project = new ProjectData();
    const now = Date.now();
    project.id = `proj_${now.toString(36)}`;
    project.name = typeof name === 'string' && name.trim() ? name.trim() : 'New Project';
    project.createdAt = now;
    project.updatedAt = now;
    return project;
  }

  /**
   * 更新日時を現在時刻へ進める。
   */
  touch() {
    this.updatedAt = Date.now();
  }

  /**
   * プロジェクト名を変更する。
   * @param {string} name
   * @returns {boolean}
   */
  rename(name) {
    if (typeof name !== 'string') return false;
    const nextName = name.trim();
    if (!nextName) return false;
    this.name = nextName;
    this.touch();
    return true;
  }

  /**
   * ドット絵ドキュメントを追加する。
   * @param {{ id?: string, name?: string, width?: number, height?: number, layerData?: LayerData|null }} [doc]
   * @returns {object}
   */
  addPixelDocument(doc = {}) {
    const asset = this._normalizePixelDocument(doc);
    this.assets.pixelDocuments.push(asset);
    this.touch();
    return asset;
  }

  /**
   * タイルセットを追加する。
   * @param {{ id?: string, name?: string, chipWidth?: number, chipHeight?: number, columns?: number, rows?: number, tilesetData?: TilesetData|null }} [tileset]
   * @returns {object}
   */
  addTileset(tileset = {}) {
    const asset = this._normalizeTileset(tileset);
    this.assets.tilesets.push(asset);
    this.touch();
    return asset;
  }

  /**
   * マップを追加する。
   * @param {{ id?: string, name?: string, width?: number, height?: number, mapData?: object|null }} [map]
   * @returns {object}
   */
  addMap(map = {}) {
    const asset = this._normalizeMap(map);
    this.assets.maps.push(asset);
    this.touch();
    return asset;
  }

  /**
   * PlayUnit を追加する。
   * @param {{ id?: string, name?: string, objects?: PlayObjectData[]|object[] }} [playUnit]
   * @returns {object}
   */
  addPlayUnit(playUnit = {}) {
    const asset = this._normalizePlayUnit(playUnit);
    this.assets.playUnits.push(asset);
    this.touch();
    return asset;
  }

  /**
   * 参照情報からアセットを取得する。
   * @param {{ type?: string, id?: string }|null} ref
   * @returns {object|null}
   */
  getAssetByRef(ref) {
    if (!ref || typeof ref.id !== 'string') return null;
    switch (ref.type) {
      case 'pixelDocument':
        return this.findPixelDocumentById(ref.id);
      case 'tileset':
        return this.findTilesetById(ref.id);
      case 'map':
        return this.findMapById(ref.id);
      case 'playUnit':
        return this.findPlayUnitById(ref.id);
      default:
        return null;
    }
  }

  /**
   * @param {string} id
   * @returns {object|null}
   */
  findPixelDocumentById(id) {
    return this.assets.pixelDocuments.find(doc => doc.id === id) || null;
  }

  /**
   * @param {string} id
   * @returns {object|null}
   */
  findTilesetById(id) {
    return this.assets.tilesets.find(tileset => tileset.id === id) || null;
  }

  /**
   * @param {string} id
   * @returns {object|null}
   */
  findMapById(id) {
    return this.assets.maps.find(map => map.id === id) || null;
  }

  /**
   * @param {string} id
   * @returns {object|null}
   */
  findPlayUnitById(id) {
    return this.assets.playUnits.find(playUnit => playUnit.id === id) || null;
  }

  /**
   * @param {{ id?: string, name?: string, width?: number, height?: number, layerData?: LayerData|null }} doc
   * @returns {object}
   */
  _normalizePixelDocument(doc) {
    return {
      id: this._normalizeId(doc.id, 'px'),
      type: 'pixelDocument',
      name: this._normalizeName(doc.name, 'untitled'),
      width: this._normalizeSize(doc.width, 32),
      height: this._normalizeSize(doc.height, 32),
      layerData: doc.layerData || null,
    };
  }

  /**
   * @param {{ id?: string, name?: string, chipWidth?: number, chipHeight?: number, columns?: number, rows?: number, tilesetData?: TilesetData|null }} tileset
   * @returns {object}
   */
  _normalizeTileset(tileset) {
    return {
      id: this._normalizeId(tileset.id, 'ts'),
      type: 'tileset',
      name: this._normalizeName(tileset.name, 'tileset'),
      chipWidth: this._normalizeSize(tileset.chipWidth, this.settings.defaultChipWidth),
      chipHeight: this._normalizeSize(tileset.chipHeight, this.settings.defaultChipHeight),
      columns: this._normalizeSize(tileset.columns, 1),
      rows: this._normalizeSize(tileset.rows, 1),
      tilesetData: tileset.tilesetData || null,
    };
  }

  /**
   * @param {{ id?: string, name?: string, width?: number, height?: number, mapData?: object|null }} map
   * @returns {object}
   */
  _normalizeMap(map) {
    return {
      id: this._normalizeId(map.id, 'map'),
      type: 'map',
      name: this._normalizeName(map.name, 'map'),
      width: this._normalizeSize(map.width, 32),
      height: this._normalizeSize(map.height, 32),
      mapData: map.mapData || null,
    };
  }

  /**
   * @param {{ id?: string, name?: string, objects?: PlayObjectData[]|object[] }} playUnit
   * @returns {object}
   */
  _normalizePlayUnit(playUnit) {
    const normalized = PlayUnitData.from(playUnit);
    normalized.id = this._normalizeId(normalized.id, 'pu');
    normalized.type = 'playUnit';
    normalized.name = this._normalizeName(normalized.name, 'play_unit');
    return normalized;
  }

  /**
   * @param {string|undefined} id
   * @param {string} prefix
   * @returns {string}
   */
  _normalizeId(id, prefix) {
    if (typeof id === 'string' && id.trim()) return id.trim();
    return `${prefix}_${Date.now().toString(36)}_${Math.random().toString(36).slice(2, 8)}`;
  }

  /**
   * @param {string|undefined} name
   * @param {string} fallback
   * @returns {string}
   */
  _normalizeName(name, fallback) {
    return typeof name === 'string' && name.trim() ? name.trim() : fallback;
  }

  /**
   * @param {number|undefined} value
   * @param {number} fallback
   * @returns {number}
   */
  _normalizeSize(value, fallback) {
    const size = value | 0;
    return size > 0 ? size : fallback;
  }

  /**
   * @param {object|null|undefined} bucket
   * @returns {object}
   */
  static _normalizeVariableBucket(bucket) {
    if (!bucket || typeof bucket !== 'object' || Array.isArray(bucket)) return {};
    const normalized = {};
    for (const [name, definition] of Object.entries(bucket)) {
      if (typeof name !== 'string' || !name.trim()) continue;
      if (!definition || typeof definition !== 'object' || Array.isArray(definition)) continue;
      const type = typeof definition.type === 'string' ? definition.type.trim() : '';
      if (!['string', 'number', 'boolean', 'json'].includes(type)) continue;
      normalized[name] = {
        type,
        initialValue: ProjectData.normalizeGlobalVariableInitialValue(definition.initialValue, type),
        description: typeof definition.description === 'string' ? definition.description : '',
      };
    }
    return normalized;
  }

  /**
   * @param {any} value
   * @param {string} fallback
   * @returns {string}
   */
  static _coerceStringLikeValue(value, fallback) {
    if (typeof value === 'string') return value;
    if (typeof value === 'number' || typeof value === 'boolean') return String(value);
    return fallback;
  }

  /**
   * @param {any} value
   * @param {number} fallback
   * @returns {number}
   */
  static _coerceFiniteNumber(value, fallback) {
    const parsed = typeof value === 'number' ? value : Number(value);
    return Number.isFinite(parsed) ? parsed : fallback;
  }

  /**
   * @param {any} value
   * @param {boolean} fallback
   * @returns {boolean}
   */
  static _coerceBooleanLikeValue(value, fallback) {
    if (typeof value === 'boolean') return value;
    if (typeof value === 'number') {
      if (value === 1) return true;
      if (value === 0) return false;
      return fallback;
    }
    if (typeof value === 'string') {
      const normalized = value.trim().toLowerCase();
      if (normalized === 'true' || normalized === '1') return true;
      if (normalized === 'false' || normalized === '0') return false;
    }
    return fallback;
  }

  /**
   * @param {any} value
   * @param {any} fallback
   * @returns {any}
   */
  static _coerceJsonLikeValueOrFallback(value, fallback) {
    const result = ProjectData._coerceJsonLikeValue(value);
    return result.ok ? result.value : ProjectData._cloneJsonLikeValue(fallback);
  }

  /**
   * @param {any} value
   * @returns {{ ok: boolean, value?: any, message?: string }}
   */
  static _coerceJsonLikeValue(value) {
    const seen = new WeakSet();
    return ProjectData._cloneJsonCompatibleValue(value, seen, true);
  }

  /**
   * @param {any} value
   * @param {WeakSet<object>} seen
   * @param {boolean} allowPrimitiveRoot
   * @returns {{ ok: boolean, value?: any, message?: string }}
   */
  static _cloneJsonCompatibleValue(value, seen, allowPrimitiveRoot) {
    if (value === null) return { ok: true, value: null };
    if (typeof value === 'string' || typeof value === 'boolean') return { ok: true, value };
    if (typeof value === 'number') {
      return Number.isFinite(value)
        ? { ok: true, value }
        : { ok: false, message: 'value must be JSON serializable' };
    }
    if (typeof value === 'undefined' || typeof value === 'function' || typeof value === 'symbol' || typeof value === 'bigint') {
      return { ok: false, message: 'value must be JSON serializable' };
    }
    if (!value || typeof value !== 'object') {
      return allowPrimitiveRoot
        ? { ok: true, value }
        : { ok: false, message: 'value must be JSON serializable' };
    }
    if (seen.has(value)) return { ok: false, message: 'value must be JSON serializable' };
    seen.add(value);

    if (Array.isArray(value)) {
      const cloned = [];
      for (const item of value) {
        const child = ProjectData._cloneJsonCompatibleValue(item, seen, true);
        if (!child.ok) return child;
        cloned.push(child.value);
      }
      seen.delete(value);
      return { ok: true, value: cloned };
    }

    const prototype = Object.getPrototypeOf(value);
    const isPlainObject = Object.prototype.toString.call(value) === '[object Object]';
    if (!isPlainObject && prototype !== null) {
      seen.delete(value);
      return { ok: false, message: 'value must be JSON serializable' };
    }

    const cloned = {};
    for (const [key, childValue] of Object.entries(value)) {
      const child = ProjectData._cloneJsonCompatibleValue(childValue, seen, true);
      if (!child.ok) return child;
      cloned[key] = child.value;
    }
    seen.delete(value);
    return { ok: true, value: cloned };
  }

  /**
   * @param {any} value
   * @returns {any}
   */
  static _cloneJsonLikeValue(value) {
    if (Array.isArray(value)) return value.map(item => ProjectData._cloneJsonLikeValue(item));
    if (value && typeof value === 'object') {
      const cloned = {};
      for (const [key, child] of Object.entries(value)) {
        cloned[key] = ProjectData._cloneJsonLikeValue(child);
      }
      return cloned;
    }
    return value;
  }
}
