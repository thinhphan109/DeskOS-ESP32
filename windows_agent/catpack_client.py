"""Developer-LAN Cat Pack OTA client; no GUI or extra dependencies.

Uploads either 16-frame static or 48-frame motion RGB565 `.meocat` packs. Existing
active assets are not replaced until the device commits a verified bank header.
No image signing or transport encryption: trusted local LAN only.
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


PACK_BYTES = 16 * 144 * 144 * 2
MOTION_BYTES = PACK_BYTES * 3
BOARD = "meodesk-esp32s3-n16r8"
CHUNK = 16 * 1024


def validate_pack(path: str | Path) -> tuple[int, str]:
    file = Path(path)
    if file.suffix.lower() != ".meocat" or file.stat().st_size not in (PACK_BYTES, MOTION_BYTES):
        raise ValueError("Cat Pack phải là .meocat: 648 KiB / 16 ảnh hoặc 1944 KiB / 48 khung hình")
    digest = hashlib.sha256()
    with file.open("rb") as handle:
        for chunk in iter(lambda: handle.read(CHUNK), b""):
            digest.update(chunk)
    return file.stat().st_size, digest.hexdigest()


def read_status(host: str, *, timeout: float = 3.0) -> dict:
    host = normalize_host(host)
    request = Request(f"http://{host}/api/v1/catpack/status",
                      headers={"Accept": "application/json"})
    with build_opener(ProxyHandler({})).open(request, timeout=timeout) as response:
        body = response.read(8193)
        if response.status != 200 or len(body) > 8192:
            raise OSError("ESP32 không trả về Cat Pack status hợp lệ")
    data = json.loads(body)
    if not isinstance(data, dict) or data.get("board") != BOARD:
        raise ValueError("Sai board hoặc thiết bị chưa hỗ trợ Cat Pack v1")
    return data


def upload_pack(host: str, pairing_key: str, filename: str | Path,
                progress: Callable[[int, int], None] | None = None) -> dict:
    host = normalize_host(host)
    if not isinstance(pairing_key, str) or not pairing_key.strip() or "\n" in pairing_key or "\r" in pairing_key:
        raise ValueError("Nhập pairing key hợp lệ")
    size, sha = validate_pack(filename)
    status = read_status(host)
    accepted = status.get("accepted_pack_bytes", [status.get("pack_bytes")])
    if not status.get("ready") or size not in accepted or status.get("phase") not in ("idle", "error"):
        raise ValueError("ESP32 chưa hỗ trợ kích thước Cat Pack này (Flash/PSRAM hoặc cần firmware v1.6)")
    boundary = "meocat-" + secrets.token_hex(12)
    prefix = (f"--{boundary}\r\nContent-Disposition: form-data; name=\"catpack\"; "
              f"filename=\"cats.meocat\"\r\nContent-Type: application/octet-stream\r\n\r\n").encode("ascii")
    suffix = f"\r\n--{boundary}--\r\n".encode("ascii")
    conn = http.client.HTTPConnection(host, 80, timeout=180)
    try:
        conn.putrequest("POST", "/api/v1/catpack/upload")
        conn.putheader("Content-Type", f"multipart/form-data; boundary={boundary}")
        conn.putheader("Content-Length", len(prefix) + size + len(suffix))
        conn.putheader("X-MEO-KEY", pairing_key.strip())
        conn.putheader("X-MEO-CAT-SIZE", str(size))
        conn.putheader("X-MEO-CAT-SHA256", sha)
        conn.endheaders()
        conn.send(prefix)
        sent = 0
        with Path(filename).open("rb") as file:
            for block in iter(lambda: file.read(CHUNK), b""):
                conn.send(block)
                sent += len(block)
                if progress:
                    progress(sent, size)
        conn.send(suffix)
        response = conn.getresponse()
        body = response.read(8193)
        if len(body) > 8192:
            raise OSError("Cat Pack response quá lớn")
        try:
            result = json.loads(body)
        except ValueError as error:
            raise OSError("ESP32 không trả về JSON hợp lệ") from error
        if response.status != 200 or result.get("ok") is not True or result.get("verified") is not True:
            raise OSError(str(result.get("error") or f"ESP32 từ chối Cat Pack (HTTP {response.status})"))
        result["expected_sha256"] = sha
        return result
    finally:
        conn.close()


def wait_for_pack(host: str, sha256: str, *, timeout_s: float = 55.0,
                  interval_s: float = 1.7,
                  callback: Callable[[str], None] | None = None) -> dict:
    host = normalize_host(host)
    if len(sha256) != 64 or any(c not in "0123456789abcdef" for c in sha256):
        raise ValueError("Expected pack SHA-256 invalid")
    deadline = time.monotonic() + timeout_s
    last = ""
    while time.monotonic() < deadline:
        time.sleep(max(0.05, interval_s))
        try:
            status = read_status(host)
        except (OSError, ValueError, TimeoutError):
            note = "Đang chờ ESP32 khởi động lại…"
        else:
            if status.get("source") == "custom" and status.get("sha256") == sha256:
                return status
            note = "ESP32 đang kiểm tra và nạp bộ mèo vào PSRAM…"
        if callback and note != last:
            callback(note)
            last = note
    raise TimeoutError("Chưa xác minh được bộ mèo mới trên ESP32. Kiểm tra Serial/PSRAM.")
