#pragma once

#include <Arduino.h>

namespace WeatherService {

struct Snapshot {
  bool valid = false;
  float tempC = 0;
  float tempMinC = 0;
  float tempMaxC = 0;
  bool hasRange = false;
  int humidity = 0;
  int weatherCode = 0;  // WMO
  uint32_t updatedMs = 0;
};

void begin();
bool poll();       // periodic Open-Meteo fetch; true if snapshot updated
void refreshNow(); // force fetch now
Snapshot get();
const char *conditionLabel(int weatherCode);

}  // namespace WeatherService
