#pragma once
#include <stdint.h>

// Geometry/cadence contract independent of LovyanGFX, JSON and sensor providers.
struct WidgetRegion {
  int16_t x, y;
  uint16_t width, height;
  bool fits(uint16_t surfaceWidth, uint16_t surfaceHeight) const {
    return x >= 0 && y >= 0 && width > 0 && height > 0 &&
           static_cast<uint32_t>(x) + width <= surfaceWidth &&
           static_cast<uint32_t>(y) + height <= surfaceHeight;
  }
};
struct WidgetSpec {
  WidgetRegion region;
  uint32_t refreshMs;
  bool alignSeconds;
};
struct RenderSurfaceInfo {
  uint16_t width, height;
  uint8_t bitsPerPixel;
  bool dma;
  bool accepts(const WidgetRegion &region) const {
    return bitsPerPixel == 16 && region.fits(width, height);
  }
};
