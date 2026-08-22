#pragma once

#include "IWatchFace.h"

class FaceCrown : public IWatchFace {
 public:
  const char *name() const override { return "Crown"; }
  void render(Arduino_GFX *gfx, const struct tm &t) override;
};
