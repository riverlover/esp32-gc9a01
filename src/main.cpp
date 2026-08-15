#include <Arduino.h>
#include <Arduino_GFX_Library.h>

#include "config.h"
#include "face/FaceId.h"
#include "face/FaceRegistry.h"
#include "face/IWatchFace.h"
#include "pins.h"
#include "time/TimeService.h"

Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, -1);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RST, 0 /* rotation */, true /* IPS */);

static FaceId gFaceId = DEFAULT_FACE;
static IWatchFace *gFace = nullptr;
static int lastDrawnSecond = -1;
static bool forceRedraw = true;

static void selectFace(FaceId id) {
  gFaceId = id;
  gFace = getFace(id);
  forceRedraw = true;
  Serial.printf("Face -> %s (%u)\n", faceName(id), static_cast<unsigned>(id));
}

static void handleSerial() {
  while (Serial.available() > 0) {
    const char c = (char)Serial.read();
    if (c == 'n' || c == 'N' || c == ' ') {
      selectFace(nextFace(gFaceId));
    } else if (c >= '1' && c <= '4') {
      selectFace(static_cast<FaceId>(c - '1'));
    } else if (c == 'h' || c == '?') {
      Serial.println("Keys: 1=Classic 2=Lume 3=Skeleton 4=Calendar  n=next");
    }
  }
}

static void redraw(const struct tm &t) {
  if (!gFace) {
    return;
  }
  gFace->render(gfx, t);
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("GC9A01 modular watch");
  Serial.println("Keys: 1=Classic 2=Lume 3=Skeleton 4=Calendar  n=next");

  if (!gfx->begin()) {
    Serial.println("Display init failed");
    while (true) {
      delay(1000);
    }
  }

  gfx->fillScreen(BLACK);
  TimeService::begin();
  selectFace(DEFAULT_FACE);

  struct tm t{};
  TimeService::now(t);
  redraw(t);
  lastDrawnSecond = t.tm_sec;
  forceRedraw = false;

  Serial.printf("Watch OK [%s] %s %02d:%02d:%02d\n", TimeService::sourceName(), gFace->name(),
                t.tm_hour, t.tm_min, t.tm_sec);
}

void loop() {
  handleSerial();

  struct tm t{};
  TimeService::now(t);

  if (!forceRedraw && t.tm_sec == lastDrawnSecond) {
    delay(20);
    return;
  }

  redraw(t);
  lastDrawnSecond = t.tm_sec;
  forceRedraw = false;
}
