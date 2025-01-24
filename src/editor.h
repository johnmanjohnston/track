#pragma once
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

    void timerCallback() override { repaint(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        AudioPluginAudioProcessorEditor)
};
