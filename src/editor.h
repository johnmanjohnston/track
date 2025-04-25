#pragma once
#include "daw/playhead.h"
// #include "daw/plugin_chain.h"
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

    // track::PluginChainComponent pluginChainComponent;
    std::vector<std::unique_ptr<track::PluginChainComponent>>
        pluginChainComponents;
    void openFxChain(int trackIndex);

    std::vector<std::unique_ptr<track::PluginEditorWindow>> pluginEditorWindows;
    void openPluginEditorWindow(int trackIndex, int pluginIndex);

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
