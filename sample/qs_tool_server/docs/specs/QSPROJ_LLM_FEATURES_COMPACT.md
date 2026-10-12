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
      "characterChipsets": [],
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

### Tilemap

Draws a `maps` asset in PlayTest. Cells whose tile has passable `false` block the Controller. Needs a map and its tileset in the project (see the tool script below).

```json
{
  "type": "Tilemap",
  "enabled": true,
  "data": { "mapAssetId": "map_demo_field", "layerId": "", "scale": 1, "offsetX": 0, "offsetY": 0 }
}
```

- `scale`: integer 1-8 (clamped). Cell size = tile chip size * scale.
- `layerId`: empty = all visible layers.
- Cells outside the map block movement.
- Tilemap objects need a `Transform`. Position is the map's top-left.

### NineSlice

Draws a pixelDocument that has `nineSlice` borders (corners fixed, edges and center stretched). In UICanvas, the UITransform rect is used.

```json
{
  "type": "NineSlice",
  "enabled": true,
  "data": { "pixelDocumentId": "px_msg_frame", "scale": 2, "width": 0, "height": 0, "originX": 0, "originY": 0, "alpha": 1 }
}
```

- `pixelDocuments[*].nineSlice`: `{ "left": 8, "top": 8, "right": 8, "bottom": 8 }` (integers >= 0; left+right <= width; top+bottom <= height). Invalid values are dropped.
- UI objects must not have a `Transform` (the world pass would also draw them).

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
    "padding": 0,
    "revealChars": -1
  }
}
```

- `revealChars`: visible character count for typewriter. `-1` = all text. Empty `text` is allowed (it is drawn as nothing).

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
- `typewriter`
- `fireEvent`
- `requestPlayUnit`
- `returnPlayUnit`
- `sequence`
- `warp`

Action-specific fields:

- `setProperty`: `targetObjectId`, `componentType`, `property`, `value`
- `setEnabled`: `targetObjectId`, `enabled`
- `playTween`: `targetObjectId`, `componentType`, `property`, `tweenDuration` (ms), `tweenFrom`, `tweenTo`, `tweenEasing`, `completeEventId` (optional; fires on natural end only)
- `typewriter`: `targetObjectId`, `componentType` (`Text`), `valueSource` (`literal` with `value`, or `variable` with `valueVariablePath`), `charsPerSecond`, `statusVariablePath` (optional, `typing` / `done`), `completeEventId` (optional)
- `fireEvent`: `eventId`
- `requestPlayUnit`: `playUnitId`
- `returnPlayUnit`: no extra fields
- `warp`: `spawnObjectId` (or numeric `spawnX` and `spawnY`; one is required), `playUnitId` (optional; omit for a move inside the same PlayUnit), `targetObjectId` (optional; default is the player1 Controller object), `facing` (optional `down` / `left` / `right` / `up`; default is the source player's facing)
  - Applied on the next frame. Put it last in a sequence. Only one warp per frame (a second one fails).
  - Switching PlayUnit clears timers, overlap state, pending events and running sequences/tweens/typewriters. A trigger under the arrival point does not fire.
- `sequence`: `steps` (required array; each step has the same shape as an EventAction), `onBusy` (`ignore` default, or `restart`), `completeEventId` (optional; fires once after the last step)
  - Step-only actions: `wait` (`duration` in seconds, default 1), `waitEvent` (`eventId`, optional `timeout` in seconds). Any step with `"waitComplete": true` (`typewriter`, `playTween`, nested `sequence`) waits until that finishes.
  - Other steps run immediately. A failing step stops the sequence (no completion event).
  - `wait` and `waitEvent` are only valid inside `steps`.
  - Can appear wherever an action can: EventAction `action`, Conditional `branches[].action` and `defaultAction`.

## Global Variables

Use only if needed. Keep minimal.

Current useful system keys:

- `system.fixed.startupPlayUnitId`
- `system.fixed.requestedPlayUnitId`
- `system.fixed.currentPlayUnitId`
- `system.fixed.returnPlayUnitId`
- `system.fixed.isPaused`
- Event `system.playUnitEnter` (not a variable): fires once each time a PlayUnit is entered (startup, requestPlayUnit, returnPlayUnit, warp to another PlayUnit). Use it as `listenTo` for fade-in.
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
3. Keep `pixelDocuments`, `tilesets`, `characterChipsets`, and `maps` empty unless required. A `Tilemap` needs one map and its tileset; tileset `storage.data` is QTS base64, so copy `tools/add_tilemap_to_character_demo.py` instead of writing it by hand.
4. Use short ASCII names.
5. Reuse known good component shapes exactly.
6. If unsure, copy the minimal camera object and one text object.

## Fast Validation

```bash
python3 -m json.tool file.qsproj > /dev/null
```