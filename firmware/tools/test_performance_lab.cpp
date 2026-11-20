#include <cassert>
#include <cmath>
#include <cstdio>
#include "../src/core/performance_lab.h"
#include "../src/core/widget_contract.h"

int main() {
  PerformanceLab lab;
  assert(!lab.enabled() && !lab.valid());
  lab.add(PerformanceLab::CAT_CPU, 500);
  lab.pixels(144, 160);
  assert(!lab.finishWindow(5000));
  lab.setEnabled(true, 100);
  assert(!lab.valid());
  lab.add(PerformanceLab::CAT_CPU, 200);
  lab.add(PerformanceLab::CAT_CPU, 400);
  lab.add(PerformanceLab::CAT_WALL, 20000);
  lab.add(PerformanceLab::DMA_WAIT, 5000);
  lab.pixels(144, 160);
  assert(!lab.finishWindow(5099));
  assert(lab.finishWindow(5100));
  assert(lab.valid());
  assert(std::abs(lab.meanMs(PerformanceLab::CAT_CPU) - 0.3) < 0.0001);
  assert(lab.last().calls[PerformanceLab::CAT_WALL] == 1);
  assert(lab.last().maxUs[PerformanceLab::CAT_CPU] == 400);
  assert(std::abs(lab.payloadBytesPerSecond() - 9216.0) < 0.0001);
  assert(lab.finishWindow(10100));
  assert(lab.last().payloadBytes == 0);
  lab.setEnabled(true, 0xfffffff0U);
  assert(lab.finishWindow(5000));
  lab.setEnabled(false, 6000);
  assert(!lab.valid() && lab.last().payloadBytes == 0);
  constexpr RenderSurfaceInfo surface{320, 240, 16, true};
  assert(surface.accepts({10, 72, 162, 120}));
  assert(!surface.accepts({-1, 0, 144, 160}));
  assert(!surface.accepts({200, 72, 162, 120}));
  assert(!surface.accepts({0, 0, 0, 0}));
  puts("OK: optional fixed counters, window cadence/reset, byte rate, rollover, widget bounds");
}
