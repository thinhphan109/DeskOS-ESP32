# Music Studio — v2.4.0-dev

The existing Windows SMTC adapter, real metadata, optional 64×64 album art, and acknowledged previous/play-pause/next commands remain the data path. This release unifies timeline projection: the ESP32 exports position at response time rather than an older raw sample, and all renderers clamp to track duration. Paused tracks stay still and new samples/seek positions reset the anchor.

Web and native Windows repaint the timeline between received samples, using a short local interpolation window. After 3.5 seconds without a new update, they show an interrupted-sync label and stop advancing. ESP media still expires at 10 seconds; source age is included in responses so repeatedly forwarded old samples cannot extend their lifetime. TFT time uses the same domain helper and handles millis rollover. Unsupported duration stays N/A; no audio beat is inferred from playing state.

Tests cover playing, pause, seek reset, end clamping, stale source/connection and counter rollover. Actual SMTC accuracy and command support depend on the player and remain a real-player acceptance check. No firmware deployment occurred.
