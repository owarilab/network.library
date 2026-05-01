#!/usr/bin/env python3
"""Generate the smallest useful qsproj JSON.

The JSON follows the rules in QSPROJ_LLM_EXAMPLES_COMPACT.md:
- format: qsproj
- version: 1
- project: id, name, createdAt, updatedAt, settings, globalVariables
- session: projectId, dirty, currentScene, editorState
- The playUnit contains one camera object with Transform, Camera, PlaySettings
- No comments or markdown
"""

import json
import time
import random
import string

now = int(time.time() * 1000)

def generate_id(prefix: str) -> str:
    """Return prefix + 8‑character random string."""
    postfix = ''.join(random.choice(string.ascii_letters + string.digits) for _ in range(8))
    return f"{prefix}{postfix}"

def make_transform_component(x: int, y: int, z: int, rotation: int,
                            scaleX: int, scaleY: int) -> dict:
    """Create a Transform component."""
    return {
        "type": "Transform",
        "enabled": True,
        "data": {
            "x": x,
            "y": y,
            "z": z,
            "rotation": rotation,
            "scaleX": scaleX,
            "scaleY": scaleY,
        },
    }

def make_text_component(text: str) -> dict:
    return {
        "type": "Text",
        "enabled": True,
        "data": {
            "text": text,
            "font": "20px sans-serif",
            "color": "#e2e8f0",
            "alpha": 1,
            "align": "center",
            "baseline": "middle",
            "wrap": False,
            "maxWidth": 0,
            "lineHeight": 24,
            "strokeColor": "",
            "strokeWidth": 0,
            "backgroundColor": "",
            "padding": 0
        }
    }

def make_empty_object(name: str) -> dict:
    return {
        "id": generate_id("obj_"),
        "name": name,
        "components": []
    }

main_object_id = generate_id("obj_")
project_id = generate_id("proj_")

text_object = make_empty_object("Text Object")
text_object["components"].append(make_transform_component(100, 100, 0, 0, 1, 1))
text_object["components"].append(make_text_component("Hello, World!"))

qsproj = {
    "format": "qsproj",
    "version": 1,
    "project": {
        "id": project_id,
        "name": "Minimal Project",
        "createdAt": now,
        "updatedAt": now,
        "settings": {
            "defaultChipWidth": 16,
            "defaultChipHeight": 16
        },
        "globalVariables": {
            "version": 1,
            "system": {"fixed": {}, "persistent": {}},
            "user": {"fixed": {}, "persistent": {}}
        },
        "assets": {
            "pixelDocuments": [],
            "tilesets": [],
            "maps": [],
            "playUnits": [
                {
                    "id": generate_id("pu_"),
                    "type": "playUnit",
                    "name": "Main",
                    "objects": [
                        {
                            "id": main_object_id,
                            "name": "Main Camera",
                            "components": [
                                {"type": "Transform", "enabled": True, "data": {"x": 0, "y": 0, "z": 0, "rotation": 0, "scaleX": 1, "scaleY": 1}},
                                {"type": "Camera", "enabled": True, "data": {"zoom": 1, "viewportX": 0, "viewportY": 0, "viewportWidth": 800, "viewportHeight": 600, "followTargetObjectId": "", "followLerp": 1}},
                                {"type": "PlaySettings", "enabled": True, "data": {"defaultCameraObjectId": main_object_id}}
                            ]
                        },
                        text_object
                    ]
                }
            ]
        }
    },
    "session": {
        "projectId": project_id,
        "dirty": False,
        "currentScene": "PlayUnitEditorScene",
        "editorState": {
            "activeTool": "pencil",
            "foreColor": 4278190080,
            "backColor": 4294967295,
            "editMode": "free",
            "selectedChip": {"col": 0, "row": 0}
        }
    }
}

print(json.dumps(qsproj, indent=4))
