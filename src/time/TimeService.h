#pragma once

#include <time.h>

#include <Arduino.h>

namespace WifiProvision {
using StatusFn = void (*)(const char *msg);
}

namespace TimeService {

enum class Source : uint8_t { Soft = 0, Ntp = 1, Manual = 2 };

void begin(WifiProvision::StatusFn status = nullptr);
void requestReprovision();
bool setManualTime(int year, int month, int day, int hour, int min, int sec);
bool now(struct tm &out);
Source source();
const char *sourceName();

}  // namespace TimeService
