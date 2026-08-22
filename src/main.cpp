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
#include "prefs/WatchPrefs.h"
#include "ui/Settings.h"
#include "weather/WeatherService.h"

Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, -1);
Arduino_GFX *display = new Arduino_GC9A01(bus, TFT_RST, 0 /* rotation */, true /* IPS */);
// Off-screen RGB565 buffer (~112KB). Faces draw here, then flush once — no black flash.
static Arduino_Canvas *canvas = nullptr;

static FaceId gFaceId = DEFAULT_FACE;  // currently shown (may be preview)
static FaceId gCommittedFace = DEFAULT_FACE;
static IWatchFace *gFace = nullptr;
static int lastDrawnKey = -1;  // sec, or hour*60+min when seconds off
static bool forceRedraw = true;
static int lastProvScreen = -1;
static bool gPreview = false;
static uint32_t gPreviewSinceMs = 0;
static bool gSettingsDirty = false;

static const char *gToast = nullptr;
static uint32_t gToastUntilMs = 0;

static constexpr uint32_t kPreviewAutoCommitMs = 4000;
static constexpr int kPreviewSize = 196;  // ~82% — floating shrink
static constexpr uint16_t kPreviewRing = 0xFFFF;
static constexpr uint16_t kPreviewRingDim = 0x8410;

// X remap for nearest-neighbor scale (src 240 → dst kPreviewSize).
static uint16_t gPreviewMapX[kPreviewSize];
static bool gPreviewMapReady = false;

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

static void clearToastRegion() {
  display->fillRect(20, 196, 200, 20, BLACK);
}

static void drawToastOverlay(Arduino_GFX *gfx) {
  if (!gToast || millis() > gToastUntilMs) {
    gToast = nullptr;
    return;
  }
  const int16_t tw = (int16_t)(strlen(gToast) * 6);
  const int16_t x = (int16_t)((240 - tw) / 2);
  const int16_t y = 200;
  gfx->fillRoundRect(x - 6, y - 4, tw + 12, 16, 4, 0x0000);
  gfx->drawRoundRect(x - 6, y - 4, tw + 12, 16, 4, 0xC618);
  gfx->setTextColor(0xFFFF);
  gfx->setTextSize(1);
  gfx->setCursor(x, y);
  gfx->print(gToast);
}

static void showToast(const char *msg, uint32_t ms = 1200) {
  gToast = msg;
  gToastUntilMs = millis() + ms;
  if (gPreview || Settings::active()) {
    drawToastOverlay(display);
  } else {
    forceRedraw = true;
  }
}

static void ensurePreviewMap() {
  if (gPreviewMapReady) {
    return;
  }
  for (int x = 0; x < kPreviewSize; ++x) {
    gPreviewMapX[x] = (uint16_t)(x * 240 / kPreviewSize);
  }
  gPreviewMapReady = true;
}

static void flushPreviewFloating() {
  if (!canvas) {
    return;
  }
  uint16_t *fb = canvas->getFramebuffer();
  if (!fb) {
    canvas->flush();
    return;
  }

  ensurePreviewMap();

  constexpr int src = 240;
  constexpr int dst = kPreviewSize;
  constexpr int ox = (240 - dst) / 2;
  constexpr int oy = (240 - dst) / 2;
  constexpr int cr = dst / 2;
  constexpr int cr2 = cr * cr;

  display->fillScreen(BLACK);

  auto *tft = static_cast<Arduino_TFT *>(display);
  uint16_t row[dst];

  // Nearest-neighbor shrink + circular mask (photo faces are square bitmaps).
  // Poll EC11 inside the SPI loop so detents aren't dropped during long flushes.
  tft->startWrite();
  tft->writeAddrWindow(ox, oy, dst, dst);
  for (int y = 0; y < dst; ++y) {
    if ((y & 15) == 0) {
      Ec11::poll();
    }
    const int dy = y - cr;
    const int dy2 = dy * dy;
    const uint16_t *srcRow = fb + (y * src / dst) * src;
    for (int x = 0; x < dst; ++x) {
      const int dx = x - cr;
      row[x] = (dx * dx + dy2 <= cr2) ? srcRow[gPreviewMapX[x]] : (uint16_t)0x0000;
    }
    tft->writePixels(row, dst);
  }
  tft->endWrite();

  const int16_t ringR = (int16_t)(cr + 2);
  display->drawCircle(120, 120, ringR, kPreviewRing);
  display->drawCircle(120, 120, ringR + 1, kPreviewRingDim);

  const char *name = gFace ? gFace->name() : "?";
  const int16_t nw = (int16_t)(strlen(name) * 6);
  display->setTextColor(0xFFFF);
  display->setTextSize(1);
  display->setCursor((240 - nw) / 2, 222);
  display->print(name);
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
    showToast("OK");
    Serial.printf("Face OK %s\n", faceName(gFaceId));
    return;
  }
  gPreview = false;
  gCommittedFace = gFaceId;
  forceRedraw = true;
  showToast(faceName(gFaceId));
  Serial.printf("Face confirm -> %s (%u)\n", faceName(gFaceId), static_cast<unsigned>(gFaceId));
}

static FaceId settingsGetFace() { return gCommittedFace; }

static void settingsSetFace(FaceId id) {
  selectFace(id, false);
  gPreview = false;
}

static void openSettings() {
  if (gPreview) {
    // Drop unsaved preview; keep last committed face.
    gPreview = false;
    selectFace(gCommittedFace, false);
  }
  gToast = nullptr;
  Settings::open();
  gSettingsDirty = true;
}

static void handleEncoder() {
  const int8_t rot = Ec11::takeRotation();
  const bool shortPress = Ec11::takeShortPress();
  const bool longPress = Ec11::takeLongPress();

  if (Settings::active()) {
    const bool wasOn = true;
    if (Settings::handleInput(rot, shortPress, longPress)) {
      gSettingsDirty = true;
    }
    if (wasOn && !Settings::active()) {
      forceRedraw = true;
      showToast("Watch");
    }
    return;
  }

  // Idle timeout / no-op poll for settings when closed — n/a

  int8_t r = rot;
  while (r > 0) {
    selectFace(nextFace(gFaceId), true);
    --r;
  }
  while (r < 0) {
    selectFace(prevFace(gFaceId), true);
    ++r;
  }

  if (shortPress) {
    if (gPreview) {
      commitFace();
    } else {
      showToast(faceName(gFaceId));
      Serial.printf("EC11 short @ %s\n", faceName(gFaceId));
    }
  }
  if (longPress) {
    openSettings();
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
  if (line.length() == 1 && line[0] >= '1' && line[0] <= '8') {
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
  if (line.equalsIgnoreCase("r")) {
    Serial.println("Weather refresh…");
    WeatherService::refreshNow();
    forceRedraw = true;
    return;
  }
  if (line.equalsIgnoreCase("e")) {
    Serial.printf("EC11 SW raw=%u activeLow=%d preview=%d settings=%d face=%s\n",
                  (unsigned)Ec11::swRawLevel(), Ec11::swActiveLow() ? 1 : 0, gPreview ? 1 : 0,
                  Settings::active() ? 1 : 0, faceName(gFaceId));
    return;
  }
  if (line.equalsIgnoreCase("h") || line == "?") {
    Serial.println("Keys: 1-8 faces | n next | r weather | p hotspot | t YYYY-MM-DD HH:MM:SS | e EC11");
    Serial.println("EC11: turn=preview | short=confirm | long=Settings");
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

  Ec11::poll();
  if (canvas) {
    gFace->render(canvas, t);
    Ec11::poll();
    if (gPreview) {
      flushPreviewFloating();
    } else {
      canvas->flush();
    }
    Ec11::poll();
  } else {
    gFace->render(display, t);
  }

  if (!gPreview && gToast && millis() <= gToastUntilMs) {
    drawToastOverlay(display);
  } else if (!gPreview) {
    gToast = nullptr;
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("GC9A01 modular watch");
  Serial.println("Setup: keep hotspot on, TAP ALLOW on phone");
  Serial.println("Or: s=skip Wi-Fi | t YYYY-MM-DD HH:MM:SS");
  Serial.println("EC11: turn=preview | short=confirm | long=Settings");

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

  ensureCanvas();

  Ec11::begin();
  WatchPrefs::begin();
  Settings::begin(Settings::Hooks{settingsGetFace, settingsSetFace});
  WeatherService::begin();
  selectFace(DEFAULT_FACE, false);

  struct tm t{};
  TimeService::now(t);
  redraw(t);
  lastDrawnKey = WatchPrefs::showSeconds() ? t.tm_sec : (t.tm_hour * 60 + t.tm_min);
  forceRedraw = false;

  Serial.printf("Watch OK [%s] %s %02d:%02d:%02d tz=UTC%+d\n", TimeService::sourceName(),
                gFace->name(), t.tm_hour, t.tm_min, t.tm_sec, TimeService::tzOffsetHours());
}

void loop() {
  handleSerial();
  Ec11::poll();
  handleEncoder();
  if (WeatherService::poll()) {
    forceRedraw = true;
  }

  // Settings UI: redraw only on input / open; still poll idle-exit via handleInput(0).
  if (Settings::active()) {
    if (Settings::handleInput(0, false, false)) {
      gSettingsDirty = true;
    }
    if (!Settings::active()) {
      forceRedraw = true;
    } else if (gSettingsDirty) {
      Settings::draw(display);
      gSettingsDirty = false;
    }
    delay(2);
    return;
  }

  if (gPreview) {
    const bool toastAlive = gToast && millis() <= gToastUntilMs;
    static bool toastWasAlive = false;
    if (toastWasAlive && !toastAlive) {
      clearToastRegion();
      gToast = nullptr;
    }
    toastWasAlive = toastAlive;

    if (forceRedraw) {
      struct tm t{};
      TimeService::now(t);
      redraw(t);
      forceRedraw = false;
    }
    delay(2);
    return;
  }

  struct tm t{};
  TimeService::now(t);

  const bool toastAlive = gToast && millis() <= gToastUntilMs;
  static bool toastWasAlive = false;
  if (toastWasAlive && !toastAlive) {
    forceRedraw = true;
  }
  toastWasAlive = toastAlive;

  if (!forceRedraw && !toastAlive) {
    const int key = WatchPrefs::showSeconds() ? t.tm_sec : (t.tm_hour * 60 + t.tm_min);
    if (key == lastDrawnKey) {
      delay(WatchPrefs::showSeconds() ? 2 : 50);
      return;
    }
  }

  redraw(t);
  lastDrawnKey = WatchPrefs::showSeconds() ? t.tm_sec : (t.tm_hour * 60 + t.tm_min);
  forceRedraw = false;
}
