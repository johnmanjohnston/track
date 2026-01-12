#pragma once
#include "track.h"

namespace track::utility {
// nodes
bool isDescendant(audioNode *parent, audioNode *possibleChild,
                  bool directDescandant);

int getIndexOfClip(audioNode *node, clip *c);
int getIndexOfClipByValue(audioNode *node, clip c);
audioNode *getNodeFromRoute(std::vector<int> route, void *p);
audioNode *getParentFromRoute(std::vector<int> route, void *p);
audioNode *findDuplicate(audioNode *parent, audioNode *src);
void copyNode(audioNode *dest, audioNode *src, void *processor);
void getTrivialNodeData(TrivialNodeData *dest, audioNode *src);
void writeTrivialNodeDataToNode(audioNode *dest, TrivialNodeData src);
void moveNodeToGroup(std::vector<int> moveRoute, std::vector<int> groupRoute,
                     Tracklist *tl, void *p);

void reorderNode(std::vector<int> r1, std::vector<int> r2,
                 std::vector<int> route, int r1End, int displayNodes, void *p);

void reorderNodeAlt(std::vector<int> r1, std::vector<int> r2, void *p);
bool isSibling(std::vector<int> r1, std::vector<int> r2);
void deleteNode(std::vector<int> route, void *p);
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

void clearSubwindows(void *e);

juce::String prettyVector(std::vector<int> x);
bool clipsEqual(track::clip x, track::clip y);
int snapSample(int sample, int division, int offset = 0);
} // namespace track::utility
