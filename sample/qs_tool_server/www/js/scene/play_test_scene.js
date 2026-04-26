/**
 * PlayTestScene
 * Transform + Text の最小描画確認用シーン。
 */
class PlayTestScene extends Scene {
  constructor() {
    super();
    this._appData = null;
    this._runtime = null;
    this._statusMessage = 'Play test preview';
    this._statusTone = 'muted';

    this._onKeyDown = this._onKeyDown.bind(this);
  }

  onEnter(input, appData) {
    this._appData = appData;
    this._runtime = PlayUnitRuntime.fromPlayUnit(appData?.getActiveProjectAsset?.());
    input.on('keydown', this._onKeyDown);
  }

  onLeave() {}

  render(ctx, canvas) {
    const runtime = this._runtime;
    const panel = {
      x: Math.max(24, (canvas.width - 860) * 0.5),
      y: 86,
      w: Math.min(860, canvas.width - 48),
      h: Math.max(300, canvas.height - 132),
    };
    const preview = {
      x: panel.x + 18,
      y: panel.y + 84,
      w: panel.w - 36,
      h: panel.h - 102,
    };

    ctx.fillStyle = '#08111f';
    ctx.fillRect(0, 0, canvas.width, canvas.height);

    ctx.fillStyle = '#f8fafc';
    ctx.textAlign = 'left';
    ctx.textBaseline = 'middle';
    ctx.font = 'bold 30px sans-serif';
    ctx.fillText(runtime?.name || 'Play Test', panel.x, 44);

    ctx.fillStyle = '#94a3b8';
    ctx.font = '14px sans-serif';
    ctx.fillText('Esc: Back to project top', panel.x, 68);

    ctx.fillStyle = this._statusTone === 'error' ? '#fca5a5' : '#93c5fd';
    ctx.textAlign = 'right';
    ctx.fillText(this._statusMessage, panel.x + panel.w, 44);

    ctx.fillStyle = '#0f172a';
    ctx.strokeStyle = '#334155';
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.roundRect(panel.x, panel.y, panel.w, panel.h, 14);
    ctx.fill();
    ctx.stroke();

    ctx.textAlign = 'left';
    ctx.textBaseline = 'middle';
    ctx.fillStyle = '#e2e8f0';
    ctx.font = 'bold 18px sans-serif';
    ctx.fillText('Preview', panel.x + 18, panel.y + 28);

    ctx.fillStyle = '#94a3b8';
    ctx.font = '13px sans-serif';
    ctx.fillText(`Text entries: ${runtime?.textEntries?.length || 0}`, panel.x + 18, panel.y + 52);

    ctx.fillStyle = '#020617';
    ctx.strokeStyle = '#1e293b';
    ctx.lineWidth = 1.5;
    ctx.beginPath();
    ctx.roundRect(preview.x, preview.y, preview.w, preview.h, 12);
    ctx.fill();
    ctx.stroke();

    ctx.save();
    ctx.beginPath();
    ctx.rect(preview.x, preview.y, preview.w, preview.h);
    ctx.clip();

    ctx.strokeStyle = '#0f2742';
    ctx.lineWidth = 1;
    ctx.strokeRect(preview.x + 0.5, preview.y + 0.5, preview.w - 1, preview.h - 1);

    if (!runtime || !Array.isArray(runtime.textEntries) || !runtime.textEntries.length) {
      ctx.fillStyle = '#64748b';
      ctx.textAlign = 'left';
      ctx.textBaseline = 'top';
      ctx.font = '14px sans-serif';
      ctx.fillText('No renderable Text entries. Add Transform + Text to a PlayObject.', preview.x + 18, preview.y + 18);
      ctx.restore();
      return;
    }

    for (const entry of runtime.textEntries) {
      this._drawTextEntry(ctx, preview, entry);
    }
    ctx.restore();
  }

  update() {}

  _drawTextEntry(ctx, preview, entry) {
    ctx.save();
    ctx.font = entry.font;

    const lines = this._buildTextLines(ctx, entry);
    const lineHeight = entry.lineHeight > 0 ? entry.lineHeight : 28;
    const padding = entry.padding > 0 ? entry.padding : 0;
    const totalHeight = Math.max(0, (lines.length - 1) * lineHeight);
    let startY = preview.y + entry.y;

    if (entry.baseline === 'middle') {
      startY -= totalHeight * 0.5;
    } else if (entry.baseline === 'bottom') {
      startY -= totalHeight;
    }

    ctx.globalAlpha = typeof entry.alpha === 'number' ? entry.alpha : 1;
    ctx.fillStyle = entry.color;
    ctx.textAlign = entry.align || 'left';
    ctx.textBaseline = 'top';
    ctx.lineJoin = 'round';
    ctx.lineWidth = entry.strokeWidth > 0 ? entry.strokeWidth : 0;
    if (entry.strokeColor) {
      ctx.strokeStyle = entry.strokeColor;
    }

    if (entry.backgroundColor) {
      const metrics = lines.map(line => ctx.measureText(line));
      const contentWidth = metrics.reduce((maxWidth, metric) => Math.max(maxWidth, metric.width), 0);
      const boxWidth = (entry.maxWidth > 0 ? Math.min(entry.maxWidth, contentWidth || entry.maxWidth) : contentWidth) + padding * 2;
      const boxHeight = Math.max(lineHeight, lines.length * lineHeight) + padding * 2;
      let boxX = preview.x + entry.x;
      if (entry.align === 'center') {
        boxX -= boxWidth * 0.5;
      } else if (entry.align === 'right') {
        boxX -= boxWidth;
      }
      const boxY = startY - padding;
      ctx.fillStyle = entry.backgroundColor;
      ctx.fillRect(boxX, boxY, boxWidth, boxHeight);
      ctx.fillStyle = entry.color;
    }

    for (let index = 0; index < lines.length; index++) {
      const lineY = startY + index * lineHeight;
      const line = lines[index];
      if (entry.strokeColor && entry.strokeWidth > 0) {
        if (entry.maxWidth > 0) ctx.strokeText(line, preview.x + entry.x, lineY, entry.maxWidth);
        else ctx.strokeText(line, preview.x + entry.x, lineY);
      }
      if (entry.maxWidth > 0) ctx.fillText(line, preview.x + entry.x, lineY, entry.maxWidth);
      else ctx.fillText(line, preview.x + entry.x, lineY);
    }

    ctx.restore();
  }

  _buildTextLines(ctx, entry) {
    const sourceText = String(entry.text || '');
    const paragraphs = sourceText.split('\n');
    const wrapWidth = entry.wrap && entry.maxWidth > 0 ? entry.maxWidth : 0;
    const lines = [];

    for (const paragraph of paragraphs) {
      if (!wrapWidth) {
        lines.push(paragraph);
        continue;
      }

      const wrappedLines = this._wrapParagraph(ctx, paragraph, wrapWidth);
      if (wrappedLines.length) lines.push(...wrappedLines);
      else lines.push('');
    }

    return lines.length ? lines : [''];
  }

  _wrapParagraph(ctx, paragraph, wrapWidth) {
    if (!paragraph) return [''];

    const tokens = paragraph.match(/\S+\s*/g);
    if (!tokens || !tokens.length) return [paragraph];

    const lines = [];
    let currentLine = '';

    for (const token of tokens) {
      const candidate = currentLine + token;
      if (!currentLine || ctx.measureText(candidate).width <= wrapWidth) {
        currentLine = candidate;
        continue;
      }

      lines.push(currentLine.trimEnd());
      currentLine = token;

      while (ctx.measureText(currentLine).width > wrapWidth && currentLine.length > 1) {
        const splitIndex = this._findWrapSplitIndex(ctx, currentLine, wrapWidth);
        lines.push(currentLine.slice(0, splitIndex).trimEnd());
        currentLine = currentLine.slice(splitIndex).trimStart();
      }
    }

    if (currentLine) {
      lines.push(currentLine.trimEnd());
    }

    return lines;
  }

  _findWrapSplitIndex(ctx, value, wrapWidth) {
    for (let index = 1; index < value.length; index++) {
      if (ctx.measureText(value.slice(0, index + 1)).width > wrapWidth) {
        return Math.max(1, index);
      }
    }
    return Math.max(1, value.length - 1);
  }

  _onKeyDown(e) {
    if (e.key === 'Escape') {
      this._appData?.changeScene(new ProjectTopScene());
    }
  }
}