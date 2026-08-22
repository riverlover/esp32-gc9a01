#pragma once

#include <Arduino_GFX_Library.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "prefs/WatchPrefs.h"

namespace watch {

static constexpr int16_t CX = 120;
static constexpr int16_t CY = 120;
static constexpr int16_t FACE_R = 118;

inline void polar(int16_t r, float deg, int16_t &x, int16_t &y) {
  const float rad = deg * DEG_TO_RAD;
  x = CX + (int16_t)lroundf(r * sinf(rad));
  y = CY - (int16_t)lroundf(r * cosf(rad));
}

inline void handAngles(const struct tm &t, float &hourDeg, float &minDeg, float &secDeg) {
  const int sec = WatchPrefs::showSeconds() ? t.tm_sec : 0;
  secDeg = sec * 6.0f;
  minDeg = t.tm_min * 6.0f + sec * 0.1f;
  hourDeg = (t.tm_hour % 12) * 30.0f + t.tm_min * 0.5f;
}

inline void drawTick(Arduino_GFX *gfx, float deg, int16_t rOuter, int16_t rInner, uint16_t color) {
  int16_t x0, y0, x1, y1;
  polar(rOuter, deg, x0, y0);
  polar(rInner, deg, x1, y1);
  gfx->drawLine(x0, y0, x1, y1, color);
}

inline void drawTaperedHand(Arduino_GFX *gfx, float deg, int16_t length, int16_t halfWidth,
                            uint16_t color, int16_t overhang = 12) {
  const float rad = deg * DEG_TO_RAD;
  const float px = sinf(rad);
  const float py = -cosf(rad);
  const float qx = cosf(rad);
  const float qy = sinf(rad);

  const int16_t tipX = CX + (int16_t)lroundf(length * px);
  const int16_t tipY = CY + (int16_t)lroundf(length * py);
  const int16_t baseX = CX - (int16_t)lroundf(overhang * px);
  const int16_t baseY = CY - (int16_t)lroundf(overhang * py);
  const int16_t leftX = baseX + (int16_t)lroundf(halfWidth * qx);
  const int16_t leftY = baseY + (int16_t)lroundf(halfWidth * qy);
  const int16_t rightX = baseX - (int16_t)lroundf(halfWidth * qx);
  const int16_t rightY = baseY - (int16_t)lroundf(halfWidth * qy);

  gfx->fillTriangle(tipX, tipY, leftX, leftY, rightX, rightY, color);
}

inline void drawSecondHand(Arduino_GFX *gfx, float deg, int16_t length, uint16_t color,
                           int16_t tail = 22) {
  int16_t tipX, tipY, tailX, tailY;
  polar(length, deg, tipX, tipY);
  polar(tail, deg + 180.0f, tailX, tailY);
  gfx->drawLine(CX, CY, tipX, tipY, color);
  gfx->drawLine(CX, CY, tipX + 1, tipY, color);
  gfx->drawLine(CX, CY, tipX, tipY + 1, color);
  gfx->drawLine(CX, CY, tailX, tailY, color);
}

inline void drawHollowHand(Arduino_GFX *gfx, float deg, int16_t length, int16_t halfWidth,
                           uint16_t color) {
  const float rad = deg * DEG_TO_RAD;
  const float px = sinf(rad);
  const float py = -cosf(rad);
  const float qx = cosf(rad);
  const float qy = sinf(rad);

  const int16_t tipX = CX + (int16_t)lroundf(length * px);
  const int16_t tipY = CY + (int16_t)lroundf(length * py);
  const int16_t baseX = CX - (int16_t)lroundf(10 * px);
  const int16_t baseY = CY - (int16_t)lroundf(10 * py);
  const int16_t l0x = baseX + (int16_t)lroundf(halfWidth * qx);
  const int16_t l0y = baseY + (int16_t)lroundf(halfWidth * qy);
  const int16_t r0x = baseX - (int16_t)lroundf(halfWidth * qx);
  const int16_t r0y = baseY - (int16_t)lroundf(halfWidth * qy);
  const int16_t l1x = tipX + (int16_t)lroundf((halfWidth / 2) * qx);
  const int16_t l1y = tipY + (int16_t)lroundf((halfWidth / 2) * qy);
  const int16_t r1x = tipX - (int16_t)lroundf((halfWidth / 2) * qx);
  const int16_t r1y = tipY - (int16_t)lroundf((halfWidth / 2) * qy);

  gfx->drawLine(l0x, l0y, l1x, l1y, color);
  gfx->drawLine(r0x, r0y, r1x, r1y, color);
  gfx->drawLine(l0x, l0y, r0x, r0y, color);
  gfx->drawLine(l1x, l1y, r1x, r1y, color);
}

inline void drawCardinalNumber(Arduino_GFX *gfx, int hour, int16_t radius, uint16_t color,
                               uint8_t textSize = 2) {
  if (hour % 3 != 0) {
    return;
  }
  const float deg = hour * 30.0f;
  int16_t x, y;
  polar(radius, deg, x, y);
  char buf[3];
  snprintf(buf, sizeof(buf), "%d", hour == 0 ? 12 : hour);
  gfx->setTextColor(color);
  gfx->setTextSize(textSize);
  const int16_t tw = (int16_t)(strlen(buf) * 6 * textSize);
  gfx->setCursor(x - tw / 2, y - 4 * textSize);
  gfx->print(buf);
}

inline void drawCenteredText(Arduino_GFX *gfx, const char *text, int16_t y, uint16_t color,
                             uint8_t textSize = 1) {
  gfx->setTextColor(color);
  gfx->setTextSize(textSize);
  const int16_t tw = (int16_t)(strlen(text) * 6 * textSize);
  gfx->setCursor(CX - tw / 2, y);
  gfx->print(text);
}

}  // namespace watch
