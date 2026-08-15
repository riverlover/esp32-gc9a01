#pragma once

#include "IWatchFace.h"

class FaceClassic : public IWatchFace {
 public:
  const char *name() const override { return "Classic"; }
  void render(Arduino_GFX *gfx, const struct tm &t) override;
};
