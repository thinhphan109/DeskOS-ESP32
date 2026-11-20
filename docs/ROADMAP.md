# MèoDesk development roadmap

20 November 2026. The user's supplied v1.8–v3.0 roadmap is the direction;
version labels are planning milestones, not completed releases.

## Current baseline

- Physical device: owner-confirmed v1.7; read-only checks confirmed A/B OTA
  readiness, 16 MiB Flash, 8 MiB PSRAM and device-reported ~25 FPS history.
- Source/dev build: compact Personal Studio, shared WebSocket state,
  bounded Midnight clock refresh and initial Performance Lab instrumentation.
- v1.8 source checkpoint is committed/pushed as `v1.8.0-dev`. The v1.9
  configuration/manifest/Focus follow-up is documented in `CONFIG_AND_OTA_V19.md`.
- Owner authorizes direct Git push after each tested source version; deployment
  remains deferred until the larger software pass is reviewed.
- Deployment choice: keep the build and preview; no OTA authorized/performed.
- Music metadata/cover art/transport, PC metrics, Focus, motion priorities and
  FPS history already exist. Their later milestones mean refinement and
  acceptance, not reimplementation.

| Milestone | Next meaningful work | Acceptance gate |
|---|---|---|
| v1.8 Performance Lab / Studio | Separate render/bridge/wait costs; keep sync and UI compact; compare matched runs | Physical before/after traces and no unexplained visual/FPS regression |
| v1.9 OTA Guardian 2 | Build/version matching, update history, config backup/restore | Real update/reboot/restore exercises; preserve current recovery route |
| v2.0–2.1 UI Engine / Scene Studio | Extract one renderer seam at a time; bounded widgets; editor/export | Stable scene IDs, clipping bounds and compatible assets; profiling evidence |
| v2.2–2.3 Cat Life / themes | Authored expressive art, behavior continuity, theme tokens | Same main render budget and credible behavior with real/offline data |
| v2.4–2.6 Windows / Music / productivity | Refine SMTC/timeline, optional sensors, configurable Focus/reminders, EXE | Real provider behavior and clean-machine Windows packaging |
| v2.7–2.9 Context / rules / integrations | Explicit context inputs, bounded notifications, debounced rules, Widget SDK | Offline behavior, TTL/priority rules, opt-out and compatibility tests |
| v3.0 Platform | Multiple devices/displays, independent packs, import/export | Independently accepted adapters and recoverable updates |

## Constraints to preserve

LovyanGFX + internal DMA strips + 40 MHz SPI remain the baseline. Animation
does not depend on network availability. New measurements identify their
coverage and unsupported inputs remain unavailable. Features can be turned
off, and each hardware release needs measured before/after evidence.

Keep `.meocat` v1/v2 and the current asset A/B layout. More raw frames exceed
one bank; v3 requires a measured codec/cache strategy before changing Flash
geometry. Decoding belongs outside the critical drawing path. Beat detection
also needs a real audio input/analysis path; `playing:true` alone is not beat.

Security/eFuse/bootloader changes are deferred at the user's request. Keep the
existing local update workflow simple while measuring and debugging.

## v2.0 source checkpoint

Widget scene ID 14, bounded snapshot renderer and atomic layout/profile persistence are implemented. See [UI Engine](UI_ENGINE_V20.md). Drag-and-drop tooling is the next source milestone; physical acceptance remains deferred.

## v2.1 source checkpoint

Web and native Windows Scene Studio edit the same persisted layout with drag/snap, geometry/overlap validation, JSON portability and unsaved-draft protection. See [Scene Studio](SCENE_STUDIO_V21.md).
