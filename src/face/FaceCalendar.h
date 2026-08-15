#pragma once

#include "IWatchFace.h"

// Classic dial with date aperture at 3 o'clock.
class FaceCalendar : public IWatchFace {
 public:
  const char *name() const override { return "Calendar"; }
  void render(Arduino_GFX *gfx, const struct tm &t) override;
};
