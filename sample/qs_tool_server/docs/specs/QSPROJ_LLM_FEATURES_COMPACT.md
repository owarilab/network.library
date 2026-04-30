# qsproj Compact Feature List for Small LLMs

Use this when generating `.qsproj` with a small model and small context.

## Goal

Generate valid, minimal `.qsproj` JSON for `sample/qs_tool_server`.

## Hard Rules

1. Root must contain `format`, `version`, `project`, `session`.
2. `format` must be `qsproj`.
3. Root `version` must be `1`.
4. `project.assets.playUnits[*].objects` must be top-level inside each playUnit. Do not nest them under `storage`.
5. Every object should have a `Transform` component.
6. At least one object should provide a valid camera path:
   `Transform` + `Camera` + `PlaySettings.defaultCameraObjectId`.
7. All IDs must be unique.
8. Output pure JSON only.

## Minimal Root Shape

```json
{
  "format": "qsproj",
  "version": 1,
  "project": {
    "id": "proj_xxx",
    "version": 1,
    "name": "Project",
    "createdAt": 1777436608074,
    "updatedAt": 1777436608074,
    "settings": {
      "defaultChipWidth": 16,
      "defaultChipHeight": 16
    },
    "globalVariables": {
      "version": 1,
      "system": { "fixed": {}, "persistent": {} },
      "user": { "fixed": {}, "persistent": {} }
    },
    "assets": {
      "pixelDocuments": [],
      "tilesets": [],
      "maps": [],
      "playUnits": []
    }
  },
  "session": {
    "projectId": "proj_xxx",
    "dirty": false,
    "currentScene": "PlayUnitEditorScene",
    "activeDocumentRef": { "type": "playUnit", "id": "pu_xxx" },
    "editorState": {
      "activeTool": "pencil",
      "foreColor": 4278190080,
      "backColor": 4294967295,
      "editMode": "free",
      "selectedChip": { "col": 0, "row": 0 }
    }
  }
}
```

## ID Prefixes

- project: `proj_...`
- pixel document: `px_...`
- playUnit: `pu_...`
- object: `obj_...`

## Minimal PlayUnit

```json
{
  "id": "pu_scene_1",
  "type": "playUnit",
  "name": "Scene",
  "description": "short text",
  "objects": []
}
```

## Core Components

### Transform

Required for almost all objects.

```json
{
  "type": "Transform",
  "enabled": true,
  "data": { "x": 0, "y": 0, "z": 0, "rotation": 0, "scaleX": 1, "scaleY": 1 }
}
```

### Camera

Used on the main camera object.

```json
{
  "type": "Camera",
  "enabled": true,
  "data": {
    "zoom": 1,
    "viewportX": 0,
    "viewportY": 0,
    "viewportWidth": 800,
    "viewportHeight": 600,
    "followTargetObjectId": "",
    "followLerp": 1
  }
}
```

### PlaySettings

Used with camera object.

```json
{
  "type": "PlaySettings",
  "enabled": true,
  "data": { "defaultCameraObjectId": "obj_camera_1" }
}
```

### Text

```json
{
  "type": "Text",
  "enabled": true,
  "data": {
    "text": "Hello",
    "font": "20px sans-serif",
    "color": "#e2e8f0",
    "alpha": 1,
    "align": "center",
    "baseline": "middle",
    "wrap": false,
    "maxWidth": 0,
    "lineHeight": 24,
    "strokeColor": "",
    "strokeWidth": 0,
    "backgroundColor": "",
    "padding": 0
  }
}
```

### Rectangle

```json
{
  "type": "Rectangle",
  "enabled": true,
  "data": {
    "shape": "rectangle",
    "width": 160,
    "height": 48,
    "fillColor": "#1e293b",
    "fillAlpha": 1,
    "strokeColor": "#38bdf8",
    "strokeWidth": 2,
    "strokeAlpha": 1,
    "rotation": 0,
    "originX": 0,
    "originY": 0,
    "sides": 4,
    "points": 5,
    "innerRadius": 0.4
  }
}
```

### Collider

```json
{
  "type": "Collider",
  "enabled": true,
  "data": { "shape": "rect", "offsetX": 0, "offsetY": 0, "width": 160, "height": 48, "isTrigger": true }
}
```

### Trigger

Supported `triggerOn` values used in current docs/examples:
- `click`
- `overlap`
- `pointerEnter`
- `pointerLeave`
- `pointerDown`
- `pointerUp`

```json
{
  "type": "Trigger",
  "enabled": true,
  "data": { "eventId": "ev_click", "triggerOn": "click", "once": false, "targetObjectId": "" }
}
```

### EventAction

Common fields:

```json
{
  "type": "EventAction",
  "enabled": true,
  "data": {
    "listenTo": "ev_click",
    "action": "setProperty"
  }
}
```

Supported `action` values:

- `setProperty`
- `setEnabled`
- `playTween`
- `fireEvent`
- `requestPlayUnit`
- `returnPlayUnit`

Action-specific fields:

- `setProperty`: `targetObjectId`, `componentType`, `property`, `value`
- `setEnabled`: `targetObjectId`, `enabled`
- `playTween`: `targetObjectId`, `componentType`, `property`, `tweenDuration`, `tweenFrom`, `tweenTo`, `tweenEasing`
- `fireEvent`: `eventId`
- `requestPlayUnit`: `playUnitId`
- `returnPlayUnit`: no extra fields

## Global Variables

Use only if needed. Keep minimal.

Current useful system keys:

- `system.fixed.startupPlayUnitId`
- `system.fixed.requestedPlayUnitId`
- `system.fixed.currentPlayUnitId`
- `system.fixed.returnPlayUnitId`
- `system.fixed.isPaused`
- `system.persistent.masterVolume`

Variable definition shape:

```json
{
  "startupPlayUnitId": {
    "type": "string",
    "initialValue": "pu_scene_1",
    "description": "first play unit"
  }
}
```

## Best Practices for Small LLMs

1. Prefer one playUnit first.
2. Prefer only these components: `Transform`, `Camera`, `PlaySettings`, `Text`, `Rectangle`, `Collider`, `Trigger`, `EventAction`.
3. Keep `pixelDocuments`, `tilesets`, and `maps` empty unless required.
4. Use short ASCII names.
5. Reuse known good component shapes exactly.
6. If unsure, copy the minimal camera object and one text object.

## Fast Validation

```bash
python3 -m json.tool file.qsproj > /dev/null
```