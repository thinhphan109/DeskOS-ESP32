"""Portable device preferences. Router credentials and pairing keys are excluded."""
from __future__ import annotations
import json
from pathlib import Path
from scene_layout import validate_layout

BOARD = "meodesk-esp32s3-n16r8"
RANGES = {"theme": (0, 2), "fps_profile": (0, 3), "motion_style": (0, 2),
          "focus_work_min": (1, 180), "focus_break_min": (1, 60), "focus_goal": (1, 24),
          "cpu_limit": (50, 99), "gpu_limit": (50, 99), "temp_limit": (55, 110), "battery_limit": (5, 50)}
FLAGS = ("motion", "auto_pages", "quiet")


def validate_backup(data: dict) -> dict:
    if not isinstance(data, dict) or type(data.get("schema")) is not int or data["schema"] != 1 or data.get("board") != BOARD:
        raise ValueError("Unsupported configuration schema or board")
    profile = data.get("profile")
    if not isinstance(profile, dict) or set(profile) != set(RANGES) | set(FLAGS) | {"scene_slots"}:
        raise ValueError("Configuration fields are missing or unknown")
    for key, (lower, upper) in RANGES.items():
        if type(profile[key]) is not int or not lower <= profile[key] <= upper:
            raise ValueError(f"Invalid {key}: expected {lower}..{upper}")
    if any(type(profile[key]) is not bool for key in FLAGS):
        raise ValueError("Configuration flags must be booleans")
    scenes = profile["scene_slots"]
    if not isinstance(scenes, list) or len(scenes) != 4 or any(type(v) is not int or not 0 <= v < 15 for v in scenes) or len(set(scenes)) != 4:
        raise ValueError("Choose four different valid scenes")
    # Only an allowlisted envelope ever reaches disk or the device.
    result = {"schema": 1, "board": BOARD, "profile": dict(profile, scene_slots=list(scenes))}
    if "scene_layout" in data:
        result["scene_layout"] = validate_layout(data["scene_layout"])
    if isinstance(data.get("firmware_version"), str):
        result["firmware_version"] = data["firmware_version"][:40]
    return result


def load_backup(path: str | Path) -> dict:
    with Path(path).open("rb") as file:
        payload = file.read(8193)
    if len(payload) > 8192:
        raise ValueError("Configuration file exceeds 8 KiB")
    return validate_backup(json.loads(payload.decode("utf-8-sig")))


def save_backup(path: str | Path, data: dict) -> None:
    clean = validate_backup(data)
    Path(path).write_text(json.dumps(clean, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
