#include <cassert>
#include <cstdio>
#include "../src/core/theme_palette.h"
int main() {
  const auto day=themePalette(0), night=themePalette(1), cyber=themePalette(3), sakura=themePalette(4), minimal=themePalette(5);
  assert(!day.dark && night.dark && cyber.dark && !sakura.dark && !minimal.dark);
  assert(themePalette(2,true).bg==night.bg && themePalette(2,false).bg==day.bg);
  assert(sakura.bg!=day.bg && cyber.bg!=night.bg && minimal.paper==minimal.bg);
  assert(themePalette(4,true).bg==sakura.bg && themePalette(3,false).dark);
  puts("OK: stable legacy theme IDs, Auto selection and three new palettes");
}
