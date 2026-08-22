#include "FaceSkeleton.h"

#include "gfx_util.h"
#include "prefs/WatchPrefs.h"

using namespace watch;

static const uint16_t SK_STEEL = 0xC618;
static const uint16_t SK_EDGE = 0x8410;
static const uint16_t SK_ACCENT = 0xFD20;  // amber gear accent
static const uint16_t SK_BG = 0x10A2;

void FaceSkeleton::render(Arduino_GFX *gfx, const struct tm &t) {
  gfx->fillScreen(SK_BG);

  // Nested open rings
  for (int r = 30; r <= FACE_R; r += 14) {
    gfx->drawCircle(CX, CY, r, SK_EDGE);
  }
  gfx->drawCircle(CX, CY, FACE_R, SK_STEEL);
  gfx->drawCircle(CX, CY, FACE_R - 1, SK_ACCENT);

  // "Gear" teeth on mid ring
  for (int i = 0; i < 24; i++) {
    const float deg = i * 15.0f;
    drawTick(gfx, deg, 72, 62, SK_ACCENT);
  }

  // Bridges / decorative bars
  gfx->drawLine(CX - 50, CY - 20, CX + 50, CY - 20, SK_EDGE);
  gfx->drawLine(CX - 40, CY + 28, CX + 40, CY + 28, SK_EDGE);
  gfx->drawCircle(CX - 28, CY + 8, 10, SK_STEEL);
  gfx->drawCircle(CX + 28, CY + 8, 14, SK_STEEL);
  gfx->drawCircle(CX + 28, CY + 8, 8, SK_EDGE);

  for (int i = 0; i < 60; i++) {
    const float deg = i * 6.0f;
    if (i % 5 == 0) {
      drawTick(gfx, deg, 112, 98, SK_STEEL);
    } else {
      drawTick(gfx, deg, 112, 106, SK_EDGE);
    }
  }

  for (int h = 0; h < 12; h++) {
    drawCardinalNumber(gfx, h, 86, SK_STEEL, 2);
  }

  drawCenteredText(gfx, "SKELETON", CY - 48, SK_ACCENT);

  float hourDeg, minDeg, secDeg;
  handAngles(t, hourDeg, minDeg, secDeg);
  drawHollowHand(gfx, hourDeg, 55, 6, SK_STEEL);
  drawHollowHand(gfx, minDeg, 78, 5, 0xFFFF);
  if (WatchPrefs::showSeconds()) {
    drawSecondHand(gfx, secDeg, 94, SK_ACCENT, 20);
  }

  gfx->drawCircle(CX, CY, 8, SK_STEEL);
  gfx->fillCircle(CX, CY, 3, SK_ACCENT);
}
