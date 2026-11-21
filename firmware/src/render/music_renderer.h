#pragma once
#include <Arduino.h>
#include <LovyanGFX.hpp>
#include "music_fonts.h"
#include "../core/utf8_text.h"

// The Music scene owns x8..311, y43..202. No cat DMA may write in this
// region. Metadata is cached; only the small timeline changes each tick.
namespace MusicView {
constexpr int X=8, Y=43, W=304, META_H=130;

inline String supported(const char *text, const lgfx::GFXfont &font) {
  String result;
  while(*text) {
    const char *start=text;
    uint32_t code;
    const bool valid=nextUtf8(text,code);
    lgfx::FontMetrics metric{};
    if(valid && code<=0xffff && font.updateFontMetric(&metric,static_cast<uint16_t>(code)))
      result+=String(start).substring(0,text-start);
    else result+='?';
  }
  return result;
}

inline void text(lgfx::LovyanGFX &target, const char *value, int x, int baseline,
                 int width, int lines, const lgfx::GFXfont &font, uint16_t color) {
  target.setFont(&font); target.setTextSize(1); target.setTextWrap(false);
  target.setTextColor(color); target.setTextDatum(lgfx::textdatum_t::baseline_left);
  const String clean=supported(value,font);
  size_t start=0;
  for(int row=0;row<lines && start<clean.length();++row) {
    size_t end=start, space=start;
    const char *cursor=clean.c_str()+start;
    while(*cursor) {
      const char *before=cursor; uint32_t code;
      nextUtf8(cursor,code);
      const size_t next=cursor-clean.c_str();
      const bool last=row==lines-1;
      String candidate=clean.substring(start,next);
      if(last && next<clean.length()) candidate+="…";
      if(target.textWidth(candidate)>width) { cursor=before; break; }
      end=next;
      if(code==' ') space=end;
    }
    const bool clipped=end<clean.length();
    if(row<lines-1 && clipped && space>start) end=space;
    String line=clean.substring(start,end); line.trim();
    if(row==lines-1 && clipped) line+="…";
    target.drawString(line,x,baseline+row*(font.yAdvance));
    if(end==start) break;
    start=end;
    while(start<clean.length() && clean[start]==' ') ++start;
  }
  target.setTextDatum(lgfx::textdatum_t::top_left);
  target.setFont(&lgfx::fonts::Font0);
}

inline void cover(lgfx::LovyanGFX &target, const uint16_t *pixels, int x, int y,
                  uint16_t background, uint16_t accent) {
  if(!pixels) {
    target.fillRoundRect(x,y,120,120,9,background);
    target.drawCircle(x+60,y+60,35,accent);
    target.drawCircle(x+60,y+60,25,accent);
    target.fillCircle(x+60,y+60,7,accent);
    return;
  }
  uint16_t row[120];
  for(int dy=0;dy<120;++dy) {
    for(int dx=0;dx<120;++dx) row[dx]=pixels[(dy*64/120)*64+dx*64/120];
    target.pushImage(x,y+dy,120,1,reinterpret_cast<const lgfx::rgb565_t *>(row));
  }
}
} // namespace MusicView
