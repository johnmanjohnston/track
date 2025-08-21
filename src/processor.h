#pragma once
#include "daw/track.h"
#include <JuceHeader.h>

class AudioPluginAudioProcessor : public juce::AudioProcessor {
  public:
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;

    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String &newName) override;

    juce::XmlElement *serializeNode(track::audioNode *node);
    void deserializeNode(juce::XmlElement *nodeElement, track::audioNode *node);
    void getStateInformation(juce::MemoryBlock &destData) override;
    void setStateInformation(const void *data, int sizeInBytes) override;

    void reset() override;

    int maxSamplesPerBlock = -1;

    std::vector<track::audioNode> tracks;
    bool soloMode = false;

    juce::AudioProcessorValueTreeState apvts;

    void updateLatency();
    void updateLatencyAfterDelay();

  private:
    juce::Random random;

    juce::AudioFormatManager afm;
    juce::AudioBuffer<float> fileBuffer;
    int startSample = 88200;

    bool prepared = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessor)
};
