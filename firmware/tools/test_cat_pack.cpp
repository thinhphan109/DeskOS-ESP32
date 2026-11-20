// Host test of the real CatPackManager with a simulated 4 MiB Flash asset
// partition, real SHA256, and heap-backed PSRAM. No hardware is accessed.
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <openssl/evp.h>
#include "cat_pack.h"
#include "cat_assets.h"

MockEsp ESP;
MockSerial Serial;
static esp_partition_t partition{"assets", CatPackManager::BANK_BYTES * 2};
static std::vector<uint8_t> flash(partition.size, 0xff);
static int eraseCalls = 0, writeCalls = 0;

const esp_partition_t *esp_partition_find_first(int type, int subtype, const char *label) {
  return type == ESP_PARTITION_TYPE_DATA && subtype == ESP_PARTITION_SUBTYPE_DATA_SPIFFS &&
         !strcmp(label, "assets") ? &partition : nullptr;
}
esp_err_t esp_partition_read(const esp_partition_t *p, uint32_t offset,
                             void *dst, size_t count) {
  if (p != &partition || (uint64_t)offset + count > flash.size()) return -1;
  memcpy(dst, flash.data() + offset, count);
  return ESP_OK;
}
esp_err_t esp_partition_write(const esp_partition_t *p, uint32_t offset,
                              const void *source, size_t count) {
  if (p != &partition || (uint64_t)offset + count > flash.size()) return -1;
  const uint8_t *bytes = static_cast<const uint8_t *>(source);
  for (size_t i = 0; i < count; ++i) {
    // Actual NOR Flash cannot change a programmed zero back to one.
    assert((flash[offset + i] & bytes[i]) == bytes[i]);
    flash[offset + i] &= bytes[i];
  }
  writeCalls++;
  return ESP_OK;
}
esp_err_t esp_partition_erase_range(const esp_partition_t *p, uint32_t offset,
                                   size_t count) {
  if (p != &partition || offset % 4096 || count % 4096 ||
      (uint64_t)offset + count > flash.size()) return -1;
  memset(flash.data() + offset, 0xff, count);
  eraseCalls++;
  return ESP_OK;
}

static std::string sha(const std::vector<uint8_t> &data) {
  uint8_t digest[32]; unsigned count = 0;
  assert(EVP_Digest(data.data(), data.size(), digest, &count, EVP_sha256(), nullptr) == 1);
  assert(count == 32);
  const char *hex = "0123456789abcdef";
  std::string result;
  for (uint8_t c : digest) {result += hex[c >> 4];result += hex[c & 15];}
  return result;
}

static void writeComplete(CatPackManager &manager, const std::vector<uint8_t> &image) {
  assert(manager.begin(String(sha(image)), image.size()));
  // Real HTTPUpload chunks may split anywhere; no whole-file buffer on ESP.
  for (size_t offset = 0; offset < image.size();) {
    size_t count = std::min(static_cast<size_t>(1436), image.size() - offset);
    assert(manager.append(image.data() + offset, count));
    offset += count;
  }
  assert(manager.finish());
  assert(manager.phase() == CatPackManager::READY_REBOOT);
}

int main() {
  std::vector<uint8_t> old(CatPackManager::PAYLOAD_BYTES, 0x9e);
  std::vector<uint8_t> next(CatPackManager::PAYLOAD_BYTES, 0x33);
  old[0] = 0x34; old[1] = 0x12;
  next[0] = 0x78; next[1] = 0x56;

  CatPackManager empty;
  empty.initialize();
  assert(empty.available() && !empty.loaded());
  assert(empty.lookup(cat_boot) == cat_boot);
  assert(!empty.begin(String("wrong hash")) && eraseCalls == 0);
  writeComplete(empty, old);
  assert(eraseCalls == 1 && empty.activeBank() == -1);

  CatPackManager reboot1;
  reboot1.initialize();
  assert(reboot1.loaded() && reboot1.activeBank() == 0 && reboot1.generation() == 1);
  assert(reboot1.lookup(cat_boot) != cat_boot);
  assert(reboot1.lookup(cat_boot)[0] == 0x1234);
  assert(!strcmp(reboot1.activeSha(), sha(old).c_str()));

  // Power loss halfway through new bank: previous committed pack survives.
  assert(reboot1.begin(String(sha(next))));
  assert(reboot1.append(next.data(), next.size() / 2));
  assert(flash[CatPackManager::BANK_BYTES] == 0xff); // Header not committed.
  reboot1.abort("simulated power loss");
  CatPackManager recovered;
  recovered.initialize();
  assert(recovered.loaded() && recovered.activeBank() == 0);
  assert(recovered.lookup(cat_boot)[0] == 0x1234);

  writeComplete(recovered, next);
  CatPackManager reboot2;
  reboot2.initialize();
  assert(reboot2.loaded() && reboot2.activeBank() == 1 && reboot2.generation() == 2);
  assert(reboot2.lookup(cat_boot)[0] == 0x5678);

  // Valid header but corrupt pixel bytes: loader falls back to older bank.
  flash[CatPackManager::BANK_BYTES + CatPackManager::DATA_OFFSET + 200] ^= 0x01;
  CatPackManager corrupt;
  corrupt.initialize();
  assert(corrupt.loaded() && corrupt.activeBank() == 0);
  assert(corrupt.lookup(cat_boot)[0] == 0x1234);

  // v1.6 must load a 3-cel animation from the second bank without changing
  // renderer API or damaging the previous v1 static bank.
  std::vector<uint8_t> motion(CatPackManager::MOTION_BYTES, 0x22);
  const size_t celBytes = CatPackManager::BYTES_PER_FRAME;
  motion[0] = 0x10; motion[1] = 0x11;
  motion[celBytes] = 0x20; motion[celBytes + 1] = 0x21;
  motion[2 * celBytes] = 0x30; motion[2 * celBytes + 1] = 0x31;
  assert(corrupt.begin(String(sha(motion)), CatPackManager::MOTION_BYTES));
  assert(corrupt.append(motion.data(), motion.size() / 2));
  corrupt.abort("power loss mid-motion upload");
  CatPackManager motionInterrupted;
  motionInterrupted.initialize();
  assert(motionInterrupted.loaded() && motionInterrupted.activeBank() == 0);
  assert(motionInterrupted.framesPerPose() == 1);
  writeComplete(motionInterrupted, motion);
  CatPackManager motionReady;
  motionReady.initialize();
  assert(motionReady.loaded() && motionReady.activeBank() == 1);
  assert(motionReady.framesPerPose() == 3 && motionReady.activeBytes() == motion.size());
  assert(motionReady.lookup(cat_boot)[0] == 0x1110);
  assert(motionReady.frame(cat_boot, 0, true)[0] == 0x1110);
  assert(motionReady.frame(cat_boot, 170, true)[0] == 0x2120);
  assert(motionReady.frame(cat_boot, 340, true)[0] == 0x3130);
  assert(motionReady.frame(cat_boot, 510, true)[0] == 0x2120);
  assert(motionReady.frame(cat_boot, 340, false)[0] == 0x1110);
  assert(motionReady.frameAt(cat_boot, 0)[0] == 0x1110);
  assert(motionReady.frameAt(cat_boot, 1)[0] == 0x2120);
  assert(motionReady.frameAt(cat_boot, 2)[0] == 0x3130);
  assert(motionReady.frameAt(cat_boot, 9)[0] == 0x1110); // bad cel clamps.
  // A device whose available PSRAM dropped below v2 demand still keeps the
  // compatible v1 backup; the v2 header must not make the screen unusable.
  ESP.freePsram = 1600000;
  CatPackManager memoryFallback;
  memoryFallback.initialize();
  assert(memoryFallback.loaded() && memoryFallback.activeBank() == 0);
  assert(memoryFallback.framesPerPose() == 1);
  assert(!memoryFallback.availableFor(CatPackManager::MOTION_BYTES));
  ESP.freePsram = 8u * 1024u * 1024u;
  // A corrupt new 48-frame pack must leave the old v1 bank bootable.
  flash[CatPackManager::BANK_BYTES + CatPackManager::DATA_OFFSET + 12] ^= 0x01;
  CatPackManager rollback;
  rollback.initialize();
  assert(rollback.loaded() && rollback.activeBank() == 0);
  assert(rollback.framesPerPose() == 1);
  assert(writeCalls > 10);
  std::puts("OK: static-v1 / motion-v2, PSRAM cel selection, A/B power-loss and hash fallback");
}
