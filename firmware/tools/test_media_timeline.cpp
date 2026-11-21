#include <cassert>
#include <cstdio>
#include "../src/core/media_state.h"
int main() {
  MediaState m; m.received=true;m.available=true;m.playing=true;m.receivedMs=100;m.positionMs=1000;m.durationMs=3000;
  assert(m.positionAt(1300)==2200);assert(m.positionAt(4000)==3000);assert(m.positionAt(10100)==0);
  m.playing=false;assert(m.positionAt(1300)==1000);
  m.playing=true;m.receivedMs=0xfffffff0UL;assert(m.positionAt(0x54UL)==1100);
  m.available=false;assert(m.positionAt(0x54UL)==0);
  puts("OK: projected media position, pause, duration clamp, stale and rollover");
}
