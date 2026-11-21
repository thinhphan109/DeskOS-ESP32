#pragma once
// Generic firmware only. Existing devices load their bridge-migrated values from NVS.
static constexpr const char *WIFI_SSID = "";
static constexpr const char *WIFI_PASSWORD = "";
static constexpr const char *MEO_API_KEY = "";
static constexpr const char *MEO_AP_SSID = "MIMI-DESK";
static constexpr const char *MEO_AP_PASSWORD = "mimi-desk-setup";
static constexpr const char *MEO_MDNS_NAME = "meodesk";
static constexpr uint32_t TELEMETRY_TIMEOUT_MS = 6000;
