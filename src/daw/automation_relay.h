#pragma once
#include "../processor.h"
#include "juce_gui_basics/juce_gui_basics.h"
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

class RelayManagerNode : public juce::Component {
  public:
    RelayManagerNode() {}
    ~RelayManagerNode() {}

    void paint(juce::Graphics &g) override;
};

class RelayManagerNodesWrapper : public juce::Component {
  public:
    RelayManagerNodesWrapper() : juce::Component() {}
    ~RelayManagerNodesWrapper(){};

    std::vector<std::unique_ptr<RelayManagerNode>> relayNodes;
    void createRelayNodes();
};

class RelayManagerViewport : public juce::Viewport {
  public:
    RelayManagerViewport() : juce::Viewport() {}
    ~RelayManagerViewport() {}
};

class RelayManagerComponent : public track::Subwindow {
  public:
    RelayManagerComponent();
    ~RelayManagerComponent();

    std::unique_ptr<track::subplugin> *getPlugin();
    audioNode *getCorrespondingTrack();
    AudioPluginAudioProcessor *processor = nullptr;
    std::vector<int> route;
    int pluginIndex = -1;

    RelayManagerViewport rmViewport;
    RelayManagerNodesWrapper rmNodesWrapper;

    void paint(juce::Graphics &g) override;
    void resized() override;
};
} // namespace track
