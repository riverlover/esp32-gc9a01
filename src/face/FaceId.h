#pragma once

#include <stdint.h>

enum class FaceId : uint8_t {
  Classic = 0,
  Lume = 1,
  Skeleton = 2,
  Calendar = 3,
  Photo = 4,
  Crown = 5,
  Dash = 6,
  Vista = 7,
  Count
};

inline const char *faceName(FaceId id) {
  switch (id) {
    case FaceId::Classic:
      return "Classic";
    case FaceId::Lume:
      return "Lume";
    case FaceId::Skeleton:
      return "Skeleton";
    case FaceId::Calendar:
      return "Calendar";
    case FaceId::Photo:
      return "Photo";
    case FaceId::Crown:
      return "Crown";
    case FaceId::Dash:
      return "Dash";
    case FaceId::Vista:
      return "Vista";
    default:
      return "?";
  }
}

inline FaceId nextFace(FaceId id) {
  const uint8_t n = (static_cast<uint8_t>(id) + 1) % static_cast<uint8_t>(FaceId::Count);
  return static_cast<FaceId>(n);
}

inline FaceId prevFace(FaceId id) {
  const uint8_t count = static_cast<uint8_t>(FaceId::Count);
  const uint8_t n = (static_cast<uint8_t>(id) + count - 1) % count;
  return static_cast<FaceId>(n);
}
