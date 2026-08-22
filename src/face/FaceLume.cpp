#include "FaceLume.h"

#include "gfx_util.h"
#include "prefs/WatchPrefs.h"

using namespace watch;

// Soft green lume palette (RGB565)
static const uint16_t LUME = 0x47E0;       // bright phosphor
static const uint16_t LUME_DIM = 0x2C40;   // dim glow
static const uint16_t LUME_HAND = 0xAFE5;  // hand fill
static const uint16_t DIAL = 0x0841;       // near-black blue-green

void FaceLume::render(Arduino_GFX *gfx, const struct tm &t) {
  gfx->fillScreen(DIAL);

  gfx->drawCircle(CX, CY, FACE_R, LUME_DIM);
  gfx->drawCircle(CX, CY, FACE_R - 2, 0x01E0);

  // Lume plots at each hour + thin minute track
  for (int i = 0; i < 60; i++) {
    const float deg = i * 6.0f;
    if (i % 5 == 0) {
      int16_t x, y;
      polar(104, deg, x, y);
      gfx->fillCircle(x, y, (i % 15 == 0) ? 4 : 3, LUME);
      if (i % 15 == 0) {
        gfx->fillCircle(x, y, 2, LUME_HAND);
      }
    } else {
      drawTick(gfx, deg, 110, 106, LUME_DIM);
    }
  }

  drawCenteredText(gfx, "LUME", CY - 40, LUME_DIM);
  drawCenteredText(gfx, "NIGHT", CY + 36, LUME_DIM);

  float hourDeg, minDeg, secDeg;
  handAngles(t, hourDeg, minDeg, secDeg);

  // Soft under-glow then bright hand
  drawTaperedHand(gfx, hourDeg, 56, 6, LUME_DIM);
  drawTaperedHand(gfx, hourDeg, 54, 3, LUME_HAND);
  drawTaperedHand(gfx, minDeg, 80, 5, LUME_DIM);
  drawTaperedHand(gfx, minDeg, 78, 2, LUME);
  if (WatchPrefs::showSeconds()) {
    drawSecondHand(gfx, secDeg, 90, 0x07E0, 18);
  }

  gfx->fillCircle(CX, CY, 7, LUME_DIM);
  gfx->fillCircle(CX, CY, 4, LUME);
  gfx->fillCircle(CX, CY, 2, 0xFFFF);
}
