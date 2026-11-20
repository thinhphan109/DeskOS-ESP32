#include <cassert>
#include <cstdio>
#include "../src/core/scene_layout.h"
int main() {
  { SceneLayout s; assert(s.valid()); s.widgets[1].bounds.y=30; assert(!s.valid()); }
  { SceneLayout s; s.widgets[0].bounds.x=1; assert(!s.valid()); }
  { SceneLayout s; s.widgets[0].bounds.x=-1; assert(!s.valid()); }
  { SceneLayout s; s.count=7; assert(!s.valid()); }
  { SceneLayout s; s.widgets[0].label[0]='\x01'; assert(!s.valid()); }
  { SceneLayout s; s.count=1; s.widgets[0]={WidgetKind::TEXT,{0,0,162,120},"Hello"}; assert(s.valid()); }
  puts("OK: bounded non-overlapping widget scene");
}
