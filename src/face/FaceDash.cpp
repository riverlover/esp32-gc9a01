#include "FaceDash.h"

#include "gfx_util.h"
#include "weather/WeatherService.h"
#include "prefs/WatchPrefs.h"

using namespace watch;

static const char *kMonthAbbr[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                                   "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
static const char *kWdayFull[] = {"SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY",
                                  "THURSDAY", "FRIDAY", "SATURDAY"};

static void drawWeatherIcon(Arduino_GFX *gfx, int16_t cx, int16_t cy, int code) {
  const uint16_t sun = 0xFEA0;
  const uint16_t cloud = 0xC618;
  const uint16_t rain = 0x5D7F;
  const uint16_t snow = 0xFFFF;
  const uint16_t storm = 0xFFE0;

  if (code == 0) {
    gfx->fillCircle(cx, cy, 10, sun);
    for (int i = 0; i < 8; ++i) {
      const float rad = i * 45.0f * DEG_TO_RAD;
      const int16_t x0 = cx + (int16_t)lroundf(14.0f * sinf(rad));
      const int16_t y0 = cy - (int16_t)lroundf(14.0f * cosf(rad));
      const int16_t x1 = cx + (int16_t)lroundf(18.0f * sinf(rad));
      const int16_t y1 = cy - (int16_t)lroundf(18.0f * cosf(rad));
      gfx->drawLine(x0, y0, x1, y1, sun);
    }
    return;
  }

  if (code <= 3) {
    if (code == 1) {
      gfx->fillCircle(cx - 8, cy - 4, 7, sun);
    }
    gfx->fillCircle(cx - 6, cy + 2, 8, cloud);
    gfx->fillCircle(cx + 4, cy, 10, cloud);
    gfx->fillCircle(cx + 12, cy + 3, 7, cloud);
    gfx->fillRect(cx - 10, cy + 2, 24, 8, cloud);
    return;
  }

  if (code == 45 || code == 48) {
    for (int i = 0; i < 4; ++i) {
      const int16_t y = cy - 8 + i * 6;
      gfx->drawLine(cx - 16, y, cx + 16, y, cloud);
    }
    return;
  }

  gfx->fillCircle(cx - 6, cy - 2, 8, cloud);
  gfx->fillCircle(cx + 4, cy - 4, 10, cloud);
  gfx->fillCircle(cx + 12, cy - 1, 7, cloud);
  gfx->fillRect(cx - 10, cy - 2, 24, 8, cloud);

  if ((code >= 71 && code <= 77) || (code >= 85 && code <= 86)) {
    gfx->fillCircle(cx - 6, cy + 12, 2, snow);
    gfx->fillCircle(cx + 2, cy + 16, 2, snow);
    gfx->fillCircle(cx + 10, cy + 12, 2, snow);
    return;
  }

  if (code >= 95) {
    gfx->drawLine(cx + 2, cy + 6, cx - 4, cy + 14, storm);
    gfx->drawLine(cx - 4, cy + 14, cx + 4, cy + 14, storm);
    gfx->drawLine(cx + 4, cy + 14, cx - 2, cy + 22, storm);
    return;
  }

  gfx->drawLine(cx - 6, cy + 10, cx - 8, cy + 18, rain);
  gfx->drawLine(cx + 2, cy + 12, cx, cy + 20, rain);
  gfx->drawLine(cx + 10, cy + 10, cx + 8, cy + 18, rain);
}

void FaceDash::render(Arduino_GFX *gfx, const struct tm &t) {
  gfx->fillScreen(0x10A2);

  gfx->drawCircle(CX, CY, FACE_R, 0x3A4A);
  gfx->drawCircle(CX, CY, FACE_R - 1, 0x2145);

  const int mon = (t.tm_mon >= 0 && t.tm_mon < 12) ? t.tm_mon : 0;
  const int wday = (t.tm_wday >= 0 && t.tm_wday < 7) ? t.tm_wday : 0;
  const int mday = t.tm_mday > 0 ? t.tm_mday : 1;

  drawCenteredText(gfx, kWdayFull[wday], 28, 0x8410, 1);

  char dateBuf[16];
  snprintf(dateBuf, sizeof(dateBuf), "%s %02d", kMonthAbbr[mon], mday);
  drawCenteredText(gfx, dateBuf, 44, 0xEF5D, 2);

  char timeBuf[8];
  snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", t.tm_hour, t.tm_min);
  drawCenteredText(gfx, timeBuf, 78, 0xFFFF, 4);

  if (WatchPrefs::showSeconds()) {
  char secBuf[4];
  snprintf(secBuf, sizeof(secBuf), "%02d", t.tm_sec);
  drawCenteredText(gfx, secBuf, 118, 0xF800, 2);
  }

  const WeatherService::Snapshot wx = WeatherService::get();
  if (wx.valid) {
    drawWeatherIcon(gfx, 78, 168, wx.weatherCode);

    char tempBuf[12];
    snprintf(tempBuf, sizeof(tempBuf), "%.0fC", (double)wx.tempC);
    gfx->setTextColor(0xFFFF);
    gfx->setTextSize(2);
    gfx->setCursor(100, 158);
    gfx->print(tempBuf);

    drawCenteredText(gfx, WeatherService::conditionLabel(wx.weatherCode), 186, 0xC618, 1);

    if (wx.humidity > 0) {
      char rhBuf[12];
      snprintf(rhBuf, sizeof(rhBuf), "RH %d%%", wx.humidity);
      drawCenteredText(gfx, rhBuf, 202, 0x8410, 1);
    }
  } else {
    drawCenteredText(gfx, "Weather...", 168, 0x8410, 1);
    drawCenteredText(gfx, "need Wi-Fi", 186, 0x5ACB, 1);
  }
}
