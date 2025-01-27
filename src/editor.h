#pragma once
#include "daw/timeline.h"
#include "daw/track.h"
#include "processor.h"

class AudioPluginAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        public juce::Timer {
  public:
    explicit AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor &);
    ~AudioPluginAudioProcessorEditor() override;

    void paint(juce::Graphics &) override;
    void resized() override;

  private:
    AudioPluginAudioProcessor &processorRef;

    // track::ClipComponent clipComponent;
    track::TimelineViewport timelineViewport;
    // track::TimelineComponent timelineComponent;

    void timerCallback() override { repaint(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        AudioPluginAudioProcessorEditor)
};
