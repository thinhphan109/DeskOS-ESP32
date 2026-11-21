#pragma once
#include <stddef.h>
#include <stdint.h>

// Strict scalar decoder. Never reads beyond the terminating NUL, and never
// returns a partial code point as a clipping boundary.
inline bool nextUtf8(const char *&text, uint32_t &code) {
  const auto *p=reinterpret_cast<const uint8_t *>(text);
  if(!*p) return false;
  uint8_t count=0;
  if(*p<0x80) { code=*p; ++text; return true; }
  if(*p>=0xc2 && *p<=0xdf) { count=1; code=*p&0x1f; }
  else if(*p>=0xe0 && *p<=0xef) { count=2; code=*p&0x0f; }
  else if(*p>=0xf0 && *p<=0xf4) { count=3; code=*p&7; }
  else { ++text; code='?'; return false; }
  for(uint8_t i=1;i<=count;++i) {
    if(!p[i] || (p[i]&0xc0)!=0x80) { ++text; code='?'; return false; }
    code=(code<<6)|(p[i]&0x3f);
  }
  text+=count+1;
  if((count==1 && code<0x80)||(count==2 && code<0x800)||(count==3 && code<0x10000)||
     code>0x10ffff||(code>=0xd800 && code<=0xdfff)) { code='?'; return false; }
  return true;
}

inline bool validMediaText(const char *text) {
  while(*text) {
    uint32_t code;
    if(!nextUtf8(text,code) || code<32 || (code>=127 && code<=159)) return false;
  }
  return true;
}
