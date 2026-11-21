#include "pc_bridge.h"
#include "core/desk_notification.h"
#include "core/utf8_text.h"
#include "ota_manager.h"
#include "cat_pack.h"
#include "server_updater.h"

#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <esp_heap_caps.h>
#include <math.h>
#include <string.h>
#include <time.h>

#ifdef MIMI_PUBLIC_FIRMWARE
#include "app_config.public.h"
#else
#include "app_config.h"
#endif
#include "app_version.h"
#include "cat_assets.h"
#include "mood_filter.h"
#include "core/mood_engine.h"
#include "core/command_queue.h"
#include "core/event_engine.h"
#include "web_ui.h"

PcTelemetry pcTelemetry;
HealthSettings healthSettings;
MediaState pcMedia;
static MoodEngine moodEngine;
static CommandQueue mediaCommands;
static EventEngine deskEvents;
static NotificationGate notificationGate;
const EventEngine::Event *pcBridgeActiveEvent() { return deskEvents.active(millis()); }
static constexpr size_t ART_WIDTH = 64;
static constexpr size_t ART_PIXELS = ART_WIDTH * ART_WIDTH;
static constexpr size_t ART_BYTES = ART_PIXELS * sizeof(uint16_t);
// Album art needs two 8 KiB buffers, but is *not* time critical. Allocate
// them in N16R8 PSRAM (with an internal-RAM fallback) instead of reserving
// 16 KiB of static DRAM even when the music feature is unused.
static uint8_t *artworkStorage = nullptr;
static uint16_t *artworkPixels = nullptr;
static uint8_t *artworkIncoming = nullptr;
static bool artworkInPsram = false;
static char artworkInstalledId[17] = "";
static char artworkUploadId[17] = "";
static size_t artworkUploadBytes = 0;
static bool artworkUploadAccept = false;
static bool artworkUploadComplete = false;
static bool artworkInstalled = false;
static bool focusModeActive = false;
static uint8_t savedTheme = 0;
static uint8_t savedFpsProfile = 1;
static uint8_t savedMotionStyle = 1;
static String wifiSsid;
static String wifiPassword;
static WebServer httpServer(80);
static WebSocketsServer stateSocket(81);
static uint32_t stateRevision = 0;
static uint32_t statePublishedMs = 0;
static bool stateDirty = true;
static String stateBootId;
static JsonDocument otaHistory;
static bool otaHistoryPending = false;
static uint32_t otaHistoryCheckMs = 0;
static bool stateAuthorized[4] = {};
static Preferences prefs;
static bool prefsReady = false;
static String apiKey, apSsid, apPassword;
static DeviceProfile deviceProfile;
static SceneLayout sceneLayout;
static uint32_t sceneLayoutRevision=0;
static RuleSet deviceRules;
static IntegrationFeed integrationFeeds[3];
const IntegrationFeed &pcBridgeFeed(uint8_t slot) {
  static const IntegrationFeed unavailable;
  return slot<3 ? integrationFeeds[slot] : unavailable;
}
static uint32_t rulesRevision=0;
const DeviceProfile &pcBridgeProfile() { return deviceProfile; }
const SceneLayout &pcBridgeSceneLayout() { return sceneLayout; }
const RuleSet &pcBridgeRules() { return deviceRules; }
uint32_t pcBridgeRulesRevision() { return rulesRevision; }
static bool persistProfile(const DeviceProfile &profile, const SceneLayout *layout = nullptr, const RuleSet *rules = nullptr);
static UiControlCallback controlHandler = nullptr;
static UiStatusCallback statusHandler = nullptr;
static UiPerformanceCallback performanceHandler = nullptr;
static UiOtaModeCallback otaModeHandler = nullptr;
static UiCatPackModeCallback catPackModeHandler = nullptr;
static bool otaUploadStarted = false;
static bool otaUploadFinished = false;
static uint32_t otaRebootAtMs = 0;
static bool catPackUploadStarted = false;
static bool catPackUploadFinished = false;
static int catPackLastPercent = -1;
static int otaLastPercent = -1;
static bool mdnsStarted = false;
static bool wifiScanActive = false;
static uint32_t lastStaAttemptMs = 0;
static uint32_t lastStaStatusMs = 0;
static volatile uint8_t lastStaDisconnectReason = 0;
static volatile bool staDisconnectPending = false;
static int previewIndex = -1;
static uint32_t previewUntilMs = 0;

static const char *lastRealMood = "rescue";
static MoodFilter moodFilter;

bool pcBridgeArtworkReady() {
  return artworkPixels && artworkInstalled && pcMedia.available && pcMedia.fresh(millis()) &&
         pcMedia.artworkId[0] &&
         !strcmp(artworkInstalledId, pcMedia.artworkId);
}

static void allocateArtworkBuffers() {
  if (artworkStorage) return;
  const size_t bytes = 2 * ART_BYTES;
  if (ESP.getPsramSize() >= 4UL * 1024 * 1024)
    artworkStorage = (uint8_t *)heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  artworkInPsram = artworkStorage != nullptr;
  if (!artworkStorage)
    artworkStorage = (uint8_t *)heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!artworkStorage) {
    Serial.println("[MEM] Album cover buffers unavailable; metadata still works.");
    return;
  }
  artworkPixels = reinterpret_cast<uint16_t *>(artworkStorage);
  artworkIncoming = artworkStorage + ART_BYTES;
  Serial.printf("[MEM] Album buffers: 16 KB in %s\n", artworkInPsram ? "PSRAM" : "internal SRAM");
}

const uint16_t *pcBridgeArtworkPixels() {
  return pcBridgeArtworkReady() ? artworkPixels : nullptr;
}

// 5 minutes at 1 Hz, encoded as 0..100 or 255 (unknown).
// RAM use: 900 bytes plus counters; never grow the sample buffer dynamically.
static constexpr uint16_t HISTORY_SIZE = 300;
static PcHistoryPoint history[HISTORY_SIZE]{};
static uint16_t historyHead = 0;
static uint16_t historyCount = 0;
static uint32_t historyTickMs = 0;

static uint8_t historyPercent(float value) {
  if (!isfinite(value) || value < 0 || value > 100) return 255;
  return (uint8_t)(value + 0.5f);
}

static void sampleHistory(uint32_t nowMs) {
  if (historyTickMs && (uint32_t)(nowMs - historyTickMs) < 1000) return;
  historyTickMs = nowMs ? nowMs : 1;
  PcHistoryPoint sample{255, 255, 255};
  if (pcTelemetryFresh()) {
    sample.cpu = historyPercent(pcTelemetry.cpuPercent);
    sample.gpu = historyPercent(pcTelemetry.gpuPercent);
    sample.ram = historyPercent(pcTelemetry.ramPercent);
  }
  history[historyHead] = sample;
  historyHead = (historyHead + 1) % HISTORY_SIZE;
  if (historyCount < HISTORY_SIZE) ++historyCount;
}

uint16_t pcHistoryCount() { return historyCount; }

bool pcHistoryAt(uint16_t oldestIndex, PcHistoryPoint &point) {
  if (oldestIndex >= historyCount) return false;
  point = history[(historyHead + HISTORY_SIZE - historyCount + oldestIndex) % HISTORY_SIZE];
  return true;
}

static constexpr const char *PREVIEW_MOODS[8] = {
  "paused", "low_battery", "rescue", "power",
  "sleep", "hot", "healthy", "hotspot"
};

bool pcTelemetryFresh() {
  return pcTelemetry.received &&
         (uint32_t)(millis() - pcTelemetry.lastReceivedMs) < TELEMETRY_TIMEOUT_MS;
}

bool pcNetworkConnected() { return WiFi.status() == WL_CONNECTED; }

bool pcBridgeLoadSceneSlot(uint8_t slot, uint8_t &page) {
  if (slot >= 4) return false;
  page = deviceProfile.scenes[slot];
  return true;
}

bool pcBridgeSaveSceneSlot(uint8_t slot, uint8_t page) {
  if (!prefsReady || slot >= 4 || page >= DeviceProfile::PAGE_COUNT) return false;
  char key[5];
  snprintf(key, sizeof(key), "sc%u", (unsigned)slot);
  deviceProfile.scenes[slot] = page;
  prefs.putUChar(key, page);
  return persistProfile(deviceProfile);
}

// The Wi-Fi event callback runs on another FreeRTOS task. Only record the
// reason here; all Serial output and reconnection happen in the Arduino loop.
static void onWifiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
    lastStaDisconnectReason = info.wifi_sta_disconnected.reason;
    staDisconnectPending = true;
  }
}

void pcBridgeSetWifiScanActive(bool active) {
  if (wifiScanActive == active) return;
  wifiScanActive = active;
  if (active) {
    WiFi.setAutoReconnect(false);
    // Stop STA association attempts, but DO NOT stop the fallback access point.
    WiFi.disconnect(false, false);
    Serial.println("[WIFI] STA paused for scan; fallback AP stays online");
  } else {
    WiFi.setAutoReconnect(true);
    if (wifiSsid.length()) {
      lastStaAttemptMs = millis();
      WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());
      Serial.printf("[WIFI] Scan finished; reconnecting to %s\n", wifiSsid.c_str());
    }
  }
}

String pcNetworkIP() {
  return pcNetworkConnected() ? WiFi.localIP().toString()
                              : WiFi.softAPIP().toString();
}

void pcBridgePushEvent(const char *code, const char *message,
                       uint8_t priority, uint32_t ttlMs) {
  if (priority > 3) priority = 3;
  deskEvents.publish(code, message, static_cast<EventEngine::Priority>(priority),
                     millis(), ttlMs);
}

void pcBridgeSetFocus(bool active) {
  if (focusModeActive == active) return;
  focusModeActive = active;
  pcBridgePushEvent(active ? "focus_started" : "focus_paused",
                    active ? "Focus session started" : "Focus session paused",
                    1, 7000);
}

uint8_t pcBridgeTheme() { return savedTheme; }

bool pcBridgeSaveTheme(uint8_t theme) {
  if (theme > 5) return false;
  savedTheme = theme;
  deviceProfile.theme = theme;
  if (prefsReady) prefs.putUChar("theme", theme);
  return persistProfile(deviceProfile);
}

uint8_t pcBridgeFpsProfile() { return savedFpsProfile; }

bool pcBridgeSaveFpsProfile(uint8_t profile) {
  if (profile > 3) return false;
  savedFpsProfile = profile;
  deviceProfile.fps = profile;
  if (prefsReady) prefs.putUChar("fps", profile);
  return persistProfile(deviceProfile);
}

uint8_t pcBridgeMotionStyle() { return savedMotionStyle; }

bool pcBridgeSaveMotionStyle(uint8_t style) {
  if (style > 2) return false;
  savedMotionStyle = style;
  deviceProfile.style = style;
  if (prefsReady) prefs.putUChar("motionstyle", style);
  return persistProfile(deviceProfile);
}

static const char *rawEmotionName() {
  MoodInputs m;
  m.pcFresh = pcTelemetryFresh();
  m.demo = pcTelemetry.demo;
  m.focus = focusModeActive;
  m.quiet = healthSettings.quiet;
  m.musicPlaying = pcMedia.playingNow(millis());
  m.cpu = pcTelemetry.cpuPercent;
  m.gpu = pcTelemetry.gpuPercent;
  m.cpuTemp = pcTelemetry.cpuTempC;
  m.gpuTemp = pcTelemetry.gpuTempC;
  m.battery = pcTelemetry.batteryPercent;
  m.plugged = pcTelemetry.batteryPlugged;
  m.cpuLimit = healthSettings.cpuLimit;
  m.gpuLimit = healthSettings.gpuLimit;
  m.tempLimit = healthSettings.tempLimit;
  m.batteryLimit = healthSettings.batteryLimit;
  return moodEngine.choose(m, millis(), moodFilter.current());
}

static const char *realEmotionName() {
  return moodFilter.update(rawEmotionName(), millis());
}

static bool previewActive() {
  return previewIndex >= 0 && (int32_t)(previewUntilMs - millis()) > 0;
}

bool pcPreviewActive() { return previewActive(); }

const char *pcEmotionName() {
  return previewActive() ? PREVIEW_MOODS[previewIndex] : realEmotionName();
}

static void trackRealMood() {
  const char *mood = realEmotionName();
  if (!strcmp(mood, lastRealMood)) return;
  lastRealMood = mood;
  const char *description = "PC status changed";
  if (!strcmp(mood, "rescue")) description = "PC stream disconnected";
  else if (!strcmp(mood, "healthy")) description = "PC running normally";
  else if (!strcmp(mood, "low_battery")) description = "Low battery; connect charger";
  else if (!strcmp(mood, "hot")) description = "Temperature threshold reached";
  else if (!strcmp(mood, "stressed")) description = "CPU or GPU load is high";
  else if (!strcmp(mood, "hotspot")) description = "Using ESP32 fallback hotspot";
  else if (!strcmp(mood, "power")) description = "PC power adapter connected";
  else if (!strcmp(mood, "sleep")) description = "PC is resting";
  else if (!strcmp(mood, "paused")) description = "Quiet desk mode enabled";
  else if (!strcmp(mood, "music")) description = "Windows media is playing";
  else if (!strcmp(mood, "demo")) description = "SIMULATED test stream active";
  const uint8_t priority = !strcmp(mood, "low_battery") || !strcmp(mood, "hot") ? 3 :
                           !strcmp(mood, "stressed") || !strcmp(mood, "rescue") ? 2 : 1;
  pcBridgePushEvent(mood, description, priority, priority == 3 ? 15000 : 7000);
}

static const uint16_t *catForEmotion(const String &state) {
  if (state == "stressed") return cat_storage;
  if (state == "sleep") return cat_sleep;
  if (state == "low_battery") return cat_shutdown;
  if (state == "hot") return cat_health;
  if (state == "healthy") return cat_healthy;
  if (state == "paused") return cat_paused;
  if (state == "rescue") return cat_rescue;
  if (state == "power") return cat_power;
  if (state == "hotspot") return cat_hotspot;
  if (state == "demo") return cat_music;
  if (state == "music") return cat_music;
  return cat_rescue;
}

static void replyJson(int code, JsonDocument &data) {
  if (httpServer.method() == HTTP_POST && code >= 200 && code < 300) {
    stateDirty = true;
    ++stateRevision;
  }
  String response;
  serializeJson(data, response);
  httpServer.sendHeader("Cache-Control", "no-store");
  httpServer.send(code, "application/json; charset=utf-8", response);
}

static void saveOtaHistory() {
  if (!prefsReady) return;
  String data; serializeJson(otaHistory, data);
  prefs.putString("ota_history", data);
}

static void recordOtaUpload(const char *sha = nullptr, const char *version = nullptr) {
  JsonArray list = otaHistory.as<JsonArray>();
  if (list.isNull()) list = otaHistory.to<JsonArray>();
  if (list.size() >= 6) list.remove(0);
  JsonObject item = list.add<JsonObject>();
  const esp_partition_t *target = esp_ota_get_next_update_partition(nullptr);
  item["phase"] = "uploaded"; item["version_before"] = MEO_VERSION;
  item["sha256"] = sha ? String(sha) : httpServer.header("X-MEO-OTA-SHA256");
  if(version)item["expected_version"]=version;
  item["target_slot"] = target ? target->label : "N/A";
  item["uptime_ms"] = millis();
  saveOtaHistory();
}

static void settleOtaHistory(uint32_t now) {
  if (!otaHistoryPending || now < 25000UL || static_cast<uint32_t>(now - otaHistoryCheckMs) < 1000UL) return;
  otaHistoryCheckMs = now;
  JsonArray list = otaHistory.as<JsonArray>();
  if (list.size() == 0) { otaHistoryPending = false; return; }
  JsonObject item = list[list.size()-1];
  const esp_partition_t *running = esp_ota_get_running_partition();
  const char *health = meoOta.bootHealth();
  if (running && item["target_slot"] == running->label && !strcmp(health, "passed")) {
    item["phase"] = "confirmed"; item["version_after"] = MEO_VERSION;
  } else if (!strcmp(health, "failed")) item["phase"] = "failed_health";
  else if (now >= 35000UL && running && item["target_slot"] != running->label) item["phase"] = "unconfirmed_slot";
  else return;
  otaHistoryPending = false;
  saveOtaHistory();
}

static void replyError(int code, const char *message) {
  JsonDocument body;
  body["ok"] = false;
  body["error"] = message;
  replyJson(code, body);
}

static void encodeProfile(JsonObject out, const DeviceProfile &p) {
  out["theme"] = p.theme; out["fps_profile"] = p.fps; out["motion_style"] = p.style;
  out["motion"] = p.motion; out["auto_pages"] = p.autoPages; out["quiet"] = p.quiet;
  out["focus_work_min"] = p.workMin; out["focus_break_min"] = p.breakMin; out["focus_goal"] = p.goal;
  out["cpu_limit"] = p.cpu; out["gpu_limit"] = p.gpu;
  out["temp_limit"] = p.temperature; out["battery_limit"] = p.battery;
  JsonArray scenes = out["scene_slots"].to<JsonArray>();
  for (uint8_t i = 0; i < 4; ++i) scenes.add(p.scenes[i]);
}

static void encodeLayout(JsonObject out, const SceneLayout &layout) {
  out["schema"] = 1;
  JsonArray widgets = out["widgets"].to<JsonArray>();
  for (uint8_t i=0;i<layout.count;++i) {
    const auto &w=layout.widgets[i]; JsonObject item=widgets.add<JsonObject>();
    item["kind"]=static_cast<uint8_t>(w.kind); item["x"]=w.bounds.x; item["y"]=w.bounds.y;
    item["w"]=w.bounds.width; item["h"]=w.bounds.height; item["label"]=w.label;
  }
}

static bool decodeLayout(JsonObjectConst input, SceneLayout &layout) {
  if (!input["schema"].is<int>() || input["schema"]!=1 || !input["widgets"].is<JsonArrayConst>() ||
      input["widgets"].size()<1 || input["widgets"].size()>SceneLayout::CAPACITY) return false;
  layout.count=input["widgets"].size();
  for(uint8_t i=0;i<layout.count;++i) {
    JsonObjectConst item=input["widgets"][i];
    for(const char *key : {"kind","x","y","w","h"}) if(!item[key].is<int>()) return false;
    const int kind=item["kind"], x=item["x"], y=item["y"], w=item["w"], h=item["h"];
    if(kind<0||kind>10||x<0||x>161||y<0||y>119||w<32||w>162||h<20||h>120||
       !item["label"].is<const char*>()||strlen(item["label"].as<const char*>())>31) return false;
    auto &widget=layout.widgets[i]; widget.kind=static_cast<WidgetKind>(kind);
    widget.bounds={static_cast<int16_t>(x),static_cast<int16_t>(y),static_cast<uint16_t>(w),static_cast<uint16_t>(h)};
    snprintf(widget.label,sizeof(widget.label),"%s",item["label"].as<const char*>());
  }
  return layout.valid();
}

static const char *ruleWhens[]={"cpu_above","gpu_above","hour","focus_complete","idle_above","music_playing"};
static const char *ruleActions[]={"scene","theme","celebrate","notify"};
static void encodeRules(JsonObject out,const RuleSet &set) {
  out["schema"]=1;JsonArray list=out["rules"].to<JsonArray>();
  for(uint8_t i=0;i<set.count;++i) {
    const auto &r=set.rules[i];JsonObject item=list.add<JsonObject>();
    item["enabled"]=r.enabled;item["when"]=ruleWhens[static_cast<uint8_t>(r.when)];
    item["threshold"]=r.threshold;item["hold_s"]=r.holdSeconds;item["cooldown_s"]=r.cooldownSeconds;
    item["do"]=ruleActions[static_cast<uint8_t>(r.action)];item["value"]=r.value;item["label"]=r.label;
  }
}
static bool decodeRules(JsonObjectConst input,RuleSet &set) {
  if(!input["schema"].is<int>()||input["schema"]!=1||!input["rules"].is<JsonArrayConst>()||input["rules"].size()>RuleSet::CAPACITY)return false;
  set.count=input["rules"].size();
  for(uint8_t i=0;i<set.count;++i) {
    JsonObjectConst item=input["rules"][i];
    if(!item["enabled"].is<bool>()||!item["when"].is<const char*>()||!item["do"].is<const char*>()||
       !item["label"].is<const char*>()||strlen(item["label"].as<const char*>())>32)return false;
    int when=-1,action=-1;
    for(int j=0;j<6;++j)if(!strcmp(item["when"].as<const char*>(),ruleWhens[j]))when=j;
    for(int j=0;j<4;++j)if(!strcmp(item["do"].as<const char*>(),ruleActions[j]))action=j;
    if(when<0||action<0)return false;
    for(const char *key:{"threshold","hold_s","cooldown_s","value"})if(!item[key].is<int>()||item[key].as<int>()<0)return false;
    if(item["threshold"].as<int>()>7200||item["hold_s"].as<int>()>600||item["cooldown_s"].as<int>()>86400||item["value"].as<int>()>14)return false;
    auto &r=set.rules[i];r.enabled=item["enabled"];r.when=static_cast<RuleWhen>(when);r.action=static_cast<RuleDo>(action);
    r.threshold=item["threshold"];r.holdSeconds=item["hold_s"];r.cooldownSeconds=item["cooldown_s"];r.value=item["value"];
    snprintf(r.label,sizeof(r.label),"%s",item["label"].as<const char*>());
  }
  return set.valid();
}

static bool decodeProfile(JsonObjectConst in, DeviceProfile &p) {
  const char *numbers[] = {"theme", "fps_profile", "motion_style", "focus_work_min",
    "focus_break_min", "focus_goal", "cpu_limit", "gpu_limit", "temp_limit", "battery_limit"};
  for (const char *name : numbers)
    if (!in[name].is<int>() || in[name].as<int>() < 0 || in[name].as<int>() > 255) return false;
  if (!in["motion"].is<bool>() || !in["auto_pages"].is<bool>() || !in["quiet"].is<bool>() ||
      !in["scene_slots"].is<JsonArrayConst>() || in["scene_slots"].size() != 4) return false;
  p.theme = in["theme"]; p.fps = in["fps_profile"]; p.style = in["motion_style"];
  p.workMin = in["focus_work_min"]; p.breakMin = in["focus_break_min"]; p.goal = in["focus_goal"];
  p.cpu = in["cpu_limit"]; p.gpu = in["gpu_limit"]; p.temperature = in["temp_limit"]; p.battery = in["battery_limit"];
  p.motion = in["motion"]; p.autoPages = in["auto_pages"]; p.quiet = in["quiet"];
  for (uint8_t i = 0; i < 4; ++i) {
    if (!in["scene_slots"][i].is<int>() || in["scene_slots"][i].as<int>() < 0 ||
        in["scene_slots"][i].as<int>() >= DeviceProfile::PAGE_COUNT) return false;
    p.scenes[i] = in["scene_slots"][i];
  }
  return p.valid();
}

static bool persistProfile(const DeviceProfile &profile, const SceneLayout *layout, const RuleSet *rules) {
  if (!prefsReady || !profile.valid()) return false;
  JsonDocument doc; doc["schema"] = DeviceProfile::SCHEMA;
  encodeProfile(doc["profile"].to<JsonObject>(), profile);
  encodeLayout(doc["scene_layout"].to<JsonObject>(), layout ? *layout : sceneLayout);
  encodeRules(doc["rules"].to<JsonObject>(),rules ? *rules : deviceRules);
  String data; serializeJson(doc, data);
  // One NVS value: a failed write cannot partially restore individual fields.
  if (prefs.getString("profile_v1", "") == data) return true;
  return prefs.putString("profile_v1", data) == data.length();
}

static void applyProfileMetadata() {
  savedTheme = deviceProfile.theme; savedFpsProfile = deviceProfile.fps; savedMotionStyle = deviceProfile.style;
  healthSettings.cpuLimit = deviceProfile.cpu; healthSettings.gpuLimit = deviceProfile.gpu;
  healthSettings.tempLimit = deviceProfile.temperature; healthSettings.batteryLimit = deviceProfile.battery;
  healthSettings.quiet = deviceProfile.quiet;
}

static void captureRuntimeProfile() {
  if (statusHandler) {
    JsonDocument status; statusHandler(status);
    deviceProfile.motion = status["motion"] | true;
    deviceProfile.autoPages = status["auto_pages"] | false;
  }
  deviceProfile.cpu = healthSettings.cpuLimit; deviceProfile.gpu = healthSettings.gpuLimit;
  deviceProfile.temperature = healthSettings.tempLimit; deviceProfile.battery = healthSettings.batteryLimit;
  deviceProfile.quiet = healthSettings.quiet;
}

static bool authorized() {
  if (httpServer.header("X-MEO-KEY") == apiKey) return true;
  replyError(401, "invalid pairing key");
  return false;
}

static void exportConfiguration() {
  if (!authorized()) return;
  captureRuntimeProfile();
  JsonDocument doc; doc["schema"] = DeviceProfile::SCHEMA;
  doc["board"] = MeoOtaManager::BOARD_ID;
  doc["firmware_version"] = MEO_VERSION;
  encodeProfile(doc["profile"].to<JsonObject>(), deviceProfile);
  encodeLayout(doc["scene_layout"].to<JsonObject>(), sceneLayout);
  encodeRules(doc["rules"].to<JsonObject>(),deviceRules);
  replyJson(200, doc);
}

static void receiveNotification() {
  if(!authorized()) return;
  const String payload=httpServer.arg("plain"); JsonDocument body;
  if(payload.length()>768 || deserializeJson(body,payload) || !body["source"].is<const char*>()) {
    replyError(400,"invalid notification"); return;
  }
  DeskNotification note;
  const char *source=body["source"];
  if(strlen(source)>20) { replyError(400,"invalid notification source"); return; }
  snprintf(note.source,sizeof(note.source),"%s",source);
  if(!body["hidden"].isNull()) {
    if(!body["hidden"].is<bool>()) { replyError(400,"hidden must be boolean"); return; }
    note.hidden=body["hidden"];
  }
  for(const char *key : {"priority","ttl_s"}) {
    if(!body[key].isNull() && (!body[key].is<int>() || body[key].as<int>()<0 || body[key].as<int>()>255)) {
      replyError(400,"invalid notification options"); return;
    }
  }
  note.priority=body["priority"] | 1; note.ttlSeconds=body["ttl_s"] | 8;
  if(!note.hidden) {
    if(!body["text"].is<const char*>() || strlen(body["text"].as<const char*>())>60) {
      replyError(400,"text must be 1..60 printable ASCII characters"); return;
    }
    snprintf(note.text,sizeof(note.text),"%s",body["text"].as<const char*>());
  }
  if(!note.valid()) { replyError(400,"invalid notification source, text, priority or TTL"); return; }
  if(!healthSettings.quiet && !notificationGate.accept(millis())) {
    replyError(429,"wait two seconds before another notification"); return;
  }
  if(!healthSettings.quiet) {
    char code[25]; snprintf(code,sizeof(code),"ext_%s",note.source);
    pcBridgePushEvent(code,note.displayText(),note.priority,static_cast<uint32_t>(note.ttlSeconds)*1000UL);
  }
  JsonDocument result;result["ok"]=true;result["suppressed"]=healthSettings.quiet;
  replyJson(200,result);
}

static void sendOtaHistory() {
  if (!authorized()) return;
  JsonDocument doc; doc["schema"] = 1; doc["updates"] = otaHistory.as<JsonArray>();
  replyJson(200, doc);
}

static void importConfiguration() {
  if (!authorized()) return;
  const String payload = httpServer.arg("plain");
  if (payload.length() == 0 || payload.length() > 4096) { replyError(413, "configuration must be 1..4096 bytes"); return; }
  JsonDocument input;
  DeviceProfile candidate;
  if (deserializeJson(input, payload) || !input["schema"].is<int>() || input["schema"] != DeviceProfile::SCHEMA ||
      input["board"] != MeoOtaManager::BOARD_ID || !decodeProfile(input["profile"].as<JsonObjectConst>(), candidate)) {
    replyError(400, "invalid or incompatible configuration"); return;
  }
  if (!controlHandler) { replyError(503, "renderer unavailable"); return; }
  SceneLayout candidateLayout=sceneLayout;
  if(!input["scene_layout"].isNull() &&
     !decodeLayout(input["scene_layout"].as<JsonObjectConst>(),candidateLayout)) { replyError(400,"invalid scene layout"); return; }
  RuleSet candidateRules=deviceRules;
  if(!input["rules"].isNull() && !decodeRules(input["rules"].as<JsonObjectConst>(),candidateRules)) { replyError(400,"invalid automation rules"); return; }
  if (!persistProfile(candidate,&candidateLayout,&candidateRules)) { replyError(507, "configuration was not persisted; current settings retained"); return; }
  deviceProfile = candidate;
  sceneLayout = candidateLayout;
  ++sceneLayoutRevision;
  deviceRules=candidateRules;if(!input["rules"].isNull())++rulesRevision;
  applyProfileMetadata();
  controlHandler("profile", 0);
  JsonDocument result; result["ok"] = true; result["persisted"] = true;
  replyJson(200, result);
}

static void configureFocus() {
  if (!authorized()) return;
  const String payload = httpServer.arg("plain");
  JsonDocument input;
  if (payload.length() > 160 || deserializeJson(input, payload) ||
      !input["work_min"].is<int>() || !input["break_min"].is<int>() || !input["goal"].is<int>()) {
    replyError(400, "invalid Focus configuration"); return;
  }
  const int work = input["work_min"], rest = input["break_min"], goal = input["goal"];
  if (work < 1 || work > 180 || rest < 1 || rest > 60 || goal < 1 || goal > 24) {
    replyError(400, "Focus ranges: work 1..180, break 1..60, goal 1..24"); return;
  }
  DeviceProfile candidate = deviceProfile;
  candidate.workMin = work; candidate.breakMin = rest; candidate.goal = goal;
  if (!controlHandler || !persistProfile(candidate)) { replyError(507, "Focus configuration not saved"); return; }
  deviceProfile = candidate;
  controlHandler("profile", 0);
  JsonDocument answer; answer["ok"] = true; answer["schema"] = 1;
  if (statusHandler) statusHandler(answer);
  replyJson(200, answer);
}

static void exportSceneLayout() {
  if(!authorized()) return;
  JsonDocument doc; encodeLayout(doc.to<JsonObject>(),sceneLayout); doc["board"]=MeoOtaManager::BOARD_ID;
  replyJson(200,doc);
}
static void importSceneLayout() {
  if(!authorized()) return;
  const String payload=httpServer.arg("plain");
  JsonDocument input; SceneLayout candidate;
  if(payload.length()>1600 || deserializeJson(input,payload) || !decodeLayout(input.as<JsonObjectConst>(),candidate)) {
    replyError(400,"invalid layout: 1..6 non-overlapping widgets inside 162x120"); return;
  }
  if(!persistProfile(deviceProfile,&candidate)) { replyError(507,"layout not persisted; current layout retained"); return; }
  sceneLayout=candidate; ++sceneLayoutRevision;
  if(controlHandler) controlHandler("refresh",0);
  JsonDocument doc; doc["ok"]=true; doc["persisted"]=true; replyJson(200,doc);
}

static void exportRules() {
  if(!authorized())return;
  JsonDocument result;encodeRules(result.to<JsonObject>(),deviceRules);replyJson(200,result);
}

static void sendFeed(uint8_t slot) {
  if(!authorized())return;
  const auto &feed=integrationFeeds[slot];const uint32_t now=millis();const bool fresh=feed.fresh(now);
  JsonDocument result;result["schema"]=1;result["slot"]=slot;result["name"]=feed.name;result["fresh"]=fresh;
  if(fresh){result["value"]=feed.value;result["unit"]=feed.unit;}else{result["value"]=nullptr;result["unit"]=nullptr;}
  result["ttl_s"]=feed.ttlSeconds;
  if(feed.received)result["age_ms"]=static_cast<uint32_t>(now-feed.updatedMs);else result["age_ms"]=nullptr;
  replyJson(200,result);
}
static void receiveFeed(uint8_t slot) {
  if(!authorized())return;
  const String payload=httpServer.arg("plain");JsonDocument body;IntegrationFeed candidate;
  if(payload.length()>512||deserializeJson(body,payload)||!body["ttl_s"].is<int>()||
     body["ttl_s"].as<int>()<1||body["ttl_s"].as<int>()>300) { replyError(400,"invalid feed or TTL");return; }
  for(const char *key:{"name","value","unit"})if(!body[key].is<const char*>()){replyError(400,"feed text fields are required");return;}
  const char *name=body["name"],*value=body["value"],*unit=body["unit"];
  if(strlen(name)>16||strlen(value)>20||strlen(unit)>8){replyError(400,"feed text exceeds display limits");return;}
  snprintf(candidate.name,sizeof(candidate.name),"%s",name);snprintf(candidate.value,sizeof(candidate.value),"%s",value);snprintf(candidate.unit,sizeof(candidate.unit),"%s",unit);
  candidate.ttlSeconds=body["ttl_s"];
  if(!candidate.valid()){replyError(400,"feed fields require printable ASCII");return;}
  candidate.received=true;candidate.updatedMs=millis();integrationFeeds[slot]=candidate;
  JsonDocument result;result["ok"]=true;result["slot"]=slot;result["ttl_s"]=candidate.ttlSeconds;replyJson(200,result);
}
static void importRules() {
  if(!authorized())return;
  const String payload=httpServer.arg("plain");JsonDocument body;RuleSet candidate;
  if(payload.length()>2048||deserializeJson(body,payload)||!decodeRules(body.as<JsonObjectConst>(),candidate)) {
    replyError(400,"invalid rules: at most six bounded conditions/actions");return;
  }
  captureRuntimeProfile();
  if(!persistProfile(deviceProfile,nullptr,&candidate)){replyError(507,"rules were not persisted");return;}
  deviceRules=candidate;++rulesRevision;
  JsonDocument result;result["ok"]=true;result["persisted"]=true;replyJson(200,result);
}

static bool validPercent(float value) {
  return isfinite(value) && value >= 0.0f && value <= 100.0f;
}

static bool validArtId(const char *value) {
  if (!value || !*value) return false;
  if (strlen(value) != 16) return false;
  for (const char *s = value; *s; ++s)
    if ((*s < '0' || *s > '9') && (*s < 'a' || *s > 'f')) return false;
  return true;
}

static bool optionalMetric(JsonDocument &doc, const char *name, float cap, float &output) {
  if (doc[name].isNull()) { output = -1; return true; }
  if (!doc[name].is<float>() && !doc[name].is<double>() &&
      !doc[name].is<int>() && !doc[name].is<uint32_t>()) return false;
  const float value = doc[name].as<float>();
  if (!isfinite(value) || value < 0 || value > cap) return false;
  output = value;
  return true;
}

static void sendMedia() {
  if (!authorized()) return;
  JsonDocument doc;
  const bool fresh = pcMedia.fresh(millis());
  doc["available"] = fresh && pcMedia.available;
  doc["playing"] = fresh && pcMedia.playingNow(millis());
  doc["stale"] = !fresh;
  doc["source_age_ms"]=pcMedia.received ? static_cast<uint32_t>(millis()-pcMedia.receivedMs) : 10000UL;
  doc["title"] = fresh && pcMedia.available ? pcMedia.title : "";
  doc["artist"] = fresh && pcMedia.available ? pcMedia.artist : "";
  doc["position_ms"] = fresh && pcMedia.available ? pcMedia.positionAt(millis()) : 0;
  doc["duration_ms"] = fresh && pcMedia.available ? pcMedia.durationMs : 0;
  doc["artwork_id"] = fresh && pcMedia.available ? pcMedia.artworkId : "";
  doc["artwork_ready"] = pcBridgeArtworkReady();
  replyJson(200, doc);
}

static void receiveMedia() {
  if (!authorized()) return;
  const String payload = httpServer.arg("plain");
  if (!payload.length() || payload.length() > 512) {
    replyError(413, "media JSON must be 1..512 bytes");
    return;
  }
  JsonDocument body;
  if (deserializeJson(body, payload) || !body["available"].is<bool>() ||
      !body["playing"].is<bool>()) {
    replyError(400, "invalid media state");
    return;
  }
  const char *title = body["title"] | "";
  const char *artist = body["artist"] | "";
  const char *artId = body["artwork_id"] | "";
  if (strlen(title) > 64 || strlen(artist) > 48 || !validMediaText(title) || !validMediaText(artist) ||
      (artId[0] && !validArtId(artId)) ||
      !body["position_ms"].is<uint32_t>() || !body["duration_ms"].is<uint32_t>() ||
      body["duration_ms"].as<uint32_t>() > 86400000UL ||
      body["position_ms"].as<uint32_t>() > 86400000UL) {
    replyError(400, "media field too long or invalid");
    return;
  }
  MediaState fresh;
  fresh.received = true;
  fresh.receivedMs = millis();
  fresh.available = body["available"].as<bool>();
  fresh.playing = fresh.available && body["playing"].as<bool>();
  fresh.positionMs = body["position_ms"].as<uint32_t>();
  fresh.durationMs = body["duration_ms"].as<uint32_t>();
  snprintf(fresh.title, sizeof(fresh.title), "%s", title);
  snprintf(fresh.artist, sizeof(fresh.artist), "%s", artist);
  snprintf(fresh.artworkId, sizeof(fresh.artworkId), "%s", artId);
  const bool oldPlaying = pcMedia.playingNow(millis());
  const bool songChanged = fresh.available && strcmp(pcMedia.title, fresh.title) != 0;
  pcMedia = fresh;
  if (fresh.playing && (!oldPlaying || songChanged)) {
    pcBridgePushEvent("music_started", fresh.title[0] ? fresh.title : "Music started", 1, 7000);
  } else if (oldPlaying && !fresh.playing) {
    pcBridgePushEvent("music_paused", "Music paused", 0, 5000);
  }
  JsonDocument response;
  response["ok"] = true;
  replyJson(200, response);
}

// Receive one bounded RGB565 64x64 image as a multipart file upload. The
// WebServer streams parts through HTTPUpload; no 8KB String/body is allocated.
// A staging buffer ensures incomplete/unauthorized uploads never replace the
// cover currently shown. No image writes to flash/NVS.
static void receiveArtworkUpload() {
  HTTPUpload &part = httpServer.upload();
  if (part.status == UPLOAD_FILE_START) {
    artworkUploadBytes = 0;
    artworkUploadComplete = false;
    artworkUploadId[0] = '\0';
    const String id = httpServer.header("X-MEO-ART-ID");
    artworkUploadAccept = artworkPixels && artworkIncoming &&
                          httpServer.header("X-MEO-KEY") == apiKey &&
                          validArtId(id.c_str()) && pcMedia.fresh(millis()) &&
                          !strcmp(pcMedia.artworkId, id.c_str());
    if (artworkUploadAccept)
      snprintf(artworkUploadId, sizeof(artworkUploadId), "%s", id.c_str());
  } else if (part.status == UPLOAD_FILE_WRITE) {
    if (!artworkUploadAccept || artworkUploadBytes + part.currentSize > ART_BYTES) {
      artworkUploadAccept = false;
      return;
    }
    memcpy(artworkIncoming + artworkUploadBytes, part.buf, part.currentSize);
    artworkUploadBytes += part.currentSize;
  } else if (part.status == UPLOAD_FILE_END) {
    if (artworkUploadAccept && artworkUploadBytes == ART_BYTES &&
        !strcmp(pcMedia.artworkId, artworkUploadId)) {
      memcpy(artworkPixels, artworkIncoming, ART_BYTES);
      snprintf(artworkInstalledId, sizeof(artworkInstalledId), "%s", artworkUploadId);
      artworkInstalled = true;
      artworkUploadComplete = true;
    }
    artworkUploadAccept = false;
  } else if (part.status == UPLOAD_FILE_ABORTED) {
    artworkUploadAccept = false;
    artworkUploadComplete = false;
  }
}

static void finishArtworkUpload() {
  if (!authorized()) return;
  if (!artworkUploadComplete) {
    replyError(400, "artwork must be a 64x64 RGB565 file matching the active track");
    return;
  }
  artworkUploadComplete = false;
  if (controlHandler) controlHandler("refresh_music", 0);
  JsonDocument response;
  response["ok"] = true;
  response["artwork_id"] = artworkInstalledId;
  replyJson(200, response);
}

static void sendArtworkBmp() {
  if (!authorized()) return;
  if (!pcBridgeArtworkReady()) {
    replyError(404, "no artwork for active track");
    return;
  }
  // 24-bit BMP, 64x64; one 192-byte BGR scanline on the stack.
  constexpr uint32_t rowBytes = ART_WIDTH * 3;
  constexpr uint32_t fileSize = 54 + rowBytes * ART_WIDTH;
  uint8_t header[54] = {'B', 'M'};
  auto put16 = [&header](uint8_t i, uint16_t n) {
    header[i] = n & 255; header[i + 1] = n >> 8;
  };
  auto put32 = [&header](uint8_t i, uint32_t n) {
    for (uint8_t j = 0; j < 4; ++j) header[i + j] = (n >> (8 * j)) & 255;
  };
  put32(2, fileSize); put32(10, 54); put32(14, 40);
  put32(18, ART_WIDTH); put32(22, ART_WIDTH);
  put16(26, 1); put16(28, 24); put32(34, rowBytes * ART_WIDTH);
  httpServer.sendHeader("Cache-Control", "no-store");
  httpServer.setContentLength(fileSize);
  httpServer.send(200, "image/bmp", "");
  auto client = httpServer.client();
  client.write(header, sizeof(header));
  uint8_t row[rowBytes];
  for (int y = (int)ART_WIDTH - 1; y >= 0; --y) {
    for (uint32_t x = 0; x < ART_WIDTH; ++x) {
      const uint16_t c = artworkPixels[y * ART_WIDTH + x];
      const uint8_t r = (c >> 11) & 31;
      const uint8_t g = (c >> 5) & 63;
      const uint8_t b = c & 31;
      row[x * 3] = (b << 3) | (b >> 2);
      row[x * 3 + 1] = (g << 2) | (g >> 4);
      row[x * 3 + 2] = (r << 3) | (r >> 2);
    }
    client.write(row, rowBytes);
  }
}

static void sendCommands() {
  if (!authorized()) return;
  JsonDocument response;
  response["schema"] = 1;
  const uint32_t after = (uint32_t)httpServer.arg("after").toInt();
  const CommandQueue::Command *command = mediaCommands.get(millis());
  if (command && command->seq > after) {
    JsonObject item = response["command"].to<JsonObject>();
    item["seq"] = command->seq;
    item["action"] = command->action;
    item["ttl_ms"] = 8000UL - (uint32_t)(millis() - command->createdMs);
  } else response["command"] = nullptr;
  replyJson(200, response);
}

static void ackCommand() {
  if (!authorized()) return;
  const String payload = httpServer.arg("plain");
  if (payload.length() > 96) { replyError(413, "ACK too large"); return; }
  JsonDocument body;
  if (deserializeJson(body, payload) || !body["seq"].is<uint32_t>() ||
      !body["ok"].is<bool>()) { replyError(400, "invalid ACK"); return; }
  if (!mediaCommands.ack(body["seq"].as<uint32_t>(), body["ok"].as<bool>(), millis())) {
    replyError(409, "command expired or sequence mismatch");
    return;
  }
  JsonDocument answer;
  answer["ok"] = true;
  answer["executed"] = mediaCommands.lastResult();
  replyJson(200, answer);
}

static void configureWifi() {
  if (!authorized()) return;
  if (wifiScanActive) { replyError(409, "wait for Wi-Fi scan to finish"); return; }
  const String payload = httpServer.arg("plain");
  if (!payload.length() || payload.length() > 200) {
    replyError(413, "Wi-Fi JSON too large"); return;
  }
  JsonDocument body;
  if (deserializeJson(body, payload) || !body["ssid"].is<const char *>() ||
      !body["password"].is<const char *>()) {
    replyError(400, "invalid Wi-Fi settings"); return;
  }
  String ssid = body["ssid"].as<String>();
  String password = body["password"].as<String>();
  if (!ssid.length() || ssid.length() > 32 || (password.length() != 0 &&
      (password.length() < 8 || password.length() > 63))) {
    replyError(400, "SSID 1..32 bytes, password empty or 8..63 bytes"); return;
  }
  if (!prefsReady) { replyError(503, "NVS unavailable"); return; }
  if (prefs.putString("ssid", ssid) != ssid.length() ||
      prefs.putString("wifi_pw", password) != password.length()) {
    replyError(507, "failed to persist network settings"); return;
  }
  wifiSsid = ssid;
  wifiPassword = password;
  WiFi.disconnect(false, false);   // Leave fallback AP on throughout reconnection.
  lastStaAttemptMs = millis();
  WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());
  Serial.printf("[WIFI] Updated network: %s; trying router (fallback AP retained)\n", wifiSsid.c_str());
  JsonDocument answer;
  answer["ok"] = true;
  answer["ssid"] = wifiSsid;
  answer["fallback_ap"] = WiFi.softAPIP().toString();
  replyJson(200, answer);
}

static void receiveTelemetry() {
  if (!authorized()) return;
  const String payload = httpServer.arg("plain");
  if (payload.length() == 0 || payload.length() > 1100) {
    replyError(413, "telemetry JSON must be 1..1100 bytes");
    return;
  }

  JsonDocument doc;
  if (deserializeJson(doc, payload)) {
    replyError(400, "invalid JSON");
    return;
  }
  const float cpu = doc["cpu_percent"] | -1.0f;
  const float ram = doc["ram_percent"] | -1.0f;
  if (!validPercent(cpu) || !validPercent(ram)) {
    replyError(400, "cpu_percent and ram_percent must be 0..100");
    return;
  }

  float gpu = -1.0f;
  if (!doc["gpu_percent"].isNull()) {
    gpu = doc["gpu_percent"].as<float>();
    if (!validPercent(gpu)) {
      replyError(400, "gpu_percent must be null or 0..100");
      return;
    }
  }
  float gpuTemp = -1.0f;
  if (!doc["gpu_temp_c"].isNull()) {
    gpuTemp = doc["gpu_temp_c"].as<float>();
    if (!isfinite(gpuTemp) || gpuTemp < -30 || gpuTemp > 160) {
      replyError(400, "invalid gpu_temp_c");
      return;
    }
  }

  float cpuTemp = -1.0f;
  if (!doc["cpu_temp_c"].isNull()) {
    cpuTemp = doc["cpu_temp_c"].as<float>();
    if (!isfinite(cpuTemp) || cpuTemp < -30 || cpuTemp > 160) {
      replyError(400, "invalid cpu_temp_c");
      return;
    }
  }
  float battery = -1.0f;
  if (!doc["battery_percent"].isNull()) {
    battery = doc["battery_percent"].as<float>();
    if (!validPercent(battery)) {
      replyError(400, "battery_percent must be null or 0..100");
      return;
    }
  }
  int8_t plugged = -1;
  if (!doc["battery_plugged"].isNull()) {
    if (!doc["battery_plugged"].is<bool>()) {
      replyError(400, "battery_plugged must be null or boolean");
      return;
    }
    plugged = doc["battery_plugged"].as<bool>() ? 1 : 0;
  }
  float netUp = -1, netDown = -1, vramUsed = -1, vramTotal = -1;
  if (!optionalMetric(doc, "net_up_kbps", 10000000, netUp) ||
      !optionalMetric(doc, "net_down_kbps", 10000000, netDown) ||
      !optionalMetric(doc, "vram_used_gb", 256, vramUsed) ||
      !optionalMetric(doc, "vram_total_gb", 256, vramTotal) ||
      (vramUsed >= 0 && vramTotal >= 0 && vramUsed > vramTotal)) {
    replyError(400, "invalid network or VRAM fields"); return;
  }

  // Commit atomically after checking the entire payload.
  int32_t idleSeconds=-1;
  if(!doc["pc_idle_s"].isNull()) {
    if(!doc["pc_idle_s"].is<int>() || doc["pc_idle_s"].as<int>()<0 || doc["pc_idle_s"].as<int>()>604800) {
      replyError(400,"pc_idle_s must be null or integer 0..604800"); return;
    }
    idleSeconds=doc["pc_idle_s"];
  }
  PcTelemetry fresh;
  fresh.received = true;
  fresh.demo = doc["demo"] | false;
  fresh.lastReceivedMs = millis();
  fresh.cpuPercent = cpu;
  fresh.ramPercent = ram;
  fresh.gpuPercent = gpu;
  fresh.gpuTempC = gpuTemp;
  fresh.cpuTempC = cpuTemp;
  fresh.batteryPercent = battery;
  fresh.batteryPlugged = plugged;
  fresh.pcUptimeS = doc["pc_uptime_s"] | 0UL;
  fresh.idleSeconds = fresh.demo ? -1 : idleSeconds;
  fresh.ramUsedGB = doc["ram_used_gb"] | 0.0f;
  fresh.ramTotalGB = doc["ram_total_gb"] | 0.0f;
  fresh.vramUsedGB = vramUsed;
  fresh.vramTotalGB = vramTotal;
  fresh.netUpKbps = netUp;
  fresh.netDownKbps = netDown;
  const char *pcName = doc["pc_name"] | "Windows PC";
  const char *gpuName = doc["gpu_name"] | "";
  snprintf(fresh.pcName, sizeof(fresh.pcName), "%s", pcName);
  snprintf(fresh.gpuName, sizeof(fresh.gpuName), "%s", gpuName);
  pcTelemetry = fresh;
  trackRealMood();

  JsonDocument response;
  response["ok"] = true;
  response["emotion"] = realEmotionName();
  response["preview_active"] = previewActive();
  replyJson(200, response);
}

static void fillStatus(JsonDocument &doc) {
  doc["version"] = MEO_VERSION;
#ifdef MIMI_PUBLIC_FIRMWARE
  doc["build_flavor"] = "mimi-public-firmware-v1:" MEO_VERSION;
#else
  doc["build_flavor"] = "mimi-private-bridge-v1:" MEO_VERSION;
#endif
  doc["state_revision"] = stateRevision;
  doc["scene_layout_revision"] = sceneLayoutRevision;
  doc["rules_revision"]=rulesRevision;
  JsonArray feedFresh=doc["feed_fresh"].to<JsonArray>();
  for(uint8_t i=0;i<3;++i)feedFresh.add(integrationFeeds[i].fresh(millis()));
  doc["state_boot_id"] = stateBootId;
  doc["device"] = "MEO DESK";
  doc["api_version"] = 1;
  doc["device_id"] = String((uint32_t)(ESP.getEfuseMac() & 0xffffffffULL), HEX);
  JsonArray capabilities = doc["capabilities"].to<JsonArray>();
  capabilities.add("pc_metrics");
  capabilities.add("network_rates");
  capabilities.add("gpu_vram");
  capabilities.add("media");
  capabilities.add("media_commands");
  capabilities.add("wifi_nvs");
  capabilities.add("mood_v3");
  capabilities.add("event_timeline");
  capabilities.add("focus_sync");
  capabilities.add("focus_completion_cursor_v1");
  capabilities.add("notifications_v1");
  capabilities.add("automation_rules_v1");
  capabilities.add("integration_feeds_v1");
  capabilities.add("tft_themes");
  capabilities.add("scene_shortcuts_v1");
  capabilities.add("cat_behavior_v1");
  capabilities.add("motion_director_v1");
  capabilities.add("album_art_rgb565_64");
  capabilities.add("n16r8_psram_memory");
  capabilities.add("fps_profiles_v1");
  capabilities.add("fps_auto_v1");
  capabilities.add("performance_history_v1");
  capabilities.add("state_websocket_v1");
  capabilities.add("performance_lab_v1");
  capabilities.add("device_profile_v1");
  capabilities.add("focus_config_v1");
  capabilities.add("widget_scene_v1");
  doc["state_ws_port"] = 81;
  if (meoOta.available()) capabilities.add("ota_local_ab_v1");
  if (meoOta.available()) capabilities.add("ota_pull_v1");
  if (catPack.available()) capabilities.add("catpack_ab_v1");
  if (catPack.availableFor(CatPackManager::MOTION_BYTES))
    capabilities.add("catpack_motion_v2");
  doc["flash_bytes"] = ESP.getFlashChipSize();
  doc["psram_bytes"] = ESP.getPsramSize();
  doc["psram_free_bytes"] = ESP.getFreePsram();
  doc["media_available"] = pcMedia.fresh(millis()) && pcMedia.available;
  doc["media_playing"] = pcMedia.playingNow(millis());
  doc["artwork_ready"] = pcBridgeArtworkReady();
  doc["command_pending"] = mediaCommands.pending(millis());
  doc["command_seq"] = mediaCommands.sequence();
  doc["command_acknowledged"] = mediaCommands.acknowledged();
  doc["command_ok"] = mediaCommands.lastResult();
  doc["wifi_connected"] = pcNetworkConnected();
  doc["wifi_sta_status"] = (int)WiFi.status();
  doc["wifi_scan_active"] = wifiScanActive;
  doc["wifi_last_disconnect_reason"] = (int)lastStaDisconnectReason;
  doc["wifi_ssid"] = pcNetworkConnected() ? WiFi.SSID() : apSsid;
  serverUpdater.writePolicy(doc["update_policy"].to<JsonObject>(), false);
  serverUpdater.writeState(doc["update_state"].to<JsonObject>());
  doc["wifi_rssi"] = pcNetworkConnected() ? WiFi.RSSI() : 0;
  doc["ip"] = pcNetworkIP();
  doc["ap_ip"] = WiFi.softAPIP().toString();
  doc["uptime_s"] = millis() / 1000UL;
  doc["esp_heap_kb"] = ESP.getFreeHeap() / 1024UL;
  const bool fresh = pcTelemetryFresh();
  doc["pc_connected"] = fresh;
  doc["demo"] = fresh && pcTelemetry.demo;
  doc["data_age_ms"] = pcTelemetry.received
                           ? (uint32_t)(millis() - pcTelemetry.lastReceivedMs)
                           : 0;
  doc["emotion"] = pcEmotionName();
  doc["real_emotion"] = realEmotionName();
  doc["preview_active"] = previewActive();
  doc["pc_name"] = fresh ? pcTelemetry.pcName : "";
  if(fresh && !pcTelemetry.demo && pcTelemetry.idleSeconds>=0) doc["pc_idle_s"]=pcTelemetry.idleSeconds;
  else doc["pc_idle_s"]=nullptr;
  doc["gpu_name"] = fresh ? pcTelemetry.gpuName : "";
  if (fresh) {
    doc["cpu_percent"] = pcTelemetry.cpuPercent;
    doc["ram_percent"] = pcTelemetry.ramPercent;
    doc["ram_used_gb"] = pcTelemetry.ramUsedGB;
    doc["ram_total_gb"] = pcTelemetry.ramTotalGB;
    if (pcTelemetry.netUpKbps >= 0) doc["net_up_kbps"] = pcTelemetry.netUpKbps;
    else doc["net_up_kbps"] = nullptr;
    if (pcTelemetry.netDownKbps >= 0) doc["net_down_kbps"] = pcTelemetry.netDownKbps;
    else doc["net_down_kbps"] = nullptr;
    if (pcTelemetry.vramUsedGB >= 0) doc["vram_used_gb"] = pcTelemetry.vramUsedGB;
    else doc["vram_used_gb"] = nullptr;
    if (pcTelemetry.vramTotalGB >= 0) doc["vram_total_gb"] = pcTelemetry.vramTotalGB;
    else doc["vram_total_gb"] = nullptr;
    if (pcTelemetry.gpuPercent >= 0) doc["gpu_percent"] = pcTelemetry.gpuPercent;
    else doc["gpu_percent"] = nullptr;
    if (pcTelemetry.gpuTempC >= 0) doc["gpu_temp_c"] = pcTelemetry.gpuTempC;
    else doc["gpu_temp_c"] = nullptr;
    if (pcTelemetry.cpuTempC >= 0) doc["cpu_temp_c"] = pcTelemetry.cpuTempC;
    else doc["cpu_temp_c"] = nullptr;
    if (pcTelemetry.batteryPercent >= 0) doc["battery_percent"] = pcTelemetry.batteryPercent;
    else doc["battery_percent"] = nullptr;
    if (pcTelemetry.batteryPlugged >= 0) doc["battery_plugged"] = pcTelemetry.batteryPlugged == 1;
    else doc["battery_plugged"] = nullptr;
    doc["pc_uptime_s"] = pcTelemetry.pcUptimeS;
  } else {
    doc["cpu_percent"] = nullptr;
    doc["gpu_percent"] = nullptr;
    doc["ram_percent"] = nullptr;
    doc["gpu_temp_c"] = nullptr;
    doc["ram_used_gb"] = nullptr;
    doc["ram_total_gb"] = nullptr;
    doc["net_up_kbps"] = nullptr;
    doc["net_down_kbps"] = nullptr;
    doc["vram_used_gb"] = nullptr;
    doc["vram_total_gb"] = nullptr;
    doc["cpu_temp_c"] = nullptr;
    doc["battery_percent"] = nullptr;
    doc["battery_plugged"] = nullptr;
    doc["pc_uptime_s"] = nullptr;
  }
  JsonObject settings = doc["settings"].to<JsonObject>();
  settings["cpu_limit"] = healthSettings.cpuLimit;
  settings["gpu_limit"] = healthSettings.gpuLimit;
  settings["temp_limit"] = healthSettings.tempLimit;
  settings["battery_limit"] = healthSettings.batteryLimit;
  settings["quiet"] = healthSettings.quiet;
  const EventEngine::Event *active = deskEvents.active(millis());
  doc["active_event"] = active ? active->code : "";
  doc["active_event_priority"] = active ? (int)active->priority : 0;
  doc["event_seq"] = deskEvents.latestSequence();
  JsonArray events = doc["events"].to<JsonArray>();
  for (uint8_t i = 0; i < deskEvents.count() && i < 6; ++i) {
    const EventEngine::Event *item = deskEvents.recentAt(i);
    JsonObject entry = events.add<JsonObject>();
    entry["code"] = item->code;
    entry["message"] = item->message;
    entry["age_s"] = (uint32_t)(millis() - item->createdMs) / 1000UL;
    entry["priority"] = (int)item->priority;
  }
  if (statusHandler) statusHandler(doc);
}

static void sendStatus() {
  JsonDocument doc;
  fillStatus(doc);
  replyJson(200, doc);
}

static void publishState() {
  const uint32_t now = millis();
  if (!stateSocket.connectedClients() || meoOta.busy() || catPack.busy()) return;
  if ((uint32_t)(now - statePublishedMs) < (stateDirty ? 40UL : 1000UL)) return;
  statePublishedMs = now;
  stateDirty = false;
  ++stateRevision;
  JsonDocument doc;
  fillStatus(doc);
  doc["type"] = "state";
  // Same state as HTTP; media is the existing authenticated data plane.
  String payload;
  payload.reserve(6000);
  serializeJson(doc, payload);
  for (uint8_t client = 0; client < 4; ++client) {
    if (!stateSocket.clientIsConnected(client)) continue;
    if (!stateAuthorized[client]) {
      stateSocket.sendTXT(client, payload);
      continue;
    }
    JsonObject media = doc["media"].to<JsonObject>();
    const uint32_t mediaNow = millis();
    const bool available = pcMedia.fresh(mediaNow) && pcMedia.available;
    media["available"] = available;
    media["source_age_ms"]=pcMedia.received ? static_cast<uint32_t>(mediaNow-pcMedia.receivedMs) : 10000UL;
    media["playing"] = pcMedia.playingNow(mediaNow);
    media["title"] = available ? pcMedia.title : "";
    media["artist"] = available ? pcMedia.artist : "";
    media["position_ms"] = available ? pcMedia.positionAt(mediaNow) : 0;
    media["duration_ms"] = available ? pcMedia.durationMs : 0;
    media["artwork_id"] = available ? pcMedia.artworkId : "";
    media["artwork_ready"] = pcBridgeArtworkReady();
    String privatePayload;
    serializeJson(doc, privatePayload);
    stateSocket.sendTXT(client, privatePayload);
    doc.remove("media");
  }
}

static void sendEvents() {
  if (!authorized()) return;
  JsonDocument doc;
  doc["schema"] = 1;
  doc["latest_seq"] = deskEvents.latestSequence();
  JsonArray list = doc["events"].to<JsonArray>();
  const uint32_t after = (uint32_t)httpServer.arg("after").toInt();
  for (uint8_t i = deskEvents.count(); i > 0; --i) {
    const EventEngine::Event *event = deskEvents.recentAt(i - 1);
    if (event->seq <= after) continue;
    JsonObject item = list.add<JsonObject>();
    item["seq"] = event->seq;
    item["code"] = event->code;
    item["message"] = event->message;
    item["priority"] = (int)event->priority;
    item["age_ms"] = (uint32_t)(millis() - event->createdMs);
    item["ttl_ms"] = event->ttlMs;
  }
  replyJson(200, doc);
}

static void sendFocus() {
  JsonDocument doc;
  doc["schema"] = 1;
  if (statusHandler) statusHandler(doc);
  replyJson(200, doc);
}

static void receiveFocus() {
  if (!authorized()) return;
  const String payload = httpServer.arg("plain");
  if (!payload.length() || payload.length() > 100) {
    replyError(413, "focus payload 1..100 bytes"); return;
  }
  JsonDocument body;
  if (deserializeJson(body, payload) || !body["action"].is<const char *>()) {
    replyError(400, "invalid focus request"); return;
  }
  String action = body["action"].as<String>();
  if (action == "toggle") action = "focus";
  else if (action == "reset") action = "focus_reset";
  else if (action == "skip") action = "focus_skip";
  else { replyError(400, "unsupported focus action"); return; }
  if (!controlHandler || !controlHandler(action, 0)) {
    replyError(503, "focus renderer unavailable"); return;
  }
  JsonDocument doc;
  doc["schema"] = 1;
  doc["ok"] = true;
  if (statusHandler) statusHandler(doc);
  replyJson(200, doc);
}

// Separate endpoint so the 1 Hz status fetch stays small. Browsers poll
// this at 3-second intervals and display 1 / 3 / 5-minute windows.
static void sendHistory() {
  JsonDocument doc;
  doc["period_s"] = 1;
  doc["capacity"] = HISTORY_SIZE;
  doc["count"] = historyCount;
  doc["mode"] = pcTelemetryFresh() ? (pcTelemetry.demo ? "demo" : "real") : "offline";
  JsonArray cpu = doc["cpu"].to<JsonArray>();
  JsonArray gpu = doc["gpu"].to<JsonArray>();
  JsonArray ram = doc["ram"].to<JsonArray>();
  for (uint16_t i = 0; i < historyCount; ++i) {
    PcHistoryPoint s;
    pcHistoryAt(i, s);
    if (s.cpu == 255) cpu.add(nullptr); else cpu.add(s.cpu);
    if (s.gpu == 255) gpu.add(nullptr); else gpu.add(s.gpu);
    if (s.ram == 255) ram.add(nullptr); else ram.add(s.ram);
  }
  replyJson(200, doc);
}

static void handleControl() {
  if (!authorized()) return;
  JsonDocument input;
  if (deserializeJson(input, httpServer.arg("plain"))) {
    replyError(400, "invalid control JSON");
    return;
  }
  String action = input["action"] | "";
  int value = input["value"] | 0;
  if (action == "media_play_pause" || action == "media_next" || action == "media_previous") {
    if (!pcTelemetryFresh()) { replyError(503, "Windows agent offline"); return; }
    const char *command = action == "media_play_pause" ? "play_pause" :
                          action == "media_next" ? "next" : "previous";
    if (!mediaCommands.submit(command, millis())) {
      replyError(409, "another media command is pending"); return;
    }
    JsonDocument answer;
    answer["ok"] = true;
    answer["pending"] = true;
    answer["seq"] = mediaCommands.sequence();
    replyJson(202, answer);
    return;
  }
  if (action == "preview") {
    if (value < 0 || value >= 8) {
      replyError(400, "preview index must be 0..7");
      return;
    }
    previewIndex = value;
    previewUntilMs = millis() + 8000UL;
    if (controlHandler) controlHandler("refresh", 0);
    JsonDocument answer;
    answer["ok"] = true;
    answer["preview"] = PREVIEW_MOODS[value];
    answer["duration_ms"] = 8000;
    replyJson(200, answer);
    return;
  }
  if (action.length() > 20 || !controlHandler || !controlHandler(action, value)) {
    replyError(400, "unknown action or value");
    return;
  }
  JsonDocument answer;
  answer["ok"] = true;
  answer["action"] = action;
  if (action == "motion" || action == "auto") {
    captureRuntimeProfile();
    answer["persisted"] = persistProfile(deviceProfile);
  }
  replyJson(200, answer);
}

static void handleSettings() {
  if (!authorized()) return;
  const String payload = httpServer.arg("plain");
  if (payload.length() == 0 || payload.length() > 400) {
    replyError(413, "settings JSON must be 1..400 bytes");
    return;
  }
  JsonDocument input;
  if (deserializeJson(input, payload)) {
    replyError(400, "invalid settings JSON");
    return;
  }
  const int cpu = input["cpu_limit"] | -1;
  const int gpu = input["gpu_limit"] | -1;
  const int temp = input["temp_limit"] | -1;
  const int battery = input["battery_limit"] | -1;
  if (cpu < 50 || cpu > 99 || gpu < 50 || gpu > 99 ||
      temp < 55 || temp > 110 || battery < 5 || battery > 50 ||
      !input["quiet"].is<bool>()) {
    replyError(400, "invalid thresholds or quiet flag");
    return;
  }
  healthSettings.cpuLimit = cpu;
  healthSettings.gpuLimit = gpu;
  healthSettings.tempLimit = temp;
  healthSettings.batteryLimit = battery;
  healthSettings.quiet = input["quiet"].as<bool>();
  if (prefsReady) {
    prefs.putUChar("cpu", cpu);
    prefs.putUChar("gpu", gpu);
    prefs.putUChar("temp", temp);
    prefs.putUChar("battery", battery);
    prefs.putBool("quiet", healthSettings.quiet);
  }
  trackRealMood();
  if (controlHandler) controlHandler("refresh", 0);
  captureRuntimeProfile();
  JsonDocument answer;
  answer["ok"] = true;
  answer["persisted"] = persistProfile(deviceProfile);
  replyJson(200, answer);
}

// Serve cat pixels from the existing 648 KiB RGB565 assets (16 images).
// BMP24 means no duplicate PNG copy in firmware flash and no JPEG decoder.
// Small 72x72 thumbnails save bandwidth when rendering the eight-mood gallery.
static void serveCatBmp() {
  const String mode = httpServer.arg("state");
  const uint16_t *pixels = catForEmotion(mode);
  const bool small = httpServer.arg("size") == "72";
  constexpr uint32_t MAX_ROW_BYTES = 144 * 3;
  const uint32_t width = small ? 72 : 144;
  const uint32_t height = width;
  const uint32_t rowBytes = width * 3; // both 72 and 144 have BMP 4-byte alignment
  const uint32_t imageBytes = rowBytes * height;
  const uint32_t fileBytes = 54 + imageBytes;
  uint8_t header[54] = {'B', 'M'};
  auto put16 = [&header](int at, uint16_t v) {
    header[at] = v & 255;
    header[at + 1] = (v >> 8) & 255;
  };
  auto put32 = [&header](int at, uint32_t v) {
    for (int i = 0; i < 4; ++i) header[at + i] = (v >> (i * 8)) & 255;
  };
  put32(2, fileBytes);
  put32(10, 54);
  put32(14, 40);       // BITMAPINFOHEADER
  put32(18, width);
  put32(22, height);   // bottom-up BGR
  put16(26, 1);        // planes
  put16(28, 24);       // RGB888
  put32(34, imageBytes);

  httpServer.sendHeader("Cache-Control", "public, max-age=60");
  httpServer.setContentLength(fileBytes);
  httpServer.send(200, "image/bmp", "");
  auto client = httpServer.client();
  client.write(header, sizeof(header));
  uint8_t line[MAX_ROW_BYTES];
  for (int y = (int)height - 1; y >= 0; --y) {
    for (uint32_t x = 0; x < width; ++x) {
      const uint32_t stride = small ? 2 : 1;
      const uint16_t c = pixels[(y * stride) * 144 + (x * stride)];
      uint8_t r = (c >> 11) & 31;
      uint8_t g = (c >> 5) & 63;
      uint8_t b = c & 31;
      line[x * 3] = (b << 3) | (b >> 2);
      line[x * 3 + 1] = (g << 2) | (g >> 4);
      line[x * 3 + 2] = (r << 3) | (r >> 2);
    }
    client.write(line, rowBytes);
  }
}

static void sendPerformanceHistory() {
  if (!performanceHandler) { replyError(503, "performance history unavailable"); return; }
  JsonDocument doc;
  performanceHandler(doc);
  replyJson(200, doc);
}

// Only count raw firmware bytes, not the multipart/form-data envelope.
static bool parseOtaLength(const String &header, size_t &length) {
  if (header.length() == 0 || header.length() > 9) return false;
  uint32_t total = 0;
  for (size_t i = 0; i < header.length(); ++i) {
    if (header[i] < '0' || header[i] > '9') return false;
    total = total * 10 + (header[i] - '0');
    if (total > 16UL * 1024UL * 1024UL) return false;
  }
  length = total;
  return length >= MeoOtaManager::MIN_IMAGE_BYTES;
}

static void sendOtaStatus() {
  JsonDocument doc;
  doc["schema"] = 1;
  doc["board"] = MeoOtaManager::BOARD_ID;
  doc["version"] = MEO_VERSION;
  doc["ready"] = meoOta.available();
  doc["phase"] = meoOta.phaseName();
  doc["message"] = meoOta.message();
  doc["written"] = meoOta.written();
  doc["expected"] = meoOta.expected();
  doc["flash_bytes"] = ESP.getFlashChipSize();
  doc["uptime_ms"] = millis();
  doc["boot_health"] = meoOta.bootHealth();
  doc["inactive_slot_bytes"] = meoOta.availableBytes();
  doc["sha256_required"] = true;
  doc["signed_image_required"] = false; // Developer LAN OTA, not Internet-ready.
#if defined(CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE) && CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
  doc["bootloader_rollback_configured"] = true;
#else
  doc["bootloader_rollback_configured"] = false;
#endif
  const esp_partition_t *running = esp_ota_get_running_partition();
  const esp_partition_t *inactive = esp_ota_get_next_update_partition(nullptr);
  doc["running_slot"] = running ? running->label : "N/A";
  doc["inactive_slot"] = inactive ? inactive->label : "N/A";
  replyJson(200, doc);
}

static void receiveFirmwareUpload() {
  HTTPUpload &upload = httpServer.upload();
  if (upload.status == UPLOAD_FILE_START) {
    otaUploadStarted = false;
    otaUploadFinished = false;
    if (httpServer.header("X-MEO-KEY") != apiKey || catPack.busy() || serverUpdater.busy()) {
      // Do not invoke the OTA engine or write flash for unauthenticated clients.
      return;
    }
    size_t bytes = 0;
    if (!parseOtaLength(httpServer.header("X-MEO-OTA-SIZE"), bytes) || !meoOta.available()) {
      meoOta.abort("Invalid size header or no OTA slot");
      return;
    }
    const String board = httpServer.header("X-MEO-OTA-BOARD");
    const String sha = httpServer.header("X-MEO-OTA-SHA256");
    if (board != MeoOtaManager::BOARD_ID || sha.length() != 64) {
      meoOta.abort("Wrong board or missing SHA-256");
      return;
    }
    // Display a static OTA screen BEFORE erasing/writing Flash, then stop
    // animation and preserve internal DMA memory for the upload handler.
    otaLastPercent = 0;
    if (otaModeHandler) otaModeHandler(0);
    otaUploadStarted = meoOta.begin(bytes, sha, board);
    if (!otaUploadStarted && otaModeHandler) otaModeHandler(-1);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (otaUploadStarted) {
      if (!meoOta.append(upload.buf, upload.currentSize)) {
        otaUploadStarted = false;
        if (otaModeHandler) otaModeHandler(-1);
      } else {
        const int percent = static_cast<int>((100ULL * meoOta.written()) / meoOta.expected());
        if (percent != otaLastPercent) {
          otaLastPercent = percent;
          if (otaModeHandler) otaModeHandler(percent);
        }
      }
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (otaUploadStarted) {
      otaUploadFinished = meoOta.finish();
      otaUploadStarted = false;
      if (!otaUploadFinished && otaModeHandler) otaModeHandler(-1);
      if (otaUploadFinished && otaModeHandler) otaModeHandler(100);
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    if (otaUploadStarted) meoOta.abort("Upload interrupted");
    otaUploadStarted = false;
    otaUploadFinished = false;
    if (otaModeHandler) otaModeHandler(-1);
  }
}

static void finishFirmwareUpload() {
  if (httpServer.header("X-MEO-KEY") != apiKey) {
    replyError(401, "invalid pairing key");
    return;
  }
  if (!otaUploadFinished || meoOta.phase() != MeoOtaManager::READY_REBOOT) {
    replyError(422, meoOta.message());
    return;
  }
  JsonDocument doc;
  doc["ok"] = true;
  doc["verified"] = true;
  recordOtaUpload();
  doc["rebooting"] = true;
  doc["bytes"] = meoOta.written();
  replyJson(200, doc);
  otaRebootAtMs = millis() + 1800UL;
  otaUploadFinished = false;
}

static void sendCatPackStatus() {
  JsonDocument doc;
  doc["schema"] = 1;
  doc["board"] = MeoOtaManager::BOARD_ID;
  doc["ready"] = catPack.available() && !meoOta.busy();
  doc["motion_ready"] = catPack.availableFor(CatPackManager::MOTION_BYTES) && !meoOta.busy();
  doc["source"] = catPack.loaded() ? "custom" : "builtin";
  doc["active_bank"] = catPack.activeBank();
  doc["generation"] = catPack.generation();
  doc["sha256"] = catPack.loaded() ? catPack.activeSha() : "";
  doc["frame_count"] = CatPackManager::FRAMES;
  doc["frames_per_pose"] = catPack.framesPerPose();
  doc["active_pack_bytes"] = catPack.activeBytes();
  doc["width"] = CatPackManager::WIDTH;
  doc["height"] = CatPackManager::HEIGHT;
  doc["pack_bytes"] = CatPackManager::PAYLOAD_BYTES;
  JsonArray accepted = doc["accepted_pack_bytes"].to<JsonArray>();
  accepted.add(CatPackManager::PAYLOAD_BYTES);
  if (catPack.availableFor(CatPackManager::MOTION_BYTES))
    accepted.add(CatPackManager::MOTION_BYTES);
  doc["phase"] = catPack.phaseName();
  doc["message"] = catPack.message();
  replyJson(200, doc);
}

static void receiveCatPackUpload() {
  HTTPUpload &upload = httpServer.upload();
  if (upload.status == UPLOAD_FILE_START) {
    catPackUploadStarted = catPackUploadFinished = false;
    if (httpServer.header("X-MEO-KEY") != apiKey || meoOta.busy() || serverUpdater.busy()) return;
    size_t expected = 0;
    if (!parseOtaLength(httpServer.header("X-MEO-CAT-SIZE"), expected) ||
        !catPack.availableFor(expected)) {
      if (!catPack.busy()) catPack.abort("Cat pack size or PSRAM invalid");
      return;
    }
    catPackLastPercent = 0;
    if (catPackModeHandler) catPackModeHandler(0);
    catPackUploadStarted = catPack.begin(httpServer.header("X-MEO-CAT-SHA256"),
                                       static_cast<uint32_t>(expected));
    if (!catPackUploadStarted && catPackModeHandler) catPackModeHandler(-1);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (catPackUploadStarted) {
      if (!catPack.append(upload.buf, upload.currentSize)) {
        catPackUploadStarted = false;
        if (catPackModeHandler) catPackModeHandler(-1);
      } else {
        const int percent = static_cast<int>((100ULL * catPack.written()) /
                                             catPack.uploadBytes());
        if (percent != catPackLastPercent) {
          catPackLastPercent = percent;
          if (catPackModeHandler) catPackModeHandler(percent);
        }
      }
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (catPackUploadStarted) {
      catPackUploadFinished = catPack.finish();
      catPackUploadStarted = false;
      if (catPackModeHandler) catPackModeHandler(catPackUploadFinished ? 100 : -1);
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    if (catPackUploadStarted) catPack.abort("Cat pack transfer interrupted");
    catPackUploadStarted = catPackUploadFinished = false;
    if (catPackModeHandler) catPackModeHandler(-1);
  }
}

static void finishCatPackUpload() {
  if (httpServer.header("X-MEO-KEY") != apiKey) {
    replyError(401, "invalid pairing key");
    return;
  }
  if (!catPackUploadFinished || catPack.phase() != CatPackManager::READY_REBOOT) {
    replyError(422, catPack.message());
    return;
  }
  JsonDocument doc;
  doc["ok"] = true;
  doc["verified"] = true;
  doc["rebooting"] = true;
  doc["bytes"] = catPack.written();
  replyJson(200, doc);
  catPackUploadFinished = false;
  otaRebootAtMs = millis() + 1800UL;
}

static void sendUpdatePolicy() {
  if(!authorized())return;
  JsonDocument doc;serverUpdater.describe(doc);replyJson(200,doc);
}
static void updateRequest(uint8_t operation) {
  if(!authorized())return;
  JsonDocument body;const String raw=httpServer.arg("plain");
  if(raw.length()>2048 || deserializeJson(body,raw)) {replyError(400,"invalid_update_request");return;}
  String error;int status=400;bool ok=false;
  if(operation==0)ok=serverUpdater.configure(body.as<JsonVariantConst>(),error,status);
  else if(operation==1)ok=serverUpdater.check(body["command_id"]|"",error,status);
  else if(body["expected_revision"].is<uint32_t>())ok=serverUpdater.start(body["release_id"]|"",
      body["expected_revision"].as<uint32_t>(),body["command_id"]|"",error,status);
  else error="policy_conflict";
  if(!ok){replyError(status,error.c_str());return;}
  JsonDocument doc;serverUpdater.describe(doc);doc["ok"]=true;replyJson(200,doc);
}
static void sendLocalPairing() {
  const IPAddress remote=httpServer.client().remoteIP(), local=WiFi.softAPIP();
  if(pcNetworkConnected() || remote[0]!=local[0] || remote[1]!=local[1] || remote[2]!=local[2]) {
    replyError(403,"connect_to_device_setup_wifi");return;
  }
  if(!prefsReady){replyError(503,"pairing_storage_unavailable");return;}
  JsonDocument doc;doc["schema"]=1;doc["pairing_key"]=apiKey;
  doc["device_id"]=String(static_cast<uint32_t>(ESP.getEfuseMac()&0xffffffffULL),HEX);
  replyJson(200,doc);
}

void pcBridgeBegin(UiControlCallback onControl, UiStatusCallback onStatus,
                   UiPerformanceCallback onPerformance,
                   UiOtaModeCallback onOtaMode,
                   UiCatPackModeCallback onCatPackMode) {
  controlHandler = onControl;
  statusHandler = onStatus;
  performanceHandler = onPerformance;
  otaModeHandler = onOtaMode;
  catPackModeHandler = onCatPackMode;
  allocateArtworkBuffers();
  prefsReady = prefs.begin("meodesk06", false);
  if (prefsReady) {
    healthSettings.cpuLimit = constrain(prefs.getUChar("cpu", 88), 50, 99);
    healthSettings.gpuLimit = constrain(prefs.getUChar("gpu", 92), 50, 99);
    healthSettings.tempLimit = constrain(prefs.getUChar("temp", 80), 55, 110);
    healthSettings.batteryLimit = constrain(prefs.getUChar("battery", 20), 5, 50);
    healthSettings.quiet = prefs.getBool("quiet", false);
    savedTheme = constrain(prefs.getUChar("theme", 0), 0, 5);
    savedFpsProfile = constrain(prefs.getUChar("fps", 1), 0, 3);
    savedMotionStyle = constrain(prefs.getUChar("motionstyle", 1), 0, 2);
    wifiSsid = prefs.getString("ssid", WIFI_SSID);
    wifiPassword = prefs.getString("wifi_pw", WIFI_PASSWORD);
  } else {
    wifiSsid = WIFI_SSID;
    wifiPassword = WIFI_PASSWORD;
  }
  apiKey = prefsReady ? prefs.getString("api_key", MEO_API_KEY) : String(MEO_API_KEY);
  if (apiKey.isEmpty()) {
    char generated[33];snprintf(generated,sizeof(generated),"%08lx%08lx%08lx%08lx",
      static_cast<unsigned long>(esp_random()),static_cast<unsigned long>(esp_random()),
      static_cast<unsigned long>(esp_random()),static_cast<unsigned long>(esp_random()));
    apiKey=generated;
  }
  apSsid=prefsReady?prefs.getString("ap_ssid",MEO_AP_SSID):String(MEO_AP_SSID);
  apPassword=prefsReady?prefs.getString("ap_pw",MEO_AP_PASSWORD):String(MEO_AP_PASSWORD);
  // One-time bridge: persist private compiled defaults before generic firmware replaces them.
  if(prefsReady) {
    if(prefs.getString("api_key","").isEmpty())prefs.putString("api_key",apiKey);
    if(!prefs.isKey("ssid"))prefs.putString("ssid",wifiSsid);
    if(!prefs.isKey("wifi_pw"))prefs.putString("wifi_pw",wifiPassword);
    if(!prefs.isKey("ap_ssid"))prefs.putString("ap_ssid",apSsid);
    if(!prefs.isKey("ap_pw"))prefs.putString("ap_pw",apPassword);
  }
  serverUpdater.begin(otaModeHandler, [](){recordOtaUpload(serverUpdater.activeSha(),serverUpdater.activeVersion());otaRebootAtMs=millis()+1800UL;});
  deviceProfile.theme = savedTheme; deviceProfile.fps = savedFpsProfile; deviceProfile.style = savedMotionStyle;
  deviceProfile.cpu = healthSettings.cpuLimit; deviceProfile.gpu = healthSettings.gpuLimit;
  deviceProfile.temperature = healthSettings.tempLimit; deviceProfile.battery = healthSettings.batteryLimit;
  deviceProfile.quiet = healthSettings.quiet;
  if (prefsReady) {
    DeviceProfile migrated = deviceProfile;
    for (uint8_t i = 0; i < 4; ++i) {
      char key[5]; snprintf(key, sizeof(key), "sc%u", static_cast<unsigned>(i));
      migrated.scenes[i] = prefs.getUChar(key, migrated.scenes[i]);
    }
    if (migrated.valid()) deviceProfile = migrated;
    JsonDocument stored;
    DeviceProfile canonical;
    const String data = prefs.getString("profile_v1", "");
    if (data.length() && !deserializeJson(stored, data) && stored["schema"] == DeviceProfile::SCHEMA &&
        decodeProfile(stored["profile"].as<JsonObjectConst>(), canonical)) {
      SceneLayout savedLayout;RuleSet savedRules;
      if((stored["scene_layout"].isNull() || decodeLayout(stored["scene_layout"].as<JsonObjectConst>(),savedLayout)) &&
         (stored["rules"].isNull() || decodeRules(stored["rules"].as<JsonObjectConst>(),savedRules))) {
        deviceProfile=canonical; sceneLayout=savedLayout;deviceRules=savedRules;
      }
    }
    const String history = prefs.getString("ota_history", "[]");
    if (history.length() > 4096 || deserializeJson(otaHistory, history) || !otaHistory.is<JsonArray>() || otaHistory.size() > 6)
      otaHistory.to<JsonArray>();
    if (otaHistory.size()) otaHistoryPending = otaHistory[otaHistory.size()-1]["phase"] == "uploaded";
  }
  applyProfileMetadata();
  WiFi.onEvent(onWifiEvent);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(apSsid.c_str(), apPassword.c_str());
  WiFi.setAutoReconnect(true);
  if (wifiSsid.length()) {
    lastStaAttemptMs = millis();
    WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());
    Serial.printf("[WIFI] Connecting to router %s (2.4 GHz only)...\n", wifiSsid.c_str());
    configTime(7 * 3600, 0, "pool.ntp.org", "time.google.com");
  } else {
    Serial.println("[WIFI] Router SSID not configured; fallback AP only");
  }

  const char *keys[] = {"X-MEO-KEY", "X-MEO-ART-ID", "X-MEO-OTA-SIZE", "X-MEO-OTA-SHA256",
                        "X-MEO-OTA-BOARD", "X-MEO-CAT-SIZE", "X-MEO-CAT-SHA256"};
  httpServer.collectHeaders(keys, 7);
  httpServer.on("/", HTTP_GET, []() {
    httpServer.sendHeader("Cache-Control", "no-cache");
    httpServer.send_P(200, "text/html; charset=utf-8", WEB_UI_HTML);
  });
  httpServer.on("/api/status", HTTP_GET, sendStatus);
  httpServer.on("/api/v1/status", HTTP_GET, sendStatus);
  httpServer.on("/api/v1/performance", HTTP_GET, sendPerformanceHistory);
  httpServer.on("/api/v1/ota/status", HTTP_GET, sendOtaStatus);
  httpServer.on("/api/v1/ota/history", HTTP_GET, sendOtaHistory);
  httpServer.on("/api/v1/update-policy",HTTP_GET,sendUpdatePolicy);
  httpServer.on("/api/v1/update-policy",HTTP_POST,[](){updateRequest(0);});
  httpServer.on("/api/v1/update/check",HTTP_POST,[](){updateRequest(1);});
  httpServer.on("/api/v1/update/start",HTTP_POST,[](){updateRequest(2);});
  httpServer.on("/api/v1/pairing",HTTP_GET,sendLocalPairing);
  httpServer.on("/api/v1/ota/upload", HTTP_POST, finishFirmwareUpload, receiveFirmwareUpload);
  httpServer.on("/api/v1/catpack/status", HTTP_GET, sendCatPackStatus);
  httpServer.on("/api/v1/catpack/upload", HTTP_POST, finishCatPackUpload, receiveCatPackUpload);
  httpServer.on("/api/history", HTTP_GET, sendHistory);
  httpServer.on("/api/telemetry", HTTP_POST, receiveTelemetry);
  httpServer.on("/api/control", HTTP_POST, handleControl);
  httpServer.on("/api/settings", HTTP_POST, handleSettings);
  httpServer.on("/api/media", HTTP_GET, sendMedia);
  httpServer.on("/api/media", HTTP_POST, receiveMedia);
  httpServer.on("/api/v1/media", HTTP_GET, sendMedia);
  httpServer.on("/api/v1/media", HTTP_POST, receiveMedia);
  httpServer.on("/api/v1/artwork", HTTP_POST, finishArtworkUpload, receiveArtworkUpload);
  httpServer.on("/api/v1/artwork.bmp", HTTP_GET, sendArtworkBmp);
  httpServer.on("/api/v1/events", HTTP_GET, sendEvents);
  httpServer.on("/api/v1/notifications", HTTP_POST, receiveNotification);
  httpServer.on("/api/v1/rules",HTTP_GET,exportRules);
  httpServer.on("/api/v1/rules",HTTP_POST,importRules);
  for(uint8_t i=0;i<3;++i) {
    const String path="/api/v1/feeds/"+String(i);
    httpServer.on(path,HTTP_GET,[i](){sendFeed(i);});
    httpServer.on(path,HTTP_POST,[i](){receiveFeed(i);});
  }
  httpServer.on("/api/v1/focus", HTTP_GET, sendFocus);
  httpServer.on("/api/v1/focus", HTTP_POST, receiveFocus);
  httpServer.on("/api/v1/focus/config", HTTP_POST, configureFocus);
  httpServer.on("/api/v1/config", HTTP_GET, exportConfiguration);
  httpServer.on("/api/v1/config", HTTP_POST, importConfiguration);
  httpServer.on("/api/v1/scene-layout", HTTP_GET, exportSceneLayout);
  httpServer.on("/api/v1/scene-layout", HTTP_POST, importSceneLayout);
  httpServer.on("/api/commands", HTTP_GET, sendCommands);
  httpServer.on("/api/commands/ack", HTTP_POST, ackCommand);
  httpServer.on("/api/v1/commands", HTTP_GET, sendCommands);
  httpServer.on("/api/v1/commands/ack", HTTP_POST, ackCommand);
  httpServer.on("/api/v1/wifi/config", HTTP_POST, configureWifi);
  httpServer.on("/api/cat.bmp", HTTP_GET, serveCatBmp);
  httpServer.onNotFound([]() { replyError(404, "not found"); });
  httpServer.begin();
  stateSocket.begin();
  stateBootId = String((uint32_t)ESP.getEfuseMac(), HEX) + "-" + String(esp_random(), HEX);
  stateSocket.enableHeartbeat(15000, 3000, 2);
  stateSocket.onEvent([](uint8_t client, WStype_t type, uint8_t *payload, size_t length) {
    if (client >= 4) return;
    if (type == WStype_CONNECTED || type == WStype_DISCONNECTED) {
      stateAuthorized[client] = false;
      stateDirty = true;
    }
    if (type == WStype_TEXT && length <= 256) {
      JsonDocument request;
      if (!deserializeJson(request, payload, length) && request["type"] == "subscribe") {
        stateAuthorized[client] = request["key"].is<const char *>() &&
                                 request["key"].as<String>() == apiKey;
        stateDirty = true;
      }
    }
    // State channel is read-only. Mutations continue through the existing
    // keyed HTTP handlers, with one authoritative broadcast after commit.
  });
  Serial.printf("[WEB] AP %s / http://%s\n", apSsid.c_str(),
                WiFi.softAPIP().toString().c_str());
}

void pcBridgeLoop() {
  httpServer.handleClient();
  stateSocket.loop();
  serverUpdater.loop(pcNetworkConnected(), focusModeActive);
  publishState();
  if (otaRebootAtMs && static_cast<int32_t>(millis() - otaRebootAtMs) >= 0) {
    Serial.println(catPack.busy() ? "[CATPACK] Restarting to load verified sprite bank"
                                  : "[OTA] Restarting into verified firmware slot");
    Serial.flush();
    ESP.restart();
  }
  const uint32_t nowMs = millis();
  settleOtaHistory(nowMs);
  sampleHistory(nowMs);
  if (previewIndex >= 0 && !previewActive()) {
    previewIndex = -1;
    if (controlHandler) controlHandler("refresh", 0);
  }
  trackRealMood();
  if (staDisconnectPending) {
    const uint8_t reason = lastStaDisconnectReason;
    staDisconnectPending = false;
    if (!wifiScanActive) {
      Serial.printf("[WIFI] Router disconnected: reason=%u (201=no AP, 202=auth failed, 204=handshake timeout)\n",
                    (unsigned int)reason);
    }
  }
  if (!wifiScanActive && wifiSsid.length() && !pcNetworkConnected()) {
    if ((uint32_t)(nowMs - lastStaStatusMs) >= 10000UL) {
      lastStaStatusMs = nowMs;
      Serial.printf("[WIFI] Router offline | status=%d | last reason=%u | AP http://%s\n",
                    (int)WiFi.status(), (unsigned int)lastStaDisconnectReason,
                    WiFi.softAPIP().toString().c_str());
    }
    if ((uint32_t)(nowMs - lastStaAttemptMs) >= 20000UL) {
      lastStaAttemptMs = nowMs;
      Serial.printf("[WIFI] Retrying router %s...\n", wifiSsid.c_str());
      WiFi.reconnect();
    }
  }
  if (!pcNetworkConnected() && mdnsStarted) {
    MDNS.end();
    mdnsStarted = false;
  }
  if (pcNetworkConnected() && !mdnsStarted) {
    mdnsStarted = MDNS.begin(MEO_MDNS_NAME);
    if (mdnsStarted) MDNS.addService("http", "tcp", 80);
    Serial.printf("[WEB] Router IP: %s | mDNS: %s\n",
                  WiFi.localIP().toString().c_str(),
                  mdnsStarted ? "meodesk.local" : "unavailable");
  }
}
