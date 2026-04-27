/**
 * PlayUnitEditorScene
 * PlayUnit の最小編集導線を担う雛形シーン。
 */
class PlayUnitEditorScene extends Scene {
  constructor() {
    super();
    this._appData = null;
    this._buttons = [];
    this._objectItems = [];
    this._componentItems = [];
    this._objectListRect = null;
    this._componentListRect = null;
    this._objectScrollOffset = 0;
    this._componentScrollOffset = 0;
    this._objectVisibleRows = 0;
    this._componentVisibleRows = 0;
    this._selectedObjectIdInput = null;
    this._hoverButtonIndex = -1;
    this._hoverObjectIndex = -1;
    this._hoverComponentIndex = -1;
    this._hoverComponentEditIndex = -1;
    this._selectedObjectId = null;
    this._statusMessage = 'PlayUnit editor scaffold';
    this._statusTone = 'muted';

    this._onMouseMove = this._onMouseMove.bind(this);
    this._onMouseDown = this._onMouseDown.bind(this);
    this._onWheel = this._onWheel.bind(this);
    this._onKeyDown = this._onKeyDown.bind(this);
  }

  onEnter(input, appData) {
    this._appData = appData;
    this._ensureSelectedObjectIdInput();
    input.on('mousemove', this._onMouseMove);
    input.on('mousedown', this._onMouseDown);
    input.on('wheel', this._onWheel);
    input.on('keydown', this._onKeyDown);
  }

  onLeave() {
    this._buttons = [];
    this._objectItems = [];
    this._componentItems = [];
    this._objectListRect = null;
    this._componentListRect = null;
    this._hoverButtonIndex = -1;
    this._hoverObjectIndex = -1;
    this._hoverComponentIndex = -1;
    this._hoverComponentEditIndex = -1;
    this._removeSelectedObjectIdInput();
  }

  render(ctx, canvas, appData) {
    const asset = this._getActivePlayUnit(appData);
    const objects = Array.isArray(asset?.objects) ? asset.objects : [];
    const left = Math.max(24, (canvas.width - 760) * 0.5);
    const top = 72;
    const panelW = Math.min(760, canvas.width - 48);
    const panelH = Math.max(260, canvas.height - 120);
    const canvasRect = canvas.getBoundingClientRect();

    this._ensureSelectedObject(asset);

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

    this._buttons = this._buildButtons(left, top, panelW, !!asset);
    const buttonBottom = this._buttons.reduce((maxY, button) => {
      const bottom = button.rect.y + button.rect.h;
      return bottom > maxY ? bottom : maxY;
    }, top + 124);
    const sectionTitleY = buttonBottom + 26;
    const selectedY = sectionTitleY + 24;
    const listTop = selectedY + 34;
    const messageY = selectedY + 22;
    const listBottom = top + 84 + panelH - 18;
    const contentGap = 16;
    const objectListW = Math.max(280, Math.floor((panelW - 28 - contentGap) * 0.58));
    const componentPanelX = left + 14 + objectListW + contentGap;
    const componentPanelW = left + panelW - 14 - componentPanelX;

    this._objectListRect = {
      x: left + 14,
      y: listTop - 14,
      w: objectListW,
      h: Math.max(64, listBottom - listTop + 14),
    };
    this._componentListRect = {
      x: componentPanelX,
      y: listTop - 14,
      w: componentPanelW,
      h: Math.max(64, listBottom - listTop + 14),
    };

    ctx.textAlign = 'left';
    ctx.textBaseline = 'middle';
    ctx.fillStyle = '#e2e8f0';
    ctx.font = 'bold 16px sans-serif';
    ctx.fillText('PlayObjects', left + 18, sectionTitleY);

    ctx.fillStyle = '#e2e8f0';
    ctx.font = 'bold 14px sans-serif';
    ctx.fillText('Components', componentPanelX, sectionTitleY);

    for (let index = 0; index < this._buttons.length; index++) {
      this._drawButton(ctx, this._buttons[index], index === this._hoverButtonIndex);
    }

    ctx.fillStyle = '#0b1220';
    ctx.strokeStyle = '#1f2937';
    ctx.lineWidth = 1.5;
    ctx.beginPath();
    ctx.roundRect(componentPanelX, this._componentListRect.y, componentPanelW, this._componentListRect.h, 10);
    ctx.fill();
    ctx.stroke();

    if (!asset) {
      this._hideSelectedObjectIdInput();
      this._objectItems = [];
      this._componentItems = [];
      ctx.fillStyle = '#fca5a5';
      ctx.font = '14px sans-serif';
      ctx.fillText('No active PlayUnit selected.', left + 18, messageY);
      return;
    }

    const selectedObject = this._getSelectedObject();
    const defaultCameraObjectId = this._getDefaultCameraObjectId(asset);
    ctx.fillStyle = '#94a3b8';
    ctx.font = '13px sans-serif';
    const selectedLabel = selectedObject?.id === defaultCameraObjectId
      ? `Selected: ${selectedObject?.name || '-'} [default camera]`
      : `Selected: ${selectedObject?.name || '-'}`;
    ctx.fillText(selectedLabel, left + 18, selectedY);
    if (selectedObject?.id) {
      ctx.fillStyle = '#64748b';
      ctx.font = '11px monospace';
      ctx.fillText('ID:', left + 18, selectedY + 18);
      this._updateSelectedObjectIdInput(selectedObject.id, {
        x: canvasRect.left + left + 48,
        y: canvasRect.top + selectedY + 6,
        w: Math.max(140, objectListW - 120),
        h: 24,
      });
    } else {
      this._hideSelectedObjectIdInput();
    }

    if (!objects.length) {
      this._objectItems = [];
      this._componentItems = [];
      ctx.fillStyle = '#94a3b8';
      ctx.font = '14px sans-serif';
      ctx.fillText('No objects yet. Use Add Object to create the first item.', left + 18, messageY);
      return;
    }

    const rowH = 28;
    const maxRows = Math.max(1, Math.floor((listBottom - listTop) / rowH));
    this._objectVisibleRows = maxRows;
    const maxObjectOffset = this._getMaxScrollOffset(objects.length, maxRows);
    if (this._objectScrollOffset > maxObjectOffset) {
      this._objectScrollOffset = maxObjectOffset;
    }
    const visibleObjects = objects.slice(this._objectScrollOffset, this._objectScrollOffset + maxRows);
    this._objectItems = visibleObjects.map((objectData, index) => ({
      objectData,
      rect: {
        x: left + 14,
        y: listTop + index * rowH - 12,
        w: objectListW,
        h: rowH - 2,
      },
    }));

    for (let index = 0; index < visibleObjects.length; index++) {
      const objectData = visibleObjects[index];
      const rowY = listTop + index * rowH;
      const isSelected = objectData.id === this._selectedObjectId;
      const isHovered = index === this._hoverObjectIndex;
      const isDefaultCamera = objectData.id === defaultCameraObjectId;
      ctx.fillStyle = isSelected ? '#1d4ed8' : isHovered ? '#132238' : index % 2 === 0 ? '#0b1220' : '#0f172a';
      ctx.strokeStyle = isSelected ? '#93c5fd' : '#1f2937';
      ctx.lineWidth = isSelected ? 1.5 : 1;
      ctx.beginPath();
      ctx.roundRect(left + 14, rowY - 12, objectListW, rowH - 2, 8);
      ctx.fill();
      ctx.stroke();

      ctx.fillStyle = objectData.enabled === false ? '#64748b' : '#f8fafc';
      ctx.font = '14px sans-serif';
      ctx.fillText(objectData.name || objectData.id || 'Object', left + 24, rowY + 1);

      if (isDefaultCamera) {
        ctx.fillStyle = '#fde68a';
        ctx.font = 'bold 11px sans-serif';
        ctx.fillText('[default camera]', left + 176, rowY + 1);
      }

      ctx.fillStyle = '#94a3b8';
      ctx.textAlign = 'right';
      ctx.fillText(`${Array.isArray(objectData.components) ? objectData.components.length : 0} components`, left + 14 + objectListW - 14, rowY + 1);
      ctx.textAlign = 'left';
    }

    this._componentItems = this._buildComponentItems(selectedObject, componentPanelX, componentPanelW, listTop, listBottom);
    this._drawComponentItems(ctx, selectedObject, componentPanelX, componentPanelW, listTop);
  }

  update() {}

  _buildButtons(left, top, panelW, hasAsset) {
    const buttonDefs = [
      { label: 'Add Object', action: () => this._addObject(), disabled: !hasAsset },
      { label: '+CameraObject', action: () => this._addCameraObject(), disabled: !hasAsset },
      { label: 'Rename', action: () => this._renameSelectedObject(), disabled: !hasAsset },
      { label: 'Delete', action: () => this._deleteSelectedObject(), disabled: !hasAsset },
      { label: '+Transform', action: () => this._addComponentToSelectedObject('Transform'), disabled: !hasAsset },
      { label: '+Camera', action: () => this._addComponentToSelectedObject('Camera'), disabled: !hasAsset },
      { label: '+Controller', action: () => this._addComponentToSelectedObject('Controller'), disabled: !hasAsset },
      { label: '+Tilemap', action: () => this._addComponentToSelectedObject('Tilemap'), disabled: !hasAsset },
      { label: '+Settings', action: () => this._addComponentToSelectedObject('PlaySettings'), disabled: !hasAsset },
      { label: '+Text', action: () => this._addComponentToSelectedObject('Text'), disabled: !hasAsset },
      { label: '+Trigger', action: () => this._addComponentToSelectedObject('Trigger'), disabled: !hasAsset },
    ];
    const buttonW = 120;
    const buttonH = 32;
    const gap = 10;
    const startX = left + 18;
    const y = top + 124;
    const maxRight = left + panelW - 18;
    const columns = Math.max(1, Math.floor((panelW - 36 + gap) / (buttonW + gap)));
    return buttonDefs.map((button, index) => {
      const col = index % columns;
      const row = Math.floor(index / columns);
      const x = startX + col * (buttonW + gap);
      return {
        ...button,
        rect: {
          x,
          y: y + row * (buttonH + gap),
          w: Math.min(buttonW, maxRight - x),
          h: buttonH,
        },
      };
    }).filter(button => button.rect.w > 40);
  }

  _drawButton(ctx, button, hovered) {
    const { x, y, w, h } = button.rect;
    const disabled = !!button.disabled;
    ctx.fillStyle = disabled ? '#1f2937' : hovered ? '#0ea5e9' : '#1e293b';
    ctx.strokeStyle = hovered ? '#bae6fd' : '#475569';
    ctx.lineWidth = 1.5;
    ctx.beginPath();
    ctx.roundRect(x, y, w, h, 8);
    ctx.fill();
    ctx.stroke();

    ctx.fillStyle = disabled ? '#64748b' : '#f8fafc';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.font = 'bold 13px sans-serif';
    ctx.fillText(button.label, x + w / 2, y + h / 2 + 1);
  }

  _onMouseMove(e) {
    this._hoverButtonIndex = this._buttons.findIndex(button => this._inRect(e.x, e.y, button.rect));
    this._hoverObjectIndex = this._objectItems.findIndex(item => this._inRect(e.x, e.y, item.rect));
    this._hoverComponentIndex = this._componentItems.findIndex(item => this._inRect(e.x, e.y, item.deleteRect));
    this._hoverComponentEditIndex = this._componentItems.findIndex(item => this._inRect(e.x, e.y, item.editRect));
  }

  _onWheel(e) {
    const selectedObject = this._getSelectedObject();
    if (this._objectListRect && this._inRect(e.x, e.y, this._objectListRect)) {
      const asset = this._getActivePlayUnit();
      const count = Array.isArray(asset?.objects) ? asset.objects.length : 0;
      const nextOffset = this._getNextScrollOffset(this._objectScrollOffset, count, this._objectVisibleRows, e.deltaY);
      if (nextOffset !== this._objectScrollOffset) {
        this._objectScrollOffset = nextOffset;
        this._hoverObjectIndex = -1;
      }
      return;
    }

    if (this._componentListRect && this._inRect(e.x, e.y, this._componentListRect)) {
      const count = Array.isArray(selectedObject?.components) ? selectedObject.components.length : 0;
      const nextOffset = this._getNextScrollOffset(this._componentScrollOffset, count, this._componentVisibleRows, e.deltaY);
      if (nextOffset !== this._componentScrollOffset) {
        this._componentScrollOffset = nextOffset;
        this._hoverComponentIndex = -1;
      }
    }
  }

  _onMouseDown(e) {
    if (e.button !== 0) return;

    const button = this._buttons.find(item => this._inRect(e.x, e.y, item.rect));
    if (button && !button.disabled && button.action) {
      button.action();
      return;
    }

    const componentItem = this._componentItems.find(item => this._inRect(e.x, e.y, item.deleteRect));
    if (componentItem) {
      this._deleteComponentFromSelectedObject(componentItem.component.type);
      return;
    }

    const editComponentItem = this._componentItems.find(item => this._inRect(e.x, e.y, item.editRect));
    if (editComponentItem) {
      this._editComponentData(editComponentItem.component.type);
      return;
    }

    const objectItem = this._objectItems.find(item => this._inRect(e.x, e.y, item.rect));
    if (!objectItem) return;
    this._selectedObjectId = objectItem.objectData.id;
    this._componentScrollOffset = 0;
    this._statusTone = 'info';
    this._statusMessage = `Selected: ${objectItem.objectData.name || objectItem.objectData.id}`;
  }

  _onKeyDown(e) {
    if (e.key === 'Escape') {
      this._appData?.changeScene(new ProjectTopScene());
      return;
    }
    if (e.key === 'Delete') {
      this._deleteSelectedObject();
    }
  }

  _getActivePlayUnit(appData = this._appData) {
    const asset = appData?.getActiveProjectAsset?.();
    return asset && asset.type === 'playUnit' ? asset : null;
  }

  _ensureSelectedObject(playUnit) {
    if (!playUnit || !Array.isArray(playUnit.objects) || !playUnit.objects.length) {
      this._selectedObjectId = null;
      this._componentScrollOffset = 0;
      return;
    }

    if (this._selectedObjectId && playUnit.objects.some(objectData => objectData.id === this._selectedObjectId)) {
      return;
    }

    this._selectedObjectId = playUnit.objects[0].id;
    this._componentScrollOffset = 0;
  }

  _addObject() {
    const playUnit = this._getActivePlayUnit();
    if (!playUnit) {
      this._statusTone = 'error';
      this._statusMessage = 'No active PlayUnit';
      return;
    }

    const objectData = playUnit.addObject({
      name: `Object ${playUnit.objects.length + 1}`,
      parentId: null,
      children: [],
      components: [],
    });
    this._selectedObjectId = objectData.id;
    this._componentScrollOffset = 0;
    this._markDirty(`Added object: ${objectData.name}`);
  }

  _addCameraObject() {
    const playUnit = this._getActivePlayUnit();
    if (!playUnit) {
      this._statusTone = 'error';
      this._statusMessage = 'No active PlayUnit';
      return;
    }

    const rootObject = playUnit.objects.find((objectData) => objectData?.parentId === null && (objectData.name === 'Root' || objectData.id === this._selectedObjectId)) || null;
    const cameraIndex = playUnit.objects.filter((objectData) => typeof objectData?.name === 'string' && objectData.name.startsWith('CameraObject')).length + 1;
    const objectData = playUnit.addObject({
      name: cameraIndex === 1 ? 'CameraObject' : `CameraObject ${cameraIndex}`,
      parentId: rootObject?.id || null,
      children: [],
      components: [
        { type: 'Transform', data: this._createComponentTemplate('Transform') },
        { type: 'Camera', data: this._createComponentTemplate('Camera') },
      ],
    });

    if (rootObject && !rootObject.children.includes(objectData.id)) {
      rootObject.children.push(objectData.id);
    }

    this._selectedObjectId = objectData.id;
    this._componentScrollOffset = 0;
    this._markDirty(`Added camera object: ${objectData.name}`);
  }

  _renameSelectedObject() {
    const objectData = this._getSelectedObject();
    if (!objectData) {
      this._statusTone = 'error';
      this._statusMessage = 'Select an object first';
      return;
    }

    const nextName = window.prompt('Object name', objectData.name || 'Object');
    if (nextName == null) return;
    if (!nextName.trim()) {
      this._statusTone = 'error';
      this._statusMessage = 'Object name is required';
      return;
    }

    objectData.name = nextName.trim();
    this._markDirty(`Renamed object: ${objectData.name}`);
  }

  _deleteSelectedObject() {
    const playUnit = this._getActivePlayUnit();
    const objectData = this._getSelectedObject();
    if (!playUnit || !objectData) {
      this._statusTone = 'error';
      this._statusMessage = 'Select an object first';
      return;
    }
    if (!window.confirm(`Delete object "${objectData.name || objectData.id}"?`)) return;

    const label = objectData.name || objectData.id;
    if (!playUnit.removeObjectById(objectData.id)) {
      this._statusTone = 'error';
      this._statusMessage = 'Object delete failed';
      return;
    }

    this._ensureSelectedObject(playUnit);
    this._markDirty(`Deleted object: ${label}`);
  }

  _addComponentToSelectedObject(type) {
    const objectData = this._getSelectedObject();
    if (!objectData) {
      this._statusTone = 'error';
      this._statusMessage = 'Select an object first';
      return;
    }

    if (objectData.findComponentByType(type)) {
      this._statusTone = 'error';
      this._statusMessage = `${type} already exists on ${objectData.name || objectData.id}`;
      return;
    }

    const component = objectData.addComponent({ type, data: this._createComponentTemplate(type) });
    this._markDirty(`Added ${component.type} to ${objectData.name || objectData.id}`);
  }

  _createComponentTemplate(type) {
    switch (type) {
      case 'Transform':
        return {
          x: 0,
          y: 0,
          z: 0,
          rotation: 0,
          scaleX: 1,
          scaleY: 1,
        };
      case 'Tilemap':
        return {
          mapAssetId: '',
          layerId: '',
        };
      case 'PlaySettings':
        return {
          defaultCameraObjectId: '',
        };
      case 'Camera':
        return {
          zoom: 1,
          viewportX: 0,
          viewportY: 0,
          viewportWidth: 0,
          viewportHeight: 0,
          followTargetObjectId: '',
          followLerp: 1,
        };
      case 'Controller':
        return {
          inputMode: 'player1',
          moveSpeed: 120,
        };
      case 'Text':
        return {
          text: 'Hello World',
          font: '24px sans-serif',
          color: '#ffffff',
          alpha: 1,
          align: 'left',
          baseline: 'top',
          wrap: false,
          maxWidth: 0,
          lineHeight: 28,
          strokeColor: '',
          strokeWidth: 0,
          backgroundColor: '',
          padding: 0,
        };
      case 'Trigger':
        return {
          eventId: '',
          once: false,
          targetObjectId: '',
          area: {
            x: 0,
            y: 0,
            w: 1,
            h: 1,
          },
        };
      default:
        return {};
    }
  }

  _deleteComponentFromSelectedObject(type) {
    const objectData = this._getSelectedObject();
    if (!objectData) {
      this._statusTone = 'error';
      this._statusMessage = 'Select an object first';
      return;
    }

    if (!window.confirm(`Delete component "${type}" from "${objectData.name || objectData.id}"?`)) return;

    const removed = objectData.removeComponentsByType(type);
    if (!removed) {
      this._statusTone = 'error';
      this._statusMessage = `${type} was not found`;
      return;
    }

    this._markDirty(`Removed ${type} from ${objectData.name || objectData.id}`);
  }

  _editComponentData(type) {
    const objectData = this._getSelectedObject();
    if (!objectData) {
      this._statusTone = 'error';
      this._statusMessage = 'Select an object first';
      return;
    }

    const component = objectData.findComponentByType(type);
    if (!component) {
      this._statusTone = 'error';
      this._statusMessage = `${type} was not found`;
      return;
    }

    const initial = JSON.stringify(component.data && typeof component.data === 'object' ? component.data : {}, null, 2);
    const nextText = window.prompt(`${type} data (JSON object)`, initial);
    if (nextText == null) return;

    let parsed;
    try {
      parsed = JSON.parse(nextText);
    } catch {
      this._statusTone = 'error';
      this._statusMessage = `${type} data must be valid JSON`;
      return;
    }

    if (!parsed || typeof parsed !== 'object' || Array.isArray(parsed)) {
      this._statusTone = 'error';
      this._statusMessage = `${type} data must be a JSON object`;
      return;
    }

    component.data = { ...parsed };
    this._markDirty(`Updated ${type} data on ${objectData.name || objectData.id}`);
  }

  _getSelectedObject() {
    const playUnit = this._getActivePlayUnit();
    if (!playUnit || !this._selectedObjectId) return null;
    return playUnit.findObjectById(this._selectedObjectId);
  }

  _getDefaultCameraObjectId(playUnit = this._getActivePlayUnit()) {
    if (!playUnit || !Array.isArray(playUnit.objects)) return '';
    for (const objectData of playUnit.objects) {
      const playSettings = objectData?.findComponentByType?.('PlaySettings') || null;
      const defaultCameraObjectId = typeof playSettings?.data?.defaultCameraObjectId === 'string'
        ? playSettings.data.defaultCameraObjectId.trim()
        : '';
      if (defaultCameraObjectId) return defaultCameraObjectId;
    }
    return '';
  }

  _ensureSelectedObjectIdInput() {
    if (this._selectedObjectIdInput) return;
    const input = document.createElement('input');
    input.type = 'text';
    input.readOnly = true;
    input.spellcheck = false;
    input.autocomplete = 'off';
    input.style.position = 'fixed';
    input.style.zIndex = '20';
    input.style.display = 'none';
    input.style.padding = '2px 8px';
    input.style.border = '1px solid #475569';
    input.style.borderRadius = '6px';
    input.style.background = '#0f172a';
    input.style.color = '#e2e8f0';
    input.style.font = '11px monospace';
    input.style.boxSizing = 'border-box';
    input.style.outline = 'none';
    input.addEventListener('focus', () => input.select());
    document.body.appendChild(input);
    this._selectedObjectIdInput = input;
  }

  _updateSelectedObjectIdInput(value, rect) {
    this._ensureSelectedObjectIdInput();
    if (!this._selectedObjectIdInput) return;
    this._selectedObjectIdInput.value = value;
    this._selectedObjectIdInput.style.display = 'block';
    this._selectedObjectIdInput.style.left = `${Math.round(rect.x)}px`;
    this._selectedObjectIdInput.style.top = `${Math.round(rect.y)}px`;
    this._selectedObjectIdInput.style.width = `${Math.round(rect.w)}px`;
    this._selectedObjectIdInput.style.height = `${Math.round(rect.h)}px`;
  }

  _hideSelectedObjectIdInput() {
    if (!this._selectedObjectIdInput) return;
    this._selectedObjectIdInput.style.display = 'none';
  }

  _removeSelectedObjectIdInput() {
    if (!this._selectedObjectIdInput) return;
    this._selectedObjectIdInput.remove();
    this._selectedObjectIdInput = null;
  }

  _markDirty(message) {
    this._appData?.projectSession?.markDirty();
    this._statusTone = 'info';
    this._statusMessage = message;
  }

  _getNextScrollOffset(currentOffset, itemCount, visibleRows, deltaY) {
    const maxOffset = this._getMaxScrollOffset(itemCount, visibleRows);
    if (maxOffset <= 0) return currentOffset;
    if (deltaY > 0) return Math.min(maxOffset, currentOffset + 1);
    if (deltaY < 0) return Math.max(0, currentOffset - 1);
    return currentOffset;
  }

  _getMaxScrollOffset(itemCount, visibleRows) {
    return Math.max(0, itemCount - Math.max(1, visibleRows | 0));
  }

  _buildComponentItems(selectedObject, panelX, panelW, listTop, listBottom) {
    if (!selectedObject || !Array.isArray(selectedObject.components) || !selectedObject.components.length) return [];
    const rowH = 30;
    const maxRows = Math.max(1, Math.floor((listBottom - listTop - 8) / rowH));
    this._componentVisibleRows = maxRows;
    const maxOffset = this._getMaxScrollOffset(selectedObject.components.length, maxRows);
    if (this._componentScrollOffset > maxOffset) {
      this._componentScrollOffset = maxOffset;
    }
    return selectedObject.components.slice(this._componentScrollOffset, this._componentScrollOffset + maxRows).map((component, index) => ({
      component,
      rect: {
        x: panelX + 10,
        y: listTop + index * rowH - 10,
        w: panelW - 20,
        h: rowH - 2,
      },
      editRect: {
        x: panelX + panelW - 124,
        y: listTop + index * rowH - 5,
        w: 50,
        h: 20,
      },
      deleteRect: {
        x: panelX + panelW - 68,
        y: listTop + index * rowH - 5,
        w: 50,
        h: 20,
      },
    }));
  }

  _drawComponentItems(ctx, selectedObject, panelX, panelW, listTop) {
    if (!selectedObject) {
      this._componentItems = [];
      ctx.fillStyle = '#64748b';
      ctx.font = '13px sans-serif';
      ctx.fillText('No object selected.', panelX + 12, listTop + 10);
      return;
    }

    if (!this._componentItems.length) {
      ctx.fillStyle = '#64748b';
      ctx.font = '13px sans-serif';
      ctx.fillText('No components yet.', panelX + 12, listTop + 10);
      return;
    }

    for (let index = 0; index < this._componentItems.length; index++) {
      const item = this._componentItems[index];
      const { x, y, w, h } = item.rect;
      const deleteHovered = index === this._hoverComponentIndex;
      const editHovered = index === this._hoverComponentEditIndex;

      ctx.fillStyle = index % 2 === 0 ? '#111827' : '#0f172a';
      ctx.strokeStyle = '#1f2937';
      ctx.lineWidth = 1;
      ctx.beginPath();
      ctx.roundRect(x, y, w, h, 8);
      ctx.fill();
      ctx.stroke();

      ctx.fillStyle = item.component.enabled === false ? '#64748b' : '#f8fafc';
      ctx.textAlign = 'left';
      ctx.textBaseline = 'middle';
      ctx.font = '13px sans-serif';
      ctx.fillText(item.component.type || 'Component', x + 10, y + h / 2 + 1);

      const dataLabel = this._formatComponentDataLabel(item.component.data);
      ctx.fillStyle = '#94a3b8';
      ctx.font = '11px sans-serif';
      ctx.fillText(dataLabel, x + 90, y + h / 2 + 1, Math.max(40, w - 210));

      const er = item.editRect;
      ctx.fillStyle = editHovered ? '#2563eb' : '#1d4ed8';
      ctx.strokeStyle = editHovered ? '#93c5fd' : '#60a5fa';
      ctx.lineWidth = 1;
      ctx.beginPath();
      ctx.roundRect(er.x, er.y, er.w, er.h, 6);
      ctx.fill();
      ctx.stroke();

      ctx.fillStyle = '#dbeafe';
      ctx.textAlign = 'center';
      ctx.font = 'bold 11px sans-serif';
      ctx.fillText('Edit', er.x + er.w / 2, er.y + er.h / 2 + 1);

      const dr = item.deleteRect;
      ctx.fillStyle = deleteHovered ? '#dc2626' : '#7f1d1d';
      ctx.strokeStyle = deleteHovered ? '#fca5a5' : '#ef4444';
      ctx.lineWidth = 1;
      ctx.beginPath();
      ctx.roundRect(dr.x, dr.y, dr.w, dr.h, 6);
      ctx.fill();
      ctx.stroke();

      ctx.fillStyle = '#fee2e2';
      ctx.textAlign = 'center';
      ctx.font = 'bold 11px sans-serif';
      ctx.fillText('Delete', dr.x + dr.w / 2, dr.y + dr.h / 2 + 1);
    }
  }

  _formatComponentDataLabel(data) {
    if (!data || typeof data !== 'object' || Array.isArray(data)) return '{}';
    const text = JSON.stringify(data);
    return text.length > 36 ? `${text.slice(0, 33)}...` : text;
  }

  _inRect(x, y, rect) {
    return !!rect && x >= rect.x && y >= rect.y && x < rect.x + rect.w && y < rect.y + rect.h;
  }
}