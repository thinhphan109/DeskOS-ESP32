# MèoDesk development roadmap

21 November 2026. The user's supplied v1.8–v3.0 roadmap is the direction;
version labels are planning milestones, not completed releases.

See [current development status](DEVELOPMENT_STATUS.md) for implemented scope,
validation evidence and remaining acceptance work.

## Current baseline

- Physical device: read-only check on 21 November reports 2.4.0-dev, healthy
  app0 and OTA-ready app1 (6 MiB). Earlier v1.7 checks recorded N16R8 memory
  and approximately 25 FPS history. No newer dev milestone was deployed in this pass.
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

## v2.2 source checkpoint

Real Windows session idle input drives bounded sleep/wake and once-daily morning greetings. Focus, media, quiet and critical priorities remain. See [Cat Life](CAT_LIFE_V22.md). New authored animation and compressed pack formats remain separate work.

## v2.3 source checkpoint

Cozy, Midnight and Auto keep IDs 0/1/2. Cyberpunk, Sakura and Minimal add IDs 3/4/5 across TFT/Web/Windows and portable profiles. Minimal selects Clock when chosen through its control. See [Themes](THEMES_V23.md).

## v2.4 source checkpoint

Music position projection now shares duration/stale rules across TFT/Web/Windows. Existing real SMTC metadata, cover and transport stay in use. See [Music Studio](MUSIC_STUDIO_V24.md). Player-specific acceptance remains pending.

## v2.5 source checkpoint

Native Windows Overview has bounded CPU/GPU/RAM history with explicit gaps, source isolation and DEMO labelling. See [PC Monitor](PC_MONITOR_V25.md).

## v2.6 source checkpoint

Observed Focus completions, local daily journal, opt-in break reminders and explicit Windows startup settings are implemented. See [Productivity Desk](PRODUCTIVITY_V26.md). Calendar account access and complete PC usage accounting remain separate integrations.

## v2.7 source checkpoint

Bounded notifications, private-by-default first-party payloads, quiet/priority/TTL policy and a small TFT event strip are implemented. See [Context and notifications](CONTEXT_NOTIFICATIONS_V27.md). OS notification capture remains an optional integration.

## v2.8 source checkpoint

Six bounded rules, dwell/cooldown/latch semantics, atomic profile backup and synchronized advanced JSON editors are implemented. See [Automation](AUTOMATION_V28.md).

## v2.9 source checkpoint

Three expiring external-data widgets, paired feed endpoints and a stdlib Python/CLI integration client are implemented. See [Widget and Integration SDK](INTEGRATION_SDK_V29.md). The current board remains on its deployed firmware until the owner approves the later deployment phase.

## v2.10 source checkpoint and production planning

The Music scene uses the full TFT content width, a 120px album tile and embedded Noto Sans glyphs for NFC Vietnamese. Metadata redraws only when changed; the small timeline updates separately. The 64px artwork transport remains compatible. Firmware deployment and physical acceptance are still deferred.

The [desktop and OTA plan](PRODUCTION_PLAN.md) proposes product naming, user-facing messages, connection lifecycle, packaging and a staged OTA experience. Branding and stable-release status are not changed by this plan.
