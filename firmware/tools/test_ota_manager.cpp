// Offline C++ test of the actual ota_manager.cpp against ESP-IDF API stubs.
// Does NOT validate bootloader configuration, real flash drivers, or power loss.
#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <openssl/evp.h>
#include "ota_manager.h"
#include "core/update_contract.h"

MockEsp ESP;
MockSerial Serial;
static esp_partition_t slotA{"app0", 6u * 1024u * 1024u};
static esp_partition_t slotB{"app1", 6u * 1024u * 1024u};
static const esp_partition_t *running = &slotA;
static const esp_partition_t *toBoot = nullptr;
static std::vector<uint8_t> received;
static int begins = 0, aborts = 0, ends = 0, confirms = 0;

const esp_partition_t *esp_ota_get_running_partition() { return running; }
const esp_partition_t *esp_ota_get_next_update_partition(const esp_partition_t *) {
  return running == &slotA ? &slotB : &slotA;
}
esp_err_t esp_ota_begin(const esp_partition_t *slot, size_t bytes, esp_ota_handle_t *handle) {
  assert(slot != running && bytes <= slot->size);
  received.clear();
  begins++;
  *handle = 9;
  return ESP_OK;
}
esp_err_t esp_ota_write(esp_ota_handle_t handle, const void *data, size_t size) {
  assert(handle == 9);
  const uint8_t *buffer = static_cast<const uint8_t *>(data);
  received.insert(received.end(), buffer, buffer + size);
  return ESP_OK;
}
esp_err_t esp_ota_abort(esp_ota_handle_t) { aborts++; return ESP_OK; }
esp_err_t esp_ota_end(esp_ota_handle_t) { ends++; return ESP_OK; }
esp_err_t esp_ota_set_boot_partition(const esp_partition_t *slot) { toBoot = slot; return ESP_OK; }
esp_err_t esp_ota_get_state_partition(const esp_partition_t *, esp_ota_img_states_t *state) {
  *state = ESP_OTA_IMG_PENDING_VERIFY; return ESP_OK;
}
esp_err_t esp_ota_mark_app_valid_cancel_rollback() { confirms++; return ESP_OK; }
esp_err_t esp_ota_mark_app_invalid_rollback_and_reboot() { return ESP_OK; }

static std::string sha(const std::vector<uint8_t> &data) {
  uint8_t digest[32] = {};
  unsigned int n = 0;
  assert(EVP_Digest(data.data(), data.size(), digest, &n, EVP_sha256(), nullptr) == 1);
  assert(n == 32);
  std::string result;
  const char *hex = "0123456789abcdef";
  for (auto byte : digest) {
    result.push_back(hex[byte >> 4]);
    result.push_back(hex[byte & 15]);
  }
  return result;
}

int main() {
  std::vector<uint8_t> image(5000, 0xa5);
  image[0] = 0xe9; image[12] = 9; image[13] = 0;
  assert(meoOta.available());
  assert(meoOta.availableBytes() == 6u * 1024u * 1024u);
  assert(!meoOta.begin(image.size(), String("bad-sha"), String(MeoOtaManager::BOARD_ID)));
  assert(begins == 0);
  assert(!meoOta.begin(image.size(), String(sha(image)), String("wrong-device")));
  assert(begins == 0);

  assert(MimiUpdate::coordinator().acquire(MimiUpdate::Owner::CatPack));
  assert(!meoOta.begin(image.size(), String(sha(image)), String(MeoOtaManager::BOARD_ID)));
  assert(begins == 0);
  MimiUpdate::coordinator().release(MimiUpdate::Owner::CatPack);
  assert(meoOta.begin(image.size(), String(sha(image)), String(MeoOtaManager::BOARD_ID)));
  assert(meoOta.append(image.data(), 1));
  assert(meoOta.append(image.data() + 1, 5));
  assert(meoOta.append(image.data() + 6, 9));
  assert(begins == 0); // No inactive partition erase before full image header.
  assert(meoOta.append(image.data() + 15, 1));
  assert(begins == 1);
  assert(meoOta.append(image.data() + 16, image.size() - 16));
  assert(meoOta.finish());
  assert(ends == 1 && toBoot == &slotB && received == image);
  assert(meoOta.phase() == MeoOtaManager::READY_REBOOT);
  assert(!strcmp(meoOta.bootHealth(), "checking"));
  meoOta.confirmRunningImageAfterSelfTest(true);
  assert(!strcmp(meoOta.bootHealth(), "passed"));
  assert(confirms == 1);
  assert(MimiUpdate::coordinator().owner()==MimiUpdate::Owner::Firmware);
  // A real reboot resets the in-RAM coordinator before the next manager exists.
  MimiUpdate::coordinator()=MimiUpdate::Coordinator{};

  // New manager instance (new firmware boot) rejects an ESP32-C3 header
  // without erasing the previous inactive slot.
  MeoOtaManager wrongChip;
  auto foreign = image;
  foreign[12] = 5; foreign[13] = 0;
  assert(wrongChip.begin(foreign.size(), String(sha(foreign)), String(MeoOtaManager::BOARD_ID)));
  assert(!wrongChip.append(foreign.data(), 30));
  assert(begins == 1 && toBoot == &slotB);

  MeoOtaManager badHash;
  assert(badHash.begin(image.size(), String(std::string(64, '0')), String(MeoOtaManager::BOARD_ID)));
  assert(badHash.append(image.data(), image.size()));
  assert(!badHash.finish());
  assert(aborts == 1 && toBoot == &slotB);

  ESP.flashSize = 8u * 1024u * 1024u;
  MeoOtaManager tooSmall;
  assert(!tooSmall.available());
  assert(!tooSmall.begin(image.size(), String(sha(image)), String(MeoOtaManager::BOARD_ID)));
  std::puts("OK: OTA stream, chip ID, SHA, partition, and rejection paths");
}
