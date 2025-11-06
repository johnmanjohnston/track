#pragma once
#include "track.h"

namespace track::utility {

bool isDescendant(audioNode *parent, audioNode *possibleChild,
                  bool directDescandant);

int getIndexOfClip(audioNode *node, clip *clip);
audioNode *getNodeFromRoute(std::vector<int> route, void *p);
void copyNode(audioNode *dest, audioNode *src, void *processor);
void moveNodeToGroup(std::vector<int> moveRoute, std::vector<int> groupRoute,
                     Tracklist *tl, void *p);
} // namespace track::utility
