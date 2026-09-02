#include "SdService.h"

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <stdio.h>
#include <string.h>

#include "pins.h"

namespace SdService {
namespace {

constexpr int kMaxEntries = 24;

bool gBegun = false;
bool gOk = false;
Entry gEntries[kMaxEntries];
int gCount = 0;
char gPath[48] = "/";
char gStatus[40] = "SD —";
BusReclaimFn gReclaim = nullptr;

void holdTftIdle() {
  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
}

void reclaim() {
  // Always deselect card before giving the bus back to GFX.
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  if (gReclaim) {
    gReclaim();
  }
}

void copyBasename(const char *in, char *out, size_t n) {
  const char *base = strrchr(in, '/');
  base = base ? base + 1 : in;
  strncpy(out, base, n - 1);
  out[n - 1] = '\0';
}

}  // namespace

void setBusReclaim(BusReclaimFn fn) { gReclaim = fn; }

bool begin() {
  if (gBegun) {
    return gOk;
  }
  gBegun = true;

  holdTftIdle();
  // Do not bind hardware SS to SD_CS — keep CS as plain GPIO.
  SPI.begin(TFT_SCLK, SD_MISO, TFT_MOSI, -1);

  if (!SD.begin(SD_CS, SPI, 4000000)) {
    gOk = false;
    snprintf(gStatus, sizeof(gStatus), "SD fail");
    Serial.println("SD.begin FAILED");
    reclaim();
    return false;
  }

  if (SD.cardType() == CARD_NONE) {
    gOk = false;
    snprintf(gStatus, sizeof(gStatus), "No card");
    Serial.println("SD: no card");
    reclaim();
    return false;
  }

  gOk = true;
  const uint64_t mb = SD.cardSize() / (1024ULL * 1024ULL);
  snprintf(gStatus, sizeof(gStatus), "SD %lluMB", (unsigned long long)mb);
  Serial.printf("SD OK type=%u size=%lluMB CS=%d MISO=%d\n", (unsigned)SD.cardType(),
                (unsigned long long)mb, SD_CS, SD_MISO);
  reclaim();
  return true;
}

bool mounted() { return gOk; }

const char *statusLine() { return gStatus; }

bool refresh(const char *path) {
  gCount = 0;
  if (!gOk && !begin()) {
    return false;
  }

  holdTftIdle();

  const char *use = path && path[0] ? path : "/";
  if (strcmp(use, "/roms") == 0 && !SD.exists("/roms")) {
    use = "/";
  }

  strncpy(gPath, use, sizeof(gPath) - 1);
  gPath[sizeof(gPath) - 1] = '\0';

  File root = SD.open(gPath);
  if (!root || !root.isDirectory()) {
    snprintf(gStatus, sizeof(gStatus), "open fail");
    Serial.printf("SD open fail: %s\n", gPath);
    if (root) {
      root.close();
    }
    reclaim();
    return false;
  }

  Serial.printf("DIR %s\n", gPath);
  for (File f = root.openNextFile(); f && gCount < kMaxEntries; f = root.openNextFile()) {
    Entry &e = gEntries[gCount];
    copyBasename(f.name(), e.name, sizeof(e.name));
    e.isDir = f.isDirectory();
    e.size = e.isDir ? 0 : (uint32_t)f.size();
    Serial.printf("  %s %s", e.isDir ? "[D]" : "[F]", f.name());
    if (!e.isDir) {
      Serial.printf(" %u", (unsigned)e.size);
    }
    Serial.println();
    ++gCount;
    f.close();
  }
  root.close();

  if (gCount >= kMaxEntries) {
    Serial.println("  … truncated");
  }

  snprintf(gStatus, sizeof(gStatus), "%d items", gCount);
  reclaim();
  return true;
}

const char *currentPath() { return gPath; }

int entryCount() { return gCount; }

const Entry *entry(int i) {
  if (i < 0 || i >= gCount) {
    return nullptr;
  }
  return &gEntries[i];
}

void formatLabel(const Entry &e, char *out, size_t n) {
  char safe[28];
  size_t j = 0;
  for (size_t i = 0; e.name[i] && j + 1 < sizeof(safe); ++i) {
    const unsigned char c = (unsigned char)e.name[i];
    if (c >= 0x20 && c < 0x7F) {
      safe[j++] = (char)c;
    } else if ((c & 0xC0) != 0x80) {
      safe[j++] = '?';
    }
  }
  safe[j] = '\0';
  if (j == 0) {
    strncpy(safe, "?", sizeof(safe));
  }
  if (e.isDir) {
    snprintf(out, n, "[D] %s", safe);
  } else {
    snprintf(out, n, "%s", safe);
  }
  if (strlen(out) > 22 && n > 22) {
    out[22] = '\0';
  }
}

}  // namespace SdService
