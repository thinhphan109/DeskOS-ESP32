#include <cassert>
#include <cstdio>
#include "../src/core/automation_rules.h"
int main() {
  { RuleSet s;s.count=1;assert(s.valid());AutomationRules engine;RuleContext c;c.realPc=true;c.cpu=95;
    s.rules[0].cooldownSeconds=60;
    assert(!engine.sample(s,c,100));assert(!engine.sample(s,c,30099));assert(engine.sample(s,c,30100)==1);
    assert(!engine.sample(s,c,60100));c.cpu=0;assert(!engine.sample(s,c,60200));c.cpu=95;
    assert(!engine.sample(s,c,60300));assert(engine.sample(s,c,90300)==1);
    engine.reset();c.realPc=false;assert(!engine.sample(s,c,100));assert(!engine.sample(s,c,40100));
    c.realPc=true;assert(!engine.sample(s,c,40200));assert(engine.sample(s,c,70200)==1);
  }
  { RuleSet s;s.count=1;auto &r=s.rules[0];r.when=RuleWhen::HOUR;r.threshold=23;r.holdSeconds=0;
    AutomationRules engine;RuleContext c;c.hour=23;c.localDay=100;
    assert(engine.sample(s,c,100)==1);assert(!engine.sample(s,c,400000));
    c.localDay=101;assert(engine.sample(s,c,86400100)==1);
  }
  { RuleSet s;s.count=1;auto &r=s.rules[0];r.when=RuleWhen::FOCUS_COMPLETE;r.threshold=0;r.holdSeconds=0;
    AutomationRules engine;RuleContext c;c.focusCompleted=5;
    assert(!engine.sample(s,c,1));c.focusCompleted=6;assert(engine.sample(s,c,2)==1);
    assert(!engine.sample(s,c,3));r.holdSeconds=1;assert(!s.valid());
  }
  { RuleSet s;s.count=1;s.rules[0].holdSeconds=1;AutomationRules engine;RuleContext c;c.realPc=true;c.cpu=95;
    assert(!engine.sample(s,c,0xffffff00UL));assert(engine.sample(s,c,0x34cUL)==1);
    engine.reset();c.quiet=true;assert(!engine.sample(s,c,100));assert(!engine.sample(s,c,2000));
    c.quiet=false;assert(engine.sample(s,c,2100)==1);
  }
  puts("OK: rule dwell/latch/cooldown, real data, clock/day, Focus baseline, quiet and rollover");
}
