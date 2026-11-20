"""Local-only ESP32 device discovery; does not require Qt or a Wi-Fi password."""
from __future__ import annotations

import concurrent.futures
import ipaddress
import json
import socket
from urllib.request import Request, build_opener, ProxyHandler


_opener = build_opener(ProxyHandler({}))


def normalize_host(value: str) -> str:
    """Accept an IPv4 address, hostname, or URL, returning an HTTP host."""
    from urllib.parse import urlsplit

    value = value.strip()
    if not value:
        raise ValueError("Nhập IP hoặc tên thiết bị")
    if "://" in value:
        url = urlsplit(value)
        if url.scheme != "http" or url.port not in (None, 80) or not url.hostname:
            raise ValueError("Thiết bị chỉ hỗ trợ HTTP port 80 trên LAN")
        value = url.hostname
    if any(c in value for c in "/?#@\\ ") or len(value) > 253:
        raise ValueError("IP hoặc hostname không hợp lệ")
    try:
        ip = ipaddress.ip_address(value)
        if ip.version != 4 or not (ip.is_private or ip.is_link_local):
            raise ValueError("Chỉ kết nối tới địa chỉ nội bộ")
    except ValueError as exc:
        # A syntactically valid public IPv4 address must NOT fall back to a hostname.
        if all(c.isdigit() or c == "." for c in value):
            raise ValueError("Chỉ dùng IPv4 nội bộ (LAN)") from exc
        if not value.lower().endswith(".local"):
            raise ValueError("Tên thiết bị phải có đuôi .local") from exc
    return value.lower()


def probe(host: str, timeout: float = 0.6) -> dict | None:
    """Accept only recognizable MèoDesk devices, not arbitrary HTTP endpoints."""
    try:
        host = normalize_host(host)
        req = Request(f"http://{host}/api/status", headers={"Accept": "application/json"})
        with _opener.open(req, timeout=timeout) as response:
            if response.status != 200 or int(response.headers.get("Content-Length", "0") or "0") > 16384:
                return None
            body = json.loads(response.read(16385))
        if not isinstance(body, dict) or body.get("device") != "MEO DESK":
            return None
        return {"host": host, "ip": body.get("ip", host), "version": body.get("version", "?"),
                "name": body.get("device", "MEO DESK"), "pc_connected": bool(body.get("pc_connected"))}
    except (OSError, ValueError, TimeoutError, json.JSONDecodeError, KeyError):
        return None


def _private_ipv4_prefixes() -> list[str]:
    """Derive only attached private /24 networks. Never sweep public networks."""
    candidates: set[str] = set()
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
            s.connect(("192.0.2.1", 9))  # UDP route lookup only; sends no traffic.
            candidates.add(s.getsockname()[0])
    except OSError:
        pass
    try:
        for info in socket.getaddrinfo(socket.gethostname(), None, socket.AF_INET):
            candidates.add(info[4][0])
    except OSError:
        pass
    prefixes: set[str] = set()
    for candidate in candidates:
        ip = ipaddress.ip_address(candidate)
        if ip.is_private and not ip.is_loopback and not ip.is_link_local:
            prefixes.add(".".join(candidate.split(".")[:3]))
    return sorted(prefixes)[:2]


def discover(last_host: str = "", *, lan_scan: bool = False, max_hosts: int = 254) -> list[dict]:
    """Prioritize saved IP then mDNS; optionally do a capped /24 LAN scan."""
    initial = [last_host, "meodesk.local", "192.168.4.1"]
    found: dict[str, dict] = {}
    for candidate in initial:
        if candidate:
            result = probe(candidate, timeout=0.7)
            if result:
                found[result["host"]] = result
    if lan_scan:
        addresses: list[str] = []
        for prefix in _private_ipv4_prefixes():
            addresses.extend(f"{prefix}.{i}" for i in range(1, 255))
        # Explicit user action only: 254 IPs max, each with short timeout.
        with concurrent.futures.ThreadPoolExecutor(max_workers=20) as pool:
            for result in pool.map(probe, addresses[:max_hosts]):
                if result:
                    found[result["host"]] = result
    return list(found.values())
