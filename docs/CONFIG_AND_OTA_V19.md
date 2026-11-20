# Configuration and OTA Guardian follow-up — 1.9.0-dev

Source/build milestone. No OTA, live settings change or partition migration
was performed. The owner's hardware remains at the previously observed v1.7.

## Device preferences

`GET /api/v1/config` exports an allowlisted schema-1 envelope with board,
firmware version and profile. `POST` validates every field and all four unique
scene IDs before changing anything. Both use the existing pairing header.

The profile covers theme, FPS, motion style/on/off, Auto pages, favorite scenes,
alert thresholds/Quiet and Focus work/break lengths/session goal. Router
passwords and pairing keys are deliberately excluded. Sessions/history/assets
are not copied as device preferences.

Restore writes the complete profile as one NVS value (`profile_v1`) before
applying it. Invalid content or failed persistence leaves the previous runtime
profile in place. Older individual preference keys are migrated when no valid
canonical profile exists. Normal controls still update their legacy keys where
applicable; older firmware does not understand the canonical restore record.

Windows Settings and Web Device & updates provide export/restore controls.
The mock validates the same JSON constraints but changes DEMO memory only.
The Windows preview guard blocks all real device/persistence operations.

## Configurable Focus

`POST /api/v1/focus/config` accepts integer `work_min` (1–180), `break_min`
(1–60), and `goal` (1–24). Duration changes reset the current session; changing
only its goal preserves the running timer. ESP32 owns countdown and phases.
Status adds `focus_work_min`, `focus_break_min`, `focus_goal`, and
`focus_duration_s`. Legacy 25/5 defaults and old clients remain valid.

Both interfaces expose the plan and use the actual duration for progress.
The goal is a session target; daily journal aggregation belongs to the
productivity milestone, not this counter. Favorite scene swaps during restore
use a batch validator rather than partially applying one slot at a time.

## OTA diagnostics

Windows checks an adjacent `.manifest.json` when present: board, exact size,
SHA-256, filename and reported target version must match the chosen image.
Bare `.bin` files remain supported for local debugging. A manifest-backed
update is confirmed only when the expected slot and expected firmware version
report successful startup health. Existing upload SHA checks remain unchanged.

`GET /api/v1/ota/history` returns up to six best-effort NVS journal entries.
Successful uploads record the selected slot/hash/version-before. A later boot
can report `confirmed`, `failed_health` or `unconfirmed_slot`. The latter does
not prove automatic rollback. A first update from v1.7 has no prior journal;
the new firmware cannot reconstruct older updates. A journal-write failure
does not determine firmware installation success; live boot verification does.

Web and Windows can read this history. No signed-update, eFuse, bootloader or
Internet exposure changes were introduced.

## Validation

81 Python tests passed, including JSON profile round trips/rejection, adjacent
manifest mismatch, wrong-version boot rejection, GUI construction and mock
HTTP config/Focus requests. Native profile/Focus/atomic scene-swap tests passed.
The existing browser suites pass. The 50/10/6 plan was exercised in the
rendered local Web preview. Full PlatformIO build is checked before tagging.
Physical NVS failure/reboot/OTA validation remains a deployment gate.
