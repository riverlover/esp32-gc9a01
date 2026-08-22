#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <string.h>
#include <time.h>

#include "config.h"
#include "face/FaceId.h"
#include "face/FaceRegistry.h"
#include "face/IWatchFace.h"
#include "input/Ec11.h"
#include "pins.h"
#include "time/TimeService.h"
#include "ui/ProvQr.h"

Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, -1);
Arduino_GFX *display = new Arduino_GC9A01(bus, TFT_RST, 0 /* rotation */, true /* IPS */);
// Off-screen RGB565 buffer (~112KB). Faces draw here, then flush once — no black flash.
static Arduino_Canvas *canvas = nullptr;

static FaceId gFaceId = DEFAULT_FACE;       // currently shown (may be preview)
static FaceId gCommittedFace = DEFAULT_FACE;
static IWatchFace *gFace = nullptr;
static int lastDrawnSecond = -1;
static bool forceRedraw = true;
static int lastProvScreen = -1;
static bool gPreview = false;
static uint32_t gPreviewSinceMs = 0;

static constexpr uint32_t kPreviewAutoCommitMs = 4000;

static void showStatus(const char *msg) {
  Serial.printf("[UI] %s\n", msg);
  if (lastProvScreen != 2) {
    drawPhoneHotspotHelp(display);
    lastProvScreen = 2;
  }
}

static bool ensureCanvas() {
  if (canvas) {
    return true;
  }
  canvas = new Arduino_Canvas(240, 240, display);
  // Display already begun; skip second begin on the panel.
  if (!canvas->begin(GFX_SKIP_OUTPUT_BEGIN)) {
    Serial.printf("Canvas OOM (free=%u) — direct draw, may flicker\n",
                  (unsigned)ESP.getFreeHeap());
    delete canvas;
    canvas = nullptr;
    return false;
  }
  Serial.printf("Canvas OK free=%u\n", (unsigned)ESP.getFreeHeap());
  return true;
}

static void selectFace(FaceId id, bool preview) {
  gFaceId = id;
  gFace = getFace(id);
  forceRedraw = true;
  gPreview = preview;
  if (preview) {
    gPreviewSinceMs = millis();
    Serial.printf("Face preview -> %s (%u)\n", faceName(id), static_cast<unsigned>(id));
  } else {
    gCommittedFace = id;
    Serial.printf("Face -> %s (%u)\n", faceName(id), static_cast<unsigned>(id));
  }
}

static void commitFace() {
  if (!gPreview && gFaceId == gCommittedFace) {
    Serial.printf("Face OK %s\n", faceName(gFaceId));
    return;
  }
  gPreview = false;
  gCommittedFace = gFaceId;
  forceRedraw = true;
  Serial.printf("Face confirm -> %s (%u)\n", faceName(gFaceId), static_cast<unsigned>(gFaceId));
}

static void handleEncoder() {
  int8_t rot = Ec11::takeRotation();
  while (rot > 0) {
    selectFace(nextFace(gFaceId), true);
    --rot;
  }
  while (rot < 0) {
    selectFace(prevFace(gFaceId), true);
    ++rot;
  }

  if (Ec11::takeShortPress()) {
    commitFace();
  }
  if (Ec11::takeLongPress()) {
    Serial.println("Long press: settings (TODO)");
  }

  if (gPreview && (millis() - gPreviewSinceMs) >= kPreviewAutoCommitMs) {
    commitFace();
  }
}

static void handleSerialLine(String line) {
  line.trim();
  if (line.length() == 0) {
    return;
  }
  if (line.equalsIgnoreCase("n")) {
    selectFace(nextFace(gFaceId), false);
    return;
  }
  if (line.length() == 1 && line[0] >= '1' && line[0] <= '5') {
    selectFace(static_cast<FaceId>(line[0] - '1'), false);
    return;
  }
  if (line.equalsIgnoreCase("p")) {
    Serial.println("Re-enter phone-hotspot wait...");
    lastProvScreen = -1;
    TimeService::requestReprovision();
    forceRedraw = true;
    return;
  }
  if (line.startsWith("t ") || line.startsWith("T ")) {
    // t 2026-08-15 21:20:00
    int y, mo, d, h, mi, s;
    if (sscanf(line.c_str() + 2, "%d-%d-%d %d:%d:%d", &y, &mo, &d, &h, &mi, &s) == 6) {
      if (TimeService::setManualTime(y, mo, d, h, mi, s)) {
        forceRedraw = true;
      }
    } else {
      Serial.println("Usage: t YYYY-MM-DD HH:MM:SS");
    }
    return;
  }
  if (line.equalsIgnoreCase("h") || line == "?") {
    Serial.println("Keys: 1-5 faces | n next | p hotspot | t YYYY-MM-DD HH:MM:SS");
    Serial.println("EC11: turn=preview face | short=confirm | long=settings(TODO)");
    Serial.println("Setup: w SSID PASS | s skip Wi-Fi");
  }
}

static void handleSerial() {
  static String buf;
  while (Serial.available() > 0) {
    const char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      handleSerialLine(buf);
      buf = "";
    } else {
      buf += c;
      if (buf.length() > 80) {
        buf = "";
      }
    }
  }
}

static void redraw(const struct tm &t) {
  if (!gFace) {
    return;
  }
  Arduino_GFX *target = canvas ? static_cast<Arduino_GFX *>(canvas) : display;
  gFace->render(target, t);
  if (canvas) {
    canvas->flush();
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("GC9A01 modular watch");
  Serial.println("Setup: keep hotspot on, TAP ALLOW on phone");
  Serial.println("Or: s=skip Wi-Fi | t YYYY-MM-DD HH:MM:SS");
  Serial.println("EC11: turn=preview | short=confirm | long=settings(TODO)");

  if (!display->begin()) {
    Serial.println("Display init failed");
    while (true) {
      delay(1000);
    }
  }

  display->fillScreen(BLACK);
  lastProvScreen = -1;
  showStatus("Connecting Wi-Fi...");
  TimeService::begin(showStatus);

  // Allocate frame buffer after Wi-Fi so STA stacks keep heap during connect.
  ensureCanvas();

  Ec11::begin();
  selectFace(DEFAULT_FACE, false);

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
  Ec11::poll();
  handleEncoder();

  struct tm t{};
  TimeService::now(t);

  if (!forceRedraw && t.tm_sec == lastDrawnSecond) {
    delay(5);  // keep encoder responsive
    return;
  }

  redraw(t);
  lastDrawnSecond = t.tm_sec;
  forceRedraw = false;
}
