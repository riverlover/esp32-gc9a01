#pragma once

#include "IWatchFace.h"

// White multi-complication dial: analog + weather + digital/lunar date.
// Omits heart / steps / sleep / battery (no sensors on this board).
class FaceVista : public IWatchFace {
 public:
  const char *name() const override { return "Vista"; }
  void render(Arduino_GFX *gfx, const struct tm &t) override;
};
