#include "cat_pack.h"
#include "core/update_contract.h"

#include <esp_heap_caps.h>
#include <mbedtls/version.h>
#include <stdio.h>
#include <string.h>
#include "cat_assets.h"

CatPackManager catPack;
static constexpr char BANK_MAGIC[8] = {'M','E','O','C','A','T','1','5'};
static constexpr uint32_t STATIC_FORMAT = 1;
static constexpr uint32_t MOTION_FORMAT = 2;

static const uint16_t *const BUILTIN_FRAMES[CatPackManager::FRAMES] = {
  cat_boot, cat_clock, cat_paused, cat_sleep, cat_storage, cat_network,
  cat_offline, cat_music, cat_bambu, cat_audio, cat_health, cat_healthy,
  cat_hotspot, cat_power, cat_rescue, cat_shutdown,
};

const char *CatPackManager::phaseName() const {
  switch (phase_) {
    case RECEIVING: return "receiving";
    case READY_REBOOT: return "rebooting";
    case ERROR: return "error";
    default: return "idle";
  }
}

bool CatPackManager::available() const {
  return availableFor(PAYLOAD_BYTES);
}

bool CatPackManager::availableFor(uint32_t bytes) const {
  if (bytes != PAYLOAD_BYTES && bytes != MOTION_BYTES) return false;
  return partition_ && partition_->size >= 2 * BANK_BYTES &&
         ((pixels_ && activeBytes_ >= bytes) ||
          ESP.getFreePsram() > bytes + 512UL * 1024UL);
}

const uint16_t *CatPackManager::lookup(const uint16_t *builtin) const {
  if (!loaded()) return builtin;
  for (size_t i = 0; i < FRAMES; ++i)
    if (builtin == BUILTIN_FRAMES[i])
      return pixels_ + i * WIDTH * HEIGHT * framesPerPose_;
  return builtin;
}

const uint16_t *CatPackManager::frameAt(const uint16_t *builtin, uint8_t cel) const {
  if (!loaded() || framesPerPose_ == 1) return lookup(builtin);
  if (cel >= framesPerPose_) cel = 0;
  for (size_t i = 0; i < FRAMES; ++i)
    if (builtin == BUILTIN_FRAMES[i])
      return pixels_ + (i * framesPerPose_ + cel) * WIDTH * HEIGHT;
  return builtin;
}

const uint16_t *CatPackManager::frame(const uint16_t *builtin, uint32_t nowMs,
                                      bool animate) const {
  if (!loaded() || framesPerPose_ == 1 || !animate) return lookup(builtin);
  // Ping-pong 0→1→2→1. 170 ms per cel; the TFT continues its independent
  // 20/25/30 Hz DMA frame pacing, keeping motion independent of web traffic.
  constexpr uint8_t order[4] = {0, 1, 2, 1};
  return frameAt(builtin, order[(nowMs / 170U) % 4U]);
}

bool CatPackManager::validHex(const String &sha) {
  if (sha.length() != 64) return false;
  for (size_t i = 0; i < 64; ++i) {
    char c = sha[i];
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
      return false;
  }
  return true;
}

bool CatPackManager::shaBegin(mbedtls_sha256_context *ctx) {
#if MBEDTLS_VERSION_MAJOR >= 3
  return mbedtls_sha256_starts(ctx, 0) == 0;
#else
  return mbedtls_sha256_starts_ret(ctx, 0) == 0;
#endif
}
bool CatPackManager::shaAppend(mbedtls_sha256_context *ctx, const uint8_t *buf, size_t count) {
#if MBEDTLS_VERSION_MAJOR >= 3
  return mbedtls_sha256_update(ctx, buf, count) == 0;
#else
  return mbedtls_sha256_update_ret(ctx, buf, count) == 0;
#endif
}
bool CatPackManager::shaEnd(mbedtls_sha256_context *ctx, uint8_t *out) {
#if MBEDTLS_VERSION_MAJOR >= 3
  return mbedtls_sha256_finish(ctx, out) == 0;
#else
  return mbedtls_sha256_finish_ret(ctx, out) == 0;
#endif
}

void CatPackManager::digestHex(const uint8_t *digest, char *out) {
  static const char *digits = "0123456789abcdef";
  for (size_t i = 0; i < 32; ++i) {
    out[2 * i] = digits[digest[i] >> 4];
    out[2 * i + 1] = digits[digest[i] & 15];
  }
  out[64] = 0;
}

bool CatPackManager::readHeader(int bank, BankHeader &header) const {
  if (!partition_ || bank < 0 || bank > 1 ||
      esp_partition_read(partition_, bank * BANK_BYTES, &header, sizeof(header)) != ESP_OK)
    return false;
  const bool isStatic = header.format == STATIC_FORMAT &&
                        header.payloadBytes == PAYLOAD_BYTES && header.frames == FRAMES;
  const bool isMotion = header.format == MOTION_FORMAT &&
                        header.payloadBytes == MOTION_BYTES &&
                        header.frames == FRAMES * MOTION_FRAMES;
  return memcmp(header.magic, BANK_MAGIC, 8) == 0 &&
         header.generation > 0 && (isStatic || isMotion) &&
         header.width == WIDTH && header.height == HEIGHT;
}

void CatPackManager::initialize() {
  partition_ = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                        ESP_PARTITION_SUBTYPE_DATA_SPIFFS, "assets");
  if (!partition_ || partition_->size < 2 * BANK_BYTES) {
    Serial.println("[CATPACK] Reserved assets partition missing; built-in sprites only");
    return;
  }
  BankHeader headers[2] = {};
  const bool valid[2] = {readHeader(0, headers[0]), readHeader(1, headers[1])};
  if (!valid[0] && !valid[1]) return;
  int newest = valid[1] && (!valid[0] || headers[1].generation > headers[0].generation) ? 1 : 0;
  uint8_t block[1024];
  for (int pass = 0; pass < 2; ++pass) {
    const int bank = pass ? 1 - newest : newest;
    if (!valid[bank]) continue;
    const uint32_t bytes = headers[bank].payloadBytes;
    if (!availableFor(bytes)) continue;
    auto *buffer = static_cast<uint8_t *>(heap_caps_malloc(bytes,
                                MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!buffer) continue;
    mbedtls_sha256_context hash;
    mbedtls_sha256_init(&hash);
    bool good = shaBegin(&hash);
    for (uint32_t i = 0; good && i < bytes; i += sizeof(block)) {
      size_t count = min(static_cast<size_t>(bytes - i), sizeof(block));
      good = esp_partition_read(partition_, bank * BANK_BYTES + DATA_OFFSET + i,
                                block, count) == ESP_OK && shaAppend(&hash, block, count);
      if (good) memcpy(buffer + i, block, count);
    }
    uint8_t digest[32] = {};
    if (good) good = shaEnd(&hash, digest) &&
                     memcmp(digest, headers[bank].sha256, sizeof(digest)) == 0;
    mbedtls_sha256_free(&hash);
    if (!good) {heap_caps_free(buffer);continue;}
    pixels_ = reinterpret_cast<uint16_t *>(buffer);
    activeBank_ = bank;
    activeBytes_ = bytes;
    framesPerPose_ = headers[bank].format == MOTION_FORMAT ? MOTION_FRAMES : 1;
    generation_ = headers[bank].generation;
    digestHex(digest, activeSha_);
    snprintf(message_, sizeof(message_), "Custom cat pack loaded from bank %d", bank);
    Serial.printf("[CATPACK] Loaded bank %d, generation %lu, %u frames/pose from PSRAM\n",
                  bank, static_cast<unsigned long>(generation_), framesPerPose_);
    return;
  }
  Serial.println("[CATPACK] No compatible verified pack/PSRAM; built-in sprites retained");
}

void CatPackManager::fail(const char *reason) {
  if (ownsLease_) { MimiUpdate::coordinator().release(MimiUpdate::Owner::CatPack); ownsLease_ = false; }
  if (hashing_) {mbedtls_sha256_free(&sha_); hashing_ = false;}
  phase_ = ERROR;
  snprintf(message_, sizeof(message_), "%s", reason);
  Serial.printf("[CATPACK] ERROR: %s\n", message_);
}

void CatPackManager::abort(const char *reason) {fail(reason);}

bool CatPackManager::begin(const String &sha, uint32_t bytes) {
  if (busy() || !availableFor(bytes)) return false;
  if (!validHex(sha)) {fail("Invalid SHA-256 header");return false;}
  if (!MimiUpdate::coordinator().acquire(MimiUpdate::Owner::CatPack)) return false;
  ownsLease_ = true;
  uploadBytes_ = bytes;
  uploadBank_ = activeBank_ == 0 ? 1 : 0;
  const uint32_t eraseBytes = DATA_OFFSET + ((bytes + 4095) / 4096) * 4096;
  if (esp_partition_erase_range(partition_, uploadBank_ * BANK_BYTES, eraseBytes) != ESP_OK) {
    fail("Could not erase inactive cat bank");return false;
  }
  mbedtls_sha256_init(&sha_);
  hashing_ = true;
  if (!shaBegin(&sha_)) {fail("SHA-256 init failed");return false;}
  for (size_t i = 0; i < 64; ++i) {
    char c = sha[i];
    expectedSha_[i] = c >= 'A' && c <= 'F' ? c + ('a' - 'A') : c;
  }
  expectedSha_[64] = 0;
  written_ = 0;
  phase_ = RECEIVING;
  snprintf(message_, sizeof(message_), "Uploading cat pack to inactive bank");
  Serial.printf("[CATPACK] Writing bank %d, %lu bytes\n", uploadBank_,
                static_cast<unsigned long>(uploadBytes_));
  return true;
}

bool CatPackManager::append(const uint8_t *bytes, size_t length) {
  if (phase_ != RECEIVING || !hashing_ || !bytes || !length ||
      length > uploadBytes_ - written_) {
    fail("Cat pack chunk/length invalid");return false;
  }
  if (!shaAppend(&sha_, bytes, length) ||
      esp_partition_write(partition_, uploadBank_ * BANK_BYTES + DATA_OFFSET + written_,
                          bytes, length) != ESP_OK) {
    fail("Cat pack flash write failed");return false;
  }
  written_ += static_cast<uint32_t>(length);
  return true;
}

bool CatPackManager::finish() {
  if (phase_ != RECEIVING || written_ != uploadBytes_) {
    fail("Incomplete cat pack upload");return false;
  }
  uint8_t digest[32] = {};
  if (!shaEnd(&sha_, digest)) {fail("SHA-256 finish failed");return false;}
  mbedtls_sha256_free(&sha_);
  hashing_ = false;
  char actual[65];digestHex(digest, actual);
  if (strcmp(actual, expectedSha_)) {fail("Cat pack SHA-256 mismatch");return false;}
  BankHeader header{};
  memcpy(header.magic, BANK_MAGIC, sizeof(header.magic));
  header.format = uploadBytes_ == MOTION_BYTES ? MOTION_FORMAT : STATIC_FORMAT;
  header.generation = generation_ + 1;
  if (header.generation == 0) header.generation = 1;
  header.payloadBytes = uploadBytes_;
  header.frames = FRAMES * (header.format == MOTION_FORMAT ? MOTION_FRAMES : 1);
  header.width = WIDTH;
  header.height = HEIGHT;
  memcpy(header.sha256, digest, sizeof(digest));
  // The bank becomes boot-selectable ONLY after its complete payload was
  // written and verified. The previous valid bank is never erased here.
  if (esp_partition_write(partition_, uploadBank_ * BANK_BYTES,
                          &header, sizeof(header)) != ESP_OK) {
    fail("Cat bank commit header failed");return false;
  }
  phase_ = READY_REBOOT;
  snprintf(message_, sizeof(message_), "Cat pack verified; reload scheduled");
  return true;
}
