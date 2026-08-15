#include "TimeService.h"

#include <sys/time.h>
#include <string.h>
#include <WiFi.h>

#include "config.h"
#include "wifi/WifiProvision.h"

namespace TimeService {

static Source gSource = Source::Soft;
static uint32_t gSoftBootMs = 0;
static WifiProvision::StatusFn gStatus = nullptr;

static void softNow(struct tm &out) {
  const uint32_t elapsed = (millis() - gSoftBootMs) / 1000UL;
  const uint32_t total = (uint32_t)SOFT_START_H * 3600UL + (uint32_t)SOFT_START_M * 60UL +
                         (uint32_t)SOFT_START_S + elapsed;
  const uint32_t tod = total % 86400UL;

  memset(&out, 0, sizeof(out));
  out.tm_year = 126;  // 2026
  out.tm_mon = 7;     // August
  out.tm_mday = 15;
  out.tm_hour = (int)(tod / 3600UL);
  out.tm_min = (int)((tod % 3600UL) / 60UL);
  out.tm_sec = (int)(tod % 60UL);
  out.tm_wday = 6;
}

static bool syncNtp() {
  configTime(NTP_TZ_OFFSET_SEC, 0, NTP_SERVER_1, NTP_SERVER_2);
  struct tm t;
  for (int i = 0; i < 40; i++) {
    if (getLocalTime(&t, 500)) {
      Serial.printf("NTP OK %04d-%02d-%02d %02d:%02d:%02d\n", t.tm_year + 1900, t.tm_mon + 1,
                    t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
      return true;
    }
  }
  Serial.println("NTP timeout");
  return false;
}

void begin(WifiProvision::StatusFn status) {
  gSoftBootMs = millis();
  gSource = Source::Soft;
  gStatus = status;

  const bool wifiOk = WifiProvision::ensureConnected(status);
  if (!wifiOk) {
    Serial.println("No Wi-Fi — soft/manual clock. Serial: s=skip  t=YYYY-MM-DD HH:MM:SS");
    return;
  }

  Serial.printf("RSSI=%d dBm\n", WiFi.RSSI());
  if (syncNtp()) {
    gSource = Source::Ntp;
  }
}

void requestReprovision() { WifiProvision::forceReprovision(gStatus); }

bool setManualTime(int year, int month, int day, int hour, int min, int sec) {
  struct tm t{};
  t.tm_year = year - 1900;
  t.tm_mon = month - 1;
  t.tm_mday = day;
  t.tm_hour = hour;
  t.tm_min = min;
  t.tm_sec = sec;
  const time_t epoch = mktime(&t);
  if (epoch < 0) {
    return false;
  }
  struct timeval tv = {.tv_sec = epoch, .tv_usec = 0};
  settimeofday(&tv, nullptr);
  gSource = Source::Manual;
  Serial.printf("Manual time %04d-%02d-%02d %02d:%02d:%02d\n", year, month, day, hour, min, sec);
  return true;
}

bool now(struct tm &out) {
  if (gSource == Source::Ntp || gSource == Source::Manual) {
    if (getLocalTime(&out, 0)) {
      return true;
    }
  }
  softNow(out);
  return true;
}

Source source() { return gSource; }

const char *sourceName() {
  switch (gSource) {
    case Source::Ntp:
      return "NTP";
    case Source::Manual:
      return "Manual";
    default:
      return "Soft";
  }
}

}  // namespace TimeService
