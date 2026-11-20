#include <cassert>
#include <cstdio>
#include "../src/core/cat_behavior.h"
int main() {
  CatBehavior cat; CatBehavior::Context c;
  c.pcOnline=true; c.sensorMood="healthy"; c.idleSeconds=400;
  cat.tick(c,100); cat.tick(c,1800); assert(cat.persona()==CatBehavior::SLEEPY);
  c.idleSeconds=0; cat.tick(c,2000); cat.tick(c,3700); assert(cat.persona()==CatBehavior::WAKING);
  c.focusing=true; cat.tick(c,4000); cat.tick(c,5700); assert(cat.persona()==CatBehavior::FOCUS);
  c.sensorMood="hot"; cat.tick(c,5800); assert(cat.persona()==CatBehavior::HOT);
  c.sensorMood="healthy"; c.focusing=false; c.morningDay=200;
  cat.tick(c,6000); cat.tick(c,7700); assert(cat.persona()==CatBehavior::GREETING);
  cat.tick(c,15000); cat.tick(c,16700); assert(cat.persona()==CatBehavior::COZY);
  cat.tick(c,18000); assert(cat.persona()==CatBehavior::COZY);
  c.morningDay=201; cat.tick(c,19000); cat.tick(c,20700); assert(cat.persona()==CatBehavior::GREETING);
  c.pcOnline=false; c.morningDay=0; c.idleSeconds=-1;
  cat.tick(c,22000); cat.tick(c,23700); assert(cat.persona()==CatBehavior::SEARCHING);
  c.pcOnline=true; c.morningDay=202; // absent input cannot fabricate a greeting
  cat.tick(c,24000); cat.tick(c,25700); assert(cat.persona()==CatBehavior::COZY);
  puts("OK: real-idle sleep/wake, once-daily greeting, Focus/critical priority and offline");
}
