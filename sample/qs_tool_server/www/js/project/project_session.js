/**
 * ProjectSession
 * プロジェクトを開いている間だけ必要な一時状態を保持する。
 * 保存対象ではなく、シーン遷移や編集中コンテキストの受け渡しに使う。
 */
class ProjectSession {
  constructor() {
    /** @type {string} */
    this.projectId = '';
    /** @type {boolean} */
    this.dirty = false;
    /** @type {string} */
    this.currentScene = 'title';

    /**
     * 現在編集中のアセット参照。
     * @type {{ type: string|null, id: string|null }}
     */
    this.activeDocumentRef = {
      type: null,
      id: null,
    };

    /**
     * エディタ系シーンで共有したい最小の編集状態。
     * @type {{ activeTool: string, foreColor: number, backColor: number, editMode: 'free'|'tileset', selectedChip: { col: number, row: number } }}
     */
    this.editorState = {
      activeTool: 'pencil',
      foreColor: 0xFF000000,
      backColor: 0xFFFFFFFF,
      editMode: 'free',
      selectedChip: { col: 0, row: 0 },
    };
  }

  /**
   * プロジェクトに紐づく初期セッションを生成する。
   * @param {ProjectData|string} projectOrId
   * @returns {ProjectSession}
   */
  static createForProject(projectOrId) {
    const session = new ProjectSession();
    if (typeof projectOrId === 'string') {
      session.projectId = projectOrId;
    } else if (projectOrId && typeof projectOrId.id === 'string') {
      session.projectId = projectOrId.id;
    }
    return session;
  }

  /**
   * 現在シーンを更新する。
   * @param {string} sceneId
   */
  setCurrentScene(sceneId) {
    if (typeof sceneId !== 'string' || !sceneId.trim()) return;
    this.currentScene = sceneId.trim();
  }

  /**
   * 現在開いているアセット参照を設定する。
  * @param {'pixelDocument'|'tileset'|'map'|'playUnit'} type
   * @param {string} id
   * @returns {boolean}
   */
  setActiveDocument(type, id) {
    if (!this._isValidDocumentType(type)) return false;
    if (typeof id !== 'string' || !id.trim()) return false;
    this.activeDocumentRef.type = type;
    this.activeDocumentRef.id = id.trim();
    return true;
  }

  /**
   * 現在開いているアセット参照をクリアする。
   */
  clearActiveDocument() {
    this.activeDocumentRef.type = null;
    this.activeDocumentRef.id = null;
  }

  /**
   * 編集状態を部分更新する。
   * @param {{ activeTool?: string, foreColor?: number, backColor?: number, editMode?: 'free'|'tileset', selectedChip?: { col?: number, row?: number } }} patch
   */
  applyEditorState(patch = {}) {
    if (typeof patch.activeTool === 'string' && patch.activeTool) {
      this.editorState.activeTool = patch.activeTool;
    }
    if (Number.isInteger(patch.foreColor)) {
      this.editorState.foreColor = patch.foreColor;
    }
    if (Number.isInteger(patch.backColor)) {
      this.editorState.backColor = patch.backColor;
    }
    if (patch.editMode === 'free' || patch.editMode === 'tileset') {
      this.editorState.editMode = patch.editMode;
    }
    if (patch.selectedChip) {
      this.editorState.selectedChip = {
        col: patch.selectedChip.col | 0,
        row: patch.selectedChip.row | 0,
      };
    }
  }

  /**
   * 未保存状態を立てる。
   */
  markDirty() {
    this.dirty = true;
  }

  /**
   * 未保存状態を下ろす。
   */
  clearDirty() {
    this.dirty = false;
  }

  /**
   * @param {string} type
   * @returns {boolean}
   */
  _isValidDocumentType(type) {
    return type === 'pixelDocument' || type === 'tileset' || type === 'map' || type === 'playUnit';
  }
}

class ProjectSerializer {
  static FORMAT = 'qsproj';
  static VERSION = 1;

  static serialize(project, session, fallbackPalette = null) {
    if (!project) throw new Error('ProjectData がありません');
    return JSON.stringify({
      format: ProjectSerializer.FORMAT,
      version: ProjectSerializer.VERSION,
      project: ProjectSerializer._serializeProject(project, fallbackPalette),
      session: ProjectSerializer._serializeSession(session, project),
    }, null, 2);
  }

  static exportProject(project, session, filename = 'project.qsproj', fallbackPalette = null) {
    const blob = new Blob([ProjectSerializer.serialize(project, session, fallbackPalette)], { type: 'application/json' });
    PixelDataConverter._downloadBlob(filename, blob);
  }

  static deserialize(jsonText) {
    const root = JSON.parse(jsonText);
    if (!root || root.format !== ProjectSerializer.FORMAT) {
      throw new Error('qsproj 形式ではありません');
    }
    if ((root.version | 0) !== ProjectSerializer.VERSION) {
      throw new Error(`未対応の qsproj version です: ${root.version}`);
    }
    const project = ProjectSerializer._deserializeProject(root.project || {});
    const session = ProjectSerializer._deserializeSession(root.session || null, project);
    return { project, session };
  }

  static importFromFile(file) {
    return new Promise((resolve, reject) => {
      const reader = new FileReader();
      reader.onload = () => {
        try {
          resolve(ProjectSerializer.deserialize(String(reader.result || '')));
        } catch (err) {
          reject(err);
        }
      };
      reader.onerror = () => reject(new Error('qsproj ファイルの読み込みに失敗しました'));
      reader.readAsText(file);
    });
  }

  static _serializeProject(project, fallbackPalette) {
    return {
      id: project.id,
      version: project.version,
      name: project.name,
      createdAt: project.createdAt,
      updatedAt: project.updatedAt,
      settings: {
        defaultChipWidth: project.settings?.defaultChipWidth | 0,
        defaultChipHeight: project.settings?.defaultChipHeight | 0,
      },
      assets: {
        pixelDocuments: (project.assets?.pixelDocuments || []).map(doc => ProjectSerializer._serializePixelDocument(doc, fallbackPalette)),
        tilesets: (project.assets?.tilesets || []).map(tileset => ProjectSerializer._serializeTileset(tileset, fallbackPalette)),
        maps: (project.assets?.maps || []).map(map => ProjectSerializer._serializeMap(map)),
        playUnits: (project.assets?.playUnits || []).map(playUnit => ProjectSerializer._serializePlayUnit(playUnit)),
      },
    };
  }

  static _serializePixelDocument(doc, fallbackPalette) {
    const width = doc.width | 0;
    const height = doc.height | 0;
    const layerData = doc.layerData || ProjectSerializer._createEmptyLayerData(width, height);
    const palette = ProjectSerializer._resolvePalette(doc.palette, fallbackPalette);
    return {
      id: doc.id,
      type: 'pixelDocument',
      name: doc.name,
      width,
      height,
      storage: {
        type: 'embedded',
        codec: 'qts-base64',
        data: ProjectSerializer._encodeQtsBase64(PixelDataConverter.wrapLayerDataAsSingleChipTileset(layerData), palette),
      },
    };
  }

  static _serializeTileset(tileset, fallbackPalette) {
    const tilesetData = tileset.tilesetData
      || new TilesetData(tileset.chipWidth | 0 || 16, tileset.chipHeight | 0 || 16, tileset.columns | 0 || 1, tileset.rows | 0 || 1);
    const palette = ProjectSerializer._resolvePalette(tileset.palette, fallbackPalette);
    return {
      id: tileset.id,
      type: 'tileset',
      name: tileset.name,
      chipWidth: tilesetData.chipWidth,
      chipHeight: tilesetData.chipHeight,
      columns: tilesetData.columns,
      rows: tilesetData.rows,
      storage: {
        type: 'embedded',
        codec: 'qts-base64',
        data: ProjectSerializer._encodeQtsBase64(tilesetData, palette),
      },
    };
  }

  static _serializeMap(map) {
    return {
      id: map.id,
      type: 'map',
      name: map.name,
      width: map.width | 0,
      height: map.height | 0,
      mapData: map.mapData || null,
    };
  }

  static _serializePlayUnit(playUnit) {
    return {
      id: playUnit.id,
      type: 'playUnit',
      name: playUnit.name,
      objects: Array.isArray(playUnit.objects)
        ? playUnit.objects.map(objectData => ({
          id: objectData.id,
          name: objectData.name,
          enabled: objectData.enabled !== false,
          parentId: typeof objectData.parentId === 'string' && objectData.parentId ? objectData.parentId : null,
          children: Array.isArray(objectData.children) ? [...objectData.children] : [],
          components: Array.isArray(objectData.components)
            ? objectData.components.map(component => ({
              type: component.type,
              enabled: component.enabled !== false,
              data: component.data && typeof component.data === 'object' && !Array.isArray(component.data)
                ? { ...component.data }
                : {},
            }))
            : [],
        }))
        : [],
    };
  }

  static _serializeSession(session, project) {
    const next = session || ProjectSession.createForProject(project);
    return {
      projectId: next.projectId,
      dirty: !!next.dirty,
      currentScene: next.currentScene,
      activeDocumentRef: {
        type: next.activeDocumentRef?.type || null,
        id: next.activeDocumentRef?.id || null,
      },
      editorState: {
        activeTool: next.editorState?.activeTool || 'pencil',
        foreColor: Number.isInteger(next.editorState?.foreColor) ? next.editorState.foreColor : 0xFF000000,
        backColor: Number.isInteger(next.editorState?.backColor) ? next.editorState.backColor : 0xFFFFFFFF,
        editMode: next.editorState?.editMode === 'tileset' ? 'tileset' : 'free',
        selectedChip: {
          col: next.editorState?.selectedChip?.col | 0,
          row: next.editorState?.selectedChip?.row | 0,
        },
      },
    };
  }

  static _deserializeProject(obj) {
    const project = new ProjectData();
    project.id = typeof obj.id === 'string' && obj.id ? obj.id : ProjectData.createDefault().id;
    project.version = Number.isInteger(obj.version) ? obj.version : 1;
    project.name = typeof obj.name === 'string' && obj.name.trim() ? obj.name.trim() : 'New Project';
    project.createdAt = Number.isInteger(obj.createdAt) ? obj.createdAt : Date.now();
    project.updatedAt = Number.isInteger(obj.updatedAt) ? obj.updatedAt : project.createdAt;
    project.settings = {
      defaultChipWidth: obj.settings?.defaultChipWidth | 0 || 16,
      defaultChipHeight: obj.settings?.defaultChipHeight | 0 || 16,
    };
    project.assets.pixelDocuments = (obj.assets?.pixelDocuments || []).map(doc => ProjectSerializer._deserializePixelDocument(doc));
    project.assets.tilesets = (obj.assets?.tilesets || []).map(tileset => ProjectSerializer._deserializeTileset(tileset));
    project.assets.maps = (obj.assets?.maps || []).map(map => ({
      id: typeof map.id === 'string' && map.id ? map.id : `map_${Date.now().toString(36)}`,
      type: 'map',
      name: typeof map.name === 'string' && map.name.trim() ? map.name.trim() : 'map',
      width: map.width | 0 || 32,
      height: map.height | 0 || 32,
      mapData: map.mapData || null,
    }));
    project.assets.playUnits = (obj.assets?.playUnits || []).map(playUnit => {
      const normalized = PlayUnitData.from(playUnit);
      normalized.type = 'playUnit';
      normalized.name = typeof normalized.name === 'string' && normalized.name.trim() ? normalized.name.trim() : 'play_unit';
      return normalized;
    });
    return project;
  }

  static _deserializePixelDocument(doc) {
    const parsed = ProjectSerializer._decodeQtsBase64(doc.storage?.data);
    const layerData = PixelDataConverter.unwrapSingleChipTilesetToLayerData(parsed.tilesetData);
    return {
      id: typeof doc.id === 'string' && doc.id ? doc.id : `px_${Date.now().toString(36)}`,
      type: 'pixelDocument',
      name: typeof doc.name === 'string' && doc.name.trim() ? doc.name.trim() : 'untitled',
      width: layerData.width,
      height: layerData.height,
      layerData,
      palette: parsed.palette.clone(),
    };
  }

  static _deserializeTileset(tileset) {
    const parsed = ProjectSerializer._decodeQtsBase64(tileset.storage?.data);
    return {
      id: typeof tileset.id === 'string' && tileset.id ? tileset.id : `ts_${Date.now().toString(36)}`,
      type: 'tileset',
      name: typeof tileset.name === 'string' && tileset.name.trim() ? tileset.name.trim() : 'tileset',
      chipWidth: parsed.tilesetData.chipWidth,
      chipHeight: parsed.tilesetData.chipHeight,
      columns: parsed.tilesetData.columns,
      rows: parsed.tilesetData.rows,
      tilesetData: parsed.tilesetData,
      palette: parsed.palette.clone(),
    };
  }

  static _deserializeSession(obj, project) {
    const session = ProjectSession.createForProject(project);
    if (!obj) return session;
    session.projectId = typeof obj.projectId === 'string' && obj.projectId ? obj.projectId : project.id;
    session.dirty = !!obj.dirty;
    session.currentScene = typeof obj.currentScene === 'string' && obj.currentScene ? obj.currentScene : 'ProjectTopScene';
    if (obj.activeDocumentRef?.type && obj.activeDocumentRef?.id) {
      session.setActiveDocument(obj.activeDocumentRef.type, obj.activeDocumentRef.id);
    }
    session.applyEditorState(obj.editorState || {});
    return session;
  }

  static _resolvePalette(assetPalette, fallbackPalette) {
    if (assetPalette?.clone && typeof assetPalette.clone === 'function') return assetPalette.clone();
    if (Array.isArray(assetPalette)) {
      const palette = new EditablePalette32();
      for (let i = 1; i < Math.min(assetPalette.length, 32); i++) palette.setColor(i, assetPalette[i]);
      return palette;
    }
    if (fallbackPalette?.clone && typeof fallbackPalette.clone === 'function') return fallbackPalette.clone();
    return new EditablePalette32();
  }

  static _createEmptyLayerData(width, height) {
    const ld = new LayerData();
    ld.init(width > 0 ? width : 32, height > 0 ? height : 32, 0x00000000);
    return ld;
  }

  static _encodeQtsBase64(tilesetData, palette) {
    return PixelDataConverter.arrayBufferToBase64(PixelDataConverter.createQtsArrayBuffer(tilesetData, palette));
  }

  static _decodeQtsBase64(base64) {
    if (typeof base64 !== 'string' || !base64) throw new Error('埋め込み QTS データがありません');
    return PixelDataConverter.parseQtsArrayBuffer(PixelDataConverter.base64ToArrayBuffer(base64));
  }
}