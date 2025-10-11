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

    int pluginParamIndex = -1;
    int outputParamID = -1;

    float percentage = -1.f;
};

class RelayManagerComponent;
class RelayManagerNode : public juce::Component {
  public:
    RelayManagerNode();
    ~RelayManagerNode();

    void paint(juce::Graphics &g) override;
    void resized() override;
    void createMenuEntries();
    int paramVectorIndex = -1;

    void mouseDown(const juce::MouseEvent &event) override;

    void removeThisRelayParam();

    juce::ComboBox hostedPluginParamSelector;
    juce::ComboBox relaySelector;

    juce::Array<juce::AudioProcessorParameter *> params;

    RelayManagerComponent *rmc = nullptr;

    static const juce::Font getInterBoldItalic() {
        static auto typeface = Typeface::createSystemTypefaceFor(
            BinaryData::Inter_18ptBoldItalic_ttf,
            BinaryData::Inter_18ptBoldItalic_ttfSize);

        return typeface;
    }
};

class RelayManagerNodesWrapper : public juce::Component {
  public:
    RelayManagerNodesWrapper() : juce::Component() {}
    ~RelayManagerNodesWrapper(){};

    void mouseDown(const juce::MouseEvent &event) override;

    std::vector<std::unique_ptr<RelayManagerNode>> relayNodes;
    void createRelayNodes();
    void setRelayNodesBounds();

    void removeRelayNode(int index);
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

class RelayParamInspector : public track::Subwindow {
  public:
    RelayParamInspector();
    ~RelayParamInspector();

    void paint(juce::Graphics &g) override;
    void resized() override;

    void initSliders();

    AudioPluginAudioProcessor *processor = nullptr;

    std::vector<juce::Slider> paramSliders;
    std::vector<juce::SliderParameterAttachment> paramSliderAttachments;
};
} // namespace track
