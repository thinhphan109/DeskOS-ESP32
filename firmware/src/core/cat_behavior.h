#pragma once

#include <stdint.h>
#include <string.h>

// Pet mood is a presentation model, not a fabricated sensor measurement.
// Critical PC warnings interrupt everything; softer modes have a dwell time
// to stop rapid oscillation between idle, music and focus.
class CatBehavior {
 public:
  enum Persona : uint8_t {
    SEARCHING, COZY, SLEEPY, MUSIC, FOCUS, BUSY, HOT, BATTERY, CELEBRATE
  };

  struct Context {
    const char *sensorMood = "rescue";
    bool pcOnline = false;
    bool playing = false;
    bool focusing = false;
    bool quiet = false;
    bool late = false;
  };

  Persona persona() const { return current_; }
  uint8_t energy() const { return energy_; }

  const char *name() const {
    switch (current_) {
      case SEARCHING: return "looking for PC";
      case COZY: return "cozy";
      case SLEEPY: return "sleepy";
      case MUSIC: return "dancing";
      case FOCUS: return "deep focus";
      case BUSY: return "busy";
      case HOT: return "too warm";
      case BATTERY: return "need charge";
      case CELEBRATE: return "celebrating";
    }
    return "cozy";
  }

  void celebrate(uint32_t now) { celebrationMs_ = now; celebrationActive_ = true; }
  void pet(uint32_t now) {
    energy_ = (uint8_t)(energy_ > 92 ? 100 : energy_ + 8);
    celebrate(now);
  }

  bool tick(const Context &ctx, uint32_t now) {
    if (!energyMark_) energyMark_ = now ? now : 1;
    const uint32_t minutes = (uint32_t)(now - energyMark_) / 60000UL;
    if (minutes) {
      energyMark_ += minutes * 60000UL;
      const uint32_t steps = minutes > 120 ? 120 : minutes;
      if (ctx.late || ctx.quiet) {
        energy_ = (uint8_t)(energy_ + steps * 3 > 100 ? 100 : energy_ + steps * 3);
      } else if (ctx.focusing || ctx.playing) {
        energy_ = steps >= energy_ ? 0 : (uint8_t)(energy_ - steps);
      }
    }

    const bool hot = ctx.sensorMood && !strcmp(ctx.sensorMood, "hot");
    const bool low = ctx.sensorMood && !strcmp(ctx.sensorMood, "low_battery");
    const bool stressed = ctx.sensorMood && !strcmp(ctx.sensorMood, "stressed");
    const bool critical = hot || low || stressed;

    Persona candidate = COZY;
    if (hot) candidate = HOT;
    else if (low) candidate = BATTERY;
    else if (stressed) candidate = BUSY;
    else if (celebrationActive_ && (uint32_t)(now - celebrationMs_) < 7500UL)
      candidate = CELEBRATE;
    else if (ctx.focusing) candidate = FOCUS;
    else if (ctx.playing) candidate = MUSIC;
    else if (!ctx.pcOnline) candidate = SEARCHING;
    else if (ctx.quiet || ctx.late || energy_ < 15) candidate = SLEEPY;
    else if (ctx.sensorMood && !strcmp(ctx.sensorMood, "sleep")) candidate = SLEEPY;

    if (candidate == current_) { pending_ = candidate; pendingSinceMs_ = now; return false; }
    // Urgent alerts and completion are immediate; otherwise require stability.
    const bool urgent = critical || candidate == CELEBRATE ||
                        (current_ == HOT || current_ == BATTERY || current_ == BUSY);
    if (!urgent) {
      if (pending_ != candidate) { pending_ = candidate; pendingSinceMs_ = now; return false; }
      if ((uint32_t)(now - pendingSinceMs_) < 1600UL) return false;
    }
    current_ = candidate;
    pending_ = candidate;
    pendingSinceMs_ = now;
    return true;
  }

 private:
  Persona current_ = SEARCHING;
  Persona pending_ = SEARCHING;
  uint32_t pendingSinceMs_ = 0;
  uint32_t energyMark_ = 0;
  uint32_t celebrationMs_ = 0;
  bool celebrationActive_ = false;
  uint8_t energy_ = 82;
};
