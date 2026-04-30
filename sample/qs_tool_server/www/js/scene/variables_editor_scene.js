/**
 * VariablesEditorScene
 * Project Variables / 将来の PlayUnit Local Variables を扱う専用シーン。
 * Phase 4 では Project Variables の最低限編集 UI を実装する。
 */
class VariablesEditorScene extends Scene {
  constructor() {
    super();
    this._appData = null;
    this._selectedScopePath = 'system.fixed';
    this._selectedVariableName = '';
    this._statusMessage = 'Phase 4 prototype: edit project variable definitions in memory';
    this._statusTone = 'muted';
    this._hoverTarget = '';
    this._backButtonRect = null;
    this._saveButtonRect = null;
    this._navItems = [];
    this._listItems = [];
    this._actionButtons = [];
    this._listPanelRect = null;
    this._listScrollOffset = 0;
    this._listVisibleRows = 0;

    this._onMouseMove = this._onMouseMove.bind(this);
    this._onMouseDown = this._onMouseDown.bind(this);
    this._onWheel = this._onWheel.bind(this);
    this._onKeyDown = this._onKeyDown.bind(this);
  }

  onEnter(input, appData) {
    this._appData = appData;
    this._ensureProjectGlobalVariables();
    this._syncSelection();
    input.on('mousemove', this._onMouseMove);
    input.on('mousedown', this._onMouseDown);
    input.on('wheel', this._onWheel);
    input.on('keydown', this._onKeyDown);
  }

  onLeave() {
    this._hoverTarget = '';
    this._backButtonRect = null;
    this._saveButtonRect = null;
    this._navItems = [];
    this._listItems = [];
    this._actionButtons = [];
    this._listPanelRect = null;
  }

  render(ctx, canvas, appData) {
    const layout = this._createLayout(canvas);
    const projectName = appData?.currentProject?.name || 'No Project';
    const bucket = this._getSelectedBucket();
    const entries = Object.entries(bucket);
    const selectedDefinition = this._getSelectedDefinition();

    this._backButtonRect = layout.backButton;
    this._saveButtonRect = layout.saveButton;
    this._navItems = this._buildNavItems(layout.nav);
    this._listItems = this._buildListItems(layout.list, entries);
    this._actionButtons = this._buildActionButtons(layout, !!selectedDefinition);

    ctx.fillStyle = '#0f172a';
    ctx.fillRect(0, 0, canvas.width, canvas.height);

    this._drawHeader(ctx, layout, projectName);
    this._drawPanels(ctx, layout);
    this._drawNav(ctx, layout.nav);
    this._drawList(ctx, layout.list, entries);
    this._drawEditor(ctx, layout.editor, selectedDefinition);
  }

  _createLayout(canvas) {
    const panel = {
      x: Math.max(28, (canvas.width - 1180) * 0.5),
      y: 56,
      w: Math.min(1180, canvas.width - 56),
      h: Math.max(420, canvas.height - 112),
    };
    const backButton = {
      x: panel.x + panel.w - 196,
      y: 12,
      w: 196,
      h: 36,
    };
    const saveButton = {
      x: panel.x + panel.w - 336,
      y: 12,
      w: 128,
      h: 36,
    };
    const innerX = panel.x + 22;
    const innerY = panel.y + 20;
    const innerW = panel.w - 44;
    const innerH = panel.h - 40;
    const gap = 16;
    let nav;
    let list;
    let editor;

    if (panel.w < 900) {
      const navH = Math.max(132, Math.min(164, innerH * 0.28));
      const bottomY = innerY + navH + gap;
      const bottomH = innerH - navH - gap;
      const listW = Math.max(210, Math.floor((innerW - gap) * 0.42));
      nav = { x: innerX, y: innerY, w: innerW, h: navH };
      list = { x: innerX, y: bottomY, w: listW, h: bottomH };
      editor = { x: innerX + listW + gap, y: bottomY, w: innerW - listW - gap, h: bottomH };
    } else {
      const navW = 264;
      const listW = 330;
      const editorW = innerW - navW - listW - gap * 2;
      nav = { x: innerX, y: innerY, w: navW, h: innerH };
      list = { x: innerX + navW + gap, y: innerY, w: listW, h: innerH };
      editor = { x: innerX + navW + gap + listW + gap, y: innerY, w: editorW, h: innerH };
    }

    return {
      panel,
      backButton,
      saveButton,
      nav,
      list,
      editor,
      titleX: panel.x,
      titleY: 30,
      subtitleY: 58,
    };
  }

  _drawHeader(ctx, layout, projectName) {
    ctx.fillStyle = '#e2e8f0';
    ctx.textAlign = 'left';
    ctx.textBaseline = 'middle';
    ctx.font = 'bold 30px sans-serif';
    ctx.fillText('Variables Editor', layout.titleX, layout.titleY);

    ctx.fillStyle = '#94a3b8';
    ctx.font = '14px sans-serif';
    ctx.fillText(`Project: ${projectName}`, layout.titleX, layout.subtitleY);

    const hovered = this._hoverTarget === 'back';
    ctx.fillStyle = hovered ? '#0ea5e9' : '#111827';
    ctx.strokeStyle = hovered ? '#bae6fd' : '#475569';
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.roundRect(layout.backButton.x, layout.backButton.y, layout.backButton.w, layout.backButton.h, 12);
    ctx.fill();
    ctx.stroke();

    ctx.fillStyle = '#f8fafc';
    ctx.font = 'bold 15px sans-serif';
    ctx.textAlign = 'center';
    ctx.fillText('Back to Project Top', layout.backButton.x + layout.backButton.w * 0.5, layout.backButton.y + layout.backButton.h * 0.5);

    const saveHovered = this._hoverTarget === 'save';
    ctx.fillStyle = saveHovered ? '#10b981' : '#111827';
    ctx.strokeStyle = saveHovered ? '#a7f3d0' : '#475569';
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.roundRect(layout.saveButton.x, layout.saveButton.y, layout.saveButton.w, layout.saveButton.h, 12);
    ctx.fill();
    ctx.stroke();

    ctx.fillStyle = '#f8fafc';
    ctx.font = 'bold 15px sans-serif';
    ctx.fillText('Save to Browser', layout.saveButton.x + layout.saveButton.w * 0.5, layout.saveButton.y + layout.saveButton.h * 0.5);
  }

  _drawPanels(ctx, layout) {
    ctx.fillStyle = '#0b1220';
    ctx.strokeStyle = '#334155';
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.roundRect(layout.panel.x, layout.panel.y, layout.panel.w, layout.panel.h, 18);
    ctx.fill();
    ctx.stroke();

    [layout.nav, layout.list, layout.editor].forEach(rect => {
      ctx.fillStyle = '#111827';
      ctx.strokeStyle = '#334155';
      ctx.lineWidth = 1.5;
      ctx.beginPath();
      ctx.roundRect(rect.x, rect.y, rect.w, rect.h, 14);
      ctx.fill();
      ctx.stroke();
    });
  }

  _drawNav(ctx, rect) {
    ctx.textAlign = 'left';
    ctx.textBaseline = 'middle';
    ctx.fillStyle = '#f8fafc';
    ctx.font = 'bold 18px sans-serif';
    ctx.fillText('Project Variables', rect.x + 18, rect.y + 24);

    ctx.fillStyle = '#64748b';
    ctx.font = '12px sans-serif';
    ctx.fillText('Left navigation for future local scopes', rect.x + 18, rect.y + 46);

    this._navItems.forEach(item => {
      const hovered = this._hoverTarget === item.id;
      const selected = this._selectedScopePath === item.scopePath;
      ctx.fillStyle = selected ? '#0f2740' : hovered ? '#15243a' : '#0f172a';
      ctx.strokeStyle = selected ? '#38bdf8' : hovered ? '#475569' : '#1f2937';
      ctx.lineWidth = selected ? 2 : 1;
      ctx.beginPath();
      ctx.roundRect(item.rect.x, item.rect.y, item.rect.w, item.rect.h, 10);
      ctx.fill();
      ctx.stroke();

      ctx.fillStyle = selected ? '#e0f2fe' : '#cbd5e1';
      ctx.font = selected ? 'bold 14px sans-serif' : '14px sans-serif';
      ctx.fillText(item.label, item.rect.x + 14, item.rect.y + item.rect.h * 0.5);
    });
  }

  _drawList(ctx, rect, entries) {
    this._listPanelRect = rect;

    ctx.textAlign = 'left';
    ctx.textBaseline = 'middle';
    ctx.fillStyle = '#f8fafc';
    ctx.font = 'bold 18px sans-serif';
    ctx.fillText(this._getSelectedScopeLabel(), rect.x + 18, rect.y + 24);

    ctx.fillStyle = '#94a3b8';
    ctx.font = '12px sans-serif';
    ctx.fillText(`${entries.length} variables`, rect.x + 18, rect.y + 46);

    this._drawButton(ctx, this._findActionButton('add'), 'Add Variable');
    this._drawButton(ctx, this._findActionButton('delete'), 'Delete');

    if (!this._listItems.length) {
      ctx.fillStyle = '#64748b';
      ctx.font = '14px sans-serif';
      ctx.fillText('No variables yet. Use Add Variable.', rect.x + 18, rect.y + 94);
      return;
    }

    this._listItems.forEach(item => {
      const hovered = this._hoverTarget === item.id;
      const selected = this._selectedVariableName === item.name;
      ctx.fillStyle = selected ? '#0f2740' : hovered ? '#15243a' : '#0b1220';
      ctx.strokeStyle = selected ? '#38bdf8' : hovered ? '#475569' : '#1f2937';
      ctx.lineWidth = selected ? 2 : 1;
      ctx.beginPath();
      ctx.roundRect(item.rect.x, item.rect.y, item.rect.w, item.rect.h, 10);
      ctx.fill();
      ctx.stroke();

      ctx.fillStyle = '#f8fafc';
      ctx.font = 'bold 14px sans-serif';
      ctx.fillText(item.name, item.rect.x + 12, item.rect.y + 16);

      ctx.fillStyle = '#93c5fd';
      ctx.font = '12px sans-serif';
      ctx.fillText(item.definition.type || 'unknown', item.rect.x + 12, item.rect.y + 36);

      ctx.fillStyle = '#94a3b8';
      ctx.fillText(this._summarizeValue(item.definition.initialValue), item.rect.x + 92, item.rect.y + 36);
    });
  }

  _drawEditor(ctx, rect, definition) {
    ctx.textAlign = 'left';
    ctx.textBaseline = 'middle';
    ctx.fillStyle = '#f8fafc';
    ctx.font = 'bold 18px sans-serif';
    ctx.fillText('Variable Details', rect.x + 18, rect.y + 24);

    const statusColor = this._statusTone === 'error'
      ? '#fca5a5'
      : this._statusTone === 'info'
        ? '#93c5fd'
        : '#94a3b8';
    ctx.fillStyle = statusColor;
    ctx.font = '12px sans-serif';
    ctx.fillText(this._statusMessage, rect.x + 18, rect.y + 46);

    if (!definition || !this._selectedVariableName) {
      ctx.fillStyle = '#64748b';
      ctx.font = '14px sans-serif';
      ctx.fillText('Select a variable from the list or create a new one.', rect.x + 18, rect.y + 92);
      this._drawButton(ctx, this._findActionButton('edit-name'), 'Edit Name', true);
      this._drawButton(ctx, this._findActionButton('edit-type'), 'Edit Type', true);
      this._drawButton(ctx, this._findActionButton('edit-value'), 'Edit Initial Value', true);
      this._drawButton(ctx, this._findActionButton('edit-description'), 'Edit Description', true);
      return;
    }

    const fields = [
      { label: 'Name', value: this._selectedVariableName },
      { label: 'Type', value: definition.type || 'string' },
      { label: 'Initial Value', value: this._summarizeValue(definition.initialValue, true) },
      { label: 'Description', value: definition.description || '(empty)' },
    ];
    fields.forEach((field, index) => {
      const y = rect.y + 92 + index * 74;
      ctx.fillStyle = '#94a3b8';
      ctx.font = '12px sans-serif';
      ctx.fillText(field.label, rect.x + 18, y);

      ctx.fillStyle = '#e2e8f0';
      ctx.font = 'bold 15px sans-serif';
      this._drawWrappedText(ctx, field.value, rect.x + 18, y + 24, rect.w - 36, 20, 2);
    });

    this._drawButton(ctx, this._findActionButton('edit-name'), 'Edit Name');
    this._drawButton(ctx, this._findActionButton('edit-type'), 'Edit Type');
    this._drawButton(ctx, this._findActionButton('edit-value'), 'Edit Initial Value');
    this._drawButton(ctx, this._findActionButton('edit-description'), 'Edit Description');

    ctx.fillStyle = '#475569';
    ctx.font = '12px sans-serif';
    ctx.fillText('Project save/export remains in ProjectTopScene during Phase 4.', rect.x + 18, rect.y + rect.h - 24);
  }

  _drawWrappedText(ctx, text, x, y, maxWidth, lineHeight, maxLines) {
    const source = String(text || '');
    const words = source.split(/\s+/);
    const lines = [];
    let current = '';
    for (const word of words) {
      const candidate = current ? `${current} ${word}` : word;
      if (ctx.measureText(candidate).width <= maxWidth || !current) {
        current = candidate;
      } else {
        lines.push(current);
        current = word;
      }
      if (lines.length >= maxLines) break;
    }
    if (lines.length < maxLines && current) lines.push(current);
    const rendered = lines.slice(0, maxLines);
    rendered.forEach((line, index) => {
      const suffix = index === maxLines - 1 && rendered.join(' ') !== source ? ' ...' : '';
      ctx.fillText(`${line}${suffix}`, x, y + index * lineHeight);
    });
  }

  _drawButton(ctx, button, label, forceDisabled = false) {
    if (!button) return;
    const disabled = forceDisabled || !!button.disabled;
    const hovered = !disabled && this._hoverTarget === button.id;
    ctx.fillStyle = disabled ? '#1f2937' : hovered ? '#0ea5e9' : '#111827';
    ctx.strokeStyle = disabled ? '#334155' : hovered ? '#bae6fd' : '#475569';
    ctx.lineWidth = 1.5;
    ctx.beginPath();
    ctx.roundRect(button.rect.x, button.rect.y, button.rect.w, button.rect.h, 10);
    ctx.fill();
    ctx.stroke();

    ctx.fillStyle = disabled ? '#64748b' : '#f8fafc';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.font = 'bold 13px sans-serif';
    ctx.fillText(label, button.rect.x + button.rect.w * 0.5, button.rect.y + button.rect.h * 0.5);
  }

  _buildNavItems(rect) {
    const labels = [
      { scopePath: 'system.fixed', label: 'system.fixed' },
      { scopePath: 'system.persistent', label: 'system.persistent' },
      { scopePath: 'user.fixed', label: 'user.fixed' },
      { scopePath: 'user.persistent', label: 'user.persistent' },
    ];
    return labels.map((item, index) => ({
      id: `nav:${item.scopePath}`,
      scopePath: item.scopePath,
      label: item.label,
      rect: {
        x: rect.x + 14,
        y: rect.y + 72 + index * 46,
        w: rect.w - 28,
        h: 34,
      },
    }));
  }

  _buildListItems(rect, entries) {
    const top = rect.y + 96;
    const rowH = 50;
    const gap = 8;
    const availableH = rect.h - 112;
    this._listVisibleRows = Math.max(1, Math.floor((availableH + gap) / (rowH + gap)));
    const maxOffset = this._getMaxListScrollOffset(entries.length, this._listVisibleRows);
    if (this._listScrollOffset > maxOffset) this._listScrollOffset = maxOffset;
    return entries
      .slice(this._listScrollOffset, this._listScrollOffset + this._listVisibleRows)
      .map(([name, definition], index) => ({
        id: `var:${name}`,
        name,
        definition,
        rect: {
          x: rect.x + 12,
          y: top + index * (rowH + gap),
          w: rect.w - 24,
          h: rowH,
        },
      }));
  }

  _buildActionButtons(layout, hasSelection) {
    const listTop = layout.list.y + 18;
    const listRight = layout.list.x + layout.list.w;
    const editorTop = layout.editor.y + layout.editor.h - 188;
    return [
      {
        id: 'add',
        rect: { x: listRight - 214, y: listTop, w: 120, h: 30 },
        disabled: false,
      },
      {
        id: 'delete',
        rect: { x: listRight - 86, y: listTop, w: 74, h: 30 },
        disabled: !hasSelection,
      },
      {
        id: 'edit-name',
        rect: { x: layout.editor.x + 18, y: editorTop, w: 168, h: 34 },
        disabled: !hasSelection,
      },
      {
        id: 'edit-type',
        rect: { x: layout.editor.x + 196, y: editorTop, w: 168, h: 34 },
        disabled: !hasSelection,
      },
      {
        id: 'edit-value',
        rect: { x: layout.editor.x + 18, y: editorTop + 46, w: 168, h: 34 },
        disabled: !hasSelection,
      },
      {
        id: 'edit-description',
        rect: { x: layout.editor.x + 196, y: editorTop + 46, w: 168, h: 34 },
        disabled: !hasSelection,
      },
    ];
  }

  _findActionButton(id) {
    return this._actionButtons.find(button => button.id === id) || null;
  }

  _getSelectedScopeLabel() {
    return `Project Variables / ${this._selectedScopePath}`;
  }

  _ensureProjectGlobalVariables() {
    if (!this._appData?.currentProject) return;
    this._appData.currentProject.globalVariables = ProjectData.normalizeGlobalVariables(this._appData.currentProject.globalVariables);
  }

  _getProjectGlobalVariables() {
    this._ensureProjectGlobalVariables();
    return this._appData?.currentProject?.globalVariables || ProjectData.createDefaultGlobalVariables();
  }

  _getSelectedBucket() {
    const [scope, tier] = this._selectedScopePath.split('.');
    const root = this._getProjectGlobalVariables();
    return root?.[scope]?.[tier] || {};
  }

  _getSelectedDefinition() {
    const bucket = this._getSelectedBucket();
    return this._selectedVariableName ? bucket[this._selectedVariableName] || null : null;
  }

  _syncSelection() {
    const bucket = this._getSelectedBucket();
    if (this._selectedVariableName && bucket[this._selectedVariableName]) return;
    const names = Object.keys(bucket);
    this._selectedVariableName = names[0] || '';
  }

  _selectScope(scopePath) {
    this._selectedScopePath = scopePath;
    this._listScrollOffset = 0;
    this._syncSelection();
    this._statusTone = 'muted';
    this._statusMessage = `Selected ${scopePath}`;
  }

  _markDefinitionChanged(message) {
    if (this._appData?.currentProject) {
      this._appData.currentProject.touch();
    }
    this._appData?.projectSession?.markDirty();
    this._appData?.resetRuntimeGlobalVariables?.();
    this._statusTone = 'info';
    this._statusMessage = message;
  }

  _addVariable() {
    const bucket = this._getSelectedBucket();
    const rawName = window.prompt('Variable name', 'newVariable');
    if (rawName == null) return;
    const name = rawName.trim();
    if (!name) {
      this._statusTone = 'error';
      this._statusMessage = 'Variable name is required';
      return;
    }
    if (bucket[name]) {
      this._statusTone = 'error';
      this._statusMessage = `Variable already exists: ${name}`;
      return;
    }

    const rawType = window.prompt('Variable type (string|number|boolean|json)', 'string');
    if (rawType == null) return;
    const type = this._normalizeType(rawType);
    if (!type) {
      this._statusTone = 'error';
      this._statusMessage = 'Unsupported variable type';
      return;
    }

    bucket[name] = {
      type,
      initialValue: this._getDefaultInitialValueForType(type),
      description: '',
    };
    this._selectedVariableName = name;
    this._markDefinitionChanged(`Added ${name} to ${this._selectedScopePath}`);
  }

  _deleteSelectedVariable() {
    const bucket = this._getSelectedBucket();
    const name = this._selectedVariableName;
    if (!name || !bucket[name]) return;
    if (!window.confirm(`Delete variable "${name}"?`)) return;
    delete bucket[name];
    this._syncSelection();
    this._markDefinitionChanged(`Deleted ${name}`);
  }

  _editSelectedName() {
    const bucket = this._getSelectedBucket();
    const name = this._selectedVariableName;
    const definition = this._getSelectedDefinition();
    if (!name || !definition) return;
    const rawName = window.prompt('Variable name', name);
    if (rawName == null) return;
    const nextName = rawName.trim();
    if (!nextName) {
      this._statusTone = 'error';
      this._statusMessage = 'Variable name is required';
      return;
    }
    if (nextName !== name && bucket[nextName]) {
      this._statusTone = 'error';
      this._statusMessage = `Variable already exists: ${nextName}`;
      return;
    }
    if (nextName === name) return;
    bucket[nextName] = definition;
    delete bucket[name];
    this._selectedVariableName = nextName;
    this._markDefinitionChanged(`Renamed ${name} to ${nextName}`);
  }

  _editSelectedType() {
    const definition = this._getSelectedDefinition();
    if (!definition) return;
    const rawType = window.prompt('Variable type (string|number|boolean|json)', definition.type || 'string');
    if (rawType == null) return;
    const nextType = this._normalizeType(rawType);
    if (!nextType) {
      this._statusTone = 'error';
      this._statusMessage = 'Unsupported variable type';
      return;
    }
    definition.type = nextType;
    definition.initialValue = this._coerceValueForType(definition.initialValue, nextType);
    this._markDefinitionChanged(`Updated type for ${this._selectedVariableName}`);
  }

  _editSelectedInitialValue() {
    const definition = this._getSelectedDefinition();
    if (!definition) return;
    const currentText = this._stringifyInitialValue(definition.initialValue, definition.type);
    const rawValue = window.prompt(`Initial value for type ${definition.type}`, currentText);
    if (rawValue == null) return;
    const parsed = this._parseInitialValue(rawValue, definition.type);
    if (!parsed.ok) {
      this._statusTone = 'error';
      this._statusMessage = parsed.message;
      return;
    }
    definition.initialValue = parsed.value;
    this._markDefinitionChanged(`Updated initial value for ${this._selectedVariableName}`);
  }

  _editSelectedDescription() {
    const definition = this._getSelectedDefinition();
    if (!definition) return;
    const raw = window.prompt('Description', definition.description || '');
    if (raw == null) return;
    definition.description = raw;
    this._markDefinitionChanged(`Updated description for ${this._selectedVariableName}`);
  }

  _normalizeType(rawType) {
    const value = typeof rawType === 'string' ? rawType.trim() : '';
    return value === 'string' || value === 'number' || value === 'boolean' || value === 'json'
      ? value
      : '';
  }

  _getDefaultInitialValueForType(type) {
    switch (type) {
      case 'number':
        return 0;
      case 'boolean':
        return false;
      case 'json':
        return {};
      case 'string':
      default:
        return '';
    }
  }

  _coerceValueForType(value, type) {
    if (type === 'string') return typeof value === 'string' ? value : String(value ?? '');
    if (type === 'number') return Number.isFinite(Number(value)) ? Number(value) : 0;
    if (type === 'boolean') return !!value;
    if (type === 'json') {
      if (value && typeof value === 'object') return this._cloneJsonLikeValue(value);
      return { value };
    }
    return value;
  }

  _cloneJsonLikeValue(value) {
    if (Array.isArray(value)) return value.map(item => this._cloneJsonLikeValue(item));
    if (value && typeof value === 'object') {
      const cloned = {};
      Object.entries(value).forEach(([key, child]) => {
        cloned[key] = this._cloneJsonLikeValue(child);
      });
      return cloned;
    }
    return value;
  }

  _stringifyInitialValue(value, type) {
    if (type === 'json') {
      try {
        return JSON.stringify(value, null, 2);
      } catch (_err) {
        return '{}';
      }
    }
    if (type === 'boolean') return value ? 'true' : 'false';
    if (type === 'number') return String(Number(value) || 0);
    return String(value ?? '');
  }

  _parseInitialValue(rawValue, type) {
    if (type === 'string') return { ok: true, value: rawValue };
    if (type === 'number') {
      const value = Number(rawValue);
      if (!Number.isFinite(value)) return { ok: false, message: 'Initial value must be a valid number' };
      return { ok: true, value };
    }
    if (type === 'boolean') {
      const normalized = String(rawValue).trim().toLowerCase();
      if (normalized === 'true') return { ok: true, value: true };
      if (normalized === 'false') return { ok: true, value: false };
      return { ok: false, message: 'Initial value must be true or false' };
    }
    if (type === 'json') {
      try {
        return { ok: true, value: JSON.parse(rawValue) };
      } catch (_err) {
        return { ok: false, message: 'Initial value must be valid JSON' };
      }
    }
    return { ok: false, message: 'Unsupported variable type' };
  }

  _summarizeValue(value, expanded = false) {
    if (typeof value === 'string') {
      return expanded ? (value || '(empty)') : value.length > 22 ? `${value.slice(0, 22)}...` : value || '(empty)';
    }
    if (typeof value === 'number' || typeof value === 'boolean') {
      return String(value);
    }
    if (value == null) {
      return 'null';
    }
    try {
      const json = JSON.stringify(value);
      if (expanded) return json;
      return json.length > 28 ? `${json.slice(0, 28)}...` : json;
    } catch (_err) {
      return '[unserializable]';
    }
  }

  _getMaxListScrollOffset(count, visibleRows) {
    return Math.max(0, count - Math.max(1, visibleRows | 0));
  }

  _goBack() {
    this._appData?.changeScene(new ProjectTopScene());
  }

  _onMouseMove(e) {
    let hover = '';
    if (this._backButtonRect && this._inRect(e.x, e.y, this._backButtonRect)) {
      hover = 'back';
    } else if (this._saveButtonRect && this._inRect(e.x, e.y, this._saveButtonRect)) {
      hover = 'save';
    } else {
      const nav = this._navItems.find(item => this._inRect(e.x, e.y, item.rect));
      if (nav) {
        hover = nav.id;
      } else {
        const listItem = this._listItems.find(item => this._inRect(e.x, e.y, item.rect));
        if (listItem) {
          hover = listItem.id;
        } else {
          const button = this._actionButtons.find(item => !item.disabled && this._inRect(e.x, e.y, item.rect));
          if (button) hover = button.id;
        }
      }
    }
    this._hoverTarget = hover;
  }

  _onMouseDown(e) {
    if (e.button !== 0) return;
    if (this._hoverTarget === 'back') {
      this._goBack();
      return;
    }
    if (this._hoverTarget === 'save') {
      this._saveProject();
      return;
    }

    const navItem = this._navItems.find(item => item.id === this._hoverTarget);
    if (navItem) {
      this._selectScope(navItem.scopePath);
      return;
    }

    const listItem = this._listItems.find(item => item.id === this._hoverTarget);
    if (listItem) {
      this._selectedVariableName = listItem.name;
      this._statusTone = 'muted';
      this._statusMessage = `Selected ${listItem.name}`;
      return;
    }

    switch (this._hoverTarget) {
      case 'add':
        this._addVariable();
        return;
      case 'delete':
        this._deleteSelectedVariable();
        return;
      case 'edit-name':
        this._editSelectedName();
        return;
      case 'edit-type':
        this._editSelectedType();
        return;
      case 'edit-value':
        this._editSelectedInitialValue();
        return;
      case 'edit-description':
        this._editSelectedDescription();
        return;
      default:
        break;
    }
  }

  _onWheel(e) {
    if (!this._listPanelRect || !this._inRect(e.x, e.y, this._listPanelRect)) return;
    const count = Object.keys(this._getSelectedBucket()).length;
    const maxOffset = this._getMaxListScrollOffset(count, this._listVisibleRows);
    if (maxOffset <= 0) return;

    const nextOffset = e.deltaY > 0
      ? Math.min(maxOffset, this._listScrollOffset + 1)
      : e.deltaY < 0
        ? Math.max(0, this._listScrollOffset - 1)
        : this._listScrollOffset;
    if (nextOffset === this._listScrollOffset) return;
    this._listScrollOffset = nextOffset;
    this._hoverTarget = '';
  }

  _onKeyDown(e) {
    if ((e.ctrlKey || e.metaKey) && (e.key === 's' || e.key === 'S')) {
      e.preventDefault();
      this._saveProject();
      return;
    }
    if (e.key === 'Escape') {
      this._goBack();
      return;
    }
    if (e.key === 'Delete' || e.key === 'Backspace') {
      this._deleteSelectedVariable();
      return;
    }
    if (e.key === 'a' || e.key === 'A') {
      this._addVariable();
    }
  }

  _inRect(x, y, rect) {
    return x >= rect.x && y >= rect.y && x < rect.x + rect.w && y < rect.y + rect.h;
  }

  _saveProject() {
    if (!this._appData?.currentProject || !this._appData?.projectSession) return;
    const validation = this._validateProjectGlobalVariables();
    if (!validation.ok) {
      this._statusTone = 'error';
      this._statusMessage = validation.message;
      if (validation.scopePath) {
        this._selectedScopePath = validation.scopePath;
        this._selectedVariableName = validation.variableName || this._selectedVariableName;
        this._listScrollOffset = 0;
      }
      return;
    }

    this._appData.syncEditorStateToProjectSession();
    ProjectBrowserStorage.saveProject(
      this._appData.currentProject,
      this._appData.projectSession,
      this._appData.palette,
    )
      .then(() => {
        this._appData.projectSession?.clearDirty();
        this._statusTone = 'info';
        this._statusMessage = 'Saved to browser storage';
      })
      .catch(err => {
        this._statusTone = 'error';
        this._statusMessage = err?.message || 'Browser save failed';
      });
  }

  _validateProjectGlobalVariables() {
    const validation = ProjectData.validateGlobalVariables(this._getProjectGlobalVariables());
    if (!validation.ok) return validation;
    return { ok: true, message: '' };
  }
}
