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

## Example 4: Global Variable Read/Write

A minimal project that verifies `user.fixed` / `user.persistent` runtime access.

```json
{
  "version": 1,
  "project": {
    "id": "proj_global_var_1",
    "name": "Global Variable Check",
    "description": "verify user fixed/persistent runtime access",
    "settings": { "defaultChipWidth": 16, "defaultChipHeight": 16 },
    "globalVariables": {
      "version": 1,
      "system": {
        "fixed": {
          "startupPlayUnitId": { "type": "string", "initialValue": "pu_global_var_1", "description": "startup playunit" },
          "requestedPlayUnitId": { "type": "string", "initialValue": "", "description": "requested playunit" },
          "currentPlayUnitId": { "type": "string", "initialValue": "", "description": "current playunit" },
          "returnPlayUnitId": { "type": "string", "initialValue": "", "description": "return playunit" },
          "isPaused": { "type": "boolean", "initialValue": false, "description": "pause flag" }
        },
        "persistent": {
          "masterVolume": { "type": "number", "initialValue": 1, "description": "volume" }
        }
      },
      "user": {
        "fixed": {
          "message": { "type": "string", "initialValue": "start", "description": "message text" },
          "counter": { "type": "number", "initialValue": 1, "description": "counter" }
        },
        "persistent": {
          "score": { "type": "number", "initialValue": 10, "description": "score" }
        }
      }
    },
    "assets": {
      "pixelDocuments": [],
      "tilesets": [],
      "maps": [],
      "playUnits": [
        {
          "id": "pu_global_var_1",
          "type": "playUnit",
          "name": "GlobalVarCheck",
          "description": "read/write global variables",
          "objects": [
            {
              "id": "obj_text_1",
              "name": "Text",
              "components": [
                { "type": "Transform", "enabled": true, "data": { "x": 16, "y": 16, "z": 0, "rotation": 0, "scaleX": 1, "scaleY": 1 } },
                { "type": "Text", "enabled": true, "data": { "text": "score: 10", "font": "24px sans-serif", "color": "#ffffff", "alpha": 1, "align": "left", "baseline": "top", "wrap": false, "maxWidth": 0, "lineHeight": 28, "strokeColor": "", "strokeWidth": 0, "backgroundColor": "", "padding": 0 } }
              ]
            },
            {
              "id": "obj_btn_1",
              "name": "Button",
              "components": [
                { "type": "Transform", "enabled": true, "data": { "x": 16, "y": 64, "z": 0, "rotation": 0, "scaleX": 1, "scaleY": 1 } },
                { "type": "Rectangle", "enabled": true, "data": { "shape": "rectangle", "width": 180, "height": 44, "fillColor": "#1e293b", "fillAlpha": 1, "strokeColor": "#38bdf8", "strokeWidth": 2, "strokeAlpha": 1, "rotation": 0, "originX": 0, "originY": 0, "sides": 4, "points": 5, "innerRadius": 0.4 } },
                { "type": "Collider", "enabled": true, "data": { "shape": "rect", "offsetX": 0, "offsetY": 0, "width": 180, "height": 44, "isTrigger": true } },
                { "type": "Trigger", "enabled": true, "data": { "eventId": "ev_add_score", "triggerOn": "click", "once": false, "targetObjectId": "" } },
                { "type": "EventAction", "enabled": true, "data": { "listenTo": "ev_add_score", "action": "setGlobalVariable", "variablePath": "user.persistent.score", "valueSource": "literal", "value": 11 } },
                { "type": "EventAction", "enabled": true, "data": { "listenTo": "ev_add_score", "action": "setProperty", "targetObjectId": "obj_text_1", "componentType": "Text", "property": "text", "value": "score: 11" } }
              ]
            }
          ]
        }
      ]
    }
  },
  "session": {
    "projectId": "proj_global_var_1",
    "dirty": false,
    "currentScene": "PlayUnitEditorScene",
    "activeDocumentRef": { "type": "playUnit", "id": "pu_global_var_1" },
    "editorState": { "activeTool": "pencil", "foreColor": 4278190080, "backColor": 4294967295, "editMode": "free", "selectedChip": { "col": 0, "row": 0 } }
  }
}
```

## Example 4: Text Template Variables

Text components can use `${variable_path}` syntax to dynamically display global variable values. Templates are evaluated each frame, so changes to variables are reflected in real-time.

```json
{
  "format": "qsproj",
  "version": 1,
  "project": {
    "id": "proj_template_vars_1",
    "version": 1,
    "name": "TemplateVariables",
    "createdAt": 1777436608074,
    "updatedAt": 1777436608074,
    "settings": { "defaultChipWidth": 16, "defaultChipHeight": 16 },
    "globalVariables": {
      "version": 1,
      "system": { "fixed": { "startupPlayUnitId": { "type": "string", "initialValue": "pu_template_1" }, "isPaused": { "type": "boolean", "initialValue": false } }, "persistent": {} },
      "user": { "fixed": {}, "persistent": { "score": { "type": "number", "initialValue": 100 }, "hp": { "type": "number", "initialValue": 50 }, "level": { "type": "number", "initialValue": 3 } } }
    },
    "assets": {
      "pixelDocuments": [],
      "tilesets": [],
      "maps": [],
      "playUnits": [
        {
          "id": "pu_template_1",
          "type": "playUnit",
          "name": "TemplateExample",
          "description": "Demonstrates text template variable syntax",
          "objects": [
            {
              "id": "obj_camera_1",
              "name": "Main Camera",
              "components": [
                { "type": "Transform", "enabled": true, "data": { "x": 0, "y": 0, "z": 0, "rotation": 0, "scaleX": 1, "scaleY": 1 } },
                { "type": "Camera", "enabled": true, "data": { "zoom": 1, "viewportX": 0, "viewportY": 0, "viewportWidth": 800, "viewportHeight": 600, "followTargetObjectId": "", "followLerp": 1 } },
                { "type": "PlaySettings", "enabled": true, "data": { "defaultCameraObjectId": "obj_camera_1" } }
              ]
            },
            {
              "id": "obj_single_var_1",
              "name": "Single Variable",
              "components": [
                { "type": "Transform", "enabled": true, "data": { "x": 50, "y": 50, "z": 0, "rotation": 0, "scaleX": 1, "scaleY": 1 } },
                { "type": "Text", "enabled": true, "data": { "text": "Score: ${user.persistent.score}", "font": "24px sans-serif", "color": "#38bdf8", "alpha": 1, "align": "left", "baseline": "top", "wrap": false, "maxWidth": 0, "lineHeight": 28, "strokeColor": "", "strokeWidth": 0, "backgroundColor": "", "padding": 0 } }
              ]
            },
            {
              "id": "obj_multivar_1",
              "name": "Multiple Variables",
              "components": [
                { "type": "Transform", "enabled": true, "data": { "x": 50, "y": 100, "z": 0, "rotation": 0, "scaleX": 1, "scaleY": 1 } },
                { "type": "Text", "enabled": true, "data": { "text": "HP: ${user.persistent.hp} / Level: ${user.persistent.level}", "font": "20px sans-serif", "color": "#10b981", "alpha": 1, "align": "left", "baseline": "top", "wrap": false, "maxWidth": 0, "lineHeight": 24, "strokeColor": "", "strokeWidth": 0, "backgroundColor": "", "padding": 0 } }
              ]
            },
            {
              "id": "obj_multiline_1",
              "name": "Multi-line Template",
              "components": [
                { "type": "Transform", "enabled": true, "data": { "x": 50, "y": 150, "z": 0, "rotation": 0, "scaleX": 1, "scaleY": 1 } },
                { "type": "Text", "enabled": true, "data": { "text": "Status:\nScore: ${user.persistent.score}\nHP: ${user.persistent.hp}\nLevel: ${user.persistent.level}", "font": "16px monospace", "color": "#f59e0b", "alpha": 1, "align": "left", "baseline": "top", "wrap": false, "maxWidth": 0, "lineHeight": 22, "strokeColor": "", "strokeWidth": 0, "backgroundColor": "", "padding": 0 } }
              ]
            },
            {
              "id": "obj_btn_increase_1",
              "name": "Increase Score Button",
              "components": [
                { "type": "Transform", "enabled": true, "data": { "x": 50, "y": 260, "z": 0, "rotation": 0, "scaleX": 1, "scaleY": 1 } },
                { "type": "Rectangle", "enabled": true, "data": { "shape": "rectangle", "width": 200, "height": 44, "fillColor": "#1e293b", "fillAlpha": 1, "strokeColor": "#22c55e", "strokeWidth": 2, "strokeAlpha": 1, "rotation": 0, "originX": 0, "originY": 0, "sides": 4, "points": 5, "innerRadius": 0.4 } },
                { "type": "Collider", "enabled": true, "data": { "shape": "rect", "offsetX": 0, "offsetY": 0, "width": 200, "height": 44, "isTrigger": true } },
                { "type": "Trigger", "enabled": true, "data": { "eventId": "ev_add_score", "triggerOn": "click", "once": false, "targetObjectId": "" } },
                { "type": "EventAction", "enabled": true, "data": { "listenTo": "ev_add_score", "action": "setGlobalVariable", "variablePath": "user.persistent.score", "valueSource": "literal", "value": 10, "op": "add" } }
              ]
            },
            {
              "id": "obj_btn_decrease_hp_1",
              "name": "Decrease HP Button",
              "components": [
                { "type": "Transform", "enabled": true, "data": { "x": 50, "y": 320, "z": 0, "rotation": 0, "scaleX": 1, "scaleY": 1 } },
                { "type": "Rectangle", "enabled": true, "data": { "shape": "rectangle", "width": 200, "height": 44, "fillColor": "#1e293b", "fillAlpha": 1, "strokeColor": "#ef4444", "strokeWidth": 2, "strokeAlpha": 1, "rotation": 0, "originX": 0, "originY": 0, "sides": 4, "points": 5, "innerRadius": 0.4 } },
                { "type": "Collider", "enabled": true, "data": { "shape": "rect", "offsetX": 0, "offsetY": 0, "width": 200, "height": 44, "isTrigger": true } },
                { "type": "Trigger", "enabled": true, "data": { "eventId": "ev_damage", "triggerOn": "click", "once": false, "targetObjectId": "" } },
                { "type": "EventAction", "enabled": true, "data": { "listenTo": "ev_damage", "action": "setGlobalVariable", "variablePath": "user.persistent.hp", "valueSource": "literal", "value": 5, "op": "subtract" } }
              ]
            }
          ]
        }
      ]
    }
  },
  "session": {
    "projectId": "proj_template_vars_1",
    "dirty": false,
    "currentScene": "PlayUnitEditorScene",
    "activeDocumentRef": { "type": "playUnit", "id": "pu_template_1" },
    "editorState": { "activeTool": "pencil", "foreColor": 4278190080, "backColor": 4294967295, "editMode": "free", "selectedChip": { "col": 0, "row": 0 } }
  }
}
```

### Template Variable Syntax

- **Format**: `${variable_path}` where `variable_path` is a dot-separated path to a global variable
- **Scope**: Works in Text component `text` field only
- **Example paths**:
  - `${user.persistent.score}` - user persistent variable
  - `${system.fixed.isPaused}` - system fixed variable
  - `${user.fixed.message}` - user fixed variable
- **Evaluation**: Templates are resolved each frame using `PlayUnitRuntime._resolveTemplateText()`
- **Fallback**: If variable path is not found, the original `${...}` expression is left as-is in the text
- **Multiple variables**: A single text can contain multiple template expressions: `"Score: ${user.persistent.score}, HP: ${user.persistent.hp}"`
- **Multi-line**: Template variables work in multi-line text (with `\n` escape sequences)

## Example 6: Conditional Branching

Multiple conditions with different actions. Evaluates in order (short-circuit).

```json
{
  "id": "obj_battle_ai",
  "name": "Enemy AI Decision",
  "components": [
    {
      "type": "Trigger",
      "enabled": true,
      "data": {
        "eventId": "ev_enemy_turn",
        "triggerOn": "click",
        "once": false
      }
    },
    {
      "type": "Conditional",
      "enabled": true,
      "data": {
        "listenTo": "ev_enemy_turn",
        "branches": [
          {
            "condition": {
              "type": "compare",
              "left": "${user.persistent.enemyHp}",
              "operator": "<",
              "right": 20
            },
            "action": {
              "action": "setGlobalVariable",
              "variablePath": "user.persistent.battleAction",
              "valueSource": "literal",
              "value": "FLEE",
              "op": "set"
            }
          },
          {
            "condition": {
              "type": "compare",
              "left": "${user.persistent.hp}",
              "operator": ">",
              "right": 80
            },
            "action": {
              "action": "setGlobalVariable",
              "variablePath": "user.persistent.battleAction",
              "valueSource": "literal",
              "value": "STRONG_ATTACK",
              "op": "set"
            }
          },
          {
            "condition": {
              "type": "truthy",
              "left": "${user.persistent.hasShield}"
            },
            "action": {
              "action": "setGlobalVariable",
              "variablePath": "user.persistent.battleAction",
              "valueSource": "literal",
              "value": "DEFEND",
              "op": "set"
            }
          }
        ],
        "defaultAction": {
          "action": "setGlobalVariable",
          "variablePath": "user.persistent.battleAction",
          "valueSource": "literal",
          "value": "NORMAL_ATTACK",
          "op": "set"
        }
      }
    }
  ]
}
```

### Conditional Branching Guide

- **Component Type**: `Conditional`
- **Required Fields**: `listenTo` (event ID), `branches` (array of branch objects)
- **Optional Field**: `defaultAction` (action if no branch matches)
- **Evaluation**: Branches are evaluated in order; first matching condition executes its action (short-circuit)
- **No Match**: If no branch condition is true and `defaultAction` exists, it executes; otherwise nothing happens

**Condition Types**:
- **compare**: Numeric comparison (`>`, `>=`, `<`, `<=`, `===`, `!==`)
  - `left`: value or template variable, `operator`: comparison operator, `right`: value
  - Example: `{"type": "compare", "left": "${hp}", "operator": ">", "right": 50}`
- **equals**: String exact match (case-sensitive)
  - `left`: value or template variable, `right`: comparison string
  - Example: `{"type": "equals", "left": "${state}", "right": "defending"}`
- **truthy**: Boolean evaluation of value (including JavaScript truthy rules)
  - `left`: value or template variable (falsy: false, 0, "", null, undefined)
  - Example: `{"type": "truthy", "left": "${hasShield}"}`
- **has**: Object property existence check
  - `left`: object or template variable, `right`: property name
  - Example: `{"type": "has", "left": "${inventory}", "right": "potion"}`
- **exists**: Global variable existence check
  - `left`: variable path string (no `${}` wrapper)
  - Example: `{"type": "exists", "left": "user.persistent.hp"}`




