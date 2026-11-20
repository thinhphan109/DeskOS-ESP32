# Dashboard Themes — v2.3.0-dev

Six TFT choices are available from Web Personalize and native Windows Studio: 0 Cozy Day, 1 Midnight, 2 Auto, 3 Cyberpunk, 4 Sakura, 5 Minimal. Existing IDs and Auto's local NTP schedule remain compatible. Minimal selects the Clock scene when chosen through the theme control; other scene controls remain available. Theme choice is persisted and included in configuration backups.

`core/theme_palette.h` owns the RGB565 palette without depending on the display or network. Cyberpunk uses a dark blue surface with mint/magenta accents; Sakura uses pale pink and plum; Minimal uses a quiet neutral surface. Existing render components consume the palette. This is an extension of the current card layouts, not an independently animated theme pack or a completely different full-screen composition.

The sprite background's exact ivory key is remapped to the selected background, including light themes. The original memcpy path remains available for the original ivory palette. Flattened anti-aliased sprite edges can retain a light fringe; they are not treated as transparent. TFT readability, color appearance and matched FPS comparisons remain physical acceptance checks. No SPI/Flash geometry or DMA buffer sizes changed.
