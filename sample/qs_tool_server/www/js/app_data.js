/**
 * AppData
 * シーン間で共有するアプリケーションデータを保持するクラス。
 * レイヤー管理は LayerData クラスに委譲し、
 * pixelData プロパティはアクティブレイヤーの PixelData を返す getter。
 *
 * タイルセットモード時は layerData getter が選択チップの LayerData を返すため、
 * PixelCanvas / LayerPanel / ツール群は無改修で動作する。
 */
class AppData {
  constructor() {
    /**
     * シーン遷移に使う SceneManager 参照。
     * @type {SceneManager|null}
     */
    this.sceneManager = null;

    /**
     * 現在開いているプロジェクト。
     * @type {ProjectData|null}
     */
    this.currentProject = null;

    /**
     * 現在開いているプロジェクトのセッション情報。
     * @type {ProjectSession|null}
     */
    this.projectSession = null;

    /**
     * Runtime 上のグローバル変数 current state。
     * project.globalVariables の定義から起動時に生成される。
     * @type {{ system: { fixed: object, persistent: object }, user: { fixed: object, persistent: object } }|null}
     */
    this.globalVariableState = null;

    /**
     * free モード用の LayerData。
     * layerData getter 経由でアクセスされる。
     * @type {LayerData}
     */
    this._layerData = new LayerData();

    /**
     * Undo / Redo の履歴管理。
     * @type {HistoryManager}
     */
    this.history = new HistoryManager(100);

    /**
     * 前景色 (描画色)。0xAARRGGBB 形式。
     * @type {number}
     */
    this.foreColor = PixelData.rgba(0, 0, 0, 255);       // 黒

    /**
     * 背景色 (消しゴム色 / 塗りつぶし背景)。0xAARRGGBB 形式。
     * @type {number}
     */
    this.backColor = PixelData.rgba(255, 255, 255, 255);  // 白

    /**
     * 現在選択中のツール ID。
    * 'pencil' | 'eraser' | 'fill' | 'eyedropper' | 'selectRect'
     * @type {string}
     */
    this.activeTool = 'pencil';

    // ---- タイルセットモード用プロパティ ----

    /**
     * タイルセットデータ。tileset モード時のみ有効。
     * @type {TilesetData|null}
     */
    this.tilesetData = null;

    /**
     * タイルセットモード時の選択中チップ位置。
     * @type {{ col: number, row: number }}
     */
    this.selectedChip = { col: 0, row: 0 };

    /**
     * 編集モード。'free' = 従来のドット絵エディタ、'tileset' = タイルセットモード。
     * @type {'free'|'tileset'}
     */
    this.editMode = 'free';

    /**
     * チップクリップボード。コピーしたチップの LayerData ディープコピーを保持する。
     * @type {LayerData|null}
     */
    this.chipClipboard = null;

    /**
     * 編集可能カラーパレット（32色）。
     * EditorScene.onEnter() で EditablePalette32 インスタンスが設定される。
     * @type {EditablePalette32|null}
     */
    this.palette = null;

    /**
     * 通過フラグの表示切替。true = チップパレット上に通過フラグを表示する。
     * @type {boolean}
     */
    this.showPassFlags = true;

    /**
     * 現在の選択範囲。
     * @type {{ active: boolean, x: number, y: number, w: number, h: number, mode: string, floating: object|null }}
     */
    this.selection = {
      active: false,
      x: 0,
      y: 0,
      w: 0,
      h: 0,
      mode: 'rect',
      floating: null,
    };

    /**
     * 将来のコピー/貼り付け用選択クリップボード。
     * @type {{ pixelData: PixelData, width: number, height: number }|null}
     */
    this.selectionClipboard = null;

    /**
     * マップエディタで編集中のマップデータ。
     * @type {object|null}
     */
    this.mapData = null;

    /**
     * 直近の runtime global variable 操作エラー。
     * @type {string}
     */
    this._lastRuntimeGlobalVariableError = '';
  }

  /**
   * 現在のプロジェクトとセッションを設定する。
   * セッションが未指定なら project から新規作成する。
   * @param {ProjectData|null} project
   * @param {ProjectSession|null} [session=null]
   * @returns {ProjectSession|null}
   */
  setCurrentProject(project, session = null) {
    this.currentProject = project || null;
    if (!project) {
      this.projectSession = null;
      this.globalVariableState = null;
      this.mapData = null;
      return null;
    }

    const nextSession = session || ProjectSession.createForProject(project);
    this.projectSession = nextSession;
    this.globalVariableState = this._createGlobalVariableRuntimeState(project.globalVariables);
    this.applyProjectSessionEditorState();
    return this.projectSession;
  }

  /**
   * Project 定義側の globalVariables を取得する。
   * @returns {{ version: number, system: { fixed: object, persistent: object }, user: { fixed: object, persistent: object } }}
   */
  getProjectGlobalVariablesDefinition() {
    return ProjectData.normalizeGlobalVariables(this.currentProject?.globalVariables);
  }

  /**
   * Runtime current state の globalVariables を取得する。
   * 未初期化時は空の4区画を返す。
   * @returns {{ system: { fixed: object, persistent: object }, user: { fixed: object, persistent: object } }}
   */
  getRuntimeGlobalVariables() {
    if (this.globalVariableState) {
      return this.globalVariableState;
    }
    const fallback = ProjectData.createDefaultGlobalVariables();
    return {
      system: {
        fixed: { ...fallback.system.fixed },
        persistent: { ...fallback.system.persistent },
      },
      user: {
        fixed: { ...fallback.user.fixed },
        persistent: { ...fallback.user.persistent },
      },
    };
  }

  /**
   * 変数パスの解決結果を返す。
   * @param {string} path
   * @returns {{ scope: 'system'|'user', tier: 'fixed'|'persistent', name: string }|null}
   */
  resolveRuntimeGlobalVariablePath(path) {
    return this._resolveGlobalVariablePath(path);
  }

  /**
   * 直近の runtime global variable 操作エラーを返す。
   * @returns {string}
   */
  getLastRuntimeGlobalVariableError() {
    return this._lastRuntimeGlobalVariableError || '';
  }

  /**
   * 変数が project 定義および runtime state に存在するかを返す。
   * @param {string} path
   * @returns {boolean}
   */
  hasRuntimeGlobalVariable(path) {
    const resolved = this._resolveGlobalVariablePath(path);
    if (!resolved) return false;
    const definition = this._getGlobalVariableBucketDefinition(resolved.scope, resolved.tier);
    if (!definition || !Object.prototype.hasOwnProperty.call(definition, resolved.name)) {
      return false;
    }
    const state = this.getRuntimeGlobalVariables()[resolved.scope]?.[resolved.tier];
    return !!state && Object.prototype.hasOwnProperty.call(state, resolved.name);
  }

  /**
   * 変数 bucket 内の定義済み変数一覧を返す。
   * @param {string} scopePath 例: "system.fixed"
   * @returns {Array<{ name: string, value: any }>}
   */
  listRuntimeGlobalVariables(scopePath) {
    const resolved = this._resolveGlobalVariableBucketPath(scopePath);
    if (!resolved) return [];
    const definition = this._getGlobalVariableBucketDefinition(resolved.scope, resolved.tier);
    const state = this.getRuntimeGlobalVariables()[resolved.scope]?.[resolved.tier];
    if (!definition || !state) return [];
    return Object.keys(definition)
      .filter(name => Object.prototype.hasOwnProperty.call(state, name))
      .map(name => ({
        name,
        value: this._cloneJsonLikeValue(state[name]),
      }));
  }

  /**
   * Project 定義から Runtime current state を再初期化する。
   * @returns {{ system: { fixed: object, persistent: object }, user: { fixed: object, persistent: object } }}
   */
  resetRuntimeGlobalVariables() {
    this.globalVariableState = this._createGlobalVariableRuntimeState(this.currentProject?.globalVariables);
    return this.getRuntimeGlobalVariables();
  }

  /**
   * パス形式のグローバル変数 current value を取得する。
   * 例: system.fixed.startupPlayUnitId
   * @param {string} path
   * @returns {any}
   */
  getRuntimeGlobalVariable(path) {
    const resolved = this._resolveGlobalVariablePath(path);
    if (!resolved) return undefined;
    if (!this.hasRuntimeGlobalVariable(path)) return undefined;
    return this._cloneJsonLikeValue(this.getRuntimeGlobalVariables()[resolved.scope][resolved.tier][resolved.name]);
  }

  /**
   * パス形式のグローバル変数 current value を更新する。
   * @param {string} path
   * @param {any} value
   * @returns {boolean}
   */
  setRuntimeGlobalVariable(path, value) {
    const resolved = this._resolveGlobalVariablePath(path);
    if (!resolved) {
      this._lastRuntimeGlobalVariableError = 'invalid variable path';
      return false;
    }
    if (!this.hasRuntimeGlobalVariable(path)) {
      this._lastRuntimeGlobalVariableError = `Global variable not found: ${path}`;
      return false;
    }
    if (!this.globalVariableState) {
      this.globalVariableState = this._createGlobalVariableRuntimeState(this.currentProject?.globalVariables);
    }
    const definition = this._getGlobalVariableDefinition(resolved.scope, resolved.tier, resolved.name);
    const type = typeof definition?.type === 'string' ? definition.type.trim() : '';
    const normalized = ProjectData.coerceRuntimeGlobalVariableValue(value, type);
    if (!normalized.ok) {
      this._lastRuntimeGlobalVariableError = normalized.message || `Invalid value for ${path}`;
      return false;
    }
    this.globalVariableState[resolved.scope][resolved.tier][resolved.name] = this._cloneJsonLikeValue(normalized.value);
    this._lastRuntimeGlobalVariableError = '';
    return true;
  }

  /**
   * 変数を runtime state から削除する。
   * project 定義に存在する変数のみ削除可能。
   * @param {string} path
   * @returns {boolean}
   */
  deleteRuntimeGlobalVariable(path) {
    const resolved = this._resolveGlobalVariablePath(path);
    if (!resolved) return false;
    if (!this.hasRuntimeGlobalVariable(path)) return false;
    delete this.getRuntimeGlobalVariables()[resolved.scope][resolved.tier][resolved.name];
    return true;
  }

  /**
   * bucket 定義を取得する。
   * @param {'system'|'user'} scope
   * @param {'fixed'|'persistent'} tier
   * @returns {object|null}
   */
  _getGlobalVariableBucketDefinition(scope, tier) {
    const definition = this.getProjectGlobalVariablesDefinition();
    return definition?.[scope]?.[tier] ?? null;
  }

  /**
   * 変数定義を取得する。
   * @param {'system'|'user'} scope
   * @param {'fixed'|'persistent'} tier
   * @param {string} name
   * @returns {object|null}
   */
  _getGlobalVariableDefinition(scope, tier, name) {
    const bucket = this._getGlobalVariableBucketDefinition(scope, tier);
    if (!bucket || !Object.prototype.hasOwnProperty.call(bucket, name)) return null;
    return bucket[name];
  }

  /**
   * Project 定義から Runtime 用 current state を生成する。
   * @param {object|null|undefined} globalVariables
   * @returns {{ system: { fixed: object, persistent: object }, user: { fixed: object, persistent: object } }}
   */
  _createGlobalVariableRuntimeState(globalVariables) {
    const normalized = ProjectData.normalizeGlobalVariables(globalVariables);
    return {
      system: {
        fixed: this._createGlobalVariableBucketState(normalized.system.fixed),
        persistent: this._createGlobalVariableBucketState(normalized.system.persistent),
      },
      user: {
        fixed: this._createGlobalVariableBucketState(normalized.user.fixed),
        persistent: this._createGlobalVariableBucketState(normalized.user.persistent),
      },
    };
  }

  /**
   * @param {object} bucket
   * @returns {object}
   */
  _createGlobalVariableBucketState(bucket) {
    const state = {};
    const entries = bucket && typeof bucket === 'object' && !Array.isArray(bucket)
      ? Object.entries(bucket)
      : [];
    for (const [name, definition] of entries) {
      if (typeof name !== 'string' || !name) continue;
      state[name] = this._extractGlobalVariableInitialValue(definition);
    }
    return state;
  }

  /**
   * @param {any} definition
   * @returns {any}
   */
  _extractGlobalVariableInitialValue(definition) {
    if (!definition || typeof definition !== 'object' || Array.isArray(definition)) {
      return undefined;
    }
    const value = definition.initialValue;
    if (Array.isArray(value)) return value.map(item => this._cloneJsonLikeValue(item));
    return this._cloneJsonLikeValue(value);
  }

  /**
   * @param {any} value
   * @returns {any}
   */
  _cloneJsonLikeValue(value) {
    if (Array.isArray(value)) return value.map(item => this._cloneJsonLikeValue(item));
    if (value && typeof value === 'object') {
      const cloned = {};
      for (const [key, child] of Object.entries(value)) {
        cloned[key] = this._cloneJsonLikeValue(child);
      }
      return cloned;
    }
    return value;
  }

  /**
   * @param {string} path
   * @returns {{ scope: 'system'|'user', tier: 'fixed'|'persistent', name: string }|null}
   */
  _resolveGlobalVariablePath(path) {
    if (typeof path !== 'string' || !path.trim()) return null;
    const parts = path.trim().split('.');
    if (parts.length !== 3) return null;
    const [scope, tier, name] = parts;
    if ((scope !== 'system' && scope !== 'user') || (tier !== 'fixed' && tier !== 'persistent') || !name) {
      return null;
    }
    return { scope, tier, name };
  }

  /**
   * bucket パスを解決する。
   * @param {string} scopePath 例: "system.fixed"
   * @returns {{ scope: 'system'|'user', tier: 'fixed'|'persistent' }|null}
   */
  _resolveGlobalVariableBucketPath(scopePath) {
    if (typeof scopePath !== 'string' || !scopePath.trim()) return null;
    const parts = scopePath.trim().split('.');
    if (parts.length !== 2) return null;
    const [scope, tier] = parts;
    if ((scope !== 'system' && scope !== 'user') || (tier !== 'fixed' && tier !== 'persistent')) {
      return null;
    }
    return { scope, tier };
  }

  /**
   * 現在のセッションへ AppData 上の編集状態を反映する。
   * @returns {boolean}
   */
  syncEditorStateToProjectSession() {
    if (!this.projectSession) return false;
    this.projectSession.applyEditorState({
      activeTool: this.activeTool,
      foreColor: this.foreColor,
      backColor: this.backColor,
      editMode: this.editMode,
      selectedChip: {
        col: this.selectedChip.col,
        row: this.selectedChip.row,
      },
    });
    return true;
  }

  /**
   * 現在のセッションから AppData 上の編集状態を反映する。
   * @returns {boolean}
   */
  applyProjectSessionEditorState() {
    if (!this.projectSession) return false;
    const editorState = this.projectSession.editorState || {};
    if (typeof editorState.activeTool === 'string' && editorState.activeTool) {
      this.activeTool = editorState.activeTool;
    }
    if (Number.isInteger(editorState.foreColor)) {
      this.foreColor = editorState.foreColor;
    }
    if (Number.isInteger(editorState.backColor)) {
      this.backColor = editorState.backColor;
    }
    this.editMode = editorState.editMode === 'tileset' ? 'tileset' : 'free';
    this.selectedChip = {
      col: editorState.selectedChip?.col | 0,
      row: editorState.selectedChip?.row | 0,
    };
    return true;
  }

  /**
   * 現在のセッションが指すアセットを取得する。
   * @returns {object|null}
   */
  getActiveProjectAsset() {
    if (!this.currentProject || !this.projectSession) return null;
    return this.currentProject.getAssetByRef(this.projectSession.activeDocumentRef);
  }

  /**
   * PlayTest 開始時に使う PlayUnit を解決してアクティブ化する。
   * startupPlayUnitId が有効なら優先し、無効なら現在選択中または先頭 PlayUnit を使う。
   * @returns {object|null}
   */
  activateStartupPlayUnit() {
    if (!this.currentProject || !this.projectSession) return null;

    const startupPlayUnitId = this.getRuntimeGlobalVariable('system.fixed.startupPlayUnitId');
    const startupAsset = this._resolvePlayUnitAsset(startupPlayUnitId);
    if (startupAsset) {
      return this.activateRuntimePlayUnitById(startupAsset.id, {
        updateReturnPlayUnitId: false,
        clearRequestedPlayUnitId: true,
      });
    }

    const activeAsset = this.getActiveProjectAsset();
    if (activeAsset?.type === 'playUnit') {
      return this.activateRuntimePlayUnitById(activeAsset.id, {
        updateReturnPlayUnitId: false,
        clearRequestedPlayUnitId: true,
      });
    }

    const firstPlayUnit = this.currentProject.assets?.playUnits?.[0] || null;
    if (!firstPlayUnit) return null;
    return this.activateRuntimePlayUnitById(firstPlayUnit.id, {
      updateReturnPlayUnitId: false,
      clearRequestedPlayUnitId: true,
    });
  }

  /**
   * Runtime から requestedPlayUnitId を消費して PlayUnit 切替を試みる。
   * 無効 ID でも requestedPlayUnitId は自動クリアする。
   * @returns {{ changed: boolean, asset: object|null, invalid: boolean, requestedId: string }}
   */
  consumeRequestedRuntimePlayUnitSwitch() {
    const requestedId = this.getRuntimeGlobalVariable('system.fixed.requestedPlayUnitId');
    if (typeof requestedId !== 'string' || !requestedId.trim()) {
      return { changed: false, asset: null, invalid: false, requestedId: '' };
    }

    const nextAsset = this._resolvePlayUnitAsset(requestedId);
    this.setRuntimeGlobalVariable('system.fixed.requestedPlayUnitId', '');
    if (!nextAsset) {
      return { changed: false, asset: null, invalid: true, requestedId: requestedId.trim() };
    }

    const activeAsset = this.getActiveProjectAsset();
    if (activeAsset?.type === 'playUnit' && activeAsset.id === nextAsset.id) {
      this.setRuntimeGlobalVariable('system.fixed.currentPlayUnitId', nextAsset.id);
      return { changed: false, asset: nextAsset, invalid: false, requestedId: nextAsset.id };
    }

    const changedAsset = this.activateRuntimePlayUnitById(nextAsset.id, {
      updateReturnPlayUnitId: true,
      clearRequestedPlayUnitId: false,
    });
    return {
      changed: !!changedAsset,
      asset: changedAsset,
      invalid: false,
      requestedId: nextAsset.id,
    };
  }

  /**
   * 指定 ID の PlayUnit を Runtime 上で現在アクティブにする。
   * @param {string} playUnitId
   * @param {{ updateReturnPlayUnitId?: boolean, clearRequestedPlayUnitId?: boolean }} [options]
   * @returns {object|null}
   */
  activateRuntimePlayUnitById(playUnitId, options = {}) {
    if (!this.currentProject || !this.projectSession) return null;
    const asset = this._resolvePlayUnitAsset(playUnitId);
    if (!asset) {
      if (options.clearRequestedPlayUnitId !== false) {
        this.setRuntimeGlobalVariable('system.fixed.requestedPlayUnitId', '');
      }
      return null;
    }

    const previousId = this.getRuntimeGlobalVariable('system.fixed.currentPlayUnitId');
    this.projectSession.setActiveDocument('playUnit', asset.id);
    this.setRuntimeGlobalVariable('system.fixed.currentPlayUnitId', asset.id);
    if (options.updateReturnPlayUnitId !== false) {
      this.setRuntimeGlobalVariable(
        'system.fixed.returnPlayUnitId',
        typeof previousId === 'string' && previousId.trim() ? previousId.trim() : '',
      );
    }
    if (options.clearRequestedPlayUnitId !== false) {
      this.setRuntimeGlobalVariable('system.fixed.requestedPlayUnitId', '');
    }
    return asset;
  }

  /**
   * returnPlayUnitId が有効なら、その PlayUnit へ即時に戻す。
   * 成功時は current/return を入れ替える。
   * @returns {object|null}
   */
  activateReturnRuntimePlayUnit() {
    const returnPlayUnitId = this.getRuntimeGlobalVariable('system.fixed.returnPlayUnitId');
    if (typeof returnPlayUnitId !== 'string' || !returnPlayUnitId.trim()) return null;
    return this.activateRuntimePlayUnitById(returnPlayUnitId, {
      updateReturnPlayUnitId: true,
      clearRequestedPlayUnitId: true,
    });
  }

  /**
   * @param {string} playUnitId
   * @returns {object|null}
   */
  _resolvePlayUnitAsset(playUnitId) {
    if (!this.currentProject || typeof playUnitId !== 'string' || !playUnitId.trim()) return null;
    return this.currentProject.findPlayUnitById(playUnitId.trim());
  }

  /**
   * 現在シーン識別子をセッションへ反映する。
   * @param {string} sceneId
   * @returns {boolean}
   */
  setCurrentSceneId(sceneId) {
    if (!this.projectSession) return false;
    this.projectSession.setCurrentScene(sceneId);
    return true;
  }

  /**
   * SceneManager 経由でシーン遷移する。
   * @param {Scene} scene
   * @returns {boolean}
   */
  changeScene(scene) {
    if (!this.sceneManager) return false;
    this.sceneManager.change(scene);
    return true;
  }

  /**
   * 現在の編集対象コンテキストを返す。
   * コマンドが対象モードやチップ位置を保存する時に使う。
   * @returns {{ mode: 'free'|'tileset', chip: { col: number, row: number }|null, layerIndex: number }}
   */
  getActiveEditTargetContext() {
    return {
      mode: this.editMode,
      chip: this.editMode === 'tileset'
        ? { col: this.selectedChip.col, row: this.selectedChip.row }
        : null,
      layerIndex: this.layerData.activeIndex,
    };
  }

  /**
   * 編集状態のスナップショットを返す。
   * @returns {{
   *   layerData: LayerData,
   *   tilesetData: TilesetData|null,
   *   editMode: 'free'|'tileset',
   *   selectedChip: { col: number, row: number },
   *   selection: { active: boolean, x: number, y: number, w: number, h: number, mode: string, floating: object|null },
   * }}
   */
  createEditStateSnapshot() {
    return {
      layerData: this._cloneLayerData(this._layerData),
      tilesetData: this.tilesetData ? this._cloneTilesetData(this.tilesetData) : null,
      editMode: this.editMode,
      selectedChip: { col: this.selectedChip.col, row: this.selectedChip.row },
      selection: this._cloneSelection(this.selection),
    };
  }

  /**
   * 編集状態のスナップショットを適用する。
   * @param {{
   *   layerData: LayerData,
   *   tilesetData: TilesetData|null,
   *   editMode: 'free'|'tileset',
   *   selectedChip: { col: number, row: number },
   *   selection: { active: boolean, x: number, y: number, w: number, h: number, mode: string, floating: object|null },
   * }} snapshot
   * @returns {boolean}
   */
  applyEditStateSnapshot(snapshot) {
    if (!snapshot?.layerData) return false;

    this._layerData = this._cloneLayerData(snapshot.layerData);
    this.tilesetData = snapshot.tilesetData ? this._cloneTilesetData(snapshot.tilesetData) : null;
    this.editMode = snapshot.editMode === 'tileset' ? 'tileset' : 'free';
    this.selectedChip = {
      col: snapshot.selectedChip?.col | 0,
      row: snapshot.selectedChip?.row | 0,
    };
    this.selection = this._cloneSelection(snapshot.selection);
    this.syncEditorStateToProjectSession();
    return true;
  }

  /**
   * @param {PixelData} src
   * @returns {PixelData}
   */
  _clonePixelData(src) {
    const pd = new PixelData();
    pd.createPixelData(src.width, src.height);
    pd.pixels.set(src.pixels);
    return pd;
  }

  /**
   * @param {LayerData} src
   * @returns {LayerData}
   */
  _cloneLayerData(src) {
    const ld = new LayerData();
    ld.width = src.width;
    ld.height = src.height;
    ld.layers = src.layers.map(layer => ({
      pixelData: this._clonePixelData(layer.pixelData),
      name: layer.name,
      visible: layer.visible,
      opacity: layer.opacity,
      locked: !!layer.locked,
    }));
    ld.activeIndex = src.activeIndex;
    ld._composite.createPixelData(ld.width, ld.height);
    ld.markCompositeDirty();
    return ld;
  }

  /**
   * @param {TilesetData} src
   * @returns {TilesetData}
   */
  _cloneTilesetData(src) {
    const td = new TilesetData(src.chipWidth, src.chipHeight, src.columns, src.rows, 0x00000000);
    td.passFlags = src.passFlags.map(row => row.slice());
    for (let row = 0; row < src.rows; row++) {
      for (let col = 0; col < src.columns; col++) {
        td.chips[row][col] = this._cloneLayerData(src.chips[row][col]);
      }
    }
    return td;
  }

  /**
   * @param {{ active: boolean, x: number, y: number, w: number, h: number, mode: string, floating: object|null }} selection
   * @returns {{ active: boolean, x: number, y: number, w: number, h: number, mode: string, floating: object|null }}
   */
  _cloneSelection(selection) {
    const floating = selection?.floating;
    return {
      active: !!selection?.active,
      x: selection?.x | 0,
      y: selection?.y | 0,
      w: selection?.w | 0,
      h: selection?.h | 0,
      mode: typeof selection?.mode === 'string' ? selection.mode : 'rect',
      floating: floating ? {
        pixelData: floating.pixelData ? this._clonePixelData(floating.pixelData) : null,
        srcX: floating.srcX | 0,
        srcY: floating.srcY | 0,
        dstX: floating.dstX | 0,
        dstY: floating.dstY | 0,
        width: floating.width | 0,
        height: floating.height | 0,
        cut: !!floating.cut,
      } : null,
    };
  }

  /**
   * モードに応じた LayerData を返す。
   * tileset モード: 選択中チップの LayerData
   * free モード: 従来の _layerData
   * → これにより PixelCanvas / LayerPanel / ツール群は無改修で動作する。
   * @type {LayerData}
   */
  get layerData() {
    if (this.editMode === 'tileset' && this.tilesetData) {
      return this.tilesetData.getChipLayerData(
        this.selectedChip.col, this.selectedChip.row
      );
    }
    return this._layerData;
  }

  /**
   * アクティブレイヤーの PixelData を返す。
   * 既存コードとの後方互換のための getter。
   * @type {PixelData}
   */
  get pixelData() {
    return this.layerData.getActiveLayer();
  }

  /**
   * pixelData を直接差し替える (インポート時の互換用)。
   * アクティブレイヤーの pixelData を入れ替え、LayerData のサイズも更新する。
   * 常に free モードの _layerData に書き込む。
   * @param {PixelData} pd
   */
  set pixelData(pd) {
    if (!pd) return;
    const ld = this._layerData;
    ld.width  = pd.width;
    ld.height = pd.height;
    // インポート時は1レイヤーにリセット
    ld.layers = [{
      pixelData: pd,
      name:      'レイヤー 1',
      visible:   true,
      opacity:   255,
      locked:    false,
    }];
    ld.activeIndex = 0;
    ld._composite.createPixelData(pd.width, pd.height);
    ld.markCompositeDirty();
  }

  /**
   * free モード用の LayerData を直接差し替える。
   * 主に多レイヤーの単体画像インポート復元に使用する。
   * @param {LayerData} layerData
   * @returns {boolean}
   */
  setLayerData(layerData) {
    if (!layerData || !Array.isArray(layerData.layers) || layerData.layers.length === 0) {
      return false;
    }
    this._layerData = this._cloneLayerData(layerData);
    this.clearSelection();
    this.syncEditorStateToProjectSession();
    return true;
  }

  /**
   * tileset モード用の TilesetData を直接差し替える。
   * @param {TilesetData} tilesetData
   * @param {{ col?: number, row?: number }} [selectedChip]
   * @returns {boolean}
   */
  setTilesetData(tilesetData, selectedChip = null) {
    if (!tilesetData) return false;
    this.tilesetData = this._cloneTilesetData(tilesetData);
    this.editMode = 'tileset';
    this.selectedChip = {
      col: selectedChip?.col | 0,
      row: selectedChip?.row | 0,
    };
    this.clearSelection();
    this.syncEditorStateToProjectSession();
    return true;
  }

  /**
   * マップデータを作成する。
   * @param {number} width
   * @param {number} height
   * @param {number} [tileWidth]
   * @param {number} [tileHeight]
   * @param {string|null} [tilesetId=null]
   * @returns {object}
   */
  createMapData(width, height, tileWidth = 0, tileHeight = 0, tilesetId = null) {
    const mapW = width | 0;
    const mapH = height | 0;
    const tw = tileWidth | 0 || this.currentProject?.settings?.defaultChipWidth | 0 || 16;
    const th = tileHeight | 0 || this.currentProject?.settings?.defaultChipHeight | 0 || 16;
    return this._cloneMapData({
      version: 1,
      width: mapW > 0 ? mapW : 24,
      height: mapH > 0 ? mapH : 18,
      tileWidth: tw > 0 ? tw : 16,
      tileHeight: th > 0 ? th : 16,
      tilesetId: typeof tilesetId === 'string' && tilesetId ? tilesetId : null,
      selectedLayer: 0,
      selectedTileRef: null,
      cursor: { x: 0, y: 0 },
      view: {
        showGrid: true,
        zoom: 2,
      },
      layers: [{
        id: 'layer_ground',
        name: 'Ground',
        visible: true,
        locked: false,
        tiles: new Array((mapW > 0 ? mapW : 24) * (mapH > 0 ? mapH : 18)).fill(-1),
      }],
    });
  }

  /**
   * マップデータを現在の編集状態へ適用する。
   * @param {object|null} mapData
   * @returns {boolean}
   */
  setMapData(mapData) {
    if (!mapData) return false;
    this.mapData = this._cloneMapData(mapData);
    return true;
  }

  /**
   * 現在編集中のアセットへ AppData の状態を保存する。
   * @returns {boolean}
   */
  saveActiveProjectAssetState() {
    const asset = this.getActiveProjectAsset();
    if (!asset || !this.currentProject) return false;

    if (asset.type === 'pixelDocument') {
      asset.width = this._layerData.width;
      asset.height = this._layerData.height;
      asset.layerData = this._cloneLayerData(this._layerData);
    } else if (asset.type === 'tileset' && this.tilesetData) {
      asset.chipWidth = this.tilesetData.chipWidth;
      asset.chipHeight = this.tilesetData.chipHeight;
      asset.columns = this.tilesetData.columns;
      asset.rows = this.tilesetData.rows;
      asset.tilesetData = this._cloneTilesetData(this.tilesetData);
    } else if (asset.type === 'map' && this.mapData) {
      asset.width = this.mapData.width | 0;
      asset.height = this.mapData.height | 0;
      asset.mapData = this._cloneMapData(this.mapData);
    } else {
      return false;
    }

    asset.palette = this.palette?.clone?.() || null;
    this.currentProject.touch();
    this.projectSession?.markDirty();
    return true;
  }

  /**
   * レイヤーデータを指定サイズで初期化する（1レイヤー構成）。
   * 常に free モードの _layerData を初期化する。
   * @param {number} width
   * @param {number} height
   * @param {number} [fillColor=0x00000000]
   */
  createPixelData(width, height, fillColor = 0x00000000) {
    this._layerData.init(width, height, fillColor);
    this.clearSelection();
    this.syncEditorStateToProjectSession();
  }

  /** 選択範囲を解除する。 */
  clearSelection() {
    this.selection.active = false;
    this.selection.x = 0;
    this.selection.y = 0;
    this.selection.w = 0;
    this.selection.h = 0;
    this.selection.mode = 'rect';
    this.selection.floating = null;
  }

  /**
   * 矩形選択を設定する。サイズが 0 以下なら選択解除する。
   * @param {number} x
   * @param {number} y
   * @param {number} w
   * @param {number} h
   */
  setSelectionRect(x, y, w, h) {
    const nx = x | 0;
    const ny = y | 0;
    const nw = w | 0;
    const nh = h | 0;
    if (nw <= 0 || nh <= 0) {
      this.clearSelection();
      return;
    }
    this.selection.active = true;
    this.selection.x = nx;
    this.selection.y = ny;
    this.selection.w = nw;
    this.selection.h = nh;
    this.selection.mode = 'rect';
    this.selection.floating = null;
  }

  /**
   * 選択範囲が存在するか返す。
   * @returns {boolean}
   */
  hasSelection() {
    return this.selection.active && this.selection.w > 0 && this.selection.h > 0;
  }

  /**
   * @param {object|null} src
   * @returns {object}
   */
  _cloneMapData(src) {
    const width = src?.width | 0 || 24;
    const height = src?.height | 0 || 18;
    const tileWidth = src?.tileWidth | 0 || this.currentProject?.settings?.defaultChipWidth | 0 || 16;
    const tileHeight = src?.tileHeight | 0 || this.currentProject?.settings?.defaultChipHeight | 0 || 16;
    const fallbackLayer = [{
      id: 'layer_ground',
      name: 'Ground',
      visible: true,
      locked: false,
      tiles: new Array(width * height).fill(-1),
    }];
    const srcLayers = Array.isArray(src?.layers) && src.layers.length ? src.layers : fallbackLayer;
    const layers = srcLayers.map((layer, index) => {
      const tiles = Array.isArray(layer?.tiles) ? layer.tiles.slice(0, width * height) : [];
      while (tiles.length < width * height) tiles.push(-1);
      return {
        id: typeof layer?.id === 'string' && layer.id ? layer.id : `layer_${index + 1}`,
        name: typeof layer?.name === 'string' && layer.name ? layer.name : `Layer ${index + 1}`,
        visible: layer?.visible !== false,
        locked: !!layer?.locked,
        tiles,
      };
    });
    const selectedLayer = Math.max(0, Math.min((src?.selectedLayer | 0), layers.length - 1));
    const selectedTileRef = src?.selectedTileRef ? {
      tilesetId: typeof src.selectedTileRef.tilesetId === 'string' && src.selectedTileRef.tilesetId
        ? src.selectedTileRef.tilesetId
        : null,
      col: src.selectedTileRef.col | 0,
      row: src.selectedTileRef.row | 0,
      index: src.selectedTileRef.index | 0,
    } : null;
    return {
      version: src?.version | 0 || 1,
      width,
      height,
      tileWidth,
      tileHeight,
      tilesetId: typeof src?.tilesetId === 'string' && src.tilesetId ? src.tilesetId : null,
      selectedLayer,
      selectedTileRef,
      cursor: {
        x: src?.cursor?.x | 0,
        y: src?.cursor?.y | 0,
      },
      view: {
        showGrid: src?.view?.showGrid !== false,
        zoom: Math.max(1, src?.view?.zoom | 0 || 2),
      },
      layers,
    };
  }

  /**
   * 浮動選択が存在するか返す。
   * @returns {boolean}
   */
  hasFloatingSelection() {
    return !!this.selection.floating;
  }
}
