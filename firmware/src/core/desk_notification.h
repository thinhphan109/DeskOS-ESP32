#pragma once
#include <stdint.h>
#include <string.h>

struct DeskNotification {
  char source[21]="integration";
  char text[61]="New notification";
  uint8_t priority=1;
  uint8_t ttlSeconds=8;
  bool hidden=true;
  bool valid() const {
    const size_t size=strnlen(source,sizeof(source));
    if(!size||size>=sizeof(source)||priority>2||ttlSeconds<1||ttlSeconds>30) return false;
    for(size_t i=0;i<size;++i) if(!((source[i]>='a'&&source[i]<='z')||(source[i]>='0'&&source[i]<='9')||source[i]=='_'||source[i]=='-')) return false;
    if(hidden) return true;
    const size_t length=strnlen(text,sizeof(text));
    if(!length||length>=sizeof(text)) return false;
    for(size_t i=0;i<length;++i) if(text[i]<32||text[i]>126) return false;
    return true;
  }
  const char *displayText() const { return hidden ? "New notification" : text; }
};
class NotificationGate {
 public:
  bool accept(uint32_t now) {
    if(seen_ && static_cast<uint32_t>(now-last_)<2000UL) return false;
    seen_=true;last_=now;return true;
  }
 private:
  bool seen_=false;
  uint32_t last_=0;
};
