#!/usr/bin/env python3
"""Local mock of the firmware web UI, for visual QA without an ESP32.

Run from any directory: python tools/web_preview_server.py
Open http://127.0.0.1:8765/ . Binds to loopback only, fake data is clearly
marked DEMO / SIMULATED, and never talks to the actual ESP32.
"""

from __future__ import annotations

import json
import argparse
import sys
import math
import re
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, urlparse

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT.parent / "windows_agent"))
from device_config import validate_backup
from scene_layout import DEFAULT_LAYOUT, validate_layout
import copy
from app_version import VERSION
HEADER = (ROOT / "src" / "web_ui.h").read_text(encoding="utf-8")
MATCH = re.search(r'R"MEODESK\((.*?)\)MEODESK"', HEADER, re.DOTALL)
if MATCH is None:
    raise SystemExit("Embedded web UI not found")
WEB = MATCH.group(1).encode("utf-8")


def current_web() -> bytes:
    """Reload the embedded UI on refresh while editing; no mock restart needed."""
    source = (ROOT / "src" / "web_ui.h").read_text(encoding="utf-8")
    match = re.search(r'R"MEODESK\((.*?)\)MEODESK"', source, re.DOTALL)
    if match is None:
        raise ValueError("Embedded web UI not found")
    return match.group(1).encode("utf-8")
START = time.monotonic()
MOODS = ["paused", "low_battery", "rescue", "power", "sleep", "hot", "healthy", "hotspot"]
ART = {"paused": "paused", "low_battery": "shutdown", "rescue": "rescue", "power": "power", "sleep": "sleep", "hot": "health", "healthy": "healthy", "hotspot": "hotspot", "stressed": "storage", "demo": "music"}
STATE = {"scene_layout": copy.deepcopy(DEFAULT_LAYOUT), "scene_layout_revision": 0, "page": 12, "motion": True, "auto_pages": False, "focus_running": False,
         "lab_enabled": False,
         "focus_completed_total": 0, "focus_last_completed_min": 0, "focus_work_min": 25, "focus_break_min": 5, "focus_goal": 4,
         "scene_slots": [12, 0, 5, 2],
         "pet_energy": 82, "pet_until": 0.0,
         "focus_phase": "work", "focus_remaining_s": 1500, "focus_sessions": 0,
         "focus_mark": time.monotonic(), "theme": 0, "fps_profile": 1,
         "motion_style": 1,
         "preview": -1, "preview_until": 0.0,
         "command_seq": 0, "command_acknowledged": False, "command_ok": False,
         "settings": {"cpu_limit": 88, "gpu_limit": 92, "temp_limit": 80,
                      "battery_limit": 20, "quiet": False}}


def effective_fps_target() -> int:
    """Mock starts AUTO at 25; real firmware may lower it to 20 under load."""
    return (20, 25, 30, 25)[STATE["fps_profile"]]


def focus_step() -> None:
    now = time.monotonic()
    if STATE["focus_running"]:
        elapsed = int(now - STATE["focus_mark"])
        if elapsed:
            STATE["focus_remaining_s"] -= elapsed
            if STATE["focus_remaining_s"] <= 0:
                if STATE["focus_phase"] == "work":
                    STATE["focus_sessions"] += 1
                    STATE["focus_completed_total"] += 1
                    STATE["focus_last_completed_min"] = STATE["focus_work_min"]
                    STATE["focus_phase"] = "break"
                    STATE["focus_remaining_s"] = STATE["focus_break_min"] * 60
                else:
                    STATE["focus_phase"] = "work"
                    STATE["focus_remaining_s"] = STATE["focus_work_min"] * 60
                STATE["focus_running"] = False
    STATE["focus_mark"] = now


def change_focus(action: str) -> bool:
    focus_step()
    if action in ("focus", "toggle"):
        STATE["focus_running"] = not STATE["focus_running"]
    elif action in ("focus_reset", "reset"):
        STATE.update(focus_running=False, focus_phase="work", focus_remaining_s=STATE["focus_work_min"]*60)
    elif action in ("focus_skip", "skip"):
        if STATE["focus_phase"] == "work":
            STATE["focus_phase"] = "break"
            STATE["focus_remaining_s"] = STATE["focus_break_min"]*60
        else:
            STATE["focus_phase"] = "work"
            STATE["focus_remaining_s"] = STATE["focus_work_min"]*60
    else:
        return False
    STATE["focus_mark"] = time.monotonic()
    return True


def sample(t: float) -> tuple[int, int, int]:
    return (round(40 + 29 * math.sin(t / 17) + 9 * math.sin(t / 5)),
            round(47 + 24 * math.sin(t / 20 + 1)),
            round(63 + 6 * math.sin(t / 70)))


class Handler(BaseHTTPRequestHandler):
    def log_message(self, fmt: str, *args: object) -> None:
        pass

    def respond(self, body: bytes, mime: str, code: int = 200) -> None:
        self.send_response(code)
        self.send_header("Content-Type", mime)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def reply(self, value: dict, code: int = 200) -> None:
        self.respond(json.dumps(value).encode("utf-8"), "application/json", code)

    def do_GET(self) -> None:
        url = urlparse(self.path)
        t = time.monotonic() - START
        if url.path == "/":
            return self.respond(current_web(), "text/html; charset=utf-8")
        if url.path == "/api/cat.bmp":
            mode = parse_qs(url.query).get("state", ["demo"])[0]
            file = ROOT / "images" / (ART.get(mode, "music") + ".png")
            return self.respond(file.read_bytes(), "image/png")
        if url.path == "/api/history":
            # 5 minutes of explicitly simulated measurements, including gaps.
            rows = [sample(t - (299 - i)) for i in range(300)]
            return self.reply({"period_s": 1, "capacity": 300, "count": 300, "mode": "demo",
                               "cpu": [v[0] for v in rows], "gpu": [v[1] for v in rows],
                               "ram": [v[2] for v in rows]})
        if url.path == "/api/v1/performance":
            target = effective_fps_target()
            fps = [round(max(0.0, target - 0.6 - 0.8 * abs(math.sin((t - (59 - i) * 5) / 31))), 1)
                   for i in range(60)]
            return self.reply({"schema": 1, "sample_period_s": 5, "count": len(fps),
                               "mode": "demo", "fps": fps, "target": [target] * 60,
                               "skipped": [0] * 60, "worst_ms": [17] * 60,
                               "internal_kb": [186] * 60, "psram_kb": [7890] * 60})
        if url.path == "/api/v1/ota/status":
            # Mock NEVER writes firmware. Display the USB migration requirement.
            return self.reply({"schema": 1, "board": "meodesk-esp32s3-n16r8",
                               "version": VERSION + "-mock", "ready": False, "phase": "idle",
                               "message": "DEMO / no actual flash", "written": 0, "expected": 0,
                               "flash_bytes": 16777216, "inactive_slot_bytes": 0,
                               "boot_health": "passed", "uptime_ms": int(t * 1000),
                               "sha256_required": True, "signed_image_required": False,
                               "bootloader_rollback_configured": False,
                               "running_slot": "mock", "inactive_slot": "N/A"})
        if url.path == "/api/v1/catpack/status":
            return self.reply({"schema": 1, "board": "meodesk-esp32s3-n16r8",
                               "ready": False, "motion_ready": False,
                               "source": "builtin", "active_bank": -1,
                               "generation": 0, "sha256": "", "frame_count": 16,
                               "frames_per_pose": 1, "active_pack_bytes": 0,
                               "width": 144, "height": 144, "pack_bytes": 663552,
                               "accepted_pack_bytes": [663552, 1990656],
                               "phase": "idle", "message": "DEMO / mock cannot flash"})
        if url.path in ("/api/media", "/api/v1/media"):
            return self.reply({"available": True, "playing": True, "stale": False,
                               "title": "DEMO TRACK / NOT REAL", "artist": "Mock Windows Agent",
                               "artwork_id": "", "artwork_ready": False,
                               "position_ms": round(t * 1000) % 160000, "duration_ms": 160000})
        if url.path == "/api/v1/artwork.bmp":
            return self.reply({"ok": False, "error": "no real album cover in mock mode"}, 404)
        if url.path == "/api/v1/events":
            return self.reply({"schema": 1, "latest_seq": 1,
                               "events": [{"seq": 1, "code": "demo", "priority": 0,
                                           "message": "SIMULATED event / not hardware", "age_ms": 2500,
                                           "ttl_ms": 8000}]})
        if url.path == "/api/v1/scene-layout":
            return self.reply(STATE["scene_layout"])
        if url.path == "/api/v1/focus":
            focus_step()
            return self.reply({"schema": 1, "focus_running": STATE["focus_running"],
                               "focus_phase": STATE["focus_phase"],
                               "focus_remaining_s": STATE["focus_remaining_s"],
                               "focus_sessions": STATE["focus_sessions"]})
        if url.path in ("/api/status", "/api/v1/status"):
            focus_step()
            cpu, gpu, ram = sample(t)
            preview = STATE["preview"] >= 0 and time.monotonic() < STATE["preview_until"]
            mood = MOODS[STATE["preview"]] if preview else "demo"
            return self.reply({"version": VERSION + "-mock", "device": "MEO DESK / MOCK",
                               "capabilities": ["performance_lab_v1", "device_profile_v1", "focus_config_v1", "widget_scene_v1"],
                               "lab_enabled": STATE["lab_enabled"], "lab_valid": False,
                               "wifi_connected": True, "wifi_ssid": "SIMULATED NETWORK", "wifi_rssi": -50,
                               "ip": "127.0.0.1 (mock)", "ap_ip": "127.0.0.1", "esp_heap_kb": 206,
                               "pc_connected": True, "demo": True, "data_age_ms": 160,
                               "emotion": mood, "real_emotion": "demo", "preview_active": preview,
                               "pc_name": "MOCK WINDOWS / DEMO", "gpu_name": "SIMULATED GPU",
                               "cpu_percent": cpu, "gpu_percent": gpu, "ram_percent": ram,
                               "gpu_temp_c": 54, "cpu_temp_c": 48, "ram_used_gb": round(32 * ram / 100, 1),
                               "ram_total_gb": 32, "battery_percent": None, "battery_plugged": None,
                               "net_up_kbps": round(24 + 12 * math.sin(t / 8), 1),
                               "net_down_kbps": round(128 + 87 * math.sin(t / 12), 1),
                               "vram_used_gb": 2.8, "vram_total_gb": 8,
                               "pc_uptime_s": 11400, "settings": STATE["settings"],
                               "events": [{"code": "demo", "message": "SIMULATED PC health signal", "priority": 0, "age_s": 2}],
                               "active_event": "demo", "active_event_priority": 0, "event_seq": 1,
                               "cat_persona": "celebrating" if time.monotonic() < STATE["pet_until"] else "dancing",
                               "cat_motion_frames": 3, "motion_style": STATE["motion_style"],
                               "motion_state": "music" if STATE["motion"] else "still",
                               "motion_cel": int(t * 6) % 3 if STATE["motion"] else 0,
                               "motion_blink": False, "motion_budget_limited": False,
                               "motion_pc_intensity": 0,  # DEMO is NOT real CPU/GPU input.
                               "cat_energy": STATE["pet_energy"],
                               "scene_slots": STATE["scene_slots"], "artwork_ready": False,
                               "scene_layout_revision": STATE["scene_layout_revision"], "page": STATE["page"], "page_count": 15,
                               "media_available": True, "media_playing": True,
                               "command_pending": False, "command_seq": STATE["command_seq"],
                               "command_acknowledged": STATE["command_acknowledged"],
                               "command_ok": STATE["command_ok"],
                               "wifi_scan_active": False, "wifi_network_count": 3,
                               "wifi_strongest_ssid": "SIMULATED NETWORK",
                               "motion": STATE["motion"], "auto_pages": STATE["auto_pages"],
                               "focus_running": STATE["focus_running"],
                               "focus_phase": STATE["focus_phase"],
                               "focus_remaining_s": STATE["focus_remaining_s"],
                               "focus_completed_total": STATE["focus_completed_total"], "focus_last_completed_min": STATE["focus_last_completed_min"], "focus_sessions": STATE["focus_sessions"],
                               "focus_work_min": STATE["focus_work_min"], "focus_break_min": STATE["focus_break_min"], "focus_goal": STATE["focus_goal"],
                               "focus_duration_s": STATE["focus_work_min" if STATE["focus_phase"] == "work" else "focus_break_min"] * 60,
                               "theme": STATE["theme"], "night_theme": STATE["theme"] in (1,3),
                               "cat_fps": effective_fps_target() - 0.3,
                               "fps_profile": STATE["fps_profile"],
                               "fps_target": effective_fps_target(),
                               "fps_auto": STATE["fps_profile"] == 3,
                               "psram_ready": True, "psram_free_kb": 7890,
                               "internal_ram_kb": 196, "internal_dma_kb": 123,
                               "cat_render_max_ms": 16.8, "cat_frames_skipped": 0,
                               "dma": True})
        if url.path == "/api/v1/config":
            profile = {key: STATE[key] for key in ("theme", "fps_profile", "motion_style", "motion", "auto_pages", "focus_work_min", "focus_break_min", "focus_goal")}
            profile.update(STATE["settings"], scene_slots=list(STATE["scene_slots"]))
            return self.reply({"schema": 1, "board": "meodesk-esp32s3-n16r8", "firmware_version": VERSION + "-mock", "profile": profile, "scene_layout": STATE["scene_layout"]})
        if url.path == "/api/v1/ota/history":
            return self.reply({"schema": 1, "updates": [], "mode": "demo"})
        return self.reply({"ok": False, "error": "not found"}, 404)

    def do_POST(self) -> None:
        if self.path == "/api/v1/ota/upload":
            return self.reply({"ok": False, "error": "DEMO cannot upload firmware"}, 501)
        if self.path == "/api/v1/catpack/upload":
            return self.reply({"ok": False, "error": "DEMO cannot upload cat packs"}, 501)
        if self.path == "/api/v1/artwork":
            return self.reply({"ok": False, "error": "mock does not accept real cover uploads"}, 501)
        try:
            data = json.loads(self.rfile.read(min(int(self.headers.get("Content-Length", "0")), 2000)))
            if self.path == "/api/v1/config":
                backup = validate_backup(data)
                clean = backup["profile"]
                if "scene_layout" in backup:
                    STATE["scene_layout"] = backup["scene_layout"]
                    STATE["scene_layout_revision"] += 1
                for key in ("theme", "fps_profile", "motion_style", "motion", "auto_pages", "focus_work_min", "focus_break_min", "focus_goal"):
                    STATE[key] = clean[key]
                STATE["scene_slots"] = clean["scene_slots"]
                STATE["settings"].update({key: clean[key] for key in STATE["settings"]})
                STATE.update(focus_running=False, focus_phase="work", focus_remaining_s=clean["focus_work_min"]*60)
                return self.reply({"ok": True, "persisted": False, "mode": "demo"})
            if self.path == "/api/v1/scene-layout":
                STATE["scene_layout"] = validate_layout(data)
                STATE["scene_layout_revision"] += 1
                return self.reply({"ok": True, "persisted": False, "mode": "demo"})
            if self.path == "/api/v1/focus/config":
                work, rest, goal = data.get("work_min"), data.get("break_min"), data.get("goal")
                if any(type(v) is not int for v in (work,rest,goal)) or not 1<=work<=180 or not 1<=rest<=60 or not 1<=goal<=24:
                    return self.reply({"ok":False,"error":"invalid Focus configuration"},400)
                STATE.update(focus_work_min=work, focus_break_min=rest, focus_goal=goal,
                             focus_running=False, focus_phase="work", focus_remaining_s=work*60)
                return self.reply({"ok":True,"persisted":False,"mode":"demo"})
            if self.path == "/api/control":
                action = data.get("action")
                value = int(data.get("value", 0))
                if action == "preview" and 0 <= value < len(MOODS):
                    STATE["preview"] = value
                    STATE["preview_until"] = time.monotonic() + 8
                elif action == "page" and 0 <= value < 15:
                    STATE["page"] = value
                elif action == "next":
                    STATE["page"] = (STATE["page"] + 1) % 15
                elif action in ("motion", "auto"):
                    field = {"motion": "motion", "auto": "auto_pages"}[action]
                    STATE[field] = not STATE[field]
                elif action in ("focus", "focus_reset", "focus_skip"):
                    change_focus(action)
                elif action == "theme" and 0 <= value <= 5:
                    STATE["theme"] = value
                elif action == "fps" and 0 <= value <= 3:
                    STATE["fps_profile"] = value
                elif action == "motion_style" and 0 <= value <= 2:
                    STATE["motion_style"] = value
                elif action == "perf_trace" and value in (0, 1):
                    STATE["lab_enabled"] = bool(value)
                elif action == "pet":
                    STATE["pet_energy"] = min(100, STATE["pet_energy"] + 8)
                    STATE["pet_until"] = time.monotonic() + 8
                elif isinstance(action, str) and action.startswith("scene_") and \
                        len(action) == 7 and action[6] in "0123" and 0 <= value < 15:
                    index = int(action[6])
                    if value in STATE["scene_slots"] and STATE["scene_slots"][index] != value:
                        return self.reply({"ok": False, "error": "duplicate favorite"}, 400)
                    STATE["scene_slots"][index] = value
                elif action == "scan":
                    pass
                elif action in ("media_previous", "media_next", "media_play_pause"):
                    STATE["command_seq"] += 1
                    STATE["command_acknowledged"] = True
                    STATE["command_ok"] = True
                    return self.reply({"ok": True, "pending": True, "seq": STATE["command_seq"]}, 202)
                else:
                    return self.reply({"ok": False, "error": "unknown command"}, 400)
                return self.reply({"ok": True})
            if self.path == "/api/v1/focus":
                if not change_focus(data.get("action", "")):
                    return self.reply({"ok": False, "error": "invalid focus action"}, 400)
                return self.reply({"schema": 1, "ok": True,
                                   "focus_running": STATE["focus_running"],
                                   "focus_phase": STATE["focus_phase"],
                                   "focus_remaining_s": STATE["focus_remaining_s"],
                                   "focus_sessions": STATE["focus_sessions"]})
            if self.path == "/api/settings":
                STATE["settings"].update(data)
                return self.reply({"ok": True, "persisted": False})
            if self.path == "/api/v1/wifi/config":
                return self.reply({"ok": False, "error": "Wi-Fi cannot be saved in mock mode"}, 501)
        except (ValueError, TypeError, json.JSONDecodeError):
            return self.reply({"ok": False, "error": "invalid JSON"}, 400)
        return self.reply({"ok": False, "error": "not found"}, 404)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", type=int, default=8765)
    args = parser.parse_args()
    server = ThreadingHTTPServer(("127.0.0.1", args.port), Handler)
    print(f"MeoDesk Web UI mock -> http://127.0.0.1:{server.server_port}/ (SIMULATED DATA)", flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        server.server_close()
