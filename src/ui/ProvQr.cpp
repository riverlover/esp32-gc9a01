#include "ProvQr.h"

#include <qrcode.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

static void drawQrPayload(Arduino_GFX *gfx, const char *payload, const char *title,
                          const char *hint1, const char *hint2) {
  Serial.printf("QR payload: %s\n", payload);

  QRCode qr;
  uint8_t buf[qrcode_getBufferSize(4)];
  if (qrcode_initText(&qr, buf, 4, ECC_MEDIUM, payload) != 0) {
    gfx->fillScreen(BLACK);
    gfx->setTextColor(RED);
    gfx->setCursor(40, 110);
    gfx->print("QR build fail");
    return;
  }

  gfx->fillScreen(WHITE);

  // Keep QR inside the round panel (corner finder patterns must not clip).
  // Circle r=120; use ~110px QR centered slightly above middle.
  int scale = 3;
  while (scale > 1 && (qr.size * scale) > 110) {
    scale--;
  }
  const int qrPx = qr.size * scale;
  const int x0 = (240 - qrPx) / 2;
  const int y0 = 36;

  // Quiet zone pad
  gfx->fillRect(x0 - 4, y0 - 4, qrPx + 8, qrPx + 8, WHITE);

  for (uint8_t y = 0; y < qr.size; y++) {
    for (uint8_t x = 0; x < qr.size; x++) {
      if (qrcode_getModule(&qr, x, y)) {
        gfx->fillRect(x0 + x * scale, y0 + y * scale, scale, scale, BLACK);
      }
    }
  }

  gfx->setTextColor(BLACK);
  gfx->setTextSize(1);
  auto center = [&](const char *t, int16_t y) {
    const int tw = (int)strlen(t) * 6;
    gfx->setCursor(120 - tw / 2, y);
    gfx->print(t);
  };
  center(title, 12);
  center(hint1, y0 + qrPx + 10);
  center(hint2, y0 + qrPx + 24);
}

void drawWifiJoinQr(Arduino_GFX *gfx) {
  char payload[96];
  // WPA QR — works better on Huawei than open/nopass SoftAP
  snprintf(payload, sizeof(payload), "WIFI:T:WPA;S:%s;P:%s;H:false;;", PROV_SERVICE_NAME,
           PROV_POP);
  char linePass[32];
  snprintf(linePass, sizeof(linePass), "pwd %s", PROV_POP);
  drawQrPayload(gfx, payload, "Scan join AP", PROV_SERVICE_NAME, linePass);
}

void drawPortalUrlQr(Arduino_GFX *gfx) {
  drawQrPayload(gfx, "http://192.168.4.1", "Scan open page", "http://192.168.4.1",
                "submit home WiFi");
}

void drawPhoneHotspotHelp(Arduino_GFX *gfx) {
  gfx->fillScreen(BLACK);
  gfx->setTextColor(0xEF5D);
  gfx->setTextSize(1);
  auto line = [&](const char *t, int16_t y) {
    const int tw = (int)strlen(t) * 6;
    gfx->setCursor(120 - tw / 2, y);
    gfx->print(t);
  };
  line("Huawei hotspot", 40);
  line("1) Hotspot ON", 68);
  char a[40];
  snprintf(a, sizeof(a), "2) %s", PHONE_HOTSPOT_SSID);
  line(a, 86);
  char b[40];
  snprintf(b, sizeof(b), "3) pwd %s", PHONE_HOTSPOT_PASS);
  line(b, 104);
  gfx->setTextColor(0xFFE0);  // yellow
  line("4) TAP ALLOW", 130);
  line("when phone asks", 146);
  gfx->setTextColor(0xEF5D);
  line("5) Wait Wi-Fi OK", 172);
}
