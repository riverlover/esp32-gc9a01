#pragma once

#include "IWatchFace.h"

// Skeleton: openwork rings + hollow hands (decorative "movement").
class FaceSkeleton : public IWatchFace {
 public:
  const char *name() const override { return "Skeleton"; }
  void render(Arduino_GFX *gfx, const struct tm &t) override;
};
