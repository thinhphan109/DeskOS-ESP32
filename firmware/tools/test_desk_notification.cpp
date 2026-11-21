#include <cassert>
#include <cstdio>
#include "../src/core/desk_notification.h"
int main() {
  DeskNotification n;assert(n.valid());strcpy(n.text,"PRIVATE");assert(!strcmp(n.displayText(),"New notification"));
  n.hidden=false;assert(!strcmp(n.displayText(),"PRIVATE"));n.priority=3;assert(!n.valid());n.priority=1;
  n.ttlSeconds=31;assert(!n.valid());n.ttlSeconds=8;n.source[0]='!';assert(!n.valid());
  NotificationGate gate;assert(gate.accept(0xfffffff0UL));assert(!gate.accept(0x54));assert(gate.accept(0x800));
  puts("OK: bounded notification source/text, privacy, priority and rate-limit rollover");
}
