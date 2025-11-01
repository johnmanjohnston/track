#include "utility.h"
#include "../processor.h"

int track::utility::getIndexOfClip(audioNode *node, clip *clip) {
    int retval = -1;

    for (size_t i = 0; i < node->clips.size(); ++i) {
        if (&node->clips[i] == clip) {
            retval = i;
            break;
        }
    }

    return retval;
}

track::audioNode *track::utility::getNodeFromRoute(std::vector<int> route,
                                                   void *p) {
    if (route.size() == 0)
        return nullptr;

    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    audioNode *head = &processor->tracks[(size_t)route[0]];

    for (size_t i = 1; i < route.size(); ++i) {
        head = &head->childNodes[(size_t)route[i]];
    }

    return head;
}
