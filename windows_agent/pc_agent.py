#!/usr/bin/env python3
"""MèoDesk v0.7 - Windows performance metrics -> ESP32-S3 over local Wi-Fi.

Required: psutil. Optional NVIDIA GPU: nvidia-ml-py (imported as pynvml).
AMD/Intel/NVIDIA alternative: LibreHardwareMonitor's local HTTP data.json.
No telemetry is sent to the Internet: the only POST target is --host.
"""

from __future__ import annotations

import argparse
import json
import math
import platform
import re
import socket
import sys
import time
from dataclasses import dataclass
from typing import Any
from urllib.error import HTTPError, URLError
from urllib.request import ProxyHandler, Request, build_opener

# Device telemetry must stay on the local network, even if Windows has
# HTTP_PROXY/HTTPS_PROXY environment variables configured.
_direct_http = build_opener(ProxyHandler({}))

try:
    import psutil
except ImportError:
    raise SystemExit(
        "Missing psutil. In windows_agent run: py -m pip install -r requirements.txt"
    )


@dataclass
class GpuSample:
    usage: float | None = None
    temp_c: float | None = None
    name: str = ""
    cpu_temp_c: float | None = None
    vram_used_gb: float | None = None
    vram_total_gb: float | None = None


class NetworkRateSampler:
    """Two consecutive cumulative samples -> real network throughput.

    First call returns null, not zero; no artificial values when counters reset.
    Windows active interface selection is platform-specific, so rates describe
    the aggregate psutil network counters, not an assumed Wi-Fi-only adapter.
    """

    def __init__(self) -> None:
        self.previous: tuple[int, int, float] | None = None

    def sample(self, counters: Any, now: float) -> tuple[float | None, float | None]:
        if counters is None:
            return None, None
        current = (int(counters.bytes_sent), int(counters.bytes_recv), float(now))
        old = self.previous
        self.previous = current
        if old is None:
            return None, None
        elapsed = current[2] - old[2]
        if elapsed <= 0.15 or current[0] < old[0] or current[1] < old[1]:
            return None, None
        upstream = (current[0] - old[0]) / elapsed / 1024.0
        downstream = (current[1] - old[1]) / elapsed / 1024.0
        return round(upstream, 1), round(downstream, 1)


_network_sampler = NetworkRateSampler()


def percent(value: float | int | None) -> float | None:
    """Keep numeric samples in 0..100 and reject unavailable/NaN data."""
    if value is None:
        return None
    try:
        number = float(value)
    except (TypeError, ValueError):
        return None
    return round(number, 1) if math.isfinite(number) and 0 <= number <= 100 else None


def sensor_number(value: Any) -> float | None:
    """Parse values such as '72.6 %', '54.0 °C', or '72,6 %'."""
    if isinstance(value, (float, int)):
        return float(value) if math.isfinite(float(value)) else None
    match = re.search(r"[-+]?\d+(?:[.,]\d+)?", str(value))
    if not match:
        return None
    try:
        return float(match.group(0).replace(",", "."))
    except ValueError:
        return None


def extract_lhm_gpu(tree: dict[str, Any]) -> GpuSample:
    """Select GPU Core/Total sensors in LibreHardwareMonitor data.json."""
    loads: list[tuple[int, float]] = []
    temperatures: list[tuple[int, float]] = []
    cpu_temperatures: list[tuple[int, float]] = []
    gpu_name = ""

    def visit(node: Any, gpu_parent: bool = False, cpu_parent: bool = False) -> None:
        nonlocal gpu_name
        if not isinstance(node, dict):
            return
        name = str(node.get("Text", ""))
        sensor_id = str(node.get("SensorId", "")).lower()
        device_name = name.lower()
        this_gpu = gpu_parent or "/gpu-" in sensor_id or any(
            token in device_name
            for token in (
                "nvidia", "geforce", "radeon", "intel arc", "intel iris", "intel uhd"
            )
        )
        this_cpu = cpu_parent or any(
            token in sensor_id for token in ("/cpu", "/amdcpu", "/intelcpu")
        ) or any(token in device_name for token in ("ryzen", "intel core", "cpu package"))
        if this_gpu and not gpu_name and not node.get("SensorId"):
            gpu_name = name[:35]

        kind = str(node.get("Type", "")).lower()
        number = sensor_number(node.get("Value"))
        if this_gpu and number is not None:
            if kind == "load" or ("gpu" in device_name and "%" in str(node.get("Value"))):
                priorities = ("gpu core", "gpu total", "gpu d3d 3d", "gpu 3d", "3d")
                score = next((i for i, p in enumerate(priorities) if p in device_name), 99)
                if score < 99 and percent(number) is not None:
                    loads.append((score, number))
            if kind == "temperature":
                score = 0 if "gpu core" in device_name else 1 if "gpu hot spot" in device_name else 99
                if score < 99 and -30 <= number <= 160:
                    temperatures.append((score, number))

        if this_cpu and kind == "temperature" and number is not None and -30 <= number <= 160:
            names = ("cpu package", "core (tctl/tdie)", "cpu (tctl/tdie)", "core average")
            score = next((i for i, candidate in enumerate(names) if candidate in device_name), 99)
            if score < 99:
                cpu_temperatures.append((score, number))

        for child in node.get("Children", []):
            visit(child, this_gpu, this_cpu)

    visit(tree)
    usage = min(loads)[1] if loads else None
    temp = min(temperatures)[1] if temperatures else None
    cpu_temp = min(cpu_temperatures)[1] if cpu_temperatures else None
    name = (gpu_name or "GPU / LibreHardwareMonitor") if usage is not None or temp is not None else ""
    return GpuSample(usage=percent(usage), temp_c=temp, name=name, cpu_temp_c=cpu_temp)


class NvidiaGpu:
    """NVML uses the installed NVIDIA driver; no subprocess or nvidia-smi parsing."""

    @classmethod
    def disabled(cls) -> NvidiaGpu:
        instance = cls.__new__(cls)
        instance._nvml = None
        instance._handle = None
        return instance

    def __init__(self) -> None:
        self._nvml: Any = None
        self._handle: Any = None
        try:
            import pynvml  # from nvidia-ml-py

            pynvml.nvmlInit()
            if pynvml.nvmlDeviceGetCount() > 0:
                self._nvml = pynvml
                self._handle = pynvml.nvmlDeviceGetHandleByIndex(0)
        except Exception:
            self._nvml = None
            self._handle = None

    @property
    def available(self) -> bool:
        return self._nvml is not None and self._handle is not None

    def sample(self) -> GpuSample:
        if not self.available:
            return GpuSample()
        nvml, handle = self._nvml, self._handle
        try:
            load = percent(nvml.nvmlDeviceGetUtilizationRates(handle).gpu)
        except Exception:
            load = None
        try:
            temperature = float(nvml.nvmlDeviceGetTemperature(handle, nvml.NVML_TEMPERATURE_GPU))
        except Exception:
            temperature = None
        try:
            name = nvml.nvmlDeviceGetName(handle)
            if isinstance(name, bytes):
                name = name.decode("utf-8", "replace")
        except Exception:
            name = "NVIDIA GPU"
        try:
            memory = nvml.nvmlDeviceGetMemoryInfo(handle)
            vram_used = round(memory.used / (1024 ** 3), 2)
            vram_total = round(memory.total / (1024 ** 3), 2)
        except Exception:
            vram_used = vram_total = None
        return GpuSample(load, temperature, str(name)[:35], None,
                         vram_used, vram_total)

    def close(self) -> None:
        if self._nvml is not None:
            try:
                self._nvml.nvmlShutdown()
            except Exception:
                pass


def get_lhm_gpu(url: str) -> GpuSample:
    try:
        with _direct_http.open(url, timeout=0.8) as response:
            obj = json.load(response)
        return extract_lhm_gpu(obj) if isinstance(obj, dict) else GpuSample()
    except (URLError, TimeoutError, OSError, ValueError):
        return GpuSample()


def pick_gpu(provider: str, nvidia: NvidiaGpu, lhm_url: str) -> GpuSample:
    if provider == "none":
        return GpuSample()
    if provider in ("auto", "nvidia") and nvidia.available:
        sample = nvidia.sample()
        if sample.usage is not None or provider == "nvidia":
            return sample
    if provider in ("auto", "lhm"):
        return get_lhm_gpu(lhm_url)
    return GpuSample()


def read_battery() -> tuple[float | None, bool | None]:
    """Battery is absent on most desktop computers, never synthesize one."""
    try:
        battery = psutil.sensors_battery()
        if battery is None:
            return None, None
        return percent(battery.percent), bool(battery.power_plugged)
    except (AttributeError, NotImplementedError, OSError):
        return None, None


def read_cpu_temperature() -> float | None:
    """psutil temperature sensors may be unavailable on Windows; return null."""
    try:
        groups = psutil.sensors_temperatures(fahrenheit=False)
    except (AttributeError, NotImplementedError, OSError):
        return None
    for group_name, readings in groups.items():
        for sensor in readings:
            label = (sensor.label or "").lower()
            if any(token in label for token in ("cpu package", "tctl", "tdie")) or (
                "coretemp" in group_name.lower() and "package" in label
            ):
                value = sensor.current
                if value is not None and -30 <= value <= 160:
                    return round(float(value), 1)
    return None


def collect_real(nvidia: NvidiaGpu, gpu_provider: str, lhm_url: str) -> dict[str, Any]:
    cpu = psutil.cpu_percent(interval=None)
    vm = psutil.virtual_memory()
    gpu = pick_gpu(gpu_provider, nvidia, lhm_url)
    battery_percent, plugged = read_battery()
    cpu_temp = read_cpu_temperature()
    if cpu_temp is None:
        cpu_temp = gpu.cpu_temp_c  # reuse the same LHM HTTP response, no extra request
    gib = 1024**3
    net_up, net_down = _network_sampler.sample(psutil.net_io_counters(), time.monotonic())
    return {
        "pc_name": platform.node()[:29] or socket.gethostname()[:29],
        "cpu_percent": percent(cpu) or 0.0,
        "ram_percent": percent(vm.percent) or 0.0,
        "ram_used_gb": round((vm.total - vm.available) / gib, 2),
        "ram_total_gb": round(vm.total / gib, 2),
        "gpu_percent": gpu.usage,
        "gpu_temp_c": gpu.temp_c,
        "gpu_name": gpu.name,
        "vram_used_gb": gpu.vram_used_gb,
        "vram_total_gb": gpu.vram_total_gb,
        "net_up_kbps": net_up,
        "net_down_kbps": net_down,
        "cpu_temp_c": cpu_temp,
        "battery_percent": battery_percent,
        "battery_plugged": plugged,
        "pc_uptime_s": max(0, int(time.time() - psutil.boot_time())),
        "demo": False,
    }


def collect_demo() -> dict[str, Any]:
    t = time.monotonic()
    cpu = 45 + 35 * math.sin(t / 6)
    gpu = 40 + 36 * math.sin(t / 9 + 1)
    ram = 55 + 7 * math.sin(t / 17)
    return {
        "pc_name": "MEO DEMO (SIMULATED)",
        "cpu_percent": round(cpu, 1),
        "ram_percent": round(ram, 1),
        "ram_used_gb": round(24 * ram / 100, 2),
        "ram_total_gb": 24.0,
        "gpu_percent": round(gpu, 1),
        "gpu_temp_c": 55.0,
        "gpu_name": "SIMULATED GPU",
        "vram_used_gb": 2.0,
        "vram_total_gb": 4.0,
        "net_up_kbps": 120.0,
        "net_down_kbps": 540.0,
        "cpu_temp_c": 48.0,
        "battery_percent": 72.0,
        "battery_plugged": False,
        "pc_uptime_s": 46850,
        "demo": True,
    }


def target_url(host: str) -> str:
    host = host.strip().rstrip("/")
    if not host.startswith(("http://", "https://")):
        host = "http://" + host
    return host + "/api/telemetry"


def send_json(url: str, key: str, payload: dict[str, Any], timeout: float = 2.0) -> dict[str, Any]:
    body = json.dumps(payload, separators=(",", ":"), allow_nan=False).encode("utf-8")
    req = Request(
        url,
        data=body,
        headers={"Content-Type": "application/json", "X-MEO-KEY": key},
        method="POST",
    )
    with _direct_http.open(req, timeout=timeout) as resp:
        return json.load(resp)


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description="Send live Windows telemetry to MèoDesk 0.7")
    ap.add_argument("--host", default="192.168.4.1", help="ESP32 IP or meodesk.local")
    ap.add_argument("--key", default="CHANGE_TO_A_LONG_RANDOM_KEY", help="Match MEO_API_KEY in src/app_config.h")
    ap.add_argument("--interval", type=float, default=1.0, help="Send rate in seconds (min 0.3)")
    ap.add_argument("--gpu", choices=("auto", "nvidia", "lhm", "none"), default="auto")
    ap.add_argument("--lhm-url", default="http://127.0.0.1:8085/data.json",
                    help="LibreHardwareMonitor local HTTP JSON endpoint")
    ap.add_argument("--demo", action="store_true", help="EXPLICIT synthetic test data (not actual PC load)")
    ap.add_argument("--dry-run", action="store_true", help="Print one telemetry JSON, do not POST")
    args = ap.parse_args(argv)
    interval = max(0.3, min(args.interval, 10.0))
    url = target_url(args.host)
    # Keep the object fully initialized even if NVIDIA support is not used.
    nvml = NvidiaGpu() if args.gpu in ("auto", "nvidia") and not args.demo else NvidiaGpu.disabled()
    psutil.cpu_percent(interval=None)  # warm up the nonblocking CPU sampler
    if args.dry_run and not args.demo:
        time.sleep(0.2)  # first sample otherwise frequently reports 0.0% on Windows

    print("MèoDesk PC Agent v0.7")
    print("Destination:", url)
    print("Data mode: ", "SIMULATED --demo" if args.demo else "REAL Windows sensors")
    print("GPU:       ", args.gpu, "(NVML available)" if nvml.available else "(NVML unavailable)")
    print("Press Ctrl+C to stop. API token is not printed.\n")

    try:
        while True:
            cycle = time.monotonic()
            data = collect_demo() if args.demo else collect_real(nvml, args.gpu, args.lhm_url)
            if args.dry_run:
                print(json.dumps(data, indent=2, ensure_ascii=False))
                return 0
            try:
                result = send_json(url, args.key, data)
                if not result.get("ok"):
                    raise ValueError("device returned ok=false")
                print("\rONLINE | CPU {:5.1f}% | RAM {:5.1f}% | GPU {:>5} | {}      ".format(
                    data["cpu_percent"], data["ram_percent"],
                    "N/A" if data["gpu_percent"] is None else f'{data["gpu_percent"]:.1f}%',
                    result.get("emotion", "unknown")), end="", flush=True)
            except HTTPError as exc:
                print(f"\nHTTP {exc.code}: {'check --key / firmware API key' if exc.code == 401 else exc.reason}")
            except (URLError, TimeoutError, OSError, ValueError) as exc:
                print(f"\nOffline, retrying: {exc}")
            remaining = interval - (time.monotonic() - cycle)
            if remaining > 0:
                time.sleep(remaining)
    except KeyboardInterrupt:
        print("\nAgent stopped.")
        return 0
    finally:
        nvml.close()


if __name__ == "__main__":
    sys.exit(main())
