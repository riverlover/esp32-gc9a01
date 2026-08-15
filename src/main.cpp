#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <math.h>

// Wiring:
// VCC->3.3  GND->G  SCL->6  SDA->7  DC->2  CS->10  RST->3
#define TFT_SCLK 6
#define TFT_MOSI 7
#define TFT_CS   10
#define TFT_DC   2
#define TFT_RST  3

static const int16_t CX = 120;
static const int16_t CY = 120;
static const int16_t FACE_R = 118;

// Classic demo pose looks good at ~10:08
static const int START_H = 10;
static const int START_M = 8;
static const int START_S = 0;

Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, -1);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RST, 0 /* rotation */, true /* IPS */);

static uint32_t bootMs = 0;
static int lastDrawnSecond = -1;

static void polar(int16_t r, float deg, int16_t &x, int16_t &y) {
  const float rad = deg * DEG_TO_RAD;
  x = CX + (int16_t)lroundf(r * sinf(rad));
  y = CY - (int16_t)lroundf(r * cosf(rad));
}

static void drawTick(float deg, int16_t rOuter, int16_t rInner, uint16_t color) {
  int16_t x0, y0, x1, y1;
  polar(rOuter, deg, x0, y0);
  polar(rInner, deg, x1, y1);
  gfx->drawLine(x0, y0, x1, y1, color);
}

static void drawTaperedHand(float deg, int16_t length, int16_t halfWidth, uint16_t color) {
  const float rad = deg * DEG_TO_RAD;
  const float px = sinf(rad);
  const float py = -cosf(rad);
  const float qx = cosf(rad);
  const float qy = sinf(rad);

  const int16_t tipX = CX + (int16_t)lroundf(length * px);
  const int16_t tipY = CY + (int16_t)lroundf(length * py);
  const int16_t baseX = CX - (int16_t)lroundf(12 * px);
  const int16_t baseY = CY - (int16_t)lroundf(12 * py);
  const int16_t leftX = baseX + (int16_t)lroundf(halfWidth * qx);
  const int16_t leftY = baseY + (int16_t)lroundf(halfWidth * qy);
  const int16_t rightX = baseX - (int16_t)lroundf(halfWidth * qx);
  const int16_t rightY = baseY - (int16_t)lroundf(halfWidth * qy);

  gfx->fillTriangle(tipX, tipY, leftX, leftY, rightX, rightY, color);
}

static void drawSecondHand(float deg, int16_t length, uint16_t color) {
  int16_t tipX, tipY, tailX, tailY;
  polar(length, deg, tipX, tipY);
  polar(22, deg + 180.0f, tailX, tailY);
  gfx->drawLine(CX, CY, tipX, tipY, color);
  gfx->drawLine(CX, CY, tipX + 1, tipY, color);
  gfx->drawLine(CX, CY, tipX, tipY + 1, color);
  gfx->drawLine(CX, CY, tailX, tailY, color);
}

static void drawHourMarker(int hour) {
  // Cardinal hours get larger Arabic numerals; others get bold ticks only.
  const float deg = hour * 30.0f;
  if (hour % 3 == 0) {
    int16_t x, y;
    polar(88, deg, x, y);
    char buf[3];
    snprintf(buf, sizeof(buf), "%d", hour == 0 ? 12 : hour);
    gfx->setTextColor(0xEF5D);  // warm off-white
    gfx->setTextSize(2);
    const int16_t tw = (int16_t)(strlen(buf) * 12);
    gfx->setCursor(x - tw / 2, y - 7);
    gfx->print(buf);
  }
}

static void drawFace() {
  gfx->fillScreen(BLACK);

  // Outer bezel ring
  gfx->drawCircle(CX, CY, FACE_R, 0x6B4D);
  gfx->drawCircle(CX, CY, FACE_R - 1, 0x8430);
  gfx->drawCircle(CX, CY, FACE_R - 2, 0x4A49);

  // Minute / hour ticks
  for (int i = 0; i < 60; i++) {
    const float deg = i * 6.0f;
    if (i % 5 == 0) {
      drawTick(deg, 112, 96, 0xEF5D);
    } else {
      drawTick(deg, 112, 104, 0x8410);
    }
  }

  for (int h = 0; h < 12; h++) {
    drawHourMarker(h);
  }

  // Brand / subtext
  gfx->setTextColor(0x8410);
  gfx->setTextSize(1);
  gfx->setCursor(CX - 18, CY - 42);
  gfx->print("QUARTZ");
  gfx->setCursor(CX - 24, CY + 34);
  gfx->print("ESP32-C3");
}

static void drawHands(int hours, int minutes, int seconds) {
  const float secDeg = seconds * 6.0f;
  const float minDeg = minutes * 6.0f + seconds * 0.1f;
  const float hourDeg = (hours % 12) * 30.0f + minutes * 0.5f;

  drawTaperedHand(hourDeg, 58, 5, 0xEF5D);
  drawTaperedHand(minDeg, 82, 4, 0xFFFF);
  drawSecondHand(secDeg, 92, 0xF800);  // red

  gfx->fillCircle(CX, CY, 6, 0xEF5D);
  gfx->fillCircle(CX, CY, 3, 0xF800);
}

static void getSoftClock(int &h, int &m, int &s) {
  const uint32_t elapsed = (millis() - bootMs) / 1000UL;
  const uint32_t total =
      (uint32_t)START_H * 3600UL + (uint32_t)START_M * 60UL + (uint32_t)START_S + elapsed;
  const uint32_t tod = total % 86400UL;
  h = (int)(tod / 3600UL);
  m = (int)((tod % 3600UL) / 60UL);
  s = (int)(tod % 60UL);
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("Analog watch starting...");

  if (!gfx->begin()) {
    Serial.println("Display init failed");
    while (true) {
      delay(1000);
    }
  }

  bootMs = millis();
  drawFace();

  int h, m, s;
  getSoftClock(h, m, s);
  drawHands(h, m, s);
  lastDrawnSecond = s;

  Serial.printf("Watch OK @ %02d:%02d:%02d\n", h, m, s);
}

void loop() {
  int h, m, s;
  getSoftClock(h, m, s);

  if (s == lastDrawnSecond) {
    delay(20);
    return;
  }

  // Full redraw keeps hands/ticks clean without a frame buffer.
  drawFace();
  drawHands(h, m, s);
  lastDrawnSecond = s;
}
