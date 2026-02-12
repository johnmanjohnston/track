#pragma once
#include "daw/automation_relay.h"
#include "daw/playhead.h"
#include "daw/timeline.h"
#include "daw/track.h"
#include "daw/transport_status.h"
#include "lookandfeel.h"
#include "processor.h"

namespace track {
class PluginChainComponent;
class PluginEditorWindow;
} // namespace track

class LatencyPoller : public juce::Timer {
  public:
    LatencyPoller(AudioPluginAudioProcessor *p, juce::AudioProcessorEditor *e) {
        this->processor = p;
        this->editor = e;
        startTimer(1500);
    }
    ~LatencyPoller() { stopTimer(); }

    void timerCallback() override { this->updateLastKnownLatency(); }

    void updateLastKnownLatency() {
        int newLatency = processor->getLatencySamples();

        if (newLatency != knownLatencySamples) {
            editor->repaint();
            this->knownLatencySamples = newLatency;
        }
    }

    AudioPluginAudioProcessor *processor = nullptr;
    juce::AudioProcessorEditor *editor = nullptr;
    int knownLatencySamples = -1;
};

class AudioPluginAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        public juce::Timer,
                                        public juce::ChangeListener {
  public:
    explicit AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor &);
    ~AudioPluginAudioProcessorEditor() override;

    void paint(juce::Graphics &) override;
    void resized() override;

    void changeListenerCallback(ChangeBroadcaster *source) override;

    // scanning
    juce::AudioPluginFormatManager apfm;

    std::unique_ptr<juce::PluginListComponent> pluginListComponent;

    juce::PropertiesFile::Options options;
    std::unique_ptr<juce::PropertiesFile> propertiesFile =
        std::make_unique<juce::PropertiesFile>(options);
    void scan();
    void lazyScan();

    std::unique_ptr<juce::FileChooser> fileChooser;

    juce::TextButton configBtn;
    LatencyPoller latencyPoller;

    std::vector<std::unique_ptr<track::PluginChainComponent>>
        pluginChainComponents;
    void openFxChain(std::vector<int> route);
    void closeAllFxChainsWithRoute(std::vector<int> route);

    std::vector<std::unique_ptr<track::RelayManagerComponent>>
        relayManagerCompnoents;
    void openRelayMenu(std::vector<int> route, int pluginIndex);
    void closeAllRelayMenusWithRouteAndPluginIndex(std::vector<int> route,
                                                   int pluginIndex);
    bool isRelayMenuOpened(std::vector<int> route, int pluginIndex);

    std::unique_ptr<track::RelayParamInspector> relayParamInspector;
    void openRelayParamInspector();

    std::vector<std::unique_ptr<track::PluginEditorWindow>> pluginEditorWindows;
    void openPluginEditorWindow(std::vector<int> route, int pluginIndex);
    void closePluginEditorWindow(std::vector<int> route, int pluginIndex);
    bool isPluginEditorWindowOpen(std::vector<int> route, int pluginIndex);

    std::vector<std::unique_ptr<track::ClipPropertiesWindow>>
        clipPropertiesWindows;
    void openClipPropertiesWindows(std::vector<int> route, int clipIndex);

    AudioPluginAudioProcessor &processorRef;

    track::TimelineViewport timelineViewport;

    track::Tracklist tracklist;
    track::TrackViewport trackViewport;
    track::TransportStatusComponent transportStatus;

    std::unique_ptr<track::TimelineComponent> timelineComponent =
        std::make_unique<track::TimelineComponent>();

    track::PlayheadComponent playhead;

    track::ui::CustomLookAndFeel lnf;

    juce::Slider masterSlider;
    juce::SliderParameterAttachment masterSliderAttachment;

    std::vector<track::subplugin *> tmpOpenedEditors;
    std::vector<track::subplugin *> tmpOpenedPCCs;
    std::vector<track::subplugin *> tmpOpenedRMCs;

    void timerCallback() override {
        transportStatus.repaint();
        playhead.updateBounds();
    }

    bool keyStateChanged(bool isKeyDown) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        AudioPluginAudioProcessorEditor)
};
