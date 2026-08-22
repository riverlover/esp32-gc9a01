#include "WeatherService.h"

#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "wifi/WifiProvision.h"

namespace WeatherService {

static Snapshot gSnap;
static uint32_t gLastAttemptMs = 0;
static bool gStarted = false;

const char *conditionLabel(int code) {
  if (code == 0) {
    return "Clear";
  }
  if (code <= 3) {
    return "Cloudy";
  }
  if (code == 45 || code == 48) {
    return "Fog";
  }
  if (code >= 51 && code <= 67) {
    return "Rain";
  }
  if (code >= 71 && code <= 77) {
    return "Snow";
  }
  if (code >= 80 && code <= 82) {
    return "Showers";
  }
  if (code >= 85 && code <= 86) {
    return "Snow";
  }
  if (code >= 95) {
    return "Storm";
  }
  return "—";
}

static bool parseAfter(const char *json, const char *key, float &out) {
  const char *p = strstr(json, key);
  if (!p) {
    return false;
  }
  p += strlen(key);
  while (*p == ' ' || *p == ':' || *p == '"') {
    ++p;
  }
  char *end = nullptr;
  const float v = strtof(p, &end);
  if (end == p) {
    return false;
  }
  out = v;
  return true;
}

static bool parseAfterInt(const char *json, const char *key, int &out) {
  float f = 0;
  if (!parseAfter(json, key, f)) {
    return false;
  }
  out = (int)lroundf(f);
  return true;
}

static bool fetchOnce() {
  if (!WifiProvision::isConnected()) {
    return false;
  }

  char url[192];
  snprintf(url, sizeof(url),
           "https://api.open-meteo.com/v1/forecast"
           "?latitude=%.4f&longitude=%.4f"
           "&current=temperature_2m,relative_humidity_2m,weather_code"
           "&timezone=auto",
           (double)WEATHER_LAT, (double)WEATHER_LON);

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(8000);
  if (!http.begin(client, url)) {
    Serial.println("Weather: begin fail");
    return false;
  }

  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("Weather: HTTP %d\n", code);
    http.end();
    return false;
  }

  String body = http.getString();
  http.end();

  const char *json = body.c_str();
  const char *current = strstr(json, "\"current\"");
  if (!current) {
    Serial.println("Weather: no current");
    return false;
  }

  float temp = 0;
  int humidity = 0;
  int wmo = 0;
  if (!parseAfter(current, "\"temperature_2m\"", temp) ||
      !parseAfterInt(current, "\"weather_code\"", wmo)) {
    Serial.println("Weather: parse fail");
    return false;
  }
  parseAfterInt(current, "\"relative_humidity_2m\"", humidity);

  gSnap.valid = true;
  gSnap.tempC = temp;
  gSnap.humidity = humidity;
  gSnap.weatherCode = wmo;
  gSnap.updatedMs = millis();
  Serial.printf("Weather OK %.1fC rh=%d code=%d (%s)\n", gSnap.tempC, gSnap.humidity,
                gSnap.weatherCode, conditionLabel(gSnap.weatherCode));
  return true;
}

void begin() {
  gStarted = true;
  gLastAttemptMs = 0;
  gSnap = Snapshot{};
}

void refreshNow() {
  gLastAttemptMs = 0;
  poll();
}

bool poll() {
  if (!gStarted) {
    return false;
  }
  const uint32_t now = millis();
  if (gLastAttemptMs != 0 && (now - gLastAttemptMs) < WEATHER_REFRESH_MS) {
    return false;
  }
  gLastAttemptMs = now;
  if (fetchOnce()) {
    return true;
  }
  if (!gSnap.valid) {
    Serial.println("Weather: waiting for next retry");
  }
  return false;
}

Snapshot get() { return gSnap; }

}  // namespace WeatherService
