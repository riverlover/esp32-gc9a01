#pragma once

#include <Arduino_GFX_Library.h>
#include <time.h>

// One style = one implementation. Full redraw each second into Canvas (main flushes).
class IWatchFace {
 public:
  virtual ~IWatchFace() = default;
  virtual const char *name() const = 0;
  virtual void render(Arduino_GFX *gfx, const struct tm &t) = 0;
};
