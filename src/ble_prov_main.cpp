// Minimal WiFiProv BLE experiment for ESP32-C3 + official "ESP BLE Provisioning" app.
// Built only by env:ble-wifiprov-min — does not replace the watch / hotspot path.

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <WiFi.h>
#include <WiFiProv.h>
#include <esp_wifi.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "pins.h"
#include "vendor/ricmoo_qrcode/qrcode.h"

// Super Mini STA knobs. Do NOT call setSleep(WIFI_PS_NONE) while BLE is still up —
// IDF aborts: "Should enable WiFi modem sleep when both WiFi and Bluetooth are enabled".
static void applyStaRfWhileBle() {
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  wifi_country_t country = {};
  memcpy(country.cc, "CN", 2);
  country.schan = 1;
  country.nchan = 13;
  country.policy = WIFI_COUNTRY_POLICY_AUTO;
  esp_wifi_set_country(&country);
  esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
  esp_wifi_set_bandwidth(WIFI_IF_STA, WIFI_BW_HT20);
  Serial.println("RF: TX=8.5dBm HT20 CN (sleep left ON for BLE coexist)");
}

static void applyStaRfAfterBle() {
  WiFi.setSleep(WIFI_PS_NONE);
  Serial.println("RF: sleep=OFF (BLE released)");
}

Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, -1);
Arduino_GFX *display = new Arduino_GC9A01(bus, TFT_RST, 0 /* rotation */, true /* IPS */);

static volatile bool gGotIp = false;
static volatile bool gProvStarted = false;
static volatile bool gNeedSuccessDraw = false;
static volatile bool gNeedQrDraw = false;
static char gSsid[33] = {0};
static char gIp[16] = {0};
static bool gSuccessShown = false;

static bool ipLooksValid(const char *ip) {
  return ip && ip[0] && strcmp(ip, "0.0.0.0") != 0;
}

static bool wifiHasValidIp() {
  const IPAddress ip = WiFi.localIP();
  return WiFi.status() == WL_CONNECTED && ip != IPAddress(0, 0, 0, 0);
}

static void centerText(const char *t, int16_t y, uint16_t color = 0xEF5D) {
  if (!t) {
    return;
  }
  display->setTextColor(color);
  display->setTextSize(1);
  const int tw = (int)strlen(t) * 6;
  display->setCursor(120 - tw / 2, y);
  display->print(t);
}

static void screen(const char *line1, const char *line2 = nullptr, const char *line3 = nullptr) {
  display->fillScreen(BLACK);
  centerText("BLE WiFiProv", 48);
  centerText(line1, 88);
  centerText(line2, 108);
  centerText(line3, 128);
  char heap[32];
  snprintf(heap, sizeof(heap), "free=%u", (unsigned)ESP.getFreeHeap());
  centerText(heap, 160);
}

// Official App QR JSON (esp-idf-provisioning-android).
// NOTE: ricmoo QRCode does NOT reject oversized payloads (@TODO in qrcode.c) — wrong
// version yields a corrupt, unscannable code. Byte capacities (approx):
//   v3-L=55  v4-L=80  v5-L=108 / v5-M=86
// Our payload is ~81 bytes → need >= v5.
static void drawBleProvQr() {
  char payload[192];
  snprintf(payload, sizeof(payload),
           "{\"ver\":\"v1\",\"name\":\"%s\",\"pop\":\"%s\",\"transport\":\"ble\",\"security\":1}",
           PROV_SERVICE_NAME, PROV_POP);
  const size_t plen = strlen(payload);
  Serial.printf("BLE QR payload (%u bytes): %s\n", (unsigned)plen, payload);

  QRCode qr;
  uint8_t buf[qrcode_getBufferSize(5)];
  // v5-M holds 86 bytes; force it — do not fall back to smaller versions.
  const int rc = qrcode_initText(&qr, buf, 5, ECC_MEDIUM, payload);
  if (rc != 0 || qr.size == 0) {
    screen("QR build fail", PROV_SERVICE_NAME, "PoP " PROV_POP);
    return;
  }

  display->fillScreen(WHITE);

  // GC9A01 is round: largest axis-aligned square inside diameter-240 is ~169px.
  // Keep QR+quiet strictly inside that so finder patterns are not clipped.
  constexpr int kMaxBlock = 156;
  constexpr int kQuietMods = 2;  // white quiet zone in module units
  int scale = kMaxBlock / ((int)qr.size + 2 * kQuietMods);
  if (scale < 2) {
    scale = 2;
  }
  const int quiet = kQuietMods * scale;
  const int qrPx = (int)qr.size * scale;
  const int block = qrPx + 2 * quiet;
  const int x0 = (240 - block) / 2;
  const int y0 = (240 - block) / 2 - 6;  // slight up so labels fit in bottom chord

  display->fillRect(x0, y0, block, block, WHITE);
  for (uint8_t y = 0; y < qr.size; y++) {
    for (uint8_t x = 0; x < qr.size; x++) {
      if (qrcode_getModule(&qr, x, y)) {
        display->fillRect(x0 + quiet + x * scale, y0 + quiet + y * scale, scale, scale, BLACK);
      }
    }
  }

  display->setTextColor(BLACK);
  display->setTextSize(1);
  auto centerBlack = [](const char *t, int16_t y) {
    const int tw = (int)strlen(t) * 6;
    display->setCursor(120 - tw / 2, y);
    display->print(t);
  };
  // Labels in bottom chord — outside the QR square so they don't force QR larger.
  centerBlack(PROV_SERVICE_NAME, y0 + block + 2);
  char popLine[24];
  snprintf(popLine, sizeof(popLine), "PoP %s", PROV_POP);
  centerBlack(popLine, y0 + block + 16);
  Serial.printf("QR ok v%u modules=%u scale=%d block=%d @(%d,%d) (must be <=%d for round)\n",
                (unsigned)qr.version, (unsigned)qr.size, scale, block, x0, y0, kMaxBlock);
}

static void refreshStaBuffersFromWifi() {
  const String ssid = WiFi.SSID();
  if (ssid.length() > 0) {
    strncpy(gSsid, ssid.c_str(), sizeof(gSsid) - 1);
    gSsid[sizeof(gSsid) - 1] = '\0';
  }
  const IPAddress ip = WiFi.localIP();
  if (ip != IPAddress(0, 0, 0, 0)) {
    snprintf(gIp, sizeof(gIp), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
  }
}

static void showConnectedScreen() {
  refreshStaBuffersFromWifi();
  if (!ipLooksValid(gIp)) {
    Serial.println("Skip success screen: IP still 0.0.0.0");
    gSuccessShown = false;
    return;
  }
  char lineSsid[40];
  char lineIp[24];
  snprintf(lineSsid, sizeof(lineSsid), "SSID %s", gSsid[0] ? gSsid : "?");
  snprintf(lineIp, sizeof(lineIp), "IP %s", gIp);
  screen("Wi-Fi OK", lineSsid, lineIp);
  Serial.printf("Screen: %s | %s\n", lineSsid, lineIp);
  gSuccessShown = true;
}

static void onProvEvent(arduino_event_t *ev) {
  switch (ev->event_id) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP: {
      gGotIp = true;
      applyStaRfAfterBle();
      // Use Arduino IPAddress — same endianness as WiFi.localIP().
      const IPAddress ip(ev->event_info.got_ip.ip_info.ip.addr);
      snprintf(gIp, sizeof(gIp), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
      if (!ipLooksValid(gIp)) {
        // Fall back to WiFi.localIP() shortly in loop.
        gIp[0] = '\0';
      }
      if (WiFi.SSID().length() > 0) {
        strncpy(gSsid, WiFi.SSID().c_str(), sizeof(gSsid) - 1);
        gSsid[sizeof(gSsid) - 1] = '\0';
      }
      Serial.printf("GOT_IP event ip=%s ssid=\"%s\" local=%s\n", gIp[0] ? gIp : "(pending)",
                    gSsid, WiFi.localIP().toString().c_str());
      gNeedSuccessDraw = true;
      break;
    }
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.printf("STA disconnect reason=%u\n",
                    (unsigned)ev->event_info.wifi_sta_disconnected.reason);
      break;
    case ARDUINO_EVENT_PROV_START:
      gProvStarted = true;
      Serial.println("PROV_START — open ESP BLE Provisioning app");
      gNeedQrDraw = true;
      break;
    case ARDUINO_EVENT_PROV_CRED_RECV:
      strncpy(gSsid, (const char *)ev->event_info.prov_cred_recv.ssid, sizeof(gSsid) - 1);
      gSsid[sizeof(gSsid) - 1] = '\0';
      Serial.printf("Prov SSID=\"%s\"\n", gSsid);
      applyStaRfWhileBle();
      screen("Got SSID", gSsid, "connecting...");
      break;
    case ARDUINO_EVENT_PROV_CRED_FAIL:
      if (ev->event_info.prov_fail_reason == WIFI_PROV_STA_AUTH_ERROR) {
        Serial.println("PROV_CRED_FAIL AUTH (reason often=2 AUTH_EXPIRE on this board)");
        Serial.println("Retry: phone hotspot WatchESP/12345678, or check 2.4G password");
        screen("AUTH fail", "try WatchESP", "or check pass");
      } else {
        Serial.println("PROV_CRED_FAIL AP");
        screen("AP fail", "2.4G only", "retry in app");
      }
      break;
    case ARDUINO_EVENT_PROV_CRED_SUCCESS:
      Serial.println("PROV_CRED_SUCCESS");
      screen("Prov success", gSsid[0] ? gSsid : "SSID ok", "waiting IP...");
      break;
    case ARDUINO_EVENT_PROV_END:
      Serial.println("PROV_END");
      applyStaRfAfterBle();
      break;
    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(400);
  Serial.println();
  Serial.println("=== ble-wifiprov-min ===");
  Serial.printf("chip=%s heap=%u\n", ESP.getChipModel(), (unsigned)ESP.getFreeHeap());
  Serial.printf("service=%s pop=%s\n", PROV_SERVICE_NAME, PROV_POP);
  Serial.println("Phone: scan QR on screen, or Provision Device -> PROV_GC9A01 / PoP 12345678");

  if (!display->begin()) {
    Serial.println("Display init failed (continuing BLE anyway)");
  } else {
    screen("starting BLE...", nullptr, nullptr);
  }

  WiFi.onEvent(onProvEvent);

  uint8_t uuid[16] = {0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf,
                      0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02};

  Serial.printf("before beginProvision heap=%u\n", (unsigned)ESP.getFreeHeap());
  Serial.flush();

#if CONFIG_BLUEDROID_ENABLED
  WiFiProv.beginProvision(WIFI_PROV_SCHEME_BLE, WIFI_PROV_SCHEME_HANDLER_FREE_BTDM,
                          WIFI_PROV_SECURITY_1, PROV_POP, PROV_SERVICE_NAME, nullptr, uuid,
                          true /* reset_provisioned */);
  applyStaRfWhileBle();
  Serial.printf("after beginProvision heap=%u\n", (unsigned)ESP.getFreeHeap());
  // Prefer screen QR; also print URL for desktop debug.
  Serial.printf(
      "QR URL: https://espressif.github.io/esp-jumpstart/qrcode.html?data="
      "{\"ver\":\"v1\",\"name\":\"%s\",\"pop\":\"%s\",\"transport\":\"ble\"}\n",
      PROV_SERVICE_NAME, PROV_POP);
  gNeedQrDraw = true;
#else
  Serial.println("FATAL: CONFIG_BLUEDROID_ENABLED=0 — cannot BLE provision");
  screen("no Bluedroid", "build broken", nullptr);
#endif
}

void loop() {
  if (gNeedQrDraw && !gSuccessShown && !gGotIp) {
    gNeedQrDraw = false;
    drawBleProvQr();
  }

  if (gNeedSuccessDraw) {
    gNeedSuccessDraw = false;
    showConnectedScreen();
  }

  // Wait until DHCP really assigns a non-zero IP (avoids SSID ok + IP 0.0.0.0).
  if (!gSuccessShown && (gGotIp || wifiHasValidIp())) {
    refreshStaBuffersFromWifi();
    if (ipLooksValid(gIp)) {
      showConnectedScreen();
    }
  }

  static uint32_t lastHb = 0;
  if ((millis() - lastHb) > 5000) {
    lastHb = millis();
    Serial.printf("hb started=%d got_ip=%d wifi=%d ssid=%s ip=%s heap=%u\n",
                  gProvStarted ? 1 : 0, gGotIp ? 1 : 0, (int)WiFi.status(),
                  gSsid[0] ? gSsid : "-", gIp[0] ? gIp : "-", (unsigned)ESP.getFreeHeap());
  }
  delay(50);
}
