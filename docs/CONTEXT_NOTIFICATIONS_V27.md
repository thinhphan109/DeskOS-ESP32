# Context and notifications — v2.7.0-dev

Real idle/wake/morning context from Cat Life continues to use fresh non-DEMO session data and valid local time. This milestone adds paired `POST /api/v1/notifications` for selected integrations and test controls in Web Device and native Settings. It does not read all Windows notifications or grant access to other apps.

```json
{"source":"companion","text":"Desk reminder","hidden":true,"ttl_s":8,"priority":1}
```

Source is 1–20 lowercase ASCII letters/digits/underscore/hyphen. Visible text is 1–60 printable ASCII characters; hidden content is replaced with `New notification`. First-party clients remove hidden text before transmission. TTL is 1–30 seconds; priority is 0–2, below critical hardware alerts. Quiet mode accepts but suppresses external notices. A two-second global ingress limit returns HTTP 429 on a faster request. Existing fixed-size event storage, expiry and priority selection are reused; there are no unbounded queues or file writes per notice.

The active event can appear in a small 160×9 strip above the TFT content card, refreshed only when its sequence changes or expires. The 26-character caption does not overlap the cat's DMA region. Longer text stays available in the existing Web event timeline. This extra direct text/shapes drawing is outside Performance Lab's tracked bulk-pixel payload, as documented for that metric.

Tests cover privacy stripping, text/source/TTL bounds, critical-priority exclusion and rate-limit rollover. The mock validates requests and reports DEMO; it does not simulate a physical TFT. Actual appearance, quiet/priority interaction and FPS remain hardware acceptance checks.
