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
      return null;
    }

    const nextSession = session || ProjectSession.createForProject(project);
    this.projectSession = nextSession;
    this.applyProjectSessionEditorState();
    return this.projectSession;
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
   * 浮動選択が存在するか返す。
   * @returns {boolean}
   */
  hasFloatingSelection() {
    return !!this.selection.floating;
  }
}
