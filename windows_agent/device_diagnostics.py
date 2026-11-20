#!/usr/bin/env python3
"""Read-only, stdlib-only MèoDesk endurance recorder. Never writes to the ESP32."""
from __future__ import annotations

import argparse
import csv
import json
import math
import time
from datetime import datetime, timezone
from pathlib import Path
from urllib.error import HTTPError, URLError
from urllib.request import HTTPRedirectHandler, ProxyHandler, Request, build_opener

from discovery import normalize_host


ENDPOINTS = {
    "status": "/api/status",
    "performance": "/api/v1/performance",
    "ota": "/api/v1/ota/status",
    "catpack": "/api/v1/catpack/status",
}
FIELDS = {
    "status": set("""device device_id version api_version capabilities uptime_s
        flash_bytes psram_bytes psram_free_bytes esp_heap_kb internal_ram_kb
        internal_dma_kb psram_free_kb psram_ready dma card_buffered cat_fps
        fps_profile fps_target fps_auto cat_render_max_ms cat_frames_skipped
        pc_connected demo data_age_ms wifi_connected wifi_rssi
        wifi_last_disconnect_reason page motion cat_motion_frames
        cat_pack_loaded cat_pack_generation motion_style motion_state motion_cel
        motion_budget_limited motion_pc_intensity focus_running focus_phase theme
        lab_enabled lab_valid lab_payload_coverage lab_window_ms lab_strip_cpu_ms
        lab_dma_submit_ms lab_dma_wait_ms lab_cat_wall_ms lab_card_wall_ms
        lab_bridge_wall_ms lab_payload_bytes_s lab_cat_frames lab_cpu_frame_ms
        lab_card_max_ms lab_bridge_max_ms""".split()),
    "performance": set("""schema sample_period_s count mode fps target skipped
        worst_ms internal_kb psram_kb""".split()),
    "ota": set("""schema board version ready phase flash_bytes uptime_ms
        boot_health inactive_slot_bytes running_slot inactive_slot
        bootloader_rollback_configured sha256_required signed_image_required""".split()),
    "catpack": set("""schema board ready motion_ready source active_bank generation
        sha256 frame_count frames_per_pose active_pack_bytes width height
        pack_bytes accepted_pack_bytes phase""".split()),
}
METRICS = ("cat_fps", "fps_target", "cat_render_max_ms", "cat_frames_skipped",
           "internal_ram_kb", "internal_dma_kb", "psram_free_kb", "esp_heap_kb",
           "lab_cpu_frame_ms", "lab_dma_submit_ms", "lab_dma_wait_ms", "lab_cat_wall_ms",
           "lab_card_wall_ms", "lab_card_max_ms", "lab_bridge_wall_ms", "lab_bridge_max_ms",
           "lab_payload_bytes_s")
CSV_FIELDS = ("utc", "elapsed_s", "source", "status_error", "version", "device_id",
              "uptime_s", "pc_mode", "fps_profile", "motion_style", "motion_state",
              "motion_budget_limited", "cat_motion_frames", "dma", "psram_ready", "theme",
              "lab_enabled", "lab_valid", "lab_window_ms", "lab_payload_coverage",
              *METRICS, "running_slot", "boot_health", "pack_source", "frames_per_pose")


def utc_now() -> str:
    return datetime.now(timezone.utc).isoformat(timespec="milliseconds")


def number(value) -> bool:
    return type(value) in (int, float) and math.isfinite(value)


def sanitized(name: str, payload: dict) -> dict:
    """Keep only diagnostic fields, never arbitrary identity/media/error text."""
    def scalar(value):
        if value is None or isinstance(value, (str, bool)) or number(value):
            return value
        return None

    return {key: [scalar(item) for item in value] if isinstance(value, list) else scalar(value)
            for key, value in payload.items() if key in FIELDS[name]}


class NoRedirect(HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        return None


class DeviceReader:
    def __init__(self, host: str, timeout: float = 2.0):
        self.host = normalize_host(host)
        self.timeout = timeout
        self.opener = build_opener(ProxyHandler({}), NoRedirect())
        self.unsupported: dict[str, int] = {}
        self.version = None

    def read(self, name: str) -> dict:
        request = Request(f"http://{self.host}{ENDPOINTS[name]}",
                          headers={"Accept": "application/json"}, method="GET")
        with self.opener.open(request, timeout=self.timeout) as response:
            body = response.read(32769)
        if len(body) > 32768:
            raise ValueError("oversized_response")
        payload = json.loads(body)
        if not isinstance(payload, dict):
            raise ValueError("invalid_response")
        return payload

    def sample(self) -> dict:
        result = {"data": {}, "errors": {}}
        for name in ENDPOINTS:
            if name in self.unsupported:
                result["errors"][name] = {"code": "unsupported", "http_status": self.unsupported[name]}
                continue
            try:
                payload = self.read(name)
                if name == "status":
                    if payload.get("device") not in ("MEO DESK", "MEO DESK / MOCK"):
                        raise ValueError("unrecognized_device")
                    if payload.get("version") != self.version:
                        self.unsupported.clear()
                        self.version = payload.get("version")
                result["data"][name] = sanitized(name, payload)
            except HTTPError as error:
                unsupported = name != "status" and error.code in (404, 405, 501)
                if unsupported:
                    self.unsupported[name] = error.code
                result["errors"][name] = {
                    "code": "unsupported" if unsupported else "http_error", "http_status": error.code}
                error.close()
            except (URLError, OSError, TimeoutError):
                result["errors"][name] = {"code": "connection_error"}
            except (ValueError, UnicodeError):
                result["errors"][name] = {"code": "invalid_response"}
            # Do not spend more requests on an offline/unrecognized device.
            if name == "status" and name not in result["data"]:
                break
        return result


def flatten(record: dict) -> dict:
    data = record["data"]
    status = data.get("status", {})
    ota = data.get("ota", {})
    pack = data.get("catpack", {})
    # PC DEMO on real firmware does not make its device FPS simulated.
    simulated = ("mock" in str(status.get("version", "")).lower()
                 or status.get("device") == "MEO DESK / MOCK"
                 or data.get("performance", {}).get("mode") == "demo")
    source = ("simulated" if simulated else "device_reported") if status else "unavailable"
    pc_mode = ("unknown" if "pc_connected" not in status else
               "offline" if not status["pc_connected"] else
               "demo" if status.get("demo") is True else
               "real" if status.get("demo") is False else "unknown")
    row = {key: status[key] for key in CSV_FIELDS if key in status}
    row.update(utc=record["utc"], elapsed_s=record["elapsed_s"], source=source,
               status_error=record["errors"].get("status", {}).get("code", ""), pc_mode=pc_mode)
    row.update({key: ota[key] for key in ("running_slot", "boot_health") if key in ota})
    row.update({dest: pack[key] for dest, key in
                (("pack_source", "source"), ("frames_per_pose", "frames_per_pose")) if key in pack})
    return row


class Summary:
    def __init__(self, host: str, duration_s: float, interval_s: float):
        self.value = {"schema": 1, "host": host, "started_utc": utc_now(),
                      "requested_duration_s": duration_s, "interval_s": interval_s,
                      "samples": 0, "status_samples": 0, "status_errors": 0,
                      "endpoint_errors": {}, "uptime_decreases": 0, "source_counts": {},
                      "pc_mode_counts": {}, "metrics_by_source": {}, "latest": {},
                      "hardware_acceptance": "not_assessed"}
        self.previous_uptime = None
        self.previous_device = None

    def add(self, record: dict) -> dict:
        row = flatten(record)
        summary = self.value
        summary["samples"] += 1
        for name in record["errors"]:
            errors = summary["endpoint_errors"]
            errors[name] = errors.get(name, 0) + 1
        if "status" not in record["data"]:
            summary["status_errors"] += 1
            return row
        summary["status_samples"] += 1
        for field, counter in (("source", "source_counts"), ("pc_mode", "pc_mode_counts")):
            counts = summary[counter]
            counts[row[field]] = counts.get(row[field], 0) + 1
        uptime = row.get("uptime_s")
        device = row.get("device_id")
        if number(uptime):
            if device == self.previous_device and number(self.previous_uptime) and uptime < self.previous_uptime:
                summary["uptime_decreases"] += 1
            self.previous_uptime, self.previous_device = uptime, device
        bucket = summary["metrics_by_source"].setdefault(row["source"], {})
        for key in METRICS:
            value = row.get(key)
            if number(value) and value >= 0:
                stats = bucket.setdefault(key, {"count": 0, "min": value, "max": value, "mean": 0.0})
                stats["count"] += 1
                stats["min"] = min(stats["min"], value)
                stats["max"] = max(stats["max"], value)
                stats["mean"] += (value - stats["mean"]) / stats["count"]
        summary["latest"] = {name: payload for name, payload in record["data"].items()
                             if name != "performance"}
        return row


def record_session(reader: DeviceReader, output: Path, duration_s: float, interval_s: float) -> dict:
    """Stream evidence to disk; Ctrl+C keeps completed samples and the summary."""
    output.mkdir(parents=True, exist_ok=False)
    summary = Summary(reader.host, duration_s, interval_s)
    started = time.monotonic()
    reason = "error"
    with (output / "samples.jsonl").open("w", encoding="utf-8") as evidence, \
            (output / "metrics.csv").open("w", encoding="utf-8", newline="") as metrics:
        writer = csv.DictWriter(metrics, fieldnames=CSV_FIELDS)
        writer.writeheader()
        try:
            while time.monotonic() - started < duration_s:
                elapsed = time.monotonic() - started
                record = {"utc": utc_now(), "elapsed_s": round(elapsed, 3), **reader.sample()}
                row = summary.add(record)
                evidence.write(json.dumps(record, ensure_ascii=False, allow_nan=False) + "\n")
                writer.writerow(row)
                evidence.flush()
                metrics.flush()
                print(f"{elapsed:7.1f}s  {row['source']}  FPS={row.get('cat_fps', 'N/A')}"
                      f"/{row.get('fps_target', 'N/A')}  PC={row['pc_mode']}", flush=True)
                remaining = duration_s - (time.monotonic() - started)
                delay = min(remaining, interval_s - (time.monotonic() - started - elapsed))
                if delay > 0:
                    time.sleep(delay)
            reason = "duration_elapsed"
        except KeyboardInterrupt:
            reason = "interrupted"
        finally:
            summary.value.update(ended_utc=utc_now(), elapsed_s=round(time.monotonic() - started, 3),
                                 stop_reason=reason)
            (output / "summary.json").write_text(
                json.dumps(summary.value, indent=2, ensure_ascii=False, allow_nan=False) + "\n",
                encoding="utf-8")
    return summary.value


def positive(value: str) -> float:
    result = float(value)
    if not math.isfinite(result) or result <= 0:
        raise argparse.ArgumentTypeError("must be a finite positive number")
    return result


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", required=True, help="device LAN IPv4 or meodesk.local (HTTP port 80)")
    parser.add_argument("--duration", type=positive, default=1800, help="recording seconds (default: 1800)")
    parser.add_argument("--interval", type=positive, default=5, help="poll seconds, minimum 5 (default: 5)")
    parser.add_argument("--output", type=Path, help="new output directory; existing directories are never overwritten")
    args = parser.parse_args(argv)
    if args.interval < 5:
        parser.error("--interval must be at least 5 seconds")
    try:
        reader = DeviceReader(args.host)
    except ValueError as error:
        parser.error(str(error))
    output = args.output or Path("diagnostics") / datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    print(f"Read-only recording: {reader.host} -> {output.resolve()}", flush=True)
    try:
        result = record_session(reader, output, args.duration, args.interval)
    except OSError:
        parser.exit(2, "Cannot create/write output; choose a new writable directory.\n")
    print(f"Saved {result['status_samples']}/{result['samples']} status snapshots. Hardware acceptance: not assessed.")
    return 0 if result["status_samples"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
