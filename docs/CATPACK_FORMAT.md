# MèoDesk v1.6 — Cat Pack Studio / `.meocat` v1 and v2

Target: **ESP32-S3 WROOM-1 N16R8** (16 MB Flash, 8 MB PSRAM), ST7789
320×240, LovyanGFX with two internal-RAM DMA strips.

**v1.6 supports two formats:** legacy 648 KiB (16 static frames) and
new 1,944 KiB Motion Pack (16 × 3 frames = 48 frames). The v1 structure and
bank layout below stay valid; see [MOTION_PACK_V2.md](MOTION_PACK_V2.md) for
v2 file names, timing, header compatibility, performance and fallback.

## Fast start

1. First install **MèoDesk v1.6 firmware**. If your board already runs
   v1.3/v1.4 with the dual-slot `meodesk_ota_16mb.csv`, use firmware OTA.
   If still on `huge_app.csv`, read `OTA_MIGRATION.md` and migrate by USB.
2. The ZIP contains `release/meodesk_v15_default_cats.meocat` — sixteen
   verified sprites identical to the built-in collection. Try this file first
   so that Cat Pack OTA can be tested without making custom pictures.
   Then test `release/meodesk_v16_motion_demo.meocat` (48 demo frames).
3. In Windows Companion: **Thiết bị** → IP/pairing key → **Cat Packs** →
   select `.meocat` → **CÀI BỘ MÈO MỚI QUA WI-FI**. Or use Web Studio's
   **Cat Pack Studio** panel at `http://<device-IP>/`.
4. Upload needs an exact SHA-256 and 663,552 bytes. The TFT shows progress.
   On successful verification the ESP32 restarts once, verifies the bank and
   loads all sprites into PSRAM. Companion/Web Studio wait for `/api/v1/catpack/status`
   to show `source:custom` and the same SHA-256.

## Create your own pack

Prepare sixteen transparent PNG files, each exactly **144×144**, in a
directory. Use the following *exact filenames and order*:

| Index | PNG filename | Scene/meaning |
|---:|---|---|
| 0 | `boot.png` | Boot and Moto |
| 1 | `clock.png` | Clock |
| 2 | `paused.png` | Deep Focus |
| 3 | `sleep.png` | Sleep |
| 4 | `storage.png` | Storage/busy |
| 5 | `network.png` | Network |
| 6 | `offline.png` | Offline fallback |
| 7 | `music.png` | Now Playing |
| 8 | `bambu.png` | Printer |
| 9 | `audio.png` | Audio |
| 10 | `health.png` | Health |
| 11 | `healthy.png` | Normal/celebrating |
| 12 | `hotspot.png` | Access point |
| 13 | `power.png` | Powered/charging |
| 14 | `rescue.png` | Waiting for Windows |
| 15 | `shutdown.png` | Low battery |

Run from the `firmware/` directory:

```powershell
py -m pip install Pillow
py tools/make_catpack.py --images images --out my_cats.meocat
```

Swap `images` for your own PNG folder as desired. The script generates:

- `my_cats.meocat`: **663,552 bytes**, uncompressed, sixteen 144×144 RGB565
  frames, each pixel **little-endian**, row-major. Alpha is flattened over
  warm ivory RGB (255, 248, 241), matching the existing TFT source assets.
- `my_cats.manifest.json`: frame order, SHA-256, size. It is **not signed**.

This is a raw, **headerless v1 pixel payload**. Never prepend the JSON or a
custom header to `.meocat`, and never send a PNG directly as `.meocat`.
The PC/browser computes its SHA-256 before upload. The ESP32 validates it
before activating the bank.

## Flash map and power-loss behavior

The existing 16 MB partition table is **unchanged**. The `assets` partition
occupies `0xC10000..0xFEFFFF` (0x3E0000 = 3.875 MiB). MèoDesk does not mount
that region as SPIFFS despite its partition *subtype*; it is used as two raw
cat banks, each 0x1F0000 (1.9375 MiB):

| Bank | Flash address | Commit header | RGB565 pixels start |
|---|---|---|---|
| A | `0xC10000` | `0xC10000` (60 bytes) | `0xC11000` |
| B | `0xE00000` | `0xE00000` (60 bytes) | `0xE01000` |

Only 4 KiB header sector plus the 648 KiB payload region are erased during
an update; the remainder is reserved for future formats. Each commit header
contains `MEOCAT15` magic, format version, generation, expected geometry,
payload length and a 32-byte SHA-256. The header is written **last**, after
the full payload passes SHA-256 verification.

At boot, MèoDesk examines both headers and hashes the newest bank while
loading its pixels into PSRAM. If corrupted, it tries the previous bank.
If both are unusable or 648 KiB PSRAM cannot be allocated, it uses the
original `PROGMEM` cat sprites embedded in the firmware. DMA strips stay in
small internal SRAM buffers. **No full-screen frame buffer is allocated.**

Power-loss and recovery logic passed host tests, but has **not been tested on
the physical N16R8**. Keep USB recovery available. On first test, verify
correct RGB color order and actual sustained cat FPS in both custom and
built-in modes. PSRAM reads could affect FPS under load; no number is promised.

## API (authenticated local HTTP)

`GET /api/v1/catpack/status` (read-only):

```json
{
  "schema": 1,
  "board": "meodesk-esp32s3-n16r8",
  "ready": true,
  "source": "custom",
  "active_bank": 0,
  "generation": 1,
  "sha256": "...64 lowercase hex characters...",
  "frame_count": 16,
  "width": 144,
  "height": 144,
  "pack_bytes": 663552,
  "phase": "idle"
}
```

`POST /api/v1/catpack/upload`: multipart `catpack` file, with:

- `X-MEO-KEY`: private pairing key (NOT Wi-Fi password)
- `X-MEO-CAT-SIZE`: `663552`
- `X-MEO-CAT-SHA256`: 64 lowercase/uppercase hex digits of raw `.meocat`

Successful HTTP response means **bytes verified and restart scheduled**;
client success is not reported until it observes the matching SHA on the
running device after reboot. This is developer-only unsigned HTTP on a trusted
LAN; **never port-forward port 80** or publish private credentials.

## Limitations and future extensions

- Fixed 16-frame layout. Changing resolution or adding frames needs a new
  versioned pack format and renderer support. The bank structure has spare
  Flash for that evolution.
- No dynamic JPEG/PNG decoding on ESP32: conversion happens on Windows/PC.
- Changing the 16 poses does not add arbitrary new animations; existing bob,
  fade and event behavior remain.
- Future: custom theme palettes, animated multi-frame tracks, signed asset
  bundles, optional bank preview and a safe restore-factory UI.
