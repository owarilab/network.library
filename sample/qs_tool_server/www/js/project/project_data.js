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
     * @type {{ pixelDocuments: object[], tilesets: object[], maps: object[] }}
     */
    this.assets = {
      pixelDocuments: [],
      tilesets: [],
      maps: [],
    };

    /**
     * プロジェクト共通設定。
     * @type {{ defaultChipWidth: number, defaultChipHeight: number }}
     */
    this.settings = {
      defaultChipWidth: 16,
      defaultChipHeight: 16,
    };
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
}