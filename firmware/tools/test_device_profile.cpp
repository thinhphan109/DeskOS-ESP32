#include <cassert>
#include <cstdio>
#include "../src/core/device_profile.h"
#include "../src/core/focus_timer.h"
#include "../src/core/scene_manager.h"
int main() {
  DeviceProfile p;
  assert(p.valid());
  p.scenes[0] = p.scenes[1]; assert(!p.valid()); p.scenes[0] = 12;
  p.workMin = 181; assert(!p.valid()); p.workMin = 50; assert(p.valid());
  SceneManager scenes(14);
  const uint8_t swapped[4] = {0,12,2,5}, duplicate[4] = {0,0,2,5};
  assert(scenes.configureAll(swapped)); assert(scenes.slot(0) == 0 && scenes.slot(1) == 12);
  assert(!scenes.configureAll(duplicate)); assert(scenes.slot(1) == 12);
  FocusTimer timer;
  assert(timer.configure(50,10,6)); assert(timer.durationMs() == 3000000UL);
  timer.toggle(100); timer.tick(3000100UL);
  assert(timer.phase() == FocusTimer::BREAK && timer.durationMs() == 600000UL);
  assert(timer.sessionsDone() == 1 && timer.goal() == 6);
  assert(timer.configure(50,10,8) && timer.running());
  assert(timer.configure(25,5,8) && !timer.running() && timer.sessionsDone() == 0);
  assert(!timer.configure(0,5,1));
  puts("OK: profile validation, atomic scene swaps, configurable Focus and goal");
}
