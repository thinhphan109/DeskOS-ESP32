#pragma once

#include <Arduino.h>
#include <esp_ota_ops.h>
#include <mbedtls/sha256.h>

// One and only one update engine for Web Studio and Windows Companion.
// All image bytes are streamed into the inactive OTA partition (no PSRAM
// framebuffer or whole-image allocation). This is LOCAL authenticated OTA,
// NOT signed-image verification; never expose HTTP upload to the Internet.
class MeoOtaManager {
 public:
  static constexpr const char *BOARD_ID = "meodesk-esp32s3-n16r8";
  static constexpr uint32_t MIN_IMAGE_BYTES = 4096;

  enum Phase : uint8_t { IDLE, RECEIVING, READY_REBOOT, ERROR };

  bool available() const;
  size_t availableBytes() const;
  bool begin(size_t bytes, const String &sha256, const String &board);
  bool append(const uint8_t *buffer, size_t size);
  bool finish();
  void abort(const char *why);

  Phase phase() const { return phase_; }
  bool busy() const { return phase_ == RECEIVING || phase_ == READY_REBOOT; }
  uint32_t expected() const { return expected_; }
  uint32_t written() const { return written_; }
  const char *message() const { return message_; }
  const char *phaseName() const;
  const char *bootHealth() const {
    return !selfTestDone_ ? "checking" : (bootHealthy_ ? "passed" : "failed");
  }
  const esp_partition_t *targetPartition() const { return target_; }
  void confirmRunningImageAfterSelfTest(bool healthy);

 private:
  void fail(const char *message);
  static bool validSha(const String &value);
  static bool shaStart(mbedtls_sha256_context *ctx);
  static bool shaUpdate(mbedtls_sha256_context *ctx, const uint8_t *data, size_t bytes);
  static bool shaFinish(mbedtls_sha256_context *ctx, uint8_t *digest);

  esp_ota_handle_t handle_ = 0;
  const esp_partition_t *target_ = nullptr;
  uint32_t expected_ = 0;
  uint32_t written_ = 0;
  uint8_t imagePrefix_[16] = {};
  uint8_t imagePrefixUsed_ = 0;
  char expectedSha_[65] = "";
  char message_[96] = "Idle";
  Phase phase_ = IDLE;
  mbedtls_sha256_context sha_{};
  bool shaLive_ = false;
  bool otaLive_ = false;
  bool selfTestDone_ = false;
  bool bootHealthy_ = false;
};

extern MeoOtaManager meoOta;
