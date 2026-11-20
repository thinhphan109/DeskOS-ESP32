# MèoDesk HTTP/JSON v1 · Compatibility contract (through v1.7)

### Living Motion v1.7 additions

- Authenticated `POST /api/control` accepts `{"action":"motion_style","value":0|1|2}`;
  the bounded preference is persisted in the existing NVS namespace.
- `/api/v1/status` optionally reports `motion_style`, `motion_state`,
  `motion_cel`, `motion_blink`, `motion_budget_limited`, `motion_pc_intensity`.
  `motion_pc_intensity` is zero when Windows telemetry is stale or demo;
  consumers should consult existing `pc_connected` and `demo` flags.
- Device capability list adds `motion_director_v1`; old clients ignore it.
  `/api/telemetry`, `/api/control` existing actions, Now Playing, Focus,
  Firmware OTA, Cat Pack OTA, partition geometry and all old page IDs are
  unchanged. Legacy Windows clients need not poll the new optional fields.

---

### Motion Pack Studio v1.6 additions

- No change to existing telemetry, Now Playing, Focus, OTA Guardian or v1
  Cat Pack endpoints. `POST /api/v1/catpack/upload` additionally accepts
  `X-MEO-CAT-SIZE:1990656` for 48-frame RGB565 v2 collections.
- `/api/v1/catpack/status` adds optional `accepted_pack_bytes`,
  `frames_per_pose`, `active_pack_bytes`, and `motion_ready`. Retains
  `pack_bytes:663552` for older clients. Old v1 packs remain accepted.
- `GET /api/status` adds optional `cat_motion_frames` and the capability
  `catpack_motion_v2` if adequate PSRAM is available. Older agents ignore
  optional fields.
- A bank header's `format:1` is the 16-frame legacy atlas; `format:2` is a
  48-frame atlas. Both use existing commit magic, fixed-bank layout and
  SHA-256 verification. No filesystem mount, new partition or GPIO changes.

---

### Cat Pack Studio v1.5 additions

- No changes to existing HTTP/JSON metrics, Focus, Media or OTA Guardian.
- `GET /api/v1/catpack/status`: fixed `frame_count:16`, `width:144`,
  `height:144`, `pack_bytes:663552`, `ready`, `source` (builtin/custom),
  `active_bank`, `generation`, `sha256`, `phase`.
- Authenticated multipart `POST /api/v1/catpack/upload` needs raw `.meocat`
  and `X-MEO-CAT-SIZE:663552`, `X-MEO-CAT-SHA256`, `X-MEO-KEY`. The response
  verifies payload and schedules a single reboot; clients confirm active
  digest after reboot. No firmware slot is overwritten.
- `GET /api/status` adds optional `cat_pack_loaded` and
  `cat_pack_generation`. Old Windows agents ignore both fields.
- The reserved data/spiffs partition is treated as **raw A/B banks** and
  MUST NOT be simultaneously mounted as a filesystem by another feature.

---

### OTA Guardian v1.4 additions

- No breaking changes to existing HTTP/JSON v1 and PC monitor routes.
- `GET /api/v1/ota/status` adds `uptime_ms` and `boot_health`, whose value
  is `checking` before the 25 s self-test, then `passed` or `failed`.
  This indicates application startup health, **not guaranteed bootloader rollback**.
- `POST /api/v1/ota/upload` keeps the v1.3 multipart/headers and `verified`
  response. Firmware upload success must never be equated with a verified boot.
  New clients save the old `inactive_slot`, then wait up to 85 s for that exact
  `running_slot` and `boot_health:passed`. An automatic rollback may depend on
  bootloader features which are not enabled in every Arduino build.
- `app0` and `app1` each remain 6 MiB on 16 MB Flash. No second partition
  migration is needed when upgrading from v1.3 A/B to v1.4.

### Local OTA v1.3 additions

- Introduced `GET /api/v1/ota/status` and pairing-authenticated multipart
  `POST /api/v1/ota/upload`. The image is ESP32-S3 `.bin`, within the
  inactive 6 MiB slot, with `X-MEO-OTA-BOARD`, `X-MEO-OTA-SIZE` and
  `X-MEO-OTA-SHA256` headers. This is unsigned HTTP on a trusted LAN only.

---

### N16R8 Performance v1.1 additions

- No breaking change to `schema: 1`, page IDs or existing authenticated
  endpoints. The `POST /api/control` action `{"action":"fps","value":0|1|2}`
  selects a persistent animation target of 20/25/30 FPS respectively. Only
  authenticated requests can change it.
- `GET /api/status` and `/api/v1/status` optionally expose `fps_profile`,
  `fps_target`, `cat_render_max_ms`, `cat_frames_skipped`, `psram_ready`,
  `psram_free_kb`, `internal_ram_kb` and `internal_dma_kb`. The measured
  `cat_fps` is distinct from the configured target; **target is not a promise**.
- `psram_bytes` and `psram_free_bytes` are real device memory measurements,
  not synthetic values. Older Windows agents can ignore every new field.
- Capability strings add `n16r8_psram_memory`, `fps_profiles_v1`.

---

### Living Companion v1.0 additions

- The previous `schema:1` HTTP/JSON fields and all legacy page IDs remain.
- Media POST may include optional `artwork_id` (16 lowercase hex chars, or an
  empty string). Older firmware ignores this extra field. Old agents need
  not send it and continue displaying text-only media on v1.0 firmware.
- The new `POST /api/v1/artwork` sends exactly one 64×64 RGB565 **little-endian**
  image (8,192 bytes) as a `multipart/form-data` file with `X-MEO-KEY` and
  `X-MEO-ART-ID` headers. The ID must equal the current media track ID. It
  is not a password and never authorizes control by itself.
- `GET /api/v1/artwork.bmp` returns a transient, authenticated BMP24 cover
  (HTTP 404 if unavailable); this endpoint is for Web Studio only.
- `POST /api/control` accepts `{"action":"pet"}` or
  `{"action":"scene_0", "value":N}` through `scene_3` with page index N
  in 0..13. Duplicate favorite pages are rejected. NVS stores four byte-sized
  scene favorites; auto-rotate cycles only these four.
- Public `GET /api/status` advertises `scene_slots`, `cat_persona`,
  `cat_energy` (virtual 0..100; **not** a physical sensor), and `artwork_ready`.
- v1.0 Companion can safely talk to v0.8/v0.9 firmware: missing artwork
  endpoints yield optional-feature errors and do not stop telemetry or music.

---

Both sides keep legacy endpoints while adding namespaced v1 endpoints.

| Capability | v0.7.1 | v0.8.0 | v0.9.0 | v1.0.0 source preview |
|---|---|---|---|---|
| `GET /api/status` / `POST /api/telemetry` | Yes | Compatible | + net/VRAM | + scene/pet fields |
| `POST /api/control` (page, preview, focus, motion) | Yes | Yes | + theme, focus_skip | + pet, scene_0..3 |
| `GET /api/history` / settings / cat BMP | Yes | Yes | Yes | Retained |
| `GET /api/v1/status` | No | Yes | Yes | Retained |
| `POST /api/media` / `POST /api/v1/media` | No | Yes | Yes | + optional artwork_id |
| `GET /api/media` / `GET /api/v1/media` | No | Yes; pairing key | Yes | + artwork_ready |
| `GET /api/v1/commands` + `POST .../ack` | No | Yes; 8 s TTL | Yes | Retained |
| `POST /api/v1/wifi/config` | No | Yes; NVS | Yes | Retained |
| `GET/POST /api/v1/focus` | No | No | Yes | Retained |
| `GET /api/v1/events` | No | No | Yes | Retained |
| `POST /api/v1/artwork` / `GET /api/v1/artwork.bmp` | No | No | No | Yes; pairing key |

**Older Windows agent → new firmware:** unchanged. New Python Companion → old firmware:
telemetry works; new music/command endpoints return 404 and are ignored. Features
are advertised in optional `capabilities` of `/api/status`; old clients ignore them.

`X-MEO-KEY` is mandatory on all mutation endpoints. Values may be absent (`null`)
when sensors are unavailable. Never convert GPU=0 to missing; never pass simulated
values without explicitly setting `demo:true`.

### v0.9 optional telemetry fields

`net_up_kbps` and `net_down_kbps` are aggregate actual network send/receive
rates in **KiB/s** (1024 bytes per unit). `vram_used_gb` and
`vram_total_gb` are NVIDIA NVML metrics in **GiB** (1024³ bytes per unit).
Fields can be `null` or omitted for older agents/unsupported hardware. The
ESP32 validates them without rejecting legacy payloads. Do not confuse 0
(a valid zero load) with `null` (unavailable). New Companion gracefully skips
Focus polling when connected to older firmware (404).

`GET /api/v1/focus` is public read-only; `POST` requires the pairing key and
accepts `{"action":"toggle"}`, `{"action":"reset"}` or
`{"action":"skip"}`. Focus state is held by the ESP32 and resets when the
ESP32 restarts. `GET /api/v1/events` exposes a bounded recent timeline with
sequence, priority (0–3) and TTL, and requires the key. Web Studio's themes
use the backward-compatible control endpoint with values 0 Day / 1 Midnight
/ 2 Auto, stored in NVS.

### Media command state

1. Web posts `{ "action": "media_play_pause" }` to `/api/control` (key needed).
2. ESP returns HTTP 202 `{ "ok":true, "pending":true, "seq": N }`.
3. Windows polls `/api/v1/commands`, executes **once per sequence**, and posts
   `{ "seq": N, "ok": true|false }` to `/api/v1/commands/ack`.
4. Web observes `command_pending/command_acknowledged/command_ok` in `/api/status`.
5. After 8 seconds, command expires without execution; no backlog is replayed.

**Security:** HTTP is LAN-only; this is a pairing token, not end-to-end TLS.
Do not expose ESP32 port 80 to the public Internet. Wi-Fi password is never
returned by any API; NVS credentials remain local to the device.
# Development extension: Personal Studio state stream

Firmware 1.8.0-dev optionally advertises `state_websocket_v1` and
`state_ws_port:81`. Existing HTTP/JSON schema 1 endpoints and writes remain.
WebSocket state packets add `type:state`, `state_boot_id`, `state_revision`
and, after subscription with the existing pairing key, a `media` object.
Status clients may ignore these additive fields. See
`../docs/PERSONAL_STUDIO_V18.md` for cadence, ordering and fallback behavior.

# Development Performance Lab fields

Capability `performance_lab_v1` adds optional `lab_*` status fields and keyed
control action `perf_trace` (0/1). Unknown fields are ignored by old clients.
Missing/disabled measurements are null. See `../docs/PERFORMANCE_LAB_V18.md`
for the meaning of CPU/wait spans and the partial pixel-payload coverage.

# 1.9 development additions

`device_profile_v1` provides keyed GET/POST `/api/v1/config`; `focus_config_v1`
provides keyed POST `/api/v1/focus/config`. Status adds duration/plan/goal
fields. Keyed GET `/api/v1/ota/history` reports a bounded, best-effort device
journal. Legacy clients ignore the added fields and retain 25/5 defaults.
See `../docs/CONFIG_AND_OTA_V19.md` for reset, migration and journal limits.

