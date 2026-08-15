#pragma once

#include "IWatchFace.h"

// Night lume: dark dial, green phosphorescent markers/hands.
class FaceLume : public IWatchFace {
 public:
  const char *name() const override { return "Lume"; }
  void render(Arduino_GFX *gfx, const struct tm &t) override;
};
