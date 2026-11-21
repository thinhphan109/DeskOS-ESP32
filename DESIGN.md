# mimi.desk interface system

The owner selected a warm, compact and modern interface. Preserve the existing cat assets and practical desk-companion identity; the redesign changes the application shell, reading order and density.

## Surfaces

The firmware embeds a self-contained Web UI. The Windows app uses PySide6 widgets and a matching palette. Both are working applications: navigation and understandable state take priority over decoration. Source CSS lives in `firmware/tools/mimi_studio.css`; `embed_studio_style.py` copies it into the firmware header. Desktop styling and SVG navigation icons live in `windows_agent/ui_theme.py`.

## Palette and type

- Ground `#f6f5ef`, paper `#fffef9`, ink `#263e30`, secondary `#596b5e`.
- Soft green `#e1ebd4` for the companion, deep green `#294b3b` for emphasis/Music, muted bronze for GPU and Focus accents.
- Segoe UI/system sans on working screens, tabular numeric values. Body 13–14px, page headings 27–30px, Music title 28–44px.
- Borders provide separation; no decorative shadows, blurred surfaces or gradients. Sections use 12–14px radii, controls 8–9px.

## Navigation and density

Overview, Music, Focus, Studio and Device are primary destinations. Settings stays secondary in the desktop sidebar. Firmware is reached from Device. Web navigation becomes a horizontal scrolling row on narrow screens; Music and Focus have separate panels.

Content uses 16–24px internal spacing and 20px section gaps. Keep related fields together. The cat has one compact overview region rather than taking the whole viewport. Technical metrics are inside collapsed Diagnostics; CatPack and JSON rules remain available under advanced disclosures.

Desktop pages use a current-page size hint, not the tallest editor's height. Cards prefer their content height and do not consume spare vertical space. The Music page should not inherit Studio's scroll extent.

## States and language

Use clear Vietnamese for navigation, actions, empty states and normal status. Preserve arbitrary track names, user-entered labels and technical identifiers. DEMO remains conspicuous wherever data is simulated. Connection status is based on a recent device response, not a socket-open event; stale responses lose the connected badge after six seconds.

Keep protocol IDs, configuration paths, saved pairing credentials and NVS namespaces unchanged by the display-brand rename. Do not add selectable server-OTA options before the firmware implements them; their behavior is discussed separately in the OTA proposal.

## Accessibility and adaptation

Controls have focus states, native labels and keyboard operation. Authored SVG navigation icons share a 1.7px stroke; icons do not replace labels. Respect reduced motion on Web. Widget dragging retains the original keyboard and numerical alternatives. At 390px the Studio canvas scales to its container with no horizontal document overflow.

## Evidence and checks

The Web overview, Studio and a 390px breakpoint were visually inspected in the local mock. Native Overview and Music were visually inspected in DEMO. Changes to content-height policy also cover the Device layout. Use focused GUI construction/state checks, embedded-JS syntax/shared-state checks and a firmware build; do not default to the full suite per edit.

Server OTA and firmware deployment are separate from this visual release. See `docs/SERVER_OTA_PROPOSAL.md`.
