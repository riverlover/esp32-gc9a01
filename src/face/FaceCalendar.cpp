#include "FaceCalendar.h"

#include "gfx_util.h"
#include "prefs/WatchPrefs.h"

using namespace watch;

void FaceCalendar::render(Arduino_GFX *gfx, const struct tm &t) {
  gfx->fillScreen(BLACK);

  gfx->drawCircle(CX, CY, FACE_R, 0x6B4D);
  gfx->drawCircle(CX, CY, FACE_R - 1, 0x8430);
  gfx->drawCircle(CX, CY, FACE_R - 2, 0x4A49);

  for (int i = 0; i < 60; i++) {
    const float deg = i * 6.0f;
    // Leave gap near 3 o'clock for date window
    if (i >= 12 && i <= 18) {
      continue;
    }
    if (i % 5 == 0) {
      drawTick(gfx, deg, 112, 96, 0xEF5D);
    } else {
      drawTick(gfx, deg, 112, 104, 0x8410);
    }
  }

  // 12 / 6 / 9 only (3 replaced by calendar)
  drawCardinalNumber(gfx, 0, 88, 0xEF5D, 2);
  drawCardinalNumber(gfx, 6, 88, 0xEF5D, 2);
  drawCardinalNumber(gfx, 9, 88, 0xEF5D, 2);

  // Date window at 3 o'clock
  const int16_t wx = CX + 62;
  const int16_t wy = CY - 12;
  const int16_t ww = 28;
  const int16_t wh = 24;
  gfx->fillRoundRect(wx - 2, wy - 2, ww + 4, wh + 4, 3, 0x8410);
  gfx->fillRoundRect(wx, wy, ww, wh, 2, 0xEF5D);
  char dayBuf[4];
  snprintf(dayBuf, sizeof(dayBuf), "%02d", t.tm_mday > 0 ? t.tm_mday : 1);
  gfx->setTextColor(BLACK);
  gfx->setTextSize(2);
  gfx->setCursor(wx + 2, wy + 4);
  gfx->print(dayBuf);

  drawCenteredText(gfx, "AUTO", CY - 42, 0x8410);
  drawCenteredText(gfx, "DATE", CY + 34, 0x8410);

  float hourDeg, minDeg, secDeg;
  handAngles(t, hourDeg, minDeg, secDeg);
  drawTaperedHand(gfx, hourDeg, 52, 5, 0xEF5D);
  drawTaperedHand(gfx, minDeg, 78, 4, 0xFFFF);
  if (WatchPrefs::showSeconds()) {
    drawSecondHand(gfx, secDeg, 88, 0xF800);
  }

  gfx->fillCircle(CX, CY, 6, 0xEF5D);
  gfx->fillCircle(CX, CY, 3, 0xF800);
}
