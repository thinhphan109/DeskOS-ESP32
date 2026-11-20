#!/usr/bin/env python3
"""MèoDesk 0.7 Windows notification-area agent.

Run ``py tray_agent.py --config agent_config.json`` or install the optional
Startup shortcut. All telemetry is posted directly to the ESP32 on local LAN.
The regular console ``pc_agent.py`` remains available for troubleshooting.
"""

from __future__ import annotations

import argparse
import json
import threading
import time
import webbrowser
from dataclasses import dataclass
from pathlib import Path
from typing import Any
from urllib.error import HTTPError, URLError

from pc_agent import NvidiaGpu, collect_demo, collect_real, psutil, send_json, target_url


@dataclass(frozen=True)
class AgentConfig:
    host: str = "192.168.4.1"
    key: str = "CHANGE_TO_A_LONG_RANDOM_KEY"
    interval: float = 1.0
    gpu: str = "auto"
    lhm_url: str = "http://127.0.0.1:8085/data.json"
    demo: bool = False


def read_config(path: Path) -> AgentConfig:
    """Load user-owned JSON, reject accidental malformed overrides."""
    if not path.is_file():
        raise FileNotFoundError("Copy agent_config.example.json to agent_config.json and set the ESP32 Router IP")
    parsed = json.loads(path.read_text(encoding="utf-8-sig"))
    if not isinstance(parsed, dict):
        raise ValueError("Configuration file must contain a JSON object")
    data: dict[str, Any] = parsed
    valid_fields = set(AgentConfig.__dataclass_fields__)
    unknown = set(data) - valid_fields
    if unknown:
        raise ValueError(f"Unknown config settings: {', '.join(sorted(unknown))}")
    config = AgentConfig(**data)
    if not config.host.strip() or not config.key.strip():
        raise ValueError("host and key cannot be empty")
    if not 0.3 <= float(config.interval) <= 10:
        raise ValueError("interval must be between 0.3 and 10 seconds")
    if config.gpu not in ("auto", "nvidia", "lhm", "none"):
        raise ValueError("gpu must be auto, nvidia, lhm or none")
    if not isinstance(config.demo, bool):
        raise ValueError("demo must be true or false")
    return config


def dashboard_url(host: str) -> str:
    return target_url(host).removesuffix("/api/telemetry") + "/"


class AgentWorker:
    """Background sender. Network errors never terminate the tray icon."""

    def __init__(self, config: AgentConfig) -> None:
        self.config = config
        self._stop = threading.Event()
        self._paused = threading.Event()
        self._lock = threading.Lock()
        self._status = "Connecting to ESP32..."
        self._icon: Any = None
        self._thread: threading.Thread | None = None

    @property
    def paused(self) -> bool:
        return self._paused.is_set()

    @property
    def status(self) -> str:
        with self._lock:
            return self._status

    def _set_status(self, text: str) -> None:
        with self._lock:
            self._status = text
        icon = self._icon
        if icon is not None:
            try:
                icon.title = "MèoDesk · " + text[:95]
                icon.update_menu()
            except (AttributeError, RuntimeError):
                pass

    def attach_icon(self, icon: Any) -> None:
        self._icon = icon

    def start(self) -> None:
        if self._thread and self._thread.is_alive():
            return
        self._thread = threading.Thread(target=self._run, name="MeoDeskTelemetry", daemon=True)
        self._thread.start()

    def toggle_pause(self) -> None:
        if self._paused.is_set():
            self._paused.clear()
            self._set_status("Resuming stream...")
        else:
            self._paused.set()
            self._set_status("Paused by user")

    def stop(self) -> None:
        self._stop.set()
        if self._thread and threading.current_thread() is not self._thread:
            self._thread.join(timeout=3.0)

    def _run(self) -> None:
        cfg = self.config
        nvml = NvidiaGpu() if cfg.gpu in ("auto", "nvidia") and not cfg.demo else NvidiaGpu.disabled()
        try:
            psutil.cpu_percent(interval=None)
            while not self._stop.is_set():
                if self._paused.is_set():
                    self._stop.wait(0.2)
                    continue
                started = time.monotonic()
                try:
                    data = collect_demo() if cfg.demo else collect_real(nvml, cfg.gpu, cfg.lhm_url)
                    response = send_json(target_url(cfg.host), cfg.key, data, timeout=1.8)
                    if not response.get("ok"):
                        raise ValueError("Device rejected telemetry")
                    gpu = data.get("gpu_percent")
                    gpu_part = "N/A" if gpu is None else f"{gpu:.0f}%"
                    label = "DEMO" if cfg.demo else "LIVE"
                    self._set_status(f"{label} · CPU {data['cpu_percent']:.0f}% · GPU {gpu_part} · RAM {data['ram_percent']:.0f}%")
                except HTTPError as exc:
                    self._set_status(f"HTTP {exc.code} · Check ESP32 IP / pairing key")
                except (URLError, TimeoutError, OSError, ValueError) as exc:
                    # Keep status concise: a menu can become unreadable otherwise.
                    self._set_status(f"Offline · {str(exc)[:55]}")
                self._stop.wait(max(0.0, float(cfg.interval) - (time.monotonic() - started)))
        finally:
            nvml.close()


def icon_image() -> Any:
    from PIL import Image, ImageDraw

    base = Image.new("RGBA", (64, 64), (0, 0, 0, 0))
    draw = ImageDraw.Draw(base)
    draw.rounded_rectangle((1, 1, 62, 62), radius=18, fill=(41, 58, 54, 255))
    asset = Path(__file__).resolve().parents[1] / "images" / "healthy.png"
    if asset.is_file():
        with Image.open(asset) as cat:
            cat = cat.convert("RGBA")
            cat.thumbnail((58, 58), Image.Resampling.LANCZOS)
            base.alpha_composite(cat, ((64 - cat.width) // 2, (64 - cat.height) // 2))
    else:
        draw.polygon([(11, 29), (11, 9), (26, 21)], fill=(255, 217, 179))
        draw.polygon([(53, 29), (53, 9), (38, 21)], fill=(255, 217, 179))
        draw.ellipse((10, 17, 54, 56), fill=(255, 230, 192))
        draw.ellipse((23, 35, 27, 39), fill="#2c2d35")
        draw.ellipse((38, 35, 42, 39), fill="#2c2d35")
        draw.arc((27, 38, 38, 46), 0, 180, fill="#2c2d35", width=2)
    return base


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="MèoDesk 0.7 Windows tray agent")
    parser.add_argument("--config", default=str(Path(__file__).with_name("agent_config.json")))
    options = parser.parse_args(argv)
    try:
        cfg = read_config(Path(options.config))
    except (OSError, ValueError, TypeError) as exc:
        parser.error(f"Invalid agent config: {exc}")

    # Imported only in tray mode; pc_agent.py remains runnable without GUI libs.
    try:
        import pystray
    except ImportError:
        parser.error("Install tray dependencies: py -m pip install -r requirements.txt")

    worker = AgentWorker(cfg)
    url = dashboard_url(cfg.host)

    def open_dashboard(icon: Any = None, item: Any = None) -> None:
        webbrowser.open(url)

    def toggle(icon: Any, item: Any) -> None:
        worker.toggle_pause()

    def quit_app(icon: Any, item: Any) -> None:
        worker._stop.set()
        icon.stop()

    menu = pystray.Menu(
        pystray.MenuItem(lambda item: worker.status, None, enabled=False),
        pystray.Menu.SEPARATOR,
        pystray.MenuItem("Open MèoDesk Web UI", open_dashboard, default=True),
        pystray.MenuItem(lambda item: "Resume streaming" if worker.paused else "Pause streaming", toggle),
        pystray.MenuItem("Exit MèoDesk", quit_app),
    )
    icon = pystray.Icon("meodesk", icon_image(), "MèoDesk · Starting...", menu)
    worker.attach_icon(icon)
    worker.start()
    try:
        icon.run()  # Blocking Windows notification-area event loop.
    finally:
        worker.stop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
