# Performance Lab — first instrumented build

This extends the Personal Studio 1.8.0-dev source. The owner chose **build and
preview only**: no firmware was installed or settings changed on the ESP32.
The existing Studio-only development image is retained separately.

## What is already measured, and what is new

Installed v1.7 already exposes actual cat FPS, target, missed deadlines,
worst cat render time, internal RAM, PSRAM and a five-minute history. Live
read-only queries on 20 November 2026 returned 23.8–25.2 FPS over the last
60 five-second samples at a target of 25. This is a pre-update observation.

The new optional `performance_lab_v1` capability adds fixed counters around
the existing rendering path. No framebuffer, per-frame allocation or sensor
polling is added. Tracing defaults off, is volatile, and can be toggled via
Web Device & updates, Companion Overview, or the existing keyed control API:

```json
{"action":"perf_trace","value":1}
```

Set value 0 to disable. Each toggle resets the window and invalidates the
previous sample. A completed window needs at least five seconds; the first
can be longer because it closes on the existing performance scheduler.
Fields are additive to `/api/status` and shared WebSocket state:

| Field | Meaning |
|---|---|
| `lab_enabled`, `lab_valid`, `lab_window_ms` | Tracing state, completed-window validity and actual interval |
| `lab_cpu_frame_ms` | Sum of strip-building CPU spans divided by cat frames |
| `lab_strip_cpu_ms` | Mean strip-building CPU span, not full-frame CPU time |
| `lab_dma_submit_ms` | Mean `pushImageDMA` call wall time per strip |
| `lab_dma_wait_ms` | Mean blocking `waitDMA` span per strip |
| `lab_cat_wall_ms` | Mean full cat-render call wall time per frame |
| `lab_card_wall_ms`, `lab_card_max_ms` | Mean/max full card-render call wall time |
| `lab_bridge_wall_ms`, `lab_bridge_max_ms` | Mean/max cooperative bridge-loop call wall time |
| `lab_payload_bytes_s` | Tracked RGB565 bulk payload bytes divided by actual window duration |
| `lab_cat_frames` | Number of cat calls in that window |

Unavailable/no-call measurements are null. Tracing-off values are not zero
performance measurements. Core counters remain bounded and allocation-free.

## Measurement boundaries

CPU strip spans, DMA submission spans, waits and full-render wall time are
nested/overlapping views; **do not add them to infer utilization**. The CPU
can prepare a strip while DMA transfers the previous one. Wait duration is
CPU stall time, not total DMA-active or SPI-active time. Submission may also
include driver work. Hardware bus utilization needs driver-level timestamps
or a logic analyzer; this implementation does not claim to measure it.

Tracked bytes cover cat DMA rectangles, buffered card pushes and album tiles.
They exclude direct text/shapes/header updates, commands, setup overhead and
unbuffered-card drawing. `lab_payload_coverage` identifies this partial
coverage. It is neither a full TFT bus-byte total nor measured SPI throughput.

Instrumentation itself has a cost. Compare trace-off and trace-on runs with
matched page/theme/style/pack/PC/browser conditions before interpreting a
regression. No post-update FPS or tracing overhead has been measured on the
physical board yet, and there is no automatic benchmark PASS threshold.

## Repeatable manual benchmark

After separately authorizing installation of the development image:

1. Keep the FPS target fixed at 25 for the first comparison. Choose a single
   page and hold PC/media/browser workload steady.
2. Record trace-off baseline, then trace-on with the same settings. Use
   ten-minute sessions once startup has settled.
3. Compare Cozy Day/Midnight, Calm/Natural/Playful and built-in/v1/v2 packs
   in separate runs. Change one variable per run, then compare AUTO if useful.
4. Use the read-only recorder from the root, for example:

```powershell
py windows_agent/device_diagnostics.py --host 192.168.0.107 --duration 600 --output diagnostics/midnight-natural-v2-trace-on
```

CSV/JSONL now preserve theme/style/frame count, tracing validity, window
duration, timing values and payload coverage alongside FPS and memory.
The existing summary statistics are poll-snapshot statistics; windows may
repeat between polls. Missing counters stay missing on old firmware. Keep
Serial and visual observations for the clock/tearing/recovery checks.

## Renderer/widget interface foundation

`core/widget_contract.h` defines renderer-independent `WidgetRegion`,
`WidgetSpec` (bounds/cadence/second alignment) and `RenderSurfaceInfo` (geometry,
RGB565 depth, DMA capability). The actual card scheduler consumes this spec
and validates its bounds. Sensor collection remains independent.

This is the first contract seam, not a complete Widget SDK or renderer
rewrite. The current scenes still draw through LovyanGFX. Scene extraction,
a drawing adapter, editor serialization and draggable widgets need isolated
follow-up work with regression evidence; no LVGL migration was introduced.

## Verification

Combined image: `release/meodesk_v18dev_performance_lab_esp32s3_n16r8_firmware.bin`
with its SHA manifest. Full PlatformIO build succeeded: static RAM 57,780 bytes
(17.6%), application Flash 36.0% of the 6 MiB slot. The initial Studio-only
image remains `release/meodesk_v18dev_esp32s3_n16r8_firmware.bin`.

74 Python tests passed, including the new recorder coverage/missing-value
check. Four Node suites and the new native C++ Performance Lab/widget tests
passed. No physical 1.8 benchmark or automatic benchmark runner was executed.

Native C++ tests cover optional counters, correct rates/means/maxima, window
reset/rollover and widget bounds. Existing CardRefresh/domain/motion tests
remain relevant. Python tests cover diagnostic CSV privacy/missing values and
the GUI/shared-state behavior. The full PlatformIO build is required before
shipping the combined image. Physical timing, bus analysis, automated
benchmark orchestration and post-update endurance remain pending.
