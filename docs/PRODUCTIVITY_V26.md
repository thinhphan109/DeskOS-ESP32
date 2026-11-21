# Productivity Desk — v2.6.0-dev

The configurable ESP Focus timer remains authoritative for work/break/goal and keeps running when Companion closes. A new per-boot completion counter is not reset by ordinary Focus resets or plan changes. It allows clients to distinguish an actual completed work interval from a skipped/reset session.

Native Focus now shows a local daily journal: completed sessions and minutes observed while Companion maintained a fresh connection. SQLite lives next to the user's Companion preferences, outside the repository. The first sample establishes a baseline. Repeated samples, reboots, resets, counter jumps and gaps ≥3.5 seconds do not create backfilled sessions. A unique device/boot/counter key prevents duplicate writes by two windows. Statistics are explicitly local observations, not a complete history of the ESP while the computer was off.

Optional tray break reminders are off by default, saved in local preferences, and shown only on an observed completion; quiet mode suppresses them. Preview mode does not write a journal/preferences or raise real completion notifications.

Windows Settings provides an explicit per-user startup checkbox and Apply action. Opening the app or preview never changes startup. Apply adds/removes only the MeoDeskCompanion HKCU Run value and uses quoted source/frozen launch arguments with `--minimized`. Tests use a mocked registry; development did not enable startup on the owner's PC. Existing GitHub Windows packaging builds the native application and assets into the distributable ZIP.

This milestone does not add a calendar account, read third-party notifications or infer unobserved PC usage. Device Focus controls stay synchronized through the shared state channel; the journal and startup preference are Windows-only utilities. Real long-session/notification and clean-machine EXE acceptance remain before deployment.

The GitHub Actions artifact for commit ec9c297 (run 44855050507) was downloaded and opened as an independent EXE with --preview. Assets loaded and a local theme change worked. This validates packaging on the development PC; it is not a clean-machine or hardware deployment test.
