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
   * @param {'pixelDocument'|'tileset'|'map'} type
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
    return type === 'pixelDocument' || type === 'tileset' || type === 'map';
  }
}