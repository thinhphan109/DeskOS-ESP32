# Scene Studio — v2.1.0-dev

Both Web Personalize and Windows Studio now edit the same widget layout: select, drag with a 2-pixel snap, enter position/size, change kind/label, add/remove, and import/export JSON. The Web editor also supports arrow keys and a large-clock preset. Coordinates are in the TFT's 162×120 content area, not the full display. The cat keeps its own DMA-rendered region.

Labels use printable ASCII because this renderer uses the existing TFT bitmap font. A layout preview shows geometry and labels; it is not a pixel-exact mirror or a source of live sensor readings. Invalid geometry, overlap and unsupported labels disable Save. Dragging rejects invalid placements. Up to six widgets are allowed.

Save POSTs the complete validated layout, then selects page 14. Both clients observe `scene_layout_revision` and fetch updates through the existing paired HTTP endpoint. The normal shared state channel remains WebSocket with HTTP fallback. Remote changes never replace an unsaved local draft: Reload device explicitly replaces it. Read and write requests are bounded; disconnects leave the draft available for export. Concurrent valid writes use last successful save; there is no collaborative merge.

Preview mode does not connect or write to an ESP32. The local Web mock is labelled DEMO and cannot flash firmware or Cat Packs. Native editor validation and remote-draft preservation have automated coverage; Web contract tests check the same geometry/ASCII rules. Visual QA covers desktop and 390px Web, native Windows, mock save and overlap errors. Physical TFT verification remains pending deployment.
