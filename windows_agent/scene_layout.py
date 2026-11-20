"""Scene layout contract shared by local editors and firmware."""
from __future__ import annotations
import json
from pathlib import Path

KINDS = ("Clock", "CPU", "GPU", "RAM", "Focus", "Music", "Network", "Text")
DEFAULT_LAYOUT = {"schema": 1, "widgets": [
    {"kind": 0, "x": 0, "y": 0, "w": 162, "h": 40, "label": "Clock"},
    {"kind": 1, "x": 0, "y": 42, "w": 80, "h": 36, "label": "CPU"},
    {"kind": 3, "x": 82, "y": 42, "w": 80, "h": 36, "label": "RAM"},
    {"kind": 4, "x": 0, "y": 80, "w": 162, "h": 40, "label": "Focus"}]}


def validate_layout(data: dict) -> dict:
    if not isinstance(data, dict) or type(data.get("schema")) is not int or data["schema"] != 1:
        raise ValueError("Unsupported scene schema")
    widgets = data.get("widgets")
    if not isinstance(widgets, list) or not 1 <= len(widgets) <= 6:
        raise ValueError("Choose 1..6 widgets")
    result = []
    for item in widgets:
        if not isinstance(item, dict) or set(item) != {"kind", "x", "y", "w", "h", "label"}:
            raise ValueError("Invalid widget fields")
        for key, low, high in (("kind", 0, 7), ("x", 0, 161), ("y", 0, 119), ("w", 32, 162), ("h", 20, 120)):
            if type(item[key]) is not int or not low <= item[key] <= high:
                raise ValueError(f"Invalid widget {key}")
        if item["x"] + item["w"] > 162 or item["y"] + item["h"] > 120:
            raise ValueError("Widget extends beyond 162×120")
        label = item["label"]
        if not isinstance(label, str) or len(label) > 31 or any(not 32 <= ord(c) <= 126 for c in label):
            raise ValueError("TFT label must contain up to 31 printable ASCII characters")
        for other in result:
            if (item["x"] < other["x"] + other["w"] and other["x"] < item["x"] + item["w"]
                    and item["y"] < other["y"] + other["h"] and other["y"] < item["y"] + item["h"]):
                raise ValueError("Widgets overlap")
        result.append(dict(item))
    return {"schema": 1, "widgets": result}


def load_layout(path: str | Path) -> dict:
    with Path(path).open("rb") as stream:
        payload = stream.read(8193)
    if len(payload) > 8192:
        raise ValueError("Scene file exceeds 8 KiB")
    return validate_layout(json.loads(payload.decode("utf-8-sig")))


def save_layout(path: str | Path, data: dict) -> None:
    Path(path).write_text(json.dumps(validate_layout(data), indent=2) + "\n", encoding="utf-8")
