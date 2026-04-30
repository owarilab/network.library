# qsproj Compact Examples for Small LLMs

Short English examples for `.qsproj` generation.

## Example 1: Smallest Useful Project

One playUnit with one camera.

```json
{
  "format": "qsproj",
  "version": 1,
  "project": {
    "id": "proj_min_1",
    "version": 1,
    "name": "Minimal",
    "createdAt": 1777436608074,
    "updatedAt": 1777436608074,
    "settings": { "defaultChipWidth": 16, "defaultChipHeight": 16 },
    "globalVariables": {
      "version": 1,
      "system": { "fixed": {}, "persistent": {} },
      "user": { "fixed": {}, "persistent": {} }
    },
    "assets": {
      "pixelDocuments": [],
      "tilesets": [],
      "maps": [],
      "playUnits": [
        {
          "id": "pu_scene_1",
          "type": "playUnit",
          "name": "Scene",
          "description": "camera only",
          "objects": [
            {
              "id": "obj_camera_1",
              "name": "Main Camera",
              "components": [
                {
                  "type": "Transform",
                  "enabled": true,
                  "data": { "x": 0, "y": 0, "z": 0, "rotation": 0, "scaleX": 1, "scaleY": 1 }
                },
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
                },
                {
                  "type": "PlaySettings",
                  "enabled": true,
                  "data": { "defaultCameraObjectId": "obj_camera_1" }
                }
              ]
            }
          ]
        }
      ]
    }
  },
  "session": {
    "projectId": "proj_min_1",
    "dirty": false,
    "currentScene": "PlayUnitEditorScene",
    "activeDocumentRef": { "type": "playUnit", "id": "pu_scene_1" },
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

## Example 2: Text + Click Button

Clicking the button changes text.

```json
{
  "id": "obj_button_1",
  "name": "Button",
  "components": [
    {
      "type": "Transform",
      "enabled": true,
      "data": { "x": 80, "y": 180, "z": 0, "rotation": 0, "scaleX": 1, "scaleY": 1 }
    },
    {
      "type": "Rectangle",
      "enabled": true,
      "data": {
        "shape": "rectangle",
        "width": 220,
        "height": 56,
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
    },
    {
      "type": "Collider",
      "enabled": true,
      "data": { "shape": "rect", "offsetX": 0, "offsetY": 0, "width": 220, "height": 56, "isTrigger": true }
    },
    {
      "type": "Trigger",
      "enabled": true,
      "data": { "eventId": "ev_click", "triggerOn": "click", "once": false, "targetObjectId": "" }
    },
    {
      "type": "EventAction",
      "enabled": true,
      "data": {
        "listenTo": "ev_click",
        "action": "setProperty",
        "targetObjectId": "obj_label_1",
        "componentType": "Text",
        "property": "text",
        "value": "Clicked"
      }
    }
  ]
}
```

Target text object:

```json
{
  "id": "obj_label_1",
  "name": "Label",
  "components": [
    {
      "type": "Transform",
      "enabled": true,
      "data": { "x": 220, "y": 100, "z": 1, "rotation": 0, "scaleX": 1, "scaleY": 1 }
    },
    {
      "type": "Text",
      "enabled": true,
      "data": {
        "text": "Hello",
        "font": "24px sans-serif",
        "color": "#e2e8f0",
        "alpha": 1,
        "align": "center",
        "baseline": "middle",
        "wrap": false,
        "maxWidth": 0,
        "lineHeight": 28,
        "strokeColor": "",
        "strokeWidth": 0,
        "backgroundColor": "",
        "padding": 0
      }
    }
  ]
}
```

## Example 3: Two-Way PlayUnit Switch

Main -> Overlay via `requestPlayUnit`, Overlay -> Main via `returnPlayUnit`.

Minimal global variables:

```json
{
  "version": 1,
  "system": {
    "fixed": {
      "startupPlayUnitId": { "type": "string", "initialValue": "pu_overlay_1", "description": "first scene" },
      "requestedPlayUnitId": { "type": "string", "initialValue": "", "description": "one-shot request" },
      "currentPlayUnitId": { "type": "string", "initialValue": "", "description": "current scene" },
      "returnPlayUnitId": { "type": "string", "initialValue": "pu_main_1", "description": "return target" },
      "isPaused": { "type": "boolean", "initialValue": false, "description": "pause flag" }
    },
    "persistent": {
      "masterVolume": { "type": "number", "initialValue": 1, "description": "volume" }
    }
  },
  "user": { "fixed": {}, "persistent": {} }
}
```

Outbound action:

```json
{
  "type": "EventAction",
  "enabled": true,
  "data": {
    "listenTo": "ev_open_overlay",
    "action": "requestPlayUnit",
    "playUnitId": "pu_overlay_1"
  }
}
```

Return action:

```json
{
  "type": "EventAction",
  "enabled": true,
  "data": {
    "listenTo": "ev_return_main",
    "action": "returnPlayUnit"
  }
}
```

## Example 4: Fast Prompt Template for LLMs

Use this prompt with a small model:

```text
Generate a valid qsproj JSON.
Rules:
- format=qsproj, version=1
- include project + session
- include one playUnit with objects at top-level
- include one camera object with Transform + Camera + PlaySettings
- use only components: Transform, Text, Rectangle, Collider, Trigger, EventAction
- keep pixelDocuments/tilesets/maps empty unless needed
- output JSON only
Task:
Create a small scene with a title text and one clickable button that changes the title text.
```

## Example 5: What to Avoid

Bad patterns:

1. Nesting `objects` inside `storage.data`.
2. Missing `PlaySettings.defaultCameraObjectId`.
3. Missing `Transform`.
4. Using unknown component types.
5. Returning markdown or comments instead of raw JSON.