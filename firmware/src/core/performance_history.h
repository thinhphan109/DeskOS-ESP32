#pragma once

#include <stdint.h>

// Five-minute history sampled once per 5s. Fixed-size and allocation-free.
struct PerformancePoint {
  uint16_t fpsTenths = 0;
  uint8_t target = 25;
  uint8_t skipped = 0;
  uint16_t worstRenderMs = 0;
  uint16_t internalKb = 0;
  uint16_t psramKb = 0;
};

class PerformanceHistory {
 public:
  static constexpr uint8_t CAPACITY = 60;

  void add(const PerformancePoint &point) {
    data_[head_] = point;
    head_ = (head_ + 1) % CAPACITY;
    if (count_ < CAPACITY) ++count_;
  }

  uint8_t count() const { return count_; }

  bool at(uint8_t oldestIndex, PerformancePoint &result) const {
    if (oldestIndex >= count_) return false;
    result = data_[(head_ + CAPACITY - count_ + oldestIndex) % CAPACITY];
    return true;
  }

 private:
  PerformancePoint data_[CAPACITY]{};
  uint8_t head_ = 0;
  uint8_t count_ = 0;
};
