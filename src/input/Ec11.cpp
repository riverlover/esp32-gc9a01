#include "Ec11.h"

#include <Arduino.h>

#include "pins.h"

namespace Ec11 {
namespace {

constexpr uint32_t kBtnDebounceMs = 30;
constexpr uint32_t kLongPressMs = 800;
// Full detent ≈ 4 gray-code steps on typical EC11.
constexpr int8_t kStepsPerDetent = 4;

// Gray-code transition table (prev<<2 | curr) → delta.
constexpr int8_t kQuadTable[16] = {
    0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0,
};

uint8_t prevAb = 0;
int8_t accum = 0;
int8_t rotPending = 0;

bool btnStable = true;  // released (pull-up high → not pressed)
bool btnRaw = true;
uint32_t btnEdgeMs = 0;
uint32_t pressStartMs = 0;
bool longFired = false;
bool shortPending = false;
bool longPending = false;

uint8_t readAb() {
  const uint8_t a = (uint8_t)digitalRead(ENC_CLK);
  const uint8_t b = (uint8_t)digitalRead(ENC_DT);
  return (uint8_t)((a << 1) | b);
}

}  // namespace

void begin() {
  pinMode(ENC_CLK, INPUT_PULLUP);
  pinMode(ENC_DT, INPUT_PULLUP);
  pinMode(ENC_SW, INPUT_PULLUP);

  prevAb = readAb();
  accum = 0;
  rotPending = 0;
  btnStable = true;
  btnRaw = true;
  btnEdgeMs = millis();
  pressStartMs = 0;
  longFired = false;
  shortPending = false;
  longPending = false;

  Serial.printf("EC11 OK CLK=%d DT=%d SW=%d\n", ENC_CLK, ENC_DT, ENC_SW);
}

void poll() {
  const uint8_t ab = readAb();
  if (ab != prevAb) {
    const uint8_t idx = (uint8_t)((prevAb << 2) | ab);
    accum = (int8_t)(accum + kQuadTable[idx]);
    prevAb = ab;

    while (accum >= kStepsPerDetent) {
      accum = (int8_t)(accum - kStepsPerDetent);
      if (rotPending < 127) {
        ++rotPending;
      }
    }
    while (accum <= -kStepsPerDetent) {
      accum = (int8_t)(accum + kStepsPerDetent);
      if (rotPending > -127) {
        --rotPending;
      }
    }
  }

  const bool raw = digitalRead(ENC_SW) == HIGH;  // HIGH = released
  const uint32_t now = millis();
  if (raw != btnRaw) {
    btnRaw = raw;
    btnEdgeMs = now;
  }
  if ((now - btnEdgeMs) < kBtnDebounceMs) {
    return;
  }
  if (raw == btnStable) {
    // Still held: may fire long press once.
    if (!btnStable && !longFired && (now - pressStartMs) >= kLongPressMs) {
      longFired = true;
      longPending = true;
    }
    return;
  }

  // Stable level changed.
  btnStable = raw;
  if (!btnStable) {
    // Pressed (active low).
    pressStartMs = now;
    longFired = false;
  } else {
    // Released.
    if (!longFired) {
      shortPending = true;
    }
    longFired = false;
  }
}

int8_t takeRotation() {
  const int8_t d = rotPending;
  rotPending = 0;
  return d;
}

bool takeShortPress() {
  if (!shortPending) {
    return false;
  }
  shortPending = false;
  return true;
}

bool takeLongPress() {
  if (!longPending) {
    return false;
  }
  longPending = false;
  return true;
}

}  // namespace Ec11
