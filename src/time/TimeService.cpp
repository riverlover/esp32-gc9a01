#include "TimeService.h"

#include <string.h>
#include <WiFi.h>

#include "config.h"

namespace TimeService {

static Source gSource = Source::Soft;
static uint32_t gSoftBootMs = 0;

static bool wifiConfigured() {
  return WIFI_SSID[0] != '\0' && strcmp(WIFI_SSID, "your-ssid") != 0;
}

static bool syncNtp() {
  Serial.printf("WiFi connecting to \"%s\"...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < WIFI_CONNECT_TIMEOUT_MS) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi failed; using soft clock");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    return false;
  }

  Serial.print("WiFi OK, IP=");
  Serial.println(WiFi.localIP());

  configTime(NTP_TZ_OFFSET_SEC, 0, NTP_SERVER_1, NTP_SERVER_2);

  struct tm t;
  for (int i = 0; i < 30; i++) {
    if (getLocalTime(&t, 500)) {
      Serial.printf("NTP OK %04d-%02d-%02d %02d:%02d:%02d\n", t.tm_year + 1900, t.tm_mon + 1,
                    t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
      return true;
    }
  }

  Serial.println("NTP timeout; using soft clock");
  return false;
}

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

void begin() {
  gSoftBootMs = millis();
  gSource = Source::Soft;

  if (!wifiConfigured()) {
    Serial.println("WIFI_SSID not set — copy config.local.h.example -> config.local.h");
    Serial.println("Using soft clock fallback");
    return;
  }

  if (syncNtp()) {
    gSource = Source::Ntp;
  }
}

bool now(struct tm &out) {
  if (gSource == Source::Ntp) {
    if (getLocalTime(&out, 0)) {
      return true;
    }
    // Transient failure: keep last soft progression rather than freeze
  }
  softNow(out);
  return true;
}

Source source() { return gSource; }

const char *sourceName() { return gSource == Source::Ntp ? "NTP" : "Soft"; }

}  // namespace TimeService
