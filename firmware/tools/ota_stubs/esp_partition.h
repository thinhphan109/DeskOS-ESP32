#pragma once
#include <cstddef>
#include <cstdint>
struct esp_partition_t {
  const char *label;
  uint32_t size;
};
using esp_err_t = int;
constexpr esp_err_t ESP_OK = 0;
constexpr int ESP_PARTITION_TYPE_DATA = 1;
constexpr int ESP_PARTITION_SUBTYPE_DATA_SPIFFS = 0x82;
const esp_partition_t *esp_partition_find_first(int type, int subtype, const char *label);
esp_err_t esp_partition_read(const esp_partition_t *partition, uint32_t offset,
                             void *dest, size_t length);
esp_err_t esp_partition_write(const esp_partition_t *partition, uint32_t offset,
                              const void *source, size_t length);
esp_err_t esp_partition_erase_range(const esp_partition_t *partition,
                                   uint32_t offset, size_t length);
