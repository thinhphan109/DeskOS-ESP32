#pragma once

#include <Arduino.h>
#include <esp_partition.h>
#include <mbedtls/sha256.h>

// v1: 16 static RGB565 poses (648 KiB). v2: 16 poses x 3 animation frames
// (1944 KiB). Both use the same dual-bank raw Flash layout and SHA-verified
// commit-last header. The complete active pack resides in PSRAM, not Flash.
class CatPackManager {
 public:
  static constexpr uint32_t WIDTH = 144;
  static constexpr uint32_t HEIGHT = 144;
  static constexpr uint32_t FRAMES = 16;
  static constexpr uint32_t BYTES_PER_FRAME = WIDTH * HEIGHT * 2;
  static constexpr uint32_t PAYLOAD_BYTES = BYTES_PER_FRAME * FRAMES; // Legacy v1
  static constexpr uint32_t MOTION_FRAMES = 3;
  static constexpr uint32_t MOTION_BYTES = PAYLOAD_BYTES * MOTION_FRAMES;
  static constexpr uint32_t BANK_BYTES = 0x1F0000;
  static constexpr uint32_t DATA_OFFSET = 0x1000;
  static_assert(DATA_OFFSET + MOTION_BYTES <= BANK_BYTES, "Motion pack exceeds flash bank");

  enum Phase : uint8_t { IDLE, RECEIVING, READY_REBOOT, ERROR };

  void initialize();
  bool available() const;
  bool availableFor(uint32_t bytes) const;
  bool loaded() const { return pixels_ != nullptr && activeBank_ >= 0; }
  int activeBank() const { return activeBank_; }
  uint32_t generation() const { return generation_; }
  const char *activeSha() const { return activeSha_; }
  const char *message() const { return message_; }
  const char *phaseName() const;
  uint32_t written() const { return written_; }
  uint32_t uploadBytes() const { return uploadBytes_; }
  uint32_t activeBytes() const { return activeBytes_; }
  uint8_t framesPerPose() const { return framesPerPose_; }
  bool busy() const { return phase_ == RECEIVING || phase_ == READY_REBOOT; }
  Phase phase() const { return phase_; }
  const uint16_t *lookup(const uint16_t *builtin) const;
  const uint16_t *frameAt(const uint16_t *builtin, uint8_t cel) const;
  const uint16_t *frame(const uint16_t *builtin, uint32_t nowMs,
                        bool animate) const;

  bool begin(const String &expectedSha, uint32_t bytes = PAYLOAD_BYTES);
  bool append(const uint8_t *bytes, size_t length);
  bool finish();
  void abort(const char *reason);

 private:
  struct __attribute__((packed)) BankHeader {
    char magic[8];
    uint32_t format;
    uint32_t generation;
    uint32_t payloadBytes;
    uint32_t frames;
    uint16_t width;
    uint16_t height;
    uint8_t sha256[32];
  };
  static_assert(sizeof(BankHeader) == 60, "Unstable CAT pack header");

  bool readHeader(int bank, BankHeader &header) const;
  static bool validHex(const String &value);
  static bool shaBegin(mbedtls_sha256_context *context);
  static bool shaAppend(mbedtls_sha256_context *context, const uint8_t *bytes, size_t count);
  static bool shaEnd(mbedtls_sha256_context *context, uint8_t *out);
  static void digestHex(const uint8_t *digest, char *out);
  void fail(const char *reason);

  const esp_partition_t *partition_ = nullptr;
  uint16_t *pixels_ = nullptr;
  int8_t activeBank_ = -1;
  int8_t uploadBank_ = -1;
  uint32_t generation_ = 0;
  uint32_t uploadBytes_ = PAYLOAD_BYTES;
  uint32_t activeBytes_ = 0;
  uint32_t written_ = 0;
  uint8_t framesPerPose_ = 1;
  char activeSha_[65] = "";
  char expectedSha_[65] = "";
  char message_[100] = "Built-in cat sprites";
  Phase phase_ = IDLE;
  bool hashing_ = false;
  mbedtls_sha256_context sha_{};
};

extern CatPackManager catPack;
