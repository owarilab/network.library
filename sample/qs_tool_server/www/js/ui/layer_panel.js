/**
 * LayerPanel
 * レイヤー一覧のコンテンツクラス。
 * UIWindow のコンテンツとして使われることを前提とし、
 * 与えられた (x, y) を起点にレイヤー行を描画する。
 *
 * ToolBar と同パターン:
 *   const size = panel.getContentSize();
 *   panel.render(ctx, x, y, appData);
 *   panel.onMouseDown(e, appData);  // → true なら消費
 */
class LayerPanel {
  // ---- レイアウト定数 ----
  static WIDTH    = 160;
  static ROW_H    = 26;
  static BTN_H    = 24;
  static BTN_W    = 30;
  static PADDING  = 4;
  static EYE_W    = 22;
  /** 最大表示レイヤー数（スクロールは未実装、これ以上は見切れる） */
  static MAX_ROWS = 12;

  // ---- スタイル定数 ----
  static BG_ROW        = 'rgba(50,50,50,0.6)';
  static BG_ROW_ACTIVE = '#1e3868';
  static BG_ROW_HOVER  = 'rgba(80,80,100,0.6)';
  static BG_BTN        = 'rgba(60,60,60,0.9)';
  static BG_BTN_HOVER  = 'rgba(80,100,140,0.9)';
  static TEXT_COLOR     = '#dcdcdc';
  static TEXT_DIM       = '#808080';
  static TEXT_FONT      = '11px sans-serif';
  static BORDER_ACTIVE  = '#7ab0ff';
  static EYE_ON         = '#e0e0e0';
  static EYE_OFF        = '#555555';

  constructor() {
    /** ホバー中の行インデックス (-1 = なし) */
    this._hoverRow  = -1;
    /** ホバー中のボタンID (null = なし) */
    this._hoverBtn  = null;
    /** 描画キャッシュ */
    this._rows    = [];
    this._btns    = [];
    this._renderX = 0;
    this._renderY = 0;
    /** レイヤー操作が行われた時のコールバック @type {(()=>void)|null} */
    this.onChange = null;

    // ---- ダブルクリック検出 ----
    this._lastClickTime  = 0;
    this._lastClickLayer = -1;
    /** @type {number} ダブルクリック判定の最大間隔 (ms) */
    this._dblClickThresh = 400;

    // ---- インライン編集 ----
    /** @type {HTMLInputElement|null} */
    this._editInput   = null;
    /** 編集中のレイヤーインデックス (-1 = なし) */
    this._editingIndex = -1;
  }

  // ----------------------------------------------------------------
  // サイズ計算
  // ----------------------------------------------------------------

  /**
   * @param {AppData} appData
   * @returns {{ w: number, h: number }}
   */
  getContentSize(appData) {
    const { WIDTH, ROW_H, BTN_H, PADDING } = LayerPanel;
    const count = appData ? appData.layerData.layers.length : 1;
    const rows  = Math.min(count, LayerPanel.MAX_ROWS);
    return {
      w: WIDTH,
      h: PADDING + rows * ROW_H + PADDING + BTN_H + PADDING,
    };
  }

  // ----------------------------------------------------------------
  // 描画
  // ----------------------------------------------------------------

  /**
   * @param {CanvasRenderingContext2D} ctx
   * @param {number}  x
   * @param {number}  y
   * @param {AppData} appData
   */
  render(ctx, x, y, appData) {
    const { WIDTH, ROW_H, BTN_H, BTN_W, PADDING, EYE_W,
            BG_ROW, BG_ROW_ACTIVE, BG_ROW_HOVER,
            BG_BTN, BG_BTN_HOVER,
            TEXT_COLOR, TEXT_DIM, TEXT_FONT,
            BORDER_ACTIVE, EYE_ON, EYE_OFF } = LayerPanel;

    this._renderX = x;
    this._renderY = y;
    this._rows    = [];
    this._btns    = [];

    const ld = appData.layerData;

    // ---- レイヤー行（最前面が上）----
    const rowCount = Math.min(ld.layers.length, LayerPanel.MAX_ROWS);
    for (let vi = 0; vi < rowCount; vi++) {
      // 表示順: 配列の末尾 (最前面) を上に
      const li     = ld.layers.length - 1 - vi;
      const layer  = ld.layers[li];
      const ry     = y + PADDING + vi * ROW_H;
      const active = li === ld.activeIndex;
      const hover  = this._hoverRow === vi;

      this._rows.push({ x, y: ry, w: WIDTH, h: ROW_H, layerIndex: li });

      // 行背景
      ctx.save();
      ctx.fillStyle = active ? BG_ROW_ACTIVE : (hover ? BG_ROW_HOVER : BG_ROW);
      ctx.fillRect(x, ry, WIDTH, ROW_H);

      // アクティブ行の左端インジケータ
      if (active) {
        ctx.fillStyle = BORDER_ACTIVE;
        ctx.fillRect(x, ry, 3, ROW_H);
      }

      // 目アイコン (visibility toggle)
      const eyeX  = x + 4;
      const eyeY  = ry + (ROW_H - 16) / 2;
      ctx.fillStyle = layer.visible ? EYE_ON : EYE_OFF;
      this._drawEyeIcon(ctx, eyeX + EYE_W / 2, ry + ROW_H / 2, layer.visible);

      // レイヤー名
      ctx.font         = TEXT_FONT;
      ctx.fillStyle    = layer.visible ? TEXT_COLOR : TEXT_DIM;
      ctx.textAlign    = 'left';
      ctx.textBaseline = 'middle';
      const nameX = x + EYE_W + 8;
      const maxNameW = WIDTH - EYE_W - 12;
      ctx.save();
      ctx.beginPath();
      ctx.rect(nameX, ry, maxNameW, ROW_H);
      ctx.clip();
      ctx.fillText(layer.name, nameX, ry + ROW_H / 2);
      ctx.restore();

      // 行下端セパレータ
      ctx.strokeStyle = 'rgba(80,80,80,0.5)';
      ctx.lineWidth   = 1;
      ctx.beginPath();
      ctx.moveTo(x,         ry + ROW_H - 0.5);
      ctx.lineTo(x + WIDTH, ry + ROW_H - 0.5);
      ctx.stroke();

      ctx.restore();
    }

    // ---- 下部ボタン ----
    const btnY = y + PADDING + rowCount * ROW_H + PADDING;
    const btns = [
      { id: 'add',    label: '+',  title: '追加' },
      { id: 'remove', label: '−',  title: '削除' },
      { id: 'up',     label: '↑',  title: '上へ' },
      { id: 'down',   label: '↓',  title: '下へ' },
    ];
    const gap    = 4;
    const totalW = btns.length * BTN_W + (btns.length - 1) * gap;
    const startX = x + (WIDTH - totalW) / 2;

    for (let i = 0; i < btns.length; i++) {
      const btn  = btns[i];
      const bx   = startX + i * (BTN_W + gap);
      const hover = this._hoverBtn === btn.id;

      this._btns.push({ x: bx, y: btnY, w: BTN_W, h: BTN_H, id: btn.id });

      ctx.save();
      ctx.fillStyle = hover ? BG_BTN_HOVER : BG_BTN;
      if (ctx.roundRect) {
        ctx.beginPath();
        ctx.roundRect(bx, btnY, BTN_W, BTN_H, 3);
        ctx.fill();
      } else {
        ctx.fillRect(bx, btnY, BTN_W, BTN_H);
      }

      ctx.font         = TEXT_FONT;
      ctx.fillStyle    = TEXT_COLOR;
      ctx.textAlign    = 'center';
      ctx.textBaseline = 'middle';
      ctx.fillText(btn.label, bx + BTN_W / 2, btnY + BTN_H / 2);
      ctx.restore();
    }
  }

  /**
   * 目のアイコンを描画する。
   */
  _drawEyeIcon(ctx, cx, cy, visible) {
    ctx.save();
    ctx.lineWidth = 1.2;
    ctx.lineCap   = 'round';
    ctx.lineJoin  = 'round';

    if (visible) {
      // 目の形（楕円弧）
      ctx.strokeStyle = LayerPanel.EYE_ON;
      ctx.beginPath();
      ctx.ellipse(cx, cy, 7, 4, 0, 0, Math.PI * 2);
      ctx.stroke();
      // 瞳
      ctx.fillStyle = LayerPanel.EYE_ON;
      ctx.beginPath();
      ctx.arc(cx, cy, 2, 0, Math.PI * 2);
      ctx.fill();
    } else {
      // 閉じた目
      ctx.strokeStyle = LayerPanel.EYE_OFF;
      ctx.beginPath();
      ctx.moveTo(cx - 7, cy);
      ctx.quadraticCurveTo(cx, cy + 5, cx + 7, cy);
      ctx.stroke();
      // 斜線
      ctx.beginPath();
      ctx.moveTo(cx - 4, cy - 4);
      ctx.lineTo(cx + 4, cy + 4);
      ctx.stroke();
    }
    ctx.restore();
  }

  // ----------------------------------------------------------------
  // イベント
  // ----------------------------------------------------------------

  /**
   * @param {{x: number, y: number}} e  コンテンツ座標ではなくスクリーン座標
   */
  onMouseMove(e) {
    this._hoverRow = -1;
    this._hoverBtn = null;

    // 行ヒットテスト
    for (let i = 0; i < this._rows.length; i++) {
      const r = this._rows[i];
      if (e.x >= r.x && e.x < r.x + r.w && e.y >= r.y && e.y < r.y + r.h) {
        this._hoverRow = i;
        break;
      }
    }

    // ボタンヒットテスト
    for (const b of this._btns) {
      if (e.x >= b.x && e.x < b.x + b.w && e.y >= b.y && e.y < b.y + b.h) {
        this._hoverBtn = b.id;
        break;
      }
    }
  }

  /**
   * @param {{x: number, y: number, button: number}} e
   * @param {AppData} appData
   * @returns {boolean} true = consumed
   */
  onMouseDown(e, appData) {
    // 編集中なら確定
    if (this._editingIndex >= 0) {
      this._commitEdit(appData);
    }

    const ld  = appData.layerData;
    const now = Date.now();

    // ---- 行クリック ----
    for (const r of this._rows) {
      if (e.x >= r.x && e.x < r.x + r.w && e.y >= r.y && e.y < r.y + r.h) {
        // 目アイコン領域 (先頭 EYE_W + 4 px)
        if (e.x < r.x + LayerPanel.EYE_W + 4) {
          ld.toggleVisibility(r.layerIndex);
          ld.markCompositeDirty();
          this._lastClickLayer = -1;
        } else {
          // 名前領域のダブルクリック → インライン編集開始
          const nameX = r.x + LayerPanel.EYE_W + 8;
          if (e.x >= nameX &&
              r.layerIndex === this._lastClickLayer &&
              now - this._lastClickTime < this._dblClickThresh) {
            this._startEdit(r, appData);
            this._lastClickLayer = -1;
            this.onChange?.();
            return true;
          }
          // シングルクリック → レイヤー選択
          ld.setActiveIndex(r.layerIndex);
          this._lastClickLayer = r.layerIndex;
          this._lastClickTime  = now;
        }
        this.onChange?.();
        return true;
      }
    }

    // ---- ボタンクリック ----
    for (const b of this._btns) {
      if (e.x >= b.x && e.x < b.x + b.w && e.y >= b.y && e.y < b.y + b.h) {
        switch (b.id) {
          case 'add':
            ld.addLayer();
            ld.markCompositeDirty();
            break;
          case 'remove':
            ld.removeLayer(ld.activeIndex);
            ld.markCompositeDirty();
            break;
          case 'up':
            // 「上へ」= 配列上では index + 1 (前面へ)
            if (ld.activeIndex < ld.layers.length - 1) {
              ld.moveLayer(ld.activeIndex, ld.activeIndex + 1);
              ld.markCompositeDirty();
            }
            break;
          case 'down':
            // 「下へ」= 配列上では index - 1 (背面へ)
            if (ld.activeIndex > 0) {
              ld.moveLayer(ld.activeIndex, ld.activeIndex - 1);
              ld.markCompositeDirty();
            }
            break;
        }
        this.onChange?.();
        return true;
      }
    }

    return false;
  }

  // ----------------------------------------------------------------
  // インライン編集
  // ----------------------------------------------------------------

  /** 現在インライン編集中かどうか */
  get isEditing() { return this._editingIndex >= 0; }

  /**
   * レイヤー名のインライン編集を開始する。
   * canvas の上に DOM <input> を重ねて表示する。
   * @param {{ x: number, y: number, w: number, h: number, layerIndex: number }} row
   * @param {AppData} appData
   */
  _startEdit(row, appData) {
    if (this._editingIndex >= 0) return;  // 既に編集中

    const layer = appData.layerData.layers[row.layerIndex];
    if (!layer) return;

    this._editingIndex = row.layerIndex;

    const nameX = row.x + LayerPanel.EYE_W + 8;
    const nameW = LayerPanel.WIDTH - LayerPanel.EYE_W - 12;

    // canvas 要素を取得
    const canvas = document.getElementById('mainCanvas');
    if (!canvas) return;
    const rect = canvas.getBoundingClientRect();
    // canvas 内部座標→CSS ピクセルへの変換比率
    const scaleX = rect.width  / canvas.width;
    const scaleY = rect.height / canvas.height;

    const input = document.createElement('input');
    input.type  = 'text';
    input.value = layer.name;
    input.style.position   = 'fixed';
    input.style.left       = (rect.left + nameX * scaleX) + 'px';
    input.style.top        = (rect.top  + row.y * scaleY) + 'px';
    input.style.width      = (nameW * scaleX) + 'px';
    input.style.height     = (row.h * scaleY) + 'px';
    input.style.padding    = '0 2px';
    input.style.margin     = '0';
    input.style.border     = '1px solid #7ab0ff';
    input.style.outline    = 'none';
    input.style.background = '#1a1a2e';
    input.style.color      = '#e0e0e0';
    input.style.font       = LayerPanel.TEXT_FONT;
    input.style.fontSize   = (11 * scaleY) + 'px';
    input.style.boxSizing  = 'border-box';
    input.style.zIndex     = '9999';

    input.addEventListener('keydown', (ev) => {
      ev.stopPropagation();
      if (ev.key === 'Enter') {
        this._commitEdit(appData);
      } else if (ev.key === 'Escape') {
        this._cancelEdit();
      }
    });
    input.addEventListener('blur', () => {
      // blur 時に確定
      this._commitEdit(appData);
    });

    document.body.appendChild(input);
    this._editInput = input;

    // フォーカスして全選択
    requestAnimationFrame(() => {
      input.focus();
      input.select();
    });
  }

  /**
   * 編集を確定してレイヤー名を更新する。
   * @param {AppData} appData
   */
  _commitEdit(appData) {
    if (this._editingIndex < 0 || !this._editInput) return;

    const newName = this._editInput.value.trim();
    const layer   = appData.layerData.layers[this._editingIndex];
    if (layer && newName.length > 0) {
      layer.name = newName;
    }

    this._removeEditInput();
  }

  /** 編集をキャンセルする（名前を変更しない）。 */
  _cancelEdit() {
    this._removeEditInput();
  }

  /** DOM から input を除去して編集状態をクリアする。 */
  _removeEditInput() {
    if (this._editInput) {
      // blur イベントの再帰呼出しを防止
      const input = this._editInput;
      this._editInput    = null;
      this._editingIndex = -1;
      input.remove();
    }
  }
}
