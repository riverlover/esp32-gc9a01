#include "Settings.h"

#include <WiFi.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "face/FaceId.h"
#include "time/TimeService.h"
#include "wifi/WifiProvision.h"

namespace Settings {
namespace {

constexpr uint32_t kIdleExitMs = 15000;
constexpr uint16_t COL_BG = 0x0000;
constexpr uint16_t COL_FG = 0xFFFF;
constexpr uint16_t COL_DIM = 0x8410;
constexpr uint16_t COL_HL = 0x05FF;  // cyan highlight bar

enum class Screen : uint8_t { Root = 0, Face, Sync, Tz, Wifi, About };

Hooks gHooks{};
bool gOn = false;
Screen gScreen = Screen::Root;
int8_t gIndex = 0;
uint32_t gLastInputMs = 0;
char gStatusLine[48] = "";

constexpr int kRootCount = 6;
const char *kRootLabels[kRootCount] = {"Face", "Sync", "Timezone", "Wi-Fi", "About", "Back"};

void touch() { gLastInputMs = millis(); }

void setStatus(const char *msg) {
  strncpy(gStatusLine, msg, sizeof(gStatusLine) - 1);
  gStatusLine[sizeof(gStatusLine) - 1] = '\0';
}

int8_t clampIndex(int8_t i, int8_t n) {
  if (n <= 0) {
    return 0;
  }
  while (i < 0) {
    i = (int8_t)(i + n);
  }
  while (i >= n) {
    i = (int8_t)(i - n);
  }
  return i;
}

int screenCount() {
  switch (gScreen) {
    case Screen::Root:
      return kRootCount;
    case Screen::Face:
      return (int)FaceId::Count + 1;  // faces + Back
    case Screen::Sync:
      return 2;  // Sync NTP / Back
    case Screen::Tz:
      return 3;  // value / hint / Back
    case Screen::Wifi:
      return 3;  // status / Reconnect / Back
    case Screen::About:
      return 1;  // Back only (info is static)
  }
  return 1;
}

const char *titleFor() {
  switch (gScreen) {
    case Screen::Root:
      return "Settings";
    case Screen::Face:
      return "Face";
    case Screen::Sync:
      return "Sync";
    case Screen::Tz:
      return "Timezone";
    case Screen::Wifi:
      return "Wi-Fi";
    case Screen::About:
      return "About";
  }
  return "Settings";
}

void goRoot() {
  gScreen = Screen::Root;
  gIndex = 0;
  gStatusLine[0] = '\0';
}

void goBack() {
  if (gScreen == Screen::Root) {
    gOn = false;
    return;
  }
  goRoot();
}

void drawCentered(Arduino_GFX *gfx, int16_t y, const char *text, uint16_t color, uint8_t size = 1) {
  gfx->setTextSize(size);
  gfx->setTextColor(color);
  const int16_t tw = (int16_t)(strlen(text) * 6 * size);
  gfx->setCursor((240 - tw) / 2, y);
  gfx->print(text);
}

void drawListItem(Arduino_GFX *gfx, int16_t y, const char *text, bool hl) {
  if (hl) {
    gfx->fillRoundRect(28, y - 3, 184, 16, 3, COL_HL);
    gfx->setTextColor(0x0000);
  } else {
    gfx->setTextColor(COL_FG);
  }
  gfx->setTextSize(1);
  const int16_t tw = (int16_t)(strlen(text) * 6);
  gfx->setCursor((240 - tw) / 2, y);
  gfx->print(text);
}

}  // namespace

void begin(const Hooks &hooks) {
  gHooks = hooks;
  gOn = false;
  goRoot();
}

bool active() { return gOn; }

void open() {
  gOn = true;
  goRoot();
  touch();
  Serial.println("Settings open");
}

void close() {
  gOn = false;
  Serial.println("Settings close");
}

bool handleInput(int8_t rot, bool shortPress, bool longPress) {
  if (!gOn) {
    return false;
  }

  bool dirty = false;

  if (rot != 0) {
    touch();
    if (gScreen == Screen::Tz && gIndex == 0) {
      // First row: adjust TZ hours with rotate.
      TimeService::setTzOffsetHours(TimeService::tzOffsetHours() + (rot > 0 ? 1 : -1));
      dirty = true;
    } else {
      gIndex = clampIndex((int8_t)(gIndex + rot), (int8_t)screenCount());
      dirty = true;
    }
  }

  if (longPress) {
    touch();
    if (gScreen == Screen::Root) {
      close();
    } else {
      goBack();
    }
    return true;
  }

  if (shortPress) {
    touch();
    dirty = true;
    switch (gScreen) {
      case Screen::Root:
        switch (gIndex) {
          case 0:
            gScreen = Screen::Face;
            gIndex = (int8_t)gHooks.getFace();
            break;
          case 1:
            gScreen = Screen::Sync;
            gIndex = 0;
            setStatus("");
            break;
          case 2:
            gScreen = Screen::Tz;
            gIndex = 0;
            break;
          case 3:
            gScreen = Screen::Wifi;
            gIndex = 0;
            setStatus("");
            break;
          case 4:
            gScreen = Screen::About;
            gIndex = 0;
            break;
          default:
            close();
            break;
        }
        break;

      case Screen::Face: {
        const int back = (int)FaceId::Count;
        if (gIndex >= back) {
          goBack();
        } else if (gHooks.setFace) {
          gHooks.setFace(static_cast<FaceId>(gIndex));
          setStatus(faceName(static_cast<FaceId>(gIndex)));
        }
        break;
      }

      case Screen::Sync:
        if (gIndex == 0) {
          setStatus("Syncing...");
          const bool ok = TimeService::syncNtpNow();
          setStatus(ok ? "NTP OK" : "NTP fail");
        } else {
          goBack();
        }
        break;

      case Screen::Tz:
        if (gIndex == 2) {
          goBack();
        }
        // index 0/1: rotate already adjusts; short confirms stay
        break;

      case Screen::Wifi:
        if (gIndex == 1) {
          setStatus("Reconnecting...");
          TimeService::requestReprovision();
          setStatus(WifiProvision::isConnected() ? "Wi-Fi OK" : "No Wi-Fi");
        } else if (gIndex == 2) {
          goBack();
        }
        break;

      case Screen::About:
        goBack();
        break;
    }
  }

  if (gOn && (millis() - gLastInputMs) >= kIdleExitMs) {
    close();
    return true;
  }

  return dirty;
}

void draw(Arduino_GFX *gfx) {
  gfx->fillScreen(COL_BG);
  gfx->drawCircle(120, 120, 118, COL_DIM);

  drawCentered(gfx, 28, titleFor(), COL_FG, 2);

  char line[40];

  switch (gScreen) {
    case Screen::Root:
      for (int i = 0; i < kRootCount; ++i) {
        drawListItem(gfx, (int16_t)(70 + i * 20), kRootLabels[i], i == gIndex);
      }
      break;

    case Screen::Face: {
      const int n = (int)FaceId::Count;
      for (int i = 0; i < n; ++i) {
        drawListItem(gfx, (int16_t)(64 + i * 18), faceName(static_cast<FaceId>(i)), i == gIndex);
      }
      drawListItem(gfx, (int16_t)(64 + n * 18), "Back", gIndex == n);
      if (gStatusLine[0]) {
        drawCentered(gfx, 210, gStatusLine, COL_HL);
      }
      break;
    }

    case Screen::Sync: {
      struct tm t{};
      TimeService::now(t);
      snprintf(line, sizeof(line), "%s  %02d:%02d:%02d", TimeService::sourceName(), t.tm_hour,
               t.tm_min, t.tm_sec);
      drawCentered(gfx, 58, line, COL_DIM);
      drawListItem(gfx, 100, "Sync NTP now", gIndex == 0);
      drawListItem(gfx, 130, "Back", gIndex == 1);
      if (gStatusLine[0]) {
        drawCentered(gfx, 170, gStatusLine, COL_HL);
      }
      break;
    }

    case Screen::Tz: {
      const int h = TimeService::tzOffsetHours();
      snprintf(line, sizeof(line), "UTC%+d", h);
      drawListItem(gfx, 90, line, gIndex == 0);
      drawListItem(gfx, 110, "Turn = adjust", gIndex == 1);
      drawListItem(gfx, 130, "Back", gIndex == 2);
      drawCentered(gfx, 170, "Long = back", COL_DIM);
      break;
    }

    case Screen::Wifi: {
      if (WifiProvision::isConnected()) {
        snprintf(line, sizeof(line), "%s", WiFi.SSID().c_str());
        if (strlen(line) > 18) {
          line[18] = '\0';
        }
        drawCentered(gfx, 56, line, COL_FG);
        snprintf(line, sizeof(line), "RSSI %d", WiFi.RSSI());
        drawCentered(gfx, 72, line, COL_DIM);
      } else {
        drawCentered(gfx, 56, "Disconnected", COL_FG);
        drawCentered(gfx, 72, PHONE_HOTSPOT_SSID, COL_DIM);
      }
      drawListItem(gfx, 100, "Status", gIndex == 0);
      drawListItem(gfx, 120, "Reconnect", gIndex == 1);
      drawListItem(gfx, 140, "Back", gIndex == 2);
      if (gStatusLine[0]) {
        drawCentered(gfx, 175, gStatusLine, COL_HL);
      }
      break;
    }

    case Screen::About: {
      drawCentered(gfx, 60, "GC9A01 Watch", COL_FG);
      drawCentered(gfx, 80, "ESP32-C3", COL_DIM);
      snprintf(line, sizeof(line), "heap %u", (unsigned)ESP.getFreeHeap());
      drawCentered(gfx, 100, line, COL_DIM);
      snprintf(line, sizeof(line), "time %s", TimeService::sourceName());
      drawCentered(gfx, 116, line, COL_DIM);
      if (WifiProvision::isConnected()) {
        snprintf(line, sizeof(line), "%s", WiFi.localIP().toString().c_str());
        drawCentered(gfx, 132, line, COL_DIM);
      }
      drawListItem(gfx, 170, "Back", true);
      break;
    }
  }

  drawCentered(gfx, 222, "long=back", COL_DIM);
}

}  // namespace Settings
