#pragma once

#include <time.h>

#include <Arduino.h>

namespace TimeService {

enum class Source : uint8_t { Soft = 0, Ntp = 1 };

void begin();
bool now(struct tm &out);
Source source();
const char *sourceName();

}  // namespace TimeService
