#pragma once

#include "IWatchFace.h"

class FacePhoto : public IWatchFace {
 public:
  const char *name() const override { return "Photo"; }
  void render(Arduino_GFX *gfx, const struct tm &t) override;
};
