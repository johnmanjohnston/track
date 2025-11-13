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

void reorderNode(std::vector<int> r1, std::vector<int> r2,
                 std::vector<int> route, int r1End, int displayNodes, void *p);

void reorderNodeAlt(std::vector<int> r1, std::vector<int> r2, void *p);

void reorderPlugin(int srcIndex, int destIndex, audioNode *node);
} // namespace track::utility
