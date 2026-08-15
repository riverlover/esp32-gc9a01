#include "FaceRegistry.h"

#include "FaceCalendar.h"
#include "FaceClassic.h"
#include "FaceLume.h"
#include "FaceSkeleton.h"

IWatchFace *getFace(FaceId id) {
  static FaceClassic classic;
  static FaceLume lume;
  static FaceSkeleton skeleton;
  static FaceCalendar calendar;

  switch (id) {
    case FaceId::Lume:
      return &lume;
    case FaceId::Skeleton:
      return &skeleton;
    case FaceId::Calendar:
      return &calendar;
    case FaceId::Classic:
    default:
      return &classic;
  }
}
