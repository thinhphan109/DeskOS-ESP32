# Widget and Integration SDK — v2.9.0-dev

Three expiring external-data slots extend the existing widget scene. Choose **Data 1 / Data 2 / Data 3** in Scene Studio (kind IDs 8/9/10); they read API slots 0/1/2. Existing widget IDs 0–7 and page IDs 0–14 stay valid. Set the widget's label and size in the normal layout editor.

Paired `POST /api/v1/feeds/{0|1|2}` accepts:

```json
{"name":"Room","value":"22.5","unit":"C","ttl_s":30}
```

Name: 1–16 printable ASCII characters; value: 1–20; unit: 0–8; TTL: 1–300 seconds. Values are formatted by their provider, not treated as executable code. `GET` on the same paired endpoint reports freshness and age, returning null value/unit after expiry. TFT widgets show N/A when missing/expired. Feed values live only in fixed-size RAM slots, never NVS or portable configuration backups. A reboot clears them. Post at about 1 Hz or less and refresh before the chosen TTL. The renderer does not fetch external services or block on provider I/O.

The stdlib-only Python `IntegrationClient` sends feeds and selected notifications through the existing local pairing protocol. It disables environment proxies and redirects, bounds responses and supports a custom HTTP port for local preview. No Qt, SMTC or sensor packages are required. Example in an integration's own Python process:

```python
import os
from integration_client import IntegrationClient
client = IntegrationClient("192.168.0.107", os.environ["MEO_API_KEY"])
# Illustrative value: replace with an actual provider reading.
client.feed(0, "22.5", name="Room", unit="C", ttl=30)
client.notify("Reminder", hidden=True)
```

CLI preview without any network request:

```powershell
python windows_agent/integration_client.py --host 127.0.0.1 --dry-run feed --slot 0 --name Room --value 22.5 --unit C
```

For a real send, set MEO_API_KEY in the process environment and omit --dry-run. For the local mock use --host 127.0.0.1 --port 8776; its replies are labelled DEMO. Hidden notification text is replaced before transmission. No integration is connected to a calendar, Home Assistant, weather or cloud account automatically.

Tests cover payload bounds, privacy, HTTP headers/path, redirect refusal, dry-run isolation, expiry, millis rollover, transient-data exclusion from backup and old/new widget kind IDs. Physical TFT readability and rendering cost remain acceptance checks. These are integration/display contracts for the current screen; simultaneous multi-device control and new screen drivers remain the v3.0 hardware/platform phase.
