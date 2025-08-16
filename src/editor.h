#pragma once
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

class AudioPluginAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        public juce::Timer {
  public:
    explicit AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor &);
    ~AudioPluginAudioProcessorEditor() override;

    void paint(juce::Graphics &) override;
    void resized() override;

    // scanning
    juce::KnownPluginList knownPluginList;
    juce::AudioPluginFormatManager apfm;

    std::unique_ptr<juce::PluginListComponent> pluginListComponent;

    juce::PropertiesFile::Options options;
    std::unique_ptr<juce::PropertiesFile> propertiesFile =
        std::make_unique<juce::PropertiesFile>(options);
    void scan();

    juce::TextButton scanBtn;
    juce::TextButton configBtn;

    // track::PluginChainComponent pluginChainComponent;
    std::vector<std::unique_ptr<track::PluginChainComponent>>
        pluginChainComponents;
    void openFxChain(std::vector<int> route);

    std::vector<std::unique_ptr<track::PluginEditorWindow>> pluginEditorWindows;
    void openPluginEditorWindow(std::vector<int> route, int pluginIndex);

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
    juce::AudioProcessorValueTreeState::SliderAttachment masterSliderAttachment;

    void timerCallback() override {
        transportStatus.repaint();
        playhead.updateBounds();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        AudioPluginAudioProcessorEditor)
};
