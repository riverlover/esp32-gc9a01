#include "FaceClassic.h"

#include "gfx_util.h"

using namespace watch;

void FaceClassic::render(Arduino_GFX *gfx, const struct tm &t) {
  gfx->fillScreen(BLACK);

  gfx->drawCircle(CX, CY, FACE_R, 0x6B4D);
  gfx->drawCircle(CX, CY, FACE_R - 1, 0x8430);
  gfx->drawCircle(CX, CY, FACE_R - 2, 0x4A49);

  for (int i = 0; i < 60; i++) {
    const float deg = i * 6.0f;
    if (i % 5 == 0) {
      drawTick(gfx, deg, 112, 96, 0xEF5D);
    } else {
      drawTick(gfx, deg, 112, 104, 0x8410);
    }
  }

  for (int h = 0; h < 12; h++) {
    drawCardinalNumber(gfx, h, 88, 0xEF5D, 2);
  }

  drawCenteredText(gfx, "QUARTZ", CY - 42, 0x8410);
  drawCenteredText(gfx, "ESP32-C3", CY + 34, 0x8410);

  float hourDeg, minDeg, secDeg;
  handAngles(t, hourDeg, minDeg, secDeg);
  drawTaperedHand(gfx, hourDeg, 58, 5, 0xEF5D);
  drawTaperedHand(gfx, minDeg, 82, 4, 0xFFFF);
  drawSecondHand(gfx, secDeg, 92, 0xF800);

  gfx->fillCircle(CX, CY, 6, 0xEF5D);
  gfx->fillCircle(CX, CY, 3, 0xF800);
}
