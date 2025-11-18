#pragma once
#include "track.h"

namespace track::utility {
// nodes
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

// plugins
void reorderPlugin(int srcIndex, int destIndex, audioNode *node);
void closeOpenedEditors(std::vector<int> nodeRoute,
                        std::vector<track::subplugin *> *openedPlugins, void *p,
                        void *e);
void openEditors(std::vector<int> nodeRoute,
                 std::vector<track::subplugin *> openedPlugins, void *p,
                 void *e);

void closeOpenedRelayParamWindows(
    std::vector<int> nodeRoute, std::vector<track::subplugin *> *openedPlugins,
    void *p, void *e);
void openRelayParamWindows(std::vector<int> nodeRoute,
                           std::vector<track::subplugin *> openedPlugins,
                           void *p, void *e);
} // namespace track::utility
