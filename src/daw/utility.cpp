#include "utility.h"
#include "../processor.h"
#include "automation_relay.h"

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

void track::utility::copyNode(audioNode *dest, audioNode *src,
                              void *processor) {
    dest->isTrack = src->isTrack;
    dest->trackName = src->trackName;
    dest->gain = src->gain;
    dest->processor = processor;
    dest->s = src->s;
    dest->m = src->m;
    dest->pan = src->pan;

    if (src->isTrack) {
        dest->clips = src->clips;
    } else {
        for (auto &child : src->childNodes) {
            auto &newChild = dest->childNodes.emplace_back();
            copyNode(&newChild, &child, processor);
        }
    }

    for (auto &p : src->plugins) {
        auto &pluginInstance = p->plugin;
        juce::MemoryBlock pluginData;
        pluginInstance->getStateInformation(pluginData);
        pluginInstance->releaseResources();

        juce::String identifier =
            pluginInstance->getPluginDescription().fileOrIdentifier;

        identifier = identifier.upToLastOccurrenceOf(".vst3", true, true);

        // add plugin to new node and copy data
        DBG("adding plugin to new node, using identifier " << identifier);
        dest->addPlugin(identifier);
        dest->plugins.back()->plugin->setStateInformation(pluginData.getData(),
                                                          pluginData.getSize());
        dest->plugins.back()->bypassed = p->bypassed;

        dest->plugins.back()->dryWetMix = p->dryWetMix;

        dest->plugins.back()->relayParams = p->relayParams;
    }
}

bool track::utility::isDescendant(audioNode *parent, audioNode *possibleChild,
                                  bool directDescandant) {
    for (audioNode &child : parent->childNodes) {
        if (directDescandant) {
            if (&child == possibleChild)
                return true;
            continue;
        }

        if (&child == possibleChild ||
            isDescendant(&child, possibleChild, false))
            return true;
    }

    return false;
}

void track::utility::moveNodeToGroup(std::vector<int> moveRoute,
                                     std::vector<int> groupRoute, Tracklist *tl,
                                     void *p) {
    audioNode *trackNode = getNodeFromRoute(moveRoute, p);
    audioNode *parentNode = getNodeFromRoute(groupRoute, p);

    if (parentNode->isTrack) {
        DBG("rejecting moving track inside a track");
        return;
    }

    bool x = isDescendant(parentNode, trackNode, true);
    if (x) {
        return;
    }

    bool y = isDescendant(trackNode, parentNode, false);
    if (y) {
        DBG("SECOND DESCENDANCY CHECK FAILED");
        return;
    }

    audioNode *newNode = &parentNode->childNodes.emplace_back();
    std::vector<int> routeCopy = moveRoute;

    // calling copyNode() should take care of all the gobbledygook
    copyNode(newNode, trackNode, p);

    // node is copied. now delete orginal node
    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;

    if (routeCopy.size() == 1) {
        DBG("deleting orphan node with index " << routeCopy[0]);
        for (auto &pluginInstance :
             processor->tracks[(size_t)routeCopy[0]].plugins) {
            AudioProcessorEditor *subpluginEditor =
                pluginInstance->plugin->getActiveEditor();
            if (subpluginEditor != nullptr) {
                pluginInstance->plugin->editorBeingDeleted(subpluginEditor);
            }
        }

        processor->tracks.erase(processor->tracks.begin() +
                                routeCopy[0]); // orphan
    } else {
        audioNode *head = &processor->tracks[(size_t)routeCopy[0]];
        for (size_t i = 1; i < routeCopy.size() - 1; ++i) {
            head = &head->childNodes[(size_t)routeCopy[i]];
        }

        head->childNodes.erase(head->childNodes.begin() +
                               routeCopy[routeCopy.size() - 1]);
    }
}
