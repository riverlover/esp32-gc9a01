#pragma once

#include <Arduino_GFX_Library.h>
#include <stdint.h>

#include "face/FaceId.h"

// Crown settings: long-press open; rotate scroll; short enter; long back; idle exit.
namespace Settings {

struct Hooks {
  FaceId (*getFace)();
  void (*setFace)(FaceId id);  // commit immediately
};

void begin(const Hooks &hooks);
bool active();
void open();
void close();

// Consume encoder events while active. Returns true if UI needs redraw.
bool handleInput(int8_t rot, bool shortPress, bool longPress);

void draw(Arduino_GFX *gfx);

}  // namespace Settings
