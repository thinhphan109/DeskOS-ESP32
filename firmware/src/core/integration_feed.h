#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct IntegrationFeed {
  bool received=false;
  uint32_t updatedMs=0;
  uint16_t ttlSeconds=30;
  char name[17]="Data",value[21]="",unit[9]="";
  static bool textValid(const char *text,size_t capacity,bool emptyAllowed=false) {
    const size_t size=strnlen(text,capacity);
    if((!size&&!emptyAllowed)||size>=capacity)return false;
    for(size_t i=0;i<size;++i)if(text[i]<32||text[i]>126)return false;
    return true;
  }
  bool valid() const {return ttlSeconds>=1&&ttlSeconds<=300&&textValid(name,sizeof(name))&&textValid(value,sizeof(value))&&textValid(unit,sizeof(unit),true);}
  bool fresh(uint32_t now) const {return received&&static_cast<uint32_t>(now-updatedMs)<static_cast<uint32_t>(ttlSeconds)*1000UL;}
  void format(char *out,size_t capacity,uint32_t now) const {
    if(!fresh(now)){snprintf(out,capacity,"N/A");return;}
    snprintf(out,capacity,"%s%s%s",value,unit[0]?" ":"",unit);
  }
};
