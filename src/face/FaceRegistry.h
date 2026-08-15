#pragma once

#include <time.h>

#include "face/FaceId.h"
#include "face/IWatchFace.h"

IWatchFace *getFace(FaceId id);
