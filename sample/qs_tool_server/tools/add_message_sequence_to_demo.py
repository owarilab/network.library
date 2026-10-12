#!/usr/bin/env python3
"""
character_walk_demo.qsproj のメッセージウィンドウを、順次実行（sequence）で進める形に置き換える。
前提: add_message_window_to_demo.py を実行済みであること。

- obj_msg_canvas の msg.show / msg.next の EventAction（P0-2 の進行）を外す
- msg.show を受ける sequence を追加する:
    窓をフェードイン（playTween、waitComplete）
    1 文目（typewriter、waitComplete）→ msg.next を待つ
    2 文目（typewriter、waitComplete）→ msg.next を待つ
    wait 0.3 秒
    3 文目（typewriter、waitComplete）→ msg.next を待つ
    窓を閉じる（setEnabled false）→ completeEventId msg.done
- 枠（NineSlice）の初期 alpha を 0 にして、開始まで見えないようにする

クリックの解釈（表示中ならスキップ、表示後なら msg.next）は msg.click の Conditional が担当する。
進行は sequence が担当する。

同じスクリプトを再実行しても結果は変わる（追加済みのものは置き換える）。
標準ライブラリのみを使う。

使い方:
  python3 -I tools/add_message_sequence_to_demo.py assets/character_walk_demo.qsproj [出力.qsproj]
  出力を省略すると入力を上書きする。
"""

from __future__ import annotations

import json
import sys
import time
from pathlib import Path
from typing import Any

PLAY_UNIT_ID = "pu_character_walk_field"
MSG_CANVAS_ID = "obj_msg_canvas"
MSG_BOX_ID = "obj_msg_box"
MSG_BODY_ID = "obj_msg_body"

BODY_VARIABLE_PATH = "user.fixed.msgBody"
STATUS_VARIABLE_PATH = "user.fixed.msgStatus"
CHARS_PER_SECOND = 24

# P0-2 で msg.show / msg.next に付けた EventAction は、この順次実行に置き換える
REPLACED_LISTEN_IDS = {"msg.show", "msg.next"}

LINE_2 = "二行目です。文字は、表示が終わるまでクリックするとまとめて出ます。"
LINE_3 = "最後の文です。クリックすると窓が閉じます。"


def now_ms() -> int:
    return int(time.time() * 1000)


def make_component(component_type: str, data: dict[str, Any]) -> dict[str, Any]:
    return {"type": component_type, "enabled": True, "data": data}


def find_object(play_unit: dict[str, Any], object_id: str) -> dict[str, Any] | None:
    return next((obj for obj in play_unit["objects"] if obj.get("id") == object_id), None)


def make_typewriter(text_source: dict[str, Any]) -> dict[str, Any]:
    """文字列の出どころ（変数または literal）だけ変えて、typewriter の step を作る。"""
    return {
        "action": "typewriter",
        "targetObjectId": MSG_BODY_ID,
        "componentType": "Text",
        **text_source,
        "charsPerSecond": CHARS_PER_SECOND,
        "statusVariablePath": STATUS_VARIABLE_PATH,
        "waitComplete": True,
    }


def make_wait_for_click() -> dict[str, Any]:
    return {"action": "waitEvent", "eventId": "msg.next"}


def build_sequence_component() -> dict[str, Any]:
    steps = [
        {
            "action": "playTween",
            "targetObjectId": MSG_BOX_ID,
            "componentType": "NineSlice",
            "property": "alpha",
            "tweenFrom": 0,
            "tweenTo": 1,
            "tweenDuration": 300,
            "tweenEasing": "easeOut",
            "waitComplete": True,
        },
        make_typewriter({"valueSource": "variable", "valueVariablePath": BODY_VARIABLE_PATH}),
        make_wait_for_click(),
        make_typewriter({"valueSource": "literal", "value": LINE_2}),
        make_wait_for_click(),
        {"action": "wait", "duration": 0.3},
        make_typewriter({"valueSource": "literal", "value": LINE_3}),
        make_wait_for_click(),
        {"action": "setEnabled", "targetObjectId": MSG_BOX_ID, "enabled": False},
    ]
    return make_component("EventAction", {
        "listenTo": "msg.show",
        "action": "sequence",
        "targetObjectId": "",
        "onBusy": "ignore",
        "completeEventId": "msg.done",
        "steps": steps,
    })


def update_play_unit(project: dict[str, Any]) -> None:
    play_units = project["assets"]["playUnits"]
    play_unit = next((unit for unit in play_units if unit.get("id") == PLAY_UNIT_ID), None)
    if play_unit is None:
        raise SystemExit(f"PlayUnit が見つかりません: {PLAY_UNIT_ID}")

    canvas = find_object(play_unit, MSG_CANVAS_ID)
    box = find_object(play_unit, MSG_BOX_ID)
    if canvas is None or box is None:
        raise SystemExit("先に add_message_window_to_demo.py を実行してください（メッセージウィンドウがありません）")

    canvas["components"] = [
        component for component in canvas["components"]
        if not (component.get("type") == "EventAction"
                and component.get("data", {}).get("listenTo") in REPLACED_LISTEN_IDS)
    ]
    canvas["components"].append(build_sequence_component())

    nine_slices = [component for component in box["components"] if component.get("type") == "NineSlice"]
    if not nine_slices:
        raise SystemExit(f"{MSG_BOX_ID} に NineSlice がありません")
    for component in nine_slices:
        component["data"]["alpha"] = 0


def format_json(data: dict[str, Any]) -> str:
    return json.dumps(data, indent=2, ensure_ascii=False) + "\n"


def main(argv: list[str]) -> int:
    if len(argv) not in (2, 3):
        print(__doc__.strip(), file=sys.stderr)
        return 2

    input_path = Path(argv[1])
    output_path = Path(argv[2]) if len(argv) == 3 else input_path
    data = json.loads(input_path.read_text(encoding="utf-8"))
    project = data["project"]

    update_play_unit(project)
    project["updatedAt"] = now_ms()

    output_path.write_text(format_json(data), encoding="utf-8")
    print(f"wrote {output_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
