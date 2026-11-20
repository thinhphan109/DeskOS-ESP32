#include <assert.h>
#include <string.h>
#include "../src/mood_filter.h"

int main() {
  MoodFilter filter;
  assert(!strcmp(filter.current(), "rescue"));
  assert(!strcmp(filter.update("healthy", 1000), "rescue"));
  assert(!strcmp(filter.update("healthy", 6500), "rescue"));
  assert(!strcmp(filter.update("healthy", 7500), "healthy"));

  // One high-load sample should not flash a stressed cat.
  assert(!strcmp(filter.update("stressed", 8000), "healthy"));
  assert(!strcmp(filter.update("healthy", 9000), "healthy"));
  assert(!strcmp(filter.update("stressed", 10000), "healthy"));
  assert(!strcmp(filter.update("stressed", 12500), "stressed"));

  // A heat warning is urgent but still needs 1.4 sec of evidence.
  assert(!strcmp(filter.update("hot", 13000), "stressed"));
  assert(!strcmp(filter.update("hot", 14399), "stressed"));
  assert(!strcmp(filter.update("hot", 14400), "hot"));
  // Telemetry loss always switches to rescue at once.
  assert(!strcmp(filter.update("rescue", 14500), "rescue"));

  // millis() wraparound must preserve elapsed debounce semantics.
  MoodFilter wrap;
  assert(!strcmp(wrap.update("stressed", 0xFFFFFF00u), "rescue"));
  assert(!strcmp(wrap.update("stressed", 0x000008C4u), "stressed"));
  return 0;
}
