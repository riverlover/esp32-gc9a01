#pragma once

#include <stdint.h>

// EC11 rotary encoder: quadrature decode + button debounce.
// Call poll() often from loop(); then take* to consume events.
namespace Ec11 {

void begin();
void poll();

// Detents since last take (CW > 0, CCW < 0). Cleared on read.
int8_t takeRotation();

// One-shot events; cleared on read.
bool takeShortPress();
bool takeLongPress();

}  // namespace Ec11
