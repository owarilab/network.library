#!/usr/bin/env python3
"""
character_walk_demo.qsproj にデータベース（project.database）の使用例を追加する。
前提: add_tilemap_to_character_demo.py を実行済みであること（PlayUnit Field がある）。

データベース:
- 定数 DB items（const）: アイテム。label / heal / price / description
- 定数 DB hazards（const）: 罠。label / damage
- 可変 DB party（mutable）: パーティ。label / hp / maxHp / mp / level / held（items の行を指す ref）

PlayUnit Field に追加するもの:
- HUD（UICanvas）: HP を ${db.mutable.party.hero.hp} で表示し、ポーションのボタンを置く
    ボタンを押す → HP が最大より少ないなら、items.potion.heal だけ回復
- 罠（赤い Rectangle）: 踏むと hazards.spike.damage だけ HP が減る
- 回復・ダメージの後は party.clamp で、HP を 0〜maxHp に収める

定数 DB（db.const...）は実行中に変更できず、可変 DB（db.mutable...）は Play Test のたびに初期値へ戻る。

同じスクリプトを再実行しても結果は変わる（追加済みのものは置き換える）。
標準ライブラリのみを使う。

使い方:
  python3 -I tools/add_database_to_demo.py assets/character_walk_demo.qsproj [出力.qsproj]
  出力を省略すると入力を上書きする。
"""

from __future__ import annotations

import json
import sys
import time
from pathlib import Path
from typing import Any

PLAY_UNIT_ID = "pu_character_walk_field"

HUD_CANVAS_ID = "obj_hud_canvas"
HUD_HP_ID = "obj_hud_hp"
HUD_POTION_ID = "obj_hud_potion"
HUD_POTION_LABEL_ID = "obj_hud_potion_label"
HAZARD_ID = "obj_hazard_spike"
OBJECT_IDS = {HUD_CANVAS_ID, HUD_HP_ID, HUD_POTION_ID, HUD_POTION_LABEL_ID, HAZARD_ID}

TABLE_IDS = {"tbl_items", "tbl_hazards", "tbl_party"}
HERO = "db.mutable.party.hero"
HP_PATH = f"{HERO}.hp"
MAX_HP_PATH = f"{HERO}.maxHp"

# 罠の位置（Field の草の上。プレイヤーの開始位置のすぐ右）
HAZARD_X = 704
HAZARD_Y = 640


def now_ms() -> int:
    return int(time.time() * 1000)


def make_component(component_type: str, data: dict[str, Any]) -> dict[str, Any]:
    return {"type": component_type, "enabled": True, "data": data}


def make_object(object_id: str, name: str, parent_id: str | None, children: list[str], components: list[dict[str, Any]]) -> dict[str, Any]:
    return {"id": object_id, "name": name, "enabled": True, "parentId": parent_id, "children": children, "components": components}


# ---------------------------------------------------------------------------
# データベース
# ---------------------------------------------------------------------------

def column(column_id: str, name: str, column_type: str, default: Any, label: str = "", **extra: Any) -> dict[str, Any]:
    return {"id": column_id, "name": name, "label": label, "type": column_type, "default": default, **extra}


def build_tables() -> list[dict[str, Any]]:
    items = {
        "id": "tbl_items", "name": "items", "label": "アイテム", "kind": "const",
        "columns": [
            column("col_items_label", "label", "string", "", "名前"),
            column("col_items_heal", "heal", "integer", 0, "回復量"),
            column("col_items_price", "price", "integer", 0, "価格"),
            column("col_items_description", "description", "text", "", "説明"),
        ],
        "rows": [
            {"key": "potion", "cells": {"label": "ポーション", "heal": 30, "price": 50, "description": "HP を 30 回復する"}},
            {"key": "hi_potion", "cells": {"label": "ハイポーション", "heal": 80, "price": 150, "description": "HP を 80 回復する"}},
            {"key": "antidote", "cells": {"label": "どくけし", "heal": 0, "price": 30, "description": "毒を治す"}},
        ],
    }
    hazards = {
        "id": "tbl_hazards", "name": "hazards", "label": "罠", "kind": "const",
        "columns": [
            column("col_hazards_label", "label", "string", "", "名前"),
            column("col_hazards_damage", "damage", "integer", 0, "ダメージ"),
        ],
        "rows": [
            {"key": "spike", "cells": {"label": "トゲ", "damage": 8}},
            {"key": "lava", "cells": {"label": "溶岩", "damage": 20}},
        ],
    }
    party = {
        "id": "tbl_party", "name": "party", "label": "パーティ", "kind": "mutable",
        "columns": [
            column("col_party_label", "label", "string", "", "名前"),
            column("col_party_hp", "hp", "integer", 30, "HP"),
            column("col_party_maxhp", "maxHp", "integer", 30, "最大 HP"),
            column("col_party_mp", "mp", "integer", 10, "MP"),
            column("col_party_level", "level", "integer", 1, "レベル"),
            column("col_party_held", "held", "ref", "potion", "所持アイテム", refTableId="tbl_items"),
        ],
        "rows": [
            {"key": "hero", "cells": {"label": "ゆうしゃ", "hp": 60, "maxHp": 100, "mp": 12, "level": 1, "held": "potion"}},
            {"key": "mage", "cells": {"label": "まほうつかい", "hp": 40, "maxHp": 40, "mp": 30, "level": 1, "held": "antidote"}},
        ],
    }
    return [items, hazards, party]


def set_database(project: dict[str, Any]) -> None:
    database = project.get("database")
    if not isinstance(database, dict):
        database = {"version": 1, "tables": []}
    database["tables"] = [table for table in database.get("tables", []) if table.get("id") not in TABLE_IDS]
    database["tables"].extend(build_tables())
    project["database"] = database


# ---------------------------------------------------------------------------
# PlayUnit（HUD、罠、イベント）
# ---------------------------------------------------------------------------

def add_to_hp(op: str, source_path: str) -> dict[str, Any]:
    """HP に、別のセルの値を足す / 引く。"""
    return {"action": "setGlobalVariable", "variablePath": HP_PATH, "op": op, "valueSource": "variable", "valueVariablePath": source_path}


def build_objects() -> list[dict[str, Any]]:
    text_base = {
        "font": "18px sans-serif", "color": "#f8fafc", "alpha": 1, "align": "left", "baseline": "middle", "wrap": False,
        "maxWidth": 0, "lineHeight": 24, "strokeColor": "#0f172a", "strokeWidth": 3, "backgroundColor": "", "padding": 0, "revealChars": -1,
    }
    canvas = make_object(HUD_CANVAS_ID, "HudCanvas", None, [HUD_HP_ID, HUD_POTION_ID], [
        make_component("UICanvas", {"referenceWidth": 640, "referenceHeight": 360, "sortOrder": 5}),
        # ポーション: HP が最大より少ないときだけ回復する。回復後は party.clamp で最大 HP に収める
        make_component("Conditional", {
            "listenTo": "hud.potion",
            "branches": [{
                "condition": {"type": "compare", "left": f"${{{HP_PATH}}}", "operator": "<", "right": f"${{{MAX_HP_PATH}}}"},
                "action": {
                    "action": "sequence",
                    "steps": [add_to_hp("add", "db.const.items.potion.heal"), {"action": "fireEvent", "eventId": "party.clamp"}],
                },
            }],
        }),
        # HP を 0 〜 最大 HP に収める
        make_component("Conditional", {
            "listenTo": "party.clamp",
            "branches": [
                {
                    "condition": {"type": "compare", "left": f"${{{HP_PATH}}}", "operator": ">", "right": f"${{{MAX_HP_PATH}}}"},
                    "action": {"action": "setGlobalVariable", "variablePath": HP_PATH, "op": "set", "valueSource": "variable", "valueVariablePath": MAX_HP_PATH},
                },
                {
                    "condition": {"type": "compare", "left": f"${{{HP_PATH}}}", "operator": "<", "right": 0},
                    "action": {"action": "setGlobalVariable", "variablePath": HP_PATH, "op": "set", "valueSource": "literal", "value": 0},
                },
            ],
        }),
        # 罠: 定数 DB の damage だけ HP を減らす
        make_component("EventAction", {"listenTo": "hazard.spike", **add_to_hp("subtract", "db.const.hazards.spike.damage")}),
        make_component("EventAction", {"listenTo": "hazard.spike", "action": "fireEvent", "eventId": "party.clamp"}),
    ])
    hp = make_object(HUD_HP_ID, "HudHp", HUD_CANVAS_ID, [], [
        make_component("UITransform", {"anchorX": 0, "anchorY": 0, "pivotX": 0, "pivotY": 0, "x": 16, "y": 14, "width": 360, "height": 28}),
        make_component("Text", {**text_base, "text": f"${{{HERO}.label}}  HP ${{{HP_PATH}}} / ${{{MAX_HP_PATH}}}   Lv ${{{HERO}.level}}"}),
    ])
    potion = make_object(HUD_POTION_ID, "HudPotionButton", HUD_CANVAS_ID, [HUD_POTION_LABEL_ID], [
        make_component("UITransform", {"anchorX": 0, "anchorY": 0, "pivotX": 0, "pivotY": 0, "x": 16, "y": 46, "width": 220, "height": 32}),
        make_component("Rectangle", {
            "shape": "rectangle", "width": 220, "height": 32, "fillColor": "#1e293b", "fillAlpha": 0.9, "strokeColor": "#38bdf8",
            "strokeWidth": 2, "strokeAlpha": 1, "rotation": 0, "originX": 0, "originY": 0, "sides": 4, "points": 5, "innerRadius": 0.4,
        }),
        make_component("Collider", {"shape": "rect", "offsetX": 0, "offsetY": 0, "width": 0, "height": 0, "isTrigger": True}),
        make_component("Trigger", {"eventId": "hud.potion", "triggerOn": "click", "once": False, "targetObjectId": ""}),
    ])
    potion_label = make_object(HUD_POTION_LABEL_ID, "HudPotionLabel", HUD_POTION_ID, [], [
        make_component("UITransform", {"anchorX": 0, "anchorY": 0, "pivotX": 0, "pivotY": 0, "x": 12, "y": 2, "width": 200, "height": 28}),
        make_component("Text", {**text_base, "strokeWidth": 0, "text": "${db.const.items.potion.label} (+${db.const.items.potion.heal})"}),
    ])
    hazard = make_object(HAZARD_ID, "SpikeTrap", None, [], [
        make_component("Transform", {"x": HAZARD_X, "y": HAZARD_Y, "z": 5, "rotation": 0, "scaleX": 1, "scaleY": 1}),
        make_component("Rectangle", {
            "shape": "rectangle", "width": 32, "height": 32, "fillColor": "#b91c1c", "fillAlpha": 0.85, "strokeColor": "#7f1d1d",
            "strokeWidth": 2, "strokeAlpha": 1, "rotation": 0, "originX": 0, "originY": 0, "sides": 4, "points": 5, "innerRadius": 0.4,
        }),
        make_component("Collider", {"shape": "rect", "offsetX": 0, "offsetY": 0, "width": 32, "height": 32, "isTrigger": True}),
        make_component("Trigger", {"eventId": "hazard.spike", "triggerOn": "overlap", "once": False, "targetObjectId": ""}),
    ])
    return [canvas, hp, potion, potion_label, hazard]


def update_play_unit(project: dict[str, Any]) -> None:
    play_unit = next((unit for unit in project["assets"]["playUnits"] if unit.get("id") == PLAY_UNIT_ID), None)
    if play_unit is None:
        raise SystemExit(f"PlayUnit が見つかりません: {PLAY_UNIT_ID}（先に add_tilemap_to_character_demo.py を実行してください）")
    if not any(obj.get("id") == "obj_player" for obj in play_unit["objects"]):
        raise SystemExit("Player オブジェクトがありません（character_walk_demo の構成ではありません）")
    objects = [obj for obj in play_unit["objects"] if obj.get("id") not in OBJECT_IDS]
    objects.extend(build_objects())
    play_unit["objects"] = objects


def format_json(data: dict[str, Any]) -> str:
    # アプリの保存形式（indent 2）と同じにして、差分を追加分だけにする
    return json.dumps(data, indent=2, ensure_ascii=False) + "\n"


def main(argv: list[str]) -> int:
    if len(argv) not in (2, 3):
        print(__doc__.strip(), file=sys.stderr)
        return 2

    input_path = Path(argv[1])
    output_path = Path(argv[2]) if len(argv) == 3 else input_path
    data = json.loads(input_path.read_text(encoding="utf-8"))
    project = data["project"]

    set_database(project)
    update_play_unit(project)
    project["updatedAt"] = now_ms()

    output_path.write_text(format_json(data), encoding="utf-8")
    print(f"wrote {output_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
