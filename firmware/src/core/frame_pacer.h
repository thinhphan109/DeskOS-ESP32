#pragma once

#include <stdint.h>

// A clock-based, rollover-safe animation deadline scheduler.
// Never delay the cooperative Wi-Fi loop to meet a target FPS; if we're late,
// skip stale frames and render the *current* pose instead of falling behind.
class FramePacer {
 public:
  explicit FramePacer(uint8_t fps = 25) { setFps(fps); }

  void setFps(uint8_t fps) {
    if (fps < 1) fps = 1;
    if (fps > 60) fps = 60;
    fps_ = fps;
    periodUs_ = 1000000UL / fps_;
    missed_ = 0;
    armed_ = false;
  }

  uint8_t fps() const { return fps_; }
  uint32_t frameBudgetUs() const { return periodUs_; }

  void rebase(uint32_t nowUs) {
    nextUs_ = nowUs + periodUs_;
    armed_ = true;
  }

  bool due(uint32_t nowUs) {
    if (!armed_) { rebase(nowUs); return false; }
    if ((int32_t)(nowUs - nextUs_) < 0) return false;
    const uint32_t late = nowUs - nextUs_;
    const uint32_t skipped = late / periodUs_;
    missed_ += skipped;
    nextUs_ += (skipped + 1UL) * periodUs_;
    return true;
  }

  uint32_t untilNextUs(uint32_t nowUs) const {
    if (!armed_) return periodUs_;
    const int32_t remaining = (int32_t)(nextUs_ - nowUs);
    return remaining > 0 ? (uint32_t)remaining : 0UL;
  }

  uint32_t takeMissed() {
    const uint32_t result = missed_;
    missed_ = 0;
    return result;
  }

 private:
  uint32_t nextUs_ = 0;
  uint32_t periodUs_ = 40000;
  uint32_t missed_ = 0;
  uint8_t fps_ = 25;
  bool armed_ = false;
};
