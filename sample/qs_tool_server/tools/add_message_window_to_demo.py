#!/usr/bin/env python3
"""
character_walk_demo.qsproj にメッセージウィンドウを追加する。

- 9 スライス用の pixelDocument px_msg_frame（32x32。insets 8/8/8/8）
- グローバル変数 user.fixed.msgBody / user.fixed.msgStatus
- PlayUnit Field に UI（UICanvas）を追加:
    MsgBox（9 スライスの枠、クリックで送る）
      MsgBody（空の Text。タイプライターで文字を入れる）
    起動 0.5 秒後に msg.show → typewriter
    msg.click: 表示中ならスキップ、そうでなければ msg.next → 窓を閉じる

同じスクリプトを再実行しても結果は変わる（追加済みのものは置き換える）。
標準ライブラリのみを使う。

使い方:
  python3 -I tools/add_message_window_to_demo.py assets/character_walk_demo.qsproj [出力.qsproj]
  出力を省略すると入力を上書きする。
"""

from __future__ import annotations

import base64
import json
import struct
import sys
import time
from pathlib import Path
from typing import Any

PLAY_UNIT_ID = "pu_character_walk_field"
FRAME_ID = "px_msg_frame"
FRAME_SIZE = 32
FRAME_INSETS = {"left": 8, "top": 8, "right": 8, "bottom": 8}

MSG_CANVAS_ID = "obj_msg_canvas"
MSG_BOX_ID = "obj_msg_box"
MSG_BODY_ID = "obj_msg_body"
OBJECT_IDS = {MSG_CANVAS_ID, MSG_BOX_ID, MSG_BODY_ID}

MSG_BODY_TEXT = "こんにちは。これはメッセージウィンドウのデモです。9 スライスの枠に、文字が一定の速さで表示されます。"

# パレット（index 0 は透明）
C_OUTLINE = 1
C_BORDER = 2
C_FILL = 3
C_SHADE = 4
PALETTE_ARGB = {
    C_OUTLINE: 0xFF0F172A,
    C_BORDER: 0xFFE2E8F0,
    C_FILL: 0xFF1E293B,
    C_SHADE: 0xFF334155,
}


def now_ms() -> int:
    return int(time.time() * 1000)


def make_frame_pixels() -> list[list[int]]:
    """角を欠けさせた枠。外周は輪郭線、その内側は明るい縁、中は暗い塗り。"""
    size = FRAME_SIZE
    last = size - 1
    # 角を 1 画素ずつ欠けさせて、丸みを出す（透明）
    cut_corners = {(0, 0), (1, 0), (0, 1), (last, 0), (last - 1, 0), (last, 1),
                   (0, last), (1, last), (0, last - 1), (last, last), (last - 1, last), (last, last - 1)}
    pixels = [[C_FILL] * size for _ in range(size)]
    for y in range(size):
        for x in range(size):
            if (x, y) in cut_corners:
                pixels[y][x] = 0
            elif x in (0, last) or y in (0, last):
                pixels[y][x] = C_OUTLINE
            elif x in (1, last - 1) or y in (1, last - 1):
                pixels[y][x] = C_BORDER
            elif y == 2 and 2 <= x <= last - 2:
                pixels[y][x] = C_SHADE
    return pixels


def encode_single_chip_qts(pixels: list[list[int]]) -> bytes:
    """
    PixelDataConverter.createQtsArrayBuffer と同じバイト配置で、1 チップ（1x1）の QTS を作る。
    pixelDocument は 1 チップのタイルセットとして保存される。
    """
    width = len(pixels[0])
    height = len(pixels)
    out = bytearray(b"QSTS")
    out += bytes([0x01, 0x00, 0x00, 0x00])
    out += struct.pack("<HHHH", width, height, 1, 1)

    used_count = max(PALETTE_ARGB) + 1
    out.append(used_count)
    for index in range(32):
        argb = PALETTE_ARGB.get(index, 0)
        out += bytes([(argb >> 16) & 0xFF, (argb >> 8) & 0xFF, argb & 0xFF, (argb >> 24) & 0xFF])

    layer_name = b"Layer 1"
    out.append(1)  # レイヤー数
    out += struct.pack("<H", len(layer_name)) + layer_name
    out += bytes([1, 255])  # visible, opacity
    for y in range(height):
        for x in range(width):
            out.append(pixels[y][x] & 0x1F)
    return bytes(out)


def build_frame_asset() -> dict[str, Any]:
    qts = encode_single_chip_qts(make_frame_pixels())
    return {
        "id": FRAME_ID,
        "type": "pixelDocument",
        "name": "Message Frame",
        "width": FRAME_SIZE,
        "height": FRAME_SIZE,
        "nineSlice": dict(FRAME_INSETS),
        "storage": {
            "type": "embedded",
            "codec": "qts-base64",
            "data": base64.b64encode(qts).decode("ascii"),
        },
    }


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


def build_message_objects() -> list[dict[str, Any]]:
    """
    UI は Transform を持たない（UITransform だけで配置する）。
    """
    canvas = make_object(MSG_CANVAS_ID, "MessageCanvas", None, [MSG_BOX_ID], [
        make_component("UICanvas", {"referenceWidth": 640, "referenceHeight": 360, "sortOrder": 10}),
        # 起動後 0.5 秒で本文の表示を始める
        make_component("Trigger", {"eventId": "msg.show", "triggerOn": "onTimer", "once": True, "duration": 0.5, "targetObjectId": ""}),
        make_component("EventAction", {
            "listenTo": "msg.show",
            "action": "typewriter",
            "targetObjectId": MSG_BODY_ID,
            "componentType": "Text",
            "valueSource": "variable",
            "valueVariablePath": "user.fixed.msgBody",
            "charsPerSecond": 24,
            "statusVariablePath": "user.fixed.msgStatus",
            "completeEventId": "msg.typed",
        }),
        # 表示中ならスキップ（全文表示）。表示済みなら次へ進む
        make_component("Conditional", {
            "listenTo": "msg.click",
            "branches": [{
                "condition": {"type": "equals", "left": "${user.fixed.msgStatus}", "right": "typing"},
                "action": {
                    "action": "typewriter",
                    "targetObjectId": MSG_BODY_ID,
                    "componentType": "Text",
                    "valueSource": "variable",
                    "valueVariablePath": "user.fixed.msgBody",
                    "statusVariablePath": "user.fixed.msgStatus",
                    "completeEventId": "msg.typed",
                },
            }],
            "defaultAction": {"action": "fireEvent", "eventId": "msg.next"},
        }),
        # 次へ進んだら窓を閉じる
        make_component("EventAction", {
            "listenTo": "msg.next",
            "action": "setEnabled",
            "targetObjectId": MSG_BOX_ID,
            "enabled": False,
        }),
    ])
    box = make_object(MSG_BOX_ID, "MessageBox", MSG_CANVAS_ID, [MSG_BODY_ID], [
        make_component("UITransform", {"anchorX": 0.5, "anchorY": 1, "pivotX": 0.5, "pivotY": 1, "x": 0, "y": -16, "width": 560, "height": 120}),
        make_component("NineSlice", {"pixelDocumentId": FRAME_ID, "scale": 2, "alpha": 1}),
        # 大きさ 0 の Collider は UITransform の矩形として扱われ、クリックを受ける
        make_component("Collider", {"shape": "rect", "offsetX": 0, "offsetY": 0, "width": 0, "height": 0, "isTrigger": True}),
        make_component("Trigger", {"eventId": "msg.click", "triggerOn": "click", "once": False, "targetObjectId": ""}),
    ])
    body = make_object(MSG_BODY_ID, "MessageBody", MSG_BOX_ID, [], [
        make_component("UITransform", {"anchorX": 0, "anchorY": 0, "pivotX": 0, "pivotY": 0, "x": 28, "y": 24, "width": 504, "height": 72}),
        make_component("Text", {
            "text": "",
            "font": "20px sans-serif",
            "color": "#f8fafc",
            "alpha": 1,
            "align": "left",
            "baseline": "top",
            "wrap": True,
            "maxWidth": 0,
            "lineHeight": 26,
            "strokeColor": "",
            "strokeWidth": 0,
            "backgroundColor": "",
            "padding": 0,
            "revealChars": -1,
        }),
    ])
    return [canvas, box, body]


def set_global_variables(project: dict[str, Any]) -> None:
    fixed = project.setdefault("globalVariables", {}).setdefault("user", {}).setdefault("fixed", {})
    fixed["msgBody"] = {"type": "string", "initialValue": MSG_BODY_TEXT, "description": "メッセージ本文（typewriter の入力）"}
    fixed["msgStatus"] = {"type": "string", "initialValue": "idle", "description": "typing / done"}


def update_play_unit(project: dict[str, Any]) -> None:
    play_units = project["assets"]["playUnits"]
    play_unit = next((unit for unit in play_units if unit.get("id") == PLAY_UNIT_ID), None)
    if play_unit is None:
        raise SystemExit(f"PlayUnit が見つかりません: {PLAY_UNIT_ID}")

    objects = [obj for obj in play_unit["objects"] if obj.get("id") not in OBJECT_IDS]
    objects.extend(build_message_objects())
    play_unit["objects"] = objects


def replace_asset(items: list[dict[str, Any]], asset: dict[str, Any]) -> None:
    items[:] = [item for item in items if item.get("id") != asset["id"]]
    items.append(asset)


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

    replace_asset(project["assets"]["pixelDocuments"], build_frame_asset())
    set_global_variables(project)
    update_play_unit(project)
    project["updatedAt"] = now_ms()

    output_path.write_text(format_json(data), encoding="utf-8")
    print(f"wrote {output_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
