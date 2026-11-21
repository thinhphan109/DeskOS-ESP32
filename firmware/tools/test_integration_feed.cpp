#include <cassert>
#include <cstdio>
#include <cstring>
#include "../src/core/integration_feed.h"
int main() {
  IntegrationFeed feed;char value[32];feed.format(value,sizeof(value),0);assert(!strcmp(value,"N/A"));
  strcpy(feed.name,"Room");strcpy(feed.value,"22.5");strcpy(feed.unit,"C");feed.received=true;feed.updatedMs=100;feed.ttlSeconds=2;
  assert(feed.valid()&&feed.fresh(2099));feed.format(value,sizeof(value),1000);assert(!strcmp(value,"22.5 C"));
  assert(!feed.fresh(2100));feed.format(value,sizeof(value),2100);assert(!strcmp(value,"N/A"));
  feed.updatedMs=0xfffffff0UL;assert(feed.fresh(0x54UL));
  feed.ttlSeconds=301;assert(!feed.valid());feed.ttlSeconds=30;feed.value[0]='\x01';assert(!feed.valid());
  puts("OK: integration feed bounds, TTL, N/A expiry and counter rollover");
}
