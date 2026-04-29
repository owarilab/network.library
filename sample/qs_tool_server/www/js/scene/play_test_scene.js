/**
 * PlayTestScene
 * Transform + Text の最小描画確認用シーン。
 */
class PlayTestScene extends Scene {
  constructor() {
    super();
    this._appData = null;
    this._input = null;
    this._runtime = null;
    this._cameraState = null;
    this._previewRect = null;
    this._imageCanvasCache = new Map();
    this._pointerState = {
      canvasX: 0,
      canvasY: 0,
      worldX: 0,
      worldY: 0,
      insidePreview: false,
      hoveredTriggerMap: new Map(),
      pressedTriggerIds: new Set(),
    };
    this._activeOverlapKeys = new Set();
    this._firedOnceTriggerKeys = new Set();
    this._pressedKeys = new Set();
    this._statusMessage = 'Play test preview';
    this._statusTone = 'muted';

    this._onKeyDown = this._onKeyDown.bind(this);
    this._onKeyUp = this._onKeyUp.bind(this);
    this._onMouseMove = this._onMouseMove.bind(this);
    this._onMouseDown = this._onMouseDown.bind(this);
    this._onMouseUp = this._onMouseUp.bind(this);
  }

  onEnter(input, appData) {
    this._input = input;
    this._appData = appData;
    this._runtime = PlayUnitRuntime.fromPlayUnit(appData?.getActiveProjectAsset?.());
    this._cameraState = this._createCameraState(this._runtime?.camera);
    input.on('keydown', this._onKeyDown);
    input.on('keyup', this._onKeyUp);
    input.on('mousemove', this._onMouseMove);
    input.on('mousedown', this._onMouseDown);
    input.on('mouseup', this._onMouseUp);
  }

  onLeave() {
    this._pressedKeys.clear();
    this._imageCanvasCache.clear();
    this._pointerState.hoveredTriggerMap.clear();
    this._pointerState.pressedTriggerIds.clear();
    this._activeOverlapKeys.clear();
  }

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
    this._previewRect = preview;

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
    ctx.fillText('Arrows / WASD: Move Controller', panel.x + 220, 68);

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
    ctx.fillText(`Text: ${runtime?.textEntries?.length || 0} / Image: ${runtime?.imageEntries?.length || 0} / Rectangle: ${runtime?.rectangleEntries?.length || 0}`, panel.x + 18, panel.y + 52);
    ctx.fillText(`Camera: ${runtime?.camera?.objectName || 'DefaultCamera'} (${Math.round(this._cameraState?.x || runtime?.camera?.x || 0)}, ${Math.round(this._cameraState?.y || runtime?.camera?.y || 0)}) zoom ${runtime?.camera?.zoom || 1}`, panel.x + 180, panel.y + 52);
    if (runtime?.camera?.followTargetObjectId) {
      ctx.fillText(`Follow: ${runtime.camera.followTargetObjectName || runtime.camera.followTargetObjectId} lerp ${runtime.camera.followLerp || 0}`, panel.x + 18, panel.y + 74);
    }

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

    const renderEntries = this._buildRenderEntries(runtime);
    if (!renderEntries.length) {
      ctx.fillStyle = '#64748b';
      ctx.textAlign = 'left';
      ctx.textBaseline = 'top';
      ctx.font = '14px sans-serif';
      ctx.fillText('No renderable entries. Add Transform + Text or Transform + Image to a PlayObject.', preview.x + 18, preview.y + 18);
      ctx.restore();
      return;
    }

    const camera = this._cameraState || runtime?.camera || { x: 0, y: 0, zoom: 1 };
    ctx.translate(preview.x, preview.y);
    ctx.scale(camera.zoom || 1, camera.zoom || 1);
    ctx.translate(-(camera.x || 0), -(camera.y || 0));

    const project = this._appData?.currentProject || null;
    for (const entry of renderEntries) {
      if (entry.kind === 'image') {
        this._drawImageEntry(ctx, entry, project);
        continue;
      }
      if (entry.kind === 'rectangle') {
        this._drawRectangleEntry(ctx, entry);
        continue;
      }
      this._drawTextEntry(ctx, entry);
    }
    ctx.restore();
  }

  update(dt) {
    const playUnit = this._appData?.getActiveProjectAsset?.();
    if (!playUnit || playUnit.type !== 'playUnit') return;

    this._applyControllerMovement(dt, playUnit);

    const previousCameraObjectId = this._runtime?.camera?.objectId || '';
    this._runtime = PlayUnitRuntime.fromPlayUnit(playUnit);
    this._processOverlapTriggers(playUnit);
    if (!this._cameraState || previousCameraObjectId !== (this._runtime?.camera?.objectId || '')) {
      this._cameraState = this._createCameraState(this._runtime?.camera);
    } else if (this._runtime?.camera) {
      this._cameraState.zoom = this._runtime.camera.zoom;
      if (!this._runtime.camera.followTargetObjectId) {
        this._cameraState.x = this._runtime.camera.x;
        this._cameraState.y = this._runtime.camera.y;
      }
    }

    if (!this._cameraState || !this._runtime?.camera) return;
    this._refreshCameraFollowTarget();
    if (!this._runtime.camera.followTargetObjectId) return;

    const targetX = Number.isFinite(Number(this._runtime.camera.targetX)) ? Number(this._runtime.camera.targetX) : this._cameraState.x;
    const targetY = Number.isFinite(Number(this._runtime.camera.targetY)) ? Number(this._runtime.camera.targetY) : this._cameraState.y;
    const followLerp = Number.isFinite(Number(this._runtime.camera.followLerp)) ? Number(this._runtime.camera.followLerp) : 1;

    if (followLerp >= 1) {
      this._cameraState.x = targetX;
      this._cameraState.y = targetY;
      return;
    }

    if (followLerp <= 0) return;

    const normalizedDt = Number.isFinite(Number(dt)) && Number(dt) > 0 ? Number(dt) / 16.6667 : 1;
    const alpha = 1 - Math.pow(1 - followLerp, normalizedDt);
    this._cameraState.x += (targetX - this._cameraState.x) * alpha;
    this._cameraState.y += (targetY - this._cameraState.y) * alpha;
  }

  _createCameraState(camera) {
    return {
      x: Number.isFinite(Number(camera?.sourceX)) ? Number(camera.sourceX) : Number.isFinite(Number(camera?.x)) ? Number(camera.x) : 0,
      y: Number.isFinite(Number(camera?.sourceY)) ? Number(camera.sourceY) : Number.isFinite(Number(camera?.y)) ? Number(camera.y) : 0,
      zoom: Number.isFinite(Number(camera?.zoom)) && Number(camera.zoom) > 0 ? Number(camera.zoom) : 1,
    };
  }

  _applyControllerMovement(dt, playUnit) {
    if (!playUnit || !Array.isArray(playUnit.objects) || !this._pressedKeys.size) return;

    let moveX = 0;
    let moveY = 0;
    if (this._pressedKeys.has('ArrowLeft') || this._pressedKeys.has('KeyA')) moveX -= 1;
    if (this._pressedKeys.has('ArrowRight') || this._pressedKeys.has('KeyD')) moveX += 1;
    if (this._pressedKeys.has('ArrowUp') || this._pressedKeys.has('KeyW')) moveY -= 1;
    if (this._pressedKeys.has('ArrowDown') || this._pressedKeys.has('KeyS')) moveY += 1;
    if (!moveX && !moveY) return;

    const magnitude = Math.hypot(moveX, moveY) || 1;
    const normalizedX = moveX / magnitude;
    const normalizedY = moveY / magnitude;
    const deltaSeconds = Number.isFinite(Number(dt)) && Number(dt) > 0 ? Number(dt) / 1000 : 1 / 60;

    for (const objectData of playUnit.objects) {
      if (!objectData || objectData.enabled === false) continue;
      const controller = objectData.findComponentByType?.('Controller') || null;
      const transform = objectData.findComponentByType?.('Transform') || null;
      if (!controller || controller.enabled === false || !transform || transform.enabled === false) continue;

      const inputMode = typeof controller.data?.inputMode === 'string' && controller.data.inputMode.trim()
        ? controller.data.inputMode.trim()
        : 'player1';
      if (inputMode !== 'player1') continue;

      const moveSpeed = Number.isFinite(Number(controller.data?.moveSpeed)) && Number(controller.data.moveSpeed) > 0
        ? Number(controller.data.moveSpeed)
        : 120;
      const currentX = Number.isFinite(Number(transform.data?.x)) ? Number(transform.data.x) : 0;
      const currentY = Number.isFinite(Number(transform.data?.y)) ? Number(transform.data.y) : 0;
      transform.data.x = currentX + normalizedX * moveSpeed * deltaSeconds;
      transform.data.y = currentY + normalizedY * moveSpeed * deltaSeconds;
    }
  }

  _processOverlapTriggers(playUnit) {
    if (!playUnit || !Array.isArray(playUnit.objects)) return;

    const controllerSources = this._getControllerOverlapSources(playUnit.objects);
    if (!controllerSources.length) {
      this._activeOverlapKeys.clear();
      return;
    }

    const nextOverlapKeys = new Set();
    for (const objectData of playUnit.objects) {
      if (!objectData || objectData.enabled === false) continue;
      const transform = objectData.findComponentByType?.('Transform') || null;
      const collider = objectData.findComponentByType?.('Collider') || null;
      const trigger = objectData.findComponentByType?.('Trigger') || null;
      if (!transform || transform.enabled === false || !collider || collider.enabled === false || !trigger || trigger.enabled === false) continue;
      if (collider.data?.shape !== 'rect' || collider.data?.isTrigger !== true) continue;

      const triggerOn = typeof trigger.data?.triggerOn === 'string' && trigger.data.triggerOn.trim()
        ? trigger.data.triggerOn.trim()
        : 'overlap';
      if (triggerOn !== 'overlap') continue;

      const triggerRect = this._getRectBounds(transform, collider);
      if (!triggerRect) continue;

      const targetObjectId = typeof trigger.data?.targetObjectId === 'string' && trigger.data.targetObjectId.trim()
        ? trigger.data.targetObjectId.trim()
        : '';

      for (const source of controllerSources) {
        if (targetObjectId && source.objectId !== targetObjectId) continue;
        if (!this._intersectsRect(triggerRect, source.rect)) continue;

        const overlapKey = `${objectData.id}:${source.objectId}`;
        nextOverlapKeys.add(overlapKey);
        if (this._activeOverlapKeys.has(overlapKey)) continue;

        this._fireTrigger({
          objectId: typeof objectData.id === 'string' ? objectData.id : '',
          objectName: typeof objectData.name === 'string' && objectData.name.trim() ? objectData.name.trim() : 'Object',
          eventId: typeof trigger.data?.eventId === 'string' ? trigger.data.eventId.trim() : '',
          triggerOn: 'overlap',
          once: trigger.data?.once === true,
          triggerKey: `${objectData.id}:${source.objectId}:${typeof trigger.data?.eventId === 'string' ? trigger.data.eventId.trim() : ''}:overlap`,
        }, 'overlap');
      }
    }

    this._activeOverlapKeys = nextOverlapKeys;
  }

  _getControllerOverlapSources(objects) {
    const sources = [];
    for (const objectData of objects) {
      if (!objectData || objectData.enabled === false) continue;
      const controller = objectData.findComponentByType?.('Controller') || null;
      const transform = objectData.findComponentByType?.('Transform') || null;
      const collider = objectData.findComponentByType?.('Collider') || null;
      if (!controller || controller.enabled === false || !transform || transform.enabled === false || !collider || collider.enabled === false) continue;
      if (collider.data?.shape !== 'rect') continue;

      const rect = this._getRectBounds(transform, collider);
      if (!rect) continue;
      sources.push({
        objectId: typeof objectData.id === 'string' ? objectData.id : '',
        objectName: typeof objectData.name === 'string' && objectData.name.trim() ? objectData.name.trim() : 'Object',
        rect,
      });
    }
    return sources;
  }

  _getRectBounds(transform, collider) {
    const width = Number(collider?.data?.width);
    const height = Number(collider?.data?.height);
    if (!Number.isFinite(width) || !Number.isFinite(height) || width <= 0 || height <= 0) return null;

    const baseX = Number.isFinite(Number(transform?.data?.x)) ? Number(transform.data.x) : 0;
    const baseY = Number.isFinite(Number(transform?.data?.y)) ? Number(transform.data.y) : 0;
    const offsetX = Number.isFinite(Number(collider?.data?.offsetX)) ? Number(collider.data.offsetX) : 0;
    const offsetY = Number.isFinite(Number(collider?.data?.offsetY)) ? Number(collider.data.offsetY) : 0;
    return {
      left: baseX + offsetX,
      top: baseY + offsetY,
      right: baseX + offsetX + width,
      bottom: baseY + offsetY + height,
    };
  }

  _intersectsRect(a, b) {
    if (!a || !b) return false;
    return a.left < b.right
      && a.right > b.left
      && a.top < b.bottom
      && a.bottom > b.top;
  }

  _refreshCameraFollowTarget() {
    const camera = this._runtime?.camera;
    if (!camera?.followTargetObjectId) return;

    const playUnit = this._appData?.getActiveProjectAsset?.();
    if (!playUnit || playUnit.type !== 'playUnit' || typeof playUnit.findObjectById !== 'function') return;

    const targetObject = playUnit.findObjectById(camera.followTargetObjectId);
    const targetTransform = targetObject?.findComponentByType?.('Transform') || null;
    if (!targetObject || !targetTransform) return;

    camera.followTargetObjectName = typeof targetObject.name === 'string' && targetObject.name.trim()
      ? targetObject.name.trim()
      : camera.followTargetObjectId;
    if (Number.isFinite(Number(targetTransform.data?.x))) {
      camera.targetX = Number(targetTransform.data.x);
    }
    if (Number.isFinite(Number(targetTransform.data?.y))) {
      camera.targetY = Number(targetTransform.data.y);
    }
  }

  _drawRectangleEntry(ctx, entry) {
    ctx.save();
    ctx.globalAlpha = 1;

    const { x, y, width, height, shape, fillColor, fillAlpha, strokeColor, strokeWidth, strokeAlpha, rotation, originX, originY } = entry;

    // 基準点による座標オフセット
    const offsetX = width * originX;
    const offsetY = height * originY;
    const drawX = x - offsetX;
    const drawY = y - offsetY;

    ctx.translate(drawX + offsetX, drawY + offsetY);
    if (rotation) {
      ctx.rotate((rotation * Math.PI) / 180);
    }
    ctx.translate(-offsetX, -offsetY);

    // 図形を描画
    if (shape === 'circle') {
      ctx.beginPath();
      ctx.arc(offsetX, offsetY, width * 0.5, 0, Math.PI * 2);
      
      if (fillColor && fillAlpha > 0) {
        ctx.globalAlpha = fillAlpha;
        ctx.fillStyle = fillColor;
        ctx.fill();
      }
      
      if (strokeColor && strokeWidth > 0 && strokeAlpha > 0) {
        ctx.globalAlpha = strokeAlpha;
        ctx.strokeStyle = strokeColor;
        ctx.lineWidth = strokeWidth;
        ctx.stroke();
      }
    } else if (shape === 'polygon') {
      const sides = Math.max(3, Math.floor(entry.sides || 4));
      const radius = Math.min(width, height) * 0.5;
      ctx.beginPath();
      for (let i = 0; i < sides; i++) {
        const angle = (i / sides) * Math.PI * 2 - Math.PI * 0.5;
        const px = offsetX + Math.cos(angle) * radius;
        const py = offsetY + Math.sin(angle) * radius;
        if (i === 0) ctx.moveTo(px, py);
        else ctx.lineTo(px, py);
      }
      ctx.closePath();
      
      if (fillColor && fillAlpha > 0) {
        ctx.globalAlpha = fillAlpha;
        ctx.fillStyle = fillColor;
        ctx.fill();
      }
      
      if (strokeColor && strokeWidth > 0 && strokeAlpha > 0) {
        ctx.globalAlpha = strokeAlpha;
        ctx.strokeStyle = strokeColor;
        ctx.lineWidth = strokeWidth;
        ctx.stroke();
      }
    } else if (shape === 'triangle') {
      const radius = Math.min(width, height) * 0.5;
      ctx.beginPath();
      for (let i = 0; i < 3; i++) {
        const angle = (i / 3) * Math.PI * 2 - Math.PI * 0.5;
        const px = offsetX + Math.cos(angle) * radius;
        const py = offsetY + Math.sin(angle) * radius;
        if (i === 0) ctx.moveTo(px, py);
        else ctx.lineTo(px, py);
      }
      ctx.closePath();
      
      if (fillColor && fillAlpha > 0) {
        ctx.globalAlpha = fillAlpha;
        ctx.fillStyle = fillColor;
        ctx.fill();
      }
      
      if (strokeColor && strokeWidth > 0 && strokeAlpha > 0) {
        ctx.globalAlpha = strokeAlpha;
        ctx.strokeStyle = strokeColor;
        ctx.lineWidth = strokeWidth;
        ctx.stroke();
      }
    } else if (shape === 'star') {
      const points = Math.max(3, Math.floor(entry.points || 5));
      const outerRadius = Math.min(width, height) * 0.5;
      const innerRadius = outerRadius * Math.max(0.1, Math.min(0.9, entry.innerRadius || 0.4));
      ctx.beginPath();
      for (let i = 0; i < points * 2; i++) {
        const radius = i % 2 === 0 ? outerRadius : innerRadius;
        const angle = (i / (points * 2)) * Math.PI * 2 - Math.PI * 0.5;
        const px = offsetX + Math.cos(angle) * radius;
        const py = offsetY + Math.sin(angle) * radius;
        if (i === 0) ctx.moveTo(px, py);
        else ctx.lineTo(px, py);
      }
      ctx.closePath();
      
      if (fillColor && fillAlpha > 0) {
        ctx.globalAlpha = fillAlpha;
        ctx.fillStyle = fillColor;
        ctx.fill();
      }
      
      if (strokeColor && strokeWidth > 0 && strokeAlpha > 0) {
        ctx.globalAlpha = strokeAlpha;
        ctx.strokeStyle = strokeColor;
        ctx.lineWidth = strokeWidth;
        ctx.stroke();
      }
    } else {
      // rectangle（デフォルト）
      if (fillColor && fillAlpha > 0) {
        ctx.globalAlpha = fillAlpha;
        ctx.fillStyle = fillColor;
        ctx.fillRect(0, 0, width, height);
      }
      
      if (strokeColor && strokeWidth > 0 && strokeAlpha > 0) {
        ctx.globalAlpha = strokeAlpha;
        ctx.strokeStyle = strokeColor;
        ctx.lineWidth = strokeWidth;
        ctx.strokeRect(0, 0, width, height);
      }
    }

    ctx.restore();
  }

  _drawTextEntry(ctx, entry) {
    ctx.save();
    ctx.font = entry.font;

    const lines = this._buildTextLines(ctx, entry);
    const lineHeight = entry.lineHeight > 0 ? entry.lineHeight : 28;
    const padding = entry.padding > 0 ? entry.padding : 0;
    const totalHeight = Math.max(0, (lines.length - 1) * lineHeight);
    let startY = entry.y;

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
      let boxX = entry.x;
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
        if (entry.maxWidth > 0) ctx.strokeText(line, entry.x, lineY, entry.maxWidth);
        else ctx.strokeText(line, entry.x, lineY);
      }
      if (entry.maxWidth > 0) ctx.fillText(line, entry.x, lineY, entry.maxWidth);
      else ctx.fillText(line, entry.x, lineY);
    }

    ctx.restore();
  }

  _drawImageEntry(ctx, entry, project) {
    const pixelDocument = project?.findPixelDocumentById?.(entry.pixelDocumentId) || null;
    const layerData = pixelDocument?.layerData || null;
    const pixelData = layerData?.composite?.() || null;
    if (!pixelDocument || !pixelData?.pixels) return;

    const sourceCanvas = this._getPixelDocumentCanvas(pixelDocument, pixelData);
    if (!sourceCanvas) return;

    const naturalWidth = pixelDocument.width | 0 || pixelData.width | 0 || sourceCanvas.width;
    const naturalHeight = pixelDocument.height | 0 || pixelData.height | 0 || sourceCanvas.height;
    const size = this._resolveImageDrawSize(entry, naturalWidth, naturalHeight);
    const drawWidth = size.width;
    const drawHeight = size.height;
    const drawX = entry.x - drawWidth * (Number.isFinite(Number(entry.originX)) ? Number(entry.originX) : 0);
    const drawY = entry.y - drawHeight * (Number.isFinite(Number(entry.originY)) ? Number(entry.originY) : 0);

    ctx.save();
    ctx.globalAlpha = typeof entry.alpha === 'number' ? entry.alpha : 1;
    ctx.imageSmoothingEnabled = false;
    ctx.drawImage(sourceCanvas, drawX, drawY, drawWidth, drawHeight);
    ctx.restore();
  }

  _resolveImageDrawSize(entry, naturalWidth, naturalHeight) {
    const width = Number.isFinite(Number(entry?.width)) && Number(entry.width) > 0 ? Number(entry.width) : 0;
    const height = Number.isFinite(Number(entry?.height)) && Number(entry.height) > 0 ? Number(entry.height) : 0;
    const keepAspect = entry?.keepAspect !== false;
    const safeNaturalWidth = naturalWidth > 0 ? naturalWidth : 1;
    const safeNaturalHeight = naturalHeight > 0 ? naturalHeight : 1;

    if (!keepAspect) {
      return {
        width: width || safeNaturalWidth,
        height: height || safeNaturalHeight,
      };
    }

    if (width > 0 && height > 0) {
      return { width, height };
    }
    if (width > 0) {
      return {
        width,
        height: width * (safeNaturalHeight / safeNaturalWidth),
      };
    }
    if (height > 0) {
      return {
        width: height * (safeNaturalWidth / safeNaturalHeight),
        height,
      };
    }
    return {
      width: safeNaturalWidth,
      height: safeNaturalHeight,
    };
  }

  _getPixelDocumentCanvas(pixelDocument, pixelData) {
    const cacheKey = typeof pixelDocument?.id === 'string' && pixelDocument.id ? pixelDocument.id : '__anonymous__';
    let canvas = this._imageCanvasCache.get(cacheKey) || null;
    if (!canvas) {
      canvas = document.createElement('canvas');
      this._imageCanvasCache.set(cacheKey, canvas);
    }

    const width = pixelData.width | 0;
    const height = pixelData.height | 0;
    if (width <= 0 || height <= 0 || !pixelData.pixels) return null;

    if (canvas.width !== width) canvas.width = width;
    if (canvas.height !== height) canvas.height = height;

    const ctx = canvas.getContext('2d');
    if (!ctx) return null;

    const imageData = ctx.createImageData(width, height);
    const data = imageData.data;
    for (let index = 0; index < pixelData.pixels.length; index++) {
      const color = pixelData.pixels[index];
      data[index * 4] = (color >>> 16) & 0xff;
      data[index * 4 + 1] = (color >>> 8) & 0xff;
      data[index * 4 + 2] = color & 0xff;
      data[index * 4 + 3] = (color >>> 24) & 0xff;
    }
    ctx.putImageData(imageData, 0, 0);
    return canvas;
  }

  _buildRenderEntries(runtime) {
    const imageEntries = Array.isArray(runtime?.imageEntries)
      ? runtime.imageEntries.map((entry) => ({ ...entry, kind: 'image' }))
      : [];
    const textEntries = Array.isArray(runtime?.textEntries)
      ? runtime.textEntries.map((entry) => ({ ...entry, kind: 'text' }))
      : [];
    const rectangleEntries = Array.isArray(runtime?.rectangleEntries)
      ? runtime.rectangleEntries.map((entry) => ({ ...entry, kind: 'rectangle' }))
      : [];
    return [...imageEntries, ...textEntries, ...rectangleEntries].sort((a, b) => {
      if (a.z !== b.z) return a.z - b.z;
      return (a.order || 0) - (b.order || 0);
    });
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
    if (e.code) {
      this._pressedKeys.add(e.code);
    }
    if (e.key === 'Escape') {
      this._appData?.changeScene(new ProjectTopScene());
    }
  }

  _onKeyUp(e) {
    if (e.code) {
      this._pressedKeys.delete(e.code);
    }
  }

  _onMouseMove(e) {
    this._updatePointerState(e);
    const hits = this._getPointerTriggerHits();
    this._processPointerHoverTransitions(hits);
    for (const hit of hits) {
      this._fireTrigger(hit, 'pointerMove');
    }
  }

  _onMouseDown(e) {
    if (e.button !== 0) return;
    this._updatePointerState(e);
    const hits = this._getPointerTriggerHits();
    this._processPointerHoverTransitions(hits);
    this._pointerState.pressedTriggerIds.clear();
    for (const hit of hits) {
      this._pointerState.pressedTriggerIds.add(hit.objectId);
      this._fireTrigger(hit, 'pointerDown');
    }
  }

  _onMouseUp(e) {
    if (e.button !== 0) return;
    this._updatePointerState(e);
    const hits = this._getPointerTriggerHits();
    this._processPointerHoverTransitions(hits);
    for (const hit of hits) {
      this._fireTrigger(hit, 'pointerUp');
      if (this._pointerState.pressedTriggerIds.has(hit.objectId)) {
        this._fireTrigger(hit, 'click');
      }
    }
    this._pointerState.pressedTriggerIds.clear();
  }

  _updatePointerState(e) {
    this._pointerState.canvasX = Number.isFinite(Number(e?.x)) ? Number(e.x) : 0;
    this._pointerState.canvasY = Number.isFinite(Number(e?.y)) ? Number(e.y) : 0;

    const preview = this._previewRect;
    if (!preview) {
      this._pointerState.insidePreview = false;
      return;
    }

    const insidePreview = this._pointerState.canvasX >= preview.x
      && this._pointerState.canvasX <= preview.x + preview.w
      && this._pointerState.canvasY >= preview.y
      && this._pointerState.canvasY <= preview.y + preview.h;
    this._pointerState.insidePreview = insidePreview;
    if (!insidePreview) return;

    const zoom = Number.isFinite(Number(this._cameraState?.zoom)) && Number(this._cameraState.zoom) > 0
      ? Number(this._cameraState.zoom)
      : 1;
    this._pointerState.worldX = (this._pointerState.canvasX - preview.x) / zoom + (this._cameraState?.x || 0);
    this._pointerState.worldY = (this._pointerState.canvasY - preview.y) / zoom + (this._cameraState?.y || 0);
  }

  _getPointerTriggerHits() {
    const playUnit = this._appData?.getActiveProjectAsset?.();
    if (!this._pointerState.insidePreview || !playUnit || playUnit.type !== 'playUnit' || !Array.isArray(playUnit.objects)) {
      return [];
    }

    const hits = [];
    for (const objectData of playUnit.objects) {
      if (!objectData || objectData.enabled === false) continue;
      const transform = objectData.findComponentByType?.('Transform') || null;
      const collider = objectData.findComponentByType?.('Collider') || null;
      const trigger = objectData.findComponentByType?.('Trigger') || null;
      if (!transform || transform.enabled === false || !collider || collider.enabled === false || !trigger || trigger.enabled === false) continue;
      if (collider.data?.shape !== 'rect' || collider.data?.isTrigger !== true) continue;

      const width = Number(collider.data?.width);
      const height = Number(collider.data?.height);
      if (!Number.isFinite(width) || !Number.isFinite(height) || width <= 0 || height <= 0) continue;

      const baseX = Number.isFinite(Number(transform.data?.x)) ? Number(transform.data.x) : 0;
      const baseY = Number.isFinite(Number(transform.data?.y)) ? Number(transform.data.y) : 0;
      const offsetX = Number.isFinite(Number(collider.data?.offsetX)) ? Number(collider.data.offsetX) : 0;
      const offsetY = Number.isFinite(Number(collider.data?.offsetY)) ? Number(collider.data.offsetY) : 0;
      const left = baseX + offsetX;
      const top = baseY + offsetY;
      if (this._pointerState.worldX < left || this._pointerState.worldX > left + width || this._pointerState.worldY < top || this._pointerState.worldY > top + height) {
        continue;
      }

      hits.push({
        objectId: typeof objectData.id === 'string' ? objectData.id : '',
        objectName: typeof objectData.name === 'string' && objectData.name.trim() ? objectData.name.trim() : 'Object',
        eventId: typeof trigger.data?.eventId === 'string' ? trigger.data.eventId.trim() : '',
        triggerOn: typeof trigger.data?.triggerOn === 'string' && trigger.data.triggerOn.trim() ? trigger.data.triggerOn.trim() : 'overlap',
        once: trigger.data?.once === true,
      });
    }
    return hits;
  }

  _processPointerHoverTransitions(hits) {
    const nextHitMap = new Map(hits.map((hit) => [hit.objectId, hit]));
    for (const [objectId, hit] of nextHitMap.entries()) {
      if (!this._pointerState.hoveredTriggerMap.has(objectId)) {
        this._fireTrigger(hit, 'pointerEnter');
      }
    }
    for (const [objectId, hit] of this._pointerState.hoveredTriggerMap.entries()) {
      if (!nextHitMap.has(objectId)) {
        this._fireTrigger(hit, 'pointerLeave');
      }
    }
    this._pointerState.hoveredTriggerMap = nextHitMap;
  }

  _fireTrigger(hit, reason) {
    if (!hit || hit.triggerOn !== reason) return false;
    const triggerKey = hit.triggerKey || `${hit.objectId}:${hit.eventId}:${reason}`;
    if (hit.once && this._firedOnceTriggerKeys.has(triggerKey)) return false;
    if (hit.once) this._firedOnceTriggerKeys.add(triggerKey);

    const eventLabel = hit.eventId || '(no eventId)';
    this._statusTone = 'info';
    this._statusMessage = `Trigger ${eventLabel} on ${hit.objectName} [${reason}]`;
    return true;
  }
}