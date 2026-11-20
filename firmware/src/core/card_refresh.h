#pragma once
#include <stdint.h>

// Cooperative card rendering: clocks have a wall-second cadence, and a busy
// cat renderer may defer a due card by at most 80 ms instead of a full period.
struct CardRefresh {
  static bool due(uint32_t nowMs, uint32_t lastMs, uint32_t intervalMs,
                  bool clockPage, bool animationActive, uint32_t spareUs) {
    const uint32_t elapsed = nowMs - lastMs;
    const bool secondChanged = nowMs / 1000UL != lastMs / 1000UL;
    if (clockPage ? !secondChanged : elapsed < intervalMs) return false;
    const bool deadline = clockPage ? nowMs % 1000UL >= 80UL : elapsed >= intervalMs + 80UL;
    return !animationActive || deadline || spareUs > 14000UL;
  }
};
