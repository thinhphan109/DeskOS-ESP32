#pragma once

#include <stdint.h>

// Portable 25/5 Pomodoro clock. All timestamps use ESP32 millis() and tolerate
// wraparound; never depends on NTP or Windows time synchronization.
class FocusTimer {
 public:
  enum Phase : uint8_t { WORK, BREAK };
  enum Transition : uint8_t { NONE, WORK_COMPLETED, BREAK_COMPLETED };

  bool running() const { return running_; }
  Phase phase() const { return phase_; }
  uint16_t sessionsDone() const { return sessionsDone_; }
  uint32_t durationMs() const { return (phase_ == WORK ? workMin_ : breakMin_) * 60000UL; }
  uint16_t workMinutes() const { return workMin_; }
  uint16_t breakMinutes() const { return breakMin_; }
  uint16_t goal() const { return goal_; }
  bool configure(uint16_t workMin, uint16_t breakMin, uint16_t goal) {
    if (workMin < 1 || workMin > 180 || breakMin < 1 || breakMin > 60 || goal < 1 || goal > 24) return false;
    if (workMin != workMin_ || breakMin != breakMin_) reset();
    workMin_ = workMin; breakMin_ = breakMin; goal_ = goal;
    return true;
  }

  uint32_t elapsedMs(uint32_t nowMs) const {
    return elapsedMs_ + (running_ ? (uint32_t)(nowMs - startedMs_) : 0);
  }

  uint32_t remainingMs(uint32_t nowMs) const {
    const uint32_t elapsed = elapsedMs(nowMs);
    return elapsed >= durationMs() ? 0 : durationMs() - elapsed;
  }

  uint32_t remainingSeconds(uint32_t nowMs) const {
    return (remainingMs(nowMs) + 999) / 1000;
  }

  void toggle(uint32_t nowMs) {
    if (running_) {
      elapsedMs_ = elapsedMs(nowMs);
      running_ = false;
    } else {
      startedMs_ = nowMs;
      running_ = true;
    }
  }

  void reset() {
    running_ = false;
    elapsedMs_ = 0;
    phase_ = WORK;
    sessionsDone_ = 0;
  }

  void skip(uint32_t nowMs) {
    phase_ = phase_ == WORK ? BREAK : WORK;
    elapsedMs_ = 0;
    startedMs_ = nowMs;
    // Retain pause state on Skip: an explicit action should not start a timer
    // the user had paused.
  }

  Transition tick(uint32_t nowMs) {
    if (!running_ || elapsedMs(nowMs) < durationMs()) return NONE;
    const Transition changed = phase_ == WORK ? WORK_COMPLETED : BREAK_COMPLETED;
    if (phase_ == WORK && sessionsDone_ < 65535) ++sessionsDone_;
    // Carry overshoot into next phase so background stalls don't lose time.
    uint32_t carry = elapsedMs(nowMs) - durationMs();
    phase_ = phase_ == WORK ? BREAK : WORK;
    if (carry >= durationMs()) carry = 0; // No chained transitions on stalled loop.
    elapsedMs_ = carry;
    startedMs_ = nowMs;
    return changed;
  }

 private:
  Phase phase_ = WORK;
  bool running_ = false;
  uint32_t elapsedMs_ = 0;
  uint32_t startedMs_ = 0;
  uint16_t sessionsDone_ = 0;
  uint16_t workMin_ = 25, breakMin_ = 5, goal_ = 4;
};
