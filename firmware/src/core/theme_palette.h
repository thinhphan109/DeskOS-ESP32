#pragma once
#include <stdint.h>

constexpr uint16_t themeRgb(uint8_t r,uint8_t g,uint8_t b) {
  return static_cast<uint16_t>(((r&0xF8)<<8)|((g&0xFC)<<3)|(b>>3));
}
struct ThemePalette {
  uint16_t bg,paper,shadow,ink,muted,mint,teal,peach,coral,lilac,gold,track;
  bool dark;
};
inline ThemePalette themePalette(uint8_t choice,bool autoNight=false) {
  const bool dark=choice==1||choice==3||(choice==2&&autoNight);
  ThemePalette p=dark ? ThemePalette{
    themeRgb(30,34,44),themeRgb(45,50,65),themeRgb(24,28,38),themeRgb(248,244,239),themeRgb(171,181,191),
    themeRgb(46,82,71),themeRgb(155,232,198),themeRgb(89,57,68),themeRgb(255,155,144),themeRgb(117,105,153),themeRgb(253,204,102),themeRgb(73,77,91),true
  } : ThemePalette{
    themeRgb(255,248,241),themeRgb(255,255,255),themeRgb(238,226,221),themeRgb(43,49,69),themeRgb(126,130,148),
    themeRgb(189,238,220),themeRgb(53,142,123),themeRgb(255,220,206),themeRgb(236,119,112),themeRgb(226,215,250),themeRgb(253,204,102),themeRgb(238,237,239),false
  };
  if(choice==3) {
    p.bg=themeRgb(12,18,28);p.paper=themeRgb(22,32,46);p.shadow=themeRgb(8,12,20);
    p.ink=themeRgb(219,255,247);p.muted=themeRgb(150,193,201);p.teal=themeRgb(68,255,215);
    p.coral=themeRgb(255,111,190);p.lilac=themeRgb(143,126,255);p.track=themeRgb(46,62,77);
  } else if(choice==4) {
    p.bg=themeRgb(255,241,246);p.paper=themeRgb(255,250,252);p.shadow=themeRgb(240,214,226);
    p.ink=themeRgb(78,43,62);p.muted=themeRgb(126,83,103);p.teal=themeRgb(126,74,110);
    p.mint=themeRgb(224,237,216);p.peach=themeRgb(250,214,225);p.coral=themeRgb(191,73,113);
    p.lilac=themeRgb(230,209,235);p.track=themeRgb(237,220,229);
  } else if(choice==5) {
    p.bg=themeRgb(246,246,242);p.paper=p.bg;p.shadow=p.bg;
    p.ink=themeRgb(31,40,36);p.muted=themeRgb(87,98,91);p.teal=p.ink;
    p.mint=themeRgb(218,226,216);p.peach=p.mint;p.coral=themeRgb(68,101,81);
    p.lilac=themeRgb(220,224,218);p.track=themeRgb(214,220,211);
  }
  return p;
}
