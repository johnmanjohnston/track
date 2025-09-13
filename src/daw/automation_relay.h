#pragma once
#include "../processor.h"
#include "subwindow.h"
#include "track.h"
#include <JuceHeader.h>

namespace track {
class relayParam {
  public:
    relayParam() {}
    ~relayParam() {}

    juce::String pluginsParamID;
    int outputParamID = -1;

    float value = -1.f;

    float getValueUsingPercentage(float min, float max);
};

class RelayManagerComponent : public track::Subwindow {
  public:
    RelayManagerComponent();
    ~RelayManagerComponent();

    std::unique_ptr<track::subplugin> *getPlugin();
    audioNode *getCorrespondingTrack();
    AudioPluginAudioProcessor *procesor = nullptr;
    std::vector<int> route;
    int pluginIndex = -1;

    void paint(juce::Graphics &override);
};
} // namespace track
