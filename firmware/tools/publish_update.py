#!/usr/bin/env python3
"""Stage immutable generic firmware and atomically advance a static update channel.

This command writes local files only. It never uploads, creates a GitHub Release,
changes device policy or flashes an ESP32.
"""
import argparse
from contextlib import contextmanager
import hashlib
import json
import os
from pathlib import Path
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "windows_agent"))
from update_contract import BOARD, LAYOUT, validate_manifest, version_key


@contextmanager
def channel_lock(root):
    root.mkdir(parents=True, exist_ok=True)
    with (root / ".publisher.lock").open("a+b") as lock:
        lock.seek(0, os.SEEK_END)
        if not lock.tell():
            lock.write(b"0"); lock.flush()
        lock.seek(0)
        try:
            if os.name == "nt":
                import msvcrt
                msvcrt.locking(lock.fileno(), msvcrt.LK_NBLCK, 1)
            else:
                import fcntl
                fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except OSError as error:
            raise ValueError("Another publisher is updating this staging directory") from error
        try:
            yield
        finally:
            lock.seek(0)
            if os.name == "nt":
                msvcrt.locking(lock.fileno(), msvcrt.LK_UNLCK, 1)
            else:
                fcntl.flock(lock, fcntl.LOCK_UN)


def atomic_write(path, payload):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(dir=path.parent, prefix=".publish-", delete=False) as out:
            temporary = Path(out.name)
            out.write(payload); out.flush(); os.fsync(out.fileno())
        os.replace(temporary, path)
    finally:
        if temporary is not None and temporary.exists():
            temporary.unlink()


def stage(image, root, *, version, channel, sequence, release_id, artifact_url, notes=(), minimum="2.13.0-dev"):
    image, root = Path(image), Path(root)
    payload = image.read_bytes()
    if len(payload) < 4096 or payload[0] != 0xe9 or payload[12:14] != b"\x09\x00":
        raise ValueError("Expected a compiled ESP32-S3 application")
    marker = ("mimi-public-firmware-v1:" + version).encode("ascii") + b"\0"
    if marker not in payload or b"mimi-private-bridge-v1:" in payload:
        raise ValueError("Only the matching generic/public firmware build can be staged")
    manifest = {"schema": 1, "release_id": release_id, "release_sequence": sequence,
                "version": version, "channel": channel, "board": BOARD, "partition_layout": LAYOUT,
                "minimum_firmware": minimum, "size": len(payload), "sha256": hashlib.sha256(payload).hexdigest(),
                "url": artifact_url, "notes": list(notes)}
    validate_manifest(manifest)
    raw = (json.dumps(manifest, ensure_ascii=False, sort_keys=True, indent=2) + "\n").encode("utf8")
    with channel_lock(root):
        channel_path = root / "channels" / (channel + ".json")
        if channel_path.exists():
            current = json.loads(channel_path.read_text(encoding="utf8"))
            validate_manifest(current, channel=channel)
            if sequence < current["release_sequence"] or version_key(version) < version_key(current["version"]):
                raise ValueError("A channel cannot move backwards")
            if sequence == current["release_sequence"] and current != manifest:
                raise ValueError("The same sequence cannot name different releases")
        destination = root / "releases" / release_id
        for name, content in (("firmware.bin", payload), ("manifest.json", raw)):
            target = destination / name
            if target.exists():
                if target.read_bytes() != content:
                    raise ValueError("An immutable release already exists with different contents")
            else:
                atomic_write(target, content)
        atomic_write(channel_path, raw)
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("image", type=Path)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--version", required=True)
    parser.add_argument("--channel", choices=("preview", "stable"), required=True)
    parser.add_argument("--sequence", type=int, required=True)
    parser.add_argument("--release-id", required=True)
    parser.add_argument("--artifact-url", required=True)
    parser.add_argument("--minimum", default="2.13.0-dev")
    parser.add_argument("--note", action="append", default=[])
    args = parser.parse_args()
    result = stage(args.image, args.out, version=args.version, channel=args.channel, sequence=args.sequence,
                   release_id=args.release_id, artifact_url=args.artifact_url, notes=args.note, minimum=args.minimum)
    print(json.dumps({k: result[k] for k in ("release_id", "size", "sha256")}, indent=2))


if __name__ == "__main__":
    main()
