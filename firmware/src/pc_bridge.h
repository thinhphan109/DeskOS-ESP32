#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "core/media_state.h"
#include "core/device_profile.h"
#include "core/scene_layout.h"

struct PcTelemetry {
  bool received = false;
  bool demo = false;
  uint32_t lastReceivedMs = 0;
  float cpuPercent = 0;
  float ramPercent = 0;
  float gpuPercent = -1;
  float gpuTempC = -1;
  float cpuTempC = -1;
  float batteryPercent = -1;
  int8_t batteryPlugged = -1; // -1=not available, 0=discharging, 1=charging/AC
  uint32_t pcUptimeS = 0;
  float ramUsedGB = 0;
  float ramTotalGB = 0;
  float vramUsedGB = -1;
  float vramTotalGB = -1;
  float netUpKbps = -1;
  float netDownKbps = -1;
  char pcName[30] = "";
  char gpuName[36] = "";
};

struct HealthSettings {
  uint8_t cpuLimit = 88;
  uint8_t gpuLimit = 92;
  uint8_t tempLimit = 80;
  uint8_t batteryLimit = 20;
  bool quiet = false;
};

struct PcHistoryPoint {
  uint8_t cpu = 255;
  uint8_t gpu = 255;
  uint8_t ram = 255;
};

extern PcTelemetry pcTelemetry;
extern HealthSettings healthSettings;
extern MediaState pcMedia;
const DeviceProfile &pcBridgeProfile();
const SceneLayout &pcBridgeSceneLayout();
// Cover art is volatile PSRAM/internal SRAM, 64x64 RGB565, owned by the bridge. The pointer
// must only be used on the Arduino main loop (HTTP and TFT are serviced there).
const uint16_t *pcBridgeArtworkPixels();
bool pcBridgeArtworkReady();

using UiControlCallback = bool (*)(const String &action, int value);
using UiStatusCallback = void (*)(JsonDocument &doc);
using UiPerformanceCallback = void (*)(JsonDocument &doc);
// -1: leave OTA screen; 0..100: show upload progress on TFT.
using UiOtaModeCallback = void (*)(int percent);
using UiCatPackModeCallback = void (*)(int percent);

void pcBridgeBegin(UiControlCallback onControl, UiStatusCallback onStatus,
                   UiPerformanceCallback onPerformance,
                   UiOtaModeCallback onOtaMode,
                   UiCatPackModeCallback onCatPackMode);
void pcBridgeLoop();
void pcBridgeSetFocus(bool focusActive);
void pcBridgePushEvent(const char *code, const char *message,
                       uint8_t priority = 1, uint32_t ttlMs = 8500);
uint8_t pcBridgeTheme();
bool pcBridgeSaveTheme(uint8_t theme);
uint8_t pcBridgeFpsProfile();
bool pcBridgeSaveFpsProfile(uint8_t profile);
uint8_t pcBridgeMotionStyle();
bool pcBridgeSaveMotionStyle(uint8_t style);
bool pcBridgeLoadSceneSlot(uint8_t slot, uint8_t &page);
bool pcBridgeSaveSceneSlot(uint8_t slot, uint8_t page);
// Pause station connection attempts while the async 2.4 GHz scan is running.
// The fallback AP and HTTP server remain available throughout the scan.
void pcBridgeSetWifiScanActive(bool active);
bool pcTelemetryFresh();
bool pcNetworkConnected();
String pcNetworkIP();
const char *pcEmotionName();
bool pcPreviewActive();
uint16_t pcHistoryCount();
bool pcHistoryAt(uint16_t oldestIndex, PcHistoryPoint &point);
