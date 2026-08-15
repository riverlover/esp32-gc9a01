#include <Arduino.h>
#include <Arduino_GFX_Library.h>

// Wiring (first recommendation):
// VCC->3.3  GND->G  SCL->6  SDA->7  DC->2  CS->10  RST->3
#define TFT_SCLK 6
#define TFT_MOSI 7
#define TFT_CS   10
#define TFT_DC   2
#define TFT_RST  3

Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, -1);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RST, 0 /* rotation */, true /* IPS */);

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("GC9A01 bring-up starting...");

  if (!gfx->begin()) {
    Serial.println("Display init failed");
    while (true) {
      delay(1000);
    }
  }

  gfx->fillScreen(BLACK);
  delay(200);

  // Color bars to verify the panel is alive
  gfx->fillScreen(RED);
  delay(400);
  gfx->fillScreen(GREEN);
  delay(400);
  gfx->fillScreen(BLUE);
  delay(400);
  gfx->fillScreen(BLACK);

  gfx->setTextColor(WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(54, 100);
  gfx->println("GC9A01");
  gfx->setCursor(46, 130);
  gfx->println("ESP32-C3");

  Serial.println("Display OK");
}

void loop() {
  static uint16_t colors[] = {RED, YELLOW, GREEN, CYAN, BLUE, MAGENTA, WHITE};
  static size_t i = 0;

  gfx->fillCircle(120, 120, 30, colors[i]);
  i = (i + 1) % (sizeof(colors) / sizeof(colors[0]));
  delay(500);
}
