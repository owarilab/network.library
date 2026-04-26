class ProjectBrowserStorage {
  static DB_NAME = 'qs_tool_server_db';
  static DB_VERSION = 1;
  static STORES = {
    projects: 'projects',
    sessions: 'project_sessions',
    assets: 'assets',
  };

  static isAvailable() {
    return typeof indexedDB !== 'undefined';
  }

  static openDatabase() {
    return new Promise((resolve, reject) => {
      if (!ProjectBrowserStorage.isAvailable()) {
        reject(new Error('IndexedDB が利用できません'));
        return;
      }

      const request = indexedDB.open(ProjectBrowserStorage.DB_NAME, ProjectBrowserStorage.DB_VERSION);
      request.onupgradeneeded = () => {
        const db = request.result;
        if (!db.objectStoreNames.contains(ProjectBrowserStorage.STORES.projects)) {
          db.createObjectStore(ProjectBrowserStorage.STORES.projects, { keyPath: 'id' });
        }
        if (!db.objectStoreNames.contains(ProjectBrowserStorage.STORES.sessions)) {
          db.createObjectStore(ProjectBrowserStorage.STORES.sessions, { keyPath: 'projectId' });
        }
        if (!db.objectStoreNames.contains(ProjectBrowserStorage.STORES.assets)) {
          const store = db.createObjectStore(ProjectBrowserStorage.STORES.assets, { keyPath: 'id' });
          store.createIndex('projectId', 'projectId', { unique: false });
        }
      };
      request.onsuccess = () => resolve(request.result);
      request.onerror = () => reject(request.error || new Error('IndexedDB open 失敗'));
    });
  }

  static async saveProject(project, session, fallbackPalette = null) {
    if (!project) throw new Error('ProjectData がありません');

    const db = await ProjectBrowserStorage.openDatabase();
    const tx = db.transaction([
      ProjectBrowserStorage.STORES.projects,
      ProjectBrowserStorage.STORES.sessions,
      ProjectBrowserStorage.STORES.assets,
    ], 'readwrite');

    const projectsStore = tx.objectStore(ProjectBrowserStorage.STORES.projects);
    const sessionsStore = tx.objectStore(ProjectBrowserStorage.STORES.sessions);
    const assetsStore = tx.objectStore(ProjectBrowserStorage.STORES.assets);
    const assetIndex = assetsStore.index('projectId');

    const assets = ProjectBrowserStorage._collectProjectAssets(project);
    const projectRecord = ProjectBrowserStorage._serializeProjectRecord(project, assets);
    const sessionRecord = ProjectBrowserStorage._serializeSessionRecord(session, project);
    const assetRecords = assets.map(asset => ProjectBrowserStorage._serializeAssetRecord(asset, project.id, fallbackPalette));

    const existingAssetIds = await ProjectBrowserStorage._getAllKeysFromIndex(assetIndex, IDBKeyRange.only(project.id));
    const nextAssetIdSet = new Set(assetRecords.map(record => record.id));

    projectsStore.put(projectRecord);
    sessionsStore.put(sessionRecord);
    for (let i = 0; i < existingAssetIds.length; i++) {
      const assetId = existingAssetIds[i];
      if (!nextAssetIdSet.has(assetId)) assetsStore.delete(assetId);
    }
    for (let i = 0; i < assetRecords.length; i++) {
      assetsStore.put(assetRecords[i]);
    }

    await ProjectBrowserStorage._awaitTransaction(tx);
    db.close();
    return projectRecord.id;
  }

  static async saveProjectAs(project, session, newProjectName, fallbackPalette = null) {
    if (!project) throw new Error('ProjectData がありません');
    const next = ProjectBrowserStorage._createSaveAsCopy(project, session, newProjectName, fallbackPalette);
    await ProjectBrowserStorage.saveProject(next.project, next.session, fallbackPalette);
    next.session.clearDirty();
    return next;
  }

  static async loadProject(projectId) {
    if (typeof projectId !== 'string' || !projectId.trim()) {
      throw new Error('projectId が不正です');
    }

    const db = await ProjectBrowserStorage.openDatabase();
    const tx = db.transaction([
      ProjectBrowserStorage.STORES.projects,
      ProjectBrowserStorage.STORES.sessions,
      ProjectBrowserStorage.STORES.assets,
    ], 'readonly');

    const projectsStore = tx.objectStore(ProjectBrowserStorage.STORES.projects);
    const sessionsStore = tx.objectStore(ProjectBrowserStorage.STORES.sessions);
    const assetsStore = tx.objectStore(ProjectBrowserStorage.STORES.assets);

    const projectRecord = await ProjectBrowserStorage._requestToPromise(projectsStore.get(projectId));
    if (!projectRecord) {
      db.close();
      throw new Error('保存済み project が見つかりません');
    }

    const sessionRecord = await ProjectBrowserStorage._requestToPromise(sessionsStore.get(projectId));
    const assetRecords = await ProjectBrowserStorage._getAllFromIndex(assetsStore.index('projectId'), IDBKeyRange.only(projectId));
    await ProjectBrowserStorage._awaitTransaction(tx);
    db.close();

    const project = ProjectBrowserStorage._deserializeProjectRecord(projectRecord, assetRecords);
    const session = ProjectBrowserStorage._deserializeSessionRecord(sessionRecord, project);
    return { project, session };
  }

  static async listProjects() {
    const db = await ProjectBrowserStorage.openDatabase();
    const tx = db.transaction([ProjectBrowserStorage.STORES.projects], 'readonly');
    const store = tx.objectStore(ProjectBrowserStorage.STORES.projects);
    const rows = await ProjectBrowserStorage._getAll(store);
    await ProjectBrowserStorage._awaitTransaction(tx);
    db.close();

    return rows
      .map(row => ({
        id: row.id,
        name: row.name,
        createdAt: ProjectBrowserStorage._normalizeTimestamp(row.createdAt),
        updatedAt: ProjectBrowserStorage._normalizeTimestamp(row.updatedAt),
        assetCounts: {
          pixelDocuments: row.assetCounts?.pixelDocuments | 0,
          tilesets: row.assetCounts?.tilesets | 0,
          maps: row.assetCounts?.maps | 0,
          playUnits: row.assetCounts?.playUnits | 0,
        },
      }))
      .sort((a, b) => b.updatedAt - a.updatedAt);
  }

  static async deleteProject(projectId) {
    if (typeof projectId !== 'string' || !projectId.trim()) return false;

    const db = await ProjectBrowserStorage.openDatabase();
    const tx = db.transaction([
      ProjectBrowserStorage.STORES.projects,
      ProjectBrowserStorage.STORES.sessions,
      ProjectBrowserStorage.STORES.assets,
    ], 'readwrite');
    const projectsStore = tx.objectStore(ProjectBrowserStorage.STORES.projects);
    const sessionsStore = tx.objectStore(ProjectBrowserStorage.STORES.sessions);
    const assetsStore = tx.objectStore(ProjectBrowserStorage.STORES.assets);
    const assetIds = await ProjectBrowserStorage._getAllKeysFromIndex(assetsStore.index('projectId'), IDBKeyRange.only(projectId));

    projectsStore.delete(projectId);
    sessionsStore.delete(projectId);
    for (let i = 0; i < assetIds.length; i++) assetsStore.delete(assetIds[i]);

    await ProjectBrowserStorage._awaitTransaction(tx);
    db.close();
    return true;
  }

  static _collectProjectAssets(project) {
    return [
      ...(project.assets?.pixelDocuments || []),
      ...(project.assets?.tilesets || []),
      ...(project.assets?.maps || []),
      ...(project.assets?.playUnits || []),
    ];
  }

  static _createSaveAsCopy(project, session, newProjectName, fallbackPalette) {
    const cloned = ProjectSerializer.deserialize(
      ProjectSerializer.serialize(project, session, fallbackPalette)
    );
    const nextProject = cloned.project;
    const nextSession = cloned.session;
    const now = Date.now();
    const activeRef = nextSession.activeDocumentRef || { type: null, id: null };

    nextProject.id = ProjectBrowserStorage._createId('proj');
    nextProject.name = typeof newProjectName === 'string' && newProjectName.trim()
      ? newProjectName.trim()
      : `${project.name || 'Project'} Copy`;
    nextProject.createdAt = now;
    nextProject.updatedAt = now;

    ProjectBrowserStorage._reassignAssetIds(nextProject.assets.pixelDocuments || [], 'px', activeRef);
    ProjectBrowserStorage._reassignAssetIds(nextProject.assets.tilesets || [], 'ts', activeRef);
    ProjectBrowserStorage._reassignAssetIds(nextProject.assets.maps || [], 'map', activeRef);
    ProjectBrowserStorage._reassignAssetIds(nextProject.assets.playUnits || [], 'pu', activeRef);

    nextSession.projectId = nextProject.id;
    nextSession.dirty = false;
    nextSession.currentScene = nextSession.currentScene || 'ProjectTopScene';
    return { project: nextProject, session: nextSession };
  }

  static _reassignAssetIds(assets, prefix, activeRef) {
    for (let i = 0; i < assets.length; i++) {
      const oldId = assets[i].id;
      const newId = ProjectBrowserStorage._createId(prefix);
      assets[i].id = newId;
      if (activeRef?.id === oldId) {
        activeRef.id = newId;
      }
    }
  }

  static _serializeProjectRecord(project, assets) {
    const createdAt = ProjectBrowserStorage._normalizeTimestamp(project.createdAt, Date.now());
    const updatedAt = ProjectBrowserStorage._normalizeTimestamp(project.updatedAt, createdAt);
    const assetRefs = assets.map(asset => ({
      id: asset.id,
      type: asset.type,
      name: asset.name,
      updatedAt,
    }));
    return {
      id: project.id,
      name: project.name,
      version: project.version | 0,
      createdAt,
      updatedAt,
      settings: {
        defaultChipWidth: project.settings?.defaultChipWidth | 0,
        defaultChipHeight: project.settings?.defaultChipHeight | 0,
      },
      assetRefs,
      assetCounts: {
        pixelDocuments: (project.assets?.pixelDocuments || []).length,
        tilesets: (project.assets?.tilesets || []).length,
        maps: (project.assets?.maps || []).length,
        playUnits: (project.assets?.playUnits || []).length,
      },
    };
  }

  static _serializeSessionRecord(session, project) {
    const payload = ProjectSerializer._serializeSession(session, project);
    payload.updatedAt = Date.now();
    return payload;
  }

  static _serializeAssetRecord(asset, projectId, fallbackPalette) {
    if (asset.type === 'playUnit') {
      return {
        id: asset.id,
        projectId,
        type: 'playUnit',
        name: asset.name,
        updatedAt: Date.now(),
        payloadVersion: 1,
        storage: { codec: 'json', data: { objects: Array.isArray(asset.objects) ? asset.objects : [] } },
        meta: {},
      };
    }

    if (asset.type === 'map') {
      return {
        id: asset.id,
        projectId,
        type: 'map',
        name: asset.name,
        updatedAt: Date.now(),
        payloadVersion: 1,
        storage: { codec: 'json', data: asset.mapData || null },
        meta: {
          width: asset.width | 0,
          height: asset.height | 0,
        },
      };
    }

    if (asset.type === 'pixelDocument') {
      const palette = ProjectSerializer._resolvePalette(asset.palette, fallbackPalette);
      const layerData = asset.layerData || ProjectSerializer._createEmptyLayerData(asset.width | 0, asset.height | 0);
      const tilesetData = PixelDataConverter.wrapLayerDataAsSingleChipTileset(layerData);
      return {
        id: asset.id,
        projectId,
        type: 'pixelDocument',
        name: asset.name,
        updatedAt: Date.now(),
        payloadVersion: 1,
        storage: {
          codec: 'qts-arraybuffer',
          data: PixelDataConverter.createQtsArrayBuffer(tilesetData, palette),
        },
        meta: {
          width: layerData.width,
          height: layerData.height,
        },
      };
    }

    const palette = ProjectSerializer._resolvePalette(asset.palette, fallbackPalette);
    const tilesetData = asset.tilesetData || new TilesetData(asset.chipWidth | 0 || 16, asset.chipHeight | 0 || 16, asset.columns | 0 || 1, asset.rows | 0 || 1);
    return {
      id: asset.id,
      projectId,
      type: 'tileset',
      name: asset.name,
      updatedAt: Date.now(),
      payloadVersion: 1,
      storage: {
        codec: 'qts-arraybuffer',
        data: PixelDataConverter.createQtsArrayBuffer(tilesetData, palette),
      },
      meta: {
        chipWidth: tilesetData.chipWidth,
        chipHeight: tilesetData.chipHeight,
        columns: tilesetData.columns,
        rows: tilesetData.rows,
      },
    };
  }

  static _deserializeProjectRecord(projectRecord, assetRecords) {
    const project = new ProjectData();
    project.id = projectRecord.id;
    project.version = projectRecord.version | 0 || 1;
    project.name = projectRecord.name || 'New Project';
    project.createdAt = ProjectBrowserStorage._normalizeTimestamp(projectRecord.createdAt, Date.now());
    project.updatedAt = ProjectBrowserStorage._normalizeTimestamp(projectRecord.updatedAt, project.createdAt);
    project.settings = {
      defaultChipWidth: projectRecord.settings?.defaultChipWidth | 0 || 16,
      defaultChipHeight: projectRecord.settings?.defaultChipHeight | 0 || 16,
    };

    project.assets.pixelDocuments = [];
    project.assets.tilesets = [];
    project.assets.maps = [];
    project.assets.playUnits = [];
    for (let i = 0; i < assetRecords.length; i++) {
      const asset = ProjectBrowserStorage._deserializeAssetRecord(assetRecords[i]);
      if (!asset) continue;
      if (asset.type === 'pixelDocument') project.assets.pixelDocuments.push(asset);
      else if (asset.type === 'tileset') project.assets.tilesets.push(asset);
      else if (asset.type === 'map') project.assets.maps.push(asset);
      else if (asset.type === 'playUnit') project.assets.playUnits.push(asset);
    }
    return project;
  }

  static _deserializeSessionRecord(sessionRecord, project) {
    return ProjectSerializer._deserializeSession(sessionRecord || null, project);
  }

  static _deserializeAssetRecord(record) {
    if (!record) return null;
    if (record.type === 'playUnit') {
      const playUnit = PlayUnitData.from({
        id: record.id,
        name: record.name || 'play_unit',
        objects: Array.isArray(record.storage?.data?.objects) ? record.storage.data.objects : [],
      });
      playUnit.type = 'playUnit';
      return playUnit;
    }

    if (record.type === 'map') {
      return {
        id: record.id,
        type: 'map',
        name: record.name || 'map',
        width: record.meta?.width | 0 || 32,
        height: record.meta?.height | 0 || 32,
        mapData: record.storage?.data || null,
      };
    }

    const parsed = PixelDataConverter.parseQtsArrayBuffer(record.storage?.data);
    if (record.type === 'pixelDocument') {
      const layerData = PixelDataConverter.unwrapSingleChipTilesetToLayerData(parsed.tilesetData);
      return {
        id: record.id,
        type: 'pixelDocument',
        name: record.name || 'untitled',
        width: layerData.width,
        height: layerData.height,
        layerData,
        palette: parsed.palette.clone(),
      };
    }

    return {
      id: record.id,
      type: 'tileset',
      name: record.name || 'tileset',
      chipWidth: parsed.tilesetData.chipWidth,
      chipHeight: parsed.tilesetData.chipHeight,
      columns: parsed.tilesetData.columns,
      rows: parsed.tilesetData.rows,
      tilesetData: parsed.tilesetData,
      palette: parsed.palette.clone(),
    };
  }

  static _requestToPromise(request) {
    return new Promise((resolve, reject) => {
      request.onsuccess = () => resolve(request.result);
      request.onerror = () => reject(request.error || new Error('IndexedDB request failed'));
    });
  }

  static _awaitTransaction(tx) {
    return new Promise((resolve, reject) => {
      tx.oncomplete = () => resolve(true);
      tx.onabort = () => reject(tx.error || new Error('IndexedDB transaction aborted'));
      tx.onerror = () => reject(tx.error || new Error('IndexedDB transaction failed'));
    });
  }

  static _getAll(store) {
    if (typeof store.getAll === 'function') {
      return ProjectBrowserStorage._requestToPromise(store.getAll());
    }
    return new Promise((resolve, reject) => {
      const rows = [];
      const request = store.openCursor();
      request.onsuccess = () => {
        const cursor = request.result;
        if (!cursor) {
          resolve(rows);
          return;
        }
        rows.push(cursor.value);
        cursor.continue();
      };
      request.onerror = () => reject(request.error || new Error('IndexedDB cursor failed'));
    });
  }

  static _getAllFromIndex(index, query) {
    if (typeof index.getAll === 'function') {
      return ProjectBrowserStorage._requestToPromise(index.getAll(query));
    }
    return new Promise((resolve, reject) => {
      const rows = [];
      const request = index.openCursor(query);
      request.onsuccess = () => {
        const cursor = request.result;
        if (!cursor) {
          resolve(rows);
          return;
        }
        rows.push(cursor.value);
        cursor.continue();
      };
      request.onerror = () => reject(request.error || new Error('IndexedDB index cursor failed'));
    });
  }

  static _getAllKeysFromIndex(index, query) {
    if (typeof index.getAllKeys === 'function') {
      return ProjectBrowserStorage._requestToPromise(index.getAllKeys(query));
    }
    return new Promise((resolve, reject) => {
      const keys = [];
      const request = index.openKeyCursor(query);
      request.onsuccess = () => {
        const cursor = request.result;
        if (!cursor) {
          resolve(keys);
          return;
        }
        keys.push(cursor.primaryKey);
        cursor.continue();
      };
      request.onerror = () => reject(request.error || new Error('IndexedDB key cursor failed'));
    });
  }

  static _normalizeTimestamp(value, fallback = 0) {
    return Number.isFinite(value) && value > 0 ? Number(value) : fallback;
  }

  static _createId(prefix) {
    return `${prefix}_${Date.now().toString(36)}_${Math.random().toString(36).slice(2, 8)}`;
  }
}