# 1.8.0-dev — Personal Studio / shared device state

- Initial optional Performance Lab adds strip CPU, DMA submit/wait, cat/card/
  bridge wall times and explicitly partial bulk-payload counting. No per-frame
  allocation. Recorder preserves timing validity and comparison context.
- Small renderer-independent widget bounds/cadence/surface contract is used
  by the card scheduler. This is an interface foundation, not a Widget SDK.
- Combined Lab build succeeded, 74 Python tests passed, and native Lab/widget
  tests passed. Physical benchmarking remains pending; owner chose preview only.

- Compact Web workspaces and Windows Studio controls for scenes, favorites,
  motion, theme and FPS; local v1/v2 Cat Pack preview in the browser.
- ESP32 WebSocket state stream shared by Windows/Web, ordered by boot epoch
  and revision, with existing HTTP writes/uploads and polling fallback.
- Bounded card deferral fixes the two-second starvation policy affecting
  Midnight TFT clocks. Actual post-update TFT behavior still needs hardware QA.
- Media heartbeat preserves paused tracks; a new device boot resets command
  and volatile artwork state. Local Web preview reloads HTML on refresh.
- 73 local Python tests passed including GUI and two Qt WebSocket clients;
  full ESP32-S3 build, C++ card/domain/motion and four Node tests passed.
  Physical 1.8 acceptance and native EXE packaging remain pending.

See [PERSONAL_STUDIO_V18.md](PERSONAL_STUDIO_V18.md).

---

# Unreleased — hardware acceptance tooling

- Added a read-only, stdlib-only device recorder for 30–60-minute FPS/RAM
  sessions, OTA/pack metadata and connection gaps. Writes JSONL, CSV and a
  summary; preserves completed samples on Ctrl+C and never overwrites a run.
- Allowlisted diagnostic fields omit SSID, machine/media identity and keys.
  Mock statistics stay separate; a real board with PC DEMO data still has
  device-reported FPS. Older firmware can omit optional endpoints/metrics.
- Thirteen new tests pass. Complete Windows Python run: 65 passed, 1 GUI
  test skipped because PySide6 is absent. Release firmware remains v1.7.0;
  physical hardware acceptance is still pending.

---

# v1.7.0 (Living Motion Engine) — real-state cat cues + FPS Guard

- Pure-C++ `MotionDirector` turns real, fresh Windows PC utilization, actual
  music playback, Focus timer, pet energy and critical warnings into discreet
  three-frame motion cues. Demo or disconnected CPU/GPU values are ignored.
- Three movement styles (Calm / Natural / Playful) are stored in existing NVS
  and controlled by paired Web Studio or Windows Companion without extra ports.
- Irregular idle blink cues, music rhythm, focus breathing, alert motion and
  celebrations. Real blinking requires user-authored closed-eye sprites;
  included v1.6 demo art only shifts existing images.
- FPS Guard uses measured 5-second render windows with hysteresis to disable
  optional effects when overloaded and restore them after recovery. Existing
  DMA strips, 20/25/30 FPS profiles and AUTO governor are unchanged.
- Optional read-only motion diagnostics and capability flag; legacy PC agents,
  firmware OTA routes and 16/48-frame Cat Pack binary formats stay compatible.
- PlatformIO full ESP32-S3 N16R8 build SUCCESS. Python 53 PASS, native C++
  Motion Director, Cat Pack, OTA/domain and browser tests PASS. Physical TFT,
  sustained measured FPS, OTA reboot/rollback and Windows EXE still need QA.

---

# v1.6.0 (Motion Pack Studio) — 48-frame cat animation with A/B compatibility

- New `.meocat` v2: 16 existing poses × 3 RGB565 144×144 frames each,
  1,990,656 bytes. The 2-bank `assets` Flash partition stays unchanged.
- Firmware reads both original 648 KiB v1 packs and 1,944 KiB v2 packs;
  validates version/geometry/hash, loads the active atlas into PSRAM and
  falls back to an older verified bank or compiled sprites as needed.
- Time-based 0→1→2→1 cel sequence at 170 ms; no animation frame heap
  allocation, Flash read or crossfade restart. The independent LovyanGFX
  20/25/30 Hz DMA frame pacing and AUTO governor remain unchanged.
- Web Studio Motion Pack panel and Windows Companion Cat Packs tab accept
  both sizes with board-side capability/PSRAM checks.
- `make_catpack.py --motion` converts hand-drawn 48 PNG sequences;
  `--motion-demo` derives a minimal 0/1/2px motion test from the old artwork.
- Included `.meocat` v2 demo and compiled N16R8 `.bin` plus SHA manifests.
  Python suite 52 PASS, native host power-loss/format test PASS, browser tests
  PASS, PlatformIO release build SUCCESS. **Physical TFT FPS unmeasured**.

---

# v1.5.0 (Cat Pack Studio) — PSRAM sprite packs with dual-bank Flash

- New fixed v1 `.meocat` pack: sixteen RGB565 little-endian 144×144 frames,
  663,552 bytes. `tools/make_catpack.py` generates from transparent PNGs,
  plus a SHA-256 inventory manifest.
- Two raw banks inside the **previously reserved 3.875 MiB assets** partition.
  Inactive bank only is erased/written, SHA-256 checked, then bank commit
  header written **last**. Boot verifies payload; older valid bank and built-in
  Flash sprites are fallback paths. No repartition or SPIFFS mount.
- 648 KiB sprites loaded once into PSRAM, then rendered from memory into the
  existing internal SRAM DMA strips. Animation behavior and FPS profiles remain.
- New Web Studio Cat Pack panel and seventh Windows Companion tab. Uses
  authenticated local multipart upload and waits for post-restart SHA match.
- Mock server and test protocol updated. 48 Python tests pass; host Cat Pack
  test simulates power-loss and corrupt newest bank; JS and OTA tests pass.
- PlatformIO ESP32-S3 firmware **build SUCCESS**, image included in release.
  Hardware FPS/OTA endurance and Windows EXE tests remain pending.
- Asset transport is unsigned local HTTP on a trusted LAN. Do not expose it
  to the Internet or distribute owner-specific Wi-Fi credentials.

---

# v1.4.0 (OTA Guardian source) — reboot verification and TFT progress

- Windows Companion and Web Studio now automatically wait up to 85 s for
  the uploaded candidate to boot into the intended `app0`/`app1` partition.
- The running firmware exposes `boot_health` and `uptime_ms`. The self-test
  transitions from `checking` to `passed` or `failed` after 25 s. Clients only
  report success after `passed` on the selected destination slot.
- Windows client detects slot rollback, rejected health checks, timeout and
  temporary network disconnection during a normal reboot.
- Physical TFT shows percent and a bounded progress strip *during* OTA;
  normal scene/animation DMA scheduling is unchanged.
- Added five Windows tests for preflight and boot-state transitions. Current
  Python suite: 42 PASS. C++ OTA, browser hash and JavaScript OTA reboot
  state-machine tests PASS.
- API backward-compatible: same partition layout, image format, pairing key,
  endpoints and existing PC/music/Focus clients. Rollback is not guaranteed
  without a bootloader built with that capability.
- **No physical ESP32 flash, measured FPS or native Windows EXE test.**
- PlatformIO **full ESP32-S3 build SUCCESS** with `espressif32@8.0.0`,
  Arduino 3.3.12; compiled 1.78 MiB candidate plus unsigned SHA manifest
  included under `release/`. Board-specific boot still requires owner QA.

---

# v1.3.0 (Local OTA A/B source) — 16 MB Flash + Web/Windows Updater

- Add custom partition `meodesk_ota_16mb.csv` with 6 MiB OTA app0/app1,
  3.875 MiB reserved assets and 64 KiB coredump; preserve existing NVS
  (`0x9000/0x5000`) and otadata (`0xE000/0x2000`) positions.
- This is a **one-time USB partition-table migration** from v1.2 `huge_app`.
  Back up private NVS and keep a known-good compiled binary first. No
  partition-table rewrite is attempted over Wi-Fi.
- Add a bounded `MeoOtaManager` using ESP-IDF OTA API. Authenticated local
  multipart upload streams to the inactive partition, validates board/length,
  SHA-256 and image format; schedules restart only after success.
- Add read-only `/api/v1/ota/status` and paired `/api/v1/ota/upload`.
- Add Web Studio Firmware Studio, Windows Companion Firmware tab and a
  stdlib-only OTA uploader with offline protocol/manifest tests.
- Stop cat animation during flash writes; first-boot health confirmation is
  conditional on the bootloader actually having rollback configured.
- **Developer-only LAN HTTP OTA**: signed image verification, HTTPS and
  tested automatic rollback are deferred. Do not expose this endpoint to
  untrusted networks.
- Host tests pass; physical N16R8 Build/Upload/OTA and Windows native EXE
  checks remain pending. See `OTA_MIGRATION.md` and `VERIFICATION.md`.

---

# v1.2.0 (Adaptive Performance source) — Auto FPS + diagnostics

- Add `FrameGovernor` conservative hysteresis: after 3 overloaded five-second
  windows change 25 → 20 FPS; after 6 healthy windows change 20 → 25. Manual
  20/25/30 FPS profiles remain available and unchanged; `k` cycles 4 modes.
- Add `PerformanceHistory`, a fixed 60-sample RAM ring for 5-minute histories
  of actual frame rate, target, missed deadlines, worst render time and free
  internal/PSRAM. History resets on boot, no persistent flash writes.
- Add unauthenticated read-only `GET /api/v1/performance` for diagnostics and
  a compact browser-rendered FPS history chart in Device Link. 5-second polling
  does not redraw the TFT or issue artwork transfers.
- Store AUTO profile as `fps_profile=3` in the existing NVS key, preserve
  legacy HTTP routes, GPIO, SPI speed, DMA strips, Windows agent and project
  partition layout.
- Host C++ domain tests and 32 Python tests pass. Actual v1.2 PlatformIO
  cross-build, N16R8 flashing and measured TFT FPS require user validation.

---

# v1.1.0 (N16R8 Performance source) — Frame pacing + PSRAM

- Correct ESP32-S3 **WROOM-1 N16R8** board memory map: 16 MB Quad Flash,
  8 MB OPI PSRAM (`qio_opi` + `BOARD_HAS_PSRAM`), preserving the existing
  `huge_app` partition and Wi-Fi NVS for a safe USB-upgrade path.
- Keep the two 144×16 DMA strips in internal SRAM; prefer fast internal SRAM
  for the card sprite, with an optional PSRAM fallback. Move the bounded
  16 KiB transient album artwork buffers to PSRAM when available.
- Portable `FramePacer` maintains 20/25/30 FPS *targets* on a monotonic clock,
  skips late frames, reports missed deadlines and maximum render time, and
  gives cat animation priority over noncritical card repainting.
- Web Studio adds a saved performance profile, actual/target FPS, PSRAM free,
  maximum cat render duration and skipped frames. Serial `k` cycles profiles.
- Hardware FPS and image quality still **require actual-board testing**; see
  `PERFORMANCE_N16R8.md`. Never label targets as measured FPS.

---

# v1.0.0 (previous source preview; verification deferred) — Living Companion

- New pure-C++ **SceneManager**, four favorite page IDs with NVS persistence,
  dynamic TFT footer labels and AUTO rotation restricted to the favorites.
- New pure-C++ **CatBehavior**, configurable presentation state with urgent
  sensor warning priority, mood stability dwell, virtual energy, focus-complete
  celebration and authenticated Web petting interaction.
- Optional **64×64 RGB565 album artwork** from the Windows SMTC real thumbnail,
  transformed by Pillow; streamed as bounded multipart to ESP32 volatile RAM,
  drawn in the TFT Now Playing card and returned to paired Web Studio as BMP.
- Updated Windows Companion with a cover tile, optional Pillow/WinRT streams
  dependencies, song-aware artwork upload and graceful legacy fallback.
- Web Studio receives themed cover display and four-scene layout selector.
- New protocol capabilities, optional `artwork_id`, source regression tests,
  offline mock updates. Legacy page IDs, GPIOs, NVS Wi-Fi and HTTP v1 retained.
- **Per user instruction: no compilation, automated tests, real hardware test,
  Windows EXE build or USB flash performed for this v1.0 source.** Previous
  v0.9 compiled firmware is retained under `baseline/` for USB rollback.

---

# v0.9.0 — Living Desk (previous release)

- **EventEngine:** lightweight ring buffer, priority (0–3), expiry TTL,
  deduplication, active event selection and authenticated event timeline API.
- **FocusTimer:** device-owned 25-minute work/5-minute break clock; start,
  pause, reset, skip and completed-session count; preserves elapsed time across
  Windows Companion restarts (not ESP32 power cycles). New Web/Qt control panel.
- **Living Cat:** RGB565 crossfades between mood sprites directly in existing
  LovyanGFX DMA strips, without requiring PSRAM or a full framebuffer.
- **Display themes:** Cozy Day, Midnight and automatic NTP-driven switch,
  persisted in ESP32 NVS; manual theme works without NTP.
- **Windows telemetry:** actual network throughput (psutil aggregate), NVIDIA
  NVML VRAM used/total; missing values remain `null` and older agents remain
  compatible. Mock data only in marked DEMO mode.
- **UI:** 14 TFT scenes incl. PC Network, Web Living Desk Focus/ambient
  dashboard and recent prioritized events, fifth Windows Companion Focus tab.
- **Protocol:** additive optional v1 fields; `/api/v1/focus` and
  `/api/v1/events`; old endpoints unchanged. Added deterministic C++/Python
  regression tests and a local web mock.
- **Unchanged:** N8 with no PSRAM, GPIO8–12 ST7789 wiring, existing Wi-Fi/NVS
  and fallback AP, 1Hz metrics, DMA strips, offline operation, USB flashing.
- **Not implemented:** OTA, cloud remote access, auto-updater, full Windows
  native EXE build on Linux; physical hardware soak tests remain for the owner.

---

# v0.8.0 — Smart Desk Hybrid (previous release)

- Four featured TFT scenes: Home/PC/Music/Focus, 9 legacy scenes retained.
- MoodEngine v2: battery/hot/load warnings before focus/media, 30-second sleep
  entry, hysteresis and existing debounce; independent Wi-Fi state.
- Real Windows SMTC Now Playing provider; unavailable music is not mocked.
- Authenticated media state API and idempotent, 8-second-TTL command/ACK path.
- Windows PySide6 Companion UI, tray icon, manual IP/mDNS/optional LAN scan,
  keyring-backed pairing secret, single-click source bootstrap, build workflow.
- ESP32 Wi-Fi config via authenticated Web UI and NVS. Fallback AP preserved.
- Local Web Studio with music controls, ACK indicator, Wi-Fi settings and 4
  scene shortcuts. Local mock server updated to v0.8 fixture mode.
- Protocol v1 schema/fixtures, host C++ tests and Windows Python unit tests.
- Firmware still uses `huge_app` USB flashing; OTA not included until hardware
  flash size and partition strategy are measured.

Release verification status should be recorded *after* running the commands
and the real-device checklist; source-only builds cannot prove TFT behavior.
