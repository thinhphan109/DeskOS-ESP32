#!/usr/bin/env python3
"""Generate a local OTA release manifest for a *compiled* N16R8 firmware.bin.

This is a checksum/inventory manifest, NOT a cryptographic signature.
It never contains pairing keys or Wi-Fi passwords. Firmware must be built in
PlatformIO first; this tool cannot compile or validate a running ESP32.
"""

import argparse
import hashlib
import json
from pathlib import Path

BOARD = "meodesk-esp32s3-n16r8"
MAX_BYTES = 6 * 1024 * 1024


def describe(path: Path, version: str) -> dict:
    data_size = path.stat().st_size
    if path.suffix.lower() != ".bin" or not 4096 <= data_size <= MAX_BYTES:
        raise ValueError("Expected a 4 KiB..6 MiB compiled ESP32 .bin")
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        header = stream.read(14)
        if header[:1] != b"\xe9" or header[12:14] != b"\x09\x00":
            raise ValueError("Not an ESP32 application image")
        stream.seek(0)
        for chunk in iter(lambda: stream.read(65536), b""):
            digest.update(chunk)
    return {"schema": 1, "board": BOARD, "firmware_version": version,
            "filename": path.name, "size": data_size, "sha256": digest.hexdigest(),
            "signature": None, "transport": "local-pairing-http"}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("firmware", type=Path, help="PlatformIO firmware.bin")
    parser.add_argument("--version", default="1.7.0")
    parser.add_argument("--out", type=Path, help="Manifest output path")
    args = parser.parse_args()
    target = args.out or args.firmware.with_name(args.firmware.stem + ".manifest.json")
    target.write_text(json.dumps(describe(args.firmware, args.version), indent=2) + "\n", encoding="utf-8")
    print(f"Manifest generated: {target}")


if __name__ == "__main__":
    main()
