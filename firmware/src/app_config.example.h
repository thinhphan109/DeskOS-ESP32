#pragma once

// COPY to app_config.h before compiling a new public checkout.
// Wi-Fi may also be configured from the fallback AP's Device Settings page.
// Prefer using an empty SSID here and storing credentials in ESP32 NVS.
static constexpr const char *WIFI_SSID = "";
static constexpr const char *WIFI_PASSWORD = "";

// Change this unique pairing key before sharing your device's Wi-Fi network.
static constexpr const char *MEO_API_KEY = "CHANGE_TO_A_LONG_RANDOM_KEY";

static constexpr const char *MEO_AP_SSID = "MEO-DESK-05";
static constexpr const char *MEO_AP_PASSWORD = "CHANGE_YOUR_AP_PASS";
static constexpr const char *MEO_MDNS_NAME = "meodesk";
static constexpr uint32_t TELEMETRY_TIMEOUT_MS = 6000;
