#pragma once

#include <stdint.h>

// Domain model: no Arduino, Wi-Fi, JSON or LovyanGFX dependencies.
struct MediaState {
  bool received = false;
  bool available = false;
  bool playing = false;
  uint32_t receivedMs = 0;
  uint32_t positionMs = 0;
  uint32_t durationMs = 0;
  char title[65] = "";
  char artist[49] = "";
  char artworkId[17] = ""; // Optional content key for a 64x64 cover image.

  bool fresh(uint32_t now) const {
    return received && (uint32_t)(now - receivedMs) < 10000UL;
  }
  bool playingNow(uint32_t now) const { return fresh(now) && available && playing; }
  uint32_t positionAt(uint32_t now) const {
    if(!fresh(now)||!available) return 0;
    const uint64_t estimate=static_cast<uint64_t>(positionMs)+(playing ? static_cast<uint32_t>(now-receivedMs) : 0);
    const uint32_t limit=durationMs ? durationMs : 86400000UL;
    return estimate>limit ? limit : static_cast<uint32_t>(estimate);
  }
};
