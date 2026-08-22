#include "FaceRegistry.h"

#include "FaceCalendar.h"
#include "FaceClassic.h"
#include "FaceCrown.h"
#include "FaceDash.h"
#include "FaceLume.h"
#include "FacePhoto.h"
#include "FaceSkeleton.h"
#include "FaceVista.h"

IWatchFace *getFace(FaceId id) {
  static FaceClassic classic;
  static FaceLume lume;
  static FaceSkeleton skeleton;
  static FaceCalendar calendar;
  static FacePhoto photo;
  static FaceCrown crown;
  static FaceDash dash;
  static FaceVista vista;

  switch (id) {
    case FaceId::Lume:
      return &lume;
    case FaceId::Skeleton:
      return &skeleton;
    case FaceId::Calendar:
      return &calendar;
    case FaceId::Photo:
      return &photo;
    case FaceId::Crown:
      return &crown;
    case FaceId::Dash:
      return &dash;
    case FaceId::Vista:
      return &vista;
    case FaceId::Classic:
    default:
      return &classic;
  }
}
