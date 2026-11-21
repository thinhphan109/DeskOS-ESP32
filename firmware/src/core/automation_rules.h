#pragma once
#include <stdint.h>
#include <string.h>

enum class RuleWhen:uint8_t { CPU_ABOVE,GPU_ABOVE,HOUR,FOCUS_COMPLETE,IDLE_ABOVE,MUSIC_PLAYING };
enum class RuleDo:uint8_t { SCENE,THEME,CELEBRATE,NOTIFY };
struct AutomationRule {
  bool enabled=true;
  RuleWhen when=RuleWhen::CPU_ABOVE;
  RuleDo action=RuleDo::NOTIFY;
  uint16_t threshold=90,holdSeconds=30;
  uint32_t cooldownSeconds=300;
  uint8_t value=0;
  char label[33]="CPU busy";
  bool valid() const {
    if(static_cast<uint8_t>(when)>5||static_cast<uint8_t>(action)>3||holdSeconds>600||cooldownSeconds<5||cooldownSeconds>86400) return false;
    if((when==RuleWhen::CPU_ABOVE||when==RuleWhen::GPU_ABOVE)&&(threshold<1||threshold>100)) return false;
    if(when==RuleWhen::HOUR && threshold>23) return false;
    if(when==RuleWhen::IDLE_ABOVE && (threshold<1||threshold>7200)) return false;
    if((when==RuleWhen::FOCUS_COMPLETE||when==RuleWhen::MUSIC_PLAYING)&&threshold!=0) return false;
    if(when==RuleWhen::FOCUS_COMPLETE && holdSeconds!=0) return false;
    if((action==RuleDo::SCENE&&value>14)||(action==RuleDo::THEME&&value>5)||
       ((action==RuleDo::CELEBRATE||action==RuleDo::NOTIFY)&&value!=0)) return false;
    const size_t length=strnlen(label,sizeof(label));
    if(!length||length>=sizeof(label)) return false;
    for(size_t i=0;i<length;++i) if(label[i]<32||label[i]>126) return false;
    return true;
  }
};
struct RuleSet {
  static constexpr uint8_t CAPACITY=6;
  uint8_t count=0;
  AutomationRule rules[CAPACITY];
  bool valid() const { if(count>CAPACITY)return false;for(uint8_t i=0;i<count;++i)if(!rules[i].valid())return false;return true; }
};
struct RuleContext {
  bool realPc=false,playing=false,quiet=false;
  float cpu=-1,gpu=-1;
  int32_t idleSeconds=-1;
  int8_t hour=-1;
  uint32_t localDay=0,focusCompleted=0;
};
class AutomationRules {
 public:
  void reset() { for(auto &r:runtime_)r=Runtime{};focusSeen_=false; }
  uint8_t sample(const RuleSet &set,const RuleContext &ctx,uint32_t now) {
    if(set.count>RuleSet::CAPACITY)return 0;
    const bool focusPulse=focusSeen_&&ctx.focusCompleted!=focusSeq_;
    focusSeq_=ctx.focusCompleted;focusSeen_=true;
    uint8_t fired=0;
    for(uint8_t i=0;i<set.count;++i) {
      const auto &rule=set.rules[i];auto &r=runtime_[i];bool match=false;
      if(rule.enabled) switch(rule.when) {
        case RuleWhen::CPU_ABOVE:match=ctx.realPc&&ctx.cpu>=rule.threshold&&ctx.cpu<=100;break;
        case RuleWhen::GPU_ABOVE:match=ctx.realPc&&ctx.gpu>=rule.threshold&&ctx.gpu<=100;break;
        case RuleWhen::HOUR:match=ctx.localDay&&ctx.hour==rule.threshold&&r.lastDay!=ctx.localDay;break;
        case RuleWhen::FOCUS_COMPLETE:match=focusPulse;break;
        case RuleWhen::IDLE_ABOVE:match=ctx.realPc&&ctx.idleSeconds>=rule.threshold;break;
        case RuleWhen::MUSIC_PLAYING:match=ctx.realPc&&ctx.playing;break;
      }
      if(!match) { r.matching=false;r.latched=false;continue; }
      if(!r.matching) { r.matching=true;r.since=now; }
      if(r.latched||static_cast<uint32_t>(now-r.since)<static_cast<uint32_t>(rule.holdSeconds)*1000UL)continue;
      if(r.fired && static_cast<uint32_t>(now-r.lastFire)<rule.cooldownSeconds*1000UL)continue;
      if(ctx.quiet&&(rule.action==RuleDo::NOTIFY||rule.action==RuleDo::CELEBRATE))continue;
      fired|=static_cast<uint8_t>(1U<<i);r.latched=true;r.fired=true;r.lastFire=now;
      if(rule.when==RuleWhen::HOUR)r.lastDay=ctx.localDay;
    }
    return fired;
  }
 private:
  struct Runtime { bool matching=false,latched=false,fired=false;uint32_t since=0,lastFire=0,lastDay=0; };
  Runtime runtime_[RuleSet::CAPACITY];
  bool focusSeen_=false;
  uint32_t focusSeq_=0;
};
