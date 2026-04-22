/**
 * ToolBar
 * ツール選択パネルのコンテンツクラス。
 * UIWindow のコンテンツとして使われることを前提とし、
 * 与えられた (x, y) を起点にツールボタンを縦並びで描画する。
 *
 * activeTool ID:
 *   'pencil' | 'eraser' | 'fill' | 'eyedropper' | 'selectRect'
 *
 * 使い方 (ToolBarWindow 経由が推奨):
 *   const size = toolBar.getContentSize();
 *   toolBar.render(ctx, x, y, appData);
 *   toolBar.onMouseMove(e);
 *   toolBar.onMouseDown(e, appData);  // → true なら消費
 */
class ToolBar {
  // ---- ツール定義 ----
  static TOOLS = [
    { id: 'pencil',     label: 'P', title: 'ペンシル'     },
    { id: 'eraser',     label: 'E', title: '消しゴム'     },
    { id: 'fill',       label: 'F', title: '塗りつぶし'   },
    { id: 'eyedropper', label: 'I', title: 'スポイト'     },
    { id: 'selectRect', label: 'S', title: '矩形選択'     },
  ];

  // ---- レイアウト定数 ----
  /** ボタン 1 個のサイズ (px) */
  static CELL    = 28;
  /** ボタン間の隙間 */
  static GAP     = 3;
  /** コンテンツ内余白 */
  static PADDING = 4;

  // ---- スタイル定数 ----
  static BG_NORMAL     = 'rgba(60,60,60,0.0)';
  static BG_HOVER      = 'rgba(80,80,100,0.9)';
  static BG_ACTIVE     = '#1e3868';
  static BG_ACTIVE_HOV = '#2855a0';
  static BORDER_ACTIVE = '#7ab0ff';
  static ICON_NORMAL   = '#a0a0a0';
  static ICON_ACTIVE   = '#ffffff';
  static ICON_HOVER    = '#e0e0e0';
  static ICON_FONT     = 'bold 13px monospace';
  static TOOLTIP_BG    = 'rgba(20,20,20,0.88)';
  static TOOLTIP_TEXT  = '#eeeeee';
  static TOOLTIP_FONT  = '11px sans-serif';

  constructor() {
    /** ホバー中のボタンインデックス (-1 = なし) */
    this._hoverIndex = -1;

    /** render() 後に格納されるボタン座標キャッシュ */
    this._buttons = [];

    /** render 時の x オフセット (tooltip 描画に使用) */
    this._renderX = 0;
    this._renderY = 0;
    /** コンテンツ幅キャッシュ */
    this._contentW = 0;
  }

  // ----------------------------------------------------------------
  // サイズ計算
  // ----------------------------------------------------------------

  /**
   * コンテンツ領域のサイズ。UIWindow.getContentSize() から呼ばれる。
   * @returns {{ w: number, h: number }}
   */
  getContentSize() {
    const { CELL, GAP, PADDING } = ToolBar;
    const n = ToolBar.TOOLS.length;
    return {
      w: PADDING * 2 + CELL,
      h: PADDING + n * CELL + (n - 1) * GAP + PADDING,
    };
  }

  // ----------------------------------------------------------------
  // 描画
  // ----------------------------------------------------------------

  /**
   * ツールボタンを描画する。
   * @param {CanvasRenderingContext2D} ctx
   * @param {number}  x       コンテンツ左上 X
   * @param {number}  y       コンテンツ左上 Y
   * @param {AppData} appData
   */
  render(ctx, x, y, appData) {
    const { CELL, GAP, PADDING,
            BG_NORMAL, BG_HOVER, BG_ACTIVE, BG_ACTIVE_HOV,
            BORDER_ACTIVE, ICON_NORMAL, ICON_ACTIVE, ICON_HOVER,
            ICON_FONT } = ToolBar;

    this._renderX  = x;
    this._renderY  = y;
    this._contentW = this.getContentSize().w;
    this._buttons  = [];

    const bx = x + PADDING;

    for (let i = 0; i < ToolBar.TOOLS.length; i++) {
      const tool   = ToolBar.TOOLS[i];
      const by     = y + PADDING + i * (CELL + GAP);
      const active = appData.activeTool === tool.id;
      const hover  = this._hoverIndex === i;

      this._buttons.push({ x: bx, y: by, w: CELL, h: CELL, id: tool.id });

      // ---- ボタン背景 ----
      ctx.save();
      if (active && hover) {
        ctx.fillStyle = BG_ACTIVE_HOV;
      } else if (active) {
        ctx.fillStyle = BG_ACTIVE;
      } else if (hover) {
        ctx.fillStyle = BG_HOVER;
      } else {
        ctx.fillStyle = BG_NORMAL;
      }
      const r = 4;
      if (ctx.roundRect) {
        ctx.beginPath();
        ctx.roundRect(bx, by, CELL, CELL, r);
        ctx.fill();
      } else {
        ctx.fillRect(bx, by, CELL, CELL);
      }

      // ---- アクティブ枠線 ----
      if (active) {
        ctx.strokeStyle = BORDER_ACTIVE;
        ctx.lineWidth   = 1.5;
        if (ctx.roundRect) {
          ctx.beginPath();
          ctx.roundRect(bx + 0.75, by + 0.75, CELL - 1.5, CELL - 1.5, r);
          ctx.stroke();
        } else {
          ctx.strokeRect(bx + 0.75, by + 0.75, CELL - 1.5, CELL - 1.5);
        }
      }

      ctx.restore();

      // ---- アイコン描画 ----
      const iconColor = active ? ICON_ACTIVE : (hover ? ICON_HOVER : ICON_NORMAL);
      const cx = bx + CELL / 2;
      const cy = by + CELL / 2;
      this._drawIcon(ctx, tool.id, cx, cy, CELL - 10, iconColor, active);
    }

    // ---- ツールチップ ----
    if (this._hoverIndex >= 0) {
      this._renderTooltip(ctx, this._hoverIndex, x, y);
    }
  }

  /**
   * ツールアイコンを描画する。
   * @param {CanvasRenderingContext2D} ctx
   * @param {string}  id       ツール ID
   * @param {number}  cx       中央 X
   * @param {number}  cy       中央 Y
   * @param {number}  size     描画サイズ (px)
   * @param {string}  color    描画色
   * @param {boolean} active
   */
  _drawIcon(ctx, id, cx, cy, size, color, active) {
    ctx.save();
    ctx.strokeStyle = color;
    ctx.fillStyle   = color;
    ctx.lineWidth   = 1.5;
    ctx.lineCap     = 'round';
    ctx.lineJoin    = 'round';

    const h = size;
    const w = size;

    switch (id) {
      case 'pencil': {
        // 斜め鉛筆: 本体の長方形 + 先端三角
        const ang = -Math.PI / 4;  // 45度傾け
        ctx.translate(cx, cy);
        ctx.rotate(ang);
        // 本体
        ctx.fillRect(-w * 0.18, -h * 0.45, w * 0.36, h * 0.68);
        // 先端
        ctx.beginPath();
        ctx.moveTo(-w * 0.18, h * 0.23);
        ctx.lineTo( w * 0.18, h * 0.23);
        ctx.lineTo(0,          h * 0.50);
        ctx.closePath();
        ctx.fillStyle = active ? 'rgba(255,220,100,0.9)' : 'rgba(200,160,80,0.8)';
        ctx.fill();
        // 消しゴム部分
        ctx.fillStyle = active ? 'rgba(255,180,180,0.9)' : 'rgba(180,120,120,0.7)';
        ctx.fillRect(-w * 0.18, -h * 0.50, w * 0.36, h * 0.12);
        break;
      }
      case 'eraser': {
        // 消しゴム: 角丸四角 + 斜め線
        ctx.translate(cx, cy);
        ctx.rotate(-Math.PI / 6);
        // 本体
        ctx.beginPath();
        if (ctx.roundRect) {
          ctx.roundRect(-w * 0.40, -h * 0.22, w * 0.80, h * 0.44, 2);
        } else {
          ctx.rect(-w * 0.40, -h * 0.22, w * 0.80, h * 0.44);
        }
        ctx.fill();
        // 消しカス部分（左半分を暗く）
        ctx.fillStyle = active ? 'rgba(255,255,255,0.25)' : 'rgba(255,255,255,0.15)';
        ctx.fillRect(-w * 0.40, -h * 0.22, w * 0.40, h * 0.44);
        break;
      }
      case 'fill': {
        // バケツ: 台形 + 注ぎ口
        ctx.translate(cx, cy);
        // バケツ本体（台形で近似）
        ctx.beginPath();
        ctx.moveTo(-w * 0.30, -h * 0.10);
        ctx.lineTo( w * 0.30, -h * 0.10);
        ctx.lineTo( w * 0.22,  h * 0.42);
        ctx.lineTo(-w * 0.22,  h * 0.42);
        ctx.closePath();
        ctx.fill();
        // バケツ上端の横棒
        ctx.lineWidth = 2;
        ctx.beginPath();
        ctx.moveTo(-w * 0.35, -h * 0.10);
        ctx.lineTo( w * 0.35, -h * 0.10);
        ctx.stroke();
        // 取っ手
        ctx.lineWidth = 1.5;
        ctx.beginPath();
        ctx.arc(-w * 0.05, -h * 0.30, w * 0.25, Math.PI, 0);
        ctx.stroke();
        // 注ぎ先の雫
        ctx.fillStyle = active ? 'rgba(100,180,255,0.9)' : 'rgba(80,140,200,0.7)';
        ctx.beginPath();
        ctx.arc(w * 0.42, h * 0.10, w * 0.13, 0, Math.PI * 2);
        ctx.fill();
        break;
      }
      case 'eyedropper': {
        // スポイト: 細長いボディ + 先端
        ctx.translate(cx, cy);
        ctx.rotate(-Math.PI / 4);
        // ボディ
        ctx.beginPath();
        if (ctx.roundRect) {
          ctx.roundRect(-w * 0.12, -h * 0.48, w * 0.24, h * 0.60, 3);
        } else {
          ctx.rect(-w * 0.12, -h * 0.48, w * 0.24, h * 0.60);
        }
        ctx.fill();
        // 先端
        ctx.beginPath();
        ctx.moveTo(-w * 0.10, h * 0.12);
        ctx.lineTo( w * 0.10, h * 0.12);
        ctx.lineTo(0,          h * 0.48);
        ctx.closePath();
        ctx.fill();
        // キャップ
        ctx.fillStyle = active ? 'rgba(255,255,255,0.4)' : 'rgba(255,255,255,0.2)';
        ctx.fillRect(-w * 0.12, -h * 0.48, w * 0.24, h * 0.14);
        break;
      }
      case 'selectRect': {
        ctx.translate(cx, cy);
        const left = -w * 0.35;
        const top = -h * 0.35;
        const rectW = w * 0.70;
        const rectH = h * 0.70;
        ctx.setLineDash([3, 2]);
        ctx.strokeRect(left, top, rectW, rectH);
        ctx.setLineDash([]);
        ctx.fillRect(left - 1, top - 1, 2, 2);
        ctx.fillRect(left + rectW - 1, top - 1, 2, 2);
        ctx.fillRect(left - 1, top + rectH - 1, 2, 2);
        ctx.fillRect(left + rectW - 1, top + rectH - 1, 2, 2);
        break;
      }
    }
    ctx.restore();
  }

  /**
   * ホバー中ボタンの右側にツールチップを描画する。
   */
  _renderTooltip(ctx, idx, panelX, panelY) {
    const { CELL, GAP, PADDING, TOOLTIP_BG, TOOLTIP_TEXT, TOOLTIP_FONT } = ToolBar;
    const tool    = ToolBar.TOOLS[idx];
    const tooltipX = panelX + this._contentW + 6;
    const tooltipY = panelY + PADDING + idx * (CELL + GAP) + Math.floor(CELL / 2);

    ctx.save();
    ctx.font = TOOLTIP_FONT;
    const tw  = ctx.measureText(tool.title).width;
    const th  = 14;
    const pad = 5;

    ctx.fillStyle = TOOLTIP_BG;
    if (ctx.roundRect) {
      ctx.beginPath();
      ctx.roundRect(tooltipX, tooltipY - th / 2 - 1, tw + pad * 2, th + 2, 3);
      ctx.fill();
    } else {
      ctx.fillRect(tooltipX, tooltipY - th / 2 - 1, tw + pad * 2, th + 2);
    }

    ctx.fillStyle    = TOOLTIP_TEXT;
    ctx.textAlign    = 'left';
    ctx.textBaseline = 'middle';
    ctx.fillText(tool.title, tooltipX + pad, tooltipY);
    ctx.restore();
  }

  // ----------------------------------------------------------------
  // 入力処理
  // ----------------------------------------------------------------

  /**
   * マウス移動。ホバー状態を更新する。
   * @param {{x: number, y: number}} e
   */
  onMouseMove(e) {
    this._hoverIndex = this._hitTestButton(e.x, e.y);
  }

  /**
   * マウスダウン。ヒットしたツールを appData.activeTool にセットする。
   * @param {{x: number, y: number, button: number}} e
   * @param {AppData} appData
   * @returns {boolean}
   */
  onMouseDown(e, appData) {
    const idx = this._hitTestButton(e.x, e.y);
    if (idx < 0) return false;
    appData.activeTool = ToolBar.TOOLS[idx].id;
    return true;
  }

  /**
   * ボタンのヒットテスト。ヒットしたインデックスを返す (-1 = なし)。
   */
  _hitTestButton(x, y) {
    for (let i = 0; i < this._buttons.length; i++) {
      const b = this._buttons[i];
      if (x >= b.x && x < b.x + b.w && y >= b.y && y < b.y + b.h) return i;
    }
    return -1;
  }
}
