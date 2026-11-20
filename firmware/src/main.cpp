#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_heap_caps.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include "display_config.h"
#include "app_version.h"
#include "cat_assets.h"
#include "pc_bridge.h"
#include "core/card_refresh.h"
#include "core/performance_lab.h"
#include "core/widget_contract.h"
#include "core/theme_palette.h"
#include "render/widget_renderer.h"

static PerformanceLab performanceLab;
#include "ota_manager.h"
#include "cat_pack.h"
#include "core/focus_timer.h"
#include "core/cat_behavior.h"
#include "core/motion_director.h"
#include "core/scene_manager.h"
#include "core/frame_pacer.h"
#include "core/frame_governor.h"
#include "core/performance_history.h"

// MEO DESK v1.7.0 - Living Motion Director with FPS-aware visual effects.
//   - LovyanGFX at 40 MHz SPI
//   - 2 x small internal-RAM DMA strips for the animated 144px cat
//   - an off-screen 16-bit sprite for the changing information card
//   - a static page frame, no full-screen redraw in the animation loop
//   - asynchronous Wi-Fi scan and real time via NTP when configured

// Wi-Fi/HTTP settings (router SSID + fallback AP) live in app_config.h.

constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

static uint16_t BG     = rgb(255, 248, 241);
static uint16_t PAPER  = rgb(255, 255, 255);
static uint16_t SHADOW = rgb(238, 226, 221);
static uint16_t INK    = rgb(43, 49, 69);
static uint16_t MUTED  = rgb(126, 130, 148);
static uint16_t MINT   = rgb(189, 238, 220);
static uint16_t TEAL   = rgb(53, 142, 123);
static uint16_t PEACH  = rgb(255, 220, 206);
static uint16_t CORAL  = rgb(236, 119, 112);
static uint16_t LILAC  = rgb(226, 215, 250);
static uint16_t GOLD   = rgb(253, 204, 102);
static uint16_t TRACK  = rgb(238, 237, 239);
static constexpr uint16_t CAT_SOURCE_BG = rgb(255, 248, 241);

static constexpr int SCREEN_W = 320;
static constexpr int SCREEN_H = 240;
static constexpr int CAT_X = 176;
static constexpr int CAT_Y = 49;
static constexpr int CAT_W = 144;
static constexpr int CAT_H = 144;
static constexpr int CAT_ZONE_H = 154;
static constexpr int CAT_STRIP_H = 16;
static constexpr int CARD_X = 10;
static constexpr int CARD_Y = 76;
static constexpr int CARD_W = 162;
static constexpr int CARD_H = 120;

static constexpr uint32_t PAGE_DURATION_MS = 8500;
static constexpr uint32_t STATS_INTERVAL_MS = 5000;

DeskDisplay tft;
lgfx::LGFX_Sprite cardSprite(&tft);
// Both LGFX_Device (hardware) and LGFX_Sprite (offscreen canvas)
// inherit from LovyanGFX. Keep the drawing target at their common base type.
lgfx::LovyanGFX *surface = &tft;
int surfaceOriginX = 0;
int surfaceOriginY = 0;
bool cardBuffered = false;

// DMA source buffers MUST remain unchanged until tft.waitDMA().
// 2 * (144 * 16 * 2) = 9216 bytes in DMA-capable internal RAM.
uint16_t *catDma[2] = {nullptr, nullptr};
bool catDmaReady = false;
bool psramAvailable = false;

enum Page : uint8_t {
  PC_PAGE, CLOCK_PAGE, FOCUS_PAGE, NETWORK_PAGE, SYSTEM_PAGE,
  MUSIC_PAGE, PRINTER_PAGE, AUDIO_PAGE, MOTO_PAGE,
  SLEEP_PAGE, HEALTH_PAGE, TRENDS_PAGE, HOME_PAGE, PC_NETWORK_PAGE, CUSTOM_PAGE, PAGE_COUNT
};

Page currentPage = HOME_PAGE;
SceneManager scenes(PAGE_COUNT);
CatBehavior petEngine;
MotionDirector motionDirector;
MotionDirector::Cue lastMotionCue{};
bool autoPages = false; // Stay on PC monitor until user enables auto pages.
bool catMotion = true;
// 0=stable 20, 1=balanced 25, 2=smooth 30, 3=adaptive 20/25.
// Profiles do not promise that a
// physical SPI TFT can sustain the requested rate under every Wi-Fi workload.
uint8_t fpsProfile = 1;
FramePacer catPacer(25);
FrameGovernor fpsGovernor;
PerformanceHistory perfHistory;
uint32_t catRenderWorstUs = 0;
uint32_t catRenderWorstLastUs = 0;
uint32_t catFramesMissedLast = 0;

void applyFpsProfile(uint8_t profile) {
  fpsGovernor.setProfile(profile);
  fpsProfile = fpsGovernor.profile();
  catPacer.setFps(fpsGovernor.target());
  catPacer.rebase(micros());
}
FocusTimer focusTimer;
bool colorTestActive = false;
bool otaScreenActive = false;
uint8_t themeChoice = 0; // 0 Cozy Day / 1 Midnight / 2 Auto by local NTP hour.
bool nightPalette = false;
uint32_t lastThemeCheckMs = 0;
bool lastWifiState = false;
bool lastPcState = false;
bool lastPcDemo = false;
uint32_t pageStartedMs = 0;
uint32_t lastCardMs = 0;
uint32_t lastCatMs = 0;
const uint16_t *lastCatImage = nullptr;
const uint16_t *lastCatPose = nullptr;
const uint16_t *fadeFromImage = nullptr;
uint32_t catFadeStartedMs = 0;
uint32_t lastStatsMs = 0;
uint32_t lastPetTickMs = 0;
uint32_t animatedFrames = 0;
float measuredCatFps = 0;
int nearbyNetworks = -1;
int strongestRssi = -127;
String strongestSSID;
enum class WifiScanPhase : uint8_t { IDLE, PREPARING, RUNNING };
WifiScanPhase wifiScanPhase = WifiScanPhase::IDLE;
uint32_t wifiScanStepMs = 0;
uint8_t wifiScanAttempts = 0;
static constexpr uint32_t WIFI_SCAN_SETTLE_MS = 650;
static constexpr uint32_t WIFI_SCAN_TIMEOUT_MS = 15000;

bool wifiConnected() { return WiFi.status() == WL_CONNECTED; }

bool realClock(tm &localTime) {
  const time_t now = time(nullptr);
  if (now < 1700000000) return false;
  localtime_r(&now, &localTime);
  return localTime.tm_year >= 124;
}

void applyPalette(bool night) {
  const ThemePalette p=themePalette(themeChoice,night);
  nightPalette=p.dark;
  BG=p.bg; PAPER=p.paper; SHADOW=p.shadow; INK=p.ink; MUTED=p.muted;
  MINT=p.mint; TEAL=p.teal; PEACH=p.peach; CORAL=p.coral;
  LILAC=p.lilac; GOLD=p.gold; TRACK=p.track;
}

bool themeShouldBeNight() {
  if (themeChoice == 1 || themeChoice == 3) return true;
  if (themeChoice != 2) return false;
  tm local{};
  return realClock(local) && (local.tm_hour >= 20 || local.tm_hour < 6);
}

void label(const String &text, int x, int y, uint16_t ink = INK,
           uint8_t size = 1) {
  surface->setFont(&lgfx::fonts::Font0);
  surface->setTextSize(size);
  surface->setTextWrap(false);
  surface->setTextColor(ink);
  surface->setCursor(x - surfaceOriginX, y - surfaceOriginY);
  surface->print(text);
}

void headline(const String &text, int x, int baseline,
              const lgfx::GFXfont *font, uint16_t ink = INK) {
  surface->setFont(font);
  surface->setTextSize(1);
  surface->setTextWrap(false);
  surface->setTextColor(ink);
  // Unlike Adafruit_GFX's setCursor() with FreeSans fonts, LovyanGFX's
  // default datum is top-left. The old call shifted large digits downward,
  // overlapping progress bars and captions on the physical screen.
  surface->setTextDatum(lgfx::textdatum_t::baseline_left);
  surface->drawString(text, x - surfaceOriginX, baseline - surfaceOriginY);
  surface->setTextDatum(lgfx::textdatum_t::top_left);
  surface->setFont(&lgfx::fonts::Font0);
}

void pill(int x, int y, int w, int h, uint16_t fill,
          const String &text, uint16_t ink = INK) {
  surface->fillRoundRect(x - surfaceOriginX, y - surfaceOriginY,
                         w, h, h / 2, fill);
  label(text, x + 10, y + (h - 8) / 2, ink);
}

void bar(int x, int y, int w, int h, int percentage,
         uint16_t background, uint16_t foreground) {
  if (percentage < 0) percentage = 0;
  if (percentage > 100) percentage = 100;
  surface->fillRoundRect(x - surfaceOriginX, y - surfaceOriginY,
                         w, h, h / 2, background);
  int active = (w * percentage) / 100;
  if (active > 0) {
    surface->fillRoundRect(x - surfaceOriginX, y - surfaceOriginY,
                           active, h, active < h ? active / 2 : h / 2,
                           foreground);
  }
}

void ring(int x, int y, int r, int percentage) {
  for (int angle = -90; angle < 270; angle += 6) {
    const float theta = angle * 0.017453293f;
    surface->fillCircle(x + (int)(r * cosf(theta)) - surfaceOriginX,
                        y + (int)(r * sinf(theta)) - surfaceOriginY,
                        3, TRACK);
  }
  const int stop = -90 + percentage * 360 / 100;
  for (int angle = -90; angle < stop; angle += 6) {
    const float theta = angle * 0.017453293f;
    surface->fillCircle(x + (int)(r * cosf(theta)) - surfaceOriginX,
                        y + (int)(r * sinf(theta)) - surfaceOriginY,
                        3, TEAL);
  }
}

const char *pageName(Page page) {
  switch (page) {
    case CUSTOM_PAGE: return "MY WIDGET DESK";
    case HOME_PAGE:    return "HELLO, HUMAN";
    case PC_NETWORK_PAGE: return "PC NETWORK";
    case PC_PAGE:      return "PC MONITOR";
    case HEALTH_PAGE:  return "PC HEALTH";
    case TRENDS_PAGE:  return "PC TRENDS";
    case CLOCK_PAGE:   return "GOOD MORNING";
    case FOCUS_PAGE:   return "FOCUS TIMER";
    case NETWORK_PAGE: return "WI-FI STATUS";
    case SYSTEM_PAGE:  return "DEVICE STATS";
    case MUSIC_PAGE:   return "NOW PLAYING";
    case PRINTER_PAGE: return "3D PRINTING";
    case AUDIO_PAGE:   return "AUDIO SPACE";
    case MOTO_PAGE:    return "RIDE MODE";
    case SLEEP_PAGE:   return "SLEEPY TIME";
    default:           return "MEO DESK";
  }
}

const char *sceneLabel(Page page) {
  switch (page) {
    case CUSTOM_PAGE: return "WIDGETS";
    case HOME_PAGE: return "HOME";
    case PC_PAGE: return "PC";
    case MUSIC_PAGE: return "MUSIC";
    case FOCUS_PAGE: return "FOCUS";
    case CLOCK_PAGE: return "CLOCK";
    case NETWORK_PAGE: return "WI-FI";
    case SYSTEM_PAGE: return "DEVICE";
    case HEALTH_PAGE: return "HEALTH";
    case TRENDS_PAGE: return "TRENDS";
    case PC_NETWORK_PAGE: return "NET";
    case PRINTER_PAGE: return "PRINT";
    case AUDIO_PAGE: return "AUDIO";
    case MOTO_PAGE: return "MOTO";
    case SLEEP_PAGE: return "SLEEP";
    default: return "DESK";
  }
}

const uint16_t *petSprite() {
  switch (petEngine.persona()) {
    case CatBehavior::SEARCHING: return cat_rescue;
    case CatBehavior::SLEEPY: return cat_sleep;
    case CatBehavior::MUSIC: return cat_music;
    case CatBehavior::FOCUS: return cat_paused;
    case CatBehavior::BUSY: return cat_storage;
    case CatBehavior::HOT: return cat_health;
    case CatBehavior::BATTERY: return cat_shutdown;
    case CatBehavior::CELEBRATE: return cat_healthy;
    case CatBehavior::COZY: return cat_healthy;
    case CatBehavior::WAKING: return cat_boot;
    case CatBehavior::GREETING: return cat_healthy;
  }
  return cat_healthy;
}

const uint16_t *catForMood(const char *emotion) {
  if (!strcmp(emotion, "stressed")) return cat_storage;
  if (!strcmp(emotion, "sleep")) return cat_sleep;
  if (!strcmp(emotion, "low_battery")) return cat_shutdown;
  if (!strcmp(emotion, "hot")) return cat_health;
  if (!strcmp(emotion, "healthy")) return cat_healthy;
  if (!strcmp(emotion, "paused")) return cat_paused;
  if (!strcmp(emotion, "rescue")) return cat_rescue;
  if (!strcmp(emotion, "power")) return cat_power;
  if (!strcmp(emotion, "hotspot")) return cat_hotspot;
  if (!strcmp(emotion, "demo")) return cat_music;
  if (!strcmp(emotion, "music")) return cat_music;
  return cat_rescue;
}

const uint16_t *builtinForPage(Page page) {
  // Health alerts always override decorative scene art, including during Focus.
  const char *mood = pcEmotionName();
  if (!strcmp(mood, "hot") || !strcmp(mood, "low_battery") ||
      !strcmp(mood, "stressed")) return catForMood(mood);
  // A web gallery preview shows over ANY scene, then restores the original cat.
  if (pcPreviewActive()) return catForMood(pcEmotionName());
  switch (page) {
    case PC_PAGE:
    case HEALTH_PAGE:
    case TRENDS_PAGE: return petSprite();
    case CUSTOM_PAGE:
    case HOME_PAGE:    return petSprite();
    case PC_NETWORK_PAGE: return cat_network;
    case CLOCK_PAGE:   return cat_clock;
    case FOCUS_PAGE:   return petSprite();
    case NETWORK_PAGE: return wifiConnected() ? cat_network : cat_hotspot;
    case SYSTEM_PAGE:  return cat_storage;
    case MUSIC_PAGE:   return petSprite();
    case PRINTER_PAGE: return cat_bambu;
    case AUDIO_PAGE:   return cat_audio;
    case MOTO_PAGE:    return cat_boot;
    case SLEEP_PAGE:   return cat_sleep;
    default:           return cat_clock;
  }
}

const uint16_t *catForPage(Page page) {
  // One mapping per cat selection. The inner pixel loop still copies directly
  // from PSRAM into the original internal DMA strips, with no Flash reads.
  return catPack.lookup(builtinForPage(page));
}

bool pageHasRealData(Page page) {
  if (page == PC_PAGE || page == HEALTH_PAGE || page == TRENDS_PAGE ||
      page == HOME_PAGE || page == PC_NETWORK_PAGE || page == CUSTOM_PAGE)
    return pcTelemetryFresh() && !pcTelemetry.demo;
  if (page == CLOCK_PAGE) {
    tm local{};
    return realClock(local);
  }
  if (page == MUSIC_PAGE) return pcMedia.fresh(millis()) && pcMedia.available;
  return page == NETWORK_PAGE || page == SYSTEM_PAGE || page == FOCUS_PAGE;
}

void drawHeaderStatus() {
  const bool live = pageHasRealData(currentPage);
  const bool waiting = (currentPage == PC_PAGE || currentPage == HEALTH_PAGE ||
                        currentPage == TRENDS_PAGE || currentPage == HOME_PAGE ||
                        currentPage == PC_NETWORK_PAGE)
                       && !pcTelemetryFresh();
  tft.fillRoundRect(244, 9, 65, 25, 12, live ? MINT : PEACH);
  label(waiting ? "WAIT" : live ? "LIVE" : "DEMO", 266, 17,
        live ? TEAL : CORAL);
}

void drawPageFrame() {
  // Full-screen redraw is allowed here ONLY when opening/changing a page.
  tft.fillScreen(BG);
  tft.fillRoundRect(9, 7, 226, 30, 15, PAPER);
  tft.fillCircle(19, 22, 3, CORAL);
  label("mimi.", 30, 12, CORAL, 2);
  label(String(nightPalette ? "NIGHT / " : "DESK / ") + MEO_VERSION, 100, 18, MUTED);
  drawHeaderStatus();

  label(pageName(currentPage), 14, 45, INK, 2);
  tft.fillRoundRect(CARD_X + 3, CARD_Y + 4,
                    CARD_W, CARD_H, 15, SHADOW);
  tft.fillRoundRect(CARD_X, CARD_Y, CARD_W, CARD_H, 15, PAPER);

  const int positions[] = {11, 85, 159, 233};
  for (int i = 0; i < 4; ++i) {
    const Page featured = (Page)scenes.slot(i);
    const bool active = featured == currentPage;
    tft.fillRoundRect(positions[i], 211, 71, 21, 9, active ? MINT : PAPER);
    label(sceneLabel(featured), positions[i] + 11, 217, active ? TEAL : MUTED);
  }
}

// RGB565 crossfade between mood sprites, done inside the SAME DMA strips.
// No full-frame sprite allocation and no extra full-screen SPI transfers.
static uint16_t mix565(uint16_t first, uint16_t second, uint8_t amount) {
  const uint32_t inv = 255U - amount;
  const uint32_t r = (((first >> 11) & 31U) * inv + ((second >> 11) & 31U) * amount + 127U) / 255U;
  const uint32_t g = (((first >> 5) & 63U) * inv + ((second >> 5) & 63U) * amount + 127U) / 255U;
  const uint32_t b = ((first & 31U) * inv + (second & 31U) * amount + 127U) / 255U;
  return (uint16_t)((r << 11) | (g << 5) | b);
}

static uint16_t backgroundForTheme(uint16_t pixel) {
  return pixel == CAT_SOURCE_BG ? BG : pixel;
}

// Asset backgrounds were flattened over warm ivory (0xFF9E); remap that exact
// key for each palette. Anti-aliased edge pixels may retain a small light halo.
void renderCatStrip(uint16_t *dest, int rowStart, int bob,
                    const uint16_t *cat, const uint16_t *previous,
                    uint8_t fadeAmount, bool sparkle) {
  const uint32_t labStarted = performanceLab.enabled() ? micros() : 0;
  for (int localRow = 0; localRow < CAT_STRIP_H; ++localRow) {
    const int row = rowStart + localRow;
    uint16_t *out = dest + localRow * CAT_W;
    if (row >= CAT_ZONE_H) {
      for (int x = 0; x < CAT_W; ++x) out[x] = BG;
      continue;
    }
    const int sourceRow = row - bob;
    if (sourceRow >= 0 && sourceRow < CAT_H) {
      const uint16_t *source = cat + sourceRow * CAT_W;
      if (!previous && BG == CAT_SOURCE_BG) {
        memcpy(out, source, CAT_W * sizeof(uint16_t));
      } else {
        const uint16_t *from = previous ? previous + sourceRow * CAT_W : nullptr;
        for (int x = 0; x < CAT_W; ++x) {
          const uint16_t target = backgroundForTheme(source[x]);
          out[x] = from ? mix565(backgroundForTheme(from[x]), target, fadeAmount) : target;
        }
      }
    } else {
      for (int x = 0; x < CAT_W; ++x) out[x] = BG;
      // A small ground shadow appears only below the moving image.
      if (row >= 149 && row <= 153) {
        const int radius = 43 - abs(row - 151) * 5;
        for (int x = 72 - radius; x <= 72 + radius; ++x) {
          if (x >= 0 && x < CAT_W) out[x] = SHADOW;
        }
      }
    }
    if (sparkle && row >= 19 && row <= 25) {
      for (int x = 116; x <= 122; ++x) {
        if (x == 119 || row == 22 || (abs(x - 119) == 1 && abs(row - 22) == 1))
          out[x] = GOLD;
      }
    }
  }
  if (performanceLab.enabled()) performanceLab.add(PerformanceLab::CAT_CPU, micros() - labStarted);
}

MotionDirector::Context liveMotionContext(uint32_t now) {
  MotionDirector::Context context{};
  context.persona = petEngine.persona();
  context.enabled = catMotion;
  context.quiet = healthSettings.quiet;
  context.playing = pcMedia.playingNow(now);
  context.focusing = focusTimer.running() && focusTimer.phase() == FocusTimer::WORK;
  context.realPc = pcTelemetryFresh() && !pcTelemetry.demo;
  if (context.realPc) {
    context.cpu = pcTelemetry.cpuPercent;
    context.gpu = pcTelemetry.gpuPercent;
  }
  return context;
}

void drawAnimatedCat(uint32_t now) {
  const uint32_t labStarted = performanceLab.enabled() ? micros() : 0;
  if (!catDmaReady) {
    // Safe fallback if DMA-capable memory cannot be allocated.
    // The cat remains static, while the rest of the UI continues to work.
    return;
  }

  const MotionDirector::Cue cue = motionDirector.sample(liveMotionContext(now), now);
  lastMotionCue = cue;
  const int bob = 3 + (cue.bob ?
      (int)lroundf(cue.bob * sinf(now * (6.2831853f / cue.bobPeriodMs))) : 0);
  const uint16_t *pose = builtinForPage(currentPage);
  const uint16_t *cat = catPack.frameAt(pose, cue.cel);
  // Each motion cel has a distinct PSRAM address. Fade only on *pose* changes,
  // otherwise we would accidentally crossfade the cat every 170 ms.
  if (pose != lastCatPose) {
    fadeFromImage = lastCatImage;
    lastCatPose = pose;
    catFadeStartedMs = now;
  }
  lastCatImage = cat;
  const uint32_t age = (uint32_t)(now - catFadeStartedMs);
  if (age >= 230UL || !catMotion || !cue.fadeAllowed) fadeFromImage = nullptr;
  const uint8_t fade = fadeFromImage ? (uint8_t)min(255UL, age * 255UL / 230UL) : 255;
  const bool sparkle = cue.sparkle;

  // Start DMA for strip N while the CPU prepares strip N+1.
  // Two buffers are alternated and never modified before DMA completes.
  int which = 0;
  renderCatStrip(catDma[which], 0, bob, cat, fadeFromImage, fade, sparkle);
  for (int row = 0; row < CAT_ZONE_H; row += CAT_STRIP_H) {
    int stripHeight = CAT_ZONE_H - row;
    if (stripHeight > CAT_STRIP_H) stripHeight = CAT_STRIP_H;

    // The packed assets store *normal* RGB565 values (e.g. ivory 0xFF9E).
    // A bare uint16_t* is treated as pre-swapped data by LovyanGFX, which
    // made the cats neon blue/magenta in the first v0.4 hardware video.
    // rgb565_t explicitly requests correct RGB565 byte order with DMA.
    const uint32_t submitStarted = performanceLab.enabled() ? micros() : 0;
    tft.pushImageDMA(CAT_X, CAT_Y + row, CAT_W, stripHeight,
                     reinterpret_cast<const lgfx::rgb565_t *>(catDma[which]));
    if (performanceLab.enabled()) {
      performanceLab.add(PerformanceLab::DMA_SUBMIT, micros() - submitStarted);
      performanceLab.pixels(CAT_W, stripHeight);
    }

    const int nextRow = row + CAT_STRIP_H;
    if (nextRow < CAT_ZONE_H) {
      renderCatStrip(catDma[which ^ 1], nextRow, bob, cat, fadeFromImage, fade, sparkle);
    }
    const uint32_t waitStarted = performanceLab.enabled() ? micros() : 0;
    tft.waitDMA();
    if (performanceLab.enabled()) performanceLab.add(PerformanceLab::DMA_WAIT, micros() - waitStarted);
    which ^= 1;
  }
  ++animatedFrames;
  if (performanceLab.enabled()) performanceLab.add(PerformanceLab::CAT_WALL, micros() - labStarted);
}

void drawCard() {
  const uint32_t labStarted = performanceLab.enabled() ? micros() : 0;
  if (cardBuffered) {
    cardSprite.fillScreen(BG);
    cardSprite.fillRoundRect(0, 0, CARD_W, CARD_H, 15, PAPER);
    surface = &cardSprite;
    surfaceOriginX = CARD_X;
    surfaceOriginY = CARD_Y;
  } else {
    // Fallback when a contiguous 39 KB sprite cannot be allocated.
    surface = &tft;
    surfaceOriginX = surfaceOriginY = 0;
    tft.fillRoundRect(CARD_X, CARD_Y, CARD_W, CARD_H, 15, PAPER);
  }

  const uint32_t nowMs = millis();
  const uint32_t seconds = nowMs / 1000;

  switch (currentPage) {
    case CUSTOM_PAGE: {
      WidgetSnapshot data;
      tm local{};
      if (realClock(local)) strftime(data.clock,sizeof(data.clock),"%H:%M:%S",&local);
      if (pcTelemetryFresh()) { data.cpu=pcTelemetry.cpuPercent; data.gpu=pcTelemetry.gpuPercent; data.ram=pcTelemetry.ramPercent; data.down=pcTelemetry.netDownKbps; }
      data.focusSeconds=focusTimer.remainingSeconds(nowMs);
      if (pcMedia.fresh(nowMs) && pcMedia.available) data.track=pcMedia.title;
      drawWidgetScene(*surface,pcBridgeSceneLayout(),data,CARD_X-surfaceOriginX,CARD_Y-surfaceOriginY,INK,MUTED,TRACK);
      break;
    }
    case HOME_PAGE: {
      tm local{};
      const bool validTime = realClock(local);
      char digits[6];
      if (validTime) strftime(digits, sizeof(digits), "%H:%M", &local);
      else snprintf(digits, sizeof(digits), "--:--");
      label(validTime ? "A LITTLE MOMENT" : "WAITING FOR NTP", 19, 88, MUTED);
      headline(digits, 17, 132, &lgfx::fonts::FreeSansBold24pt7b);
      if (validTime) {
        char date[26];
        strftime(date, sizeof(date), "%a / %d %b", &local);
        label(date, 19, 143, TEAL);
      }
      if (pcTelemetryFresh()) {
        label("CPU " + String((int)pcTelemetry.cpuPercent) + "%", 19, 168, CORAL);
        label("RAM " + String((int)pcTelemetry.ramPercent) + "%", 95, 168, TEAL);
        label(pcTelemetry.demo ? "DEMO / SYNTHETIC" : "WINDOWS CONNECTED", 19, 183, MUTED);
      } else label("PC OFFLINE / N-A", 19, 181, CORAL);
      break;
    }
    case PC_PAGE: {
      if (!pcTelemetryFresh()) {
        label("WAITING FOR WINDOWS", 19, 87, CORAL);
        label("Start pc_agent.py", 19, 113, INK);
        label("HTTP  /  PORT 80", 19, 130, MUTED);
        pill(18, 150, 112, 22, PEACH, "NO PC DATA", CORAL);
        label(pcNetworkIP(), 19, 181, TEAL);
        break;
      }
      String name(pcTelemetry.pcName);
      if (name.length() > 20) name = name.substring(0, 20);
      label(name, 19, 86, pcTelemetry.demo ? CORAL : TEAL);
      label("CPU", 19, 101, MUTED);
      label(String((int)lroundf(pcTelemetry.cpuPercent)) + "%", 126, 101, INK);
      bar(19, 114, 135, 6, (int)pcTelemetry.cpuPercent, TRACK,
          pcTelemetry.cpuPercent >= 88 ? CORAL : TEAL);

      label("GPU", 19, 127, MUTED);
      label(pcTelemetry.gpuPercent >= 0
              ? String((int)lroundf(pcTelemetry.gpuPercent)) + "%" : "N/A",
            126, 127, INK);
      bar(19, 140, 135, 6, (int)max(0.f, pcTelemetry.gpuPercent), TRACK, LILAC);

      label("RAM", 19, 153, MUTED);
      label(String((int)lroundf(pcTelemetry.ramPercent)) + "%", 126, 153, INK);
      bar(19, 166, 135, 6, (int)pcTelemetry.ramPercent, TRACK, CORAL);
      label(pcTelemetry.demo ? "SIMULATED / NOT REAL" :
            (String(pcTelemetry.ramUsedGB, 1) + " / " +
             String(pcTelemetry.ramTotalGB, 1) + " GB"), 19, 182, MUTED);
      break;
    }
    case HEALTH_PAGE: {
      if (!pcTelemetryFresh()) {
        label("NO PC TELEMETRY", 19, 88, CORAL);
        label("Awaiting PC agent", 19, 113, INK);
        label("Live health checks", 19, 138, MUTED);
        pill(18, 155, 111, 22, PEACH, "OFFLINE", CORAL);
        break;
      }
      label(pcTelemetry.demo ? "DEMO / NOT REAL" : "HEALTH SENSORS", 19, 87,
            pcTelemetry.demo ? CORAL : TEAL);
      label("CPU TEMP", 19, 106, MUTED);
      label(pcTelemetry.cpuTempC >= 0
              ? String((int)lroundf(pcTelemetry.cpuTempC)) + "C" : "N/A",
            123, 106, INK);
      label("GPU TEMP", 19, 127, MUTED);
      label(pcTelemetry.gpuTempC >= 0
              ? String((int)lroundf(pcTelemetry.gpuTempC)) + "C" : "N/A",
            123, 127, INK);
      label("BATTERY", 19, 148, MUTED);
      label(pcTelemetry.batteryPercent >= 0
              ? String((int)lroundf(pcTelemetry.batteryPercent)) + "%" : "N/A",
            123, 148, INK);
      const String mood = pcEmotionName();
      label("CAT / " + mood, 19, 172, TEAL);
      label(healthSettings.quiet ? "QUIET MODE" : "SMART ALERTS ON", 19, 184,
            MUTED);
      break;
    }
    case TRENDS_PAGE: {
      label("5 MIN / CPU GPU RAM", 19, 87, TEAL);
      const int left = 19, top = 103, width = 135, height = 65;
      const int bottom = top + height;
      for (int grid = 0; grid <= 2; ++grid) {
        const int y = top + grid * height / 2;
        surface->drawFastHLine(left - surfaceOriginX, y - surfaceOriginY,
                               width, TRACK);
      }
      const uint16_t count = pcHistoryCount();
      if (count < 2) {
        label("Collecting samples...", 19, 137, MUTED);
      } else {
        const uint16_t step = max((uint16_t)1, (uint16_t)((count - 1) / 130));
        const uint16_t palette[] = {TEAL, CORAL, rgb(143, 110, 208)};
        for (int series = 0; series < 3; ++series) {
          bool havePrevious = false;
          int prevX = 0, prevY = 0;
          for (uint16_t i = 0; i < count; i += step) {
            PcHistoryPoint point;
            if (!pcHistoryAt(i, point)) continue;
            const uint8_t val = series == 0 ? point.cpu :
                                series == 1 ? point.gpu : point.ram;
            if (val == 255) { havePrevious = false; continue; }
            const int x = left + (uint32_t)i * (width - 1) / (count - 1);
            const int y = bottom - (int)val * height / 100;
            if (havePrevious)
              surface->drawLine(prevX - surfaceOriginX, prevY - surfaceOriginY,
                                x - surfaceOriginX, y - surfaceOriginY, palette[series]);
            havePrevious = true;
            prevX = x; prevY = y;
          }
        }
      }
      label("CPU", 19, 179, TEAL);
      label("GPU", 69, 179, CORAL);
      label("RAM", 121, 179, rgb(143, 110, 208));
      break;
    }
    case CLOCK_PAGE: {
      tm local{};
      const bool live = realClock(local);
      int hour, minute, second;
      char date[25];
      if (live) {
        hour = local.tm_hour;
        minute = local.tm_min;
        second = local.tm_sec;
        strftime(date, sizeof(date), "%a, %d %b", &local);
      } else {
        const uint32_t demo = (8 * 3600UL + 45 * 60UL + seconds) % 86400UL;
        hour = demo / 3600;
        minute = (demo / 60) % 60;
        second = demo % 60;
        snprintf(date, sizeof(date), "Thu, 19 Nov (demo)");
      }
      char timeText[6];
      snprintf(timeText, sizeof(timeText), "%02d:%02d", hour, minute);
      label(date, 19, 87, MUTED);
      headline(timeText, 17, 128, &lgfx::fonts::FreeSansBold24pt7b);
      pill(18, 143, 109, 22, MINT, "SECONDS " + String(second), TEAL);
      label(live ? "NTP time / GMT+7" : "Preview clock", 19, 181, MUTED);
      break;
    }
    case FOCUS_PAGE: {
      const uint32_t remain = focusTimer.remainingSeconds(nowMs);
      const uint32_t total = focusTimer.durationMs() / 1000UL;
      char timer[16];
      snprintf(timer, sizeof(timer), "%02lu:%02lu",
               (unsigned long)(remain / 60), (unsigned long)(remain % 60));
      char phaseLabel[32];
      snprintf(phaseLabel, sizeof(phaseLabel), "%s / %u MIN",
               focusTimer.phase() == FocusTimer::BREAK ? "BREAK" : "FOCUS",
               static_cast<unsigned>(focusTimer.phase() == FocusTimer::BREAK ? focusTimer.breakMinutes() : focusTimer.workMinutes()));
      label(phaseLabel, 19, 88, MUTED);
      headline(timer, 17, 128, &lgfx::fonts::FreeSansBold24pt7b);
      bar(19, 143, 137, 9, 100 - (int)(remain * 100 / total), TRACK,
          focusTimer.phase() == FocusTimer::BREAK ? TEAL : CORAL);
      label(String("SESSIONS  ") + focusTimer.sessionsDone(), 19, 167, TEAL);
      label(focusTimer.running() ? "f PAUSE / WEB CONTROL" : "f START / WEB CONTROL", 19, 182, MUTED);
      break;
    }
    case NETWORK_PAGE: {
      if (wifiConnected()) {
        label("CONNECTED", 19, 88, TEAL);
        String ssid(WiFi.SSID());
        if (ssid.length() > 18) ssid = ssid.substring(0, 18);
        label(ssid, 19, 107, INK);
        headline(String(WiFi.RSSI()), 17, 146,
                 &lgfx::fonts::FreeSansBold18pt7b);
        label("dBm", 111, 129, MUTED);
        label(WiFi.localIP().toString(), 19, 178, MUTED);
      } else {
        label("NOT CONNECTED", 19, 89, CORAL);
        if (wifiScanPhase != WifiScanPhase::IDLE) {
          headline("...", 19, 143, &lgfx::fonts::FreeSansBold24pt7b);
          label("Scanning 2.4 GHz", 19, 161, MUTED);
          label("Please wait", 19, 181, TEAL);
        } else if (nearbyNetworks == -2) {
          label("SCAN FAILED", 19, 130, CORAL, 2);
          label("See Serial log", 19, 160, MUTED);
          label("r  retry scan", 19, 181, TEAL);
        } else {
          headline(nearbyNetworks < 0 ? "?" : String(nearbyNetworks), 19, 143,
                   &lgfx::fonts::FreeSansBold24pt7b);
          label(nearbyNetworks < 0 ? "networks unknown" : "networks nearby", 19, 161, MUTED);
          label("r  scan 2.4 GHz", 19, 181, TEAL);
        }
      }
      break;
    }
    case PC_NETWORK_PAGE: {
      if (!pcTelemetryFresh()) {
        label("PC NETWORK / OFFLINE", 19, 90, CORAL);
        label("Waiting for Windows", 19, 119, MUTED);
        label("Start Companion agent", 19, 152, TEAL);
        break;
      }
      label(pcTelemetry.demo ? "SIMULATED / DEMO" : "LIVE NETWORK TRAFFIC", 19, 88,
            pcTelemetry.demo ? CORAL : TEAL);
      label("DOWNLOAD", 19, 105, MUTED);
      label(pcTelemetry.netDownKbps >= 0 ? String(pcTelemetry.netDownKbps, 1) + " KB/s" : "N/A",
            19, 121, INK, 2);
      label("UPLOAD", 19, 146, MUTED);
      label(pcTelemetry.netUpKbps >= 0 ? String(pcTelemetry.netUpKbps, 1) + " KB/s" : "N/A",
            19, 160, INK);
      label(pcTelemetry.vramTotalGB > 0 ?
            ("VRAM " + String(pcTelemetry.vramUsedGB, 1) + "/" +
             String(pcTelemetry.vramTotalGB, 1) + " GB") : "VRAM / N-A",
            19, 182, TEAL);
      break;
    }
    case SYSTEM_PAGE: {
      const uint32_t freeKb = ESP.getFreeHeap() / 1024UL;
      const uint32_t totalKb = ESP.getHeapSize() / 1024UL;
      label("AVAILABLE MEMORY", 19, 90, MUTED);
      headline(String(freeKb), 17, 135,
               &lgfx::fonts::FreeSansBold18pt7b);
      label("KB", 116, 119, MUTED);
      bar(19, 145, 135, 9,
          totalKb ? (int)(freeKb * 100 / totalKb) : 0, TRACK, TEAL);
      label("UP " + String(seconds) + "s", 19, 171, MUTED);
      break;
    }
    case MUSIC_PAGE: {
      const bool valid = pcMedia.fresh(nowMs) && pcMedia.available;
      const bool art = valid && pcBridgeArtworkReady();
      label(valid ? (pcMedia.playing ? "NOW PLAYING" : "PAUSED") : "NO MEDIA SESSION",
            19, 87, valid ? TEAL : MUTED);
      if (valid) {
        String track(pcMedia.title);
        if (!track.length()) track = "Untitled track";
        String artist(pcMedia.artist);
        if (art) {
          // The album tile occupies x19..82; two 10-char title lines fit next
          // to it. Actual pixels are pasted *after* the buffered card.
          label(track.substring(0, 10), 93, 116, INK);
          if (track.length() > 10) label(track.substring(10, 20), 93, 128, INK);
          label(artist.length() ? artist.substring(0, 11) : "Unknown", 93, 150, MUTED);
        } else {
          if (track.length() > 21) track = track.substring(0, 19) + "..";
          label(track, 19, track.length() > 11 ? 115 : 110, INK,
                track.length() > 11 ? 1 : 2);
          if (artist.length() > 21) artist = artist.substring(0, 20) + "...";
          label(artist.length() ? artist : "Unknown artist", 19, 143, MUTED);
        }
        const uint32_t duration = pcMedia.durationMs;
        const uint32_t elapsed = pcMedia.positionMs + (pcMedia.playing ? nowMs - pcMedia.receivedMs : 0);
        // Keep the 64x64 cover (y=107..170) clear of the timeline.
        // In the text-only layout we retain the denser original spacing.
        const int timelineY = art ? 174 : 158;
        const int stampY = art ? 185 : 180;
        bar(19, timelineY, 135, 7, duration ? (int)(100ULL * min(elapsed, duration) / duration) : 0,
            TRACK, CORAL);
        char stamp[24];
        snprintf(stamp, sizeof(stamp), "%02lu:%02lu / %02lu:%02lu",
                 (unsigned long)(elapsed / 60000UL), (unsigned long)((elapsed / 1000UL) % 60),
                 (unsigned long)(duration / 60000UL), (unsigned long)((duration / 1000UL) % 60));
        label(duration ? stamp : "TIME N/A", 19, stampY, MUTED);
      } else {
        label("Nothing playing", 19, 123, INK, 2);
        label("OPEN A MUSIC PLAYER", 19, 154, MUTED);
        label("REAL WINDOWS MEDIA", 19, 179, TEAL);
      }
      break;
    }
    case PRINTER_PAGE: {
      const int progress = 55 + (seconds / 6) % 45;
      ring(90, 128, 39, progress);
      headline(String(progress), 58, 136,
               &lgfx::fonts::FreeSansBold18pt7b);
      label("%", 114, 122, MUTED);
      label("205C NOZZLE | MOCK", 22, 183, MUTED);
      break;
    }
    case AUDIO_PAGE: {
      label("YOUR SOUND LEVEL / DEMO", 19, 90, MUTED);
      headline("65%", 17, 132, &lgfx::fonts::FreeSansBold24pt7b);
      for (int i = 0; i < 10; ++i) {
        const int h = 8 + (int)(fabsf(sinf(nowMs * 0.007f + i * .85f)) * 22);
        surface->fillRoundRect(20 + i * 13 - surfaceOriginX,
                               180 - h - surfaceOriginY,
                               7, h, 3, i % 3 ? CORAL : TEAL);
      }
      break;
    }
    case MOTO_PAGE: {
      const int speed = (int)(fabsf(sinf(nowMs / 3200.f)) * 65);
      label("SPEED / MOCK", 19, 90, MUTED);
      headline(String(speed), 17, 134,
               &lgfx::fonts::FreeSansBold24pt7b);
      label("km/h", 113, 125, MUTED);
      bar(19, 148, 135, 8, speed * 100 / 65, TRACK, CORAL);
      label("GPS -- | BAT 84%", 19, 178, MUTED);
      break;
    }
    case SLEEP_PAGE: {
      label("REST TIME", 19, 90, MUTED);
      headline("Zzz...", 17, 139,
               &lgfx::fonts::FreeSansBold18pt7b);
      pill(19, 158, 123, 23, LILAC, "Sweet dreams", INK);
      break;
    }
    default: break;
  }

  surface = &tft;
  surfaceOriginX = surfaceOriginY = 0;
  if (cardBuffered) {
    cardSprite.pushSprite(CARD_X, CARD_Y);
    performanceLab.pixels(CARD_W, CARD_H);
    tft.waitDMA(); // Safe to modify cardSprite on the next update.
  }
  if (currentPage == MUSIC_PAGE && pcBridgeArtworkReady()) {
    const uint16_t *pixels = pcBridgeArtworkPixels();
    if (pixels) {
      tft.pushImage(19, 107, 64, 64, reinterpret_cast<const lgfx::rgb565_t *>(pixels));
      performanceLab.pixels(64, 64);
    }
  }
  lastCardMs = nowMs;
  if (performanceLab.enabled()) performanceLab.add(PerformanceLab::CARD_WALL, micros() - labStarted);
}

uint32_t cardIntervalMs(Page page) {
  switch (page) {
    case AUDIO_PAGE:
    case MOTO_PAGE:    return 120;
    case MUSIC_PAGE:   return 500;
    case PRINTER_PAGE: return 250;
    case SLEEP_PAGE:   return 500;
    default:           return 1000;
  }
}

void setPage(Page page) {
  tft.waitDMA();
  currentPage = page;
  // Focus state persists across pages; navigating never pauses the timer.
  pageStartedMs = millis();
  drawPageFrame();
  drawCard();
  if (catDmaReady) {
    drawAnimatedCat(millis());
  } else {
    tft.pushImage(CAT_X, CAT_Y + 3, CAT_W, CAT_H,
                  reinterpret_cast<const lgfx::rgb565_t *>(catForPage(page)));
  }
  lastCatMs = millis();
  catPacer.rebase(micros());
  lastWifiState = wifiConnected();
  lastPcState = pcTelemetryFresh();
  lastPcDemo = pcTelemetry.demo;
  Serial.printf("Page %d: %s\n", (int)page, pageName(page));
}

void drawColorTest() {
  const uint16_t colors[] = {
    rgb(255, 0, 0), rgb(0, 255, 0), rgb(0, 0, 255),
    rgb(0, 255, 255), rgb(255, 0, 255), rgb(255, 255, 0)
  };
  const char *names[] = {"RED", "GREEN", "BLUE", "CYAN", "MAGENTA", "YELLOW"};
  tft.fillScreen(rgb(0, 0, 0));
  for (int i = 0; i < 6; ++i) {
    const int x0 = i * SCREEN_W / 6;
    const int x1 = (i + 1) * SCREEN_W / 6;
    tft.fillRect(x0, 15, x1 - x0, 80, colors[i]);
    label(names[i], x0 + 3, 52,
          (i == 2 || i == 4) ? rgb(255, 255, 255) : rgb(0, 0, 0));
  }
  for (int i = 0; i < 8; ++i) {
    uint8_t shade = (uint8_t)(255 * i / 7);
    tft.fillRect(i * 40, 115, 40, 45, rgb(shade, shade, shade));
  }
  tft.fillRect(0, 180, 160, 60, rgb(0, 0, 0));
  tft.fillRect(160, 180, 160, 60, rgb(255, 255, 255));
  label("BLACK", 40, 200, rgb(255, 255, 255), 2);
  label("WHITE", 205, 200, rgb(0, 0, 0), 2);
}

void startWifiScan() {
  if (wifiConnected()) {
    Serial.printf("[WIFI] Already connected: %s / %s\n",
                  WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
    return;
  }
  if (wifiScanPhase != WifiScanPhase::IDLE) {
    Serial.println("[WIFI] Scan in progress; ignored repeated 'r' (no restart)");
    return;
  }
  // AP+STA scanning while STA is associating can fail with ESP_ERR_WIFI_STATE.
  // Pause STA and let its disconnect event settle without blocking animation.
  pcBridgeSetWifiScanActive(true);
  nearbyNetworks = -1;
  wifiScanPhase = WifiScanPhase::PREPARING;
  wifiScanStepMs = millis();
  wifiScanAttempts = 0;
  Serial.println("[WIFI] Preparing asynchronous 2.4 GHz scan...");
  if (!colorTestActive && currentPage == NETWORK_PAGE) drawCard();
}

void finishWifiScan(int result) {
  if (result >= 0) {
    nearbyNetworks = result;
    strongestRssi = -127;
    strongestSSID = "";
    Serial.printf("[WIFI] Found %d networks (2.4 GHz):\n", result);
    for (int i = 0; i < result; ++i) {
      const int rssi = WiFi.RSSI(i);
      if (rssi > strongestRssi) {
        strongestRssi = rssi;
        strongestSSID = WiFi.SSID(i);
      }
      if (i < 12) {
        Serial.printf("[WIFI] %2d. %s | %ld dBm | ch %ld\n",
                      i + 1, WiFi.SSID(i).c_str(), (long)rssi, (long)WiFi.channel(i));
      }
    }
    if (result > 12) Serial.printf("[WIFI] ... and %d more\n", result - 12);
  } else {
    nearbyNetworks = -2; // Explicit error, never show failed scans as zero.
    Serial.println("[WIFI] Scan FAILED or timed out; press 'r' to retry");
  }
  WiFi.scanDelete();
  wifiScanPhase = WifiScanPhase::IDLE;
  pcBridgeSetWifiScanActive(false); // Resume router connection after scan.
  if (!colorTestActive && currentPage == NETWORK_PAGE) drawCard();
}

void pollWifiScan() {
  if (wifiScanPhase == WifiScanPhase::IDLE) return;
  const uint32_t nowMs = millis();
  if (wifiScanPhase == WifiScanPhase::PREPARING) {
    if ((uint32_t)(nowMs - wifiScanStepMs) < WIFI_SCAN_SETTLE_MS) return;
    WiFi.scanDelete();
    ++wifiScanAttempts;
    const int startResult = WiFi.scanNetworks(true, false, false, 120);
    if (startResult == WIFI_SCAN_FAILED) {
      Serial.printf("[WIFI] Driver rejected scan start (attempt %u/3)\n",
                    (unsigned int)wifiScanAttempts);
      if (wifiScanAttempts >= 3) finishWifiScan(-1);
      else wifiScanStepMs = nowMs; // Retry after STA event has settled.
      return;
    }
    wifiScanPhase = WifiScanPhase::RUNNING;
    wifiScanStepMs = nowMs;
    Serial.printf("[WIFI] Scanning asynchronously (attempt %u/3)...\n",
                  (unsigned int)wifiScanAttempts);
    return;
  }
  const int result = WiFi.scanComplete();
  if (result == WIFI_SCAN_FAILED) {
    Serial.println("[WIFI] Driver reported WIFI_SCAN_FAILED (-2)");
    finishWifiScan(-1);
    return;
  }
  if (result >= 0) {
    finishWifiScan(result);
    return;
  }
  if ((uint32_t)(nowMs - wifiScanStepMs) >= WIFI_SCAN_TIMEOUT_MS) {
    Serial.println("[WIFI] Driver scan timeout after 15 seconds; stopping scan");
    esp_wifi_scan_stop();
    finishWifiScan(-1);
  }
}

void showHelp() {
  Serial.println("--- MEO DESK 1.7.0 - LIVING MOTION ENGINE ---");
  Serial.println("g HOME | n next | p previous | 0..9 page | h Health | t Trends | a auto");
  Serial.println("m cat motion on/off | k FPS 20/25/30/AUTO | f focus play/pause");
  Serial.println("r Wi-Fi scan | c color test toggle | b performance | ? help");
  Serial.printf("[WEB] http://%s  /  http://meodesk.local (if mDNS works)\n",
                pcNetworkIP().c_str());
  Serial.printf("SPI: 40MHz | DMA: %s | card sprite: %s | free heap: %lu KB\n",
                catDmaReady ? "on" : "fallback",
                cardBuffered ? "on" : "fallback",
                (unsigned long)ESP.getFreeHeap() / 1024UL);
  Serial.printf("[HARDWARE] Flash: %lu MB | PSRAM: %lu KB | free heap %lu KB\n",
                (unsigned long)ESP.getFlashChipSize() / 1048576UL,
                (unsigned long)ESP.getPsramSize() / 1024UL,
                (unsigned long)ESP.getFreeHeap() / 1024UL);
  Serial.printf("[MEM] PSRAM %s | free PSRAM %lu KB | internal 8-bit %lu KB | DMA internal %lu KB\n",
                psramAvailable ? "READY" : "UNAVAILABLE",
                (unsigned long)ESP.getFreePsram() / 1024UL,
                (unsigned long)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT) / 1024UL,
                (unsigned long)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA) / 1024UL);
}

void handleSerial() {
  while (Serial.available()) {
    const char command = (char)Serial.read();
    if (command == 'c') {
      tft.waitDMA();
      colorTestActive = !colorTestActive;
      if (colorTestActive) drawColorTest();
      else setPage(currentPage);
      continue;
    }
    if (colorTestActive) continue;

    if (command == 'n') {
      autoPages = false;
      setPage((Page)((currentPage + 1) % PAGE_COUNT));
    } else if (command == 'p') {
      autoPages = false;
      setPage((Page)((currentPage + PAGE_COUNT - 1) % PAGE_COUNT));
    } else if (command == 'a') {
      autoPages = !autoPages;
      setPage(currentPage);
    } else if (command == 'm') {
      catMotion = !catMotion;
      Serial.printf("Cat animation: %s\n", catMotion ? "ON" : "OFF");
      if (!catMotion) drawAnimatedCat(millis());
      catPacer.rebase(micros());
    } else if (command == 'k') {
      applyFpsProfile((fpsProfile + 1) % 4);
      pcBridgeSaveFpsProfile(fpsProfile);
      Serial.printf("[PERF] Target cat FPS: %u\n", (unsigned)catPacer.fps());
    } else if (command == 'f') {
      focusTimer.toggle(millis());
      pcBridgeSetFocus(focusTimer.running() && focusTimer.phase() == FocusTimer::WORK);
      if (currentPage == FOCUS_PAGE) drawCard();
    } else if (command == 'g') {
      autoPages = false;
      setPage(HOME_PAGE);
    } else if (command == 'r') {
      startWifiScan();
    } else if (command == 'h') {
      autoPages = false;
      setPage(HEALTH_PAGE);
    } else if (command == 't') {
      autoPages = false;
      setPage(TRENDS_PAGE);
    } else if (command >= '0' && command <= '9') {
      autoPages = false;
      setPage((Page)(command - '0'));
      if (command == '3' && !wifiConnected() && nearbyNetworks == -1)
        startWifiScan(); // Auto-scan on first visit; no need to press 'r'.
    } else if (command == 'b' || command == '?') {
      showHelp();
    }
  }
}

void drawUploadDisplay(int percent, const char *heading) {
  if (percent == 0) {
    tft.waitDMA();
    otaScreenActive = true;
    surface = &tft;
    surfaceOriginX = surfaceOriginY = 0;
    tft.fillScreen(BG);
    label("mimi. / UPDATE", 21, 35, CORAL, 2);
    label(heading, 21, 86, INK, 2);
    label("Keep the device powered", 21, 126, MUTED);
    label("Your cat will return soon.", 21, 145, MUTED);
    tft.drawRoundRect(19, 177, 278, 16, 8, CORAL);
    Serial.println("[OTA] Animations paused; keep power stable");
  } else if (percent < 0) {
    otaScreenActive = false;
    setPage(currentPage);
  }
  if (percent >= 0 && otaScreenActive) {
    const int filled = 274 * constrain(percent, 0, 100) / 100;
    if (filled) tft.fillRect(21, 179, filled, 12, CORAL);
    tft.fillRect(110, 202, 90, 18, BG);
    char value[12];
    snprintf(value, sizeof(value), "%d%%", percent);
    label(value, 146, 204, INK, 1);
  }
}

void setOtaDisplay(int percent) {drawUploadDisplay(percent, "UPDATING FIRMWARE");}
void setCatPackDisplay(int percent) {drawUploadDisplay(percent, "UPDATING CATS");}

bool webControl(const String &action, int value) {
  if (action == "profile") {
    const DeviceProfile &p = pcBridgeProfile();
    themeChoice = p.theme;
    applyFpsProfile(p.fps);
    motionDirector.setStyle(p.style);
    catMotion = p.motion; autoPages = p.autoPages;
    scenes.configureAll(p.scenes);
    focusTimer.configure(p.workMin, p.breakMin, p.goal);
    pcBridgeSetFocus(focusTimer.running() && focusTimer.phase() == FocusTimer::WORK);
    applyPalette(themeShouldBeNight());
    if (!colorTestActive && !otaScreenActive) setPage(currentPage);
    return true;
  }
  if (action == "perf_trace") {
    if (value != 0 && value != 1) return false;
    performanceLab.setEnabled(value == 1, millis());
    return true;
  }
  if (action == "motion_style") {
    if (value < 0 || value > MotionDirector::PLAYFUL) return false;
    motionDirector.setStyle((uint8_t)value);
    pcBridgeSaveMotionStyle(motionDirector.style());
    return true;
  }
  if (action == "fps") {
    if (value < 0 || value > 3) return false;
    applyFpsProfile((uint8_t)value);
    pcBridgeSaveFpsProfile(fpsProfile);
    return true;
  }
  if (action == "pet") {
    petEngine.pet(millis());
    pcBridgePushEvent("cat_petted", "Your tiny companion is happy!", 0, 4500);
    return true;
  }
  if (action == "refresh_music") {
    if (currentPage == MUSIC_PAGE && !colorTestActive) drawCard();
    return true;
  }
  if (action.startsWith("scene_") && action.length() == 7 &&
      action[6] >= '0' && action[6] <= '3') {
    const uint8_t slot = (uint8_t)(action[6] - '0');
    if (value < 0 || value >= PAGE_COUNT ||
        !scenes.configure(slot, (uint8_t)value)) return false;
    pcBridgeSaveSceneSlot(slot, (uint8_t)value);
    if (!colorTestActive) setPage(currentPage);
    return true;
  }
  if (action == "refresh") {
    if (!colorTestActive) setPage(currentPage);
    return true;
  }
  if (action == "page") {
    if (value < 0 || value >= PAGE_COUNT) return false;
    colorTestActive = false;
    autoPages = false;
    setPage((Page)value);
    return true;
  }
  if (action == "next") {
    colorTestActive = false;
    autoPages = false;
    setPage((Page)((currentPage + 1) % PAGE_COUNT));
    return true;
  }
  if (action == "scan") {
    if (wifiScanPhase == WifiScanPhase::IDLE) startWifiScan();
    return true;
  }
  if (action == "motion") {
    catMotion = !catMotion;
    if (!catMotion && !colorTestActive) drawAnimatedCat(millis());
    // A paused animation must not accumulate stale scheduler deadlines.
    catPacer.rebase(micros());
    return true;
  }
  if (action == "auto") {
    autoPages = !autoPages;
    pageStartedMs = millis();
    return true;
  }
  if (action == "theme") {
    if (value < 0 || value > 5) return false;
    themeChoice = (uint8_t)value;
    pcBridgeSaveTheme(themeChoice);
    applyPalette(themeShouldBeNight());
    if(themeChoice==5) currentPage=CLOCK_PAGE;
    if (!colorTestActive) setPage(currentPage);
    return true;
  }
  if (action == "focus") {
    focusTimer.toggle(millis());
    pcBridgeSetFocus(focusTimer.running() && focusTimer.phase() == FocusTimer::WORK);
    if (currentPage == FOCUS_PAGE && !colorTestActive) drawCard();
    return true;
  }
  if (action == "focus_reset") {
    focusTimer.reset();
    pcBridgeSetFocus(false);
    if (currentPage == FOCUS_PAGE && !colorTestActive) drawCard();
    return true;
  }
  if (action == "focus_skip") {
    focusTimer.skip(millis());
    pcBridgeSetFocus(focusTimer.running() && focusTimer.phase() == FocusTimer::WORK);
    if (currentPage == FOCUS_PAGE && !colorTestActive) drawCard();
    return true;
  }
  return false;
}

void webUiStatus(JsonDocument &doc) {
  doc["lab_enabled"] = performanceLab.enabled();
  doc["lab_valid"] = performanceLab.valid();
  doc["lab_payload_coverage"] = "cat_dma+buffered_card+album_only";
  const auto &lab = performanceLab.last();
  doc["lab_window_ms"] = lab.periodMs;
  const char *names[] = {"lab_strip_cpu_ms", "lab_dma_submit_ms", "lab_dma_wait_ms",
                        "lab_cat_wall_ms", "lab_card_wall_ms", "lab_bridge_wall_ms"};
  for (uint8_t stage = 0; stage < PerformanceLab::STAGES; ++stage) {
    if (performanceLab.valid() && lab.calls[stage])
      doc[names[stage]] = performanceLab.meanMs(static_cast<PerformanceLab::Stage>(stage));
    else doc[names[stage]] = nullptr;
  }
  if (performanceLab.valid()) {
    doc["lab_payload_bytes_s"] = performanceLab.payloadBytesPerSecond();
    doc["lab_cat_frames"] = lab.calls[PerformanceLab::CAT_WALL];
    if (lab.calls[PerformanceLab::CAT_WALL])
      doc["lab_cpu_frame_ms"] = static_cast<double>(lab.totalUs[PerformanceLab::CAT_CPU]) /
                                  lab.calls[PerformanceLab::CAT_WALL] / 1000.0;
    else doc["lab_cpu_frame_ms"] = nullptr;
    doc["lab_card_max_ms"] = lab.maxUs[PerformanceLab::CARD_WALL] / 1000.0;
    doc["lab_bridge_max_ms"] = lab.maxUs[PerformanceLab::NETWORK_WALL] / 1000.0;
  } else {
    doc["lab_payload_bytes_s"] = nullptr;
    doc["lab_cat_frames"] = nullptr;
    doc["lab_card_max_ms"] = nullptr;
    doc["lab_bridge_max_ms"] = nullptr;
    doc["lab_cpu_frame_ms"] = nullptr;
  }
  doc["page"] = (int)currentPage;
  doc["page_count"] = (int)PAGE_COUNT;
  JsonArray slots = doc["scene_slots"].to<JsonArray>();
  for (uint8_t i = 0; i < SceneManager::SLOTS; ++i) slots.add(scenes.slot(i));
  doc["cat_persona"] = petEngine.name();
  doc["cat_pack_loaded"] = catPack.loaded();
  doc["cat_pack_generation"] = catPack.generation();
  doc["cat_motion_frames"] = catPack.framesPerPose();
  doc["motion_style"] = motionDirector.style();
  doc["motion_state"] = MotionDirector::modeName(lastMotionCue.mode);
  doc["motion_cel"] = lastMotionCue.cel;
  doc["motion_blink"] = lastMotionCue.blink;
  doc["motion_budget_limited"] = motionDirector.budgetLimited();
  doc["motion_pc_intensity"] = lastMotionCue.pcIntensity;
  doc["cat_energy"] = petEngine.energy();
  doc["page_name"] = pageName(currentPage);
  doc["motion"] = catMotion;
  doc["auto_pages"] = autoPages;
  doc["theme"] = themeChoice;
  doc["night_theme"] = nightPalette;
  doc["focus_running"] = focusTimer.running();
  doc["focus_phase"] = focusTimer.phase() == FocusTimer::WORK ? "work" : "break";
  doc["focus_remaining_s"] = focusTimer.remainingSeconds(millis());
  doc["focus_sessions"] = focusTimer.sessionsDone();
  doc["focus_work_min"] = focusTimer.workMinutes();
  doc["focus_break_min"] = focusTimer.breakMinutes();
  doc["focus_goal"] = focusTimer.goal();
  doc["focus_duration_s"] = focusTimer.durationMs() / 1000UL;
  doc["cat_fps"] = measuredCatFps;
  doc["fps_profile"] = fpsProfile;
  doc["fps_target"] = catPacer.fps();
  doc["fps_auto"] = fpsProfile == FrameGovernor::AUTO;
  doc["cat_render_max_ms"] = catRenderWorstLastUs / 1000.0f;
  doc["cat_frames_skipped"] = catFramesMissedLast;
  doc["internal_ram_kb"] = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT) / 1024UL;
  doc["internal_dma_kb"] = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA) / 1024UL;
  doc["psram_free_kb"] = ESP.getFreePsram() / 1024UL;
  doc["psram_ready"] = psramAvailable;
  doc["dma"] = catDmaReady;
  doc["card_buffered"] = cardBuffered;
  doc["wifi_network_count"] = nearbyNetworks;
  doc["wifi_strongest_ssid"] = strongestSSID;
}

void webUiPerformance(JsonDocument &doc) {
  doc["schema"] = 1;
  doc["sample_period_s"] = STATS_INTERVAL_MS / 1000;
  doc["count"] = perfHistory.count();
  JsonArray actual = doc["fps"].to<JsonArray>();
  JsonArray target = doc["target"].to<JsonArray>();
  JsonArray skipped = doc["skipped"].to<JsonArray>();
  JsonArray worst = doc["worst_ms"].to<JsonArray>();
  JsonArray internal = doc["internal_kb"].to<JsonArray>();
  JsonArray psram = doc["psram_kb"].to<JsonArray>();
  for (uint8_t i = 0; i < perfHistory.count(); ++i) {
    PerformancePoint point{};
    if (!perfHistory.at(i, point)) continue;
    actual.add(point.fpsTenths / 10.0f);
    target.add(point.target);
    skipped.add(point.skipped);
    worst.add(point.worstRenderMs);
    internal.add(point.internalKb);
    psram.add(point.psramKb);
  }
}

void setup() {
  Serial.begin(115200);
  delay(250);

  psramAvailable = psramFound() && ESP.getPsramSize() >= 4UL * 1024 * 1024;
  if (!psramAvailable) {
    Serial.println("[MEM] No usable PSRAM; internal DMA renderer remains available.");
  }
  tft.init();
  tft.setColorDepth(16);
  tft.setRotation(1); // landscape: 320 x 240
  tft.invertDisplay(false);

  // Reserve the *time-critical* DMA-capable strips before any optional card
  // sprite can fragment or consume internal heap.
  const size_t stripBytes = CAT_W * CAT_STRIP_H * sizeof(uint16_t);
  catDma[0] = (uint16_t *)heap_caps_malloc(stripBytes,
                                          MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
  catDma[1] = (uint16_t *)heap_caps_malloc(stripBytes,
                                          MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
  catDmaReady = catDma[0] && catDma[1];
  if (!catDmaReady) {
    if (catDma[0]) heap_caps_free(catDma[0]);
    if (catDma[1]) heap_caps_free(catDma[1]);
    catDma[0] = catDma[1] = nullptr;
  }

  // The changing card is rendered off-screen; the static frame is untouched.
  // Fast path uses internal SRAM for the frequently changing 162x120 card.
  // If internal allocation fails, PSRAM keeps the card usable without
  // stealing the small DMA strip buffers from the critical internal pool.
  cardSprite.setPsram(false);
  cardSprite.setColorDepth(16);
  cardBuffered = cardSprite.createSprite(CARD_W, CARD_H) != nullptr;
  if (!cardBuffered && psramAvailable) {
    cardSprite.setPsram(true);
    cardBuffered = cardSprite.createSprite(CARD_W, CARD_H) != nullptr;
    Serial.println(cardBuffered ? "[MEM] Card sprite PSRAM fallback." :
                                 "[MEM] Card sprite direct-draw fallback.");
  }

  // Load a committed and checksum-verified cat pack once at boot. The
  // 663 KiB RGB565 atlas lives in PSRAM; DMA strips stay internal RAM.
  catPack.initialize();

  tft.fillScreen(BG);
  label("mimi.", 115, 26, CORAL, 3);
  tft.pushImage(88, 52, CAT_W, CAT_H,
                reinterpret_cast<const lgfx::rgb565_t *>(catPack.lookup(cat_boot)));
  label("Your tiny desk buddy", 104, 208, MUTED);
  delay(450);

  // HTTP/Web UI + Wi-Fi router + fallback AP are managed by pc_bridge.cpp.
  pcBridgeBegin(webControl, webUiStatus, webUiPerformance, setOtaDisplay,
                setCatPackDisplay);
  themeChoice = pcBridgeTheme();
  applyFpsProfile(pcBridgeFpsProfile());
  motionDirector.setStyle(pcBridgeMotionStyle());
  applyPalette(themeShouldBeNight());
  webControl("profile", 0);

  showHelp();
  setPage(HOME_PAGE);
  lastStatsMs = millis();
}

void loop() {
  const bool traceNetwork = performanceLab.enabled();
  const uint32_t networkStarted = traceNetwork ? micros() : 0;
  pcBridgeLoop();
  if (traceNetwork && performanceLab.enabled()) performanceLab.add(PerformanceLab::NETWORK_WALL, micros() - networkStarted);
  if (otaScreenActive || meoOta.busy() || catPack.busy()) {
    delay(2);
    return;
  }
  // Application-controlled confirmation only takes effect when the installed
  // bootloader actually supports OTA rollback. Do not require router access:
  // the offline fallback AP is a valid mode for this desktop device.
  if (millis() > 25000UL) {
    const bool healthy = ESP.getFlashChipSize() >= 16UL * 1024UL * 1024UL &&
                         psramAvailable && catDmaReady &&
                         heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT) > 24UL * 1024UL;
    meoOta.confirmRunningImageAfterSelfTest(healthy);
  }
  handleSerial();
  pollWifiScan();

  const uint32_t now = millis();
  if ((uint32_t)(now - lastThemeCheckMs) >= 60000UL) {
    lastThemeCheckMs = now;
    if (themeChoice == 2 && themeShouldBeNight() != nightPalette) {
      applyPalette(!nightPalette);
      if (!colorTestActive) setPage(currentPage);
    }
  }
  const FocusTimer::Transition focusEvent = focusTimer.tick(now);
  if (focusEvent != FocusTimer::NONE) {
    if (focusEvent == FocusTimer::WORK_COMPLETED) petEngine.celebrate(now);
    pcBridgeSetFocus(focusTimer.running() && focusTimer.phase() == FocusTimer::WORK);
    pcBridgePushEvent(focusEvent == FocusTimer::WORK_COMPLETED ? "focus_completed" : "break_completed",
                      focusEvent == FocusTimer::WORK_COMPLETED ? "Focus complete! Take 5 minutes" :
                                                               "Break complete! Back to focus",
                      1, 12000);
    if (currentPage == FOCUS_PAGE && !colorTestActive) drawCard();
  }
  if (colorTestActive) {
    delay(3);
    return;
  }

  if ((uint32_t)(now - lastPetTickMs) >= 250UL) {
    lastPetTickMs = now;
    tm timeInfo{};
    const bool night = realClock(timeInfo) && (timeInfo.tm_hour >= 23 || timeInfo.tm_hour < 7);
    CatBehavior::Context context{};
    context.sensorMood = pcEmotionName();
    context.pcOnline = pcTelemetryFresh();
    context.playing = pcMedia.playingNow(now);
    context.focusing = focusTimer.running() && focusTimer.phase() == FocusTimer::WORK;
    context.quiet = healthSettings.quiet;
    context.late = night;
    if(pcTelemetryFresh() && !pcTelemetry.demo) {
      context.idleSeconds=pcTelemetry.idleSeconds;
      if(realClock(timeInfo) && timeInfo.tm_hour>=7 && timeInfo.tm_hour<11)
        context.morningDay=static_cast<uint32_t>(timeInfo.tm_year*366+timeInfo.tm_yday+1);
    }
    const bool moodChanged = petEngine.tick(context, now);
    if (moodChanged) {
      if (!catDmaReady) {
        // Static fallback still reflects urgent mood changes without DMA.
        tft.pushImage(CAT_X, CAT_Y + 3, CAT_W, CAT_H,
                      reinterpret_cast<const lgfx::rgb565_t *>(catForPage(currentPage)));
      } else if (!catMotion) {
        drawAnimatedCat(now);
      }
    }
  }

  if (autoPages && now - pageStartedMs >= PAGE_DURATION_MS &&
      !focusTimer.running()) {
    setPage((Page)scenes.nextFeatured((uint8_t)currentPage));
  }
  if (currentPage == NETWORK_PAGE && lastWifiState != wifiConnected()) {
    setPage(currentPage); // refresh online/offline cat once.
  }
  const bool pcLive = pcTelemetryFresh();
  if ((currentPage == PC_PAGE || currentPage == HEALTH_PAGE ||
       currentPage == HOME_PAGE || currentPage == PC_NETWORK_PAGE) &&
      (pcLive != lastPcState || (pcLive && pcTelemetry.demo != lastPcDemo))) {
    lastPcState = pcLive;
    lastPcDemo = pcTelemetry.demo;
    drawHeaderStatus();
    drawCard();
  }

  // Page changes and incoming HTTP controls can repaint the card above;
  // sample a *fresh* clock now so an earlier timestamp never underflows
  // against drawCard()'s newer lastCardMs and causes a duplicate repaint.
  const uint32_t renderNow = millis();
  if (catMotion && catDmaReady && catPacer.due(micros())) {
    const uint32_t renderStarted = micros();
    drawAnimatedCat(renderNow);
    const uint32_t renderTime = micros() - renderStarted;
    if (renderTime > catRenderWorstUs) catRenderWorstUs = renderTime;
    lastCatMs = millis();
  }
  const uint32_t cardNow = millis(); // Cat rendering above can consume 25+ ms in Midnight.
  const bool clockPage = currentPage == CLOCK_PAGE || currentPage == HOME_PAGE || currentPage == FOCUS_PAGE || currentPage == CUSTOM_PAGE;
  const WidgetSpec cardWidget{{CARD_X, CARD_Y, CARD_W, CARD_H}, cardIntervalMs(currentPage), clockPage};
  constexpr RenderSurfaceInfo renderSurface{320, 240, 16, true};
  if (renderSurface.accepts(cardWidget.region) && CardRefresh::due(cardNow, lastCardMs, cardWidget.refreshMs,
                       cardWidget.alignSeconds, catMotion && catDmaReady, catPacer.untilNextUs(micros()))) {
      drawCard();
      if (currentPage == CLOCK_PAGE || currentPage == PC_PAGE || currentPage == HOME_PAGE ||
          currentPage == HEALTH_PAGE)
        drawHeaderStatus();
  }

  if (now - lastStatsMs >= STATS_INTERVAL_MS) {
    performanceLab.finishWindow(millis());
    const float elapsed = (now - lastStatsMs) / 1000.f;
    measuredCatFps = elapsed > 0 ? animatedFrames / elapsed : 0.f;
    catRenderWorstLastUs = catRenderWorstUs;
    catRenderWorstUs = 0;
    catFramesMissedLast = catPacer.takeMissed();
    PerformancePoint perf{};
    const int fpsTenths = (int)lroundf(measuredCatFps * 10.f);
    perf.fpsTenths = (uint16_t)constrain(fpsTenths, 0, 600);
    perf.target = catPacer.fps();
    perf.skipped = (uint8_t)(catFramesMissedLast > 255 ? 255 : catFramesMissedLast);
    perf.worstRenderMs = (uint16_t)min(catRenderWorstLastUs / 1000UL, 65535UL);
    perf.internalKb = (uint16_t)min(heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT) / 1024UL, 65535UL);
    perf.psramKb = (uint16_t)min(ESP.getFreePsram() / 1024UL, 65535UL);
    perfHistory.add(perf);
    if (catMotion && catDmaReady)
      motionDirector.observeBudget(measuredCatFps, catPacer.fps(),
                                   catFramesMissedLast, catRenderWorstLastUs);
    if (catMotion && catDmaReady &&
        fpsGovernor.observe(measuredCatFps, catFramesMissedLast, catRenderWorstLastUs)) {
      catPacer.setFps(fpsGovernor.target());
      catPacer.rebase(micros());
      Serial.printf("[PERF] AUTO adjusted to %u FPS target\n", (unsigned)catPacer.fps());
    }
    Serial.printf("[PERF] cat %.1f/%u fps | render max %.1f ms | skipped %lu | int %lu KB | psram %lu KB | PC %s | DMA %s\n",
                  measuredCatFps,
                  (unsigned)catPacer.fps(),
                  catRenderWorstLastUs / 1000.0f,
                  (unsigned long)catFramesMissedLast,
                  (unsigned long)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT) / 1024UL,
                  (unsigned long)ESP.getFreePsram() / 1024UL,
                  pcLive ? (pcTelemetry.demo ? "DEMO" : "LIVE") : "WAIT",
                  catDmaReady ? "yes" : "no");
    animatedFrames = 0;
    lastStatsMs = now;
  }
  delay(1);
}
