# Automation Rules — v2.8.0-dev

Paired `GET/POST /api/v1/rules` edits up to six rules. The empty default disables automation. Web Personalize and native Studio provide collapsed advanced JSON editors with presets, import/export, validation and explicit Save. `rules_revision` drives remote reloads; unsaved drafts are preserved. No shell commands or arbitrary scripts are supported.

```json
{"schema":1,"rules":[{"enabled":true,"when":"cpu_above","threshold":90,"hold_s":30,"cooldown_s":300,"do":"notify","value":0,"label":"CPU busy"}]}
```

Conditions: `cpu_above`/`gpu_above` (threshold 1–100 percent, matched at or above), `idle_above` (1–7200 seconds), `hour` (0–23 local hour), `focus_complete`, `music_playing`. The last two use threshold 0; Focus completion requires hold 0. Hold is 0–600 seconds. Cooldown is 5–86400 seconds. CPU/GPU/idle/music require a fresh non-DEMO PC source. Hour requires valid local NTP date/time. Focus completion establishes a baseline on engine start and does not replay old completions.

Actions: `scene` (value 0–14), `theme` (0–5), `celebrate`/`notify` (value 0). Labels contain 1–32 printable ASCII characters. Notify uses normal-priority events with an eight-second TTL; quiet mode suppresses notify/celebrate. Critical cat health priorities remain in the existing behavior engine.

Conditions must hold continuously; stale/missing data resets the dwell. A condition fires once until it becomes false, and cooldown still applies after rearming. Hour rules fire once per observed local day during the matching hour, including a first observation after xx:00. Rules are evaluated at 1 Hz in list order; later simultaneous scene/theme actions win. Editing/restoring rules or rebooting resets volatile dwell/cooldown/day state. The engine uses fixed memory and handles millis rollover.

Rules are serialized in the same canonical NVS value as profile/layout. Full restore validates all three candidates before one write, then replaces active state. Older backups without rules preserve current rules; older stored profiles load the empty default. The configuration endpoint now accepts compact bodies up to 4096 bytes; portable JSON files remain limited to 8 KiB.

Tests cover dwell, latching, cooldown, real/DEMO separation, offline reset, day timing, completion baseline, quiet mode, rollover, bounds, portable backup and draft preservation. The mock validates/stores rule definitions for UI QA; it does not claim to execute ESP rules or measure hardware performance. Physical trigger/action/FPS acceptance remains after deployment approval.
