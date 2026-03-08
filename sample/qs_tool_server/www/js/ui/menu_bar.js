/**
 * MenuBar
 * canvas 上部にメモ帳風のメニューバーを描画するクラス。
 * 各項目は子属の DropdownMenu を保持し、選択時に onSelect コールバックを呼ぶ。
 *
 * 使い方:
 *   const menuBar = new MenuBar();
 *   menuBar.onSelect = (id) => { ... };
 *   menuBar.render(ctx, canvas);
 *   menuBar.onMouseMove(e);
 *   menuBar.onMouseDown(e);
 */
class MenuBar {
  /** メニューバーの高さ (px) */
  static HEIGHT = 24;

  // スタイル定数
  static FONT              = '13px sans-serif';
  static PADDING_X         = 10;
  static COLOR_BG          = '#d4d0c8';
  static COLOR_BORDER      = '#a0a0a0';
  static COLOR_TEXT        = '#000000';
  static COLOR_HOVER       = '#e8e0d8';
  static COLOR_ACTIVE_BG   = '#0078d7';
  static COLOR_ACTIVE_TEXT = '#ffffff';

  constructor() {
    // トップレベル項目定義（メニューラベル + DropdownMenu）
    this._menuDefs = [
      {
        label: 'ファイル',
        dropdown: new DropdownMenu([
          { id: MenuConstants.FILE_NEW,          label: '新規作成'            },
          { id: MenuConstants.FILE_NEW_TILESET,   label: '新規タイルセット作成' },
          { id: MenuConstants.FILE_OPEN,          label: '開く...'              },
          { id: MenuConstants.FILE_OPEN_TILESET,  label: 'タイルセットを開く...' },
          { id: MenuConstants.SEPARATOR },
          { id: MenuConstants.FILE_SAVE,          label: '保存'            },
          { id: MenuConstants.FILE_EXPORT_TILESET, label: 'タイルセット書き出し' },
          { id: MenuConstants.FILE_EXPORT_CHIP,    label: '選択チップ書き出し'   },
          { id: MenuConstants.SEPARATOR },
          { id: MenuConstants.FILE_EXIT,          label: '終了'                },
        ]),
      },
      {
        label: '編集',
        dropdown: new DropdownMenu([
          { id: MenuConstants.EDIT_UNDO,       label: '元に戻す'   },
          { id: MenuConstants.EDIT_REDO,       label: 'やり直し'   },
          { id: MenuConstants.SEPARATOR },
          { id: MenuConstants.EDIT_CUT,        label: '切り取り' },
          { id: MenuConstants.EDIT_COPY,       label: 'コピー'     },
          { id: MenuConstants.EDIT_PASTE,      label: '貼り付け' },
          { id: MenuConstants.SEPARATOR },
          { id: MenuConstants.EDIT_SELECT_ALL, label: 'すべて選択' },
        ]),
      },
      {
        label: '表示',
        dropdown: new DropdownMenu([
          { id: MenuConstants.VIEW_GRID,       label: 'グリッド表示' },
          { id: MenuConstants.SEPARATOR },
          { id: MenuConstants.VIEW_ZOOM_IN,    label: '拡大'       },
          { id: MenuConstants.VIEW_ZOOM_OUT,   label: '縮小'       },
          { id: MenuConstants.VIEW_ZOOM_RESET, label: '等倍 (リセット)' },
        ]),
      },
      {
        label: 'タイルセット',
        dropdown: new DropdownMenu([
          { id: MenuConstants.TILESET_ADD_ROW,     label: '行を追加'     },
          { id: MenuConstants.TILESET_ADD_COL,      label: '列を追加'     },
          { id: MenuConstants.TILESET_REMOVE_ROW,   label: '行を削除'     },
          { id: MenuConstants.TILESET_REMOVE_COL,    label: '列を削除'     },
          { id: MenuConstants.SEPARATOR },
          { id: MenuConstants.TILESET_COPY_CHIP,    label: 'チップをコピー'   },
          { id: MenuConstants.TILESET_PASTE_CHIP,   label: 'チップをペースト' },
          { id: MenuConstants.TILESET_CLEAR_CHIP,   label: 'チップをクリア'   },
          { id: MenuConstants.TILESET_SWAP_CHIP,    label: 'チップを入れ替え' },
          { id: MenuConstants.SEPARATOR },
          { id: MenuConstants.TILESET_TILE_PREVIEW,  label: 'タイルプレビュー表示' },
        ]),
      },
    ];

    /** 各トップレベル項目の描画領域 (render で算出) */
    this._rects = [];

    /** ホバー中のインデックス (-1 = なし) */
    this._hoverIndex = -1;

    /** 開いているドロップダウンのインデックス (-1 = 閉じている) */
    this._openIndex = -1;

    /**
     * メニュー項目が選択されたときに呼ばれるコールバック
     * @type {((id: string) => void) | null}
     */
    this.onSelect = null;
  }

  /** バーの高さを返す */
  get height() { return MenuBar.HEIGHT; }

  /** 現在ドロップダウンが開いているか */
  get isOpen() { return this._openIndex >= 0; }

  // ----------------------------------------------------------------
  // 描画
  // ----------------------------------------------------------------

  /**
   * メニューバーと開いているドロップダウンを描画する。
   * render() の最後（最前面）に呼ぶこと。
   * @param {CanvasRenderingContext2D} ctx
   * @param {HTMLCanvasElement}       canvas
   */
  render(ctx, canvas) {
    const { HEIGHT, FONT, PADDING_X,
            COLOR_BG, COLOR_BORDER, COLOR_TEXT,
            COLOR_HOVER, COLOR_ACTIVE_BG, COLOR_ACTIVE_TEXT } = MenuBar;

    ctx.save();
    ctx.font = FONT;

    // ---- バー背景 ----
    ctx.fillStyle = COLOR_BG;
    ctx.fillRect(0, 0, canvas.width, HEIGHT);

    // ---- 下境界線 ----
    ctx.strokeStyle = COLOR_BORDER;
    ctx.lineWidth   = 1;
    ctx.beginPath();
    ctx.moveTo(0,            HEIGHT - 0.5);
    ctx.lineTo(canvas.width, HEIGHT - 0.5);
    ctx.stroke();

    // ---- 各トップレベル項目の rect を算出 ----
    let curX = 2;
    this._rects = this._menuDefs.map(def => {
      const tw = ctx.measureText(def.label).width;
      const w  = tw + PADDING_X * 2;
      const rect = { x: curX, y: 0, w, h: HEIGHT };
      curX += w;
      return rect;
    });

    // ---- 各トップレベル項目を描画 ----
    const textBaseline = Math.floor(HEIGHT * 0.72);

    this._menuDefs.forEach((def, i) => {
      const r = this._rects[i];
      const isActive = i === this._openIndex;

      if (isActive) {
        ctx.fillStyle = COLOR_ACTIVE_BG;
        ctx.fillRect(r.x, r.y + 1, r.w, r.h - 2);
        ctx.fillStyle = COLOR_ACTIVE_TEXT;
      } else if (i === this._hoverIndex) {
        ctx.fillStyle = COLOR_HOVER;
        ctx.fillRect(r.x, r.y + 1, r.w, r.h - 2);
        ctx.fillStyle = COLOR_TEXT;
      } else {
        ctx.fillStyle = COLOR_TEXT;
      }

      ctx.font = FONT;
      ctx.fillText(def.label, r.x + PADDING_X, textBaseline);
    });

    // ---- 開いているドロップダウンを描画 ----
    if (this._openIndex >= 0) {
      const r = this._rects[this._openIndex];
      this._menuDefs[this._openIndex].dropdown.render(ctx, r.x, HEIGHT);
    }

    ctx.restore();
  }

  // ----------------------------------------------------------------
  // ヒットテスト
  // ----------------------------------------------------------------

  /**
   * 座標がバー項目上にあるか返す
   * @param {number} x
   * @param {number} y
   * @returns {number} インデックス、なければ -1
   */
  _hitTestBar(x, y) {
    if (y < 0 || y >= MenuBar.HEIGHT || this._rects.length === 0) return -1;
    for (let i = 0; i < this._rects.length; i++) {
      const r = this._rects[i];
      if (x >= r.x && x < r.x + r.w) return i;
    }
    return -1;
  }

  // ----------------------------------------------------------------
  // 入力ハンドラ
  // ----------------------------------------------------------------

  /** @param {{ x:number, y:number }} e */
  onMouseMove(e) {
    this._hoverIndex = this._hitTestBar(e.x, e.y);

    // ドロップダウンが開いている間に別のバー項目へホバーするとスイッチ
    if (this._openIndex >= 0 && this._hoverIndex >= 0 &&
        this._hoverIndex !== this._openIndex) {
      this._menuDefs[this._openIndex].dropdown.reset();
      this._openIndex = this._hoverIndex;
      return;
    }

    // 開済みドロップダウン内のホバーを更新
    if (this._openIndex >= 0) {
      this._menuDefs[this._openIndex].dropdown.onMouseMove(e);
    }
  }

  /** @param {{ x:number, y:number, button:number }} e */
  onMouseDown(e) {
    if (e.button !== 0) return;

    // (1) 開済みドロップダウン内のクリック→項目選択
    if (this._openIndex >= 0) {
      const dd = this._menuDefs[this._openIndex].dropdown;
      if (dd.containsPoint(e.x, e.y)) {
        const id = dd.pick(e);
        if (id) {
          this.onSelect?.(id);
        }
        this._closeDropdown();
        return;
      }
    }

    // (2) バー項目のクリック→ ドロップダウン 開閉トグル
    const barIdx = this._hitTestBar(e.x, e.y);
    if (barIdx >= 0) {
      if (this._openIndex === barIdx) {
        this._closeDropdown();
      } else {
        if (this._openIndex >= 0) this._menuDefs[this._openIndex].dropdown.reset();
        this._openIndex = barIdx;
      }
      return;
    }

    // (3) バー外・ドロップダウン外のクリック→閉じる
    this._closeDropdown();
  }

  /** @param {{ x:number, y:number }} e */
  onMouseUp(e) {
    // 今後: ドラッグ選択等の指策
  }

  /** ドロップダウンを閉じて状態をリセット */
  _closeDropdown() {
    if (this._openIndex >= 0) {
      this._menuDefs[this._openIndex].dropdown.reset();
      this._openIndex = -1;
    }
  }

  /** メニューを強制に閉じる（外部から呼ぶ用） */
  close() {
    this._closeDropdown();
  }
}
