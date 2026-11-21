"""Shared release-feed validation for the desktop and offline publisher."""
from dataclasses import dataclass
import ipaddress
import re
from urllib.parse import urlsplit

BOARD = "meodesk-esp32s3-n16r8"
LAYOUT = "meodesk-ab-6m-assets-v1"
VERSION = re.compile(r"(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)(?:-([0-9A-Za-z.-]+))?(?:\+([0-9A-Za-z.-]+))?\Z")


def version_key(value):
    if not isinstance(value, str) or len(value) > 63:
        raise ValueError("Invalid version")
    match = VERSION.fullmatch(value)
    if not match:
        raise ValueError("Invalid version")
    major, minor, patch, pre, build = match.groups()
    numbers = tuple(int(n) for n in (major, minor, patch))
    if any(n > 0xFFFFFFFF for n in numbers):
        raise ValueError("Version component too large")
    for field in (pre, build):
        if field is not None and any(not part for part in field.split(".")):
            raise ValueError("Empty version identifier")
    if pre and (len(pre) > 31 or any(p.isdigit() and len(p) > 1 and p[0] == "0" for p in pre.split("."))):
        raise ValueError("Invalid prerelease")
    identifiers = tuple((0, int(p)) if p.isdigit() else (1, p) for p in pre.split(".")) if pre else ()
    return numbers, (0, identifiers) if pre else (1, ())


def valid_url(value):
    if not isinstance(value, str) or not 10 <= len(value) <= 511 or any(ord(c) <= 32 for c in value):
        return False
    try:
        url = urlsplit(value)
        if not url.hostname or url.username is not None or url.password is not None or url.fragment:
            return False
        if url.port is not None and not 1 <= url.port <= 65535:
            return False
        if url.scheme == "https":
            return True
        if url.scheme != "http":
            return False
        host = url.hostname
        if host.endswith(".local"):
            return True
        address = ipaddress.IPv4Address(host)
        return any(address in ipaddress.IPv4Network(n) for n in
                   ("10.0.0.0/8", "172.16.0.0/12", "192.168.0.0/16", "169.254.0.0/16"))
    except ValueError:
        return False


@dataclass(frozen=True)
class Release:
    release_id: str
    version: str
    sequence: int
    size: int
    sha256: str
    url: str
    channel: str


def validate_manifest(data, *, channel=None, current_version=None, slot_bytes=6*1024*1024):
    if not isinstance(data, dict) or type(data.get("schema")) is not int or data["schema"] != 1:
        raise ValueError("Invalid manifest schema")
    if data.get("board") != BOARD or data.get("partition_layout") != LAYOUT:
        raise ValueError("Incompatible board or partition layout")
    selected = data.get("channel")
    if selected not in ("stable", "preview") or (channel and selected != channel):
        raise ValueError("Channel mismatch")
    release_id = data.get("release_id", "")
    if not isinstance(release_id, str) or not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9.-]{0,62}", release_id):
        raise ValueError("Invalid release ID")
    for name in ("version", "minimum_firmware"):
        value = data.get(name)
        version_key(value)
        if len(value) > 31:
            raise ValueError("Version is too long for the device")
    if current_version is not None and version_key(current_version) < version_key(data["minimum_firmware"]):
        raise ValueError("A bridge firmware update is required")
    size, sequence = data.get("size"), data.get("release_sequence")
    if type(size) is not int or not 4096 <= size <= slot_bytes:
        raise ValueError("Image exceeds the inactive slot")
    if type(sequence) is not int or not 1 <= sequence <= 0xFFFFFFFF:
        raise ValueError("Invalid release sequence")
    sha = data.get("sha256", "")
    if not isinstance(sha, str) or not re.fullmatch(r"[0-9a-fA-F]{64}", sha) or not valid_url(data.get("url")):
        raise ValueError("Invalid artifact URL or SHA-256")
    notes = data.get("notes", [])
    if not isinstance(notes, list) or len(notes) > 8 or any(not isinstance(n, str) or len(n.encode("utf8")) > 512 for n in notes):
        raise ValueError("Invalid release notes")
    return Release(release_id, data["version"], sequence, size, sha.lower(), data["url"], selected)
