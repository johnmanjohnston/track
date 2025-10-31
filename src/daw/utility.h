#pragma once
#include "track.h"

namespace track::utility {
int getIndexOfClip(audioNode *node, clip *clip);
// audioNode *getNodeFromRoute(std::vector<int> route, void *processor);
} // namespace track::utility
