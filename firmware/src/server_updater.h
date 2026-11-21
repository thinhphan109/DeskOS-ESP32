#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

class ServerUpdater {
 public:
  using Progress = void (*)(int);
  using Reboot = void (*)();
  void begin(Progress progress, Reboot reboot);
  void loop(bool connected, bool focusRunning);
  void describe(JsonDocument &doc) const;
  void writePolicy(JsonObject out, bool includeUrl = true) const;
  void writeState(JsonObject out) const;
  bool configure(JsonVariantConst input, String &error, int &status);
  bool check(const String &command, String &error, int &status);
  bool start(const String &release, uint32_t revision, const String &command, String &error, int &status);
  bool busy() const { return downloading_ || phase_ == Rebooting; }
  const char *activeSha() const { return active_.sha; }
  const char *activeVersion() const { return active_.version; }

 private:
  enum Phase { Idle, Checking, Available, Waiting, Countdown, Downloading, Rebooting, Confirmed, Error };
  struct Release {
    char id[64]{}, version[32]{}, minimum[32]{}, sha[65]{}, url[512]{}, notes[384]{};
    uint32_t sequence = 0, size = 0;
  };
  struct Packet { uint16_t size = 0; uint8_t kind = 0; uint8_t data[4096]; };
  bool acceptManifest(String &error);
  bool launch(bool image);
  bool beginDownload(String &error);
  void fail(const char *code);
  bool duplicate(const String &command, const String &operation, String &error, int &status);
  void remember(const String &command, const String &operation);
  bool validCommand(const String &command) const;
  static bool validUrl(const String &url);
  static void taskEntry(void *self);
  void fetch();
  static const char *phaseName(Phase phase);
  Preferences prefs_;
  bool prefsReady_ = false;
  QueueHandle_t queue_ = nullptr;
  Progress progress_ = nullptr;
  Reboot reboot_ = nullptr;
  String url_, channel_ = "preview";
  uint32_t revision_ = 1, interval_ = 120, nextCheck_ = 0, countdownAt_ = 0;
  uint32_t lastCheckEpoch_ = 0, highestSequence_ = 0, failures_ = 0;
  bool automatic_ = false, deferFocus_ = true, taskRunning_ = false, downloading_ = false;
  bool imageTask_ = false, manualRequested_ = false, bootSettled_ = false;
  volatile bool cancelTask_ = false;
  Phase phase_ = Idle;
  Release pending_{}, active_{};
  String manifest_, taskUrl_, message_ = "idle", seenId_, seenFingerprint_, attemptId_, bootTarget_;
  String commands_[8], operations_[8];
  uint8_t commandCursor_ = 0;
  uint32_t attemptCount_ = 0;
};
extern ServerUpdater serverUpdater;
