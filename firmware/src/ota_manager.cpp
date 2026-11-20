#include "ota_manager.h"

#include <esp_partition.h>
#include <mbedtls/version.h>
#include <stdio.h>
#include <string.h>

MeoOtaManager meoOta;

bool MeoOtaManager::available() const {
  if (ESP.getFlashChipSize() < 16UL * 1024UL * 1024UL) return false;
  const esp_partition_t *running = esp_ota_get_running_partition();
  const esp_partition_t *other = esp_ota_get_next_update_partition(nullptr);
  return running && other && running != other && other->size >= MIN_IMAGE_BYTES;
}

size_t MeoOtaManager::availableBytes() const {
  return available() ? esp_ota_get_next_update_partition(nullptr)->size : 0;
}

const char *MeoOtaManager::phaseName() const {
  switch (phase_) {
    case RECEIVING: return "receiving";
    case READY_REBOOT: return "rebooting";
    case ERROR: return "error";
    default: return "idle";
  }
}

bool MeoOtaManager::validSha(const String &value) {
  if (value.length() != 64) return false;
  for (size_t i = 0; i < 64; ++i) {
    const char c = value[i];
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
          (c >= 'A' && c <= 'F'))) return false;
  }
  return true;
}

bool MeoOtaManager::shaStart(mbedtls_sha256_context *ctx) {
#if MBEDTLS_VERSION_MAJOR >= 3
  return mbedtls_sha256_starts(ctx, 0) == 0;
#else
  return mbedtls_sha256_starts_ret(ctx, 0) == 0;
#endif
}

bool MeoOtaManager::shaUpdate(mbedtls_sha256_context *ctx, const uint8_t *data, size_t bytes) {
#if MBEDTLS_VERSION_MAJOR >= 3
  return mbedtls_sha256_update(ctx, data, bytes) == 0;
#else
  return mbedtls_sha256_update_ret(ctx, data, bytes) == 0;
#endif
}

bool MeoOtaManager::shaFinish(mbedtls_sha256_context *ctx, uint8_t *digest) {
#if MBEDTLS_VERSION_MAJOR >= 3
  return mbedtls_sha256_finish(ctx, digest) == 0;
#else
  return mbedtls_sha256_finish_ret(ctx, digest) == 0;
#endif
}

void MeoOtaManager::fail(const char *message) {
  if (otaLive_) {
    esp_ota_abort(handle_);
    otaLive_ = false;
  }
  if (shaLive_) {
    mbedtls_sha256_free(&sha_);
    shaLive_ = false;
  }
  phase_ = ERROR;
  snprintf(message_, sizeof(message_), "%s", message);
  Serial.printf("[OTA] ERROR: %s\n", message_);
}

void MeoOtaManager::abort(const char *why) { fail(why); }

bool MeoOtaManager::begin(size_t bytes, const String &sha256, const String &board) {
  if (phase_ == RECEIVING || phase_ == READY_REBOOT) return false;
  if (board != BOARD_ID || !validSha(sha256)) {
    fail("Invalid board identifier or SHA-256 header");
    return false;
  }
  if (!available() || bytes < MIN_IMAGE_BYTES || bytes > availableBytes()) {
    fail("OTA unavailable or firmware exceeds inactive slot");
    return false;
  }
  target_ = esp_ota_get_next_update_partition(nullptr);
  if (!target_ || target_ == esp_ota_get_running_partition()) {
    fail("No independent inactive OTA partition");
    return false;
  }
  // Delay erasing the previous inactive image until the first 16 bytes prove
  // this really targets ESP32-S3. A wrong-chip upload must not destroy the
  // potential rollback image in the inactive slot.
  otaLive_ = false;
  mbedtls_sha256_init(&sha_);
  shaLive_ = true;
  if (!shaStart(&sha_)) {
    fail("Unable to initialize SHA-256");
    return false;
  }
  for (size_t i = 0; i < 64; ++i) {
    const char c = sha256[i];
    expectedSha_[i] = (c >= 'A' && c <= 'F') ? c + ('a' - 'A') : c;
  }
  expectedSha_[64] = 0;
  expected_ = static_cast<uint32_t>(bytes);
  written_ = 0;
  imagePrefixUsed_ = 0;
  phase_ = RECEIVING;
  snprintf(message_, sizeof(message_), "Receiving firmware into inactive slot");
  Serial.printf("[OTA] Slot %s: %lu bytes\n", target_->label, (unsigned long)expected_);
  return true;
}

bool MeoOtaManager::append(const uint8_t *buffer, size_t size) {
  if (phase_ != RECEIVING || !shaLive_) return false;
  if (!buffer || !size || size > expected_ - written_) {
    fail("Unexpected OTA chunk size");
    return false;
  }
  if (!written_ && buffer[0] != 0xE9) {
    fail("Invalid ESP32 firmware image magic");
    return false;
  }
  // ESP-IDF application header: chip_id (little-endian) is at bytes 12..13.
  // 0x0009 is ESP32-S3. A matching HTTP board header alone is NOT proof that
  // a user-selected .bin was actually compiled for this ESP32-S3 target.
  size_t offset = 0;
  if (imagePrefixUsed_ < sizeof(imagePrefix_)) {
    while (imagePrefixUsed_ < sizeof(imagePrefix_) && offset < size) {
      imagePrefix_[imagePrefixUsed_++] = buffer[offset++];
    }
    if (imagePrefixUsed_ == sizeof(imagePrefix_) &&
        (imagePrefix_[12] != 0x09 || imagePrefix_[13] != 0x00)) {
      fail("Firmware chip ID is not ESP32-S3");
      return false;
    }
  }
  if (!shaUpdate(&sha_, buffer, size)) {
    fail("SHA-256 calculation failed");
    return false;
  }
  if (!otaLive_ && imagePrefixUsed_ == sizeof(imagePrefix_)) {
    if (esp_ota_begin(target_, expected_, &handle_) != ESP_OK) {
      fail("Unable to erase inactive firmware partition");
      return false;
    }
    otaLive_ = true;
    if (esp_ota_write(handle_, imagePrefix_, sizeof(imagePrefix_)) != ESP_OK) {
      fail("Flash write failed");
      return false;
    }
  }
  if (otaLive_ && size > offset) {
    if (esp_ota_write(handle_, buffer + offset, size - offset) != ESP_OK) {
      fail("Flash write failed");
      return false;
    }
  }
  written_ += static_cast<uint32_t>(size);
  return true;
}

bool MeoOtaManager::finish() {
  if (phase_ != RECEIVING || !otaLive_ || written_ != expected_) {
    fail("Incomplete firmware upload");
    return false;
  }
  uint8_t digest[32] = {};
  if (!shaFinish(&sha_, digest)) {
    fail("SHA-256 calculation failed");
    return false;
  }
  mbedtls_sha256_free(&sha_);
  shaLive_ = false;
  uint8_t mismatch = 0;
  const char *hex = "0123456789abcdef";
  for (size_t i = 0; i < 32; ++i) {
    mismatch |= static_cast<uint8_t>(expectedSha_[2 * i] ^ hex[digest[i] >> 4]);
    mismatch |= static_cast<uint8_t>(expectedSha_[2 * i + 1] ^ hex[digest[i] & 15]);
  }
  if (mismatch) {
    fail("Firmware SHA-256 does not match uploaded file");
    return false;
  }
  const esp_err_t verified = esp_ota_end(handle_);
  otaLive_ = false;
  if (verified != ESP_OK) {
    fail("ESP-IDF rejected firmware image");
    return false;
  }
  if (esp_ota_set_boot_partition(target_) != ESP_OK) {
    fail("Could not activate inactive boot slot");
    return false;
  }
  phase_ = READY_REBOOT;
  snprintf(message_, sizeof(message_), "Verified; reboot scheduled");
  Serial.printf("[OTA] SHA-256 OK, next boot: %s\n", target_->label);
  return true;
}

void MeoOtaManager::confirmRunningImageAfterSelfTest(bool healthy) {
  if (selfTestDone_) return;
  selfTestDone_ = true;
  bootHealthy_ = healthy;
  const esp_partition_t *running = esp_ota_get_running_partition();
  if (!running) return;
  esp_ota_img_states_t state = ESP_OTA_IMG_UNDEFINED;
  if (esp_ota_get_state_partition(running, &state) != ESP_OK ||
      state != ESP_OTA_IMG_PENDING_VERIFY) return;
  if (healthy) {
    const esp_err_t result = esp_ota_mark_app_valid_cancel_rollback();
    Serial.printf("[OTA] First boot health PASS, confirmation result=%d\n", (int)result);
  } else {
    Serial.println("[OTA] First boot health FAILED; requesting rollback");
    const esp_err_t result = esp_ota_mark_app_invalid_rollback_and_reboot();
    Serial.printf("[OTA] Rollback request result=%d; check bootloader configuration\n", (int)result);
  }
}
