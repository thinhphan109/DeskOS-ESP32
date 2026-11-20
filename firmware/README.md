# MèoDesk v1.7 — PlatformIO ESP32-S3 WROOM-1 N16R8 firmware

Open **this `firmware/` directory** in VS Code + PlatformIO, not the ZIP root.

```powershell
pio run
pio run --target upload
pio device monitor --baud 115200
```

Target: ESP32-S3 **WROOM-1 N16R8**, 16 MB Quad Flash and 8 MB OPI PSRAM
(`qio_opi`). ST7789 320×240: SCK GPIO12, MOSI GPIO11, CS GPIO10,
DC GPIO9, RESET GPIO8. Normal animation uses LovyanGFX DMA strips.

## Living Motion Engine (v1.7)

- `src/core/motion_director.h`: deterministic state machine for idle breathing,
  blink cues, music, Focus, warnings and celebrations. Uses real fresh PC data
  only (never demo or missing GPU data).
- Three NVS-persistent movement styles: Calm=0, Natural=1, Playful=2.
  Authenticated `POST /api/control` with `{"action":"motion_style","value":1}`.
- Read-only optional `/api/v1/status` fields report active state, cel and
  adaptive-effects guard; old Windows agents and HTTP/JSON v1 clients work.
- FPS Guard reduces optional bob/crossfade/sparkles after two overloaded
  5-second windows and recovers after four healthy windows; it does **not**
  claim an increased FPS target or interfere with the DMA pacer.
- The 16/48-frame `.meocat` packs, local Web controls, USB/OTA recovery and
  unchanged 6 MiB A/B firmware partitions are retained.
- The bundled v2 motion demo moves the cat slightly; a visible blink requires
  custom closed-eye art in an author-provided `*_2.png` frame.

See `../docs/LIVING_MOTION_V17.md` for behavior priority and physical QA.

## OTA Guardian

- `meodesk_ota_16mb.csv` reserves `app0`/`app1`, 6 MiB each. The new layout
  **requires one USB migration** if upgrading from single-app `huge_app.csv`.
  Back up private NVS and a known-good source/binary beforehand.
- Once dual slots exist, updates use `/api/v1/ota/upload` via Web Studio or
  the Windows Companion Firmware tab. The updater streams image bytes into
  the inactive slot, validates ESP32-S3 header and SHA-256, then reboots.
- Web Studio/Companion wait for the **new slot** and `boot_health:passed`,
  after a 25-second startup health check, before reporting success.
- TFT pauses ordinary cat animation and shows flash upload progress. Normal
  DMA strips, pages and PC/media/Focus APIs are unchanged.

## Motion Pack OTA (v1.6; backward-compatible with v1.5)

- `src/cat_pack.{h,cpp}` operates on two independently checksummed sprite
  banks in the previously reserved `assets` partition. It does not mount
  SPIFFS. A bank is selected only after the full hash is confirmed.
- On reboot, committed RGB565 pixels are loaded into PSRAM once: 648 KiB for
  the old 16-frame format or 1,944 KiB for new 48-frame Motion Packs, then
  fed through the same small internal DMA strips at normal FPS.
- Use `python tools/make_catpack.py --images images --out custom.meocat`
  to build a new pack from 16 144×144 PNGs. Requires Pillow. See
  `../docs/CATPACK_FORMAT.md` for file names, checksums and recovery.
- Use `python tools/make_catpack.py --motion-demo --out animated.meocat`
  to derive a tiny motion test from the original 16 images. For your own
  3-frame animation, prepare `boot_0.png`, `boot_1.png`, `boot_2.png`, etc.
  and pass `--motion --images animated_pngs`.
- Actual DMA FPS is unchanged as a *target*, not a guarantee. Motion frame
  selection is time-based and does not create extra Flash I/O per TFT frame.
- Update from the Cat Pack Studio Web panel or the seventh Windows Companion
  tab. Existing 6 MiB firmware OTA A/B partitions remain unchanged.

**Important:** Automatic rollback depends on your bootloader configuration.
It has not been tested on your module. An app reporting health PASS does not
guarantee future stability. This local HTTP updater accepts unsigned firmware,
so never expose it to the Internet or publish its pairing key.

## Project structure

- `src/main.cpp`: ST7789 renderer, OTA screen and runtime startup self-test.
- `src/core/motion_director.h`: allocation-free animation cue policy.
- `src/ota_manager.{h,cpp}`: inactive-slot streaming, image checks, SHA-256.
- `src/pc_bridge.cpp`: Wi-Fi, NVS, REST endpoints, OTA streaming callbacks.
- `src/web_ui.h`: self-contained browser UI with post-reboot verification.
- `src/core/`: testable pure-C++ app state and FPS logic.
- `src/app_config.h`: private Wi-Fi/AP/pairing defaults; do not publish.
- `tools/`: host OTA tests, mock web server, SHA cross-check and manifest.

See `../docs/OTA_MIGRATION.md`, `../docs/VERIFICATION.md` and the main README.

The ZIP includes a PlatformIO-compiled v1.7 OTA image, the original v1 `.meocat`
and a new v2 motion demo under `../release/`.
Compilation succeeded for ESP32-S3/16 MB; **real boot, SPI display, PSRAM,
FPS and recovery are not yet validated on your module**.
