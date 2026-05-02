#!/usr/bin/env python3
"""Generate qsproj fixtures for quick manual testing.

Examples:
    python generate_qs_project.py --template minimal
    python generate_qs_project.py --template ui_canvas --output ../assets/tmp_ui_canvas.qsproj
    python generate_qs_project.py --template ui_canvas --ui-spec ./ui_canvas_spec_example.json --validate
    python generate_qs_project.py --template world_ui --seed 42 --validate
"""

from __future__ import annotations

import argparse
import json
import random
import string
import sys
import time
from pathlib import Path
from typing import Any


def now_ms() -> int:
    return int(time.time() * 1000)


def generate_id(rng: random.Random, prefix: str) -> str:
    postfix = "".join(rng.choice(string.ascii_letters + string.digits) for _ in range(8))
    return f"{prefix}{postfix}"


def make_component(component_type: str, data: dict[str, Any], enabled: bool = True) -> dict[str, Any]:
    return {
        "type": component_type,
        "enabled": enabled,
        "data": data,
    }


def make_object(
    object_id: str,
    name: str,
    components: list[dict[str, Any]],
    *,
    parent_id: str = "",
    children: list[str] | None = None,
    enabled: bool = True,
) -> dict[str, Any]:
    return {
        "id": object_id,
        "name": name,
        "enabled": enabled,
        "parentId": parent_id,
        "children": children or [],
        "components": components,
    }


def transform(x: int, y: int, z: int = 0, rotation: int = 0, scale_x: int = 1, scale_y: int = 1) -> dict[str, Any]:
    return make_component(
        "Transform",
        {
            "x": x,
            "y": y,
            "z": z,
            "rotation": rotation,
            "scaleX": scale_x,
            "scaleY": scale_y,
        },
    )


def camera(*, follow_target_object_id: str = "", follow_lerp: float = 1) -> dict[str, Any]:
    return make_component(
        "Camera",
        {
            "zoom": 1,
            "viewportX": 0,
            "viewportY": 0,
            "viewportWidth": 800,
            "viewportHeight": 600,
            "followTargetObjectId": follow_target_object_id,
            "followLerp": follow_lerp,
        },
    )


def play_settings(default_camera_object_id: str) -> dict[str, Any]:
    return make_component("PlaySettings", {"defaultCameraObjectId": default_camera_object_id})


def controller(*, input_mode: str = "player1", move_speed: int = 120) -> dict[str, Any]:
    return make_component("Controller", {"inputMode": input_mode, "moveSpeed": move_speed})


def collider(width: int, height: int, *, is_trigger: bool = False, offset_x: int = 0, offset_y: int = 0) -> dict[str, Any]:
    return make_component(
        "Collider",
        {
            "shape": "rect",
            "offsetX": offset_x,
            "offsetY": offset_y,
            "width": width,
            "height": height,
            "isTrigger": is_trigger,
        },
    )


def rectangle(
    width: int,
    height: int,
    *,
    fill_color: str,
    stroke_color: str = "#000000",
    stroke_width: int = 2,
    fill_alpha: float = 1,
    stroke_alpha: float = 1,
    origin_x: float = 0,
    origin_y: float = 0,
) -> dict[str, Any]:
    return make_component(
        "Rectangle",
        {
            "shape": "rectangle",
            "width": width,
            "height": height,
            "fillColor": fill_color,
            "fillAlpha": fill_alpha,
            "strokeColor": stroke_color,
            "strokeWidth": stroke_width,
            "strokeAlpha": stroke_alpha,
            "rotation": 0,
            "originX": origin_x,
            "originY": origin_y,
        },
    )


def text_component(
    text: str,
    *,
    font: str = "20px sans-serif",
    color: str = "#e2e8f0",
    align: str = "center",
    baseline: str = "middle",
    line_height: int = 24,
    stroke_color: str = "",
    stroke_width: int = 0,
) -> dict[str, Any]:
    return make_component(
        "Text",
        {
            "text": text,
            "font": font,
            "color": color,
            "alpha": 1,
            "align": align,
            "baseline": baseline,
            "wrap": False,
            "maxWidth": 0,
            "lineHeight": line_height,
            "strokeColor": stroke_color,
            "strokeWidth": stroke_width,
            "backgroundColor": "",
            "padding": 0,
        },
    )


def ui_canvas(*, sort_order: int = 100, reference_width: int = 640, reference_height: int = 360) -> dict[str, Any]:
    return make_component(
        "UICanvas",
        {
            "renderMode": "screen",
            "sortOrder": sort_order,
            "referenceWidth": reference_width,
            "referenceHeight": reference_height,
        },
    )


def ui_transform(
    x: int,
    y: int,
    width: int,
    height: int,
    *,
    anchor_x: float = 0.5,
    anchor_y: float = 0.5,
    pivot_x: float = 0.5,
    pivot_y: float = 0.5,
) -> dict[str, Any]:
    return make_component(
        "UITransform",
        {
            "x": x,
            "y": y,
            "width": width,
            "height": height,
            "anchorX": anchor_x,
            "anchorY": anchor_y,
            "pivotX": pivot_x,
            "pivotY": pivot_y,
        },
    )


def trigger(event_id: str, trigger_on: str) -> dict[str, Any]:
    return make_component(
        "Trigger",
        {
            "eventId": event_id,
            "triggerOn": trigger_on,
            "once": False,
            "targetObjectId": "",
        },
    )


def trigger_component(event_id: str, trigger_on: str, *, once: bool = False, target_object_id: str = "") -> dict[str, Any]:
    return make_component(
        "Trigger",
        {
            "eventId": event_id,
            "triggerOn": trigger_on,
            "once": once,
            "targetObjectId": target_object_id,
        },
    )


def event_action(event_id: str, target_object_id: str, component_type: str, property_name: str, value: Any) -> dict[str, Any]:
    return make_component(
        "EventAction",
        {
            "listenTo": event_id,
            "action": "setProperty",
            "targetObjectId": target_object_id,
            "componentType": component_type,
            "property": property_name,
            "value": value,
        },
    )


def make_event_action_component(data: dict[str, Any], *, enabled: bool = True) -> dict[str, Any]:
    return make_component("EventAction", data, enabled=enabled)


def build_root_project(project_name: str, play_unit_name: str, play_unit_id: str, *, include_session: bool) -> dict[str, Any]:
    project_id = ""
    root = {
        "format": "qsproj",
        "version": 1,
        "project": {
            "id": project_id,
            "version": 1,
            "name": project_name,
            "createdAt": now_ms(),
            "updatedAt": now_ms(),
            "settings": {
                "defaultChipWidth": 16,
                "defaultChipHeight": 16,
            },
            "globalVariables": {
                "version": 1,
                "system": {
                    "fixed": {
                        "startupPlayUnitId": {
                            "type": "string",
                            "initialValue": play_unit_id,
                        },
                        "timer": {
                            "type": "number",
                            "initialValue": 0,
                        },
                    },
                    "persistent": {},
                },
                "user": {
                    "fixed": {},
                    "persistent": {},
                },
            },
            "assets": {
                "pixelDocuments": [],
                "tilesets": [],
                "maps": [],
                "playUnits": [
                    {
                        "id": play_unit_id,
                        "type": "playUnit",
                        "name": play_unit_name,
                        "description": "Generated by generate_qs_project.py",
                        "objects": [],
                    }
                ],
            },
        },
    }
    if include_session:
        root["session"] = {
            "projectId": project_id,
            "dirty": False,
            "currentScene": "PlayUnitEditorScene",
            "editorState": {
                "activeTool": "pencil",
                "foreColor": 4278190080,
                "backColor": 4294967295,
                "editMode": "free",
                "selectedChip": {"col": 0, "row": 0},
            },
        }
    return root


def build_minimal_template(rng: random.Random, args: argparse.Namespace) -> dict[str, Any]:
    play_unit_id = generate_id(rng, "pu_")
    project_id = generate_id(rng, "proj_")
    camera_id = generate_id(rng, "obj_")
    text_id = generate_id(rng, "obj_")

    root = build_root_project(args.project_name, args.play_unit_name, play_unit_id, include_session=args.include_session)
    root["project"]["id"] = project_id
    if "session" in root:
        root["session"]["projectId"] = project_id

    play_unit = root["project"]["assets"]["playUnits"][0]
    play_unit["objects"] = [
        make_object(camera_id, "Main Camera", [transform(0, 0, 0), camera(), play_settings(camera_id)]),
        make_object(text_id, "Text Object", [transform(args.text_x, args.text_y, 0), text_component(args.text)]),
    ]
    return root


def build_ui_canvas_template(rng: random.Random, args: argparse.Namespace) -> dict[str, Any]:
    if args.ui_spec:
        return build_ui_canvas_from_spec(rng, args)

    play_unit_id = generate_id(rng, "pu_")
    project_id = generate_id(rng, "proj_")
    camera_id = generate_id(rng, "obj_")
    ui_root_id = generate_id(rng, "obj_")
    panel_id = generate_id(rng, "obj_")
    title_id = generate_id(rng, "obj_")
    button_id = generate_id(rng, "obj_")
    label_id = generate_id(rng, "obj_")
    status_id = generate_id(rng, "obj_")

    root = build_root_project(args.project_name, args.play_unit_name, play_unit_id, include_session=args.include_session)
    root["project"]["id"] = project_id
    if "session" in root:
        root["session"]["projectId"] = project_id

    play_unit = root["project"]["assets"]["playUnits"][0]
    play_unit["description"] = "Generated UI canvas template"
    play_unit["objects"] = [
        make_object(camera_id, "MainCamera", [transform(0, 0, 0), camera(), play_settings(camera_id)]),
        make_object(ui_root_id, "UIRoot", [ui_canvas()], children=[panel_id, title_id, button_id, status_id]),
        make_object(
            panel_id,
            "UIPanel",
            [
                ui_transform(0, 0, 420, 220),
                rectangle(420, 220, fill_color="#0f172a", stroke_color="#38bdf8"),
            ],
            parent_id=ui_root_id,
        ),
        make_object(
            title_id,
            "UITitle",
            [
                ui_transform(0, -72, 360, 40),
                text_component("Generated UI Canvas", font="bold 26px sans-serif"),
            ],
            parent_id=ui_root_id,
        ),
        make_object(
            button_id,
            "UIButton",
            [
                ui_transform(0, 8, 220, 56),
                rectangle(220, 56, fill_color="#1d4ed8", stroke_color="#93c5fd"),
                collider(220, 56, is_trigger=True),
                trigger_component("ev_button_click", "click"),
                make_event_action_component(
                    {
                        "listenTo": "ev_button_click",
                        "action": "setProperty",
                        "targetObjectId": status_id,
                        "componentType": "Text",
                        "property": "text",
                        "value": "Status: Button clicked",
                    }
                ),
            ],
            parent_id=ui_root_id,
            children=[label_id],
        ),
        make_object(
            label_id,
            "UIButtonLabel",
            [
                ui_transform(0, 0, 220, 56),
                text_component("Click", font="bold 20px sans-serif", color="#eff6ff"),
            ],
            parent_id=button_id,
        ),
        make_object(
            status_id,
            "UIStatus",
            [
                ui_transform(0, 74, 320, 28),
                text_component("Status: Waiting", font="bold 18px sans-serif", color="#fbbf24"),
            ],
            parent_id=ui_root_id,
        ),
    ]
    return root


def _resolve_object_reference(value: Any, node_id_map: dict[str, str]) -> Any:
    if not isinstance(value, str):
        return value
    next_value = value.strip()
    if not next_value:
        return ""
    return node_id_map.get(next_value, next_value)


def _resolve_component_references(component_type: str, data: dict[str, Any], node_id_map: dict[str, str]) -> dict[str, Any]:
    resolved = dict(data)
    if component_type == "EventAction":
        resolved["targetObjectId"] = _resolve_object_reference(resolved.get("targetObjectId", ""), node_id_map)
    elif component_type == "Trigger":
        resolved["targetObjectId"] = _resolve_object_reference(resolved.get("targetObjectId", ""), node_id_map)
    elif component_type == "Camera":
        resolved["followTargetObjectId"] = _resolve_object_reference(resolved.get("followTargetObjectId", ""), node_id_map)
    elif component_type == "PlaySettings":
        resolved["defaultCameraObjectId"] = _resolve_object_reference(resolved.get("defaultCameraObjectId", ""), node_id_map)
    return resolved


def _normalize_component_specs(component_specs: Any, node_id_map: dict[str, str] | None = None) -> list[dict[str, Any]]:
    if component_specs is None:
        return []
    if not isinstance(component_specs, list):
        raise ValueError("ui-spec components must be a list")

    normalized: list[dict[str, Any]] = []
    for component in component_specs:
        if not isinstance(component, dict):
            raise ValueError("ui-spec component entries must be objects")
        component_type = component.get("type")
        if not isinstance(component_type, str) or not component_type.strip():
            raise ValueError("ui-spec component.type must be a non-empty string")
        component_type = component_type.strip()
        data = component.get("data", {})
        if not isinstance(data, dict):
            raise ValueError(f"ui-spec component.data for '{component_type}' must be an object")
        if node_id_map is not None:
            data = _resolve_component_references(component_type, data, node_id_map)
        normalized.append({
            "type": component_type,
            "enabled": component.get("enabled", True) is not False,
            "data": data,
        })
    return normalized


def _build_event_action_specs(event_spec: dict[str, Any], node_id_map: dict[str, str], event_id: str) -> list[dict[str, Any]]:
    actions = event_spec.get("actions")
    if actions is None:
        return []
    if not isinstance(actions, list):
        raise ValueError("ui-spec event.actions must be a list")

    normalized: list[dict[str, Any]] = []
    for action in actions:
        if not isinstance(action, dict):
            raise ValueError("ui-spec event.actions entries must be objects")
        action_type = action.get("action", "setProperty")
        if not isinstance(action_type, str) or not action_type.strip():
            raise ValueError("ui-spec event action.action must be a non-empty string")

        data = dict(action)
        data["action"] = action_type.strip()
        data["listenTo"] = action.get("listenTo", event_id)
        if "target" in data and "targetObjectId" not in data:
            data["targetObjectId"] = data.pop("target")
        data["targetObjectId"] = _resolve_object_reference(data.get("targetObjectId", ""), node_id_map)
        if "component" in data and "componentType" not in data:
            data["componentType"] = data.pop("component")
        normalized.append(make_event_action_component(data, enabled=action.get("enabled", True) is not False))
    return normalized


def _build_node_event_components(node: dict[str, Any], node_id_map: dict[str, str]) -> list[dict[str, Any]]:
    event_specs = node.get("events")
    if event_specs is None:
        return []
    if not isinstance(event_specs, list):
        raise ValueError("ui-spec node.events must be a list")

    node_key = node["id"].strip()
    components: list[dict[str, Any]] = []
    for index, event_spec in enumerate(event_specs):
        if not isinstance(event_spec, dict):
            raise ValueError("ui-spec node.events entries must be objects")
        trigger_on = event_spec.get("on", event_spec.get("triggerOn"))
        if not isinstance(trigger_on, str) or not trigger_on.strip():
            raise ValueError(f"ui-spec node.events[{index}] for '{node_key}' must define 'on' or 'triggerOn'")
        trigger_on = trigger_on.strip()
        default_event_id = f"ev_{node_key}_{trigger_on}_{index + 1}"
        event_id = event_spec.get("eventId", default_event_id)
        if not isinstance(event_id, str) or not event_id.strip():
            raise ValueError(f"ui-spec node.events[{index}] for '{node_key}' has invalid eventId")
        event_id = event_id.strip()
        target_object_id = _resolve_object_reference(event_spec.get("targetObjectId", event_spec.get("target", "")), node_id_map)
        components.append(
            trigger_component(
                event_id,
                trigger_on,
                once=event_spec.get("once", False) is True,
                target_object_id=target_object_id,
            )
        )
        components.extend(_build_event_action_specs(event_spec, node_id_map, event_id))
    return components


def _build_node_components(node: dict[str, Any], node_id_map: dict[str, str]) -> list[dict[str, Any]]:
    components = _normalize_component_specs(node.get("components"), node_id_map)
    components.extend(_build_node_event_components(node, node_id_map))
    return components


def _load_ui_spec(path: Path) -> dict[str, Any]:
    try:
        raw = path.read_text(encoding="utf-8")
    except OSError as exc:
        raise ValueError(f"failed to read ui-spec: {path}") from exc

    try:
        data = json.loads(raw)
    except json.JSONDecodeError as exc:
        raise ValueError(f"ui-spec is not valid JSON: {path}") from exc

    if not isinstance(data, dict):
        raise ValueError("ui-spec root must be an object")
    if not isinstance(data.get("nodes"), list) or not data["nodes"]:
        raise ValueError("ui-spec must contain a non-empty 'nodes' array")
    return data


def _build_ui_canvas_project_from_spec(rng: random.Random, args: argparse.Namespace, spec: dict[str, Any]) -> dict[str, Any]:
    play_unit_id = generate_id(rng, "pu_")
    project_id = generate_id(rng, "proj_")
    camera_id = generate_id(rng, "obj_")
    ui_root_id = generate_id(rng, "obj_")

    project_name = spec.get("projectName") if isinstance(spec.get("projectName"), str) and spec.get("projectName").strip() else args.project_name
    play_unit_name = spec.get("playUnitName") if isinstance(spec.get("playUnitName"), str) and spec.get("playUnitName").strip() else args.play_unit_name
    root = build_root_project(project_name, play_unit_name, play_unit_id, include_session=args.include_session)
    root["project"]["id"] = project_id
    if "session" in root:
        root["session"]["projectId"] = project_id

    play_unit = root["project"]["assets"]["playUnits"][0]
    canvas_spec = spec.get("canvas") if isinstance(spec.get("canvas"), dict) else {}
    canvas_name = canvas_spec.get("name") if isinstance(canvas_spec.get("name"), str) and canvas_spec.get("name").strip() else "UIRoot"
    canvas_sort_order = int(canvas_spec.get("sortOrder", 100)) if isinstance(canvas_spec.get("sortOrder", 100), (int, float)) else 100
    canvas_reference_width = int(canvas_spec.get("referenceWidth", 640)) if isinstance(canvas_spec.get("referenceWidth", 640), (int, float)) else 640
    canvas_reference_height = int(canvas_spec.get("referenceHeight", 360)) if isinstance(canvas_spec.get("referenceHeight", 360), (int, float)) else 360
    canvas_extra_components = _normalize_component_specs(canvas_spec.get("components"), node_id_map={})

    node_specs = spec["nodes"]
    node_id_map: dict[str, str] = {}
    child_map: dict[str, list[str]] = {"root": []}
    normalized_nodes: list[dict[str, Any]] = []

    for index, node in enumerate(node_specs):
        if not isinstance(node, dict):
            raise ValueError(f"ui-spec node at index {index} must be an object")
        node_key = node.get("id")
        if not isinstance(node_key, str) or not node_key.strip():
            raise ValueError(f"ui-spec node at index {index} must define a non-empty id")
        node_key = node_key.strip()
        if node_key in node_id_map:
            raise ValueError(f"duplicate ui-spec node id: {node_key}")
        node_id_map[node_key] = generate_id(rng, "obj_")
        normalized_nodes.append(node)

    for node in normalized_nodes:
        node_key = node["id"].strip()
        parent_key = node.get("parent", "root")
        if not isinstance(parent_key, str) or not parent_key.strip():
            parent_key = "root"
        parent_key = parent_key.strip()
        if parent_key != "root" and parent_key not in node_id_map:
            raise ValueError(f"ui-spec node '{node_key}' refers to unknown parent '{parent_key}'")
        child_map.setdefault(parent_key, []).append(node_key)

    objects: list[dict[str, Any]] = [
        make_object(camera_id, "MainCamera", [transform(0, 0, 0), camera(), play_settings(camera_id)]),
        make_object(
            ui_root_id,
            canvas_name,
            [ui_canvas(sort_order=canvas_sort_order, reference_width=canvas_reference_width, reference_height=canvas_reference_height), *canvas_extra_components],
            children=[node_id_map[node_key] for node_key in child_map.get("root", [])],
        ),
    ]

    for node in normalized_nodes:
        node_key = node["id"].strip()
        object_id = node_id_map[node_key]
        parent_key = node.get("parent", "root")
        if not isinstance(parent_key, str) or not parent_key.strip():
            parent_key = "root"
        parent_key = parent_key.strip()
        parent_id = ui_root_id if parent_key == "root" else node_id_map[parent_key]
        name = node.get("name") if isinstance(node.get("name"), str) and node.get("name").strip() else node_key
        enabled = node.get("enabled", True) is not False
        components = _build_node_components(node, node_id_map)
        objects.append(
            make_object(
                object_id,
                name,
                components,
                parent_id=parent_id,
                children=[node_id_map[child_key] for child_key in child_map.get(node_key, [])],
                enabled=enabled,
            )
        )

    play_unit["description"] = spec.get("description") if isinstance(spec.get("description"), str) else "Generated UI canvas from ui-spec"
    play_unit["objects"] = objects
    return root


def build_ui_canvas_from_spec(rng: random.Random, args: argparse.Namespace) -> dict[str, Any]:
    if not args.ui_spec:
        raise ValueError("--ui-spec is required for spec-based ui_canvas generation")
    spec = _load_ui_spec(args.ui_spec)
    return _build_ui_canvas_project_from_spec(rng, args, spec)


def build_world_ui_template(rng: random.Random, args: argparse.Namespace) -> dict[str, Any]:
    play_unit_id = generate_id(rng, "pu_")
    project_id = generate_id(rng, "proj_")
    camera_id = generate_id(rng, "obj_")
    player_id = generate_id(rng, "obj_")
    player_label_id = generate_id(rng, "obj_")
    world_button_id = generate_id(rng, "obj_")
    world_button_label_id = generate_id(rng, "obj_")
    ui_root_id = generate_id(rng, "obj_")
    hud_id = generate_id(rng, "obj_")
    hud_status_id = generate_id(rng, "obj_")

    root = build_root_project(args.project_name, args.play_unit_name, play_unit_id, include_session=args.include_session)
    root["project"]["id"] = project_id
    if "session" in root:
        root["session"]["projectId"] = project_id

    play_unit = root["project"]["assets"]["playUnits"][0]
    play_unit["description"] = "World objects plus fixed HUD for coordinate-system testing"
    play_unit["objects"] = [
        make_object(camera_id, "MainCamera", [transform(0, 0, 0), camera(follow_target_object_id=player_id, follow_lerp=0.18), play_settings(camera_id)]),
        make_object(generate_id(rng, "obj_"), "FieldBackground", [transform(0, 0, -20), rectangle(1400, 900, fill_color="#1f3b2d", stroke_color="#365f49", stroke_width=0)]),
        make_object(player_id, "Player", [transform(300, 320, 20), rectangle(42, 52, fill_color="#2563eb", stroke_color="#dbeafe"), controller(move_speed=150), collider(42, 52)]),
        make_object(player_label_id, "PlayerLabel", [transform(321, 306, 21), text_component("Player", font="bold 14px sans-serif", align="center", baseline="bottom", stroke_color="#0f172a", stroke_width=3)]),
        make_object(
            world_button_id,
            "WorldButton",
            [
                transform(760, 336, 12),
                rectangle(180, 56, fill_color="#7c2d12", stroke_color="#fdba74"),
                collider(180, 56, is_trigger=True),
                trigger("ev_world_button_click", "click"),
                event_action("ev_world_button_click", hud_status_id, "Text", "text", "Status: World button clicked"),
            ],
        ),
        make_object(world_button_label_id, "WorldButtonLabel", [transform(850, 364, 13), text_component("World Button", font="bold 18px sans-serif")]),
        make_object(ui_root_id, "UIRoot", [ui_canvas()], children=[hud_id, hud_status_id]),
        make_object(hud_id, "HUDPanel", [ui_transform(18, 16, 250, 90, anchor_x=0, anchor_y=0, pivot_x=0, pivot_y=0), rectangle(250, 90, fill_color="#0f172a", stroke_color="#38bdf8", fill_alpha=0.9)], parent_id=ui_root_id),
        make_object(hud_status_id, "HUDStatus", [ui_transform(34, 40, 200, 24, anchor_x=0, anchor_y=0, pivot_x=0, pivot_y=0), text_component("Status: Waiting", font="bold 18px sans-serif", align="left", baseline="top", color="#f8fafc")], parent_id=ui_root_id),
    ]
    return root


def validate_qsproj(data: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    if data.get("format") != "qsproj":
        errors.append("root.format must be 'qsproj'")
    if data.get("version") != 1:
        errors.append("root.version must be 1")

    project = data.get("project")
    if not isinstance(project, dict):
        return errors + ["root.project must exist"]

    assets = project.get("assets")
    if not isinstance(assets, dict):
        return errors + ["project.assets must exist"]

    play_units = assets.get("playUnits")
    if not isinstance(play_units, list) or not play_units:
        return errors + ["project.assets.playUnits must contain at least one playUnit"]

    first_play_unit = play_units[0]
    objects = first_play_unit.get("objects") if isinstance(first_play_unit, dict) else None
    if not isinstance(objects, list) or not objects:
        return errors + ["playUnit.objects must contain at least one object"]

    camera_found = False
    play_settings_found = False
    for object_data in objects:
        if not isinstance(object_data, dict):
            continue
        components = object_data.get("components")
        if not isinstance(components, list):
            continue
        component_types = {component.get("type") for component in components if isinstance(component, dict)}
        if "Camera" in component_types and "Transform" in component_types:
            camera_found = True
        if "PlaySettings" in component_types:
            play_settings_found = True

    if not camera_found:
        errors.append("at least one object must include Transform + Camera")
    if not play_settings_found:
        errors.append("at least one object must include PlaySettings")
    return errors


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate qsproj fixtures for manual testing")
    parser.add_argument("--template", choices=["minimal", "ui_canvas", "world_ui"], default="minimal")
    parser.add_argument("--project-name", default="Generated Project")
    parser.add_argument("--play-unit-name", default="Generated PlayUnit")
    parser.add_argument("--text", default="Hello, World!")
    parser.add_argument("--text-x", type=int, default=100)
    parser.add_argument("--text-y", type=int, default=100)
    parser.add_argument("--ui-spec", type=Path, help="JSON spec used to build a custom UICanvas when template=ui_canvas")
    parser.add_argument("--output", type=Path, help="Write output to file instead of stdout")
    parser.add_argument("--seed", type=int, help="Use deterministic IDs for reproducible fixtures")
    parser.add_argument("--indent", type=int, default=2)
    parser.add_argument("--include-session", action="store_true", help="Include the legacy session section")
    parser.add_argument("--validate", action="store_true", help="Validate the generated qsproj before printing")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    rng = random.Random(args.seed)

    builders = {
        "minimal": build_minimal_template,
        "ui_canvas": build_ui_canvas_template,
        "world_ui": build_world_ui_template,
    }
    try:
        root = builders[args.template](rng, args)
    except ValueError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    if args.validate:
        errors = validate_qsproj(root)
        if errors:
            for error in errors:
                print(f"validation error: {error}", file=sys.stderr)
            return 1

    serialized = json.dumps(root, indent=args.indent, ensure_ascii=False)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(serialized + "\n", encoding="utf-8")
    else:
        print(serialized)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
