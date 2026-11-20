#pragma once

#include <stdint.h>
#include "cat_behavior.h"

// v1.7 Living Motion Engine. Deliberately no Arduino, heap allocation, sprite
// ownership, filesystem or TFT access. One cheap cue per scheduled DMA frame.
// The engine selects one cel from a v2 pack; original/static art remains valid.
class MotionDirector {
 public:
  enum Style : uint8_t { CALM = 0, NATURAL = 1, PLAYFUL = 2 };
  enum Mode : uint8_t {
    STILL, IDLE, SEARCHING, GROOVE, DEEP_FOCUS, WARNING, NAPPING, CELEBRATING
  };

  struct Context {
    CatBehavior::Persona persona = CatBehavior::COZY;
    bool enabled = true;
    bool quiet = false;
    bool playing = false;
    bool focusing = false;
    // CPU/GPU load must only be supplied for fresh, NON-DEMO telemetry.
    bool realPc = false;
    float cpu = -1.0f;
    float gpu = -1.0f;
  };

  struct Cue {
    Mode mode = STILL;
    uint8_t cel = 0;          // 0/1/2 in a 3-frame per-pose .meocat
    uint8_t bob = 0;          // vertical amplitude in screen pixels
    uint16_t bobPeriodMs = 1000;
    uint16_t celPeriodMs = 0; // time for a cel change, 0=still
    bool blink = false;       // cel 2 cue; drawn eyes depend on supplied artwork
    bool sparkle = false;
    bool fadeAllowed = false;
    bool budgetLimited = false;
    uint8_t pcIntensity = 0;  // real CPU/GPU load only, not mock telemetry
  };

  void setStyle(uint8_t style) {
    if (style <= PLAYFUL) style_ = static_cast<Style>(style);
  }
  uint8_t style() const {return style_;}
  bool budgetLimited() const {return budgetLimited_;}

  static const char *modeName(Mode mode) {
    switch (mode) {
      case STILL: return "still";
      case IDLE: return "cozy";
      case SEARCHING: return "waiting";
      case GROOVE: return "music";
      case DEEP_FOCUS: return "focus";
      case WARNING: return "alert";
      case NAPPING: return "sleep";
      case CELEBRATING: return "celebrate";
    }
    return "still";
  }

  // Called once per existing five-second performance window. The guard only
  // disables visual extras and slows cel changes; it never changes the TFT
  // FramePacer or pretends to guarantee a particular actual FPS.
  void observeBudget(float actualFps, uint8_t targetFps,
                     uint32_t missed, uint32_t worstRenderUs) {
    if (!targetFps) return;
    const uint32_t budgetUs = 1000000UL / targetFps;
    const bool slow = actualFps + 2.5f < targetFps || missed >= 3 ||
                      worstRenderUs > budgetUs * 9UL / 10UL;
    const bool healthy = actualFps + 1.0f >= targetFps && missed <= 1 &&
                         worstRenderUs < budgetUs * 7UL / 10UL;
    if (slow) {
      healthyWindows_ = 0;
      if (slowWindows_ < 2) ++slowWindows_;
      if (slowWindows_ >= 2) budgetLimited_ = true;
    } else if (healthy) {
      slowWindows_ = 0;
      if (healthyWindows_ < 4) ++healthyWindows_;
      if (healthyWindows_ >= 4) budgetLimited_ = false;
    } else {
      // Neither overload nor headroom: preserve the current guard state.
      slowWindows_ = healthyWindows_ = 0;
    }
  }

  Cue sample(const Context &ctx, uint32_t nowMs) const {
    Cue out{};
    out.budgetLimited = budgetLimited_;
    if (!ctx.enabled || ctx.quiet) return out;

    const bool critical = ctx.persona == CatBehavior::HOT ||
                          ctx.persona == CatBehavior::BATTERY ||
                          ctx.persona == CatBehavior::BUSY;
    float load = -1.0f;
    if (ctx.realPc) {
      if (ctx.cpu >= 0 && ctx.cpu <= 100) load = ctx.cpu;
      if (ctx.gpu >= 0 && ctx.gpu <= 100 && ctx.gpu > load) load = ctx.gpu;
    }
    if (load >= 0) out.pcIntensity = static_cast<uint8_t>(load);

    // Preserve CatBehavior's urgency priorities; do not let music override
    // hot GPU, low battery, or the completed Focus celebration.
    if (critical || load >= 90.0f) out.mode = WARNING;
    else if (ctx.persona == CatBehavior::CELEBRATE) out.mode = CELEBRATING;
    else if (ctx.focusing || ctx.persona == CatBehavior::FOCUS) out.mode = DEEP_FOCUS;
    else if (ctx.playing || ctx.persona == CatBehavior::MUSIC) out.mode = GROOVE;
    else if (ctx.persona == CatBehavior::SLEEPY) out.mode = NAPPING;
    else if (ctx.persona == CatBehavior::SEARCHING) out.mode = SEARCHING;
    else out.mode = load >= 75.0f ? WARNING : IDLE;

    const uint16_t styleShift = style_ == CALM ? 160 : style_ == PLAYFUL ? 80 : 0;
    switch (out.mode) {
      case GROOVE:
        out.celPeriodMs = static_cast<uint16_t>(style_ == CALM ? 220 :
                                         style_ == PLAYFUL ? 110 : 155);
        out.bob = style_ == CALM ? 2 : style_ == PLAYFUL ? 6 : 4;
        out.bobPeriodMs = 560;
        out.sparkle = true;
        break;
      case CELEBRATING:
        out.celPeriodMs = style_ == CALM ? 180 : 100;
        out.bob = style_ == CALM ? 3 : 6;
        out.bobPeriodMs = 420;
        out.sparkle = true;
        break;
      case WARNING:
        out.celPeriodMs = 340 + styleShift;
        out.bob = 2;
        out.bobPeriodMs = 740;
        break;
      case DEEP_FOCUS:
        out.celPeriodMs = 950 + styleShift;
        out.bob = 1;
        out.bobPeriodMs = 1800;
        break;
      case NAPPING:
        out.celPeriodMs = 1400 + styleShift;
        out.bob = 0;
        break;
      case SEARCHING:
        out.celPeriodMs = 850 + styleShift;
        out.bob = 1;
        break;
      case IDLE:
        out.celPeriodMs = 660 + styleShift;
        out.bob = style_ == CALM ? 1 : style_ == PLAYFUL ? 3 : 2;
        break;
      default: break;
    }

    // Music and celebration use the 0→1→2→1 loop. For quieter modes,
    // frame 2 is reserved for a short, deterministically irregular blink
    // cue. This is visible as a real blink only in artist-authored packs.
    if (out.mode == GROOVE || out.mode == CELEBRATING || out.mode == WARNING) {
      constexpr uint8_t seq[4] = {0, 1, 2, 1};
      out.cel = seq[(nowMs / out.celPeriodMs) % 4U];
    } else {
      out.cel = static_cast<uint8_t>((nowMs / out.celPeriodMs) % 2U);
      if (out.mode != NAPPING) {
        const uint32_t cycleMs = out.mode == DEEP_FOCUS ? 11500U :
                                 style_ == PLAYFUL ? 5600U :
                                 style_ == CALM ? 9800U : 7500U;
        const uint32_t block = nowMs / cycleMs;
        const uint32_t jitter = (block * 1664525UL + 1013904223UL) % 1300UL;
        const uint32_t phase = nowMs % cycleMs;
        out.blink = phase >= 2300UL + jitter && phase < 2440UL + jitter;
        if (out.blink) out.cel = 2;
      }
    }

    if (budgetLimited_) {
      out.bob = out.bob > 1 ? 1 : out.bob;
      out.sparkle = false;
      out.blink = false;
      // Keep content reactive but reduce cel transitions. Full TFT refresh
      // remains governed by FramePacer; this is not an FPS throttle.
      out.cel = static_cast<uint8_t>((nowMs / 1200UL) % 2UL);
      out.celPeriodMs = 1200;
    }
    out.fadeAllowed = !budgetLimited_;
    out.sparkle = out.sparkle && (nowMs % 1800UL < 140UL);
    return out;
  }

 private:
  Style style_ = NATURAL;
  bool budgetLimited_ = false;
  uint8_t slowWindows_ = 0;
  uint8_t healthyWindows_ = 0;
};
