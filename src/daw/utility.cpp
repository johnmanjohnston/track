#include "utility.h"
#include "../editor.h"
#include "automation_relay.h"
#include "defs.h"
#include "plugin_chain.h"
#include "track.h"

int track::utility::getIndexOfClip(audioNode *node, clip *c) {
    int retval = -1;

    for (size_t i = 0; i < node->clips.size(); ++i) {
        if (&node->clips[i] == c) {
            retval = i;
            break;
        }
    }

    return retval;
}

int track::utility::getIndexOfClipByValue(audioNode *node, clip c) {
    int retval = -1;

    for (size_t i = 0; i < node->clips.size(); ++i) {
        if (clipsEqual(node->clips[i], c)) {
            retval = i;
            break;
        }
    }

    return retval;
}

track::audioNode *track::utility::getParentFromRoute(std::vector<int> route,
                                                     void *p) {
    if (route.size() == 1)
        return nullptr;

    route.pop_back();
    return utility::getNodeFromRoute(route, p);
}

track::audioNode *track::utility::findDuplicate(audioNode *parent,
                                                audioNode *src) {
    for (auto &child : parent->childNodes) {
        if (child.trackName == src->trackName &&
            child.isTrack == src->isTrack &&
            child.clips.size() == src->clips.size()) {
            for (size_t i = 0; i < child.clips.size(); ++i) {
                if (child.clips[i].buffer != src->clips[i].buffer)
                    continue;
            }

            return &child;
        }
    }
    return nullptr;
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
    dest->stain = src->stain;

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

void track::utility::getTrivialNodeData(TrivialNodeData *dest, audioNode *src) {
    dest->trackName = src->trackName;
    dest->gain = src->gain;
    dest->m = src->m;
    dest->s = src->s;
    dest->pan = src->pan;
}

void track::utility::writeTrivialNodeDataToNode(audioNode *dest,
                                                TrivialNodeData src) {
    dest->trackName = src.trackName;
    dest->gain = src.gain;
    dest->m = src.m;
    dest->s = src.s;
    dest->pan = src.pan;
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

void track::utility::reorderNodeAlt(std::vector<int> r1, std::vector<int> r2,
                                    void *p) {

    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;
    audioNode *src = utility::getNodeFromRoute(r1, p);
    audioNode *adj = utility::getNodeFromRoute(r2, p);

    jassert(src != nullptr);
    jassert(adj != nullptr);

    audioNode tmp;
    utility::copyNode(&tmp, src, p);

    if (r2.size() == 1) {
        // invalid bounds check
        jassert((size_t)r2.back() < processor->tracks.size());

        audioNode &newNode =
            *processor->tracks.emplace(processor->tracks.begin() + r2.back());

        utility::copyNode(&newNode, &tmp, p);

        // delete node
        int deletionIndex = r1.back();

        if (deletionIndex > r2.back())
            ++deletionIndex;

        processor->tracks.erase(processor->tracks.begin() + deletionIndex);
    }

    // if nested and siblings
    if (r1.size() == r2.size() && r2.size() > 1) {
        std::vector<int> insertionParentRoute = r2;
        insertionParentRoute.resize(r2.size() - 1);

        audioNode *parent = utility::getNodeFromRoute(insertionParentRoute, p);

        // invalid bounds check
        jassert((size_t)r2.back() < parent->childNodes.size());

        audioNode &newNode =
            *parent->childNodes.emplace(parent->childNodes.begin() + r2.back());

        utility::copyNode(&newNode, &tmp, p);

        // delete node
        int deletionIndex = r1.back();

        if (deletionIndex > r2.back())
            ++deletionIndex;

        parent->childNodes.erase(parent->childNodes.begin() + deletionIndex);
    }
}

bool track::utility::isSibling(std::vector<int> r1, std::vector<int> r2) {
    r1.pop_back();
    r2.pop_back();
    return r1 == r2;
}

void track::utility::deleteNode(std::vector<int> route, void *p) {
    jassert(route.size() > 0);

    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;

    if (route.size() == 1) {
        processor->tracks.erase(processor->tracks.begin() + route.back());
        return;
    }

    audioNode *parent = utility::getParentFromRoute(route, p);
    parent->childNodes.erase(parent->childNodes.begin() + route.back());
}

std::vector<track::audioNode *> track::utility::getFlattenedNodes(void *p) {
    std::vector<track::audioNode *> retval;

    AudioPluginAudioProcessor *processor = (AudioPluginAudioProcessor *)p;

    for (size_t i = 0; i < processor->tracks.size(); ++i) {
        retval.push_back(&processor->tracks[i]);

        if (!processor->tracks[i].isTrack)
            traverseAndFlattenNodes(&retval, &processor->tracks[i], p);
    }

    return retval;
}

void track::utility::traverseAndFlattenNodes(std::vector<audioNode *> *vec,
                                             audioNode *parent, void *p) {
    for (size_t i = 0; i < parent->childNodes.size(); ++i) {
        vec->push_back(&parent->childNodes[i]);

        if (!parent->childNodes[i].isTrack)
            traverseAndFlattenNodes(vec, &parent->childNodes[i], p);
    }
}

void track::utility::reorderPlugin(int srcIndex, int destIndex,
                                   audioNode *node) {
    // std::move is absolute magic how have i not known of this sooner
    std::unique_ptr<track::subplugin> plugin =
        std::move(node->plugins[(size_t)srcIndex]);

    // remove plugin
    node->plugins.erase(node->plugins.begin() + srcIndex);

    // insert plugin and bypassed entry at intended indices
    node->plugins.insert(node->plugins.begin() + destIndex, std::move(plugin));
}

void track::utility::closeOpenedEditors(
    std::vector<int> nodeRoute, std::vector<track::subplugin *> *openedPlugins,
    void *p, void *e) {
    audioNode *node = utility::getNodeFromRoute(nodeRoute, p);
    AudioPluginAudioProcessorEditor *editor =
        (AudioPluginAudioProcessorEditor *)e;

    for (size_t i = 0; i < node->plugins.size(); ++i) {
        if (editor->isPluginEditorWindowOpen(nodeRoute, i)) {
            editor->closePluginEditorWindow(nodeRoute, i);
            openedPlugins->push_back(node->plugins[i].get());
        }
    }
}

void track::utility::openEditors(std::vector<int> nodeRoute,
                                 std::vector<track::subplugin *> openedPlugins,
                                 void *p, void *e) {
    audioNode *node = utility::getNodeFromRoute(nodeRoute, p);
    AudioPluginAudioProcessorEditor *editor =
        (AudioPluginAudioProcessorEditor *)e;

    // reopen closed ediors
    for (auto *pl : openedPlugins) {
        for (size_t i = 0; i < node->plugins.size(); ++i) {
            if (node->plugins[i].get() == pl) {
                editor->openPluginEditorWindow(nodeRoute, i);
                break;
            }
        }
    }
}

void track::utility::closeOpenedRelayParamWindows(
    std::vector<int> nodeRoute, std::vector<track::subplugin *> *openedPlugins,
    void *p, void *e) {
    audioNode *node = utility::getNodeFromRoute(nodeRoute, p);
    AudioPluginAudioProcessorEditor *editor =
        (AudioPluginAudioProcessorEditor *)e;

    DBG("");
    DBG("editor relay menu count = " << editor->relayManagerCompnoents.size());
    DBG("");

    for (size_t i = 0; i < node->plugins.size(); ++i) {
        if (editor->isRelayMenuOpened(nodeRoute, i)) {
            editor->closeAllRelayMenusWithRouteAndPluginIndex(nodeRoute, i);
            openedPlugins->push_back(node->plugins[i].get());
            continue;
        }
    }
}

void track::utility::openRelayParamWindows(
    std::vector<int> nodeRoute, std::vector<track::subplugin *> openedPlugins,
    void *p, void *e) {
    audioNode *node = utility::getNodeFromRoute(nodeRoute, p);
    AudioPluginAudioProcessorEditor *editor =
        (AudioPluginAudioProcessorEditor *)e;

    DBG("");
    DBG("editor relay menu count = " << editor->relayManagerCompnoents.size());
    DBG("");

    // reopen closed ediors
    for (auto *pl : openedPlugins) {
        for (size_t i = 0; i < node->plugins.size(); ++i) {
            if (node->plugins[i].get() == pl) {
                DBG("found match! plugin index " << i << " with name "
                                                 << pl->plugin->getName());
                editor->openRelayMenu(nodeRoute, i);
                break;
            }
        }
    }
}

void track::utility::clearSubwindows(void *e) {
    AudioPluginAudioProcessorEditor *editor =
        (AudioPluginAudioProcessorEditor *)e;

    editor->pluginChainComponents.clear();
    editor->pluginEditorWindows.clear();
    editor->relayManagerCompnoents.clear();
}

juce::String track::utility::prettyVector(std::vector<int> x) {
    juce::String retval = "[";

    for (size_t i = 0; i < x.size(); ++i) {
        retval += juce::String(x[i]);

        if (i != x.size() - 1) {
            retval += ", ";
        }
    }

    retval += "]";
    return retval;
}

bool track::utility::clipsEqual(track::clip x, track::clip y) {
    bool retval = true;

    if (x.buffer != y.buffer)
        retval = false;
    else if (!juce::approximatelyEqual(x.gain, y.gain))
        retval = false;
    else if (x.startPositionSample != y.startPositionSample)
        retval = false;
    else if (x.path != y.path)
        retval = false;
    else if (x.trimLeft != y.trimLeft)
        retval = false;
    else if (x.trimRight != y.trimRight)
        retval = false;

    return retval;
}

int track::utility::snapSample(int sample, int division, int offset) {
    if (division == 0)
        division = 1;

    double secondsPerBeat = 60.f / BPM;
    int samplesPerBar = (secondsPerBeat * SAMPLE_RATE) * 4; // for 4/4
    int samplesPerSnap = samplesPerBar / division;

    int snapped = (((sample + samplesPerSnap / 2) / samplesPerSnap) + offset) *
                  samplesPerSnap;

    return snapped;
}

std::vector<int> track::utility::rWithPopBack(std::vector<int> r) {
    std::vector<int> retval = r;
    retval.pop_back();
    return retval;
}

std::vector<int> track::utility::rWithSize(std::vector<int> r, size_t s) {
    std::vector<int> retval = r;
    retval.resize(s);
    return retval;
}

void track::utility::setAutoGrid() {
    if (track::AUTO_GRID) {
        // set grid depending on UI zoom muls

        DBG("uiz mul = " << UI_ZOOM_MULTIPLIER);

        int snap = UI_ZOOM_MULTIPLIER / 10;

        // round to nearest power of 2
        snap--;
        snap |= snap >> 1;
        snap |= snap >> 2;
        snap |= snap >> 4;
        snap |= snap >> 8;
        snap |= snap >> 16;
        snap++;

        DBG("snap = " << snap);
        DBG("");

        SNAP_DIVISION = snap;
    }
}

void track::utility::gloss(juce::Graphics &g, juce::Rectangle<float> b,
                           juce::Colour c, float cornerSize = 0.f) {
    juce::LookAndFeel_V2::drawGlassLozenge(g, b.getX(), b.getY(), b.getWidth(),
                                           b.getHeight(), c, 0.f, cornerSize,
                                           true, true, false, true);
}
void track::utility::gloss(juce::Graphics &g, juce::Rectangle<int> b,
                           juce::Colour c, float cornerSize = 0.f) {
    track::utility::gloss(g, b.toFloat(), c, cornerSize);
}

void track::utility::customDrawGlassPointer(juce::Graphics &g, const float x,
                                            const float y, const float diameter,
                                            const juce::Colour &colour,
                                            const float outlineThickness,
                                            const int direction) {
    if (diameter <= outlineThickness)
        return;

    Path p;
    p.startNewSubPath(x + diameter * 0.5f, y);
    p.lineTo(x + diameter, y + diameter * 0.6f);
    p.lineTo(x + diameter, y + diameter);
    p.lineTo(x, y + diameter);
    p.lineTo(x, y + diameter * 0.6f);
    p.closeSubPath();

    p.applyTransform(AffineTransform::rotation(
        (float)direction * MathConstants<float>::halfPi, x + diameter * 0.5f,
        y + diameter * 0.5f));

    {
        ColourGradient cg(Colours::white.withAlpha(0.4f).overlaidWith(
                              colour.withMultipliedAlpha(0.5f)),
                          0, y, Colours::white.darker().withAlpha(0.2f), 0,
                          y + diameter, false);

        // cg.addColour(0.4, Colours::white.overlaidWith(colour));

        g.setGradientFill(cg);
        g.fillPath(p);
    }

    // highlight
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.drawHorizontalLine(2, 0, diameter);

    ColourGradient cg(Colours::transparentBlack, x + diameter * 0.5f,
                      y + diameter * 0.5f,
                      Colours::black.withAlpha(0.5f * outlineThickness *
                                               colour.getFloatAlpha()),
                      x - diameter * 0.2f, y + diameter * 0.5f, true);

    cg.addColour(0.5, Colours::transparentBlack);
    cg.addColour(0.7, Colours::black.withAlpha(0.07f * outlineThickness));

    g.setGradientFill(cg);
    g.fillPath(p);

    g.setColour(Colours::black.withAlpha(0.5f * colour.getFloatAlpha()));
    g.strokePath(p, PathStrokeType(outlineThickness));
}
