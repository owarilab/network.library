#!/usr/bin/env python3
"""
character_walk_demo.qsproj に、室内マップへの遷移（扉）を追加する。
前提: add_tilemap_to_character_demo.py を実行済みであること（タイルセットと Field の Player がある）。

- 室内マップ map_demo_house（12x10。ts_demo_field を使う。外周は壁、入口は下の 2 マス）
- 室内の PlayUnit pu_character_walk_house:
    Player（Field と同じ ID の obj_player。見た目と Controller を揃える）
    出口 obj_house_exit（overlap で house.exit）
    入口の出現位置 obj_spawn_from_field
    フェード用 UICanvas obj_house_fade（到着で黒い幕を開け、出口でフェードアウトしてから戻る）
- Field に追加:
    扉 obj_house_door（overlap で door.house。建物の下）
    戻りの出現位置 obj_spawn_from_house
    フェード用 UICanvas obj_field_fade（到着で開け、扉でフェードアウトしてから室内へ）

遷移は sequence（playTween の waitComplete → warp）で組む。
到着の演出は system.playUnitEnter（PlayUnit に入るたびに 1 回）で行う。

同じスクリプトを再実行しても結果は変わる（追加済みのものは置き換える）。
標準ライブラリのみを使う。

使い方:
  python3 -I tools/add_map_transition_to_demo.py assets/character_walk_demo.qsproj [出力.qsproj]
  出力を省略すると入力を上書きする。
"""

from __future__ import annotations

import copy
import json
import sys
import time
from pathlib import Path
from typing import Any

FIELD_ID = "pu_character_walk_field"
HOUSE_ID = "pu_character_walk_house"
TILESET_ID = "ts_demo_field"
HOUSE_MAP_ID = "map_demo_house"
CHARACTER_ID = "cc_character_walk_hero"

# add_tilemap_to_character_demo.py のタイルセット内の位置（column）と一致させる
TILE_ROAD = 1
TILE_WALL = 2
HOUSE_WIDTH = 12
HOUSE_HEIGHT = 10
CHIP_SIZE = 32

# 室内の入口（下の壁の 2 マス）。出口の Trigger と Rectangle もここに置く
EXIT_X = 5
EXIT_Y = HOUSE_HEIGHT - 1
EXIT_COLUMNS = 2

FADE_DURATION_MS = 250
REFERENCE_WIDTH = 640
REFERENCE_HEIGHT = 360

ENTER_EVENT_ID = "system.playUnitEnter"
DOOR_EVENT_ID = "door.house"
EXIT_EVENT_ID = "house.exit"

# Field の追加物
FIELD_DOOR_ID = "obj_house_door"
FIELD_SPAWN_ID = "obj_spawn_from_house"
FIELD_FADE_ID = "obj_field_fade"
FIELD_FADE_BOX_ID = "obj_field_fade_box"
FIELD_DOOR_X = 320  # セル (10, 11)。建物（セル 8..13 x 8..10）の真下
FIELD_DOOR_Y = 352
FIELD_SPAWN_X = 336  # 扉の真下（セル (10, 12) の足元）
FIELD_SPAWN_Y = 412

# 室内の追加物
HOUSE_MAP_OBJECT_ID = "obj_house_map"
HOUSE_EXIT_ID = "obj_house_exit"
HOUSE_SPAWN_ID = "obj_spawn_from_field"
HOUSE_FADE_ID = "obj_house_fade"
HOUSE_FADE_BOX_ID = "obj_house_fade_box"
HOUSE_START_X = EXIT_X * CHIP_SIZE + CHIP_SIZE // 2  # 入口の真上（セル (5, 8) の中央）
HOUSE_START_Y = (EXIT_Y - 1) * CHIP_SIZE + CHIP_SIZE - 8

# Field から室内へ（Field の共通の要素を室内にも使う）
SHARED_OBJECT_IDS = ("obj_root", "obj_camera", "obj_settings", "obj_player")

FIELD_OBJECT_IDS = {FIELD_DOOR_ID, FIELD_SPAWN_ID, FIELD_FADE_ID, FIELD_FADE_BOX_ID}
HOUSE_OBJECT_IDS = {HOUSE_MAP_OBJECT_ID, HOUSE_EXIT_ID, HOUSE_SPAWN_ID, HOUSE_FADE_ID, HOUSE_FADE_BOX_ID}


def now_ms() -> int:
    return int(time.time() * 1000)


def make_component(component_type: str, data: dict[str, Any]) -> dict[str, Any]:
    return {"type": component_type, "enabled": True, "data": data}


def make_object(object_id: str, name: str, parent_id: str | None, children: list[str], components: list[dict[str, Any]]) -> dict[str, Any]:
    return {
        "id": object_id,
        "name": name,
        "enabled": True,
        "parentId": parent_id,
        "children": children,
        "components": components,
    }


def make_transform(x: float, y: float, z: float = 0) -> dict[str, Any]:
    return make_component("Transform", {"x": x, "y": y, "z": z, "rotation": 0, "scaleX": 1, "scaleY": 1})


def make_door_rectangle(width: int, height: int) -> dict[str, Any]:
    return make_component("Rectangle", {
        "shape": "rectangle",
        "width": width,
        "height": height,
        "fillColor": "#7c4a2d",
        "fillAlpha": 1,
        "strokeColor": "#3f2a1a",
        "strokeWidth": 2,
        "strokeAlpha": 1,
        "rotation": 0,
        "originX": 0,
        "originY": 0,
        "sides": 4,
        "points": 5,
        "innerRadius": 0.4,
    })


def make_trigger_collider(width: int, height: int) -> dict[str, Any]:
    return make_component("Collider", {"shape": "rect", "offsetX": 0, "offsetY": 0, "width": width, "height": height, "isTrigger": True})


def make_overlap_trigger(event_id: str) -> dict[str, Any]:
    return make_component("Trigger", {"eventId": event_id, "triggerOn": "overlap", "once": False, "targetObjectId": ""})


def make_fade_canvas(object_id: str, name: str, box_id: str, sort_order: int, actions: list[dict[str, Any]]) -> list[dict[str, Any]]:
    """黒い幕（全画面の Rectangle）を持つ UICanvas と、その子の box を作る。"""
    canvas = make_object(object_id, name, None, [box_id], [
        make_component("UICanvas", {"referenceWidth": REFERENCE_WIDTH, "referenceHeight": REFERENCE_HEIGHT, "sortOrder": sort_order}),
        *actions,
    ])
    box = make_object(box_id, f"{name}Box", object_id, [], [
        make_component("UITransform", {"anchorX": 0, "anchorY": 0, "pivotX": 0, "pivotY": 0, "x": 0, "y": 0, "width": REFERENCE_WIDTH, "height": REFERENCE_HEIGHT}),
        make_component("Rectangle", {
            "shape": "rectangle",
            "width": REFERENCE_WIDTH,
            "height": REFERENCE_HEIGHT,
            "fillColor": "#000000",
            "fillAlpha": 0,
            "strokeColor": "",
            "strokeWidth": 0,
            "strokeAlpha": 1,
            "rotation": 0,
            "originX": 0,
            "originY": 0,
            "sides": 4,
            "points": 5,
            "innerRadius": 0.4,
        }),
    ])
    return [canvas, box]


def make_fade_in_action(box_id: str) -> dict[str, Any]:
    """PlayUnit に入るたびに、黒い幕を開ける（alpha 1 → 0）。"""
    return make_component("EventAction", {
        "listenTo": ENTER_EVENT_ID,
        "action": "playTween",
        "targetObjectId": box_id,
        "componentType": "Rectangle",
        "property": "fillAlpha",
        "tweenFrom": 1,
        "tweenTo": 0,
        "tweenDuration": FADE_DURATION_MS,
        "tweenEasing": "easeOut",
        "completeEventId": "",
    })


def make_transition_action(listen_id: str, box_id: str, target_play_unit_id: str, spawn_object_id: str, facing: str) -> dict[str, Any]:
    """幕を閉じてから warp する sequence。二重発火は onBusy: ignore で捨てる。"""
    return make_component("EventAction", {
        "listenTo": listen_id,
        "action": "sequence",
        "targetObjectId": "",
        "onBusy": "ignore",
        "steps": [
            {
                "action": "playTween",
                "targetObjectId": box_id,
                "componentType": "Rectangle",
                "property": "fillAlpha",
                "tweenFrom": 0,
                "tweenTo": 1,
                "tweenDuration": FADE_DURATION_MS,
                "tweenEasing": "linear",
                "waitComplete": True,
            },
            {
                "action": "warp",
                "playUnitId": target_play_unit_id,
                "spawnObjectId": spawn_object_id,
                "facing": facing,
            },
        ],
    })


def build_field_objects() -> list[dict[str, Any]]:
    door = make_object(FIELD_DOOR_ID, "HouseDoor", None, [], [
        make_transform(FIELD_DOOR_X, FIELD_DOOR_Y),
        make_door_rectangle(CHIP_SIZE, CHIP_SIZE),
        make_trigger_collider(CHIP_SIZE, CHIP_SIZE),
        make_overlap_trigger(DOOR_EVENT_ID),
    ])
    spawn = make_object(FIELD_SPAWN_ID, "SpawnFromHouse", None, [], [
        make_transform(FIELD_SPAWN_X, FIELD_SPAWN_Y),
    ])
    fade = make_fade_canvas(FIELD_FADE_ID, "FieldFade", FIELD_FADE_BOX_ID, 100, [
        make_fade_in_action(FIELD_FADE_BOX_ID),
        make_transition_action(DOOR_EVENT_ID, FIELD_FADE_BOX_ID, HOUSE_ID, HOUSE_SPAWN_ID, "up"),
    ])
    return [door, spawn, *fade]


def build_house_map() -> dict[str, Any]:
    road = TILE_ROAD
    wall = TILE_WALL
    tiles = [wall] * (HOUSE_WIDTH * HOUSE_HEIGHT)

    def fill(x0: int, y0: int, x1: int, y1: int, tile: int) -> None:
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                tiles[y * HOUSE_WIDTH + x] = tile

    # 床（外周の内側）と、下の壁の入口
    fill(1, 1, HOUSE_WIDTH - 2, HOUSE_HEIGHT - 2, road)
    fill(EXIT_X, EXIT_Y, EXIT_X + EXIT_COLUMNS - 1, EXIT_Y, road)
    # 家具（置いた壁は通行不可）
    fill(2, 2, 3, 2, wall)
    fill(8, 2, 9, 2, wall)

    return {
        "id": HOUSE_MAP_ID,
        "type": "map",
        "name": "House Map",
        "width": HOUSE_WIDTH,
        "height": HOUSE_HEIGHT,
        "mapData": {
            "version": 1,
            "width": HOUSE_WIDTH,
            "height": HOUSE_HEIGHT,
            "tileWidth": CHIP_SIZE,
            "tileHeight": CHIP_SIZE,
            "tilesetId": TILESET_ID,
            "layers": [{
                "id": "layer_ground",
                "name": "Ground",
                "visible": True,
                "locked": False,
                "tiles": tiles,
            }],
            "selectedLayer": 0,
            "selectedTileRef": {"tilesetId": TILESET_ID, "col": TILE_ROAD, "row": 0, "index": TILE_ROAD},
            "cursor": {"x": 0, "y": 0},
            "view": {"showGrid": True, "zoom": 1},
        },
    }


def build_house_play_unit(field_objects: dict[str, dict[str, Any]]) -> dict[str, Any]:
    root = copy.deepcopy(field_objects["obj_root"])
    root["children"] = ["obj_camera", "obj_settings"]
    camera = copy.deepcopy(field_objects["obj_camera"])
    settings = copy.deepcopy(field_objects["obj_settings"])

    player = copy.deepcopy(field_objects["obj_player"])
    for component in player["components"]:
        if component["type"] == "Transform":
            component["data"]["x"] = HOUSE_START_X
            component["data"]["y"] = HOUSE_START_Y

    tilemap = make_object(HOUSE_MAP_OBJECT_ID, "HouseMap", None, [], [
        make_transform(0, 0),
        make_component("Tilemap", {"mapAssetId": HOUSE_MAP_ID, "layerId": "", "scale": 1, "offsetX": 0, "offsetY": 0}),
    ])
    spawn = make_object(HOUSE_SPAWN_ID, "SpawnFromField", None, [], [
        make_transform(HOUSE_START_X, HOUSE_START_Y),
    ])
    exit_object = make_object(HOUSE_EXIT_ID, "HouseExit", None, [], [
        make_transform(EXIT_X * CHIP_SIZE, EXIT_Y * CHIP_SIZE),
        make_door_rectangle(EXIT_COLUMNS * CHIP_SIZE, CHIP_SIZE),
        make_trigger_collider(EXIT_COLUMNS * CHIP_SIZE, CHIP_SIZE),
        make_overlap_trigger(EXIT_EVENT_ID),
    ])
    fade = make_fade_canvas(HOUSE_FADE_ID, "HouseFade", HOUSE_FADE_BOX_ID, 100, [
        make_fade_in_action(HOUSE_FADE_BOX_ID),
        make_transition_action(EXIT_EVENT_ID, HOUSE_FADE_BOX_ID, FIELD_ID, FIELD_SPAWN_ID, "down"),
    ])

    return {
        "id": HOUSE_ID,
        "type": "playUnit",
        "name": "House",
        "objects": [root, camera, settings, tilemap, player, spawn, exit_object, *fade],
    }


def find_object(play_unit: dict[str, Any], object_id: str) -> dict[str, Any] | None:
    return next((obj for obj in play_unit["objects"] if obj.get("id") == object_id), None)


def replace_asset(items: list[dict[str, Any]], asset: dict[str, Any]) -> None:
    items[:] = [item for item in items if item.get("id") != asset["id"]]
    items.append(asset)


def check_prerequisites(project: dict[str, Any]) -> dict[str, Any]:
    tilesets = project["assets"].get("tilesets", [])
    if not any(item.get("id") == TILESET_ID for item in tilesets):
        raise SystemExit("先に add_tilemap_to_character_demo.py を実行してください（タイルセット ts_demo_field がありません）")
    if not any(item.get("id") == CHARACTER_ID for item in project["assets"].get("characterChipsets", [])):
        raise SystemExit(f"キャラクターチップセット {CHARACTER_ID} がありません")

    field = next((unit for unit in project["assets"]["playUnits"] if unit.get("id") == FIELD_ID), None)
    if field is None:
        raise SystemExit(f"PlayUnit が見つかりません: {FIELD_ID}")
    for object_id in SHARED_OBJECT_IDS:
        if find_object(field, object_id) is None:
            raise SystemExit(f"{FIELD_ID} に {object_id} がありません（character_walk_demo の構成ではありません）")
    return field


def update_project(project: dict[str, Any]) -> None:
    field = check_prerequisites(project)
    field_objects = {obj["id"]: obj for obj in field["objects"]}

    field["objects"] = [obj for obj in field["objects"] if obj.get("id") not in FIELD_OBJECT_IDS]
    field["objects"].extend(build_field_objects())

    replace_asset(project["assets"]["maps"], build_house_map())
    replace_asset(project["assets"]["playUnits"], build_house_play_unit(field_objects))


def format_json(data: dict[str, Any]) -> str:
    # アプリの保存形式（indent 2、タイル配列も 1 要素 1 行）と同じにして、差分を追加分だけにする
    return json.dumps(data, indent=2, ensure_ascii=False) + "\n"


def main(argv: list[str]) -> int:
    if len(argv) not in (2, 3):
        print(__doc__.strip(), file=sys.stderr)
        return 2

    input_path = Path(argv[1])
    output_path = Path(argv[2]) if len(argv) == 3 else input_path
    data = json.loads(input_path.read_text(encoding="utf-8"))
    project = data["project"]

    update_project(project)
    project["updatedAt"] = now_ms()

    output_path.write_text(format_json(data), encoding="utf-8")
    print(f"wrote {output_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
