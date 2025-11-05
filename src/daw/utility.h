#pragma once
#include "track.h"

namespace track::utility {
int getIndexOfClip(audioNode *node, clip *clip);
audioNode *getNodeFromRoute(std::vector<int> route, void *p);
void copyNode(audioNode *dest, audioNode *src, void *processor);
} // namespace track::utility
