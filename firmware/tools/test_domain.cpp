#include <assert.h>
#include <stdio.h>
#include "../src/core/command_queue.h"
#include "../src/core/media_state.h"
#include "../src/core/mood_engine.h"
#include "../src/core/event_engine.h"
#include "../src/core/focus_timer.h"
#include "../src/core/scene_manager.h"
#include "../src/core/cat_behavior.h"
#include "../src/core/frame_pacer.h"
#include "../src/core/frame_governor.h"
#include "../src/core/performance_history.h"
#include "../src/mood_filter.h"

int main() {
  MoodEngine engine;
  MoodInputs in;
  assert(!strcmp(engine.choose(in, 100, "rescue"), "rescue"));
  in.pcFresh = true;
  in.cpu = 5;
  in.gpu = 0; // Valid zero must not be mistaken for missing data.
  in.battery = -1; // Unavailable sensor is not empty battery.
  assert(!strcmp(engine.choose(in, 100, "rescue"), "healthy"));
  assert(!strcmp(engine.choose(in, 30200, "healthy"), "sleep"));
  in.focus = true;
  assert(!strcmp(engine.choose(in, 31000, "healthy"), "paused"));
  in.musicPlaying = true;
  assert(!strcmp(engine.choose(in, 31001, "healthy"), "paused"));
  in.focus = false;
  assert(!strcmp(engine.choose(in, 31002, "healthy"), "music"));
  in.cpuTemp = 85;
  assert(!strcmp(engine.choose(in, 31003, "healthy"), "hot"));
  in.cpuTemp = -1;
  in.plugged = 0;
  in.battery = 9;
  assert(!strcmp(engine.choose(in, 31004, "healthy"), "low_battery"));
  in.plugged = 1;
  assert(strcmp(engine.choose(in, 31005, "low_battery"), "low_battery"));
  in.demo = true;
  assert(!strcmp(engine.choose(in, 31006, "healthy"), "demo"));
  in.pcFresh = false;
  assert(!strcmp(engine.choose(in, 31007, "demo"), "rescue"));

  MediaState media;
  assert(!media.fresh(1000));
  media.received = true; media.available = true; media.playing = true;
  media.receivedMs = 1234;
  assert(media.playingNow(11233));
  assert(!media.playingNow(11234));

  CommandQueue queue;
  assert(!queue.submit("delete_everything", 100));
  assert(queue.submit("play_pause", 100));
  assert(queue.sequence() == 1);
  assert(queue.pending(100));
  assert(!queue.submit("next", 200));
  assert(queue.get(200)->seq == 1);
  assert(!queue.ack(2, true, 300));
  assert(queue.ack(1, true, 300));
  assert(queue.acknowledged() && queue.lastResult());
  assert(!queue.ack(1, true, 400));
  assert(queue.submit("next", 500));
  assert(!queue.pending(8500));
  assert(!queue.ack(2, true, 8501));
  assert(queue.submit("previous", 8502));
  EventEngine events;
  assert(events.publish("music_started", "Now playing", EventEngine::NOTICE, 500));
  assert(!events.publish("music_started", "Now playing", EventEngine::NOTICE, 600));
  assert(events.publish("gpu_hot", "GPU temperature high", EventEngine::CRITICAL, 1000, 2000));
  assert(!strcmp(events.active(1500)->code, "gpu_hot"));
  assert(!strcmp(events.active(3000)->code, "music_started"));
  assert(events.active(9000) == nullptr);
  for (int n = 0; n < 12; ++n) {
    char code[24]; snprintf(code, sizeof(code), "event_%d", n);
    assert(events.publish(code, "test", EventEngine::INFO, 10000 + n * 2000));
  }
  assert(events.count() == EventEngine::CAPACITY);
  assert(!strcmp(events.recentAt(0)->code, "event_11"));

  FocusTimer timer;
  assert(timer.remainingSeconds(0) == 1500);
  timer.toggle(100);
  assert(timer.remainingSeconds(10100) == 1490);
  timer.toggle(10100);
  assert(timer.remainingSeconds(20100) == 1490);
  timer.toggle(20100);
  assert(timer.tick(1510100) == FocusTimer::WORK_COMPLETED);
  assert(timer.sessionsDone() == 1 && timer.phase() == FocusTimer::BREAK);
  assert(timer.remainingSeconds(1510100) == 300);
  timer.skip(1510100);
  assert(timer.phase() == FocusTimer::WORK);
  timer.reset();
  assert(!timer.running() && timer.sessionsDone() == 0);
  // 32-bit millis rollover must not reset the countdown.
  timer.toggle(0xfffffff0UL);
  assert(timer.remainingSeconds(0x00000054UL) == 1500);
  SceneManager scenes(14);
  assert(scenes.slot(0) == 12 && scenes.slot(1) == 0);
  assert(scenes.nextFeatured(12) == 0 && scenes.nextFeatured(0) == 5);
  assert(!scenes.configure(0, 0)); // no duplicate shortcuts
  assert(!scenes.configure(2, 14)); // no invalid legacy ID
  assert(scenes.configure(0, 1));
  assert(scenes.nextFeatured(2) == 1);

  CatBehavior cat;
  CatBehavior::Context pet{};
  assert(cat.persona() == CatBehavior::SEARCHING);
  pet.pcOnline = true;
  pet.sensorMood = "healthy";
  assert(!cat.tick(pet, 100));
  assert(cat.tick(pet, 1800));
  assert(cat.persona() == CatBehavior::COZY);
  pet.focusing = true;
  cat.tick(pet, 2000);
  assert(cat.tick(pet, 3700) && cat.persona() == CatBehavior::FOCUS);
  pet.sensorMood = "hot";
  assert(cat.tick(pet, 3800) && cat.persona() == CatBehavior::HOT);
  pet.sensorMood = "healthy";
  cat.celebrate(4000);
  assert(cat.tick(pet, 4001) && cat.persona() == CatBehavior::CELEBRATE);

  FramePacer pacer(25);
  assert(pacer.fps() == 25 && pacer.frameBudgetUs() == 40000);
  pacer.rebase(100000);
  assert(!pacer.due(139999));
  assert(pacer.due(140000));
  assert(!pacer.due(140001));
  assert(pacer.due(260000)); // 2 scheduled frames skipped (180k, 220k)
  assert(pacer.takeMissed() == 2);
  assert(pacer.takeMissed() == 0);
  pacer.setFps(30);
  assert(pacer.fps() == 30 && pacer.frameBudgetUs() == 33333);
  pacer.rebase(0xfffffff0UL);
  assert(!pacer.due(0x00000010UL));
  assert(pacer.due((uint32_t)(0xfffffff0UL + 33333UL)));

  FrameGovernor governor;
  governor.setProfile(FrameGovernor::AUTO);
  assert(governor.target() == 25);
  assert(!governor.observe(20.5f, 9, 60000));
  assert(!governor.observe(20.5f, 9, 60000));
  assert(governor.observe(20.5f, 9, 60000) && governor.target() == 20);
  for (int i = 0; i < 5; ++i) assert(!governor.observe(19.8f, 0, 25000));
  assert(governor.observe(19.8f, 0, 25000) && governor.target() == 25);
  governor.setProfile(FrameGovernor::SMOOTH30);
  for (int i = 0; i < 10; ++i) assert(!governor.observe(10.f, 40, 85000));
  assert(governor.target() == 30); // Manual choice never overridden.

  PerformanceHistory perf;
  PerformancePoint input{}, output{};
  for (uint16_t i = 0; i < 65; ++i) {
    input.fpsTenths = i;
    perf.add(input);
  }
  assert(perf.count() == 60);
  assert(perf.at(0, output) && output.fpsTenths == 5);
  assert(perf.at(59, output) && output.fpsTenths == 64);
  assert(!perf.at(60, output));
  puts("OK: mood, events, focus, scene, cat, pacing, auto-governor and performance ring");
}
