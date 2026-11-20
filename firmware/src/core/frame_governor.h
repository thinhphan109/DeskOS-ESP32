#pragma once

#include <stdint.h>

// Conservative automatic FPS policy for a small SPI display.
// "Auto" is deliberately limited to 20/25 FPS; 30 FPS remains manual
// because a single good measurement cannot prove a sustained 30 FPS panel.
// Only pure time-window measurements enter this component (no Arduino APIs).
class FrameGovernor {
 public:
  enum Profile : uint8_t { STABLE20 = 0, BALANCED25 = 1, SMOOTH30 = 2, AUTO = 3 };

  void setProfile(uint8_t value) {
    if (value > AUTO) value = BALANCED25;
    profile_ = value;
    target_ = profile_ == STABLE20 ? 20 : profile_ == SMOOTH30 ? 30 : 25;
    slowWindows_ = 0;
    healthyWindows_ = 0;
  }

  uint8_t profile() const { return profile_; }
  uint8_t target() const { return target_; }

  // Called only for real 5-second animation windows when DMA is active.
  // Returns true if the effective target changed.
  bool observe(float fps, uint32_t missed, uint32_t worstRenderUs) {
    if (profile_ != AUTO) return false;
    const uint32_t budget = 1000000UL / target_;
    const bool overloaded = fps < target_ * 0.88f || missed >= 8 ||
                            worstRenderUs > budget * 2UL;
    const bool healthy = fps >= target_ * 0.97f && missed <= 1 &&
                         worstRenderUs <= budget * 8UL / 10UL;

    if (overloaded) {
      healthyWindows_ = 0;
      if (slowWindows_ < 3) ++slowWindows_;
      if (slowWindows_ >= 3 && target_ > 20) {
        target_ = 20;
        slowWindows_ = 0;
        return true;
      }
    } else if (healthy) {
      slowWindows_ = 0;
      if (healthyWindows_ < 6) ++healthyWindows_;
      if (healthyWindows_ >= 6 && target_ < 25) {
        target_ = 25;
        healthyWindows_ = 0;
        return true;
      }
    } else {
      slowWindows_ = 0;
      healthyWindows_ = 0;
    }
    return false;
  }

 private:
  uint8_t profile_ = BALANCED25;
  uint8_t target_ = 25;
  uint8_t slowWindows_ = 0;
  uint8_t healthyWindows_ = 0;
};
