#pragma once
#include "esp_partition.h"
#include <cstddef>
#include <cstdint>

using esp_ota_handle_t = uint32_t;

enum esp_ota_img_states_t {
  ESP_OTA_IMG_UNDEFINED = 0,
  ESP_OTA_IMG_PENDING_VERIFY = 1,
};

const esp_partition_t *esp_ota_get_running_partition();
const esp_partition_t *esp_ota_get_next_update_partition(const esp_partition_t *start);
esp_err_t esp_ota_begin(const esp_partition_t *target, size_t size, esp_ota_handle_t *handle);
esp_err_t esp_ota_write(esp_ota_handle_t handle, const void *data, size_t size);
esp_err_t esp_ota_abort(esp_ota_handle_t handle);
esp_err_t esp_ota_end(esp_ota_handle_t handle);
esp_err_t esp_ota_set_boot_partition(const esp_partition_t *target);
esp_err_t esp_ota_get_state_partition(const esp_partition_t *partition, esp_ota_img_states_t *state);
esp_err_t esp_ota_mark_app_valid_cancel_rollback();
esp_err_t esp_ota_mark_app_invalid_rollback_and_reboot();
