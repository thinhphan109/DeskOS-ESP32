# UI Engine 2 — v2.0.0-dev

Scene ID 14 adds a configurable widget desk. Existing IDs 0–13 and their data sources remain valid. The cat still uses the existing LovyanGFX DMA strips at 40 MHz; widgets draw into the existing 162×120 card surface, refreshed on second boundaries. No full-screen framebuffer or framework migration is introduced.

`core/scene_layout.h` defines a fixed-capacity layout (1–6 widgets), independent of networking and LovyanGFX. `render/widget_renderer.h` consumes a data snapshot: clock, CPU/GPU/RAM, Focus, music, network and static text. Missing readings show N/A. Labels use the existing TFT ASCII font, limited to 31 printable characters. Rectangles must stay within the card and cannot overlap.

Paired clients GET or POST `/api/v1/scene-layout` using the existing X-MEO-KEY header. Example:

```json
{"schema":1,"widgets":[{"kind":0,"x":0,"y":0,"w":162,"h":120,"label":"Clock"}]}
```

Kinds: 0 Clock, 1 CPU, 2 GPU, 3 RAM, 4 Focus, 5 Music, 6 Network download, 7 Text. Minimum width 32 and height 20. POST validates the entire candidate and persists the layout together with the profile in one NVS value before replacing the active scene. It does not switch scenes automatically. Use the existing `page` control with value 14 to view it.

Full configuration backups include `scene_layout`; older backups without it preserve the current layout. Status advertises `widget_scene_v1` and `scene_layout_revision` so editors can detect remote changes. Python and C++ validators cover geometry, overlaps and labels.

This milestone provides the rendering and persistence contract. The drag-and-drop editor follows in v2.1. No firmware was deployed to the owner's board; physical FPS and TFT legibility remain hardware acceptance checks.
