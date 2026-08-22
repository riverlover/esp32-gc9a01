#pragma once

#include "IWatchFace.h"

class FaceDash : public IWatchFace {
 public:
  const char *name() const override { return "Dash"; }
  void render(Arduino_GFX *gfx, const struct tm &t) override;
};
