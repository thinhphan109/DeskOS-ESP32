"""MèoDesk Companion service layer. Safe to import without PySide6/WinRT."""
from __future__ import annotations

import json
import hashlib
import unicodedata
import os
import threading
import time
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Callable, Any
from urllib.error import HTTPError, URLError
from urllib.request import Request, ProxyHandler, build_opener

from pc_agent import NvidiaGpu, collect_real, psutil, send_json, target_url
import media_provider
import discovery
from connection_state import RetryBackoff


_http = build_opener(ProxyHandler({}))


def media_artwork_id(snapshot: dict) -> str:
    """Stable, public 16-hex content key; not an authentication credential."""
    if not snapshot.get("available") or not snapshot.get("title"):
        return ""
    identity = "\x00".join(str(snapshot.get(field) or "")
                          for field in ("title", "artist", "album"))
    return hashlib.blake2s(identity.encode("utf-8"), digest_size=8).hexdigest()


def wire_media(snapshot: dict) -> dict:
    """Strict schema-1 envelope: safe for the ESP32's 512-byte input limit."""
    def short(value: Any, n: int) -> str:
        clean = unicodedata.normalize("NFC", str(value or ""))
        clean = " ".join("".join(c if unicodedata.category(c) not in {"Cc", "Cs"}
                                  else " " for c in clean).split())
        return clean.encode("utf-8")[:n].decode("utf-8", "ignore")

    return {"schema": 1, "available": bool(snapshot.get("available")),
            "playing": bool(snapshot.get("playing")),
            "title": short(snapshot.get("title"), 64),
            "artist": short(snapshot.get("artist"), 48),
            "position_ms": max(0, min(int(snapshot.get("position_ms") or 0), 86400000)),
            "duration_ms": max(0, min(int(snapshot.get("duration_ms") or 0), 86400000)),
            "artwork_id": media_artwork_id(snapshot)}


def post_artwork(host: str, key: str, art_id: str, pixels: bytes,
                 timeout: float = 2.0) -> dict:
    """Stream a bounded album tile as multipart; never write it to device NVS."""
    if len(pixels) != 8192 or len(art_id) != 16 or any(c not in "0123456789abcdef" for c in art_id):
        raise ValueError("Artwork must be 64x64 RGB565 with a 16-hex track ID")
    host = discovery.normalize_host(host)
    boundary = "----MeoDeskCover64V1"
    prefix = (f"--{boundary}\r\n"
              'Content-Disposition: form-data; name="artwork"; filename="album.rgb565"\r\n'
              'Content-Type: application/octet-stream\r\n\r\n').encode("ascii")
    body = prefix + pixels + (f"\r\n--{boundary}--\r\n").encode("ascii")
    req = Request(f"http://{host}/api/v1/artwork", data=body, method="POST",
                  headers={"X-MEO-KEY": key, "X-MEO-ART-ID": art_id,
                           "Content-Type": f"multipart/form-data; boundary={boundary}"})
    with _http.open(req, timeout=timeout) as response:
        return json.load(response)


@dataclass
class Settings:
    host: str = ""
    interval: float = 1.0
    gpu: str = "auto"
    media: bool = True
    start_minimized: bool = False
    break_reminders: bool = False
    device_id: str = ""


def user_config_path() -> Path:
    root = os.environ.get("LOCALAPPDATA") or str(Path.home() / ".config")
    return Path(root) / "MeoDesk" / "companion.json"


def read_settings(path: Path | None = None) -> Settings:
    path = path or user_config_path()
    if not path.exists():
        return Settings()
    try:
        raw = json.loads(path.read_text(encoding="utf-8"))
        allowed = {key: raw[key] for key in Settings.__dataclass_fields__ if key in raw}
        result = Settings(**allowed)
        if not isinstance(result.device_id, str) or len(result.device_id) > 64:
            return Settings()
        if any(type(getattr(result,key)) is not bool for key in ("media","start_minimized","break_reminders")):
            return Settings()
        if not 0.5 <= result.interval <= 5.0 or result.gpu not in ("auto", "nvidia", "lhm", "none"):
            return Settings()
        if result.host:
            discovery.normalize_host(result.host)
        return result
    except (ValueError, OSError, TypeError, AttributeError):
        return Settings()


def write_settings(settings: Settings, path: Path | None = None) -> None:
    path = path or user_config_path()
    path.parent.mkdir(parents=True, exist_ok=True)
    temp = path.with_suffix(".tmp")
    temp.write_text(json.dumps(asdict(settings), indent=2), encoding="utf-8")
    temp.replace(path)


def json_request(host: str, endpoint: str, key: str = "", payload: dict | None = None,
                 timeout: float = 1.3) -> dict:
    if not endpoint.startswith("/api/"):
        raise ValueError("Endpoint must be inside /api/")
    host = discovery.normalize_host(host)
    headers = {"Accept": "application/json"}
    body = None
    if key:
        headers["X-MEO-KEY"] = key
    if payload is not None:
        body = json.dumps(payload, ensure_ascii=False, separators=(",", ":")).encode("utf-8")
        headers["Content-Type"] = "application/json"
    req = Request(f"http://{host}{endpoint}", headers=headers, data=body,
                  method="POST" if body is not None else "GET")
    with _http.open(req, timeout=timeout) as response:
        return json.load(response)


class CompanionWorker:
    """Single background loop, media polling throttled to 2 seconds.

    Notifications are immutable snapshots emitted to GUI as Qt signals.
    On a legacy v0.7 device, missing new endpoints are quietly ignored.
    """

    def __init__(self, settings: Settings, key: str,
                 on_update: Callable[[dict], None] | None = None, *, media_source=None) -> None:
        self.settings = settings
        self.key = key
        self.on_update = on_update or (lambda _: None)
        self.media_source = media_source
        self._stop = threading.Event()
        self._paused = threading.Event()
        self._thread: threading.Thread | None = None
        self._last_artwork_id = ""
        self._current_song_id = ""
        self._artwork_next_try = 0.0
        self._last_seq = 0
        self.stream_active = False
        self._reset_session = threading.Event()

    def reset_device_session(self) -> None:
        """New ESP boot: its command sequence and volatile artwork start over."""
        self._reset_session.set()

    @property
    def running(self) -> bool:
        return bool(self._thread and self._thread.is_alive() and not self._paused.is_set())

    def start(self) -> None:
        if self._thread is None or not self._thread.is_alive():
            self._stop.clear()
            self._paused.clear()
            self._thread = threading.Thread(target=self._loop, name="MeoDeskCompanion", daemon=True)
            self._thread.start()
        else:
            self._paused.clear()

    def pause(self) -> None:
        self._paused.set()
        self.on_update({"type": "state", "message": "Tạm dừng gửi dữ liệu"})

    def stop(self) -> None:
        self._stop.set()
        if self._thread and self._thread is not threading.current_thread():
            self._thread.join(timeout=3.0)

    def _emit(self, message: dict) -> None:
        if not self._stop.is_set(): self.on_update(message)

    def _process_commands(self, host: str) -> None:
        if self._stop.is_set(): return
        try:
            result = json_request(host, "/api/v1/commands", self.key)
            if self._stop.is_set(): return
            command = result.get("command")
            if not isinstance(command, dict):
                return
            seq = command.get("seq")
            if not isinstance(seq, int) or seq <= self._last_seq:
                return
            action = command.get("action", "")
            # Deliver a command once, even if the ACK times out; duplicates must
            # not toggle playback again.
            self._last_seq = seq
            ok = media_provider.control(action)
            if self._stop.is_set(): return
            json_request(host, "/api/v1/commands/ack", self.key,
                         {"seq": seq, "ok": ok}, timeout=1.2)
        except (HTTPError, URLError, OSError, ValueError, TimeoutError):
            pass  # v0.7.x firmware has no command endpoint

    def _poll_focus(self, host: str) -> None:
        if self._stop.is_set(): return
        try:
            result = json_request(host, "/api/v1/focus", timeout=0.9)
            if result.get("schema") == 1:
                self._emit({"type": "focus", "data": result})
        except (HTTPError, URLError, OSError, ValueError, TimeoutError):
            pass  # Gracefully degrade with firmware <=v0.8.x.

    def _poll_motion(self, host: str) -> None:
        """Optional v1.7 read-only status, sampled only every five seconds."""
        if self._stop.is_set(): return
        try:
            result = json_request(host, "/api/v1/status", timeout=0.9)
            if result.get("motion_state") and result.get("motion_style") in (0, 1, 2):
                self._emit({"type": "motion_status", "data": result})
        except (HTTPError, URLError, OSError, ValueError, TimeoutError):
            pass  # Older firmware works without the new engine.

    def _loop(self) -> None:
        settings = self.settings
        nvml = NvidiaGpu() if settings.gpu in ("auto", "nvidia") else NvidiaGpu.disabled()
        psutil.cpu_percent(interval=None)
        next_media = 0.0
        last_status = 0.0
        last_motion = 0.0
        host = settings.host
        retry = RetryBackoff(initial=settings.interval)
        verified_device_id = ""
        try:
            while not self._stop.is_set():
                if self._reset_session.is_set():
                    self._reset_session.clear()
                    self._last_seq = 0
                    self._last_artwork_id = ""
                    self._artwork_next_try = 0.0
                if self._paused.is_set():
                    self._stop.wait(0.25)
                    continue
                started = time.monotonic()
                if not retry.ready(started):
                    self._stop.wait(min(settings.interval, retry.next_at - started))
                    continue
                try:
                    if settings.device_id and verified_device_id != settings.device_id:
                        identity = json_request(host, "/api/status", timeout=1.0)
                        if identity.get("device_id") != settings.device_id:
                            raise ValueError("device_mismatch")
                        verified_device_id = settings.device_id
                    if self._stop.is_set(): break
                    data = collect_real(nvml, settings.gpu, "http://127.0.0.1:8085/data.json")
                    if self._stop.is_set(): break
                    result = send_json(target_url(host), self.key, data, timeout=1.3)
                    if self._stop.is_set(): break
                    if result.get("ok") is not True:
                        raise ValueError("Thiết bị từ chối dữ liệu")
                    retry.reset()
                    self._emit({"type": "telemetry", "data": data, "host": host,
                                "emotion": result.get("emotion", "healthy")})
                    if settings.media and started >= next_media:
                        next_media = started + 2.0
                        media_source = self.media_source
                        if media_source:
                            song, cached_pixels = media_source()
                        else:
                            song, cached_pixels = media_provider.snapshot(), None
                        if self._stop.is_set(): break
                        if not media_source:
                            self._emit({"type": "media", "data": song})
                        song_id = media_artwork_id(song)
                        if song_id != self._current_song_id:
                            self._current_song_id = song_id
                            self._last_artwork_id = ""
                            self._artwork_next_try = 0.0
                            if not media_source:
                                self._emit({"type": "artwork", "pixels": None, "id": song_id})
                        try:
                            # Paused/unchanged metadata also needs a heartbeat;
                            # otherwise ESP media expires while Windows keeps
                            # displaying it, breaking the shared state.
                            json_request(host, "/api/v1/media", self.key, wire_media(song))
                        except (HTTPError, URLError, OSError, ValueError, TimeoutError):
                            pass
                        if song_id and song_id != self._last_artwork_id and started >= self._artwork_next_try:
                            # Optional provider: a missing thumbnail means N/A.
                            # Only ever extract once per track unless a network
                            # upload failed (retry with a 15s backoff).
                            pixels = cached_pixels if media_source else media_provider.artwork_rgb565(song)
                            if self._stop.is_set(): break
                            if pixels is None:
                                if not media_source:
                                    self._last_artwork_id = song_id
                            else:
                                self._artwork_next_try = started + 15.0
                                if not media_source:
                                    self._emit({"type": "artwork", "pixels": pixels, "id": song_id})
                                try:
                                    reply = post_artwork(host, self.key, song_id, pixels)
                                    if reply.get("ok"):
                                        self._last_artwork_id = song_id
                                except (HTTPError, URLError, OSError, ValueError, TimeoutError):
                                    pass  # media metadata and PC metrics continue
                    if started - last_status >= 2.0:
                        last_status = started
                        self._process_commands(host)
                        if not self.stream_active:
                            self._poll_focus(host)
                    if not self.stream_active and started - last_motion >= 5.0:
                        last_motion = started
                        self._poll_motion(host)
                except HTTPError as exc:
                    verified_device_id = ""
                    delay = retry.failed(started)
                    self._emit({"type": "error", "message":
                                "Mã ghép nối chưa đúng. Kiểm tra lại trong Thiết bị." if exc.code == 401
                                else f"Thiết bị chưa phản hồi. Sẽ thử lại sau {delay:g} giây.",
                                "host": host, "retry_in": delay, "code": exc.code})
                except (URLError, OSError, ValueError, TimeoutError) as exc:
                    verified_device_id = ""
                    delay = retry.failed(started)
                    self._emit({"type": "error", "host": host, "retry_in": delay,
                                "message": f"Chưa liên lạc được với thiết bị. Sẽ thử lại sau {delay:g} giây."})
                self._stop.wait(max(0.0, settings.interval - (time.monotonic() - started)))
        finally:
            nvml.close()
