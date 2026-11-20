#pragma once

#include <stdint.h>
#include <string.h>

// Portable, allocation-free debounce for the cat state machine.
// Call update() continuously with millis(); the input is already prioritized.
// No heap allocation, no changing moods on isolated 1-second load spikes.
class MoodFilter {
 public:
  const char *current() const { return current_; }

  const char *update(const char *proposed, uint32_t nowMs) {
    if (!proposed || !*proposed) return current_;
    if (strcmp(current_, proposed) == 0) {
      pending_ = nullptr;
      return current_;
    }
    if (!pending_ || strcmp(pending_, proposed) != 0) {
      pending_ = proposed;
      pendingSince_ = nowMs;
    }

    // Losing telemetry is immediate. Heat and low battery are urgent.
    // Normal / sleeping states need sustained evidence to appear.
    uint32_t debounceMs = 3500;
    if (!strcmp(proposed, "rescue") || !strcmp(proposed, "demo") ||
        !strcmp(proposed, "paused")) debounceMs = 0;
    else if (!strcmp(proposed, "hot") || !strcmp(proposed, "low_battery"))
      debounceMs = 1400;
    else if (!strcmp(proposed, "stressed")) debounceMs = 2500;
    else if (!strcmp(proposed, "sleep") || !strcmp(proposed, "healthy"))
      debounceMs = 6500;

    if ((uint32_t)(nowMs - pendingSince_) >= debounceMs) {
      current_ = pending_;
      pending_ = nullptr;
    }
    return current_;
  }

 private:
  const char *current_ = "rescue";
  const char *pending_ = nullptr;
  uint32_t pendingSince_ = 0;
};
