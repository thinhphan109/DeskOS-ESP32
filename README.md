# MèoDesk v1.7.0 — Living Motion Engine

**Development follow-up: Personal Studio 1.8.0-dev** adds compact Web/Windows
controls, WebSocket shared state and a Midnight TFT clock scheduling fix.
See [docs/PERSONAL_STUDIO_V18.md](docs/PERSONAL_STUDIO_V18.md) for the exact
scope and validation. The v1.7 release information below is retained.

The next source slice adds [Performance Lab](docs/PERFORMANCE_LAB_V18.md)
instrumentation and a small renderer/widget contract; see the reconciled
[roadmap](docs/ROADMAP.md). Both are build/preview work; no OTA was performed.

**ESP32-S3 + ST7789 TFT 2.4″ / LovyanGFX + DMA / PlatformIO / Windows Companion / local Web Studio**

This update builds on the v1.6 Motion Pack Studio baseline without changing
the proven ESP32-S3/ST7789 wiring or breaking its HTTP/JSON v1 protocol.
Both the PlatformIO firmware and Windows agent are included. The 16 cat sprites,
40 MHz SPI, two DMA strips, optional offscreen card, fallback Wi-Fi AP and
300-point **PC telemetry** history remains intact.

**Release state: PlatformIO ESP32-S3 build SUCCESS** with `espressif32@8.0.0`,
Arduino 3.3.12, LovyanGFX 1.2.33 and ArduinoJson 7.4.3. The release includes
a **compiled ESP32-S3 firmware.bin**, SHA-256 manifest, **53 passing Python
tests**, C++ OTA/CatPack/domain tests and JavaScript tests.
**Hardware flashing, TFT/FPS tests and Windows-native EXE validation are still
pending.** The previous v0.9 firmware remains in `baseline/` as historical
reference; it was built for a different board-memory assumption.
**The previous v1.2 `huge_app` partition cannot
accept an OTA image. Read `docs/OTA_MIGRATION.md` before attempting first USB
upload of v1.7. This changes the partition table.**

The compiled OTA image is `release/meodesk_v17_esp32s3_n16r8_firmware.bin`.
Its image header reports chip **ESP32-S3**, **16 MB Flash**, DIO/80 MHz
(the Arduino bootloader's conservative image mode); verified length **1.83 MiB**,
well below the 6 MiB slot. The file embeds the owner's local configuration;
do not redistribute it.

## New in v1.7: Living Motion Engine

- A **pure-C++ Motion Director** drives expressive motion cues from pet state,
  real Windows CPU/GPU utilization, real Now Playing state and Focus progress.
  DEMO/offline telemetry **never** triggers load-based movements.
- Three persistent styles: **Calm**, **Natural** (default) and **Playful**.
  Change from Web Studio's Living Motion controls or Windows Companion's
  Cat Packs tab. Settings survive ESP32 reboot in existing NVS.
- Cozy breathing, irregular blink *cues*, rhythmic music movements, focus
  stillness, celebration, waiting and high-load warnings. Visible eyelid
  blinks require artist-made closed-eye `*_2.png` frames; the included motion
  demo is intentionally limited to small translations.
- **FPS Guard** monitors actual five-second render windows, reducing optional
  effects under sustained load and restoring them once healthy. It never
  changes the frame scheduler's saved target or guarantees measured FPS.
- No new framebuffer, per-frame allocations, Flash reads or network polling
  inside the TFT loop. Old v1 16-frame packs and v2 48-frame packs are both
  accepted, with the existing safe dual-bank asset loader and OTA A/B layout.
- Optional motion status diagnostics and a backward-compatible Windows
  Companion poll; 53 Python tests and native C++ motion/asset/OTA tests pass.

See [`docs/LIVING_MOTION_V17.md`](docs/LIVING_MOTION_V17.md) for state
priorities, exact controls, FPS budget policy and on-device test protocol.
**No physical hardware FPS numbers have been measured for this build.**

## New in v1.6: Motion Packs without changing the DMA renderer

- Supports **two interchangeable `.meocat` formats**: v1's 16 static poses
  (648 KiB) and v2's **16 poses × 3 animation frames** (1,944 KiB).
  Old v1.5 packs remain valid and do not need converting.
- **Real 3-cel sprite playback** (0→1→2→1, 170 ms per cel) driven by elapsed
  device time. The normal TFT DMA refresh remains independently scheduled
  at 20/25/30 FPS; switching cels does not start a crossfade, reallocate
  buffers or perform any Flash read per frame. Existing "cat motion off" and
  quiet behavior show frame 0.
- Both formats use the **same dual raw Flash banks** and commit-last SHA-256
  scheme; a corrupted new v2 pack falls back to the valid old v1 pack.
  Version, size and frame count are recorded in each bank header.
- Updated **Web Motion Pack Studio** and **Windows Companion** recognize both
  formats and reject too-large packs if device firmware/PSRAM is incompatible.
- Included `release/meodesk_v16_motion_demo.meocat` offers 48 derived frames
  using a subtle 1–2 pixel movement of the existing 16 art assets. This
  demonstrates the update/playback path; for genuine expressive animation,
  draw three unique PNGs per pose (`boot_0.png` … `shutdown_2.png`).
- Generate demo: `py tools/make_catpack.py --motion-demo --out motion.meocat`
  from `firmware/`; custom art: add `--motion --images animated_pngs`.

**Hardware FPS is unmeasured.** v2 reserves 1,990,656 bytes of PSRAM at
startup; if insufficient, the firmware tries the previous valid v1 pack, then
built-in art. Keep USB recovery. See `docs/MOTION_PACK_V2.md`.

## What's new in v1.5: Cat Pack OTA

- Upload **all 16 cat sprites** as a `.meocat` collection, exactly **648 KiB**
  of RGB565 little-endian pixels (16 × 144 × 144 × 2), without recompiling
  the firmware or changing any GPIO/DMA buffer.
- Uses the existing reserved `assets` partition: two independent **1.94 MiB
  banks**. The inactive bank is erased and streamed, SHA-256 is verified, and
  a commit header is written **last**. If power fails mid-update, the old
  bank remains bootable. If the newest bank later fails checksum verification,
  the previous valid bank is tried; built-in sprites are the ultimate fallback.
- Custom pixels are **loaded once into 648 KiB of 8 MB PSRAM at startup**.
  Runtime animation uses existing internal DMA strips and RGB565 mixing;
  no per-frame Flash I/O or full-screen buffer was introduced.
- **Web Studio Cat Pack panel** and **Windows Companion Cat Packs tab**:
  choose a `.meocat`, upload with progress, restart once, auto-confirm the new
  SHA-256 is active in PSRAM. Firmware OTA Guardian remains available.
- `firmware/tools/make_catpack.py` converts 16 named PNGs into `.meocat` and
  a manifest. Included `release/meodesk_v15_default_cats.meocat` is a safe
  baseline pack made from the existing cat images for the first OTA test.
- Compatible with v1.3/v1.4's A/B partition layout: **no new USB partition
  migration** when those versions are already installed.

See [docs/CATPACK_FORMAT.md](docs/CATPACK_FORMAT.md) for the exact file
format, bank safety, 16 image names and instructions. Real device FPS is
not yet measured; PSRAM rendering throughput must still be verified.

## What's new in v1.4: OTA Guardian

- **Boot confirmation, not upload-only success.** Windows Companion and Web
  Studio wait up to 85 seconds for the device to reboot into the exact inactive
  slot selected before upload, and for the firmware's startup health check to
  report `passed`. They distinguish uploading, rebooting, testing, success,
  timeout and rollback. A lost connection during reboot is expected.
- **Live progress on the physical TFT.** Firmware writes draw a small progress
  bar and percent on a dedicated static update screen. Normal 40 MHz SPI/DMA
  animation and partial redraw paths remain unchanged outside OTA.
- **New read-only diagnostic fields** in `GET /api/v1/ota/status`:
  `uptime_ms` and `boot_health` (`checking`, `passed`, `failed`). The health
  test runs after 25 seconds and checks flash capacity, PSRAM readiness,
  internal RAM and DMA buffers. This is an application self-check, **not**
  evidence that the precompiled bootloader supports automatic rollback.
- **Safer client preflight** ensures the inactive partition is `app0` or
  `app1` and differs from the running partition. No progress is marked
  successfully installed until the new slot is observed running.
- Compatible with v1.3's partition table, OTA routes, GPIOs and Windows PC
  telemetry protocol. **No second partition migration** from v1.3 is needed.

**Limits:** A successful startup check is not long-term stability proof.
Firmware remains unsigned over HTTP on a trusted local LAN. USB recovery,
NVS backup and real-device testing are still essential.

## Introduced in v1.3: local A/B OTA

- New **16 MB Flash partition layout**, two **6 MiB** OTA app slots, reserved
  3.875 MiB asset partition and 64 KiB crash log. The old NVS and otadata
  offsets are intentionally preserved; back up private NVS before migration.
- Firmware streams multipart `.bin` into the **inactive slot** and requires
  exact byte count, expected board `meodesk-esp32s3-n16r8`, SHA-256 and the
  existing pairing key before it activates the candidate boot partition.
- New read-only `GET /api/v1/ota/status` and authenticated multipart
  `POST /api/v1/ota/upload`. All existing PC/music/Focus/Web API routes remain.
- **Web Studio Firmware Studio**: upload, device/slot/rollback status, progress.
- **Windows Companion Firmware tab**: manual IP or existing LAN discovery,
  choose compiled `.bin`, explicit confirmation, streaming progress.
- Normal cat animation pauses during Flash upload; the device shows a static
  OTA screen. First-boot confirmation runs after an internal health check
  **only when a rollback-enabled bootloader is actually installed**.
- The built-in Arduino bootloader may not support automatic rollback. The
  device reports `bootloader_rollback_configured` and **does not claim a
  safety property that hasn't been tested.**
- OTA transport is **HTTP on trusted LAN, with no firmware signatures**.
  It must not be exposed to the public Internet or used for unattended updates.

## Retained from v1.2: FPS and diagnostics

### Adaptive FPS and five-minute diagnostics

- **AUTO FPS profile (20 ↔ 25 FPS):** monitors real five-second animation
  windows. After three consistently overloaded windows it steps down to 20;
  after six healthy windows it can recover to 25. No automatic 30 FPS.
  Existing manually saved 20/25/30 profiles continue to behave as before.
- **History without framebuffer overhead:** 60 fixed-size samples in SRAM,
  recording actual FPS, target, missed deadlines, worst render time, internal
  heap and free PSRAM over **five minutes** (once every five seconds).
- **Web Studio Device Link chart:** browser-rendered actual versus target FPS,
  retrieved from `GET /api/v1/performance` every five seconds. The ESP32 only
  returns compact JSON and does not spend CPU drawing the graph.
- **Kept stable:** N16R8 memory settings, existing GPIO wiring, SPI 40 MHz,
  DMA buffer path, NVS namespace, Wi-Fi credentials and all previous agent APIs.
  `fps_profile=3` identifies AUTO; `fps_target` is its current effective target.

## Retained from 1.1: N16R8 + FPS

- Dedicated `esp32-s3-devkitc-1` PlatformIO configuration for **ESP32-S3
  WROOM-1 N16R8**, 16 MB Quad Flash + 8 MB OPI PSRAM. This is NOT the
  WROOM-2 N16R8V flash configuration.
- Preserve the proven **internal DMA ping-pong strips** for the cat; do not
  relocate their SPI transfer buffers to PSRAM. Keep the fast internal card
  sprite, with PSRAM backup if internal allocation fails.
- Allocate optional 16 KiB album artwork buffers in **PSRAM**, with a safe
  fallback to internal RAM. No permanent flash writes for album images.
- Pure-C++ `FramePacer` (manual 20/25/30 FPS scheduling profiles, default 25) with
  late-frame skips to keep animation time-based. Web Studio and Serial show
  **measured FPS, target, worst frame render time and missed deadlines**.
  Web Studio can save a selected profile; Serial `k` cycles it.
- See `docs/PERFORMANCE_N16R8.md` for memory layout and real-device FPS
  acceptance goals. **FPS targets are not measured guarantees.**

## Retained from v1.0

- **Cat Behavior Engine:** independent pet persona with priority warnings,
  short dwell time between ordinary moods, a virtual energy value and a
  celebration after a 25-minute Focus session. Pet the cat from Web Studio.
- **Scene Manager:** four favorites configurable in Web Studio, saved in ESP32
  NVS; AUTO rotates those favorites. All 14 original page IDs remain stable.
- **Real album artwork:** Windows SMTC thumbnail → optional Pillow 64×64 RGB565
  → authenticated multipart upload to ESP32 → TFT card/Web Studio. Artwork is
  transferred only on song changes and stored in volatile RAM, **never NVS**.
  No thumbnail? The player displays normal title/artist text without fake art.
- **Windows Companion:** actual album thumbnail when available, preserving
  existing CPU/GPU/RAM telemetry, Focus and device discovery.
- **Web UI:** Living Companion visuals, album cover, pet interaction, favorite
  scenes and pet state. No cloud account or additional ESP32 hardware.

The next hardware milestone is verifying adaptive target switching and sustained
FPS on your own board; see `docs/HARDWARE_QA.md`.

## Retained from v0.9

- **Living cat:** priority-based, expiring event journal for PC warnings,
  music, mood changes, and completed Focus sessions; RGB565 cat sprite
  crossfade performed inside the existing DMA strips (no full framebuffer).
- **25/5 Focus:** countdown owned by ESP32, work/break phases and completed
  sessions. Control on TFT (Serial `f`), Web Studio or Companion; Windows app
  can close while the ESP32 continues counting. Reboot resets the session.
- **Cozy Day / Midnight / Auto:** TFT color palettes switch without changing
  existing 16-bit cat artwork; theme choice survives reboot in ESP32 NVS.
  Auto follows device-local NTP time (when available).
- **Real network and VRAM telemetry:** Windows psutil aggregate network rates
  (KB/s) and NVIDIA NVML used/total VRAM (GB); unsupported values are `null`,
  **never** invented except in explicitly labeled DEMO mode.
- **Windows Focus page:** start/pause/reset/skip with device-synchronized timer.
- **New Network TFT scene**, browser Focus studio, theme controls, and richer
  event timeline. Total **14** scenes; legacy indexes are unchanged.

**This ZIP contains owner-specific Wi-Fi defaults and a pairing key. Keep it
private; never publish or upload to a public Git repository.**

## Quick start (Windows)

1. **Before first dual-slot installation:** follow `docs/OTA_MIGRATION.md` to back up the current
   private NVS and plan the USB partition migration. This is NOT an ordinary
   firmware-only update. Then open VS Code → **File → Open Folder → `firmware/`**.
   PlatformIO: **Build first**
   and inspect errors before Upload. This source specifically targets
   **ESP32-S3 WROOM-1-N16R8**; the USB port and TFT pins are unchanged.
2. After a successful **USB Build and Upload**, open Serial Monitor at **115200**.
   Check `[HARDWARE] Flash` (16 MB), `PSRAM` (~8192 KB), `DMA on` and `[WEB] Router IP`.
   If COM port stays blank, use the *other USB connector*, press **RESET**, and
   enable `-DARDUINO_USB_CDC_ON_BOOT=1` in `firmware/platformio.ini` when your
   particular ESP32-S3 board uses native USB CDC.
3. In `windows_agent/`, double-click **`start_here.pyw`** (Python installed),
   click **① Cài thư viện còn thiếu**, then **② Mở MèoDesk**. One-time setup only.
   Or use a prebuilt `MeoDeskCompanion.exe` from the Windows CI artifact.
4. Companion → **Thiết bị** → **Quét IP trong LAN** or input your current device
   IP (e.g. `192.168.0.107` only if still correct). Enter the firmware pairing
   key, **Lưu và kết nối**. The key is stored via Windows Credential Manager.
5. Open **http://<device-IP>/** → Web Studio. The TFT has **Home, PC, Music,
   Focus** and the new Network scene. The earlier scenes remain available.
6. Confirm `GET http://<device-IP>/api/v1/ota/status` reports `ready:true`
   and different active/inactive slots. From v1.4 onward, the Firmware tab
   and Web Studio also wait for a positive post-boot check before reporting
   success when updating *subsequent* compiled `.bin` files. Select the included
   `release/meodesk_v17_esp32s3_n16r8_firmware.bin` for an OTA upgrade **only
   if v1.3 with the A/B layout is already running**.
7. After v1.7 starts, choose **Living Motion** in Web Studio or **Cat Packs →
   Living Motion** in Windows Companion and test Calm / Natural / Playful.
   Confirm the selected style survives reboot. Existing v1/v2 cat packs do
   **not** require another upload. For a first test, the included
   `release/meodesk_v16_motion_demo.meocat` should report `source:custom`,
   `frames_per_pose:3` after Cat Pack upload and reboot.

### Power-on behavior

- Wi-Fi settings are read from ESP32 NVS when present. Otherwise the firmware
  uses its local `src/app_config.h` defaults. This private archive includes the
  owner's preconfigured Wi-Fi credentials: **do not publish this folder/ZIP**.
- AP fallback is always on (`MEO-DESK-05`, `http://192.168.4.1/`). Configure
  router SSID/password in Web Studio → Device Settings, with the pairing key.
- Python agent does **not** need the Wi-Fi password; it only sends to the ESP32.
- Music comes from Windows Media Control. If nothing is playing, TFT/Web show
  *Nothing playing*, never fabricated playback data.
- No Windows media support or no NVML GPU → those features show unavailable;
  CPU/RAM streaming continues.

## Hardware wiring — unchanged from the proven color test

| TFT ST7789 pin | ESP32-S3 |
|---|---|
| VCC / GND | 3V3 / GND |
| LED / BL | 3V3 (if TFT backlight circuit supports it) |
| SCK | GPIO12 |
| MOSI / SDI | GPIO11 |
| CS | GPIO10 |
| DC | GPIO9 |
| RESET | GPIO8 |

SPI 40 MHz, 320 × 240 rotation 1, 16-bit RGB565. If long jumper wires cause
artifacts, lower `freq_write` to 20 MHz in `firmware/src/display_config.h`.

## Project layout

```text
meodesk_v17/
├── firmware/             PlatformIO + TFT/Web renderer + NVS Wi-Fi + REST APIs
│   ├── src/core/         C++ SceneManager, CatBehavior, EventEngine, FocusTimer
│   └── tools/            Asset packing, local Web preview, host C++ tests
├── windows_agent/        Original pc_agent/tray_agent + new desktop Companion
│   ├── companion.py      PySide6 GUI + system tray
│   ├── companion_core.py Telemetry transport, worker and command client
│   ├── media_provider.py WinRT native Now Playing (optional)
│   ├── discovery.py      Saved IP → mDNS → AP → opt-in bounded LAN scan
│   └── start_here.pyw    One-click dependency installer / launcher
├── protocol/             JSON v1 schema, test fixtures, compatibility matrix
├── packaging/windows/   PyInstaller onedir build pipeline
├── baseline/             Prior v0.9 firmware plus v0.8/v0.7.1 rollback
├── docs/                 Design decisions, hardware QA, release notes
└── .github/workflows/    Windows build & test CI
```

## Verification commands (when you choose to test)

Read-only 30-minute FPS/RAM/OTA evidence from the actual device (no pairing key):

```powershell
py windows_agent/device_diagnostics.py --host meodesk.local --duration 1800
```

Writes JSONL snapshots, CSV metrics and a summary to a new `diagnostics/` folder.
See [docs/DEVICE_DIAGNOSTICS.md](docs/DEVICE_DIAGNOSTICS.md) for outage handling,
older firmware, DEMO labels and measurement limits. Physical acceptance is
always a separate check.

```powershell
cd windows_agent
py -m unittest discover -s tests -v

cd ..\firmware
pio run
# pio run --target upload
```

Host C++ logic tests (where `g++` exists):

```sh
g++ -std=c++17 -Wall -Wextra -Werror firmware/tools/test_domain.cpp -o meodesk-domain-test
./meodesk-domain-test

# Allocation-free Living Motion policy tests:
g++ -std=c++17 -Wall -Wextra -Werror firmware/tools/test_motion_director.cpp -o meodesk-motion-test
./meodesk-motion-test

# OTA core logic against host ESP-IDF stubs (OpenSSL dev library required):
g++ -std=c++17 -Wall -Wextra -Werror -I firmware/tools/ota_stubs -I firmware/src firmware/src/ota_manager.cpp firmware/tools/test_ota_manager.cpp -lcrypto -o meodesk-ota-test
./meodesk-ota-test

# SHA-256 fallback used by HTTP Web Studio:
node firmware/tools/test_web_sha.js
node firmware/tools/test_web_boot.js
node firmware/tools/test_web_catpack.js

# Cat Pack host bank/rollback test (needs OpenSSL headers):
g++ -std=c++17 -Wall -Wextra -Werror -I firmware/tools/ota_stubs -I firmware/src firmware/src/cat_pack.cpp firmware/src/cat_assets.cpp firmware/tools/test_cat_pack.cpp -lcrypto -o meodesk-cat-test
./meodesk-cat-test
```

Local mock Web UI: `python firmware/tools/web_preview_server.py` then open
`http://127.0.0.1:8765/`. **All mock values are labeled DEMO**; the mock
cannot change real Wi-Fi settings.

### Main HTTP endpoints (firmware 1.7; previous routes preserved)

| Method | Path | Purpose | Pairing key |
|---|---|---|---|
| GET | `/api/status` | All live status and six latest events | No |
| GET | `/api/v1/performance` | Five-minute cat FPS/target/heap history, 5 s samples | No |
| GET | `/api/v1/ota/status` | A/B readiness, active slot, rollback capability | No |
| POST | `/api/v1/ota/upload` | Stream `.bin` with size/SHA-256/board headers | Yes |
| GET | `/api/v1/focus` | Focus work/break/countdown/sessions | No |
| POST | `/api/v1/focus` | `{"action":"toggle|reset|skip"}` | Yes |
| GET | `/api/v1/events` | Priority/TTL event timeline | Yes |
| POST | `/api/control` | `{"action":"theme","value":0|1|2}` | Yes |
| POST | `/api/control` | `{"action":"fps","value":0|1|2|3}` (3 = AUTO) | Yes |
| POST | `/api/control` | `{"action":"motion_style","value":0|1|2}` | Yes |
| GET | `/api/v1/status` | Motion state, cel, FPS Guard, real PC intensity (optional fields) | No |
| POST | `/api/telemetry` | Live optional `net_*_kbps` / `vram_*_gb` | Yes |
| POST | `/api/v1/artwork` | Multipart 64×64 RGB565 album art, track ID header | Yes |
| GET | `/api/v1/artwork.bmp` | Album art image for browser | Yes |
| POST | `/api/control` | `pet` or `scene_0`..`scene_3` with page ID | Yes |

Web UI has a pairing-key field. Enter the key from your own private
`firmware/src/app_config.h`, not your Wi-Fi password.

### Windows EXE build

On Windows PowerShell, from the repository root run
`powershell -ExecutionPolicy Bypass -File packaging/windows/build.ps1`.
It creates `MeoDeskCompanion-Windows-x64.zip` with a **PyInstaller onedir** app
that can run without a Python installation. The GitHub Actions workflow also
builds this artifact on `windows-latest`. Do not rename the directory's
internal files: the executable depends on them.

## Known limitations

- **First-time migration is USB-only.** Previous v1.2 builds use single-app
  `huge_app.csv`, while v1.3 switches to two 6 MiB OTA slots. Back up private
  NVS and keep your last working binary before flashing the new partition
  layout. Hardware partition/rollback behavior remains unverified here.
- If Windows media is unsupported by the player or WinRT bindings are missing,
  playback metadata and remote controls are unavailable (no fake data).
- Art is capped at 64×64 and uses ~16 KiB SRAM for active/staging buffers;
  a player without an SMTC thumbnail or optional Windows dependencies stays
  in text-only mode. Album cover availability has not yet been confirmed on
  the user's Windows media apps.
- A native Windows build and a real ESP32/TFT 30–60 minute endurance run must
  be verified **on those machines**, not inferred from Linux unit tests.
- HTTP pairing is for a trusted LAN. Never port-forward ESP32 port 80 or
  distribute a ZIP containing private Wi-Fi or pairing credentials.

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md),
[docs/PERFORMANCE_N16R8.md](docs/PERFORMANCE_N16R8.md),
[docs/EXTENSION_POINTS.md](docs/EXTENSION_POINTS.md),
[docs/HARDWARE_QA.md](docs/HARDWARE_QA.md), and
[protocol/compatibility.md](protocol/compatibility.md).
