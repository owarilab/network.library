/**
 * GridTheme
 * グリッド・ポップアップ・エディタが共有する色、フォント、描画ヘルパー。
 * シーンごとに複製されていた _drawButton / _inRect / roundRect の代わりに使う。
 */
class GridTheme {
  static COLORS = {
    bg: '#0b1220',
    panel: '#111827',
    panelAlt: '#0f172a',
    header: '#1e293b',
    headerHover: '#273449',
    headerText: '#cbd5e1',
    text: '#e2e8f0',
    muted: '#94a3b8',
    border: '#334155',
    rowAlt: '#0e1626',
    selectionFill: 'rgba(56, 189, 248, 0.16)',
    selectionBorder: '#38bdf8',
    focusBorder: '#7dd3fc',
    errorFill: 'rgba(239, 68, 68, 0.22)',
    errorBorder: '#ef4444',
    readOnlyFill: 'rgba(148, 163, 184, 0.06)',
    accent: '#38bdf8',
    accentText: '#082f49',
    button: '#1e293b',
    buttonHover: '#334155',
    buttonDisabled: '#172033',
    warning: '#fbbf24',
    ok: '#34d399',
  };

  static FONT = '13px sans-serif';
  static FONT_BOLD = 'bold 13px sans-serif';
  static FONT_SMALL = '11px sans-serif';
  static FONT_MONO = '12px monospace';

  /**
   * @param {{x:number,y:number,w:number,h:number}|null} rect
   * @param {number} px
   * @param {number} py
   * @returns {boolean}
   */
  static inRect(rect, px, py) {
    return !!rect && px >= rect.x && px < rect.x + rect.w && py >= rect.y && py < rect.y + rect.h;
  }

  static roundRect(ctx, x, y, w, h, r) {
    const radius = Math.max(0, Math.min(r, w / 2, h / 2));
    ctx.beginPath();
    ctx.moveTo(x + radius, y);
    ctx.lineTo(x + w - radius, y);
    ctx.quadraticCurveTo(x + w, y, x + w, y + radius);
    ctx.lineTo(x + w, y + h - radius);
    ctx.quadraticCurveTo(x + w, y + h, x + w - radius, y + h);
    ctx.lineTo(x + radius, y + h);
    ctx.quadraticCurveTo(x, y + h, x, y + h - radius);
    ctx.lineTo(x, y + radius);
    ctx.quadraticCurveTo(x, y, x + radius, y);
    ctx.closePath();
  }

  /**
   * 幅に収まるよう末尾を … に置き換える。改行は空白として扱う。
   * @param {CanvasRenderingContext2D} ctx  font 設定済み
   * @param {string} text
   * @param {number} maxWidth
   * @returns {string}
   */
  static fitText(ctx, text, maxWidth) {
    const source = String(text ?? '').replace(/\s*\n\s*/g, ' ');
    if (maxWidth <= 0) return '';
    if (ctx.measureText(source).width <= maxWidth) return source;
    let low = 0;
    let high = source.length;
    while (low < high) {
      const mid = Math.ceil((low + high) / 2);
      if (ctx.measureText(`${source.slice(0, mid)}…`).width <= maxWidth) low = mid;
      else high = mid - 1;
    }
    return low > 0 ? `${source.slice(0, low)}…` : '…';
  }

  /**
   * ボタンを描く。
   * @param {CanvasRenderingContext2D} ctx
   * @param {{x:number,y:number,w:number,h:number}} rect
   * @param {string} label
   * @param {{ hover?: boolean, disabled?: boolean, active?: boolean }} [state]
   */
  static drawButton(ctx, rect, label, state = {}) {
    const colors = GridTheme.COLORS;
    ctx.save();
    GridTheme.roundRect(ctx, rect.x, rect.y, rect.w, rect.h, 5);
    ctx.fillStyle = state.active ? colors.accent : state.disabled ? colors.buttonDisabled : state.hover ? colors.buttonHover : colors.button;
    ctx.fill();
    ctx.strokeStyle = colors.border;
    ctx.lineWidth = 1;
    ctx.stroke();
    ctx.font = GridTheme.FONT;
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillStyle = state.active ? colors.accentText : state.disabled ? '#475569' : colors.text;
    ctx.fillText(GridTheme.fitText(ctx, label, rect.w - 8), rect.x + rect.w / 2, rect.y + rect.h / 2 + 0.5);
    ctx.restore();
  }
}
