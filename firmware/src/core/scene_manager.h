#pragma once

#include <stdint.h>

// Four configurable desk shortcuts layered over stable legacy page IDs.
// This never renumbers an existing page or depends on Arduino/LovyanGFX.
class SceneManager {
 public:
  static constexpr uint8_t SLOTS = 4;

  explicit SceneManager(uint8_t pageCount) : pageCount_(pageCount) {}

  uint8_t slot(uint8_t index) const {
    return index < SLOTS ? shortcuts_[index] : shortcuts_[0];
  }

  bool configure(uint8_t index, uint8_t page) {
    if (index >= SLOTS || page >= pageCount_) return false;
    for (uint8_t i = 0; i < SLOTS; ++i)
      if (i != index && shortcuts_[i] == page) return false;
    shortcuts_[index] = page;
    return true;
  }
  bool configureAll(const uint8_t pages[SLOTS]) {
    for (uint8_t i = 0; i < SLOTS; ++i) {
      if (pages[i] >= pageCount_) return false;
      for (uint8_t j = 0; j < i; ++j) if (pages[i] == pages[j]) return false;
    }
    for (uint8_t i = 0; i < SLOTS; ++i) shortcuts_[i] = pages[i];
    return true;
  }

  uint8_t nextFeatured(uint8_t current) const {
    for (uint8_t i = 0; i < SLOTS; ++i)
      if (shortcuts_[i] == current) return shortcuts_[(i + 1) % SLOTS];
    return shortcuts_[0];
  }

  uint8_t nextLegacy(uint8_t current) const {
    return (uint8_t)((current + 1) % pageCount_);
  }

 private:
  uint8_t pageCount_;
  uint8_t shortcuts_[SLOTS] = {12, 0, 5, 2}; // Home, PC, Music, Focus.
};
