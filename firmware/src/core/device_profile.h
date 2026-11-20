#pragma once
#include <stdint.h>

struct DeviceProfile {
  static constexpr uint8_t SCHEMA = 1;
  static constexpr uint8_t PAGE_COUNT = 15;
  uint8_t theme = 0, fps = 1, style = 1;
  bool motion = true, autoPages = false, quiet = false;
  uint8_t scenes[4] = {12, 0, 5, 2};
  uint16_t workMin = 25, breakMin = 5, goal = 4;
  uint8_t cpu = 88, gpu = 92, temperature = 80, battery = 20;
  bool valid() const {
    if (theme > 2 || fps > 3 || style > 2 || workMin < 1 || workMin > 180 ||
        breakMin < 1 || breakMin > 60 || goal < 1 || goal > 24 ||
        cpu < 50 || cpu > 99 || gpu < 50 || gpu > 99 || temperature < 55 ||
        temperature > 110 || battery < 5 || battery > 50) return false;
    for (uint8_t i = 0; i < 4; ++i) {
      if (scenes[i] >= PAGE_COUNT) return false;
      for (uint8_t j = 0; j < i; ++j) if (scenes[i] == scenes[j]) return false;
    }
    return true;
  }
};
