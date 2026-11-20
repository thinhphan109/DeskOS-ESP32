# Personal Studio / shared state — 1.8.0-dev

Development build, 20 November 2026 (Vietnam). Firmware and Companion must
both be updated for push synchronization. Existing HTTP v1 clients still work.

## Confirmed starting point

Read-only requests to the owner's `192.168.0.107` returned firmware 1.7.0,
16 MiB Flash, 8 MiB PSRAM, DMA and buffered cards active. OTA reported
`ready:true`, `running_slot:app0`, `inactive_slot:app1`, a 6 MiB inactive
slot and `boot_health:passed`. Cat Pack reported both v1/v2 support.
The last 60 device-reported five-second FPS samples ranged from 23.8 to
25.2 with a target of 25. These are device diagnostics, not visual TFT or
independent rollback validation. The owner confirms the two-second clock
symptom occurs on the TFT when Midnight is selected.

This installed A/B layout supports subsequent firmware and Cat Pack uploads
over Wi-Fi without UART. It does not require another partition migration.
USB remains the recovery route if the device cannot boot or join a network.

## Shared state architecture

ESP32 is authoritative for scenes, four favorites, theme, FPS profile, motion
style/on/off, Auto pages and Focus. Windows and Web subscribe to
`ws://<device>:81/`; HTTP on port 80 continues to handle controls, telemetry,
artwork, firmware uploads and Cat Pack uploads. There is no broker/cloud.

- Capability: `state_websocket_v1`; HTTP status advertises `state_ws_port:81`.
- Up to four WebSocket clients. Status uses the existing public status fields.
- Send `{"type":"subscribe","key":"<existing-pairing-key>"}` to include
  media metadata. The key is not put in a URL. HTTP mutation authentication
  is unchanged. No new account, TLS setup or pairing workflow is introduced.
- Frames have `type:"state"`, a per-boot `state_boot_id`, and a
  `state_revision`. Revision increases on committed HTTP writes and periodic
  broadcasts. Clients reject older revisions within the same boot epoch.
- Successful mutations mark state dirty; broadcasts coalesce on a 40 ms
  minimum interval. Idle snapshots arrive about once a second. These are
  scheduling intervals, not guaranteed end-to-end latency measurements.
- HTTP uploads remain streamed; state broadcasts pause while Flash updates
  are active. Clients reconnect after restart and fall back to HTTP when
  no recent stream state is available. Firmware <=1.7 uses HTTP fallback.
- Qt `QWebSocket` runs on the GUI event loop. Sensor collection/HTTP writes
  remain on worker threads. No additional Companion runtime package is needed.
- Focus interpolates displayed seconds only while recent device state is
  available; it never invents phase completion after disconnect.
- New boot epochs reset the worker's command sequence and volatile artwork
  cache. Paused media now receives a heartbeat instead of silently expiring
  on ESP32 after ten seconds.

Windows startup/media-provider preferences and browser workspace selection
remain local settings. A selected local pack preview is not installed or
shared until the user explicitly starts its upload. Media transport commands
retain the existing sequence/ACK protocol and background polling.

## Compact Studio

Web organizes the same functionality into Overview, Personalize, Focus &
music, and Device & updates. Primary cards are roughly 250 px tall on desktop
instead of the previous 385 px minimum. Controls and spacing are tighter;
long motion explanations are expandable. The layout reflows on mobile.

Personalize previews any of the 16 poses in a local v1/v2 `.meocat`, decoding
RGB565 little-endian pixels and playing v2's 0-1-2-1 sequence. Preview is local;
selecting a file does not write Flash. Existing checksum/upload/reboot
verification remains in the explicit Install action.

Companion's Studio adds the matching scene selector, motion/Auto toggles,
theme/FPS controls and favorite scenes, alongside existing motion styles and
pack upload. Device state updates the controls without emitting new commands.
The sidebar, margins, typography and cards are more compact. Content has an
explicit light surface so Windows dark system colors cannot hide its text.

## Midnight TFT clock fix

The old renderer deferred a due card whenever there were <=14 ms before the
next cat frame. Under sustained rendering pressure it forced a card only
after **twice** the interval, causing a nominal one-second clock to update
every two seconds. Midnight's per-pixel background remapping makes this
starvation path more likely.

`core/card_refresh.h` aligns clock/Home/Focus refresh to second boundaries and
limits optional deferral to 80 ms. Other cards retain their intervals with
the same bounded deferral. Main samples a fresh time after drawing the cat.
The policy changes which work receives priority; a time card may cause a
missed animation deadline. Actual post-update TFT timing and FPS must be
checked on hardware. GPIO, SPI speed, DMA buffers and partitions are unchanged.

## Validation and device acceptance

- 73 Python tests passed in the local `.venv`, including GUI construction,
  two actual Qt WebSocket clients, state ordering/reboot epochs, Focus display
  freshness and paused media heartbeat. No GUI tests skipped in this run.
- Native C++ CardRefresh, domain and Motion Director tests passed.
- Four Node tests cover SHA, OTA, Cat Pack and shared state/pack RGB565 decode.
- Full PlatformIO ESP32-S3 build succeeded. See the release manifest for the
  exact image length and SHA-256. The dev build embeds the local owner config.
- Web checked at desktop size and 390 px viewport: no horizontal overflow;
  local v2 pack preview rendered. Windows Studio was visually inspected.
- Physical 1.8 TFT/FPS, cross-app latency, OTA reboot and endurance are
  **pending until this development image is installed**. Existing 1.7 runtime
  observations do not validate the new firmware. Windows EXE packaging was
  not performed in this iteration.

After installing: open two clients, change theme/FPS/style/page/favorites from
each, and confirm the other follows without reload. Test reconnect and old
HTTP clients. On TFT Clock/Midnight observe at least 60 consecutive seconds,
then record measured FPS and missed frames with both clients open. Compare
against the original firmware and use `DEVICE_DIAGNOSTICS.md` for longer runs.

## Preview-only handoff

The owner chose to keep the build and preview the UI; no OTA was performed.
The device remains on its installed v1.7 firmware.

```powershell
# Windows preview: no saved device settings, network scans or uploads.
./.venv/Scripts/python.exe windows_agent/companion.py --preview

# Browser preview: simulated data, loopback only; refresh reloads the UI source.
python firmware/tools/web_preview_server.py --port 8768
```

Open `http://127.0.0.1:8768/`. The browser mock cannot flash firmware or packs.
Windows preview controls change only local demo state; persistence, connection,
scan and update actions are disabled by the preview guard.
