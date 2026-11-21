#pragma once
#include <stdint.h>
#include <string.h>
#include "widget_contract.h"

enum class WidgetKind : uint8_t { CLOCK, CPU, GPU, RAM, FOCUS, MUSIC, NETWORK, TEXT, DATA_1, DATA_2, DATA_3 };
struct SceneWidget {
  WidgetKind kind = WidgetKind::CLOCK;
  WidgetRegion bounds{0,0,162,40};
  char label[32] = "";
};
struct SceneLayout {
  static constexpr uint8_t CAPACITY = 6;
  uint8_t count = 4;
  SceneWidget widgets[CAPACITY] = {
    {WidgetKind::CLOCK,{0,0,162,40},"Clock"}, {WidgetKind::CPU,{0,42,80,36},"CPU"},
    {WidgetKind::RAM,{82,42,80,36},"RAM"}, {WidgetKind::FOCUS,{0,80,162,40},"Focus"}};
  bool valid() const {
    if (count < 1 || count > CAPACITY) return false;
    for (uint8_t i=0;i<count;++i) {
      const auto &w=widgets[i];
      if (static_cast<uint8_t>(w.kind)>10 || !w.bounds.fits(162,120) || w.bounds.width<32 || w.bounds.height<20) return false;
      bool terminated=false;
      for (uint8_t j=0;j<32;++j) {
        if (!w.label[j]) { terminated=true; break; }
        if (static_cast<uint8_t>(w.label[j])<32 || static_cast<uint8_t>(w.label[j])>126) return false;
      }
      if (!terminated) return false;
      for (uint8_t j=0;j<i;++j) {
        const auto &a=w.bounds; const auto &b=widgets[j].bounds;
        if (a.x<b.x+b.width && b.x<a.x+a.width && a.y<b.y+b.height && b.y<a.y+a.height) return false;
      }
    }
    return true;
  }
};
struct WidgetSnapshot {
  char clock[12] = "--:--";
  float cpu=-1, gpu=-1, ram=-1, down=-1;
  uint32_t focusSeconds=1500;
  const char *track="";
  char external[3][32]={"N/A","N/A","N/A"};
};
