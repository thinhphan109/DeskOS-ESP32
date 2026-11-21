#include <cassert>
#include "../src/core/update_contract.h"
int main() {
  const char *ordered[]={"1.0.0-alpha","1.0.0-alpha.1","1.0.0-alpha.beta","1.0.0-beta",
    "1.0.0-beta.2","1.0.0-beta.11","1.0.0-rc.1","1.0.0","2.9.0","2.10.0","2.13.0-dev","2.13.0"};
  int result=0;
  for(unsigned i=1;i<sizeof(ordered)/sizeof(*ordered);++i) {
    assert(MimiUpdate::compareVersions(ordered[i-1],ordered[i],result) && result<0);
    assert(MimiUpdate::compareVersions(ordered[i],ordered[i-1],result) && result>0);
  }
  assert(MimiUpdate::compareVersions("2.13.0+one","2.13.0+two",result) && result==0);
  const char *invalid[]={"","1","1.2","01.2.3","1.02.3","1.2.03","1.2.3-01","1.2.3-","1.2.3-a..b","1.2.3+", "4294967296.0.0", "-1.0.0"};
  for(auto value:invalid) {MimiUpdate::Version parsed;assert(!MimiUpdate::parseVersion(value,parsed));}
  MimiUpdate::Coordinator coordinator;
  assert(coordinator.acquire(MimiUpdate::Owner::Firmware));
  assert(!coordinator.acquire(MimiUpdate::Owner::Firmware));
  assert(!coordinator.acquire(MimiUpdate::Owner::CatPack));
  coordinator.release(MimiUpdate::Owner::CatPack);
  assert(coordinator.owner()==MimiUpdate::Owner::Firmware);
  coordinator.release(MimiUpdate::Owner::Firmware);
  assert(coordinator.acquire(MimiUpdate::Owner::CatPack));
  assert(!coordinator.acquire(MimiUpdate::Owner::Firmware));
}
