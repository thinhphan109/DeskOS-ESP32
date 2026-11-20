# Hardware acceptance checklist — run on user's actual ESP32-S3/Windows PC

Do **not** mark these tests passed based solely on Linux builds or mock views.

For 30–60-minute FPS/RAM evidence, use the read-only recorder described in
[DEVICE_DIAGNOSTICS.md](DEVICE_DIAGNOSTICS.md). Keep Serial logs and complete
the physical checks below; recording alone does not mark hardware accepted.

## P0 snapshot & flash

- [ ] Save the working v0.7.1 source and, if available, built `firmware.bin`.
- [ ] Record Serial `[HARDWARE] Flash: ... MB | PSRAM: ... KB`, 115200 baud.
- [ ] Confirm `40 MHz` ST7789 RGB565 color bars without swapped red/blue.
- [ ] Record heap and cat FPS before/after enabling Media and Wi-Fi Web Studio.

## P4 firmware / TFT

- [ ] Flash `firmware/` via USB, confirm exactly one `setup()` and `loop()`.
- [ ] Observe Home clock NTP, PC metrics, Music track and Focus timer.
- [ ] Navigate Home/PC/Music/Focus and all 9 old scene IDs without blank screen.
- [ ] No ghost trails at 20 FPS target; DMA yes and card sprite yes if memory fits.
- [ ] Reboot 20×; network, display and two DMA strips all recover.

## P5 Windows media and commands

- [ ] Windows Companion CPU/GPU/RAM matches Task Manager, unavailable CPU temp is N/A.
- [ ] Start media player with Windows SMTC integration; title and artist show in
      app, Web UI, TFT; no player shows *Nothing playing*.
- [ ] Play/pause, next, previous from Web UI: observe 202 pending → ACK true,
      or ACK false for unsupported player. Never claim success on HTTP 202.
- [ ] Stop agent during queued command; no stale execution after 8 seconds.

## P6 Wi-Fi / recovery

- [ ] On Web Studio, configure known-good 2.4 GHz SSID+password, confirm STA IP.
- [ ] Enter a deliberately invalid password; after timeout, AP `MEO-DESK-05`
      remains reachable at `192.168.4.1` for corrective setup.
- [ ] Test separate COM connectors: if no Serial logs, enable USB CDC on boot.
- [ ] Companion scan discovers device when host IP changes; manual host works.

## P7 stability

- [ ] Windows built EXE launches by double-click **without system Python**.
- [ ] 30–60-minute run with Web history open and Windows media changing; record
      min heap, fps, disconnect causes and active WDT resets.
- [ ] Rollback to original v0.7.1 via USB upload if any hardware check fails.

## P8 Living Desk v0.9 acceptance

- [ ] Change Cozy Day → Midnight → Auto in Web Studio, confirm correct TFT
      theme, smooth cat mood crossfade, no cream rectangle. Reboot and confirm
      saved theme. Test Auto again after NTP time sync.
- [ ] On Focus scene, tap Web **Start/Pause/Skip/Reset** and Companion Focus tab;
      all three show the same phase and countdown within two seconds. Close
      Companion while timer runs; verify ESP32 continues. Reboot resets timer.
- [ ] Compare network down/up in Windows Companion with Windows Task Manager
      (note these are psutil *aggregate* adapter rates, not a single NIC).
- [ ] Confirm NVIDIA NVML VRAM used/total roughly matches GPU Task Manager;
      unplug/no NVIDIA should show N/A, not mock values. Existing GPU 0% remains 0.
- [ ] Induce safe CPU stress, verify WARNING event supersedes info but ends
      after TTL, with history still accessible from `/api/v1/events` using key.
- [ ] Visit **PC Network** page (index 13), verify real rate/VRAM and no
      overflow with missing counters. Record minimum heap/FPS with Midnight.

## P9 Living Companion v1.0 acceptance (DEFERRED)

- [ ] Four Web Scene Studio dropdowns configure different favorite pages.
      AUTO rotates only these four; legacy page numbering and manual `next`
      still visit all 14 scenes. Favorites survive a hardware reboot.
- [ ] Web heart button triggers a cat celebration and increments virtual
      energy; critical GPU/temperature/battery warnings still override petting.
- [ ] Media thumbnail from a player supporting Windows SMTC appears on the
      Windows Companion, ESP32 TFT Music page and authenticated Web Studio.
      Color bars/red-blue accuracy remain correct after RGB565 transfer.
- [ ] No media thumbnail: text-only Music still works; CPU/GPU/RAM continue.
- [ ] A song changing during upload cannot display the previous cover; a
      truncated multipart image must be rejected without replacing old data.
- [ ] Send artwork with invalid pairing key: receive HTTP 401; get thumbnail
      URL without pairing key: receive HTTP 401.
- [ ] Confirm no notable animation stutter and safe minimum free heap after
      an hour, including frequent album art and midnight theme switches.

## P10 OTA Guardian v1.4 acceptance (NOT YET RUN)

- [ ] Save a private NVS backup and known-good v1.2 source/binary. If your
      device still uses `huge_app.csv`, first migrate by USB (not over Wi-Fi).
      If v1.3 A/B is already running, **do not migrate partitions again**.
- [ ] Confirm `GET /api/v1/ota/status`: `ready:true`, different `app0`/`app1`,
      16 MB Flash and a 6 MiB inactive slot.
- [ ] Use included `release/meodesk_v14_esp32s3_n16r8_firmware.bin` to
      update through Windows Companion from a working v1.3 A/B device.
- [ ] TFT draws an advancing progress strip during upload; no animation
      frame/palette corruption after reboot. Verify progress does not impair
      Wi-Fi transfer or cause watchdog resets.
- [ ] Web Studio and Windows Companion report **confirmed** only when
      `running_slot` equals the previously inactive slot and `boot_health`
      changes `checking` → `passed` after approximately 25 seconds.
- [ ] Test interrupted upload, invalid pairing key and offline router; no
      false confirmation. Record actual `bootloader_rollback_configured`.
- [ ] 30-minute post-OTA endurance check: record measured FPS, DMA readiness,
      minimum internal heap, PSRAM free and unexpected resets.

Do not attempt network OTA on a single-slot `huge_app.csv` installation. The
new dual-slot table must first be installed and verified by USB.

## P11 Cat Pack Studio v1.5 acceptance (NOT YET RUN)

- [ ] Install v1.5 firmware through a valid A/B OTA slot (or use USB for
      initial migration). Confirm 16 MB Flash, 8 MB PSRAM, DMA on and cat FPS.
- [ ] With no custom pack: `/api/v1/catpack/status` reports `source:builtin`.
- [ ] Upload `release/meodesk_v15_default_cats.meocat` from Web Studio.
      TFT shows progress, ESP32 reboots, `source:custom` reports matching hash.
- [ ] Compare all 16 sprite pages and red/blue RGB565 colors with the old
      built-in appearance. Verify 25 FPS target and actual measured cat FPS.
- [ ] Repeat with Windows Companion **Cat Packs** tab and confirm automatic
      SHA check after reboot; manual IP and scan must both work.
- [ ] Interrupt a Cat Pack upload before commit: previous pack or built-in
      images should remain available on restart. Perform this only with USB
      recovery available. Check Serial for `[CATPACK]` errors.
- [ ] Compare measured FPS, internal heap and PSRAM free for built-in sprites
      versus the custom pack. PSRAM uses approximately 648 KiB for 16 frames.

## P12 Motion Pack Studio v1.6 acceptance (NOT YET RUN)

- [ ] Upgrade from v1.5 A/B firmware to v1.6 using the included compiled
      `release/meodesk_v16_esp32s3_n16r8_firmware.bin`. Existing v1 Cat
      Packs must still load and show `frames_per_pose:1`.
- [ ] Upload `release/meodesk_v16_motion_demo.meocat` over the router using
      Web Studio and/or Companion. Confirm SHA-256 matches the manifest,
      48 frames are present and `frames_per_pose:3` appears in status.
- [ ] Observe gentle 0→1→2→1 cel shifts with cat motion on. Switch motion off
      and Quiet on: the sprite must freeze at cel 0 without breaking PC data.
- [ ] Compare at least 10 minutes of real FPS, skipped frames, worst frame
      duration and PSRAM free in built-in, static v1 and motion v2 modes.
      Recommended initial FPS profile: AUTO 20–25. Do not assume 30 FPS.
- [ ] Try interrupted v2 upload only with working USB rescue; check that the
      previously committed v1/v2 bank survives. Do not interrupt during
      the first real firmware OTA migration.

## P13 Living Motion v1.7 acceptance (NOT YET RUN)

- [ ] Confirm N16R8 Serial shows 16 MB Flash, ~8 MB PSRAM, DMA on, and
      `/api/v1/status` advertises `motion_director_v1` with `motion_style:1`.
- [ ] Web Studio Living Motion and Windows Companion Cat Packs controls both
      switch Calm/Natural/Playful. Each selected style persists after reboot.
- [ ] Verify music playback triggers `motion_state:music`, Focus work triggers
      `focus`, and a real critical warning overrides ordinary idle/music.
      Test low energy/sleep/celebration without making up telemetry values.
- [ ] With Windows telemetry active and genuinely high CPU/GPU load, verify
      motion changes. Stop the Windows agent or enter demo mode; verify the
      PC-driven motion is absent and no fake CPU/GPU load is displayed.
- [ ] Use v2 48-frame pack to observe different cels and test a v1 legacy
      16-frame pack; no data corruption, boot loop or forced pack migration.
      A real closed-eye blink requires custom `*_2.png` art.
- [ ] Compare actual FPS, missed deadlines, render time and free PSRAM for
      Calm/Natural/Playful for at least 10 minutes, with music and Web open.
      Under load, `motion_budget_limited` may engage; it must release after
      sustained healthy windows. FPS target must not change except AUTO.
- [ ] Confirm Wi-Fi reconnect, AP fallback, OTA Firmware and Cat Pack upload
      still work, then perform a longer 30–60 minute soak test.
