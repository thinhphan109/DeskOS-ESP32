# Building a source checkout

This public repository contains source, example configuration and sprite packs.
Owner-specific configuration, firmware binaries and historical archives are
kept locally and excluded from Git; they can embed Wi-Fi and pairing values.

Before compiling a new checkout, copy
`firmware/src/app_config.example.h` to `firmware/src/app_config.h` and set local
values. An empty router SSID lets the fallback AP provide first-time setup.
Do not commit that local header. Then run PlatformIO in `firmware/`.

Windows: install `windows_agent/requirements-gui.txt` in a virtual environment.
`python windows_agent/companion.py --preview` opens local DEMO controls without
connecting to or updating hardware. `python firmware/tools/web_preview_server.py`
serves the simulated browser preview on loopback port 8765.

Source tags describe tested software milestones. Physical validation and
deployment status are recorded separately in the verification documents.
