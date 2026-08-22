#include "WatchPrefs.h"

#include <Preferences.h>

#include "config.h"

namespace WatchPrefs {
namespace {

Preferences gPrefs;
bool gShowSeconds = true;
bool gReady = false;

#ifndef DEFAULT_SHOW_SECONDS
#define DEFAULT_SHOW_SECONDS 1
#endif

}  // namespace

void begin() {
  if (gReady) {
    return;
  }
  gPrefs.begin("watch", false);
  gShowSeconds = gPrefs.getBool("sec", DEFAULT_SHOW_SECONDS != 0);
  gReady = true;
  Serial.printf("Prefs: showSeconds=%d\n", gShowSeconds ? 1 : 0);
}

bool showSeconds() {
  if (!gReady) {
    begin();
  }
  return gShowSeconds;
}

void setShowSeconds(bool on) {
  if (!gReady) {
    begin();
  }
  if (gShowSeconds == on) {
    return;
  }
  gShowSeconds = on;
  gPrefs.putBool("sec", on);
  Serial.printf("Prefs: showSeconds -> %d\n", on ? 1 : 0);
}

}  // namespace WatchPrefs
