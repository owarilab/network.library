#!/usr/bin/env python3
"""
character_walk_demo.qsproj にタイルセットとタイルマップを追加する。

- 32x32 タイルセット ts_demo_field（草 / 道 / 壁 / 水。壁と水は通行不可）
- 50x50 マップ map_demo_field（外周は壁。内側は草の平地で、道・壁・池を置く）
- PlayUnit Field の Ground / Pebble を外し、Tilemap オブジェクトを追加
- Player に足元の Collider を追加し、草の上に配置し直す

同じスクリプトを再実行しても結果は変わる（追加済みのものは置き換える）。
標準ライブラリのみを使う。

使い方:
  python3 -I tools/add_tilemap_to_character_demo.py assets/character_walk_demo.qsproj [出力.qsproj]
  出力を省略すると入力を上書きする。
"""

from __future__ import annotations

import base64
import json
import re
import struct
import sys
import time
from pathlib import Path
from typing import Any

PLAY_UNIT_ID = "pu_character_walk_field"
TILESET_ID = "ts_demo_field"
MAP_ID = "map_demo_field"
TILEMAP_OBJECT_ID = "obj_field_map"
PLAYER_OBJECT_ID = "obj_player"
REMOVED_OBJECT_IDS = {"ground", *(f"pebble_{i}" for i in range(8))}

CHIP_SIZE = 32
TILESET_COLUMNS = 4
TILESET_ROWS = 1
MAP_SIZE = 50
PLAYER_START = (656.0, 666.0)  # 草の上（セル 20,20 の足元）

# タイルセット内の位置（column）。マップの tiles にはこの index を入れる
TILE_GRASS = 0
TILE_ROAD = 1
TILE_WALL = 2
TILE_WATER = 3
TILE_PASSABLE = {TILE_GRASS: 1, TILE_ROAD: 1, TILE_WALL: 0, TILE_WATER: 0}

# パレット（index 0 は透明）
C_OUTLINE = 1
C_WHITE = 2
C_GRASS = 3
C_GRASS_DARK = 4
C_GRASS_LIGHT = 5
C_ROAD = 6
C_ROAD_DARK = 7
C_ROAD_LIGHT = 8
C_STONE = 9
C_MORTAR = 10
C_STONE_LIGHT = 11
C_WATER = 12
C_WATER_DARK = 13
C_WATER_LIGHT = 14
PALETTE_ARGB = {
    C_OUTLINE: 0xFF1F2937,
    C_WHITE: 0xFFFFFFFF,
    C_GRASS: 0xFF5FA84A,
    C_GRASS_DARK: 0xFF3F8A35,
    C_GRASS_LIGHT: 0xFF7CC45E,
    C_ROAD: 0xFFC8A46E,
    C_ROAD_DARK: 0xFFA9854F,
    C_ROAD_LIGHT: 0xFFDCBC8C,
    C_STONE: 0xFF8A8F98,
    C_MORTAR: 0xFF5C616B,
    C_STONE_LIGHT: 0xFFB5BAC2,
    C_WATER: 0xFF3B82F6,
    C_WATER_DARK: 0xFF1D4ED8,
    C_WATER_LIGHT: 0xFF93C5FD,
}


def now_ms() -> int:
    return int(time.time() * 1000)


def hash2(x: int, y: int, salt: int) -> int:
    """座標から決まる疑似乱数（描画を毎回同じにするため）。"""
    h = (x * 374761393 + y * 668265263 + salt * 2246822519) & 0xFFFFFFFF
    h = ((h ^ (h >> 13)) * 1274126177) & 0xFFFFFFFF
    return (h ^ (h >> 16)) & 0xFFFFFFFF


def make_grass_chip() -> list[list[int]]:
    pixels = [[C_GRASS] * CHIP_SIZE for _ in range(CHIP_SIZE)]
    for y in range(CHIP_SIZE):
        for x in range(CHIP_SIZE):
            roll = hash2(x, y, 1) % 100
            if roll < 6:
                pixels[y][x] = C_GRASS_DARK
            elif roll < 10:
                pixels[y][x] = C_GRASS_LIGHT
    return pixels


def make_road_chip() -> list[list[int]]:
    pixels = [[C_ROAD] * CHIP_SIZE for _ in range(CHIP_SIZE)]
    for y in range(CHIP_SIZE):
        for x in range(CHIP_SIZE):
            roll = hash2(x, y, 2) % 100
            if roll < 5:
                pixels[y][x] = C_ROAD_DARK
            elif roll < 8:
                pixels[y][x] = C_ROAD_LIGHT
    return pixels


def make_wall_chip() -> list[list[int]]:
    """レンガ状の石壁。外周は輪郭線にする。"""
    pixels = [[C_STONE] * CHIP_SIZE for _ in range(CHIP_SIZE)]
    last = CHIP_SIZE - 1
    for y in range(CHIP_SIZE):
        for x in range(CHIP_SIZE):
            if x in (0, last) or y in (0, last):
                pixels[y][x] = C_OUTLINE
                continue
            if y % 8 == 7:
                pixels[y][x] = C_MORTAR
                continue
            offset = 0 if (y // 8) % 2 == 0 else 8
            if (x + offset) % 16 == 15:
                pixels[y][x] = C_MORTAR
            elif (x + offset) % 16 == 1 and y % 8 == 1:
                pixels[y][x] = C_STONE_LIGHT
    return pixels


def make_water_chip() -> list[list[int]]:
    pixels = [[C_WATER] * CHIP_SIZE for _ in range(CHIP_SIZE)]
    for y in range(CHIP_SIZE):
        phase = y % 8
        for x in range(CHIP_SIZE):
            if phase in (2, 3) and (x + (y // 8) * 5) % 10 < 4:
                pixels[y][x] = C_WATER_LIGHT
            elif phase == 6 and (x + 3) % 12 < 5:
                pixels[y][x] = C_WATER_DARK
    return pixels


def encode_qts(chips: list[list[tuple[list[list[int]], int]]]) -> bytes:
    """
    PixelDataConverter.createQtsArrayBuffer と同じバイト配置で QTS を作る。
    chips[row][col] = (ピクセル配列 [y][x] のパレット index, 通過フラグ 0/1)
    """
    out = bytearray(b"QSTS")
    out += bytes([0x01, 0x00, 0x00, 0x00])
    out += struct.pack("<HHHH", CHIP_SIZE, CHIP_SIZE, TILESET_COLUMNS, TILESET_ROWS)

    # パレット: 有効色数 N（index 0 を含む）と 32 色の RGBA
    used_count = max(PALETTE_ARGB) + 1
    out.append(used_count)
    for index in range(32):
        argb = PALETTE_ARGB.get(index, 0)
        out += bytes([(argb >> 16) & 0xFF, (argb >> 8) & 0xFF, argb & 0xFF, (argb >> 24) & 0xFF])

    layer_name = b"Layer 1"
    for row in range(TILESET_ROWS):
        for col in range(TILESET_COLUMNS):
            pixels, passable = chips[row][col]
            out.append(1)  # レイヤー数
            out += struct.pack("<H", len(layer_name)) + layer_name
            out += bytes([1, 255])  # visible, opacity
            for y in range(CHIP_SIZE):
                for x in range(CHIP_SIZE):
                    out.append(((passable & 1) << 7) | (pixels[y][x] & 0x1F))
    return bytes(out)


def build_tileset_asset() -> dict[str, Any]:
    chips = [[
        (make_grass_chip(), TILE_PASSABLE[TILE_GRASS]),
        (make_road_chip(), TILE_PASSABLE[TILE_ROAD]),
        (make_wall_chip(), TILE_PASSABLE[TILE_WALL]),
        (make_water_chip(), TILE_PASSABLE[TILE_WATER]),
    ]]
    qts = encode_qts(chips)
    return {
        "id": TILESET_ID,
        "type": "tileset",
        "name": "Field Tiles",
        "chipWidth": CHIP_SIZE,
        "chipHeight": CHIP_SIZE,
        "columns": TILESET_COLUMNS,
        "rows": TILESET_ROWS,
        "storage": {
            "type": "embedded",
            "codec": "qts-base64",
            "data": base64.b64encode(qts).decode("ascii"),
        },
    }


def build_map_tiles() -> list[int]:
    tiles = [TILE_GRASS] * (MAP_SIZE * MAP_SIZE)

    def fill(x0: int, y0: int, x1: int, y1: int, tile: int) -> None:
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                tiles[y * MAP_SIZE + x] = tile

    # 道: 横に 2 本（行 24-25）、縦に 2 本（列 24-25）
    fill(1, 24, MAP_SIZE - 2, 25, TILE_ROAD)
    fill(24, 1, 25, MAP_SIZE - 2, TILE_ROAD)
    # 外周の壁（マップ外へ出られないようにする）
    fill(0, 0, MAP_SIZE - 1, 0, TILE_WALL)
    fill(0, MAP_SIZE - 1, MAP_SIZE - 1, MAP_SIZE - 1, TILE_WALL)
    fill(0, 0, 0, MAP_SIZE - 1, TILE_WALL)
    fill(MAP_SIZE - 1, 0, MAP_SIZE - 1, MAP_SIZE - 1, TILE_WALL)
    # 建物の壁、縦の壁、小さな壁
    fill(8, 8, 13, 10, TILE_WALL)
    fill(36, 6, 36, 18, TILE_WALL)
    fill(16, 35, 17, 40, TILE_WALL)
    # 池
    fill(34, 34, 39, 38, TILE_WATER)
    return tiles


def build_map_asset() -> dict[str, Any]:
    return {
        "id": MAP_ID,
        "type": "map",
        "name": "Field Map",
        "width": MAP_SIZE,
        "height": MAP_SIZE,
        "mapData": {
            "version": 1,
            "width": MAP_SIZE,
            "height": MAP_SIZE,
            "tileWidth": CHIP_SIZE,
            "tileHeight": CHIP_SIZE,
            "tilesetId": TILESET_ID,
            "layers": [{
                "id": "layer_ground",
                "name": "Ground",
                "visible": True,
                "locked": False,
                "tiles": build_map_tiles(),
            }],
            "selectedLayer": 0,
            "selectedTileRef": {"tilesetId": TILESET_ID, "col": 0, "row": 0, "index": TILE_GRASS},
            "cursor": {"x": 0, "y": 0},
            "view": {"showGrid": True, "zoom": 1},
        },
    }


def make_component(component_type: str, data: dict[str, Any]) -> dict[str, Any]:
    return {"type": component_type, "enabled": True, "data": data}


def make_tilemap_object() -> dict[str, Any]:
    return {
        "id": TILEMAP_OBJECT_ID,
        "name": "FieldMap",
        "enabled": True,
        "parentId": None,
        "children": [],
        "components": [
            make_component("Transform", {"x": 0, "y": 0, "z": 0, "rotation": 0, "scaleX": 1, "scaleY": 1}),
            make_component("Tilemap", {
                "mapAssetId": MAP_ID,
                "layerId": "",
                "scale": 1,
                "offsetX": 0,
                "offsetY": 0,
            }),
        ],
    }


def update_player(obj: dict[str, Any]) -> None:
    """Player を草の上に置き、足元の Collider を付ける。"""
    for component in obj["components"]:
        if component["type"] == "Transform":
            component["data"]["x"], component["data"]["y"] = PLAYER_START
    obj["components"] = [c for c in obj["components"] if c["type"] != "Collider"]
    obj["components"].append(make_component("Collider", {
        "shape": "rect",
        "offsetX": -6,
        "offsetY": -6,
        "width": 12,
        "height": 6,
        "isTrigger": False,
    }))


def update_play_unit(project: dict[str, Any]) -> None:
    play_units = project["assets"]["playUnits"]
    play_unit = next((unit for unit in play_units if unit.get("id") == PLAY_UNIT_ID), None)
    if play_unit is None:
        raise SystemExit(f"PlayUnit が見つかりません: {PLAY_UNIT_ID}")

    objects = [obj for obj in play_unit["objects"]
               if obj.get("id") not in REMOVED_OBJECT_IDS and obj.get("id") != TILEMAP_OBJECT_ID]
    player = next((obj for obj in objects if obj.get("id") == PLAYER_OBJECT_ID), None)
    if player is None:
        raise SystemExit(f"Player オブジェクトが見つかりません: {PLAYER_OBJECT_ID}")
    update_player(player)

    # Tilemap は先頭に置く（描画順は z で決まるため、ここでは見やすさだけを考える）
    objects.insert(0, make_tilemap_object())
    play_unit["objects"] = objects


def replace_asset(items: list[dict[str, Any]], asset: dict[str, Any]) -> None:
    items[:] = [item for item in items if item.get("id") != asset["id"]]
    items.append(asset)


def format_json(data: dict[str, Any]) -> str:
    """
    indent 2 で出力し、タイル配列だけは 1 行 1 マップ行にまとめる（2500 行に広がるのを防ぐ）。
    """
    text = json.dumps(data, indent=2, ensure_ascii=False) + "\n"

    def compact(match: re.Match[str]) -> str:
        indent = match.group(1)
        numbers = re.findall(r"-?\d+", match.group(2))
        rows = [", ".join(numbers[i:i + MAP_SIZE]) for i in range(0, len(numbers), MAP_SIZE)]
        body = ",\n".join(f"{indent}  {row}" for row in rows)
        return f'{indent}"tiles": [\n{body}\n{indent}]'

    return re.sub(r'^( *)"tiles": \[([^\]]*)\]', compact, text, flags=re.M)


def main(argv: list[str]) -> int:
    if len(argv) not in (2, 3):
        print(__doc__.strip(), file=sys.stderr)
        return 2

    input_path = Path(argv[1])
    output_path = Path(argv[2]) if len(argv) == 3 else input_path
    data = json.loads(input_path.read_text(encoding="utf-8"))
    project = data["project"]
    assets = project["assets"]

    replace_asset(assets["tilesets"], build_tileset_asset())
    replace_asset(assets["maps"], build_map_asset())
    update_play_unit(project)
    project["updatedAt"] = now_ms()

    output_path.write_text(format_json(data), encoding="utf-8")
    print(f"wrote {output_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
