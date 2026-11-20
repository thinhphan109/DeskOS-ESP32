# MèoDesk v1.7 — Living Motion Engine

Hardware target: ESP32-S3 **WROOM-1 N16R8**, ST7789 320×240, SPI 40 MHz,
LovyanGFX, two small internal-RAM DMA strips, optional 8 MB PSRAM cache.

## Architecture and performance budget

`firmware/src/core/motion_director.h` is a self-contained C++ policy engine.
It has **no dependencies on Arduino, Wi-Fi, TFT, PSRAM or filesystems** and
does not allocate memory. Per rendered TFT frame it computes a tiny cue:

- `mode`: why the cat is behaving this way;
- `cel`: index 0–2 into an existing v2 Motion Pack (or cel 0 for v1/built-in);
- `bob`: small vertical motion amplitude;
- `blink`: a short, nonperiodic cel-2 cue;
- `sparkle` and `fadeAllowed`: optional extras;
- `budgetLimited`: whether the visual effects guard is engaged.

The **actual TFT frame scheduler is unchanged**: `FramePacer` owns the
20/25/30 FPS targets, the AUTO governor can switch between 20 and 25,
the two 144×16 RGB565 DMA buffers remain in internal RAM, and Web/Now
Playing/PC data updates run at their own rates. No full-frame buffer,
decompression, extra Flash writes, per-frame file reads or network calls
are introduced in the animation loop. The active 16/48-frame `.meocat`
atlas still loads into PSRAM once at startup.

## State priorities

| Priority | Condition | Motion response |
|---|---|---|
| 1 | Hot/low battery/busy warning, or fresh non-demo CPU/GPU ≥90% | Alert: short rhythmic movement; no idle blink |
| 2 | Focus completion/petting celebration | Celebrate: lively 0→1→2→1 loop, small sparkles |
| 3 | Focus work timer running | Quiet Focus: subtle bob, slow frame changes and rare blink |
| 4 | Windows Now Playing `playing:true` | Music: rhythmic 0→1→2→1 loop, responsive bob |
| 5 | Sleeping/quiet/low virtual energy | Sleep: almost still, infrequent frame changes |
| 6 | Waiting for Windows PC telemetry | Searching: idle movement and blink cues |
| 7 | Fresh real PC load ≥75%, or otherwise healthy desk | More alert idle / cozy breathing and occasional blink |

**Crucial:** `cpu`, `gpu` and computed intensity are considered only when
`pcTelemetryFresh() && !pcTelemetry.demo`. A missing GPU (`-1`) is not a zero
or a warning. High CPU/GPU does not masquerade as a real measurement when
the Windows companion is in demo mode. Critical warning sprites already
override ordinary page art through the existing mood engine.

## Natural timing and artwork requirements

- Idle alternates between cel 0 and 1. Short cel-2 blink cues occur roughly
  every 5–10 seconds at deterministic irregular times; Focus blinks less.
- Music and celebration loop `0→1→2→1`; the step interval depends on style.
- **Blink is an animation cue, not a computer vision eyelid edit.** The user
  needs to draw an actual closed-eye `*_2.png` to show an eye closing.
  The bundled v1.6 Motion Pack demo only shifts artwork 0/1/2 pixels.
- The `--motion` pack tool still accepts 48 144×144 PNGs arranged
  pose-major. The v1 16-image format remains valid but cannot show distinct
  cels. The binary `.meocat` v2 format and Flash A/B layout are unchanged.
- The `--motion-demo` pack is only a transport/motion test; it contains no
  new authored facial expressions.

## Three personality styles

All styles use the same priority rules and local telemetry. They only
change timing/amplitude and are stored in the existing ESP32 NVS namespace
as one bounded byte (`motionstyle`, range 0–2):

| Value | Name | Behavior |
|---:|---|---|
| 0 | Calm | More rests, smaller bob and slower dance |
| 1 | Natural (default) | Balanced ambient movement and blink cues |
| 2 | Playful | Faster dance, more pronounced bob and more frequent blink cues |

Web Studio: **Living Motion** buttons in the ambient panel. Windows
Companion: **Cat Packs** tab → Living Motion section. Both use the existing
authenticated `POST /api/control` endpoint:

```json
{"action":"motion_style","value":1}
```

`X-MEO-KEY` (pairing key, **not** Wi-Fi password) is required. No additional
port or cloud account. Invalid style values are rejected. Older firmware
does not have this capability; the Windows agent skips optional polling.

The read-only `/api/v1/status` adds these **optional** fields:

```json
{
  "motion_style": 1,
  "motion_state": "music",
  "motion_cel": 2,
  "motion_blink": false,
  "motion_budget_limited": false,
  "motion_pc_intensity": 63,
  "cat_motion_frames": 3
}
```

`motion_pc_intensity` is real CPU/GPU load only; it is 0 for DEMO or
offline data. Clients should also check existing `pc_connected` and `demo`
flags. The new capability string is `motion_director_v1`. No existing JSON
fields or HTTP routes were removed.

## FPS Guard — what it does and does *not* do

Once per existing five-second performance window, `MotionDirector` observes
**measured** FPS, the current FPS target, missed deadlines and worst cat
render duration. It enters the limited-effects state after **two overloaded
windows**, and exits after **four healthy windows**. While limited it:

- reduces bobbing to at most one pixel;
- turns off optional sparkles and pose crossfade;
- reduces cel switching to one change every 1.2 seconds.

It **does not** change the configured FPS target, skip any new DMA buffer
transfer, drop network data or guarantee a higher measured FPS. The existing
AUTO FPS governor separately controls 20↔25 if that profile is enabled.
The guard adds no memory allocation or extra timers/tasks. Direct user
feedback is available from `motion_budget_limited`, actual `cat_fps`, the
five-minute Web Studio performance graph and Serial `[PERF]` lines.

## Upgrade from v1.6 and verify on the real screen

1. Keep the last known-good USB firmware and NVS backup. If already on the
   v1.3+ dual 6 MiB `ota_0`/`ota_1` partition layout, upload the included
   `release/meodesk_v17_esp32s3_n16r8_firmware.bin` using **Firmware OTA**.
   A device still using `huge_app.csv` requires the documented one-time USB
   partition migration; do not OTA straight from that layout.
2. Keep the existing `release/meodesk_v16_motion_demo.meocat` or any custom
   16/48-frame pack. **No asset re-upload is required** when moving to v1.7.
3. Verify network and Windows agent, switch Natural/Calm/Playful in Web and
   Windows Companion, then reboot and confirm the style was retained.
4. Test real music, a Focus session and a simulated warning **by using the
   built-in preview only for art**. A preview is not evidence of real GPU
   temperature or CPU load.
5. Compare actual FPS, missed deadlines, PSRAM free, night-mode mixing and
   card updates for at least ten minutes before/after. Test AUTO20–25 first;
   only use 30 FPS if measured time budget allows it.
6. Test offline router and no-Windows conditions; the device must remain
   usable on its fallback AP and continue drawing without sensor data.

Firmware and native host tests are provided, but **no physical TFT/FPS,
Wi-Fi OTA soak run or Windows-native EXE build** was performed in this
release. Serial, screenshots and real FPS are needed to validate the result
on the owner's board.
