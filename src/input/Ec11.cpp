#include "Ec11.h"

#include <Arduino.h>

#include "pins.h"

namespace Ec11 {
namespace {

constexpr uint32_t kBtnDebounceMs = 25;
constexpr uint32_t kLongPressMs = 700;
constexpr uint32_t kMinShortMs = 25;
// Full detent ≈ 4 gray-code steps on typical EC11.
constexpr int8_t kStepsPerDetent = 4;

// Gray-code transition table (prev<<2 | curr) → delta.
constexpr int8_t kQuadTable[16] = {
    0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0,
};

// Quadrature state — updated from ISR during long SPI flushes.
volatile uint8_t prevAb = 0;
volatile int8_t accum = 0;
volatile int8_t rotPending = 0;

bool activeLow = true;  // pressed drives pin to GND (most modules)
bool samplePressed = false;
bool stablePressed = false;
uint32_t edgeMs = 0;
uint32_t pressStartMs = 0;
bool longFired = false;
bool shortPending = false;
bool longPending = false;

uint8_t readAb() {
  const uint8_t a = (uint8_t)digitalRead(ENC_CLK);
  const uint8_t b = (uint8_t)digitalRead(ENC_DT);
  return (uint8_t)((a << 1) | b);
}

bool readPressed() {
  const int level = digitalRead(ENC_SW);
  return activeLow ? (level == LOW) : (level == HIGH);
}

void IRAM_ATTR onQuadIsr() {
  const uint8_t ab = (uint8_t)(((digitalRead(ENC_CLK) & 1) << 1) | (digitalRead(ENC_DT) & 1));
  if (ab == prevAb) {
    return;
  }
  const uint8_t idx = (uint8_t)((prevAb << 2) | ab);
  int8_t a = (int8_t)(accum + kQuadTable[idx]);
  prevAb = ab;

  while (a >= kStepsPerDetent) {
    a = (int8_t)(a - kStepsPerDetent);
    if (rotPending < 127) {
      rotPending = (int8_t)(rotPending + 1);
    }
  }
  while (a <= -kStepsPerDetent) {
    a = (int8_t)(a + kStepsPerDetent);
    if (rotPending > -127) {
      rotPending = (int8_t)(rotPending - 1);
    }
  }
  accum = a;
}

}  // namespace

void begin() {
  pinMode(ENC_CLK, INPUT_PULLUP);
  pinMode(ENC_DT, INPUT_PULLUP);
  pinMode(ENC_SW, INPUT_PULLUP);
  delay(5);

  // Idle should be released. If SW sits LOW at boot, treat as active-high module.
  uint8_t highN = 0;
  for (int i = 0; i < 16; ++i) {
    if (digitalRead(ENC_SW) == HIGH) {
      ++highN;
    }
    delay(2);
  }
  activeLow = (highN >= 8);

  noInterrupts();
  prevAb = readAb();
  accum = 0;
  rotPending = 0;
  interrupts();

  samplePressed = false;
  stablePressed = false;
  edgeMs = millis();
  pressStartMs = 0;
  longFired = false;
  shortPending = false;
  longPending = false;

  // Edge ISR so detents aren't lost while canvas/SPI flush blocks loop().
  attachInterrupt(digitalPinToInterrupt(ENC_CLK), onQuadIsr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_DT), onQuadIsr, CHANGE);

  Serial.printf("EC11 OK CLK=%d DT=%d SW=%d idle=%u activeLow=%d isr=1\n", ENC_CLK, ENC_DT, ENC_SW,
                (unsigned)digitalRead(ENC_SW), activeLow ? 1 : 0);
}

void poll() {
  // Button only — rotation is ISR-driven.
  const bool pressed = readPressed();
  const uint32_t now = millis();

  if (pressed != samplePressed) {
    samplePressed = pressed;
    edgeMs = now;
  }

  if ((now - edgeMs) < kBtnDebounceMs) {
    return;
  }

  if (pressed != stablePressed) {
    stablePressed = pressed;
    if (stablePressed) {
      pressStartMs = now;
      longFired = false;
      Serial.println("EC11 SW down");
    } else {
      const uint32_t held = now - pressStartMs;
      if (!longFired && held >= kMinShortMs) {
        shortPending = true;
        Serial.printf("EC11 SW short (%lums)\n", (unsigned long)held);
      } else {
        Serial.printf("EC11 SW up (%lums)\n", (unsigned long)held);
      }
      longFired = false;
    }
    return;
  }

  if (stablePressed && !longFired && (now - pressStartMs) >= kLongPressMs) {
    longFired = true;
    longPending = true;
    Serial.println("EC11 SW long");
  }
}

int8_t takeRotation() {
  noInterrupts();
  const int8_t d = rotPending;
  rotPending = 0;
  interrupts();
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

uint8_t swRawLevel() {
  return (uint8_t)digitalRead(ENC_SW);
}

bool swActiveLow() {
  return activeLow;
}

}  // namespace Ec11
