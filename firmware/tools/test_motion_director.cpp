#include <cassert>
#include <cstdint>
#include <cstdio>
#include "../src/core/motion_director.h"

int main() {
  MotionDirector engine;
  MotionDirector::Context ctx{};
  assert(engine.style() == MotionDirector::NATURAL);
  assert(engine.sample(ctx, 0).mode == MotionDirector::IDLE);
  assert(engine.sample(ctx, 0).cel == 0);
  assert(engine.sample(ctx, 660).cel == 1);

  // Timed blink cues are deterministic without allocation or RNG writes.
  bool blinkFound = false;
  for (uint32_t t = 0; t < 30000; t += 20) {
    auto cue = engine.sample(ctx, t);
    if (cue.blink) {
      assert(cue.cel == 2);
      assert(engine.sample(ctx, t).blink);
      blinkFound = true;
    }
  }
  assert(blinkFound);

  ctx.enabled = false;
  assert(engine.sample(ctx, 1000).mode == MotionDirector::STILL);
  assert(engine.sample(ctx, 1000).cel == 0);
  ctx.enabled = true;
  ctx.quiet = true;
  assert(engine.sample(ctx, 1000).bob == 0);
  ctx.quiet = false;

  // A synthetic load must NOT claim to be a real CPU/GPU reaction.
  ctx.cpu = 99;
  ctx.gpu = 97;
  assert(engine.sample(ctx, 1000).mode == MotionDirector::IDLE);
  assert(engine.sample(ctx, 1000).pcIntensity == 0);
  ctx.realPc = true;
  assert(engine.sample(ctx, 1000).mode == MotionDirector::WARNING);
  assert(engine.sample(ctx, 1000).pcIntensity == 99);
  ctx.cpu = 2; ctx.gpu = -1;

  ctx.playing = true;
  auto song = engine.sample(ctx, 0);
  assert(song.mode == MotionDirector::GROOVE);
  assert(song.cel == 0 && song.celPeriodMs == 155);
  assert(engine.sample(ctx, 155).cel == 1);
  assert(engine.sample(ctx, 310).cel == 2);
  assert(engine.sample(ctx, 465).cel == 1);

  ctx.focusing = true;
  assert(engine.sample(ctx, 500).mode == MotionDirector::DEEP_FOCUS);
  ctx.persona = CatBehavior::HOT;
  assert(engine.sample(ctx, 500).mode == MotionDirector::WARNING);
  ctx.persona = CatBehavior::CELEBRATE;
  assert(engine.sample(ctx, 500).mode == MotionDirector::CELEBRATING);
  ctx.persona = CatBehavior::SLEEPY;
  ctx.focusing = ctx.playing = false;
  assert(engine.sample(ctx, 500).mode == MotionDirector::NAPPING);
  assert(engine.sample(ctx, 500).bob == 0);

  ctx.persona = CatBehavior::COZY;
  engine.setStyle(MotionDirector::CALM);
  const auto calm = engine.sample(ctx, 300);
  engine.setStyle(MotionDirector::PLAYFUL);
  const auto playful = engine.sample(ctx, 300);
  assert(playful.bob > calm.bob);
  assert(playful.celPeriodMs < calm.celPeriodMs);
  engine.setStyle(99); // invalid preferences leave current setting intact
  assert(engine.style() == MotionDirector::PLAYFUL);

  // Two overloaded five-second windows disable crossfades/sparkles and
  // soften bobbing. Four healthy windows recover; TFT refresh is unchanged.
  engine.observeBudget(16.0f, 25, 4, 42000);
  assert(!engine.budgetLimited());
  engine.observeBudget(17.0f, 25, 4, 42000);
  assert(engine.budgetLimited());
  ctx.playing = true;
  auto guarded = engine.sample(ctx, 310);
  assert(guarded.budgetLimited && guarded.bob <= 1);
  assert(!guarded.sparkle && !guarded.fadeAllowed);
  assert(guarded.celPeriodMs == 1200);
  for (int i = 0; i < 4; ++i) engine.observeBudget(25.0f, 25, 0, 9000);
  assert(!engine.budgetLimited());
  assert(engine.sample(ctx, 310).fadeAllowed);

  // 32-bit millis overflow must not create out-of-range cels or durations.
  assert(engine.sample(ctx, UINT32_MAX - 12).cel < 3);
  assert(engine.sample(ctx, 18).cel < 3);
  std::puts("OK: 8 motion modes, real-only PC load, blink timing, priorities, budget hysteresis");
}
