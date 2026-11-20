#pragma once
#include <stdint.h>

// Optional instrumentation only; fixed counters, no allocation or I/O.
class PerformanceLab {
 public:
  enum Stage { CAT_CPU, DMA_SUBMIT, DMA_WAIT, CAT_WALL, CARD_WALL, NETWORK_WALL, STAGES };
  struct Window {
    uint64_t totalUs[STAGES]{};
    uint32_t maxUs[STAGES]{};
    uint32_t calls[STAGES]{};
    uint64_t payloadBytes = 0;
    uint32_t periodMs = 0;
  };
  void setEnabled(bool enabled, uint32_t nowMs) {
    enabled_ = enabled;
    current_ = Window{};
    last_ = Window{};
    startedMs_ = nowMs;
    valid_ = false;
  }
  bool enabled() const { return enabled_; }
  bool valid() const { return enabled_ && valid_; }
  void add(Stage stage, uint32_t elapsedUs) {
    if (!enabled_ || stage >= STAGES) return;
    current_.totalUs[stage] += elapsedUs;
    ++current_.calls[stage];
    if (elapsedUs > current_.maxUs[stage]) current_.maxUs[stage] = elapsedUs;
  }
  void pixels(uint32_t width, uint32_t height) {
    if (enabled_) current_.payloadBytes += static_cast<uint64_t>(width) * height * 2;
  }
  bool finishWindow(uint32_t nowMs) {
    const uint32_t elapsed = nowMs - startedMs_;
    if (!enabled_ || elapsed < 5000UL) return false;
    current_.periodMs = elapsed;
    last_ = current_;
    current_ = Window{};
    startedMs_ = nowMs;
    valid_ = true;
    return true;
  }
  const Window &last() const { return last_; }
  double meanMs(Stage stage) const {
    return valid() && last_.calls[stage] ?
      static_cast<double>(last_.totalUs[stage]) / last_.calls[stage] / 1000.0 : 0.0;
  }
  double payloadBytesPerSecond() const {
    return valid() ? static_cast<double>(last_.payloadBytes) * 1000.0 / last_.periodMs : 0.0;
  }
 private:
  Window current_{}, last_{};
  uint32_t startedMs_ = 0;
  bool enabled_ = false;
  bool valid_ = false;
};
