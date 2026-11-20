"""MèoDesk N16R8 LAN OTA transport (stdlib only, no Qt dependency).

Uses a bounded multipart upload streamed from disk. The pairing key is never
put in a URL, command-line argument, release manifest, or log. This developer
endpoint uses HTTP on the private LAN: it is NOT signed/release OTA.
"""

from __future__ import annotations

import hashlib
import http.client
import json
import secrets
import time
from pathlib import Path
from typing import Callable
from urllib.request import ProxyHandler, Request, build_opener

from discovery import normalize_host


BOARD_ID = "meodesk-esp32s3-n16r8"
MAX_FIRMWARE_BYTES = 6 * 1024 * 1024
MIN_FIRMWARE_BYTES = 4096
CHUNK_BYTES = 16 * 1024


def validate_image(path: str | Path) -> tuple[int, str]:
    """Return size and SHA-256; reject obvious wrong files before contacting ESP32."""
    image = Path(path)
    size = image.stat().st_size
    if image.suffix.lower() != ".bin" or not MIN_FIRMWARE_BYTES <= size <= MAX_FIRMWARE_BYTES:
        raise ValueError("Chọn firmware .bin hợp lệ, tối đa 6 MiB")
    digest = hashlib.sha256()
    with image.open("rb") as file:
        header = file.read(14)
        if header[:1] != b"\xe9" or header[12:14] != b"\x09\x00":
            raise ValueError("Đây không phải ESP32 application firmware (.bin)")
        file.seek(0)
        for chunk in iter(lambda: file.read(CHUNK_BYTES), b""):
            digest.update(chunk)
    return size, digest.hexdigest()


def read_ota_status(host: str, *, timeout: float = 3.0) -> dict:
    target = normalize_host(host)
    opener = build_opener(ProxyHandler({}))
    request = Request(f"http://{target}/api/v1/ota/status", headers={"Accept": "application/json"})
    with opener.open(request, timeout=timeout) as response:
        if response.status != 200:
            raise OSError(f"ESP32 returned HTTP {response.status}")
        payload = response.read(8193)
        if len(payload) > 8192:
            raise ValueError("Invalid oversized OTA status response")
    data = json.loads(payload)
    if not isinstance(data, dict) or data.get("board") != BOARD_ID:
        raise ValueError("Thiết bị không phải MèoDesk ESP32-S3 N16R8 hỗ trợ OTA")
    return data


def upload_firmware(host: str, pairing_key: str, firmware: str | Path,
                    progress: Callable[[int, int], None] | None = None) -> dict:
    """Explicitly initiated local OTA; caller must obtain human confirmation."""
    target = normalize_host(host)
    if not isinstance(pairing_key, str) or not pairing_key.strip() or "\n" in pairing_key or "\r" in pairing_key:
        raise ValueError("Nhập pairing key hợp lệ")
    size, sha256 = validate_image(firmware)
    status = read_ota_status(target)
    if not status.get("ready") or status.get("phase") not in ("idle", "error"):
        raise ValueError("ESP32 chưa sẵn sàng OTA: kiểm tra phân vùng và trạng thái")
    if size > int(status.get("inactive_slot_bytes") or 0):
        raise ValueError("Firmware lớn hơn phân vùng OTA chưa sử dụng")
    target_slot = status.get("inactive_slot")
    current_slot = status.get("running_slot")
    if not isinstance(target_slot, str) or target_slot not in ("app0", "app1") or target_slot == current_slot:
        raise ValueError("ESP32 không báo phân vùng đích OTA độc lập hợp lệ")
    boundary = "meodesk-" + secrets.token_hex(12)
    prefix = (f"--{boundary}\r\nContent-Disposition: form-data; name=\"firmware\"; "
              f"filename=\"firmware.bin\"\r\nContent-Type: application/octet-stream\r\n\r\n").encode("ascii")
    suffix = f"\r\n--{boundary}--\r\n".encode("ascii")
    connection = http.client.HTTPConnection(target, 80, timeout=180)
    try:
        connection.putrequest("POST", "/api/v1/ota/upload")
        connection.putheader("Content-Type", f"multipart/form-data; boundary={boundary}")
        connection.putheader("Content-Length", len(prefix) + size + len(suffix))
        connection.putheader("X-MEO-KEY", pairing_key.strip())
        connection.putheader("X-MEO-OTA-BOARD", BOARD_ID)
        connection.putheader("X-MEO-OTA-SIZE", str(size))
        connection.putheader("X-MEO-OTA-SHA256", sha256)
        connection.endheaders()
        connection.send(prefix)
        sent = 0
        with Path(firmware).open("rb") as file:
            while True:
                chunk = file.read(CHUNK_BYTES)
                if not chunk:
                    break
                connection.send(chunk)
                sent += len(chunk)
                if progress:
                    progress(sent, size)
        connection.send(suffix)
        response = connection.getresponse()
        body = response.read(8193)
        if len(body) > 8192:
            raise ValueError("Unexpected oversized OTA response")
        try:
            result = json.loads(body)
        except ValueError as error:
            raise OSError("ESP32 did not return a valid OTA result") from error
        if response.status != 200 or result.get("ok") is not True or result.get("verified") is not True:
            raise OSError(str(result.get("error") or f"ESP32 rejected OTA (HTTP {response.status})"))
        result["target_slot"] = target_slot
        return result
    finally:
        connection.close()


def wait_for_boot(host: str, target_slot: str, *, timeout_s: float = 85.0,
                  interval_s: float = 2.0,
                  status_callback: Callable[[str], None] | None = None) -> dict:
    """Prove the ESP rebooted into the target slot and passed its startup check.

    A completed HTTP upload is NOT a successful firmware boot. Device runtime
    checks take ~25 s; connection errors are normal during reboot. This is
    a liveness check, not a cryptographic proof or a substitute for rollback.
    """
    target = normalize_host(host)
    if target_slot not in ("app0", "app1"):
        raise ValueError("Invalid expected OTA slot")
    deadline = time.monotonic() + timeout_s
    saw_target = False
    last_notice = ""
    while time.monotonic() < deadline:
        time.sleep(max(0.05, interval_s))
        try:
            status = read_ota_status(target, timeout=3.0)
        except (OSError, TimeoutError, ValueError):
            notice = "Đang chờ ESP32 kết nối trở lại…"
        else:
            running = status.get("running_slot")
            health = status.get("boot_health")
            if running == target_slot:
                saw_target = True
                if health == "failed":
                    raise OSError("ESP32 đã khởi động firmware mới nhưng kiểm tra sức khỏe thất bại")
                if health == "passed" and status.get("ready") is True:
                    return status
                notice = "Đã vào phân vùng mới; đang chờ ESP32 tự kiểm tra khởi động…"
            elif saw_target:
                raise OSError("Thiết bị quay lại phân vùng cũ sau khi thử firmware mới")
            else:
                notice = "Đang chờ ESP32 chuyển sang phân vùng firmware mới…"
        if status_callback and notice != last_notice:
            status_callback(notice)
            last_notice = notice
    raise TimeoutError("Chưa xác nhận được firmware mới đã khởi động ổn định. Kiểm tra Serial/USB trước khi thử lại.")
