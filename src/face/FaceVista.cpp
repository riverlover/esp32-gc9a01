#include "FaceVista.h"

#include <string.h>

#include "gfx_util.h"
#include "prefs/WatchPrefs.h"
#include "time/Lunar.h"
#include "weather/WeatherService.h"

using namespace watch;

static const char *kWdayCn[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

// Default GFX font is thin; +1px horizontal strike ≈ medium weight without size-2 blobs.
static void printBold(Arduino_GFX *gfx, int16_t x, int16_t y, const char *s) {
  gfx->setCursor(x, y);
  gfx->print(s);
  gfx->setCursor(x + 1, y);
  gfx->print(s);
}

static void printBoldCentered(Arduino_GFX *gfx, int16_t cx, int16_t y, const char *s,
                              uint8_t size) {
  gfx->setTextSize(size);
  const int16_t tw = (int16_t)(strlen(s) * 6 * size);
  printBold(gfx, cx - tw / 2, y, s);
}

static void drawHourNum(Arduino_GFX *gfx, int hour, int16_t radius, uint16_t color) {
  const float deg = hour * 30.0f;
  int16_t x, y;
  polar(radius, deg, x, y);
  char buf[3];
  snprintf(buf, sizeof(buf), "%d", hour == 0 ? 12 : hour);
  gfx->setTextColor(color);
  gfx->setTextSize(1);
  const int16_t tw = (int16_t)(strlen(buf) * 6);
  gfx->setCursor(x - tw / 2, y - 4);
  gfx->print(buf);
}

static void drawWxIcon(Arduino_GFX *gfx, int16_t cx, int16_t cy, int code) {
  const uint16_t sun = 0xFE60;
  const uint16_t cloud = 0x8410;
  const uint16_t rain = 0x3B7F;
  const uint16_t snow = 0x7BEF;
  const uint16_t storm = 0xFEA0;

  if (code == 0) {
    gfx->fillCircle(cx, cy, 5, sun);
    for (int i = 0; i < 8; ++i) {
      const float rad = i * 45.0f * DEG_TO_RAD;
      const int16_t x0 = cx + (int16_t)lroundf(7.0f * sinf(rad));
      const int16_t y0 = cy - (int16_t)lroundf(7.0f * cosf(rad));
      const int16_t x1 = cx + (int16_t)lroundf(10.0f * sinf(rad));
      const int16_t y1 = cy - (int16_t)lroundf(10.0f * cosf(rad));
      gfx->drawLine(x0, y0, x1, y1, sun);
    }
    return;
  }

  if (code <= 3) {
    if (code == 1) {
      gfx->fillCircle(cx - 5, cy - 2, 4, sun);
    }
    gfx->fillCircle(cx - 3, cy + 1, 4, cloud);
    gfx->fillCircle(cx + 3, cy, 5, cloud);
    gfx->fillCircle(cx + 7, cy + 2, 3, cloud);
    gfx->fillRect(cx - 5, cy + 1, 13, 4, cloud);
    return;
  }

  gfx->fillCircle(cx - 3, cy - 1, 4, cloud);
  gfx->fillCircle(cx + 3, cy - 2, 5, cloud);
  gfx->fillCircle(cx + 7, cy, 3, cloud);
  gfx->fillRect(cx - 5, cy - 1, 13, 4, cloud);

  if ((code >= 71 && code <= 77) || (code >= 85 && code <= 86)) {
    gfx->fillCircle(cx - 3, cy + 7, 1, snow);
    gfx->fillCircle(cx + 2, cy + 9, 1, snow);
    gfx->fillCircle(cx + 6, cy + 7, 1, snow);
    return;
  }
  if (code >= 95) {
    gfx->drawLine(cx + 1, cy + 4, cx - 2, cy + 9, storm);
    gfx->drawLine(cx - 2, cy + 9, cx + 2, cy + 9, storm);
    gfx->drawLine(cx + 2, cy + 9, cx - 1, cy + 14, storm);
    return;
  }
  if (code == 45 || code == 48) {
    gfx->drawLine(cx - 8, cy + 6, cx + 8, cy + 6, cloud);
    gfx->drawLine(cx - 8, cy + 9, cx + 8, cy + 9, cloud);
    return;
  }
  gfx->drawLine(cx - 3, cy + 6, cx - 4, cy + 11, rain);
  gfx->drawLine(cx + 2, cy + 7, cx + 1, cy + 12, rain);
  gfx->drawLine(cx + 6, cy + 6, cx + 5, cy + 11, rain);
}

void FaceVista::render(Arduino_GFX *gfx, const struct tm &t) {
  gfx->fillScreen(WHITE);

  gfx->drawCircle(CX, CY, FACE_R, 0x4208);
  gfx->drawCircle(CX, CY, FACE_R - 1, 0x8410);
  for (int i = 0; i < 60; ++i) {
    const float deg = i * 6.0f;
    if (i % 5 == 0) {
      drawTick(gfx, deg, 112, 102, BLACK);
    } else {
      drawTick(gfx, deg, 112, 107, 0x8410);
    }
  }

  gfx->fillTriangle(CX, 8, CX - 4, 16, CX + 4, 16, BLACK);

  for (int h = 0; h < 12; ++h) {
    drawHourNum(gfx, h, 94, BLACK);
  }

  // Complications: size 1 + bold. Bottom time size 2.
  constexpr uint8_t kInfo = 1;
  constexpr uint8_t kTime = 2;
  constexpr int16_t kLeftCx = 58;
  constexpr int16_t kRightCx = 182;
  constexpr int16_t kMidY0 = 98;
  constexpr int16_t kMidY1 = 110;
  // Temps closer to center (was 62 / 178).
  constexpr int16_t kTempL = 88;
  constexpr int16_t kTempR = 152;

  const WeatherService::Snapshot wx = WeatherService::get();
  gfx->setTextColor(BLACK);
  if (wx.valid) {
    char tempBuf[10];
    snprintf(tempBuf, sizeof(tempBuf), "%.0fC", (double)wx.tempC);
    printBoldCentered(gfx, kTempL, 46, tempBuf, kInfo);

    drawWxIcon(gfx, CX, 48, wx.weatherCode);

    const char *wxTxt = WeatherService::conditionLabel(wx.weatherCode);
    if (strcmp(wxTxt, "Cloudy") == 0) {
      wxTxt = "Cloud";
    } else if (strcmp(wxTxt, "Showers") == 0) {
      wxTxt = "Shower";
    }
    if (wxTxt[0] != '\0' && strcmp(wxTxt, "-") != 0) {
      printBoldCentered(gfx, CX, 64, wxTxt, kInfo);
    }

    if (wx.hasRange) {
      char rangeBuf[12];
      snprintf(rangeBuf, sizeof(rangeBuf), "%.0f/%.0f", (double)wx.tempMinC,
               (double)wx.tempMaxC);
      printBoldCentered(gfx, kTempR, 46, rangeBuf, kInfo);
    }
  } else {
    printBoldCentered(gfx, CX, 50, "Wx...", kInfo);
  }

  Lunar::Date lunar{};
  gfx->setTextColor(BLACK);
  if (Lunar::fromGregorian(t, lunar)) {
    char lunarBuf[12];
    Lunar::formatShort(lunar, lunarBuf, sizeof(lunarBuf));
    printBoldCentered(gfx, kLeftCx, kMidY0, "LUNAR", kInfo);
    printBoldCentered(gfx, kLeftCx, kMidY1, lunarBuf, kInfo);
  } else {
    printBoldCentered(gfx, kLeftCx, kMidY0 + 6, "--", kInfo);
  }

  const int wday = (t.tm_wday >= 0 && t.tm_wday < 7) ? t.tm_wday : 0;
  char dateBuf[10];
  snprintf(dateBuf, sizeof(dateBuf), "%d-%d", t.tm_mon + 1, t.tm_mday);
  printBoldCentered(gfx, kRightCx, kMidY0, kWdayCn[wday], kInfo);
  printBoldCentered(gfx, kRightCx, kMidY1, dateBuf, kInfo);

  char timeBuf[12];
  if (WatchPrefs::showSeconds()) {
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
  } else {
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", t.tm_hour, t.tm_min);
  }
  gfx->setTextColor(BLACK);
  // Size 2 once (no double-strike) — larger than mid info, not a solid block.
  gfx->setTextSize(kTime);
  const int16_t timeTw = (int16_t)(strlen(timeBuf) * 6 * kTime);
  gfx->setCursor(CX - timeTw / 2, 176);
  gfx->print(timeBuf);

  float hourDeg, minDeg, secDeg;
  handAngles(t, hourDeg, minDeg, secDeg);
  drawTaperedHand(gfx, hourDeg, 46, 4, BLACK, 10);
  drawTaperedHand(gfx, minDeg, 66, 3, BLACK, 12);
  if (WatchPrefs::showSeconds()) {
    drawSecondHand(gfx, secDeg, 78, 0xF800, 14);
  }

  gfx->fillCircle(CX, CY, 5, BLACK);
  gfx->fillCircle(CX, CY, 2, 0xF800);
}
