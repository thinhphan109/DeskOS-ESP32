# MèoDesk — extension points after v1.0

**Status:** design constraints, not claims of tested v1.0 behavior.

## Compatibility boundaries

1. **Stable page IDs:** the `Page` enumeration in `firmware/src/main.cpp`
   remains append-only. `SceneManager` references IDs, not page positions in a
   new UI list. Never renumber an existing ID; append new optional scenes.
2. **Stable public schema:** the `schema: 1` payloads stay backward compatible.
   New values and optional fields require validation and a capability flag,
   documented in `protocol/compatibility.md`; clients ignore unknown fields.
3. **Separate data ownership:** ESP owns focus timing, cat energy and selected
   scenes. Windows owns measurements and media playback commands. Both may
   disconnect without making up CPU load, temperature or song data.
4. **Optional hardware:** Wi-Fi fallback and the existing 320×240 TFT remain
   operational when the Windows app, an audio player or a sensor is unavailable.

## Where to add features

| New feature | Main module | Public contract | Safety constraint |
|---|---|---|---|
| Add a TFT page | `firmware/src/main.cpp` / SceneManager | Append page ID | Preserve old scene IDs |
| New cat emotion | `core/cat_behavior.h` | `cat_persona` + optional new asset | Critical warnings always win |
| More Windows metric | `windows_agent/pc_agent.py` | Optional telemetry field | Unsupported = `null`, not 0 |
| PC/media integration | `media_provider.py`, `companion_core.py` | `api/v1` or new versioned route | Do not block the 1Hz telemetry loop |
| Per-user choices | `pc_bridge.cpp` NVS adapters | Separate short keys / migration version | Avoid excessive flash writes |
| Browser screens | `web_ui.h` | Read status, keyed control | All mutations require pairing key |
| New images | `images/`, `cat_assets.cpp` | Flash RGB565 or bounded volatile upload | No full 320×240 framebuffer on N8 |

## Why artwork is a separate plane

JSON status is small and polled frequently; putting thousands of image bytes
there would destabilize both the Web UI and the 1Hz agent loop. Artwork uses a
fixed 8,192-byte authenticated upload with an explicit track ID, plus a single
active/staging pair of RAM buffers. A cover is **never** persisted in flash.
Future weather icons, QR codes and remote images should use similarly bounded
binary routes (potentially `api/v2`) instead of embedding base64 in status.

## Next milestones (require their own approval and tests)

1. **v1.0 hardware acceptance:** build in PlatformIO, flash ESP32-S3, observe
   TFT colors/artwork, exercise real Windows media and repeat a 30-minute
   endurance run. Deferred by owner at the time this source preview was made.
2. **v1.1 component split:** move the existing renderer functions to a
   `render/` module behind a narrow drawing interface. Do this as an isolated
   mechanical move with screenshot regression, not simultaneously with features.
3. **v1.2 sensor adapters:** use a small provider interface for optional real
   ambient temperature, brightness or touch input. No sensor, no data claim.
4. **v1.3 onboarding:** add device pairing discovery and explicit session key
   rotation without invalidating installed v1 clients.
5. **Before OTA:** verify real flash capacity/partition table, rollback path,
   failure recovery and signature policy. Existing `huge_app` is **not** a safe
   dual-bank OTA layout.

This file does not replace `ARCHITECTURE.md` or the machine-readable
`protocol/schema-v1.json`; it records the extension policy that keeps future
work additive.
