#pragma once

#include <Arduino.h>

namespace WatchPrefs {

void begin();  // load from NVS

bool showSeconds();
void setShowSeconds(bool on);  // persists

}  // namespace WatchPrefs
