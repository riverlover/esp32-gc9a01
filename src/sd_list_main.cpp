// Standalone SD directory listing (env:sd_list).
// Shared SPI with GC9A01: SCK=TFT_SCLK, MOSI=TFT_MOSI, MISO=SD_MISO, CS=SD_CS.

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>

#include "pins.h"

static void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
  Serial.printf("DIR %s\n", dirname);
  File root = fs.open(dirname);
  if (!root) {
    Serial.println("  (open failed)");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("  (not a directory)");
    root.close();
    return;
  }

  for (File f = root.openNextFile(); f; f = root.openNextFile()) {
    if (f.isDirectory()) {
      Serial.printf("  [D] %s\n", f.name());
      if (levels > 0) {
        // SD card file names may be absolute or relative depending on FS.
        char path[128];
        const char *n = f.name();
        if (n[0] == '/') {
          snprintf(path, sizeof(path), "%s", n);
        } else if (strcmp(dirname, "/") == 0) {
          snprintf(path, sizeof(path), "/%s", n);
        } else {
          snprintf(path, sizeof(path), "%s/%s", dirname, n);
        }
        listDir(fs, path, levels - 1);
      }
    } else {
      Serial.printf("  [F] %s  %u bytes\n", f.name(), (unsigned)f.size());
    }
    f.close();
  }
  root.close();
}

void setup() {
  Serial.begin(115200);
  delay(800);
  Serial.println();
  Serial.println("=== SD list (shared SPI) ===");
  Serial.printf("SCK=%d MOSI=%d MISO=%d CS=%d\n", TFT_SCLK, TFT_MOSI, SD_MISO, SD_CS);

  // Keep display off the bus while we talk to the card.
  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);

  SPI.begin(TFT_SCLK, SD_MISO, TFT_MOSI, SD_CS);

  if (!SD.begin(SD_CS, SPI, 4000000)) {
    Serial.println("SD.begin FAILED — check wiring / 3.3V / card format (FAT32)");
    return;
  }

  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card detected");
    return;
  }
  Serial.printf("Card type: %s\n",
                cardType == CARD_MMC    ? "MMC"
                : cardType == CARD_SD   ? "SDSC"
                : cardType == CARD_SDHC ? "SDHC"
                                        : "UNKNOWN");
  Serial.printf("Size: %llu MB\n", SD.cardSize() / (1024ULL * 1024ULL));

  listDir(SD, "/", 2);

  if (SD.exists("/roms") || SD.exists("roms")) {
    Serial.println("OK: found roms/");
  } else {
    Serial.println("WARN: roms/ not found at root");
  }
  Serial.println("=== done ===");
}

void loop() {
  delay(1000);
}
