# MèoDesk v1.7 — ESP32-S3 WROOM-1 N16R8 performance contract

**Target:** 320×240 SPI ST7789, 40 MHz, fixed legacy GPIOs (SCK12,
MOSI11, CS10, DC9, RESET8). This is **N16R8**, not WROOM-2 N16R8V.

## Memory policy

- N16R8: **16 MB Quad SPI Flash, 8 MB Octal (OPI) PSRAM**. PlatformIO uses
  `qio_opi`, declares 16 MB Flash and adds `BOARD_HAS_PSRAM`.
- The ~9 KiB ping-pong SPI DMA buffers stay in **internal DMA-capable RAM**.
- The ~38 KiB card canvas prefers **internal SRAM for speed**; if that
  allocation fails, fall back to a PSRAM canvas. If neither succeeds, draw
  the changing card directly. The main animation does not require PSRAM.
- A temporary **16 KiB** cover-art pair (incoming + active) prefers PSRAM,
  with an internal RAM fallback; missing cover buffers only disables covers,
  never CPU/RAM telemetry.
- Default 144×144 cat sprites remain in Flash (`PROGMEM`), not PSRAM. No full
  screen framebuffer or cache thrash is added solely because PSRAM is large.
- Never force external PSRAM into a DMA buffer without checking the exact
  driver, alignment, SPI peripheral and ESP-IDF support.

## Frame pacing

### v1.7 Living Motion FPS Guard

`MotionDirector` inspects the existing five-second measured FPS/target/missed
deadline/worst-render window. After two overloaded windows it suppresses
optional pose fade/sparkles, reduces bob to ≤1 pixel and limits cel changes.
It restores the effect budget only after four healthy windows. **This is an
effect-budget guard, not an FPS booster or a frame-target regulator.** The
existing AUTO governor independently manages its 20↔25 FPS policy. No new
full-screen buffer or Flash I/O is introduced to render living motion. Test
measured FPS and PSRAM bandwidth with the actual hardware.

`FramePacer` is a pure-C++ clock-based scheduler. It measures deadlines in
microseconds, handles 32-bit `micros()` wraparound and **skips stale animation
deadlines** instead of accumulating blocking delays. The pose is always
calculated from the current `millis()` (animation is time-based, not tick-based).

| Web Studio profile | Target | Nominal interval | Comment |
|---|---:|---:|---|
| Stable | 20 FPS | 50.0 ms | Lower SPI/CPU load |
| Balanced (default) | 25 FPS | 40.0 ms | Best starting point for N16R8 |
| Smooth | 30 FPS | 33.3 ms | Only if measured headroom allows it |
| Auto | 20 ↔ 25 FPS | 50 or 40 ms | Conservative recovery under changing load; starts at 25 |

### AUTO governor, v1.2

`FrameGovernor` inspects completed five-second windows **only when** cat
animation and DMA rendering are active. It starts at 25 FPS, downgrades to
20 after **three consecutive overloaded windows**, and upgrades after **six
consecutive healthy windows**. Switching targets rebases deadlines so the
new profile does not inherit stale missed frames. Measurements are intentionally
conservative to reduce oscillation; the governor never selects 30 FPS.

Selecting a manual 20/25/30 profile disables automatic changes. The choice is
saved in existing NVS preferences as `fps_profile` (0, 1, 2, or 3); Web Studio
shows both the saved profile and the instantaneous effective `fps_target`.

### Five-minute performance history, v1.2

`PerformanceHistory` is an allocation-free ring of **60 × 10-byte** logical
records (plus tiny indexes), sampled every 5 seconds. On-device fields are
measured FPS ×10, effective target, deadline misses (capped at 255), maximum
render duration (ms), internal free heap (KiB) and PSRAM free (KiB).

`GET /api/v1/performance` returns oldest-to-newest arrays:

```json
{
  "schema": 1, "sample_period_s": 5, "count": 2,
  "fps": [24.8, 19.7], "target": [25, 20], "skipped": [1, 2],
  "worst_ms": [18, 21], "internal_kb": [182, 181],
  "psram_kb": [7840, 7840]
}
```

This is an **example format**, not a claim of measured hardware values. The
history is volatile (resets on boot). Web Studio polls once per 5 seconds and
draws its graph on the **browser** canvas, not on the ESP32 TFT.

**v1.5 Cat Pack memory:** If a valid custom pack is present, a contiguous
663,552-byte RGB565 sprite atlas is loaded into PSRAM once during startup.
Normal animation reads from that buffer and still uses two internal DMA
strips; no per-frame Flash read and no full-screen backbuffer are added.
If PSRAM allocation/validation fails, built-in sprites are selected. These
additional PSRAM reads may change measured FPS; the effect must be checked
on the actual board before claiming parity with v1.4.

**v1.6 Motion Pack memory:** a v2 collection allocates **1,990,656 bytes
of PSRAM**, holds all 48 RGB565 frames and selects frame pointers every
170 ms. The CPU still renders each target TFT refresh into the same small
internal DMA strips; no cel decoder, per-frame Flash read, dynamic buffer
allocation or full-screen DMA frame is introduced. Cel switches do not restart
the 230 ms pose crossfade, reducing redundant RGB565 interpolation. With
8 MiB PSRAM, this uses roughly 1.9 MiB more than built-in assets (and 1.3
MiB more than v1 static packs). Startup SHA/PSRAM copy is heavier; real
FPS and latency must be compared with a working board. See
`MOTION_PACK_V2.md` for a reproducible 10-minute acceptance test.

These are **targets**, not hardware-certified measurements. Actual FPS may
be below target when Wi-Fi, web traffic, card redraws, or media uploads consume
CPU/SPI bandwidth. Lowering target is safer than forcing an over-budget loop.

Animation has priority over noncritical card repainting. When a cat deadline
is approaching, ordinary card updates wait for a free slot, capped at 2× the
card's normal interval to prevent starvation. Static frames are never redrawn
on every cat frame. Web Studio and Serial expose measured FPS, worst cat frame
render time in the last 5-second window, deadline skips and free RAM.

At 40 MHz SPI, one 320×240 RGB565 full-screen transfer takes 30.72 ms at the
absolute theoretical payload limit (~32.5 FPS), **before** control overhead.
The cat strip area is smaller, so a 25 FPS *target* is reasonable to try,
but must be verified on the actual TFT and wiring.

## Safe migration and rollback

1. **v1.3 changes `huge_app.csv` to `meodesk_ota_16mb.csv`.** This requires
   one-time USB flashing and a private NVS backup before any LAN OTA is
   attempted. Read `OTA_MIGRATION.md`; the new partition table is NOT itself
   updated via the firmware OTA upload API.
2. Keep the previous compiled `baseline/meodesk_v09_esp32s3_firmware.bin`.
   The new source has no newly verified firmware until PlatformIO compilation
   and actual-device TFT tests succeed.
3. On the real N16R8, Serial should report ~16 MB Flash and ~8192 KiB PSRAM.
   If PSRAM reports 0, check the module marking and board config instead of
   assuming no physical PSRAM exists.
4. Never apply `opi_opi` (WROOM-2/N16R8V) to a WROOM-1/N16R8 without verifying
   the actual module. SPI 40 MHz on the TFT is independent of OPI PSRAM mode.

## Physical acceptance criteria (not yet measured)

- Boot and reconnect Wi-Fi with the existing NVS SSID/password; fallback AP
  remains reachable.
- Confirm no wrong colors in the cat DMA strips, no tearing during cover-art
  upload or page switches, and no watchdog reset during a 30-minute session.
- Watch `[PERF]` over 30 minutes with Windows telemetry streaming, Web Studio
  open, media changes and AUTO scenes active. Aim for **≥23 measured FPS** in
  Balanced25 during steady animation; measure missed deadlines and render max
  to identify bottlenecks. This is an acceptance goal, not a present result.
- If Balanced25 cannot hold reliably, try Auto (20–25) or select Stable20.
  If Smooth30 is sustained, it can be chosen per-device and saved in NVS.
- Track `internal_ram_kb`, `internal_dma_kb`, `psram_free_kb`; the smallest
  free contiguous block matters too. Do not claim full-frame 60 FPS at 40 MHz.
