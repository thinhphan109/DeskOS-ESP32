"""Convert the sixteen unique 144x144 RGBA cat PNGs into flash RGB565 arrays.

This is a mechanical pixel-format conversion, not AI image generation.
Alpha is composited onto the same background used by the display UI.
Run: python3 tools/pack_assets.py
Dependency: Pillow
"""

from pathlib import Path
from PIL import Image


ROOT = Path(__file__).resolve().parent.parent
NAMES = ("boot", "clock", "paused", "sleep", "storage",
         "network", "offline", "music", "bambu", "audio",
         "health", "healthy", "hotspot", "power", "rescue", "shutdown")
BG = (255, 248, 241, 255)  # v0.2 warm ivory theme


def rgb565(r, g, b):
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def main():
    header_lines = [
        "#pragma once",
        "#include <Arduino.h>",
        "// Generated from original 144x144 RGBA cat sprites.",
    ]
    source_lines = [
        '#include "cat_assets.h"',
        "// Each array is stored in flash, never copied into a full RAM framebuffer.",
    ]
    for name in NAMES:
        input_path = ROOT / "images" / f"{name}.png"
        with Image.open(input_path) as im:
            image = im.convert("RGBA")
        if image.size != (144, 144):
            raise ValueError(f"{input_path} must be 144x144, got {image.size}")
        canvas = Image.new("RGBA", image.size, BG)
        canvas.alpha_composite(image)
        rgba = canvas.load()  # Compatible with older Pillow releases, too.
        pixels = [rgb565(*rgba[x, y][:3])
                  for y in range(144) for x in range(144)]
        header_lines.append(f"extern const uint16_t cat_{name}[20736] PROGMEM;")
        source_lines.append(f"const uint16_t cat_{name}[20736] PROGMEM = {{")
        for i in range(0, len(pixels), 16):
            source_lines.append("  " + ", ".join(f"0x{v:04X}" for v in pixels[i:i + 16]) + ",")
        source_lines.append("};")
    (ROOT / "src" / "cat_assets.h").write_text("\n".join(header_lines) + "\n")
    (ROOT / "src" / "cat_assets.cpp").write_text("\n".join(source_lines) + "\n")
    print(f"Packed {len(NAMES)} cat images: {len(NAMES) * 144 * 144 * 2:,} bytes RGB565")


if __name__ == "__main__":
    main()
