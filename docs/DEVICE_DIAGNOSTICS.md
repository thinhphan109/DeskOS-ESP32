# Read-only device endurance recording

With a Performance Lab build, CSV/JSONL also preserve tracing validity,
CPU/DMA-wait/card/bridge timings, tracked payload rate and theme. See
[PERFORMANCE_LAB_V18.md](PERFORMANCE_LAB_V18.md) for coverage and a repeatable
manual comparison. Older firmware leaves these fields blank.

The next v1.7 milestone is physical acceptance. Use the stdlib-only recorder
from the project root on a PC attached to the device's LAN:

```powershell
py windows_agent/device_diagnostics.py --host meodesk.local --duration 1800
```

Use the actual private IPv4 if mDNS is unavailable. No Python packages, pairing
key, Wi-Fi password or connected Companion process are required by the recorder.
For the load test, run Companion separately to send real PC/media data and keep
Web Studio open. The tool only issues GET requests to four existing endpoints:
`/api/status`, `/api/v1/performance`, `/api/v1/ota/status`, and
`/api/v1/catpack/status`. It cannot flash firmware, change settings or restart
the board. It bypasses proxy settings and does not follow HTTP redirects.

Choose a new output folder for each configuration:

```powershell
py windows_agent/device_diagnostics.py --host 192.168.0.107 --duration 3600 --output diagnostics/natural-v2-auto
```

The address above is only an example. Select the desired FPS profile, motion
style and pack in Web Studio before starting. Record separate sessions for
built-in art, static v1, motion v2, and Calm/Natural/Playful comparisons.

## Files and interpretation

Each run creates a new folder under `diagnostics/` unless `--output` is set.
An existing folder is never overwritten. Files are UTF-8:

- `samples.jsonl`: one allowlisted snapshot per poll, including endpoint errors
  and the rolling five-minute performance history when available.
- `metrics.csv`: current device FPS/target, worst render time, missed frames,
  internal/DMA RAM and PSRAM, plus configuration, PC mode and boot/pack status.
  Missing values stay blank; valid zero readings remain zero.
- `summary.json`: sample/error counts, observed metric minima/maxima/means,
  latest device/OTA/pack metadata, and duration/stop reason.

SSIDs, PC/GPU names, song metadata, event text, pairing keys and arbitrary
response/error strings are not saved. Host and device ID are retained so a
session can be traced to its device. Review logs before sharing them.

`source=device_reported` means measurements reported by recognized device
firmware, not an independent physical attestation. Recognized mock responses
are labeled `simulated` and their summary statistics are separate. `pc_mode`
is independently `real`, `demo`, `offline` or `unknown`: a real board still
measures its actual display FPS when PC telemetry is in DEMO mode.

Polls default to five seconds; `--interval` cannot be less than five. The
summary uses current status snapshots, not concatenated history rings. Samples
can repeat the same firmware measurement window, so means are **poll-snapshot
means**, not unique-window or frame-weighted FPS. `cat_frames_skipped` is a
per-window count; the recorder reports its min/max/mean and never adds it into
a potentially double-counted total. Compare separate runs at the same cadence.
The early startup window may report zero FPS before the first measurement.

An uptime decrease is recorded as `uptime_decreases`. This can indicate reboot
or the firmware's `millis()` rollover; use Serial logs to establish the cause.
Neither boot-health PASS nor `bootloader_rollback_configured` proves actual
hardware rollback. `hardware_acceptance` always remains `not_assessed`.

## Outages, older firmware, and stopping

Connection failures are recorded as gaps and retried on the next poll. When
the primary status request fails, optional requests are skipped for that poll.
Optional endpoints returning 404/405/501 are marked unsupported for the rest
of that firmware version, so a v1.1 board can still provide its available
status fields. Optional transient failures are retried. Each request has a
two-second timeout; a slow poll may exceed the selected interval, with no
catch-up bursts. No measurements are invented for missing APIs or outages.

Press Ctrl+C to stop early. Completed samples are flushed after every poll,
and Ctrl+C writes a final summary. Forced termination/power loss can leave
the summary absent; already flushed JSONL/CSV samples remain usable.
Timestamps are UTC with an offset; `elapsed_s` uses a monotonic clock.
Exit code 0 means at least one status snapshot was saved; 1 means no valid
device status was received; 2 means invalid arguments or an output-file error.
None of these codes means hardware acceptance passed.

Run for 30–60 minutes alongside the visual and recovery checks in
[HARDWARE_QA.md](HARDWARE_QA.md). This recorder does not replace Serial logs,
TFT color/tearing checks, Wi-Fi recovery tests or USB partition migration.
