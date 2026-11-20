# MèoDesk v1.6 — Motion Pack v2 implementation and hardware acceptance

Target: ESP32-S3 WROOM-1 **N16R8** (16 MiB Flash, 8 MiB OPI PSRAM),
ST7789 320×240, LovyanGFX 40 MHz SPI, two internal-memory DMA strips.

## Why three animation frames per pose?

The existing `assets` partition has two `0x1F0000`-byte raw banks. A v2
collection of **16 poses × 3 frames × 144 × 144 × 2 bytes = 1,990,656 bytes**
fits in one bank, leaving **36,864 bytes** after the 4 KiB commit sector and
payload. Four uncompressed frames would *not* fit without changing the
partition map or adopting compression. We retain fixed raw RGB565 to avoid
per-frame decompression, dynamic allocation or fragmented Flash reads.

Each v2 `.meocat` has precisely **48 consecutive frames in pose-major order**:

- Pose 0 `boot_0`, `boot_1`, `boot_2`
- Pose 1 `clock_0`, `clock_1`, `clock_2`
- … same pose order as the existing v1 format …
- Pose 15 `shutdown_0`, `shutdown_1`, `shutdown_2`

Every frame is RGB565 **little-endian**, row-major, 144×144. `.meocat`
contains pixel data only, **no leading header**. Filename is not embedded.
The generated `.manifest.json` documents SHA-256, geometry, names, whether
the images were synthetic demo frames, and the per-pose frame count.

## Renderer: smooth refresh is separate from animation cels

- Cat cel sequence: `0 → 1 → 2 → 1 → …`, selecting a new cel every **170 ms**
  according to `millis()`. This gives roughly 5.9 cel transitions/second;
  it does **not** claim 25 distinct illustrated frames per second.
- SPI DMA **continues at its configured 20/25/30 FPS target**. Existing
  bobbing, sparkle and transition fade retain independent timing. Pose
  changes crossfade; changing cels within the same pose does **not** restart
  the fade (which would otherwise add per-pixel RGB mixing each 170 ms).
- When motion is off or Quiet mode is active, the cat stays on cel 0.
  Static v1 packs always select their one frame regardless of timing.
- All RGB565 v2 frames are copied into **1,990,656 bytes of PSRAM once** at
  boot after full SHA-256 verification. No runtime decoding, Flash reads,
  new TFT frame buffer, or PSRAM DMA target is introduced.

FPS and PSRAM throughput have **not been measured on real hardware**. The
previous observed ~19–20 FPS is not a guarantee for v2. Keep `Auto20–25`
initially; compare Web Studio's 5-minute FPS history, skipped frames and
peak render latency between built-in/v1 and v2. Try manual 30 only after a
stable baseline, and check both bright/dark theme modes.

## Create a test pack, then your own animation

Run from the `firmware/` directory (Pillow required):

```powershell
py -m pip install Pillow
py tools/make_catpack.py --motion-demo --images images --out motion_demo.meocat
```

The included `release/meodesk_v16_motion_demo.meocat` is produced this way:
it **reuses the original artwork**, shifting it upward by 0/1/2 pixels.
It is a transport and timing demo, not 48 newly illustrated expressions.

To create *real* animations, prepare three transparent 144×144 PNGs for each
pose using the exact names from the list above, then run:

```powershell
py tools/make_catpack.py --motion --images my_48_pngs --out my_motion.meocat
```

For original 16 static images, omit the new flags:

```powershell
py tools/make_catpack.py --images images --out classic_cats.meocat
```

All formats use a fixed 16-pose ordering. The script validates every source
PNG before writing the output. It reports the full SHA-256 of the payload.

## On-device compatibility, update and fallback

| Concern | v1 static (v1.5) | v2 motion (v1.6) |
|---|---|---|
| Payload | 663,552 bytes | 1,990,656 bytes |
| Bank header format | 1 | 2 |
| Frame count in header | 16 | 48 |
| PSRAM atlas | 648 KiB | 1,944 KiB |
| Bank commit magic | `MEOCAT15` | `MEOCAT15` |
| Header sector | 4 KiB | 4 KiB |
| Flash layout | Unchanged A/B | Unchanged A/B |

The firmware reads either header format. It orders valid bank headers by
generation, then verifies pixel SHA-256 into PSRAM. If the newest header,
pixel data, or PSRAM allocation fails, it attempts the older bank. If that
also fails, it renders the 16 original Flash-resident cat images. Upload
always erases the **inactive** bank and writes the commit header **last**.

`GET /api/v1/catpack/status` retains `pack_bytes:663552` for clients that
only understand v1, adding these optional fields:

```json
{
  "accepted_pack_bytes": [663552, 1990656],
  "frames_per_pose": 3,
  "active_pack_bytes": 1990656,
  "motion_ready": true
}
```

`POST /api/v1/catpack/upload` uses the **actual file size** in
`X-MEO-CAT-SIZE`; it must be exactly one of the two supported sizes. Other
headers (`X-MEO-KEY`, `X-MEO-CAT-SHA256`) are unchanged. A v1.5 firmware only
accepts v1 payloads; therefore **upgrade to v1.6 firmware before uploading
the new v2 `.meocat`**. Legacy `.meocat` files and older Windows agents
continue to work against v1.6. Reboot confirmation verifies the active hash.

This updater is unsigned **HTTP on a private LAN**. The pairing key is not
the Wi-Fi password. Do not port-forward its endpoints or share this project
with embedded credentials. Firmware A/B rollback depends on bootloader
support, while Cat Pack A/B uses its own verified header fallback.

## Hardware acceptance (not performed here)

1. Verify ESP32 reports 16 MiB Flash, about 8 MiB PSRAM and DMA on.
2. Update from an already-working v1.5 firmware with dual 6 MiB app slots
   using `release/meodesk_v16_esp32s3_n16r8_firmware.bin`.
   **Old `huge_app` requires a USB partition migration** instead.
3. Upload v1.5's 648 KiB default cat collection; check hash, visuals and
   `frames_per_pose:1`.
4. Upload `release/meodesk_v16_motion_demo.meocat`; check
   `frames_per_pose:3`, matching manifest SHA and 0→1→2→1 movement.
5. Disable cat motion/enable Quiet; confirm the cat freezes on frame 0 while
   Windows telemetry, Now Playing and Focus controls continue to function.
6. Compare FPS and free PSRAM for 10+ minutes in v1 and v2 modes, with the
   PC agent running, one browser open and the Wi-Fi router connected.
7. Test a deliberately *invalid* hash (no bank commit) and **do not**
   intentionally cut the board's power mid-Flash unless USB recovery is
   established. Host-side simulated power loss is covered by tests.

No physical flash/upload, visual verification, endurance run or native
Windows `.exe` build was performed in this release.
