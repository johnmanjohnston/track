#pragma once
#include "../processor.h"
#include "track.h"

namespace track::utility {
// nodes
bool isDescendant(audioNode *parent, audioNode *possibleChild,
                  bool directDescandant);

int getIndexOfClip(audioNode *node, clip *c);
int getIndexOfClipByValue(audioNode *node, clip c);
inline audioNode *getNodeFromRoute(std::vector<int> route, void *p) {
    if (route.size() == 0)
        return nullptr;

    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    audioNode *head = &processor->tracks[(size_t)route[0]];

    for (size_t i = 1; i < route.size(); ++i) {
        head = &head->childNodes[(size_t)route[i]];
    }

    return head;
}
audioNode *getParentFromRoute(std::vector<int> route, void *p);
audioNode *findDuplicate(audioNode *parent, audioNode *src);
void copyNode(audioNode *dest, audioNode *src, void *processor);
void getTrivialNodeData(TrivialNodeData *dest, audioNode *src);
void writeTrivialNodeDataToNode(audioNode *dest, TrivialNodeData src);

void reorderNodeAlt(std::vector<int> r1, std::vector<int> r2, void *p);
bool isSibling(std::vector<int> r1, std::vector<int> r2);
void deleteNode(std::vector<int> route, void *p);

std::vector<audioNode *> getFlattenedNodes(void *p);
void traverseAndFlattenNodes(std::vector<audioNode *> *vec, audioNode *parent,
                             void *p);

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

std::vector<int> rWithPopBack(std::vector<int> r);
std::vector<int> rWithSize(std::vector<int> r, size_t s);

void setAutoGrid();

// ui
void gloss(juce::Graphics &g, juce::Rectangle<int> b, juce::Colour c,
           float cornerSize);
void gloss(juce::Graphics &g, juce::Rectangle<float> b, juce::Colour c,
           float cornerSize);

void customDrawGlassPointer(juce::Graphics &g, const float x, const float y,
                            const float diameter, const juce::Colour &colour,
                            const float outlineThickness, const int direction);

} // namespace track::utility
