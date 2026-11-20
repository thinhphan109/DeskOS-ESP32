#include <cassert>
#include <cstdio>
#include "../src/core/card_refresh.h"

int main() {
  // Midnight uses ~26 of a 40ms cat frame: <14ms spare must not defer
  // the time card until two seconds, as the old starvation policy did.
  assert(!CardRefresh::due(1000, 100, 1000, true, true, 13000));
  assert(CardRefresh::due(1080, 100, 1000, true, true, 13000));
  assert(CardRefresh::due(2080, 1080, 1000, true, true, 0));
  assert(!CardRefresh::due(2090, 2080, 1000, true, true, 20000));
  assert(CardRefresh::due(3001, 2080, 1000, true, true, 15000));
  assert(CardRefresh::due(4000, 3001, 1000, true, false, 0));
  assert(!CardRefresh::due(499, 0, 500, false, true, 20000));
  assert(!CardRefresh::due(500, 0, 500, false, true, 0));
  assert(CardRefresh::due(580, 0, 500, false, true, 0));
  assert(CardRefresh::due(100, 0xfffffff0U, 100, false, true, 20000));
  puts("OK: second-aligned clock, bounded dark-mode deferral, quiet mode, rollover");
}
