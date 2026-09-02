#pragma once

#include <stddef.h>
#include <stdint.h>

// microSD on shared SPI with GC9A01 (see pins.h).
namespace SdService {

struct Entry {
  char name[28];  // real basename (for path join)
  bool isDir;
  uint32_t size;
};

// Optional: restore TFT SPI / panel after SD uses SPIClass on the same bus.
using BusReclaimFn = void (*)();

void setBusReclaim(BusReclaimFn fn);

bool begin();  // mount once; call after display init (or lazily from Settings)
bool mounted();
const char *statusLine();  // short UI string

// Fill entry cache for path (default "/roms", fallback "/").
bool refresh(const char *path);

const char *currentPath();
int entryCount();
const Entry *entry(int i);

// Built-in font cannot draw CJK; map to ASCII/'?' into out.
void formatLabel(const Entry &e, char *out, size_t n);

}  // namespace SdService
