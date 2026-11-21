# MèoDesk v1.7.0 — verification report

Date: 2026-11-20 (Vietnam time). Target: ESP32-S3 WROOM-1 N16R8.

## 1.9.0-dev profile and update follow-up

81 Python tests passed, including GUI and mock config/Focus flows; native
profile/configurable-Focus/atomic-scene-swap tests and browser state suites
passed. The local preview exercised a 50/10/6 plan. Full PlatformIO build
succeeded. Physical NVS restore/OTA/clock/FPS checks remain deferred.
The public v1.8 GitHub Windows CI also completed successfully and packaged
Companion. See `CONFIG_AND_OTA_V19.md` for this source milestone's scope.

## Initial Performance Lab source follow-up

The combined Studio + Lab build succeeded (57,780 bytes static RAM,
36.0% application Flash). 74 Python tests, four Node suites and native
Performance Lab/widget tests passed. The new dev image and manifest use the
`meodesk_v18dev_performance_lab_esp32s3_n16r8_firmware` prefix. Tracing is off
by default and reports explicit partial measurement coverage. No OTA was
performed; physical timing and benchmark comparisons remain pending. See
[PERFORMANCE_LAB_V18.md](PERFORMANCE_LAB_V18.md).

## Personal Studio 1.8.0-dev follow-up

See [PERSONAL_STUDIO_V18.md](PERSONAL_STUDIO_V18.md) for current results:
73 Python tests passed including GUI and two Qt WebSocket clients, C++
card/domain/motion tests passed, four Node tests passed, and full PlatformIO
build succeeded. A dev OTA image and SHA manifest are in `release/`.
Live read-only checks confirmed the owner's installed v1.7 A/B OTA readiness.
The owner chose **preview only**; no firmware update was performed.
Physical 1.8 TFT/latency/FPS acceptance remains pending. Sections below
describe earlier release and diagnostics-only checkpoints.

## Source follow-up: read-only acceptance recorder (2026-11-20)

Added `windows_agent/device_diagnostics.py` and
[usage/measurement notes](DEVICE_DIAGNOSTICS.md) to prepare actual-board QA.
This is a source tooling change; release firmware binaries/manifests were not
rebuilt or replaced.

Validation rerun on Windows:

- Python: **66 tests run, 65 passed, 1 skipped** (PySide6 is not installed,
  so the existing GUI smoke test was skipped). Thirteen new recorder tests
  cover a real loopback HTTP server, GET-only transport, refused redirects,
  privacy filtering, response limits, old firmware, transient failures,
  interruption, disk output and real-device/PC-DEMO versus mock labels.
- Native C++ domain and Motion Director tests: **PASS** with local `g++`.
- Three existing Node Web Studio SHA/OTA/Cat Pack tests: **PASS**.
- Full PlatformIO firmware build, C++ OTA/Cat Pack tests requiring OpenSSL,
  physical device logging/flashing and packaged Windows EXE: **not rerun**
  in this follow-up. The table below records the original release results.

No real-device FPS, PSRAM, OTA, rollback or endurance acceptance is claimed
by this follow-up. The recorder always reports `hardware_acceptance:not_assessed`.

**Important:** The owner reports the previous v1.1 firmware working on the
device. The archive **includes a compiled v1.7 firmware.bin, static pack and
motion pack demo**, but they have
**not been flashed on any physical ESP32**. Compilation and host tests are
not a substitute for a real N16R8 partition migration,
flash transfer, reboot and rollback validation. In particular, the installed
Arduino bootloader may have rollback disabled.

## Results from this development session

| Check | Result | Meaning |
|---|---|---|
| Portable C++ domain tests | PASS | Mood, Focus, Scene Manager, Cat Behavior, FramePacer, AUTO FrameGovernor and 60-sample PerformanceHistory ring |
| Native C++ Living Motion test | **PASS** | No heap, deterministic timing, modes/styles, real-versus-DEMO PC load isolation and five-second FPS Guard hysteresis |
| Actual `ota_manager.cpp` host C++ tests | **PASS** | Stubbed ESP-IDF flash with real SHA-256; verifies header chunking, wrong-chip rejection before erase, hash mismatch and inactive-slot commit |
| Python agent/mock/protocol tests | **53 tests PASS** | Windows transport, seven-tab GUI, optional motion polling, Web mock, 16 MB partition geometry, OTA and v1/v2 Cat Pack streaming, demo generator, renderer regression, v1.6 compatibility and reboot confirmation |
| Actual `cat_pack.cpp` native host tests | **PASS** | Simulated Flash writes/erases, legacy v1 + motion v2 headers, 3-frame pointer selection, two-bank commits, simulated power loss and corrupt-newest fallback to valid v1 bank |
| Browser SHA-256 fallback | **PASS** | Known input digests and 1 MiB binary against `crypto` reference |
| Web Studio OTA Guardian state machine | **PASS** | Embedded JavaScript parses under Node; simulated new slot, rollback, failed check and 85 s timeout |
| Web Studio Cat Pack state machine | **PASS** | Embedded JavaScript with mocked browser requests verifies reboot discovery, active-SHA confirmation and upload timeout |
| PlatformIO `pio run` for `espressif32@8.0.0` | **SUCCESS** | Full `.elf` linked, custom partition generated and `.bin` produced; Flash **30.2%** of 6 MiB, static RAM **17.2%** |
| ESP32-S3 image information | **PASS** | `esptool image-info`: chip ID 9 (S3), 16 MB Flash, DIO/80 MHz, valid checksum/hash; v1.7 image matches included release manifest |
| Windows native `.exe` build | NOT RUN | Requires Windows/PyInstaller |
| Physical ESP32-S3 + TFT boot and color | NOT RUN | Requires user's N16R8 module; custom PSRAM cat artwork has not been rendered on real TFT |
| Measured FPS / endurance | NOT RUN | Requires actual LCD, Wi-Fi workload and 30–60 min runtime |

The firmware was compiled and linked against Arduino ESP32 3.3.12, while
the C++ unit test was compiled separately with host `g++ -std=c++17 -Wall
-Wextra -Werror`. Python tests used `python3 -m unittest discover -s
windows_agent/tests -v`. The build confirms source/library linkage but cannot
validate electrical wiring, hardware DMA, PSRAM or OTA rollback.

## Hardware acceptance checklist for the owner

1. On Windows, open `firmware/` in VS Code with PlatformIO and run **Build**.
   Do not upload if the compiler reports an error. Save complete error text.
2. Check board shield marking: WROOM-1 **N16R8**, not WROOM-2 N16R8V;
   `board_build.arduino.memory_type=qio_opi` is intentional for WROOM-1.
3. Upload via the same working USB/COM port. Serial 115200 should report
   `Flash: 16 MB`, `PSRAM: ~8192 KB` and DMA `on`.
4. Verify RGB565 cat colors, color-test chart, album cover RGB channel order,
   and Web Studio controls without tearing or crashes.
5. In Web Studio, compare Stable20, Balanced25, Smooth30 and **Auto20–25**
   with the Windows agent streaming and at least one browser open. Watch the
   **five-minute measured/target FPS chart** and max render/skip statistics.
   Auto should step down only after 3 overloaded five-second windows and
   recover only after 6 healthy windows; do not assume it will oscillate under
   normal load. Inspect `GET /api/v1/performance` independently if the browser
   chart is blank.
6. Check low-memory behavior, interrupted artwork upload, Wi-Fi reconnect,
   Focus expiry, legacy Python agent and fallback AP `192.168.4.1`.
7. **v1.3+ includes a new 16 MB dual-partition scheme** that must be migrated
   using USB before OTA. It has not been tested on the device. Follow
   `OTA_MIGRATION.md` and back up NVS.
8. In v1.6, upload the provided v1 `.meocat` default pack. Confirm
   `/api/v1/catpack/status` switches `builtin` → `custom`, its SHA matches
   `release/meodesk_v15_default_cats.manifest.json` and all 16 scenes display
   correct colors with no FPS regression. Test a second custom pack only
   after verifying the baseline.
9. Upload `release/meodesk_v16_motion_demo.meocat`. Confirm
   `/api/v1/catpack/status` reports `frames_per_pose:3`, active SHA matches
   the v2 manifest and all 16 scenes show the subtle 0→1→2→1 motion.
   Compare 10-minute measured FPS, worst render time, free PSRAM and missed
   deadlines against the original built-in/v1 static sprites; motion can
   add PSRAM bandwidth pressure even when TFT DMA design stays unchanged.
10. In v1.7, change Calm → Natural → Playful on both Web and Windows.
    Check `/api/v1/status` updates `motion_style` and `motion_state`, and
    confirm NVS persistence after reboot. No new asset upload is necessary.
11. Compare music, Focus, warning and idle behavior with genuine Windows
    telemetry. Switch to DEMO/disconnect the Windows agent: CPU/GPU-driven
    movement must disappear, while basic ambient motion remains.
12. Observe `motion_budget_limited` during deliberately heavy workload;
    validate that optional effects reduce without changing the configured
    FPS profile. Record a ten-minute `/api/v1/performance` and PSRAM trace.
    The animation cues are not actual eyelid drawings in the demo pack.

## Required OTA acceptance tests (not run here)

1. Flash v1.4 OTA partition layout once using USB (unless v1.3 A/B is already
   installed). Confirm `ota_0`/`ota_1`,
   16 MB Flash, 8 MB PSRAM and unchanged Wi-Fi/Focus/media behavior.
2. Upload a correctly built N16R8 `.bin` using Web Studio. Check firmware hash,
   physical TFT progress, new boot and `/api/v1/ota/status` running-slot change.
   Verify `boot_health` changes from `checking` to `passed` around 25 s.
3. Repeat using Windows Companion's Firmware tab (with manual IP and discovered
   IP); verify foreground GUI stays responsive and no pairing key is logged.
   Confirm it reports installed only after reading the new slot and health PASS.
4. Wrong key, wrong board, wrong size, bad hash and interrupted Wi-Fi upload:
   current app must stay bootable; inactive candidate may be incomplete.
5. Record `bootloader_rollback_configured` and **verify on physical hardware**
   before relying on automatic recovery. Test a deliberately failing app only
   with a verified USB recovery path; do not intentionally brick hardware.
6. Keep the existing known-good USB binary and a private NVS backup for rescue.

## Safe rollback and privacy

- `baseline/meodesk_v09_esp32s3_firmware.bin`: last *historically* compiled
  baseline, built for the previous N8 assumptions and wiring. It is not a
  v1.7 build and should only be used if known compatible with your board.
- `baseline/meodesk_v08_esp32s3_firmware.bin`, `baseline/meodesk_esp32s3_v071_wifi_hotfix.zip`:
  older project artifacts.
- `firmware/src/app_config.h` contains the owner's private Wi-Fi defaults and
  pairing key. Do not publish this ZIP/source to a public repository.
- `release/SHA256SUMS.txt` lists v1.7 firmware, v1.6 motion demo, v1.5 static
  Cat Pack and historical
  known files. Hash checks do not provide publisher authentication.

## 21 November 2026 source pass

The v2.9.0-dev source passes 113 Python tests (including GUI, shared state, atomic full-profile restore and the HTTP SDK), 12 native C++ domain programs and eight browser JavaScript programs. The final local PlatformIO build succeeds at 58,820 bytes static RAM and 2,372,603 bytes program Flash (37.7% of a 6 MiB slot). The versioned local binary has a SHA-256 manifest; owner-configured binaries remain excluded from Git.

Read-only checks of the owner's device report firmware 2.4.0-dev, app0 running, app1 ready (6 MiB) and boot health passed. No later milestone was flashed in this development pass. See DEVELOPMENT_STATUS.md for the scope and physical acceptance work still pending.


## v2.10 Music and production planning checkpoint

Local Python regression: **115 tests passed**. Native domain programs: **13 passed**, including the new strict UTF-8/Vietnamese boundary checks. Browser JavaScript programs: **8 passed**. Both embedded fonts contain real bitmaps for every NFC Vietnamese vowel/tone combination.

The final PlatformIO build succeeds with **59,172 bytes static RAM** and **2,395,419 bytes program Flash** (38.1% of the 6 MiB slot). The optional Music metadata canvas reserves **79,040 bytes PSRAM**; direct drawing remains a fallback. Firmware BIN size: **2,454,800 bytes**. SHA-256: `3e07dd4c84a96281c817d9bba1a5f64bd67c2afc4abe876351ee3c1ceb590d1f`.

The TFT design image is a simulation using the embedded bitmaps, not a physical capture. Web's expanded workspace was inspected in a local mock. Native GUI regression passed; final visual acceptance of the latest spacing and the packaged app on a clean Windows installation remains pending. No firmware was deployed. The next desktop/naming/OTA work is a proposal in [the production plan](PRODUCTION_PLAN.md).


## v2.11 mimi.desk focused verification

Per the owner's faster-development preference, this UI slice runs the five GUI smoke cases, embedded-JavaScript syntax/OTA-boot checks and the shared-state JavaScript check, not the complete regression suite. The GUI cases and both Node programs pass. Web Overview/Studio/Device were inspected, including a 390px viewport with document width exactly 390px and a 316px Studio canvas. Native Overview/Music were inspected in DEMO; the last Device card-height adjustment was checked through GUI construction, without restarting the user's active preview.

Final PlatformIO build: **59,172 bytes static RAM**, **2,378,459 bytes program Flash** (37.8% of a 6 MiB slot). Private BIN size: **2437840 bytes**, SHA-256 `37cbd7289300adb20030798a92bdec415ccc20b55cd5c4de729ac49523671a19`. Only the manifest/checksum are committed. No OTA, server deployment or public firmware binary publication occurred.

This source checkpoint skips the broad push CI to avoid redundant checks during UI development. Packaging and full release acceptance remain for the release milestone.
