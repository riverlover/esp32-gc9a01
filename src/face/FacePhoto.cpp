#include "FacePhoto.h"

#include "assets/bg_portrait_240.h"
#include "gfx_util.h"
#include "prefs/WatchPrefs.h"

using namespace watch;

static void drawShadowedCentered(Arduino_GFX *gfx, const char *text, int16_t y, uint16_t color,
                                 uint8_t textSize) {
  // Soft shadow for contrast over the photo.
  drawCenteredText(gfx, text, y + 1, 0x0000, textSize);
  drawCenteredText(gfx, text, y, color, textSize);
}

void FacePhoto::render(Arduino_GFX *gfx, const struct tm &t) {
  gfx->draw16bitRGBBitmap(0, 0, BG_PORTRAIT_240, 240, 240);

  // Slim bezel so the round panel edge reads as a watch.
  gfx->drawCircle(CX, CY, FACE_R, 0xFFFF);
  gfx->drawCircle(CX, CY, FACE_R - 1, 0xC618);

  char timeBuf[8];
  snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", t.tm_hour, t.tm_min);
  drawShadowedCentered(gfx, timeBuf, 22, 0xFFFF, 3);

  char subBuf[16];
  static const char *kWday[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  const char *wd = (t.tm_wday >= 0 && t.tm_wday < 7) ? kWday[t.tm_wday] : "---";
  snprintf(subBuf, sizeof(subBuf), "%s  %02d", wd, t.tm_mday > 0 ? t.tm_mday : 1);
  drawShadowedCentered(gfx, subBuf, 52, 0xDEFB, 1);

  if (WatchPrefs::showSeconds()) {
    const float secDeg = t.tm_sec * 6.0f;
    drawTick(gfx, secDeg, 114, 104, 0xF800);
  }
}
