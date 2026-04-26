/**
 * PlayUnitEditorScene
 * PlayUnit の最小編集導線を担う雛形シーン。
 */
class PlayUnitEditorScene extends Scene {
  constructor() {
    super();
    this._appData = null;
    this._statusMessage = 'PlayUnit editor scaffold';
    this._statusTone = 'muted';

    this._onKeyDown = this._onKeyDown.bind(this);
  }

  onEnter(input, appData) {
    this._appData = appData;
    input.on('keydown', this._onKeyDown);
  }

  onLeave() {}

  render(ctx, canvas, appData) {
    const asset = appData.getActiveProjectAsset();
    const objects = Array.isArray(asset?.objects) ? asset.objects : [];
    const left = Math.max(24, (canvas.width - 760) * 0.5);
    const top = 72;
    const panelW = Math.min(760, canvas.width - 48);
    const panelH = Math.max(260, canvas.height - 120);

    ctx.fillStyle = '#0f172a';
    ctx.fillRect(0, 0, canvas.width, canvas.height);

    ctx.fillStyle = '#f8fafc';
    ctx.textAlign = 'left';
    ctx.textBaseline = 'middle';
    ctx.font = 'bold 28px sans-serif';
    ctx.fillText(asset?.name || 'PlayUnit', left, top);

    ctx.fillStyle = '#94a3b8';
    ctx.font = '14px sans-serif';
    ctx.fillText(`Objects: ${objects.length}`, left, top + 34);
    ctx.fillText('Esc: Back to project top', left, top + 56);

    ctx.fillStyle = this._statusTone === 'error' ? '#fca5a5' : '#93c5fd';
    ctx.textAlign = 'right';
    ctx.fillText(this._statusMessage, left + panelW, top + 34);

    ctx.fillStyle = '#111827';
    ctx.strokeStyle = '#334155';
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.roundRect(left, top + 84, panelW, panelH, 14);
    ctx.fill();
    ctx.stroke();

    ctx.textAlign = 'left';
    ctx.textBaseline = 'middle';
    ctx.fillStyle = '#e2e8f0';
    ctx.font = 'bold 16px sans-serif';
    ctx.fillText('PlayObjects', left + 18, top + 112);

    if (!asset || asset.type !== 'playUnit') {
      ctx.fillStyle = '#fca5a5';
      ctx.font = '14px sans-serif';
      ctx.fillText('No active PlayUnit selected.', left + 18, top + 148);
      return;
    }

    if (!objects.length) {
      ctx.fillStyle = '#94a3b8';
      ctx.font = '14px sans-serif';
      ctx.fillText('No objects yet. Phase 6 will add object editing.', left + 18, top + 148);
      return;
    }

    const rowH = 28;
    const maxRows = Math.max(1, Math.floor((panelH - 66) / rowH));
    const visibleObjects = objects.slice(0, maxRows);
    for (let index = 0; index < visibleObjects.length; index++) {
      const objectData = visibleObjects[index];
      const rowY = top + 148 + index * rowH;
      ctx.fillStyle = index % 2 === 0 ? '#0b1220' : '#0f172a';
      ctx.fillRect(left + 14, rowY - 12, panelW - 28, rowH - 2);

      ctx.fillStyle = objectData.enabled === false ? '#64748b' : '#f8fafc';
      ctx.font = '14px sans-serif';
      ctx.fillText(objectData.name || objectData.id || 'Object', left + 24, rowY + 1);

      ctx.fillStyle = '#94a3b8';
      ctx.textAlign = 'right';
      ctx.fillText(`${Array.isArray(objectData.components) ? objectData.components.length : 0} components`, left + panelW - 28, rowY + 1);
      ctx.textAlign = 'left';
    }
  }

  update() {}

  _onKeyDown(e) {
    if (e.key === 'Escape') {
      this._appData?.changeScene(new ProjectTopScene());
    }
  }
}