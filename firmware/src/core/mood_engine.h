#pragma once

#include <stdint.h>
#include <string.h>

struct MoodInputs {
  bool pcFresh = false;
  bool demo = false;
  bool focus = false;
  bool musicPlaying = false;
  bool quiet = false;
  float cpu = -1;
  float gpu = -1;
  float cpuTemp = -1;
  float gpuTemp = -1;
  float battery = -1;
  int8_t plugged = -1;
  uint8_t cpuLimit = 88;
  uint8_t gpuLimit = 92;
  uint8_t tempLimit = 80;
  uint8_t batteryLimit = 20;
};

class MoodEngine {
 public:
  const char *choose(const MoodInputs &input, uint32_t nowMs, const char *current) {
    if (!input.pcFresh) { idleSince_ = 0; return "rescue"; }
    if (input.demo) return "demo";
    if (input.plugged == 0 && input.battery >= 0 &&
        input.battery <= input.batteryLimit + (same(current, "low_battery") ? 5 : 0))
      return "low_battery";
    const int heatRelief = same(current, "hot") ? 4 : 0;
    if ((input.cpuTemp >= 0 && input.cpuTemp >= input.tempLimit - heatRelief) ||
        (input.gpuTemp >= 0 && input.gpuTemp >= input.tempLimit - heatRelief))
      return "hot";
    const int loadRelief = same(current, "stressed") ? 6 : 0;
    if ((input.cpu >= 0 && input.cpu >= input.cpuLimit - loadRelief) ||
        (input.gpu >= 0 && input.gpu >= input.gpuLimit - loadRelief))
      return "stressed";
    if (input.focus || input.quiet) return "paused";
    if (input.musicPlaying) return "music";
    if (input.cpu >= 0 && input.cpu < 20 && (input.gpu < 0 || input.gpu < 25)) {
      if (!idleSince_) idleSince_ = nowMs ? nowMs : 1;
      if ((uint32_t)(nowMs - idleSince_) >= 30000UL) return "sleep";
    } else {
      idleSince_ = 0;
    }
    if (input.plugged == 1 && input.battery >= 0 && input.battery < 95)
      return "power";
    return "healthy";
  }

 private:
  static bool same(const char *a, const char *b) { return a && strcmp(a, b) == 0; }
  uint32_t idleSince_ = 0;
};
