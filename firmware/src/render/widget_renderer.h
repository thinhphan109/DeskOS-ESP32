#pragma once
#include <LovyanGFX.hpp>
#include <stdio.h>
#include "../core/scene_layout.h"

// Small drawing adapter: its data snapshot has no Wi-Fi/sensor/JSON dependency.
inline void drawWidgetScene(lgfx::LovyanGFX &target, const SceneLayout &layout,
                            const WidgetSnapshot &data, int originX, int originY,
                            uint16_t ink, uint16_t muted, uint16_t line) {
  for (uint8_t i=0;i<layout.count;++i) {
    const auto &widget=layout.widgets[i]; const auto &r=widget.bounds;
    const int x=originX+r.x, y=originY+r.y;
    target.setClipRect(x,y,r.width,r.height);
    target.drawRoundRect(x,y,r.width,r.height,4,line);
    target.setFont(&lgfx::fonts::Font0); target.setTextWrap(false);
    target.setTextDatum(lgfx::textdatum_t::top_left); target.setTextSize(1);
    target.setTextColor(muted); target.setCursor(x+4,y+3);
    if(widget.kind!=WidgetKind::TEXT) target.print(widget.label);
    char value[64]="N/A";
    const auto metric=[&value](float v,const char *suffix) { if(v>=0) snprintf(value,sizeof(value),"%.0f%s",static_cast<double>(v),suffix); };
    switch(widget.kind) {
      case WidgetKind::CLOCK: snprintf(value,sizeof(value),"%s",data.clock); break;
      case WidgetKind::CPU: metric(data.cpu,"%"); break;
      case WidgetKind::GPU: metric(data.gpu,"%"); break;
      case WidgetKind::RAM: metric(data.ram,"%"); break;
      case WidgetKind::NETWORK: metric(data.down," KB/s"); break;
      case WidgetKind::FOCUS: snprintf(value,sizeof(value),"%lu:%02lu",static_cast<unsigned long>(data.focusSeconds/60),static_cast<unsigned long>(data.focusSeconds%60)); break;
      case WidgetKind::MUSIC: snprintf(value,sizeof(value),"%s",data.track&&data.track[0]?data.track:"No track"); break;
      case WidgetKind::DATA_1: case WidgetKind::DATA_2: case WidgetKind::DATA_3:
        snprintf(value,sizeof(value),"%s",data.external[static_cast<uint8_t>(widget.kind)-8]);break;
      case WidgetKind::TEXT: snprintf(value,sizeof(value),"%s",widget.label); break;
    }
    target.setTextColor(ink);
    const bool large=r.width>=72 && r.height>=38 && widget.kind!=WidgetKind::MUSIC && widget.kind!=WidgetKind::TEXT && strlen(value)*12<=static_cast<size_t>(r.width-8);
    target.setTextSize(large?2:1);
    target.setCursor(x+4,y+(r.height>=32?16:10)); target.print(value);
  }
  target.clearClipRect(); target.setTextSize(1);
}
